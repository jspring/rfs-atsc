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

#include "alert_vars.h"	// CICAS version of IDS header file
#include "atsc.h"	// actuated traffic signal controller header file

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
	{DB_ATSC_BCAST_VAR, sizeof(atsc_typ)},
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
	fprintf(stderr, "[-tvh] [-d domain] [-x xport] [-s site] [-i timer_interval]\n");
	fprintf(stderr, "\t-t: Process and write the traffic signal data.\n");
	fprintf(stderr, "\t-v: Verbose. Print debug info for db read.\n");
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
	exit(-1);
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
	int trig_list[2];
	int trig_nums = 2;
	// control variables
	int site_id = 1;
	int signal_db_id = 1; 
	int timer_interval = 200; 
	int traffic_flag = 0;
	int verbose = 0;
	int signal_trig = 0;
	// local variables
	float signal_state_onset[MAX_PLANS][MAX_NMEA_PHAZ][3]; // the defaul onsets
	float signal_EG_onset[MAX_NMEA_PHAZ][3]; // the active onsets for EG priority
	signal_trace_typ signal_trace;
	E170_timing_typ *pix_timing;       // pointer to intersection configurations
	signal_status_typ *psignal_status; // pointer to receive signal status data from database
	signal_priority_request_typ *pPRS;  // pointer to receive priority request data from database 
	ix_msg_t *pix = malloc (sizeof(ix_msg_t)); // pointer to ix_msg
	ix_approach_t *pappr; // pointer to approach array  
	traffic_signal_typ traffic_signal;	// CICAS IDS info
	atsc_typ atsc;			// ATSC info	
	// time structure
	struct timespec now;
	int pattern = 0;
	float cycle_len,f;
	float time_left,time_used;
	double time_gap,fL,fM;
	int i,j,k,jj,kk;
	
	// get argument inputs
	while ( (option = getopt( argc, argv, "d:x:s:S:i:tvh" )) != EOF )
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
			break;
		case 'i':
			timer_interval = atoi(optarg);
			break;
		case 't':
			traffic_flag = 1;
			break;
		case 'v':
			verbose = 1; 
			break;
		case 'h':
			// same as default case
		default:
			do_usage(argv[0]);
			break;
		}
	}
	
	// validate argument inputs
	if ( !(site_id >= 1 && site_id <= MAX_SITES) )
	{
		fprintf(stderr,"%s: site_id must be between 1 and %d\n",argv[0],MAX_SITES);
		return (-1);
	}
	if ( !(signal_db_id >= 1 && signal_db_id <= db_numssig) )
	{
		fprintf(stderr,"%s: siingal_id must be between 1 and %d\n",argv[0],db_numssig);
		return (-1);
	}
	// The current version only deals with rfs_intersection.
	// Other sites can be added later
	site_id = 1;
	pix_timing = &signal_timing_cfg[site_id-1];	
	// get the onset of signal state change for each phase and control plan
	memset(signal_state_onset,0,sizeof(signal_state_onset));
	get_signal_change_onset(pix_timing, signal_state_onset);
	if (verbose == 1)
	{
		// echo intersection configurations
		echo_cfg(pix_timing, signal_state_onset);	
	}
	// allocate memory for pappr, as the total number of approaches for this site is already known 
	pappr = malloc(pix_timing->total_no_approaches * sizeof(ix_approach_t));	
	// setup db trigger list (SIGNAL_STATUS and PRIORITY_REQUEST)
	trig_list[0] = DB_SIGNAL_STATUS_VAR_BASE + signal_db_id;
	trig_list[1] = DB_SIGNAL_PRIORITY_REQUEST_VAR_BASE + signal_db_id;
	trig_nums = 2;	
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
		return (-1);
	}
	// Initialize the timer. 
	if ((ptmr = timer_init(timer_interval, DB_CHANNEL(pclt))) == NULL)
	{
		fprintf(stderr,"timer_init failed in %s\n",argv[0]);
		db_list_done( pclt, db_vars_list, NUM_DB_VARS, 
			trig_list, trig_nums );
		return (-1);
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
			fprintf(stderr,"%s clock_gettimg failed\n",argv[0]);
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
				fprintf(stderr,"%s didn't receive signal status update for more than 5 sec\n",argv[0]);
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
				signal_trace.bus_time_saved = 0;
				if ( verbose == 1) 
					printf("GE done\n");				
			}
			else if ( signal_trace.priority_status == Early_Green &&
				signal_trace.signal.signal_state[1] == SIGNAL_STATE_GREEN )					
			{
				// upon the finishing of EG execution, phase 2 starts with green
				signal_trace.priority_status = 0;
				signal_trace.bus_time_saved = 0;
				if ( verbose == 1) 
					printf("EG done\n");				
			}			
			// get the countdown time for each permitted phase			
			for (i=0;i<MAX_NMEA_PHAZ;i++)
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
					jj = 0; // count used time w.r.t green onset;
					break;
				case SIGNAL_STATE_YELLOW:
					j = 2; // countdown w.r.t red onset
					jj = 1; // count used time w.r.t green onset;
					break;
				case SIGNAL_STATE_RED:
					j = 0; // countdown w.r.t green onset
					jj = 2; // count used time w.r.t green onset;
					break;
				default:
					j = -1; // do not countdown for other signal state types
					break;
				}
				if (j == -1) break;					
				time_left = *(signal_trace.pactive_onsets + j) - signal_trace.active_local_cycle_clock;				
				if ( time_left < -1.5) 
				{
					// this would happen when crossing the cycle end
					time_left += cycle_len;
				}
				else if (time_left < 0.0)
				{					
					// since the signal status is obtained by pulling and the data resolution is at second level, 
					// the latency could be as large as 1 sec. If it is the case, fix countdown time to 0
					time_left = 0.0;
				}
				signal_trace.time2next[i] = time_left;				
				time_used = *(signal_trace.pactive_onsets + j) - *(signal_trace.pactive_onsets + jj)
					- time_left;
				if (time_used < -1.5)
					time_used += cycle_len;
				else if (time_used < 0.0)
					time_used = 0.0;
				signal_trace.timeused[i] = time_used;
			}
			// write ix_msg to database
			assem_ix_msg(pix,pappr,pix_timing,&signal_trace,pclt,argv[0]);			
			if (verbose == 1)
			{
				printf("ix_msg: ");
				print_timespec(stdout,&now);
				for (i=0;i<pix->num_approaches;i++) 
				{
					printf("%s %.1f ",
						ix_signal_state_string(pappr[i].signal_state),
						(float)pappr[i].time_to_next/10.0);
				}
				printf("%d %d %d\n",
					pix->bus_priority_calls,pix->reserved[0],pix->reserved[1]);
			}
			
			// write ATSC to database
			memset(&atsc,0,sizeof(atsc_typ));
			get_current_timestamp(&(atsc.ts));
			atsc.info_source = ATSC_SOURCE_NTCIP;
			for (i=0;i<pix->num_approaches;i++)
			{			
				j = pix_timing->approch[i].control_phase - 1; // signal phase that controls the approach
				k = pix_timing->atsc_phase_swap[j] - 1; // swap the phase for CICAS-SLTA application
				switch (pappr[i].signal_state)
				{
				case SIGNAL_STATE_GREEN:
					atsc.phase_status_greens[k] = 1;
					atsc.phase_status_ons[k] = 1;
					break;
				case SIGNAL_STATE_YELLOW:
					atsc.phase_status_yellows[k] = 1;
					atsc.phase_status_ons[k] = 1;
					break;
				case SIGNAL_STATE_RED:
					atsc.phase_status_reds[k] = 1;					
					break;
				}
				if (atsc.phase_status_ons[k] == 1)
				{
					jj = pix_timing->phase_timing.phase_af[j] - 1;
					kk = pix_timing->atsc_phase_swap[jj] -1;
					atsc.phase_status_next[kk] = 1;
				}
			}	
			clt_update(pclt,DB_ATSC_BCAST_VAR,DB_ATSC_BCAST_TYPE,sizeof(atsc_typ),(void *) &atsc);
			if (verbose == 1)
			{
				printf("ATSC: %02d.%02d.%03d %d%d%d %d%d%d %d%d %d%d\n",
					atsc.ts.hour,atsc.ts.min,atsc.ts.sec,atsc.ts.millisec,
					atsc.phase_status_greens[1],atsc.phase_status_yellows[1],atsc.phase_status_reds[1],
					atsc.phase_status_greens[3],atsc.phase_status_yellows[3],atsc.phase_status_reds[3],
					atsc.phase_status_ons[1],atsc.phase_status_ons[3],
					atsc.phase_status_next[1],atsc.phase_status_next[3]);				
			}				
			
			// write taffic signal for CICAS database
			memset(&traffic_signal,0,sizeof(traffic_signal_typ));
			for (i=0;i<pix->num_approaches;i++)
			{			
				j = pix_timing->approch[i].control_phase - 1; // signal phase that controls the approach
				k = pix_timing->atsc_phase_swap[j] - 1; // swap the phase for CICAS-SLTA application
				traffic_signal.ring_phase[k].phase = pappr[i].signal_state- 1;
				traffic_signal.ring_phase[k].time_left = (double)pappr[i].time_to_next/10.0;
				jj = pix_timing->approch[i].control_phase - 1; // signal phase that controls the approach
				j = pix_timing->phase_timing.phase_swap[jj] - 1;
				k = pix_timing->atsc_phase_swap[j] - 1;				
				traffic_signal.ring_phase[k].time_used = signal_trace.timeused[j];
			}			
			if (traffic_flag == 1)
			{
				clt_update(pclt,DB_TRAFFIC_SIGNAL_VAR,DB_TRAFFIC_SIGNAL_TYPE,
					sizeof(traffic_signal_typ),(void *) &traffic_signal);
			}
			if (verbose == 1)
			{
				printf("CICAS: %s %.1f %.1f %s %.1f %.1f\n",
					ix_signal_state_string(traffic_signal.ring_phase[1].phase+1),
					traffic_signal.ring_phase[1].time_used,traffic_signal.ring_phase[1].time_left,
					ix_signal_state_string(traffic_signal.ring_phase[3].phase+1),
					traffic_signal.ring_phase[3].time_used,traffic_signal.ring_phase[3].time_left);
			}
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
			if ( psignal_status->master_cycle_clock != signal_trace.signal.master_cycle_clock)
			{
				// in case received duplicated messages, only update the tracer with the new message 
				update_signal_state(&signal_trace,psignal_status);
				signal_trace.signal.tp.tv_sec = now.tv_sec;
				signal_trace.signal.tp.tv_nsec = now.tv_nsec;			
			}
			if ( verbose == 1 ) 
			{
				// print received signal status 
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
				signal_trace.bus_time_saved = (unsigned char)(signal_state_onset[pattern][1][0] 
					- signal_EG_onset[1][0]);
			}
			else if (pPRS->requested_type == Green_Extension)
			{
				// controller is executing GE. signal onsets remain the same as the logic for executing GE
				// is to trick the local_cycle _clock
				// do nothing
				signal_trace.bus_time_saved = (unsigned char)(signal_state_onset[pattern][1][0]
					- signal_state_onset[pattern][1][1] - MAX_GE);
			}				
			if ( verbose == 1)
			{
				// print received pririty request info
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
	return (0);
}

