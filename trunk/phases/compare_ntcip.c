/**\file 
 * 
 * compare.c   	File to compare timing between NTCIP and sniffers. 
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

#undef DEBUG_ATSC
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

static db_id_t db_vars_list[] = {
};

#define NUM_DB_VARS (sizeof(db_vars_list)/sizeof(db_id_t))

static int trig_list[] = {DB_ATSC_VAR, DB_ATSC2_VAR};

#define NUM_TRIG_VARS (sizeof(trig_list)/sizeof(int))


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
	int trig_type;
	db_data_typ db_data;
	atsc_typ atsc, atsc2;	/* ATSC info */
	atsc_typ *patsc = &atsc; // pointer for convenience
	atsc_typ *patsc2 = &atsc2; // pointer for convenience

	double start_time, curr_time=0, old_time;
	struct timespec now;
	struct timespec trig_time, trig2_time;	//when triggered by ATSC data
	struct tm tmval, tmval2;	/* Formatted time output */
	time_t time_t_secs, time_t_sec2;
	int status;
	int sniffer_trig[4][100], ntcip_trig[4][100];	
	int t1_p2 = 0, t1_p4 = 0, t2_p2 = 0, t2_p4 = 0;
	bool_typ print_flag = FALSE;

	bool_typ timing_plan_start = FALSE;
	bool_typ timing_interval_start = FALSE;
	bool_typ timing_table_start = FALSE;
	bool_typ read_plans_only = TRUE;
	bool_typ end_plans = FALSE;
	bool_typ stop_line_start = FALSE;
	bool_typ sniffer_start = FALSE;
	bool_typ overlap_start = FALSE;
	int tindex;
	int i=1, j=1, k=1, m=1, line=1, jj;	
	int recv_type;
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

	// offset must be float because may include yellow intervals and
	// all-red that are not whole numbers
	float offset_for_green[MAX_TIMINGS+1][MAX_PHASES+1];
	float green_length[MAX_TIMINGS+1][MAX_PHASES+1]; //split, coordinated
	// yellow and red intervals may be specified in tenths of a second
	float yellow_length[MAX_TIMINGS+1][MAX_APPROACHES+1];
	float all_red_length[MAX_TIMINGS+1][MAX_APPROACHES+1];
	int plan_no[MAX_INTERVALS];	// timing plan code/interval

	int red[MAX_PHASES+1], yellow[MAX_PHASES+1], green[MAX_PHASES+1];
	int red2[MAX_PHASES+1], yellow2[MAX_PHASES+1], green2[MAX_PHASES+1];
	int old_red[MAX_PHASES+1],
		 old_yellow[MAX_PHASES+1], old_green[MAX_PHASES+1];
	int old_red2[MAX_PHASES+1],
		 old_yellow2[MAX_PHASES+1], old_green2[MAX_PHASES+1];
	int signal_color[MAX_PHASES+1], signal_color2[MAX_PHASES+1];
	int old_color[MAX_PHASES+1], old_color2[MAX_PHASES+1];
	float start_green[MAX_PHASES+1],
		 start_yellow[MAX_PHASES+1], start_red[MAX_PHASES+1];
	float start_green2[MAX_PHASES+1],
		 start_yellow2[MAX_PHASES+1], start_red2[MAX_PHASES+1];
	float time_sec = 0.0; 
	float cycle_sec = 0.0;
	char *site = "rfs";
	int verbose = 0;
	int millisec;

	while ( (option = getopt( argc, argv, "d:s:vx:hV" )) != EOF )
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
				fprintf(stderr, "%s [-vVh] [-d domain] [-x xport] [-s site]\n",
					argv[0]);
				fprintf(stderr, "\t-v: Verbose level 1. Prints a single line\n");
				fprintf(stderr, "\t    per db read for debugging purposes.\n");
				fprintf(stderr, "\t-V: Verbose level 2. Prints super debug info\n");
				fprintf(stderr, "\t    (originally the -v flag).\n");
				fprintf(stderr, "\t-h: Prints this message.\n");
				fprintf(stderr, "\t-d: Specify the domain.\n");
				fprintf(stderr, "\t-x: Specify the xport.\n");
				fprintf(stderr, "\t-s: Specify the name of the site files to load.\n");
				fprintf(stderr, "\t    e.g. -s rts would load rts.approach.cfg and rts.phase.cfg.\n");
                                exit(1);
                                break;
                }
        }

	/* Log in to the database (shared global memory).  Default to the
	 * the current host. */
	get_local_name(hostname, MAXHOSTNAMELEN);
	if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			db_vars_list, NUM_DB_VARS, trig_list, NUM_TRIG_VARS))
			== NULL ) {
	    printf("Database initialization error in compare\n");
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
		old_green2[i] = -1;
		old_yellow2[i] = -1;
		old_red2[i] = -1;
	}

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
			/********************************************************/
			/* Read the database for the ATSC data from the sniffer */
			/********************************************************/
			status = clock_gettime( CLOCK_REALTIME, &trig_time);
			db_clt_read(pclt, DB_ATSC_VAR, sizeof(atsc), patsc);
