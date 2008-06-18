/**\file 
 * 
 * phase170E.c   File to output phase and time left, and write 
 *		intersection message to database.
 *
 *
 * Copyright (c) 2006   Regents of the University of California
 */
 
#include "sys_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/time.h"
#include "signal.h"
#include "local.h"
#include "sys_rt.h"
#include "db_clt.h"
#include "db_utils.h"
#include "timestamp.h"

#include "atsc_clt_vars.h"
#include "ix_msg.h"	  //intersection message header file
#include "clt_vars.h" // TSP vatiable header file
#include "veh_sig.h"  // TSP structure header file
#include "int_cfg.h"  // intersection configuration header file
#include "phase170e.h"

static int sig_list[] = {
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGALRM,	// for timer
	ERROR
};
static jmp_buf exit_env;
static void sig_hand( int code )
{
	if( code == SIGALRM )
		return;
	else
		longjmp( exit_env, code );
}

// List of db variables that this process writes.
// Does not include DB_SIGNAL_STATUS_VAR and DB_SIGNAL_PRIORITY_REQUEST_VAR,
// which are created by ATSP
// Does not include TRAFFIC_SIGNAL, which is created by cicas_create
static db_id_t db_vars_list[] = {
	{DB_IX_MSG_VAR, sizeof(ix_msg_t)},
	{DB_IX_APPROACH1_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH2_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH3_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH4_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH5_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH6_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH7_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH8_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH9_VAR, MAX_APPROACH_SIZE},
	{DB_IX_APPROACH10_VAR, MAX_APPROACH_SIZE},
};

#define NUM_DB_VARS (sizeof(db_vars_list)/sizeof(db_id_t))

void do_usage(char *progname)
{
	fprintf(stderr, "%s usage:\n",progname);
	fprintf(stderr, "[-vVh] [-d domain] [-x xport] [-s site] [-i timer_interval]\n");
	fprintf(stderr, "\t-v: Verbose level 1. Print debug info for db read\n");
	fprintf(stderr, "\t-V: Verbose level 2. Print debug info for db write\n");
	fprintf(stderr, "\t    (default: no print).\n");
	fprintf(stderr, "\t-h: Print this message.\n");
	fprintf(stderr, "\t-d: Specify the domain.\n");
	fprintf(stderr, "\t-x: Specify the xport.\n");
	fprintf(stderr, "\t-s: Specify the site_id as shown in int_cfg.h.\n");	
	fprintf(stderr, "\t    (default: site_id = 1 --> rfs_intersection.\n");
	fprintf(stderr, "\t-S: Specify the signal_id in db for this site.\n");
	fprintf(stderr, "\t    (default: signal_id = 1.\n");		
	fprintf(stderr, "\t-i: Specify the timer interval in millisec.\n");
	fprintf(stderr, "\t    (default: 200 millisec interval.\n");		
}