// function to roud the clock when passing the cycle end
void cycle_rounding(float *f, float cycle_len)
{
	if ( *f >= cycle_len)
		*f -= cycle_len;
	return;
}
				  
// function to convert struct timespec into double
double ts2sec(struct timespec *ts)
{
	return ( ts->tv_sec + ((double) ts->tv_nsec/1000000000L) );
}

// function to calculate the onset of signal state change for each phase and each control plan
void get_signal_change_onset(E170_timing_typ *ptiming,float onsets[MAX_PLANS][MAX_NMEA_PHAZ][3])
{
	int i,j,k;
	float f;
	// plan 0 is running free, and there is no pre-determined onset
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 0)
			continue; // not in use plan
		for (j=0;j<MAX_NMEA_PHAZ;j++)
		{
			if (ptiming->phase_timing.permitted_phase[j] == 0)
				continue; // not in use phase
			k = ptiming->phase_timing.phase_bf[j] - 1; // k is the previous phase
			f = ptiming->plan_timing[i].force_off[k] +
				ptiming->phase_timing.yellow_intv[k] +
				ptiming->phase_timing.allred_intv[k];
			cycle_rounding(&f, ptiming->plan_timing[i].cycle_length);	
			onsets[i][j][0] = f; // green start
			onsets[i][j][1] = ptiming->plan_timing[i].force_off[j]; // yellow start
			onsets[i][j][2] = onsets[i][j][1] + ptiming->phase_timing.yellow_intv[j];
			cycle_rounding(&onsets[i][j][2], ptiming->plan_timing[i].cycle_length);
		}		
	}
	return;
}	

