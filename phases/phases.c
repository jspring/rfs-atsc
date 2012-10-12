/**\file 
 * 
 * phases.c    	File to output phase and time left, and write 
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
#include "atsc_clt_vars.h"
#include "timestamp.h"
#include "atsc.h"	/* actuated traffic signal controller header file */
#include "ix_msg.h"	/* intersection message header file */
#include "exp_traffic_signal.h"	/* CICAS version of IDS header file */

#undef DEBUG
#define DEBUG_ATSC
#undef DEBUG_FILE_READ

#define TEST_DIR	"/home/atsc/test/"

#define MAX_OVERLAPS	4	/* Right-turn overlap phases */
#define MAX_PARENTS	2	/* Number of parent phases */

static int sig_list[] = {
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGALRM,	// for timer
	ERROR
};

static jmp_buf exit_env;
static void sig_hand(int code);
static void approach_update(db_clt_typ *pclt, void * papp, int approach_num, int size);

/* List of variables that this process writes */
/* Does not include TRAFFIC_SIGNAL, which is created by cicas_create */
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

static int trig_list[] = {DB_ATSC_VAR};

#define NUM_TRIG_VARS (sizeof(trig_list)/sizeof(int))

// Routine to scan the lists of start and end hour/minute pairs
// and compare with the current time to determine and return
// the number of the active timing plan from the array pintervals
int get_timing_plan(int local_time_sec, int no_intervals, 
	int *hour_start, int *minute_start, int *pintervals)
{
	int active_plan, i;

	/* No interval means only one timing plan */
	if( no_intervals == 0 )
		active_plan = 1;
	else
	{
		struct tm tmval;
		time_t time_t_secs = local_time_sec;

		localtime_r(&time_t_secs, &tmval); 

		for( i=1; i<=no_intervals; i++ )
		{
			if( tmval.tm_hour < hour_start[1] )
			{
				active_plan = pintervals[no_intervals];	
				/* Exit the loop */
				i = no_intervals;
			}
			else if( tmval.tm_hour == hour_start[1] )
			{
				if( tmval.tm_min >= minute_start[1] )
					active_plan = pintervals[1];	
				else
					active_plan = pintervals[no_intervals];	
				/* Exit the loop */
				i = no_intervals;
			}
			else if( tmval.tm_hour == hour_start[no_intervals] )
			{
				if( tmval.tm_min >= minute_start[no_intervals] )
					active_plan = pintervals[no_intervals];	
				else
					active_plan = pintervals[no_intervals-1];	

				/* Exit the loop */
				i = no_intervals;
			}
			else if( tmval.tm_hour > hour_start[no_intervals] )
			{
				active_plan = pintervals[no_intervals];	
				/* Exit the loop */
				i = no_intervals;
			}
			else if( (tmval.tm_hour >= hour_start[i])
			 && (tmval.tm_hour <= hour_start[i+1]) )
			{
				if( tmval.tm_hour == hour_start[i] )
				{
					if( tmval.tm_min >= minute_start[i] )
						active_plan = pintervals[i];	
					else
						active_plan = pintervals[i-1];	
				}
				else if( tmval.tm_hour == hour_start[i+1] )
				{
					if( tmval.tm_min >= minute_start[i+1] )
						active_plan = pintervals[i+1];	
					else
						active_plan = pintervals[i];	
				}
				else
					active_plan = pintervals[i];	

				/* Exit the loop */
				i = no_intervals;
			}
		}
		
//		if( (active_plan != 1 ) && (active_plan != 2) )
//		{
//			printf("Wrong active plan %d at %2d:%02d:%02d !\n",
//				active_plan, tmval.tm_hour, tmval.tm_min, tmval.tm_sec);
//			active_plan = 1;
//		}
//		printf("localtime: %2d:%02d:%02d plan: %d ", 
//			tmval.tm_hour, tmval.tm_min, tmval.tm_sec, active_plan);
	}
	return( active_plan );	
}