int main(int argc, char *argv[])
{
	// db variables
	db_clt_typ *pclt;
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = DEFAULT_SERVICE;
	int xport = COMM_PSX_XPORT;
	trig_info_typ trig_info;	/* Trigger info */
	unsigned var = 0,typ = 0;
	db_data_typ db_data;
	posix_timer_typ *ptmr;		/* Timer info */	
	int option,recv_type;
	int trig_list[2],trig_nums =2;
	// control variables
	int site_id = 1, signal_db_id = 1; 
	int timer_interval = 200; 
	int verbose = 0, signal_trig = 0;
	// local variables
	float signal_state_onset[MAX_PLANS][MAX_PHASES][3]; // the defaul onsets
	float signal_EG_onset[MAX_PHASES][3]; // the active onsets for EG priority
	signal_trace_typ signal_trace;
	E170_timing_typ *pix_timing;       // pointer to intersection configurations
	signal_status_typ *psignal_status; // pointer to receive signal status data from database
	signal_priority_request_typ *pPRS;  // pointer to receive priority request data from database 
	ix_msg_t *pix = malloc (sizeof(ix_msg_t)); // pointer to ix_msg
	ix_approach_t *pappr; // pointer to approach array  
	// time structure
	struct timespec now;
	int pattern = 0,i,j;
	float cycle_len,f;
	double time_gap,fL,fM;
	
	// get argument inputs
	while ( (option = getopt( argc, argv, "d:x:s:S:i:hvV" )) != EOF )
	{
		switch( option )
		{
		case 'd':
			domain = strdup(optarg);
			break;
		case 'x':
			xport = atoi(optarg);
			break;
		case 's':
			site_id = atoi(optarg);
			break;
		case 'S':
			signal_db_id = atoi(optarg);
		case 'i':
			timer_interval = atoi(optarg);
			break;
		case 'v':
			verbose = 1; 
			break;
		case 'V':
			verbose = 2; 
			break;
		case 'h':
			// same as default case
		default:
			do_usage(argv[0]);
			exit(1);
			break;
		}
	}
	
	// validate argument inputs
	if ( !(site_id >= 1 && site_id <= MAX_SITES) )
	{
		fprintf(stderr,"%s: site_id must be between 1 and %d\n",argv[0],MAX_SITES);
		exit (EXIT_FAILURE);
	}
	if ( !(signal_db_id >= 1 && signal_db_id <= db_numssig) )
	{
		fprintf(stderr,"%s: siingal_id must be between 1 and %d\n",argv[0],db_numssig);
		exit (EXIT_FAILURE);
	}
	// The current version only deals with rfs_intersection.
	// Other sites can be added later
	site_id = 1;
	pix_timing = &signal_timing_cfg[site_id-1];	
	// get the onset of signal state change for each phase and control plan
	memset(signal_state_onset,0,sizeof(signal_state_onset));
	get_signal_change_onset(pix_timing, signal_state_onset);
	if (verbose != 0)
		// echo intersection configurations
		echo_cfg(pix_timing, signal_state_onset);	
	// allocate memory for pappr, as the total number of approaches for this site is already known 
	pappr = malloc(pix_timing->total_no_approaches * sizeof(ix_approach_t));	
	// setup db trigger list (SIGNAL_STATUS and PRIORITY_REQUEST)
	trig_list[0] = DB_SIGNAL_STATUS_VAR_BASE + signal_db_id;
	trig_list[1] = DB_SIGNAL_PRIORITY_REQUEST_VAR_BASE + signal_db_id;
	// initial
	memset(&signal_trace,0,sizeof(signal_trace_typ));	
	memset(signal_EG_onset,0,sizeof(signal_EG_onset));
	// Log in to the database (shared global memory).  Default to the current host. 
	get_local_name(hostname, MAXHOSTNAMELEN);
	if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			db_vars_list, NUM_DB_VARS, trig_list, trig_nums)) == NULL ) 
	{
		fprintf(stderr,"Database initialization error in %s\n",argv[0]);
		db_list_done( pclt, db_vars_list, NUM_DB_VARS, 
			trig_list, trig_nums );
		exit( EXIT_FAILURE );
	}
	// Initialize the timer. 
	if ((ptmr = timer_init(timer_interval, DB_CHANNEL(pclt))) == NULL)
	{
		fprintf(stderr,"timer_init failed in %s\n",argv[0]);
		db_list_done( pclt, db_vars_list, NUM_DB_VARS, 
			trig_list, trig_nums );
		exit( EXIT_FAILURE );
	}
	// Exit code on receiving signal 
	if (setjmp(exit_env) != 0) 
	{
		db_list_done(pclt, db_vars_list, NUM_DB_VARS,
			trig_list, trig_nums);
		exit( EXIT_SUCCESS );
	} else
		sig_ign( sig_list, sig_hand );
	
	// main loop	
	for( ;; )
	{
		// Now wait for a trigger
		recv_type = clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		if ( clock_gettime( CLOCK_REALTIME, &now ) != 0 )
		{
			fprintf(stderr,"%s clock_gettimg failed\n");
			continue;
		}			
		if (recv_type == DB_TIMER)
		{
			// time to provide signal state (head color)  and countdown
			if (signal_trig == 0)
				continue; // waite until received signal status update
			pattern = (int)signal_trace.signal.pattern;
			if ( pattern == 0 )
				continue; // do nothing if the signal is running free;
			time_gap = ts2sec(&now) - ts2sec(&signal_trace.signal.tp);
			if (time_gap > 5.0)
			{
				fprintf(stderr,"%s didn't receive signal status update for more than 5 sec\n");
				signal_trig = 0;
				continue;
			}
			// get the current local and master cycle closk
			cycle_len = pix_timing->plan_timing[pattern].cycle_length;
			fL  = signal_trace.signal.local_cycle_clock + time_gap;
			f = (float)fL;
			cycle_rounding(&f, cycle_len);
			signal_trace.active_local_cycle_clock = f;			
			fM  = signal_trace.signal.master_cycle_clock + time_gap;
			f = (float)fM;
			cycle_rounding(&f, cycle_len);
			signal_trace.active_master_cycle_clock = f;			
			// get timespec
			signal_trace.trace_time.tv_sec = now.tv_sec;
			signal_trace.trace_time.tv_nsec = now.tv_nsec;				
			// get the signal state (head color) for each permitted phase
			signal_trace.psignal_state = &signal_trace.signal.signal_state[0];
			// clear priority flag if it is the time to do so
			if ( signal_trace.priority_status == Green_Extension &&
				 signal_trace.signal.signal_state[3] == SIGNAL_STATE_GREEN )
			{
				// upon the finishing of GE execution, phase 4 starts with green
				signal_trace.priority_status = 0;
				if ( verbose != 0) printf("GE done\n");				
			}
			else if ( signal_trace.priority_status == Early_Green &&
				signal_trace.signal.signal_state[1] == SIGNAL_STATE_GREEN )					
			{
				// upon the finishing of EG execution, phase 2 starts with green
				signal_trace.priority_status = 0;
				if ( verbose != 0) printf("EG done\n");				
			}			
			// get the countdown time for each permitted phase			
			for (i=0;i<MAX_PHASES;i++)
			{
				if (pix_timing->phase_timing.permitted_phase[i] == 0)
					continue;
				if ( signal_trace.priority_status == Early_Green )
				{
					// the controller is providing EG priority,using  signal_EG_onset			
					signal_trace.pactive_onsets = &signal_EG_onset[i][0];
				}						
				else 
				{
					// the controller is either in normal mode or in providing GE, using the default onsets paramters 			
					signal_trace.pactive_onsets = &signal_state_onset[pattern][i][0];
				}
				switch ( *(signal_trace.psignal_state + i) )
				{
				case SIGNAL_STATE_GREEN:
					j = 1; // countdown w.r.t. yellow onset
					break;
				case SIGNAL_STATE_YELLOW:
					j =2; // countdown w.r.t red onset
					break;
				case SIGNAL_STATE_RED:
					j = 0; // countdown w.r.t green onset
					break;
				default:
					j = -1; // do not countdown for other signal state types
					break;
				}
				if (j == -1) break;					
				fL = *(signal_trace.pactive_onsets + j) - signal_trace.active_local_cycle_clock;
				if ( fL < -1.5) 
				{
					// this would happen when crossing the cycle end
					fL += cycle_len;
				}
				else if (fL < 0)
				{					
					// since the signal status is obtained by pulling and the data resolution is at second level, 
					// the latency could be as large as 1 sec. If it is the case, fix countdown time to 0
					fL = 0.0;
				}
				signal_trace.time2next[i] = (float)fL;
			}
			// write ix_msg to database
			assem_ix_msg(pix,pappr,pix_timing,&signal_trace,verbose,pclt,argv[0]);			
			continue;
		}		
		var = DB_TRIG_VAR(&trig_info);
		typ = DB_TRIG_TYPE(&trig_info);
		if (clt_read(pclt,var,typ,&db_data ) == FALSE )
		{
			fprintf(stderr,"%s clt_read ( %d ) faied.\n",argv[0],var);
			continue;
		}
		if (var == trig_list[0])
		{
			// received a signal status message trigger
			signal_trig = 1;
			psignal_status = (signal_status_typ *) db_data.value.user;
			signal_trace.signal.tp.tv_sec = now.tv_sec;
			signal_trace.signal.tp.tv_nsec = now.tv_nsec;			
			if ( psignal_status->master_cycle_clock != signal_trace.signal.master_cycle_clock)
			{
				// in case received duplicated messages, only update the tracer with the new message 
				update_signal_state(&signal_trace,psignal_status);
			}
			if ( verbose == 1 ) 
			{
				// print trigger info
				printf("signal: %02d:%02d:%02d:%03d, phase=%d%d, intv=%02d%02d, local=%d, pattern=%d\n",
					psignal_status->hour,psignal_status->min,
					psignal_status->sec,psignal_status->millisec,
					psignal_status->ringA_phase,psignal_status->ringB_phase,
					psignal_status->ringA_interval,psignal_status->ringB_interval,
					psignal_status->local_cycle_clock,psignal_status->pattern);
			}					
		}
		else if (var == trig_list[1])
		{
			// received a priority request message trigger
			pPRS = (signal_priority_request_typ *) db_data.value.user;
			signal_trace.prio.tp.tv_sec = now.tv_sec;
			signal_trace.prio.tp.tv_nsec = now.tv_nsec;			
			update_priority_state( &signal_trace,pPRS );
			if ( pPRS->requested_type == Early_Green)
			{
				// controller is executing EG, needs to update the signal onsets 
				memset(signal_EG_onset,0,sizeof(signal_EG_onset));
				pattern = (int)signal_trace.signal.pattern;
				cycle_len = pix_timing->plan_timing[pattern].cycle_length;				
				// EG changes the yellow onset and red onset for phase 4
				signal_EG_onset[3][0] = signal_state_onset[pattern][3][0];
				signal_EG_onset[3][1] = (float)pPRS->desired_force_off;
				signal_EG_onset[3][2] = signal_EG_onset[3][1] +
					pix_timing->phase_timing.yellow_intv[3];
				cycle_rounding(&signal_EG_onset[3][2], cycle_len);
				// EG changes the green onset for the synch phase (phase 2)
				signal_EG_onset[1][1] = signal_state_onset[pattern][1][1];
				signal_EG_onset[1][2] = signal_state_onset[pattern][1][2];
				signal_EG_onset[1][0] = signal_EG_onset[3][2] + 
					pix_timing->phase_timing.allred_intv[3];
				cycle_rounding(&signal_EG_onset[1][0], cycle_len);
			}
			else if (pPRS->requested_type == Green_Extension)
			{
				// controller is executing GE. signal onsets remain the same as the logic for executing GE
				// is to trick the local_cycle _clock
				// do nothing
			}				
			if ( verbose == 1)
			{
				// print trigger info
				printf("priority: %02d:%02d:%02d.%03d, bus=%d, type=%d, phase=%d, FO=%d\n",
					pPRS->hour,pPRS->min,pPRS->sec,pPRS->millisec,
					pPRS->requested_busID,pPRS->requested_type,
					pPRS->execution_phase,pPRS->desired_force_off);
			}		
		}
		else
		{
			fprintf(stderr,"%s received an unknown trigger (%d)\n",
				argv[0],var);
		}
	}
}