// function to get the signal head color for active phase
unsigned char update_head_color(unsigned char intv)
{
	if ( intv <= 7) return (SIGNAL_STATE_GREEN);
	else if ( intv >= 12 && intv <= 14) return (SIGNAL_STATE_YELLOW);
	else return (SIGNAL_STATE_RED);
}

// function to update signal status upon db trigger
void update_signal_state(signal_trace_typ *ptracer,signal_status_typ *pinput)
{
	int i,active_phase,active_intv;
	int active_color;
	ptracer->signal.ringA_phase = pinput->ringA_phase;
	ptracer->signal.ringB_phase = pinput->ringB_phase;
	ptracer->signal.ringA_interval = pinput->ringA_interval;
	ptracer->signal.ringB_interval = pinput->ringB_interval;
	ptracer->signal.local_cycle_clock = pinput->local_cycle_clock;
	ptracer->signal.master_cycle_clock = pinput->master_cycle_clock;
	for (i=0;i<8;i++)
	{
		ptracer->signal.force_off_point[i] = pinput->force_off_point[i];
	}	
	ptracer->signal.pattern = pinput->pattern;
	ptracer->signal.ts.hour = pinput->hour;
	ptracer->signal.ts.min = pinput->min;
	ptracer->signal.ts.sec = pinput->sec;
	ptracer->signal.ts.millisec = pinput->millisec;
	// get the head color for each phase
	for (i=0;i<8;i++)
	{
		ptracer->signal.signal_state[i] = SIGNAL_STATE_RED;
	}
	if (pinput->ringA_phase > 0)
	{
		ptracer->signal.signal_state[pinput->ringA_phase - 1] = 
			update_head_color(pinput->ringA_interval);
	}
	if (pinput->ringB_phase > 0)
	{
		ptracer->signal.signal_state[pinput->ringB_phase - 1] = 
			update_head_color(pinput->ringB_interval);
	}
	return;
}