#ifdef DEBUG_ATSC
			printf("sniffer trigger: green %02hhx\n",
                                patsc->phase_status_greens[0]);
#endif
			trig_type = ATSC_SOURCE_SNIFF;
		}

		if( DB_TRIG_VAR( &trig_info) == DB_ATSC2_VAR )
                {
                        /**************************************************/
                        /* Read the database for the ATSC data from NTCIP */
                        /**************************************************/
                        status = clock_gettime( CLOCK_REALTIME, &trig2_time);
                        db_clt_read(pclt, DB_ATSC2_VAR, sizeof(atsc2), patsc2);
#ifdef DEBUG_ATSC
                        printf("ntcip trigger: green %02hhx\n",
                                patsc2->phase_status_greens[0]);
#endif
                        trig_type = ATSC_SOURCE_NTCIP;
                }


		/***************************/
		/* Decode the signal color */
		/***************************/

		tindex =  1;	//get_timing_plan(trig_time.tv_sec, no_intervals, hour_start, minute_start, plan_no);

		for( i=1; i <= no_phases; i++ )
		{
			int phase_num = phase_index[tindex][i];

			if( (trig_type == ATSC_SOURCE_SNIFF) && (sniff_green==1) && (sniff_yellow==1) )
			{ 	/* If we have input on green and yellow */
				green[phase_num] = atsc_phase_is_set( patsc->phase_status_greens, phase_num);
				yellow[phase_num] = atsc_phase_is_set( patsc->phase_status_yellows, phase_num);

#ifdef DEBUG_ATSC
				printf("phase%d green: %d yellow: %d\n",phase_num,green[phase_num], yellow[phase_num]);
#endif
					
				/* Fill signal_color by phase */
				if( (green[phase_num] == 1) && (yellow[phase_num] == 0) )
				{
					signal_color[phase_num] = SIGNAL_STATE_GREEN;
					red[phase_num] = 0;
					if( green[phase_num] != old_green[phase_num] )
						start_green[phase_num] = time_sec;
				}
				//else if( (green[phase_num] == 0) && (yellow[phase_num] == 1) )
				else if( (yellow[phase_num] == 1) )	//bug at RFS where green and yellow are both on for one trigger !!
				{
					signal_color[phase_num] = SIGNAL_STATE_YELLOW;
					red[phase_num] = 0;
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
					if( red[phase_num] != old_red[phase_num] )
						start_red[phase_num] = time_sec;
				}
			}

			/* If we have inputs on all 3 colors */
			else if( trig_type == ATSC_SOURCE_NTCIP )
			{
				green2[phase_num] = atsc_phase_is_set( patsc2->phase_status_greens, phase_num);
				yellow2[phase_num] = atsc_phase_is_set( patsc2->phase_status_yellows, phase_num);
				red2[phase_num] = atsc_phase_is_set( patsc2->phase_status_reds, phase_num);
#ifdef DEBUG_ATSC
				printf("phase%d green: %d yellow: %d red: %d\n",phase_num,green2[phase_num], yellow2[phase_num],red2[phase_num]);
#endif

				/* Fill signal_color by phase */
				if( green2[phase_num] == 1 )
				{
					signal_color2[phase_num] = SIGNAL_STATE_GREEN;
					if( green2[phase_num] != old_green2[phase_num] )
						start_green2[phase_num] = time_sec;
				}	
				else if( yellow2[phase_num] == 1 )
				{
					signal_color2[phase_num] = SIGNAL_STATE_YELLOW;
					if( yellow2[phase_num] != old_yellow2[phase_num] )
						start_yellow2[phase_num] = time_sec;
				}	
				else if( red2[phase_num] == 1 )
				{
					signal_color2[phase_num] = SIGNAL_STATE_RED;
					if( red2[phase_num] != old_red2[phase_num] )
						start_red2[phase_num] = time_sec;
				}	
			}
		}

                /* Print trigger times for sniffer and ntcip for GREEN turning ON */
                if( trig_type == ATSC_SOURCE_SNIFF )
                {
                        for (i = 0; i < no_phases; i++)
                        {
				if( (signal_color[phase_no[i+1]] == SIGNAL_STATE_GREEN) 
					&& (signal_color[phase_no[i+1]] != old_color[phase_no[i+1]]))
				{
                        		time_t_secs = trig_time.tv_sec;
                        		localtime_r(&time_t_secs, &tmval);
					if( verbose == 1 )
                        			printf("%2d:%02d:%02d.%03d  ",
                                			tmval.tm_hour, tmval.tm_min, tmval.tm_sec,
                                			trig_time.tv_nsec/1000000);
					else if( verbose == 2 )
					{
                				printf("%6.3f: ", cycle_sec);
                        			printf("SNIFFER ");
                        			printf("%2d:%02d:%02d.%03d  ",
                                			tmval.tm_hour, tmval.tm_min, tmval.tm_sec,
                                			trig_time.tv_nsec/1000000);
                                		printf("%d%s ",  phase_no[i+1],
                                			ix_signal_state_string(signal_color[phase_no[i+1]]));
                				printf("\n");
					}
					if( phase_no[i+1] == 2 )
					{
						sniffer_trig[0][t1_p2] = trig_time.tv_sec;
                                		sniffer_trig[1][t1_p2] = trig_time.tv_nsec/1000000;
						t1_p2++;
					}
					else
					{
						sniffer_trig[2][t1_p4] = trig_time.tv_sec;
                                		sniffer_trig[3][t1_p4] = trig_time.tv_nsec/1000000;
						t1_p4++;
					}
				}
                        }
                        for( i=1; i <= no_phases; i++ )
                        {
                                int phase_num = phase_index[tindex][i];
                                old_green[phase_num] = green[phase_num];
                                old_yellow[phase_num] = yellow[phase_num];
                                old_red[phase_num] = red[phase_num];
                                old_color[phase_no[i+1]] = signal_color[phase_no[i+1]];
                        }
                }
                else if( trig_type == ATSC_SOURCE_NTCIP )
                {
                        for (i = 0; i < no_phases; i++)
                        {
				if( (signal_color2[phase_no[i+1]] == SIGNAL_STATE_GREEN) 
					&& (signal_color2[phase_no[i+1]] != old_color2[phase_no[i+1]]))
				{
                       	 		time_t_sec2 = trig2_time.tv_sec;
                        		localtime_r(&time_t_sec2, &tmval2);
					if( verbose == 1 )
                        			printf("%2d:%02d:%02d.%03d\n",
                                			tmval2.tm_hour, tmval2.tm_min, tmval2.tm_sec,
                                			trig2_time.tv_nsec/1000000);
					else if( verbose == 2 )
					{
                				printf("%6.3f: ", cycle_sec);
                        			printf("NTCIP   ");
                        			printf("%2d:%02d:%02d.%03d  ",
                                			tmval2.tm_hour, tmval2.tm_min, tmval2.tm_sec,
                                			trig2_time.tv_nsec/1000000);
                                		printf("%d%s ",  phase_no[i+1],
                                		ix_signal_state_string(signal_color2[phase_no[i+1]]));
                				printf("\n");
					}
					if( phase_no[i+1] == 2 )
					{
						ntcip_trig[0][t2_p2] = trig2_time.tv_sec;
                                		ntcip_trig[1][t2_p2] = trig2_time.tv_nsec/1000000;
						t2_p2++;
					}
					else
					{
						ntcip_trig[2][t2_p4] = trig2_time.tv_sec;
                                		ntcip_trig[3][t2_p4] = trig2_time.tv_nsec/1000000;
						t2_p4++;
					}
				}
                        }
                        for( i=1; i <= no_phases; i++ )
                        {
                                int phase_num = phase_index[tindex][i];
                                old_green2[phase_num] = green2[phase_num];
                                old_yellow2[phase_num] = yellow2[phase_num];
                                old_red2[phase_num] = red2[phase_num];
                                old_color2[phase_no[i+1]] = signal_color2[phase_no[i+1]];
                        }
                }
               	fflush(stdout);

		/* Print the array once after one hour */
		if( (time_sec >= 60*60.0) && (print_flag==FALSE) )
		{
        		for( i=0; i < min(t1_p4,t2_p4); i++ )
			{
				/* For phase 2 */
				printf("%d.", sniffer_trig[0][i]);	/* Sniffer trigger in sec */
				printf("%03d ", sniffer_trig[1][i]);	/* Sniffer trigger in msec */
				printf("%d.", ntcip_trig[0][i]);	/* NTCIP trigger in sec */
				printf("%03d ", ntcip_trig[1][i]);	/* NTCIP trigger in msec */

				/* For phase 4 */
				printf("%d.", sniffer_trig[2][i]);	/* Sniffer trigger in sec */
				printf("%03d ", sniffer_trig[3][i]);	/* Sniffer trigger in msec */
				printf("%d.", ntcip_trig[2][i]);	/* NTCIP trigger in sec */
				printf("%03d\n", ntcip_trig[3][i]);	/* NTCIP trigger in msec */
			}
			print_flag = TRUE;
		}
	}
}

static void sig_hand( int code )
{
	if( code == SIGALRM )
		return;
	else
        	longjmp( exit_env, code );
}