int main(int argc, char *argv[])
{
	int option;
	db_clt_typ *pclt;
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = DEFAULT_SERVICE;
	FILE *fp_site, *fp_approach, *fp_phase;
	char filename_site[80], filename_approach[80], filename_phase[80];
	char lineinput[512];
	trig_info_typ trig_info;	/* Trigger info */
	db_data_typ db_data;
	posix_timer_typ *ptmr;		/* Timer info */
	traffic_signal_typ *psignal;	/* CICAS IDS info */
	atsc_typ atsc;			/* ATSC info */
	atsc_typ *patsc = &atsc; // pointer for convenience
	ix_approach_t *pappr;	/* Approach info */
	ix_stop_line_t *pstop;	/* Stop line info */
	ix_msg_t *pix = malloc (sizeof(ix_msg_t));
	int ptrsize = sizeof(void *);

	double start_time, curr_time=0, old_time;
	struct timespec now;
	struct timespec trig_time;	//when triggered by ATSC data
	struct tm tmval;	/* Formatted time output */
	time_t timer;
	int status;
	bool_typ timing_plan_start = FALSE;
	bool_typ timing_interval_start = FALSE;
	bool_typ timing_table_start = FALSE;
	bool_typ read_plans_only = TRUE;
	bool_typ end_plans = FALSE;
	bool_typ stop_line_start = FALSE;
	bool_typ sniffer_start = FALSE;
	bool_typ overlap_start = FALSE;
	bool_typ first_time = TRUE;
	bool_typ first_trigger = FALSE;
	int tindex;
	int i=1, j=1, k=1, m=1, line=1, jj;	
	int recv_type;
	int sizeof_appr;
	int xport = COMM_PSX_XPORT;
	static int total_no_stops = 0;
	int intersection_id, node_id; 
	int no_approaches = 100;	// must be greater than i at start
	int lat1, long1, alt1, lat2, long2, alt2;

	// 0 element for these arrays is unused, FORTRAN style
	// phase_no is the NEMA phase corresponding to an approach index
	int phase_no[MAX_APPROACHES+1], no_stop[MAX_APPROACHES+1];
	int stop_lat[MAX_APPROACHES+1][MAX_STOP_LINES+1];
	int stop_long[MAX_APPROACHES+1][MAX_STOP_LINES+1];
	int stop_length[MAX_APPROACHES+1][MAX_STOP_LINES+1];
	int stop_orient[MAX_APPROACHES+1][MAX_STOP_LINES+1];
	int no_phases, no_timings;
	int no_overlaps = 100;	// must be greater than m at start
	int no_intervals = 100;	// must be greater than actual number
	int sniff_green, sniff_yellow, sniff_red;

	// phase_index is the NEMA phase for the corresponding
	// elements of the timing plans (those with the same "j")
	int phase_index[MAX_TIMINGS+1][MAX_PHASES+1];
	int hour_start[MAX_TIMINGS+1];
	int minute_start[MAX_TIMINGS+1];
	int cycle_length[MAX_TIMINGS+1]; 
	int timing_type[MAX_TIMINGS+1]; //0 fixed, 1 actuated, 2 coordinate
	int final_phase_no[MAX_TIMINGS+1]; //end of this phase begins cycle

	// elements of the overlap phases (those with the same "l")
	// we have 2 parent and 2 omit phases
	int overlap_phase[MAX_OVERLAPS+1], overlap_approach[MAX_OVERLAPS+1];
	int parent_phase[MAX_OVERLAPS+1][MAX_PARENTS+1];
	int omit_phase[MAX_OVERLAPS+1][MAX_PARENTS+1];
	static int old_parent_color[MAX_OVERLAPS+1][MAX_PARENTS+1];
	float start_parentsred;

	// offset must be float because may include yellow intervals and
	// all-red that are not whole numbers
	float offset_for_green[MAX_TIMINGS+1][MAX_PHASES+1];
	float green_length[MAX_TIMINGS+1][MAX_PHASES+1]; //split, coordinated
	// yellow and red intervals may be specified in tenths of a second
	float yellow_length[MAX_TIMINGS+1][MAX_APPROACHES+1];
	float all_red_length[MAX_TIMINGS+1][MAX_APPROACHES+1];
	int plan_no[MAX_INTERVALS];	// timing plan code/interval

	int red[MAX_PHASES+1], yellow[MAX_PHASES+1], green[MAX_PHASES+1];
	int old_red[MAX_PHASES+1],
		 old_yellow[MAX_PHASES+1], old_green[MAX_PHASES+1];
	int signal_color[MAX_PHASES+1];
	float time_left[MAX_PHASES+1], time_used[MAX_PHASES+1];
	float start_green[MAX_PHASES+1],
		 start_yellow[MAX_PHASES+1], start_red[MAX_PHASES+1];
	int start_phase;	// phase at beginning of timing plan
	float time_sec = 0.0; 
	float cycle_sec = 0.0;
	char *site = "rfs";
	int verbose = 0, traffic_flag = 0;
	int millisec, timer_interval = 200;

	while ( (option = getopt( argc, argv, "d:x:s:i:hvVt" )) != EOF )
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
				site = strdup(optarg);
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
                        case 'V':
				verbose = 2; 
                                break;
                        case 'h':
				// same as default case
                        default:
				fprintf(stderr, "%s usage:\n", argv[0]);
				fprintf(stderr, "%s [-vVth] [-d domain] [-x xport] [-s site] [-i timer_interval]\n",
					argv[0]);
				fprintf(stderr, "\t-v: Verbose level 1. Print a single line\n");
				fprintf(stderr, "\t    per db read for debugging purposes.\n");
				fprintf(stderr, "\t-V: Verbose level 2. Print super debug info\n");
				fprintf(stderr, "\t    (originally the -v flag).\n");
				fprintf(stderr, "\t-t: Process and write the traffic signal data.\n");
				fprintf(stderr, "\t-h: Print this message.\n");
				fprintf(stderr, "\t-d: Specify the domain.\n");
				fprintf(stderr, "\t-x: Specify the xport.\n");
				fprintf(stderr, "\t-s: Specify the name of the site files to load.\n");
				fprintf(stderr, "\t    e.g. -s rts would load rts.approach.cfg and rts.phase.cfg.\n");
				fprintf(stderr, "\t-i: Specify the timer interval in millisec.\n");
                                exit(1);
                                break;
                }
        }

	/* Log in to the database (shared global memory).  Default to the
	 * the current host. */
	get_local_name(hostname, MAXHOSTNAMELEN);
	if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			db_vars_list, NUM_DB_VARS, trig_list, 1))
			== NULL ) {
	    printf("Database initialization error in phases\n");
	    exit( EXIT_FAILURE );
	}
	   
        /* Initialize the timer. */
        if ((ptmr = timer_init(timer_interval, DB_CHANNEL(pclt))) == NULL)
            {
            printf("timer_init failed\n");
            exit( EXIT_FAILURE );
            }

	/* Exit code on receiving signal */
	if (setjmp(exit_env) != 0) {
		if( pclt != NULL ) {
			db_list_done(pclt, 
				db_vars_list, NUM_DB_VARS,
				trig_list, NUM_TRIG_VARS);
		}
		clt_logout( pclt );
		exit( EXIT_SUCCESS );
	} else
		sig_ign( sig_list, sig_hand );

	/* Determine the start time: amount of time in sec and nanosec
	 since the Epoch time */
	status = clock_gettime( CLOCK_REALTIME, &now );
	start_time = now.tv_sec + ((double) now.tv_nsec/1000000000L);

	printf("Welcome to the %s intersection !\n", site);

	for( i=1; i <= MAX_PHASES; i++ )
	{
		old_green[i] = -1;
		old_yellow[i] = -1;
		old_red[i] = -1;
	}
	for( i=1; i <= MAX_OVERLAPS; i++ )
		for( j=1; j <= MAX_PARENTS; j++ )
			old_parent_color[i][j] = -1;

	/********************************/
	/* Read the configuration files */
	/********************************/

	/* Specify the whole path */
	strcpy( filename_approach, TEST_DIR);
	strcpy( filename_phase, TEST_DIR);

	strcat( filename_approach, site);
	strcat( filename_phase, site);

	strcat( filename_approach, ".approach.cfg");
	strcat( filename_phase, ".phase.cfg");

	// Open the approach configuration file for that site 
	fp_approach = fopen( filename_approach, "r");
	if( fp_approach == NULL )
	{
		printf("Could not open %s\n", filename_approach);
    		exit( EXIT_FAILURE );
	}
	/* Read the approach configuration file */
	printf("Reading %s\n", filename_approach);
	fflush(stdout);
	i = 0;
	j = 0;
	while( fgets( lineinput, 100, fp_approach) != NULL )
	{
		if( strncmp(lineinput,"#",1) == 0 )
			continue;
#ifdef DEBUG_FILE_READ
		printf("%s\n", lineinput);
#endif
		/* Scan the file without the # */
		switch (line) {
		case 1: sscanf(lineinput, "%d",
				 &intersection_id);
			break;
		case 2: sscanf( lineinput, "%d", &node_id);
			break;
		case 3: sscanf( lineinput, "%d %d %d", 
				&lat1, &long1, &alt1);
			break;
		case 4: sscanf( lineinput, "%d %d %d",
				 &lat2, &long2, &alt2);
			break;
		case 5:	sscanf( lineinput, "%d",
				 &no_approaches);
			stop_line_start = TRUE;
			i = 1;	// approach count
			j = 1;	// stop line count
			break;
		default:  
			if (stop_line_start) {
				sscanf( lineinput, "%d %d",
				  &phase_no[i], &no_stop[i]);
				stop_line_start = FALSE;
				total_no_stops += no_stop[i];
				printf("i: %d phase_no %d no_stop %d\n",
					i, phase_no[i], no_stop[i]);
			} else {
				sscanf( lineinput, 
				"%d %d %d %d", 
				&stop_lat[i][j],
				 &stop_long[i][j],
				&stop_length[i][j],
				 &stop_orient[i][j]);
				j++;
				if (j > no_stop[i]) {
					i++;
					j = 1;
					stop_line_start = TRUE;
				}
			}
			break;
		}
		if (i > no_approaches)
			// nothing left to read
			break;
		line += 1;
	}
	fclose( fp_approach );
	printf("Intersection: %d, node %d\n", intersection_id, node_id);
	printf("Center: lat %d long %d alt %d\n", lat1,long1,alt1);
	printf("Antenna: lat %d long %d alt %d\n", lat2,long2,alt2);
	for (i = 1; i <= no_approaches; i++) {
		printf("Approach %d: phase %d stop lines %d\n",  
			i, phase_no[i], no_stop[i]);
		for (j = 1; j <= no_stop[i]; j++)
			printf("Stop %d: lat %d long %d length %d orient %d\n",
				j, stop_lat[i][j], stop_long[i][j],
				stop_length[i][j], stop_orient[i][j]);
	}
	printf("Total no stops: %d\n", total_no_stops);

	// Open the timing/phase configuration file for
	// that specific site (could give command line)
	fp_phase = fopen( filename_phase, "r");
	if( fp_phase == NULL )
	{
		printf("Could not open %s\n", filename_phase);
    		exit( EXIT_FAILURE );
	}
	/* Read the phase configuration file */
	printf("Reading %s\n", filename_phase);
	fflush(stdout);
	line = 1;
	while( fgets( lineinput, 100, fp_phase) != NULL )
	{
		if( strncmp(lineinput,"#",1) == 0 )
			continue;
#ifdef DEBUG_FILE_READ
		printf("%s\n", lineinput);
#endif

		/* Scan the file without the # */
		switch (line) {	
		case 1: sscanf( lineinput, "%d", &no_phases);
			break;
		case 2:
			sscanf( lineinput, "%d", &no_timings);
			i = 1;	// counts timing plans
			timing_plan_start = TRUE;
			break;
		default:
#ifdef DEBUG_FILE_READ
//			printf("i %d j %d k %d: %s\n", i, j, k, lineinput);
#endif
			if (timing_plan_start) {
				sscanf(lineinput, 
					"%d", &timing_type[i]);
				if( timing_type[i] != 1 )
					sscanf(lineinput, 
					"%d %d %d",
					&timing_type[i],
					&final_phase_no[i],
					&cycle_length[i]);
				timing_plan_start = FALSE;
				j = 1;	// counts active phases
			} else {
				if (read_plans_only) {
					sscanf( lineinput, 
					"%d %f %f %f %f", 
					&phase_index[i][j],
					&offset_for_green[i][j],
					&green_length[i][j],
					&yellow_length[i][j],
					&all_red_length[i][j]);
					printf("i %d j %d index %d\n",
						i, j, phase_index[i][j]);
					j++;
					if (j > no_phases) {
						j = 1;
						i++;
						timing_plan_start=TRUE;
					}
					if (i > no_timings)  {
						timing_plan_start=FALSE;
						timing_interval_start=TRUE;
						read_plans_only=FALSE;
						end_plans=TRUE;
					}
				} else if (timing_interval_start) {
					sscanf(lineinput, 
						"%d", &no_intervals);
					k = 1;	// counts timing intervals
					timing_interval_start=FALSE;
					if( no_intervals != 0 )
						timing_table_start=TRUE;
					else
						sniffer_start=TRUE;
				} else if (timing_table_start) {
					sscanf( lineinput, 
						"%d %d %d", 
						&hour_start[k],
						&minute_start[k],
						&plan_no[k]);
					k++;
					if( k > no_intervals ) 
					{
						timing_table_start=FALSE;
						sniffer_start=TRUE;
					}
				} else if (sniffer_start) {
					sscanf(lineinput, 
						"%d %d %d", &sniff_green, &sniff_yellow, &sniff_red);
					sniffer_start=FALSE;	
					overlap_start=TRUE;	
				} else if (overlap_start) {
					sscanf(lineinput, 
						"%d", &no_overlaps);
					m = 1;	// counts overlaps
					overlap_start=FALSE;
				}
				else
				{
					sscanf( lineinput, 
						"%d %d %d %d %d %d", 
						&overlap_phase[m],
						&overlap_approach[m],
						&parent_phase[m][1],
						&parent_phase[m][2],
						&omit_phase[m][1],
						&omit_phase[m][2]);
					m++;
				}
			}
			break;
		}
		if (m > no_overlaps) 	//k > no_intervals)
			break;
		line++;
	}	
	fclose( fp_phase );

	// Echo timing plan parameters
	printf("no_intervals %d \n", no_intervals);

	for (k = 1; k <= no_intervals; k++) 
		printf("Interval %d, plan %d, %d:%02d\n",
			 k, plan_no[k], hour_start[k], minute_start[k]);

	for (i = 1; i <= no_timings; i++) {
		if( timing_type[i] != 1 )
			printf("Plan %d, mode %d, final phase %d, cycle length %d\n",
		    		i, timing_type[i], final_phase_no[i], cycle_length[i]);
		else
			printf("Plan %d, mode %d\n", i, timing_type[i]);
		for (j = 1; j <= no_phases; j++) {
		    printf("\tPhase %d, Offset %.1f, G %.1f Y %.1f AllR %.1f\n",
				phase_index[i][j], offset_for_green[i][j],
				green_length[i][j], yellow_length[i][j],
				all_red_length[i][j]);
		}
	}

	// Echo sniffer parameters
	if( (sniff_green==0) && (sniff_yellow==0) && (sniff_red==0) )
		printf("No sniffers\n"); 
	else 
		printf("Sniffer signals: green %d,  yellow %d, red %d\n", 
			sniff_green, sniff_yellow, sniff_red);

	// Echo overlap parameters
	printf("no_overlaps %d \n", no_overlaps);
	for (i = 1; i <= no_overlaps; i++) {
		printf("Overlap %d, phase %d, approach %d, parent phases %d %d, omit phases %d %d\n", 
			i, overlap_phase[i], overlap_approach[i], parent_phase[i][1], 
			parent_phase[i][2], omit_phase[i][1], omit_phase[i][2]);
	}
	fflush(stdout);

	// Special case because phase numbering is wrong at Page Mill
	if( strncmp(site,"page",4) == 0 )
	{
		parent_phase[1][1] = 4;
		parent_phase[1][2] = 5;
		omit_phase[1][1] = 3;
		omit_phase[1][2] = 6;
	}

	for( ;; )
	{
		old_time = curr_time;
		status = clock_gettime( CLOCK_REALTIME, &now );
		curr_time = now.tv_sec + ((double) now.tv_nsec/1000000000L) - start_time;
		millisec = (int)(now.tv_nsec/1000000L);

		/* Time in sec */
		time_sec += curr_time-old_time; 
		cycle_sec += curr_time-old_time;

		/* Now wait for a trigger from the ATSC */
            	recv_type = clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));

		if( DB_TRIG_VAR( &trig_info) == DB_ATSC_VAR )
		{
			/***************************************/
			/* Read the database for the ATSC data */
			/***************************************/
			first_trigger = TRUE;
			status = clock_gettime( CLOCK_REALTIME, &trig_time);
			db_clt_read(pclt, DB_ATSC_VAR, sizeof(atsc), patsc);
#ifdef DEBUG_ATSC
			if( patsc->info_source == ATSC_SOURCE_NTCIP )
				printf("atsc trigger: green %02hhx yellow %02hhx red %02hhx\n",
					patsc->phase_status_greens[0],
					patsc->phase_status_yellows[0],
					patsc->phase_status_reds[0]);
			else if( patsc->info_source == ATSC_SOURCE_SNIFF )
			{
				if( sniff_green == 1 )
					printf("atsc trigger: green %02hhx",
						patsc->phase_status_greens[0]);
				if( sniff_yellow == 1 )
					printf(" yellow %02hhx",
						patsc->phase_status_yellows[0]);
				printf("\n");
			}
			else if( patsc->info_source == ATSC_SOURCE_AB3418 )
				printf("atsc trigger: green %02hhx\n",
					patsc->phase_status_greens[0]);
#endif
		}
		else if( recv_type == DB_TIMER )
                {
                //	printf("received timer alarm\n");
			status = clock_gettime( CLOCK_REALTIME, &trig_time);
                }
		else if( DB_TRIG_VAR( &trig_info) != DB_ATSC_VAR )
			printf("Unknown trigger %d in phases\n", recv_type);

		/***************************/
		/* Decode the signal color */
		/***************************/

		/* From rfs.approach.cfg, we know that: 
		 * no_approaches=4
		 * approach 1: phase_no[1]=2
		 * approach 2: phase_no[2]=4
		 * approach 3: phase_no[3]=2
		 * approach 4: phase_no[4]=4
		 *
		 * From rfs.phase.cfg, we know that: 
		 * no_phases=2
		 * phase_index[1]=2
		 * phase_index[2]=4
		 * => we only read the ATSC for phases 2 and 4
		 *    and fill green[2] and green[4]
		 */

		// if there are multiple timing plans, scan the arrays,
		// compare to the current time, and figure out what
		// plan is in effect now
		tindex =  get_timing_plan(trig_time.tv_sec, no_intervals, hour_start, minute_start, plan_no);
		start_phase = phase_index[tindex][1]; 

		for( i=1; i <= no_phases; i++ )
		{
			int phase_num = phase_index[tindex][i];

			/* If we have inputs on all 3 colors */
			if( patsc->info_source == ATSC_SOURCE_NTCIP )
			{
				green[phase_num] = atsc_phase_is_set( patsc->phase_status_greens, phase_num);
				yellow[phase_num] = atsc_phase_is_set( patsc->phase_status_yellows, phase_num);
				red[phase_num] = atsc_phase_is_set( patsc->phase_status_reds, phase_num);
#ifdef DEBUG_ATSC
				printf("phase%d green: %d yellow: %d red: %d\n",phase_num,green[phase_num], yellow[phase_num],red[phase_num]);
#endif

				/* Fill signal_color by phase */
				if( green[phase_num] == 1 )
				{
					signal_color[phase_num] = SIGNAL_STATE_GREEN;
					if( green[phase_num] != old_green[phase_num] )
						start_green[phase_num] = time_sec;
				}	
				else if( yellow[phase_num] == 1 )
				{
					signal_color[phase_num] = SIGNAL_STATE_YELLOW;
					if( yellow[phase_num] != old_yellow[phase_num] )
						start_yellow[phase_num] = time_sec;
				}	
				else if( red[phase_num] == 1 )
				{
					signal_color[phase_num] = SIGNAL_STATE_RED;
					if( red[phase_num] != old_red[phase_num] )
						start_red[phase_num] = time_sec;
				}	
			}
			else if( (patsc->info_source == ATSC_SOURCE_SNIFF) && (sniff_green==1) && (sniff_yellow==1) )
			{ 	/* If we have input on green and yellow */
				green[phase_num] = atsc_phase_is_set( patsc->phase_status_greens, phase_num);
				yellow[phase_num] = atsc_phase_is_set( patsc->phase_status_yellows, phase_num);

#ifdef DEBUG_ATSC
//				printf("phase%d green: %d yellow: %d\n",phase_num,green[phase_num], yellow[phase_num]);
#endif
					
				/* Fill signal_color by phase */
				if( (green[phase_num] == 1) && (yellow[phase_num] == 0) )
				{
					signal_color[phase_num] = SIGNAL_STATE_GREEN;
					red[phase_num] = 0;
					clear_atsc_phase( patsc->phase_status_reds, phase_num);
					if( green[phase_num] != old_green[phase_num] )
						start_green[phase_num] = time_sec;
				}
				//else if( (green[phase_num] == 0) && (yellow[phase_num] == 1) )
				else if( (yellow[phase_num] == 1) )	//bug at RFS where green and yellow are both on for one trigger !!
				{
					signal_color[phase_num] = SIGNAL_STATE_YELLOW;
					red[phase_num] = 0;
					clear_atsc_phase( patsc->phase_status_reds, phase_num);
					if( yellow[phase_num] != old_yellow[phase_num] )
					{
						start_yellow[phase_num] = time_sec;
						if (phase_num == final_phase_no[tindex])
							cycle_sec = 0.0;
					}
				}
				else if( (green[phase_num] == 0) && (yellow[phase_num] == 0) )
				{	/* Color has to be red ! */
					signal_color[phase_num] = SIGNAL_STATE_RED;
					red[phase_num] = 1;
					set_atsc_phase( patsc->phase_status_reds, phase_num);
					if( red[phase_num] != old_red[phase_num] )
						start_red[phase_num] = time_sec;
				}
			}
			else if( ((patsc->info_source == ATSC_SOURCE_SNIFF) && (sniff_green==1) && (sniff_yellow==0))
				|| (patsc->info_source == ATSC_SOURCE_AB3418) )
			{ 	/* If we have input on green only */
				green[phase_num] = atsc_phase_is_set( patsc->phase_status_greens, phase_num);

#ifdef DEBUG_ATSC
//				printf("phase%d green: %d\n",phase_num,green[phase_num]);
#endif
					
				/* Fill signal_color by phase */
				if( green[phase_num] == 1 )
				{
					signal_color[phase_num] = SIGNAL_STATE_GREEN;
					yellow[phase_num] = 0;
					red[phase_num] = 0;
					clear_atsc_phase( patsc->phase_status_yellows, phase_num);
					clear_atsc_phase( patsc->phase_status_reds, phase_num);
					if( green[phase_num] != old_green[phase_num] )
						start_green[phase_num] = time_sec;
				}
				else if( green[phase_num] == 0 )
				{
					if( (green[phase_num] != old_green[phase_num]) && (old_green[phase_num] != -1) )
					{
						/* If we were in green before, then we're in yellow now */
						start_yellow[phase_num] = time_sec;
						if (phase_num == final_phase_no[tindex])
							cycle_sec = 0.0;
					}
					if( (old_green[phase_num] == -1) || (start_yellow[phase_num] == -1) )
					{
						/* If we start the process while green is 0, we don't know
						   for sure whether we are in yellow or red */
						signal_color[phase_num] = SIGNAL_STATE_UNKNOWN;
						start_yellow[phase_num] = -1;
					}
					else if( (time_sec - start_yellow[phase_num]) <= yellow_length[tindex][i] )
					{
						signal_color[phase_num] = SIGNAL_STATE_YELLOW;
						yellow[phase_num] = 1;
						red[phase_num] = 0;
						set_atsc_phase( patsc->phase_status_yellows, phase_num);
						clear_atsc_phase( patsc->phase_status_reds, phase_num);
					}
					else
					{	/* Color has to be red !!! */
						signal_color[phase_num] = SIGNAL_STATE_RED;
						yellow[phase_num] = 0;
						red[phase_num] = 1;
						set_atsc_phase( patsc->phase_status_reds, phase_num);
						clear_atsc_phase( patsc->phase_status_yellows, phase_num);
						if( red[phase_num] != old_red[phase_num] )
							start_red[phase_num] = time_sec;
					}
				}
			}
		}

		/**************************************/
		/* Calculate countdown for each phase */
		/**************************************/

		for( i=1; i <= no_phases; i++ )
		{
			int phase_num;
			phase_num = phase_index[tindex][i];

			if( signal_color[phase_num] == SIGNAL_STATE_UNKNOWN )
				time_left[phase_num] = 0.0;
			else if( signal_color[phase_num] == SIGNAL_STATE_GREEN )
			{
				if( timing_type[tindex] == 1 )	/* actuated */
				{	/* Negative because we are not sure of the value, then hardcode at zero */
					time_left[phase_num] = -(start_green[phase_num]+yellow_length[tindex][i]-time_sec);
					if( time_left[phase_num] > 0 )
						time_left[phase_num] = 0; 
				}
				else
					time_left[phase_num] = (start_green[phase_num]+green_length[tindex][i])-time_sec;
				time_used[phase_num] = time_sec-start_green[phase_num];
			}
			else if( signal_color[phase_num] == SIGNAL_STATE_YELLOW )
			{
				time_left[phase_num] = (start_yellow[phase_num]+yellow_length[tindex][i])-time_sec;
				time_used[phase_num] = time_sec-start_yellow[phase_num];
			}
			else if( signal_color[phase_num] == SIGNAL_STATE_RED )
			{
		 	//	if( timing_type[tindex] == 0 )		/* fixed */
			//	{	/* If fixed time, we are in red for the whole cycle length except for that phase's 
			//		   green and yellow (ie active signals) */
			//		time_left[phase_num] = (start_red[phase_num]+offset_for_green[tindex][no_phases]
			//			+green_length[tindex][no_phases]-green_length[tindex][i]-yellow_length[tindex][i])-time_sec;
			//	}
				if( timing_type[tindex] == 1 )	/* actuated */
				{	/* Negative because we are not sure of the value, then hardcode at zero */
					time_left[phase_num] = -(start_red[phase_num]+yellow_length[tindex][i]-time_sec);
					if( time_left[phase_num] > 0 )
						time_left[phase_num] = 0; 
				}
				else
					time_left[phase_num] = (start_red[phase_num]+cycle_length[tindex]
						-green_length[tindex][i]-yellow_length[tindex][i])-time_sec;
				time_used[phase_num] = time_sec-start_red[phase_num];
			}
		}

		/**********************************************/
                /* Special case for right-turn overlap phases */
                /**********************************************/

		for(i=1; i<= no_overlaps; i++)
                {
			/* No countdown for overlap phases except in yellow ! */
                        if( (signal_color[parent_phase[i][1]] == SIGNAL_STATE_GREEN) || (signal_color[parent_phase[i][2]] == SIGNAL_STATE_GREEN) )
			{
                                signal_color[overlap_phase[i]] = SIGNAL_STATE_GREEN;
				time_left[overlap_phase[i]] = 0;
			}
                        else if( signal_color[parent_phase[i][2]] == SIGNAL_STATE_YELLOW )
			{
				if( old_parent_color[i][2] != SIGNAL_STATE_YELLOW )
				{
					/* Find the yellow length for that phase and that timing plan */
  					for (j = 1; j <= no_phases; j++)
    						if( phase_index[tindex][j] == parent_phase[i][2] )
							jj=j;
					start_parentsred = time_sec;
				}
                                signal_color[overlap_phase[i]] = SIGNAL_STATE_YELLOW;
				time_left[overlap_phase[i]] = (start_parentsred+yellow_length[tindex][jj])-time_sec;
			}
			else if( signal_color[parent_phase[i][1]] == SIGNAL_STATE_YELLOW )
                        {
                                /* Stays green during phase 1 yellow if next phase is 8, and vice versa */
                        	if( signal_color[parent_phase[i][2]] == SIGNAL_STATE_RED )
				{
					if( old_parent_color[i][1] != SIGNAL_STATE_YELLOW )
					{
						//printf("1 yellow and 8 red\n");
						/* Find the yellow length for that phase and that timing plan */
  						for (j = 1; j <= no_phases; j++)
    							if( phase_index[tindex][j] == parent_phase[i][1] )
								jj = j;
						start_parentsred = time_sec;
					}
					if( (time_sec - start_parentsred) <= (yellow_length[tindex][jj]+all_red_length[tindex][jj]) )
					{
                                		signal_color[overlap_phase[i]] = SIGNAL_STATE_GREEN;
						time_left[overlap_phase[i]] = 0;
					}
					else
					{
                                		signal_color[overlap_phase[i]] = SIGNAL_STATE_YELLOW;
						time_left[overlap_phase[i]] = (start_parentsred+yellow_length[tindex][jj])-time_sec;
					}
				}
				else
				{
                                	signal_color[overlap_phase[i]] = SIGNAL_STATE_YELLOW;
					time_left[overlap_phase[i]] = (start_parentsred+yellow_length[tindex][jj])-time_sec;
				}
                        }
                        else if( (signal_color[parent_phase[i][1]] == SIGNAL_STATE_RED) && (signal_color[parent_phase[i][2]] == SIGNAL_STATE_RED) )
			{	
				/* Stays green when we have the red clearance phase between phases 1 and 8 being green */
				if( (old_parent_color[i][1] == SIGNAL_STATE_YELLOW) && (old_parent_color[i][2] == SIGNAL_STATE_RED) 
					&& (time_left[parent_phase[i][2]] <= 90) )
				{
					//printf("1 and 8 red\n");
					/* Find the all red length for that phase and that timing plan */
  					for (j = 1; j <= no_phases; j++)
    						if( phase_index[tindex][j] == parent_phase[i][1] )
							jj = j;
					start_parentsred = time_sec;
				}
				if( (time_sec - start_parentsred) <= all_red_length[tindex][jj] )
                                	signal_color[overlap_phase[i]] = SIGNAL_STATE_GREEN;
				else
                                	signal_color[overlap_phase[i]] = SIGNAL_STATE_RED;
                        	time_left[overlap_phase[i]] = 0.0;
			}
                        else
			{
                                signal_color[overlap_phase[i]] = SIGNAL_STATE_RED;
                        	time_left[overlap_phase[i]] = 0.0;
			}

			old_parent_color[i][1] = signal_color[parent_phase[i][1]];
			old_parent_color[i][2] = signal_color[parent_phase[i][2]];

                        /* Red during omit phases, overwrites everything before */
                        if( ((omit_phase[i][1] != 0) && ((signal_color[omit_phase[i][1]] == SIGNAL_STATE_GREEN) 
				|| (signal_color[omit_phase[i][1]] == SIGNAL_STATE_YELLOW)))
                         || ((omit_phase[i][2] != 0) && ((signal_color[omit_phase[i][2]] == SIGNAL_STATE_GREEN) 
				|| (signal_color[omit_phase[i][2]] == SIGNAL_STATE_YELLOW))) )
			{
                                signal_color[overlap_phase[i]] = SIGNAL_STATE_RED;
                        	time_left[overlap_phase[i]] = 0.0;
			}
                }

		/**************************************/
		/* Fill the DSRC intersection message */
		/**************************************/

		/* Fill in the message structure */
		memset(pix, 0, sizeof(ix_msg_t));
		pix->flag = 0x7e;		// identify this as a message
        	pix->version = 0x2;		// message version

		/* Total length of the message in bytes */
		pix->message_length = sizeof(ix_msg_t)-ptrsize +
			no_approaches * (sizeof(ix_approach_t) - ptrsize) +
			(total_no_stops) * sizeof(ix_stop_line_t);

        	pix->message_type = 0;
        	pix->control_field = 0;
        	pix->ipi_byte = 0;

        	pix->intersection_id = intersection_id;
        	pix->map_node_id = node_id;

		/* Center of intersection */
        	pix->ix_center_lat = lat1;		
        	pix->ix_center_long = long1; 
        	pix->ix_center_alt = alt1;

		/* DSRC antenna */
        	pix->antenna_lat = lat2;    		
        	pix->antenna_long = long2;
        	pix->antenna_alt = alt2;

		/* Timestamp: change time in milliseconds */
		pix->seconds = trig_time.tv_sec;
		pix->nanosecs = trig_time.tv_nsec;

        	pix->cabinet_err = 0;
        	pix->preempt_calls = 0;
        	pix->bus_priority_calls = 0;
        	pix->preempt_state = 0;
        	pix->special_alarm = 0;
		for( i=0; i < 4; i++ )
        		pix->reserved[i] = 0;

	      	pix->num_approaches = no_approaches;

		/*************************/
		/* Write pix to database */
		/*************************/
		if( first_trigger == TRUE )
 			if( clt_update(pclt,DB_IX_MSG_VAR, DB_IX_MSG_VAR,
       				sizeof(ix_msg_t),(void *)pix) == FALSE )
 			{
       				fprintf( stderr, "clt_update( DB_IX_MSG_VAR )\n" );
    				exit( EXIT_FAILURE );
			}

		/* Fill in the approach structure */
		pappr = malloc(pix->num_approaches * sizeof(ix_approach_t));
		memset(pappr, 0, pix->num_approaches * sizeof(ix_approach_t));
		pix->approach_array = pappr;

		for (i = 0; i < pix->num_approaches; i++) 
		{
			/* Odd phases are left-turn, others are straight through */
			if( (phase_no[i+1] %2) == 1 )
				pappr[i].approach_type = APPROACH_T_LEFT_TURN;
			else
				pappr[i].approach_type = APPROACH_T_STRAIGHT_THROUGH;

			/* Special case because phase 4 is a left-turn at Fifth Ave */
			if( (strncmp(site,"fifth",5) == 0) && (phase_no[i+1] == 4) )
				pappr[i].approach_type = APPROACH_T_LEFT_TURN;

			/* Phase number is phase_no[i+1] since i starts from 0 */
			pappr[i].signal_state = signal_color[phase_no[i+1]];
			pappr[i].time_to_next = (int)(10*time_left[phase_no[i+1]]);

			/* Special case for overlap phases which are right-turn */
			for(j = 1; j <= no_overlaps; j++)
				if( overlap_approach[j] == i+1 ) 
				{
                                	pappr[i].approach_type = APPROACH_T_RIGHT_TURN;
					pappr[i].signal_state = signal_color[overlap_phase[j]];
					pappr[i].time_to_next = (int)(10*time_left[overlap_phase[j]]);
				}

        		pappr[i].vehicles_detected = 0;

			/* Pedestrian */
        		pappr[i].ped_signal_state = 0;  
        		pappr[i].seconds_to_ped_signal_state_change = 255;
        		pappr[i].ped_detected = 0;    
        		pappr[i].seconds_since_ped_detect = 255;
        		pappr[i].seconds_since_ped_phase_started = 255;

        		pappr[i].emergency_vehicle_approach = 0;
        		pappr[i].seconds_until_light_rail = 255;
        		pappr[i].high_priority_freight_train = 0;
        		pappr[i].vehicle_stopped_in_ix = 0;
			for( j=0; j < 2; j++ )
        			pappr[i].reserved[j] = 0;

        		pappr[i].number_of_stop_lines = no_stop[i+1];

			/* Fill in the stop line structure */
			pstop = malloc(pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
			memset(pstop, 0, pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
			pappr[i].stop_line_array = pstop;

			for( j=0; j < pappr[i].number_of_stop_lines ; j++ )
			{
        			pstop[j].latitude = stop_lat[i+1][j+1];
        			pstop[j].longitude = stop_long[i+1][j+1];
        			pstop[j].line_length = stop_length[i+1][j+1];	// cm
        			pstop[j].orientation = stop_orient[i+1][j+1];	// degrees clockwise from north
			}

			/***************************/
			/* Write pappr to database */
			/***************************/
			if( first_trigger == TRUE )
				ix_approach_update(pclt, &pappr[i], i, DB_IX_APPROACH1_VAR);
		}

		/* Print the whole message for debugging after we've had the first trigger from the ATSC */
		if( (verbose == 1) && (first_trigger == TRUE) )
		{
			if( first_time == TRUE )
			{
				first_time = FALSE;
				if( patsc->info_source == ATSC_SOURCE_NTCIP ) 
					printf("Using NTCIP\n");
				else if( patsc->info_source == ATSC_SOURCE_SNIFF ) 
					printf("Using sniffers\n");
				else if( patsc->info_source == ATSC_SOURCE_AB3418 )
					printf("Using AB3418\n");
			}
			struct tm tmval;
			time_t time_t_secs = pix->seconds;

			localtime_r(&time_t_secs, &tmval); 
			printf("localtime: %2d:%02d:%02d.%03d  ", 
				tmval.tm_hour, tmval.tm_min, tmval.tm_sec,
				pix->nanosecs/1000000); 

			/* Show millisec */
			printf("%6.3f: ", cycle_sec); 

			/* Print only the signal color and countdown seconds */
		//bb	for (i = 0; i < no_approaches; i++) {
			for (i = 0; i < no_phases; i++) {
				printf("%d%s %3.0f : ",  phase_no[i+1],
				ix_signal_state_string(pappr[i].signal_state),
				pappr[i].time_to_next/10.0);
			}
			printf("\n");
			fflush(stdout);
		}
		else if( (verbose == 2) && (first_trigger == TRUE) )
			ix_msg_print(pix);

		/***************************************/
		/* Write missing ATSC data to database */
		/***************************************/
		/* Fill the ATSC message completely since we only had info
		 * on green with the sniffers or the AB3418 */
		if( (patsc->info_source == ATSC_SOURCE_SNIFF) || (patsc->info_source == ATSC_SOURCE_AB3418) )
		{
			if( first_trigger == TRUE )
				db_clt_write(pclt, DB_ATSC_BCAST_VAR, sizeof(atsc_typ),
					patsc);
		}

		/**********************************************/
		/* Write traffic signal for CICAS to database */
		/**********************************************/
		if( (traffic_flag == 1) && (first_trigger == TRUE) )
		{
			/* Use no_phases instead of no_approaches (do not fill overlap phases) */
			for (i = 1; i <= no_phases; i++) 
			{
				int phase_num;
				phase_num = phase_index[tindex][i];

				/* ring_phase[0] corresponds to NEMA phase 1, etc */
				/* ring phase=1,3,5,7 for NEMA phase=2,4,2,4 at RFS

				/* Different definition of phase color for Joel: */
				/* SV phase green=0, amber=1, red=2, flashing red=3 */
				if( (signal_color[phase_num] >= SIGNAL_STATE_GREEN)&&(signal_color[phase_num] <= SIGNAL_STATE_RED) )
					psignal->ring_phase[phase_num-1].phase = signal_color[phase_num]-1;
				else if( signal_color[phase_num] == SIGNAL_STATE_FLASHING_RED )
					psignal->ring_phase[phase_num-1].phase = signal_color[phase_num]-2;

				/* Time left and time used is a double in sec */
				psignal->ring_phase[phase_num-1].time_left = (double)time_left[phase_num];
				psignal->ring_phase[phase_num-1].time_used = (double)time_used[phase_num];
				if (verbose) 
					printf("traffic_signal: phase%d %d %lf %lf : ",
			phase_num, psignal->ring_phase[phase_num-1].phase,
			psignal->ring_phase[phase_num-1].time_left, psignal->ring_phase[phase_num-1].time_used);
			}
			if (verbose) {
				printf("\n");
				fflush(stdout);
			}

			// ignore error
			(void) clt_update(pclt, DB_TRAFFIC_SIGNAL_TYPE,
			 DB_TRAFFIC_SIGNAL_VAR, sizeof(traffic_signal_typ),
					(void *) psignal);
		}

		for( i=1; i <= no_phases; i++ )
		{
			int phase_num = phase_index[tindex][i];
			old_green[phase_num] = green[phase_num];
			old_yellow[phase_num] = yellow[phase_num];
			old_red[phase_num] = red[phase_num];
		}	

		/* Free the pointers */
		for( i=0; i < pix->num_approaches; i++ )
			free(pappr[i].stop_line_array);
		free(pappr);
	}
}

static void sig_hand( int code )
{
	if( code == SIGALRM )
		return;
	else
        	longjmp( exit_env, code );
}