// function to update priority status upon db trigger
void update_priority_state(signal_trace_typ *ptracer,signal_priority_request_typ *pinput)
{
	ptracer->prio.requested_busID = pinput->requested_busID;
	ptracer->prio.priority_type = pinput->requested_type;
	ptracer->prio.request_phase = pinput->execution_phase;
	ptracer->prio.requested_forceoff = pinput->desired_force_off;
	ptracer->prio.ts.hour = pinput->hour;
	ptracer->prio.ts.min = pinput->min;
	ptracer->prio.ts.sec = pinput->sec;
	ptracer->prio.ts.millisec = pinput->millisec;
	ptracer->priority_status = pinput->requested_type;
	return;
}

// function to assemble ix_msg and write to database 
void assem_ix_msg(ix_msg_t *pix,ix_approach_t *pappr,E170_timing_typ *ptiming,
	signal_trace_typ *psignal_trace,db_clt_typ *pclt,char *progname)
{
	ix_stop_line_t *pstop;	// Stop line info 
	int i,j,k;
	// Fill in the message structure
	memset(pix,0,sizeof(ix_msg_t));
	pix->flag = 0x7e;	// identify this as a message
	pix->version = 0x2;	// message version
	pix->message_length = (unsigned short)( sizeof(ix_msg_t)-IX_POINTER_SIZE +
		ptiming->total_no_approaches * (sizeof(ix_approach_t) - IX_POINTER_SIZE) +
		ptiming->total_no_stoplines * sizeof(ix_stop_line_t) );
	pix->message_type = 0;
	pix->control_field = 0;
	pix->ipi_byte = 0;
	pix->intersection_id = ptiming->intersection_id;
	pix->map_node_id = ptiming->map_node_id;
	pix->ix_center_lat = ptiming->center.latitude;		
	pix->ix_center_long = ptiming->center.longitude; 
	pix->ix_center_alt = ptiming->center.altitude;
	pix->antenna_lat = ptiming->antenna.latitude;    		
	pix->antenna_long = ptiming->antenna.longitude;
	pix->antenna_alt = ptiming->antenna.altitude;
	pix->seconds = (unsigned int)psignal_trace->trace_time.tv_sec;
	pix->nanosecs = psignal_trace->trace_time.tv_nsec;
	pix->cabinet_err = 0;
	pix->preempt_calls = 0;
	pix->bus_priority_calls = psignal_trace->priority_status;
	pix->preempt_state = 0;
	pix->special_alarm = 0;
	for( i=0;i<4;i++ )
	{
		pix->reserved[i] = 0;
	}
	// fill in reserve[0] with bus phase and reserve[1] with bus time saved
	// when the traffic controller is under priority
	if ( psignal_trace->priority_status != 0)
	{
		pix->reserved[0] = (unsigned char)(psignal_trace->prio.request_phase);
		pix->reserved[1] = psignal_trace->bus_time_saved;
	}	
	pix->num_approaches = ptiming->total_no_approaches;
	// Fill in the approach structure 
	memset(pappr,0,pix->num_approaches * sizeof(ix_approach_t));
	pix->approach_array = pappr;
	for (i=0;i<pix->num_approaches;i++) 
	{
		pappr[i].approach_type = ptiming->approch[i].approach_type;
		j = ptiming->approch[i].control_phase-1; // signal phase that controls the approach
		k = ptiming->phase_timing.phase_swap[j] - 1;
		pappr[i].signal_state = *(psignal_trace->psignal_state + k);
		pappr[i].time_to_next = (short)(10*psignal_trace->time2next[k]);
		pappr[i].vehicles_detected = 0;
		pappr[i].ped_signal_state = 0;  
		pappr[i].seconds_to_ped_signal_state_change = 255;
		pappr[i].ped_detected = 0;    
		pappr[i].seconds_since_ped_detect = 255;
		pappr[i].seconds_since_ped_phase_started = 255;
		pappr[i].emergency_vehicle_approach = 0;
		pappr[i].seconds_until_light_rail = 255;
		pappr[i].high_priority_freight_train = 0;
		pappr[i].vehicle_stopped_in_ix = 0;
		for(j=0;j<2;j++)
		{
			pappr[i].reserved[j] = 0;
		}
		pappr[i].number_of_stop_lines = ptiming->approch[i].no_stoplines;
		// Fill in the stop line structure 
		pstop = malloc(pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
		memset(pstop,0,pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
		pappr[i].stop_line_array = pstop;
		for( j=0;j<pappr[i].number_of_stop_lines;j++ )
		{
			pstop[j].latitude = ptiming->approch[i].stopline_cfg[j].latitude;
			pstop[j].longitude = ptiming->approch[i].stopline_cfg[j].longitude;
			pstop[j].line_length = ptiming->approch[i].stopline_cfg[j].line_length;	// cm
			pstop[j].orientation = ptiming->approch[i].stopline_cfg[j].orientation;	// degrees clockwise from north
		}
	}
	// write ix_msg to database
	ix_msg_update(pclt,pix,DB_IX_MSG_VAR,DB_IX_APPROACH1_VAR);
	// Free the pstop pointers 
	for( i=0;i<pix->num_approaches;i++ )
	{
		free(pix->approach_array[i].stop_line_array);
		pix->approach_array[i].stop_line_array = NULL;
	}
	return;
}

// function to echo intersection configurations
void echo_cfg(E170_timing_typ *ptiming,float onsets[MAX_PLANS][MAX_NMEA_PHAZ][3])
{
	int i,j,k;
	printf("Welcome to %s\n\n",ptiming->name);
	printf("Approach configurations:\n");
	printf("\tIntersection ID %d\n",ptiming->intersection_id);
	printf("\tMap node ID %d\n",ptiming->map_node_id);
	printf("\tIntersection center gps %d %d %d\n",
		ptiming->center.latitude,ptiming->center.longitude,ptiming->center.altitude);
	printf("\tAntenna gps %d %d %d\n",
		ptiming->antenna.latitude,ptiming->antenna.longitude,ptiming->antenna.altitude);
	printf("\tNumber of approaches %d\n",ptiming->total_no_approaches);
	printf("\tNumber of stoplines %d\n",ptiming->total_no_stoplines);
	printf("\tStopline configurations:\n");
	printf("\t\t(approach_type, control_phase, no_stoplines)\n");
	printf("\t\t(stopline_lat, stopline_long, stopline_length, stopline_orient)\n");
	for (i=0;i<ptiming->total_no_approaches;i++)
	{
		printf("\t\t%d %d %d\n",
			ptiming->approch[i].approach_type,ptiming->approch[i].control_phase,
			ptiming->approch[i].no_stoplines);
		for (j=0;j<ptiming->approch[i].no_stoplines;j++)
		{
			printf("\t\t%d %d %d %d\n",
				ptiming->approch[i].stopline_cfg[j].latitude,
				ptiming->approch[i].stopline_cfg[j].longitude,
				ptiming->approch[i].stopline_cfg[j].line_length,
				ptiming->approch[i].stopline_cfg[j].orientation);
		}
	}
	printf("\n");
	printf("Phase configurations:\n");
	j = 0;
	for (i=0;i<MAX_NMEA_PHAZ;i++)
	{
		if (ptiming->phase_timing.permitted_phase[i] == 1) 
			j+= 1;
	}
	printf("\tNumber of activated phases %d\n",j);
	// plan 0 is running free
	j = 0;
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 1)
			j+= 1;
	}
	printf("\tNumber of activated plans %d\n",j);
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 0)
			continue;
		printf("\tPlan_no, Synch_phase, Cycle_length\n");
		k = ptiming->plan_timing[i].synch_phase-1;
		printf("\t\t%d\t%d\t%d\n",
			ptiming->plan_timing[i].control_type,
			ptiming->phase_timing.phase_swap[k],
			(int)ptiming->plan_timing[i].cycle_length);
		printf("\tPlan_no, phase, green_start, yellow_start, red_start\n");
		for (j=0;j<MAX_NMEA_PHAZ;j++)
		{
			if (ptiming->phase_timing.permitted_phase[j] == 0)
				continue;
			printf("\t\t%d %d %5.1f %5.1f %5.1f\n",
				i,ptiming->phase_timing.phase_swap[j],
				onsets[i][j][0],onsets[i][j][1],onsets[i][j][2]);
		}		
	}
	fflush(stdout);
	return;
}

