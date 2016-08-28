/* urms.c - Controls URMS running on a 2070 via ethernet
**
*/
#undef ALLOW_SET_METER
#include "urms.h"
#include "tos.h"
#include "ab3418_lib.h"

#define BUF_SIZE	sizeof(gen_mess_t)

static jmp_buf exit_env;
static int sig_list[] = {
        SIGINT,
        SIGQUIT,
        SIGTERM,
        SIGALRM,
        ERROR,
};
static void sig_hand(int code)
{
        if (code == SIGALRM)
                return;
        else
                longjmp(exit_env, code);
}

const char *usage = "-d <Database number (Modulo 4!)> -v (verbose) -r <controller IP address (def. 10.0.1.126)> -s (standalone, no DB) -u (use standalone, with DB)\n\nThe following tests are mutually exclusive, so don't mix the options by entering, for instance, a '-1' and a '-7'.  I don't check - just don't do it!\n\nFor standalone testing:\n\t-1 <lane 1 release rate (VPH)>\n\t-2 <lane 1 action (1=dark,2=rest in green,3=fixed rate,6=skip)>\n\t-3 <lane 1 plan>\n\t-4 <lane 2 release rate (VPH)>\n\t-5 <lane 2 action>\n\t-6 <lane 2 plan>\n\t-E <lane 3 release rate (VPH)>\n\t-F <lane 3 action>\n\t-G <lane 3 plan>\n\t-H <lane 4 release rate (VPH)>\n\t-I <lane 4 action>\n\t-J <lane 4 plan>\n\nFor TOS action code testing:\n\t-7 <lane 1 action code (0=skip,0x155=150 VPHPL)>\n\t-8 <lane 2 action code>\n\t-9 <lane 3 action code>\n\nTOS detector enable testing:\n\t-A <station type (1=mainline, 2=ramp,def.=2)>\n\t-B<number of lanes>\n\t-C <first logical lane (def.=1)>\n\t-D <detector enable code (mainline: 1=disabled,2=single lead,3=single trail,4=dual  ramp: recall=0,1=enable,2=red lock)>\n\n";


db_id_t db_vars_list[] =  {
        {0, sizeof(db_urms_status_t)},
        {1, sizeof(urms_datafile_t)},
        {2, sizeof(db_urms_t)},
        {3, sizeof(db_urms_status2_t)},
        {4, sizeof(db_urms_status3_t)},
};
int num_db_vars = sizeof(db_vars_list)/sizeof(db_id_t);


int db_trig_list[] =  {
        DB_URMS_VAR
};
unsigned int num_trig_variables = sizeof(db_trig_list)/sizeof(int);

static int OpenURMSConnection(char *controllerIP, char *port);
int urms_set_meter(int fd, db_urms_t *db_urms, db_urms_t *db_urms_sav, char verbose);
int urms_get_status(int fd, gen_mess_t *gen_mess, char verbose);
int tos_set_action(int fd, gen_mess_t *gen_mess, char verbose, 
	unsigned char *rm_action_code_tos2, unsigned char num_lanes);
int tos_enable_detectors(int fd, gen_mess_t *gen_mess, char verbose, 
	unsigned char *det_enable, unsigned char num_lanes, 
	unsigned char station_type, unsigned char first_logical_lane);

int main(int argc, char *argv[]) {
	int urmsfd;
	gen_mess_t gen_mess;

	char *controllerIP = "10.254.25.113";
	char *port = "1000";

        int option;

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        unsigned int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h
        posix_timer_typ *ptimer;
        trig_info_typ trig_info;
	int db_urms_status_var = 0;
	int db_urms_status2_var = 0;
	int db_urms_status3_var = 0;
	int db_urms_var = 0;
	int db_urms_datafile_var = 0;
	db_urms_t db_urms;
	db_urms_t db_urms_sav;
	db_urms_status_t db_urms_status;
	char *buf = (char *)&db_urms_status;
	db_urms_status2_t db_urms_status2;
	char *buf2 = (char *)&db_urms_status2;
	db_urms_status3_t db_urms_status3;
	char *buf3 = (char *)&db_urms_status3;
	urms_datafile_t urms_datafile;
	unsigned char rm2rmc_ctr = 0;

	int standalone = 0;
	int use_db_with_standalone = 0; // Added later to use standalone block to write to an
					// already-running urms.c
	int loop_interval = 5000; 	// Loop interval, ms
	int verbose = 0;
	int set_urms = 0;
	int get_urms = 0;
	int set_tos_action = 0;
	int set_tos_det = 0;
	int i;
	int j;
	unsigned char rm_action_code_tos2[4];
	unsigned char num_lanes = 3;
	unsigned char det_enable[8] = {1, 1, 1, 1, 1, 1, 1, 1};
	unsigned char station_type = 2; //mainline=1, ramp=2
	unsigned char first_logical_lane = 1;
	int get_status_err = 0;
	double comp_finished_temp = 0;
	double comp_finished_sav = 0;

	double curr_time = 0;
	double time_sav = 0;
	struct timespec curr_timespec;
	struct tm *ltime;
	int dow;

	int lane_1_release_rate = 0;
	unsigned char lane_1_action = 0;
	unsigned char lane_1_plan = 0;
	int lane_2_release_rate = 0;
	unsigned char lane_2_action = 0;
	unsigned char lane_2_plan = 0;
	int lane_3_release_rate = 0;
	unsigned char lane_3_action = 0;
	unsigned char lane_3_plan = 0;
	int lane_4_release_rate = 0;
	unsigned char lane_4_action = 0;
	unsigned char lane_4_plan = 0;
	unsigned char no_control = 0;
	unsigned char no_control_sav = 0;

	memset(&db_urms, 0, sizeof(db_urms_t));

printf("sizeof(db_urms_status_t) %d sizeof(db_urms_status2_t) %d sizeof(db_urms_status3_t) %d mainline_stat %d metered_lane_stat %d queue_stat %d addl_det_stat %d\n",
	sizeof(db_urms_status_t),
	sizeof(db_urms_status2_t),
	sizeof(db_urms_status3_t),
	sizeof(struct mainline_stat),
	sizeof(struct metered_lane_stat),
	sizeof(struct queue_stat),
	sizeof(struct addl_det_stat)
);
        while ((option = getopt(argc, argv, "d:r:vgsup:i:n1:2:3:4:5:6:7:8:9:A:B:C:D:E:F:G:H:I:")) != EOF) {
                switch(option) {
                case 'd':
                        db_urms_status_var = atoi(optarg);
                        db_urms_datafile_var = db_urms_status_var + 1;
                        db_urms_var = db_urms_status_var + 2;
                        db_urms_status2_var = db_urms_status_var + 3;
                        db_urms_status3_var = db_urms_status_var + 4;
                        break;
                case 'r':
			controllerIP = strdup(optarg);
                        break;
                case 'v':
                        verbose = 1;
                        break;
                case 'g':
                        get_urms = 1;
                        break;
                case 's':
                        standalone = 1;
                        break;
                case 'u':
                        use_db_with_standalone = 1;
			set_urms = 1;
                        break;
                case 'p':
                        port = strdup(optarg);
                        break;
                case 'i':
                        loop_interval = atoi(optarg);
                        break;
                case 'n':
                        no_control = 1;
                        break;
                case '1':
			lane_1_release_rate = atoi(optarg);
			set_urms = 1;
                        break;
                case '2':
			lane_1_action = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case '3':
			lane_1_plan = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case '4':
			lane_2_release_rate = atoi(optarg);
			set_urms = 1;
                        break;
                case '5':
			lane_2_action = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case '6':
			lane_2_plan = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case 'E':
			lane_3_release_rate = atoi(optarg);
			set_urms = 1;
                        break;
                case 'F':
			lane_3_action = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case 'G':
			lane_3_plan = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case 'H':
			lane_4_release_rate = atoi(optarg);
			set_urms = 1;
                        break;
                case 'I':
			lane_4_action = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case 'J':
			lane_4_plan = (unsigned char)atoi(optarg);
			set_urms = 1;
                        break;
                case '7':
			rm_action_code_tos2[0] = (unsigned char)strtol(optarg, NULL, 0);
			port = "2101";
			set_tos_action = 1;
                        break;
                case '8':
			rm_action_code_tos2[1] = (unsigned char)strtol(optarg, NULL, 0);
			port = "2101";
			set_tos_action = 1;
                        break;
                case '9':
			rm_action_code_tos2[2] = (unsigned char)strtol(optarg, NULL, 0);
			port = "2101";
			set_tos_action = 1;
                        break;
                case 'A':
			station_type = (unsigned char)atoi(optarg);
			port = "2101";
			set_tos_det = 1;
                        break;
                case 'B':
			num_lanes = (unsigned char)atoi(optarg);
			port = "2101";
                        break;
                case 'C':
			first_logical_lane = (unsigned char)atoi(optarg);
			port = "2101";
			set_tos_det = 1;
                        break;
                case 'D':
			memset(&det_enable[0], 0, sizeof(det_enable));
			det_enable[0] = (unsigned char)strtol(optarg, NULL, 0);
			for(i=1; i<num_lanes; i++)
				det_enable[i] = det_enable[0];
			port = "2101";
			set_tos_det = 1;
                        break;
                default:
                        printf("Usage: %s %s\n", argv[0], usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

	db_vars_list[0].id = db_urms_status_var;
	db_vars_list[1].id = db_urms_status_var + 1;
	db_vars_list[2].id = db_urms_status_var + 2;
	db_vars_list[3].id = db_urms_status_var + 3;
	db_vars_list[4].id = db_urms_status_var + 4;
	db_trig_list[0] = db_urms_status_var + 2;

	if(set_tos_action)
	for(i=0 ; i<num_lanes; i++)
		printf("top: rm_action_code_tos2[%d] %#hhx\n", i, rm_action_code_tos2[i]);

	if(!use_db_with_standalone) {
		// Open connection to URMS controller
		urmsfd = OpenURMSConnection(controllerIP, port);
		if(urmsfd < 0) {
			fprintf(stderr, "Could not open connection to URMS controller\n");
			exit(EXIT_FAILURE);
		}
	}
	// If just testing in standalone, write message to controller and exit
	if(standalone) {
		if(set_urms) {
		    if(!use_db_with_standalone) {
			if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
				fprintf(stderr, "Bad status command\n");
				exit(EXIT_FAILURE);
			}
		    }
		    else {
			// Connect to database
		        get_local_name(hostname, MAXHOSTNAMELEN);
//		        if ( (pclt = db_list_init(argv[0], hostname, domain, xport, db_vars_list, num_db_vars, db_trig_list, num_trig_variables)) == NULL) {
		        if ( (pclt = db_list_init(argv[0], hostname, domain, xport, NULL, 0, NULL, 0)) == NULL) {
		            exit(EXIT_FAILURE);
			}
			db_clt_read(pclt, db_urms_status_var, sizeof(db_urms_status_t), &db_urms_status);
			db_clt_read(pclt, db_urms_status2_var, sizeof(db_urms_status2_t), &db_urms_status2);
			db_clt_read(pclt, db_urms_status3_var, sizeof(db_urms_status3_t), &db_urms_status3);
			if(lane_1_release_rate != 0)
				db_urms.lane_1_release_rate = lane_1_release_rate;
			else
				db_urms.lane_1_release_rate = (db_urms_status.metered_lane_stat[0].metered_lane_rate_msb << 8) + (unsigned char)db_urms_status.metered_lane_stat[0].metered_lane_rate_lsb;
			if(lane_2_release_rate != 0)
				db_urms.lane_2_release_rate = lane_2_release_rate;
			else
				db_urms.lane_2_release_rate = (db_urms_status.metered_lane_stat[1].metered_lane_rate_msb << 8) + (unsigned char)db_urms_status.metered_lane_stat[1].metered_lane_rate_lsb;
			if(lane_3_release_rate != 0)
				db_urms.lane_3_release_rate = lane_3_release_rate;
			else
				db_urms.lane_3_release_rate = (db_urms_status.metered_lane_stat[2].metered_lane_rate_msb << 8) + (unsigned char)db_urms_status.metered_lane_stat[2].metered_lane_rate_lsb;
			if(lane_1_plan != 0)
				db_urms.lane_1_plan = lane_1_plan;
			else
				db_urms.lane_1_plan = db_urms_status3.plan[0];
			if(lane_2_plan != 0)
				db_urms.lane_2_plan = lane_2_plan;
			else
				db_urms.lane_2_plan = db_urms_status3.plan[1];
			if(lane_3_plan != 0)
				db_urms.lane_3_plan = lane_3_plan;
			else
				db_urms.lane_3_plan = db_urms_status3.plan[2];
			if(lane_1_action != 0)
				db_urms.lane_1_action = lane_1_action;
			else
				db_urms.lane_1_action = db_urms_status3.action[0];
			if(lane_2_action != 0)
				db_urms.lane_2_action = lane_2_action;
			else
				db_urms.lane_2_action = db_urms_status3.action[1];
			if(lane_3_action != 0)
				db_urms.lane_3_action = lane_3_action;
			else
				db_urms.lane_3_action = db_urms_status3.action[2];
			printf("lane 1 rate=%d lane 2 rate=%d lane 3 rate=%d\nlane 1 plan=%d lane 2 plan=%d lane 3 plan=%d\nlane 1 action=%d lane 2 action=%d lane 3 action=%d\n", 
				db_urms.lane_1_release_rate, 
				db_urms.lane_2_release_rate, 
				db_urms.lane_3_release_rate, 
				db_urms.lane_1_plan, 
				db_urms.lane_2_plan, 
				db_urms.lane_3_plan, 
				db_urms.lane_1_action, 
				db_urms.lane_2_action, 
				db_urms.lane_3_action
			);
			db_clt_write(pclt, db_urms_var, sizeof(db_urms_t), &db_urms);
                	db_list_done(pclt, NULL, 0, NULL, 0);
			exit(EXIT_SUCCESS);
		    }
			if(lane_1_release_rate != 0) {
				db_urms.lane_1_release_rate = lane_1_release_rate;
				db_urms.lane_1_action = lane_1_action;
				db_urms.lane_1_plan = lane_1_plan;
			}
			else {
				db_urms.lane_1_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
					+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_lsb);
				db_urms.lane_1_action = gen_mess.urms_status_response.metered_lane_ctl[0].action;
				db_urms.lane_1_plan = gen_mess.urms_status_response.metered_lane_ctl[0].plan;
			}
			if(lane_2_release_rate != 0) {
				db_urms.lane_2_release_rate = lane_2_release_rate;
				db_urms.lane_2_action = lane_2_action;
				db_urms.lane_2_plan = lane_2_plan;
			}
			else {
				db_urms.lane_2_release_rate = (gen_mess.urms_status_response.metered_lane_stat[1].metered_lane_rate_msb << 8) 
					+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[1].metered_lane_rate_lsb);
				db_urms.lane_2_action = gen_mess.urms_status_response.metered_lane_ctl[1].action;
				db_urms.lane_2_plan = gen_mess.urms_status_response.metered_lane_ctl[1].plan;
			}
			if(lane_3_release_rate != 0) {
				db_urms.lane_3_release_rate = lane_3_release_rate;
				db_urms.lane_3_action = lane_3_action;
				db_urms.lane_3_plan = lane_3_plan;
			}
			else {
				db_urms.lane_3_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
					+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[2].metered_lane_rate_lsb);
				db_urms.lane_3_action = gen_mess.urms_status_response.metered_lane_ctl[2].action;
				db_urms.lane_3_plan = gen_mess.urms_status_response.metered_lane_ctl[2].plan;
			}
			if(lane_4_release_rate != 0) {
				db_urms.lane_4_release_rate = lane_4_release_rate;
				db_urms.lane_4_action = lane_4_action;
				db_urms.lane_4_plan = lane_4_plan;
			}
			else {
				db_urms.lane_4_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
					+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[3].metered_lane_rate_lsb);
				db_urms.lane_4_action = gen_mess.urms_status_response.metered_lane_ctl[3].action;
				db_urms.lane_4_plan = gen_mess.urms_status_response.metered_lane_ctl[3].plan;
			}
#ifdef ALLOW_SET_METER
			if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose) < 0) {
				fprintf(stderr, "Bad meter setting command\n");
				exit(EXIT_FAILURE);
			}
#endif
		}
		if(get_urms) {
			if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
				fprintf(stderr, "Bad status command\n");
				exit(EXIT_FAILURE);
			}
		}
/*
		if( tos_get_detector_request(urmsfd, verbose) < 0) {
			fprintf(stderr, "Bad TOS status command\n");
			exit(EXIT_FAILURE);
		}
*/
		if(set_tos_det) {
			if( tos_enable_detectors(urmsfd, &gen_mess, verbose, 
			    &det_enable[0], num_lanes, station_type, 
			    first_logical_lane) < 0) {
				fprintf(stderr, "Bad TOS ENABLE DETECTORS command\n");
				exit(EXIT_FAILURE);
			}
		}
		if(set_tos_action) {
			if( tos_set_action(urmsfd, &gen_mess, verbose, &rm_action_code_tos2[0], num_lanes) < 0) {
				fprintf(stderr, "Bad TOS SET ACTION command\n");
				exit(EXIT_FAILURE);
			}
		}
		exit(EXIT_SUCCESS);
	}
	// Connect to database
        get_local_name(hostname, MAXHOSTNAMELEN);
	if ( (pclt = db_list_init(argv[0], hostname, domain, xport, NULL, 0, NULL, 0)) == NULL) {
//        if ( (pclt = db_list_init(argv[0], hostname, domain, xport, db_vars_list, num_db_vars, db_trig_list, num_trig_variables)) == NULL) {
            exit(EXIT_FAILURE);
	}

        if (setjmp(exit_env) != 0) {
		// If we can exit gracefully, tell the controller to 
		// SKIP the interconnect channel
		db_urms.lane_1_action = 6;
		db_urms.lane_2_action = 6;
		db_urms.lane_3_action = 6;
		db_urms.lane_4_action = 6;
#ifdef ALLOW_SET_METER
		if(no_control == 0)
			urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose);
#endif
                close(urmsfd);
                exit(EXIT_SUCCESS);
        } else
               sig_ign(sig_list, sig_hand);

        if ((ptimer = timer_init( loop_interval, ChannelCreate(0) )) == NULL) {
                fprintf(stderr, "Unable to initialize delay timer\n");
                exit(EXIT_FAILURE);
        }
	
	//Initialize the saved copy of db_urms_sav with the current values in the controller
	// ALL of the parameters MUST be set!
	if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
		fprintf(stderr, "Bad status command\n");
		exit(EXIT_FAILURE);
	}
	if(lane_1_release_rate != 0) {
		db_urms_sav.lane_1_release_rate = lane_1_release_rate;
		db_urms_sav.lane_1_action = lane_1_action;
		db_urms_sav.lane_1_plan = lane_1_plan;
	}
	else {
		db_urms_sav.lane_1_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
			+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_lsb);
		db_urms_sav.lane_1_action = gen_mess.urms_status_response.metered_lane_ctl[0].action;
		db_urms_sav.lane_1_plan = gen_mess.urms_status_response.metered_lane_ctl[0].plan;
	}
	if(lane_2_release_rate != 0) {
		db_urms_sav.lane_2_release_rate = lane_2_release_rate;
		db_urms_sav.lane_2_action = lane_2_action;
		db_urms_sav.lane_2_plan = lane_2_plan;
	}
	else {
		db_urms_sav.lane_2_release_rate = (gen_mess.urms_status_response.metered_lane_stat[1].metered_lane_rate_msb << 8) 
			+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[1].metered_lane_rate_lsb);
		db_urms_sav.lane_2_action = gen_mess.urms_status_response.metered_lane_ctl[1].action;
		db_urms_sav.lane_2_plan = gen_mess.urms_status_response.metered_lane_ctl[1].plan;
	}
	if(lane_3_release_rate != 0) {
		db_urms_sav.lane_3_release_rate = lane_3_release_rate;
		db_urms_sav.lane_3_action = lane_3_action;
		db_urms_sav.lane_3_plan = lane_3_plan;
	}
	else {
		db_urms_sav.lane_3_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
			+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[2].metered_lane_rate_lsb);
		db_urms_sav.lane_3_action = gen_mess.urms_status_response.metered_lane_ctl[2].action;
		db_urms_sav.lane_3_plan = gen_mess.urms_status_response.metered_lane_ctl[2].plan;
	}
	if(lane_4_release_rate != 0) {
		db_urms_sav.lane_4_release_rate = lane_4_release_rate;
		db_urms_sav.lane_4_action = lane_4_action;
		db_urms_sav.lane_4_plan = lane_4_plan;
	}
	else {
		db_urms_sav.lane_4_release_rate = (gen_mess.urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) 
			+ (unsigned char) (gen_mess.urms_status_response.metered_lane_stat[3].metered_lane_rate_lsb);
		db_urms_sav.lane_4_action = gen_mess.urms_status_response.metered_lane_ctl[3].action;
		db_urms_sav.lane_4_plan = gen_mess.urms_status_response.metered_lane_ctl[3].plan;
	}

	while(1) {
                // Now wait for a trigger. 
                // Data or timer, it doesn't matter - we read and write on both
		// BTW, if you're using clt_ipc_receive, you just
		// need to do the timer_init. Do NOT also use 
		// TIMER_WAIT; it'll add another timer interval to
		// the loop.
		clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		if( DB_TRIG_VAR(&trig_info) == db_urms_var ) {
			db_clt_read(pclt, db_urms_var, sizeof(db_urms_t), &db_urms);
			printf("db_clt_read: db_urms_var %d\n", db_urms_var);
			if(verbose)
				printf("Got db_urms_var trigger\n");
			if(no_control == 0)
				{
				db_urms.lane_1_action = 6;
#ifdef ALLOW_SET_METER
				if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose) < 0) {
					fprintf(stderr, "Bad meter setting command\n");
				}
#endif
			}
		}
		else {
		     if(!use_db_with_standalone) {
				// Open connection to URMS controller
				urmsfd = OpenURMSConnection(controllerIP, port);
				if(urmsfd < 0) {
					fprintf(stderr, "Could not open connection to URMS controller\n");
					exit(EXIT_FAILURE);
				}
		    }
		    if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
			fprintf(stderr, "Bad status command\n");
			get_status_err++;
		    }
		    else {
			if( clock_gettime(CLOCK_REALTIME, &curr_timespec) < 0)
				perror("urms clock_gettime");
			curr_time = curr_timespec.tv_sec + (curr_timespec.tv_nsec / 1000000000.0);

                        memcpy(&db_urms_status.mainline_stat[0], &gen_mess.urms_status_response.mainline_stat[0], sizeof(struct mainline_stat)*8);
                        memcpy(&db_urms_status.metered_lane_stat[0], &gen_mess.urms_status_response.metered_lane_stat[0], sizeof(struct metered_lane_stat)*4);
                        memcpy(&db_urms_status2.queue_stat[0][0], &gen_mess.urms_status_response.queue_stat[0][0], sizeof(struct queue_stat)*16);
                        memcpy(&db_urms_status3.additional_det[0], &gen_mess.urms_status_response.additional_det[0], sizeof(struct addl_det_stat)*16);

			db_urms_status3.rm2rmc_ctr = rm2rmc_ctr++;
			db_urms_status.num_meter = gen_mess.urms_status_response.num_meter;
			db_urms_status.num_main = gen_mess.urms_status_response.num_main;
			db_urms_status3.num_opp = gen_mess.urms_status_response.num_opp;
			db_urms_status3.num_addl_det = gen_mess.urms_status_response.num_addl_det;
			db_urms_status3.mainline_dir = gen_mess.urms_status_response.mainline_dir;
			db_urms_status3.is_metering = gen_mess.urms_status_response.is_metering;
			db_urms_status.hour = gen_mess.urms_status_response.hour;
			db_urms_status.minute = gen_mess.urms_status_response.minute;
			db_urms_status.second = gen_mess.urms_status_response.second;
			comp_finished_temp = 0;

			ltime = localtime(&curr_timespec.tv_sec);
			dow = ltime->tm_wday;
//			printf("dow=%d dow%%6=%d hour %d\n", dow, dow % 6, db_urms_status.hour);

			if( ((dow % 6) == 0) || (db_urms_status.hour < 15) || (db_urms_status.hour >= 19) || (db_urms.no_control != 0)) {
				no_control = 1;
				if( no_control_sav == 0) {
                                printf("%02d/%02d/%04d %02d:%02d:%02d Disabling control of ramp meter controller: hour=%d no_control %d DOW=%d\n",
                                        ltime->tm_mon+1,
                                        ltime->tm_mday,
                                        ltime->tm_year+1900,
                                        ltime->tm_hour,
                                        ltime->tm_min,
                                        ltime->tm_sec,
                                        db_urms_status.hour,
                                        db_urms_status.no_control,
                                        dow
                                );
					no_control_sav = 1;
					db_urms.lane_1_action = 6;
					db_urms.lane_2_action = 6;
					db_urms.lane_3_action = 6;
#ifdef ALLOW_SET_METER
				if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose) < 0) {
					fprintf(stderr, "Bad meter setting command\n");
				}
#endif
				}
			}
			else {
				no_control = 0;
				if( no_control_sav == 1) {
					printf("%02d/%02d/%04d %02d:%02d:%02d Enabling control of ramp meter controller: hour=%d no_control %d DOW=%d\n",
						ltime->tm_mon+1,
						ltime->tm_mday,
						ltime->tm_year+1900,
						ltime->tm_hour,
						ltime->tm_min,
						ltime->tm_sec,
						db_urms_status.hour,
						db_urms_status.no_control,
						dow
					);
					no_control_sav = 0;
				}
			}

			for(i = 0; i < MAX_METERED_LANES; i++) {
//printf("urms.c: interval_zone %d %d rate %hu\n", i,db_urms_status.metered_lane_stat[i].metered_lane_interval_zone, (db_urms_status.metered_lane_stat[i].metered_lane_rate_msb << 8) + (unsigned char)db_urms_status.metered_lane_stat[i].metered_lane_rate_lsb);
			    comp_finished_temp += db_urms_status.metered_lane_stat[i].demand_vol + 
						db_urms_status.metered_lane_stat[i].passage_vol + 
						db_urms_status.mainline_stat[i].lead_vol + 
						db_urms_status.mainline_stat[i].trail_vol + 
						db_urms_status2.queue_stat[i][0].vol; 
			    if( (comp_finished_temp != comp_finished_sav) ||
			    	((curr_time + 0.025 - time_sav) >= 30.0) ) { //The 0.025 is one-half the loop interval of wrfiles_ac_rm. This was done only so
									 // that the printed times are synchronized.  However, Dongyan's code will be using
									 // the printed string as input, so this has a real use.
				db_urms_status3.computation_finished = 1;
				time_sav = curr_time;
				if( comp_finished_temp != comp_finished_sav ) {
					comp_finished_sav = comp_finished_temp;
				}
				if(verbose)
					printf("timediff %lf comp_finished_diff %lf\n", curr_time - time_sav, comp_finished_temp - comp_finished_temp);
			
			    }
			    else
				db_urms_status3.computation_finished = 0;
			}

			for(i = 0; i < MAX_MAINLINES; i++) {
			    db_urms_status3.cmd_src[i] = gen_mess.urms_status_response.metered_lane_ctl[i].cmd_src;
			    db_urms_status3.action[i] = gen_mess.urms_status_response.metered_lane_ctl[i].action;
			    db_urms_status3.plan[i] = gen_mess.urms_status_response.metered_lane_ctl[i].plan;
			    db_urms_status3.plan_base_lvl[i] = gen_mess.urms_status_response.metered_lane_ctl[i].plan_base_lvl;
			    urms_datafile.mainline_lead_occ[i] = 0.1 * ((gen_mess.urms_status_response.mainline_stat[i].lead_occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.mainline_stat[i].lead_occ_lsb));
			    urms_datafile.mainline_trail_occ[i] = 0.1 * ((gen_mess.urms_status_response.mainline_stat[i].trail_occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.mainline_stat[i].trail_occ_lsb));
			    urms_datafile.queue_occ[i] = 0.1 * ((gen_mess.urms_status_response.queue_stat[0][i].occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.queue_stat[0][i].occ_lsb));
			    urms_datafile.metering_rate[i] = ((gen_mess.urms_status_response.metered_lane_stat[i].metered_lane_rate_msb << 8) + (unsigned char)(gen_mess.urms_status_response.metered_lane_stat[i].metered_lane_rate_lsb));
			    if(verbose) {
				printf("1:ML%d occ %.1f ", i+1, urms_datafile.mainline_lead_occ[i]);
			 	printf("1:MT%d occ %.1f ", i+1, urms_datafile.mainline_trail_occ[i]);
				printf("1:Q%d-1 occ %.1f ", i+1, urms_datafile.queue_occ[i]);
				printf("1:lane %d cmd_src %hhu ", i+1, db_urms_status3.cmd_src[i]);
				printf("1:lane %d action %hhu\n", i+1, db_urms_status3.action[i]);
			    }
			    if(verbose) {
				printf("2:ML%d occ %.1f ", i+1, urms_datafile.mainline_lead_occ[i]);
			 	printf("2:MT%d occ %.1f ", i+1, urms_datafile.mainline_trail_occ[i]);
				printf("2:Q%d-1 occ %.1f\n", i+1, urms_datafile.queue_occ[i]);
			    }
			}

			if(verbose) {
				for(i=0; i < sizeof(db_urms_status_t); i++)
					printf("%d:%#hhx ", i, buf[i]);
				for(i=0; i < sizeof(db_urms_status2_t); i++)
					printf("%d:%#hhx ", i, buf2[i]);
				for(i=0; i < sizeof(db_urms_status3_t); i++)
					printf("%d:%#hhx ", i, buf3[i]);
				printf("\n");
			}

			get_current_timestamp(&db_urms_status2.ts);
			db_clt_write(pclt, db_urms_status_var, sizeof(db_urms_status_t), &db_urms_status);
			db_clt_write(pclt, db_urms_status_var + 1, sizeof(urms_datafile_t), &urms_datafile);
			db_clt_write(pclt, db_urms_status_var + 3, sizeof(db_urms_status2_t), &db_urms_status2);
			db_clt_write(pclt, db_urms_status_var + 4, sizeof(db_urms_status3_t), &db_urms_status3);
		    }
		}
		close(urmsfd);
	}
}

static int OpenURMSConnection(char *controllerIP, char *port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;	   /* Any protocol */
	s = getaddrinfo(controllerIP, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully connect(2).
	If socket(2) (or connect(2)) fails, we (close the socket
	and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			perror("socket");
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;		    /* Success */
		}
		perror("connect");
		close(sfd);
	}
	if (rp == NULL) {		 /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		return -1;
	}
	freeaddrinfo(result);	    /* No longer needed */
	return sfd;
}


int urms_set_meter(int fd, db_urms_t *db_urms, db_urms_t *db_urms_sav, char verbose) {
	gen_mess_t gen_mess;
	char *msgbuf = (char *)&gen_mess;
	int csum;
	int i;
	int nread;
	char buf[BUF_SIZE];
	timestamp_t ts;

	// Initialize URMS message
	// Lane 1: 900 VPH release rate, fixed rate, plan 1
	// Lane 2: 900 VPH release rate, fixed rate, plan 1
	memset(&gen_mess, 0, sizeof(gen_mess_t));
	gen_mess.urmsctl.addr = 2;
	gen_mess.urmsctl.hdr1 = 0xe;
	gen_mess.urmsctl.hdr2 = 0x15;
	gen_mess.urmsctl.hdr3 = 0x70;
	gen_mess.urmsctl.hdr4 = 0x01;
	gen_mess.urmsctl.hdr5 = 0x11;
	gen_mess.urmsctl.tail0 = 0x00;
	gen_mess.urmsctl.tail1 = 0xaa;
	gen_mess.urmsctl.tail2 = 0x55;

	// If requested metering rate is 0, assume no change
	// ALL of the parameters MUST be set!
/*
	if(db_urms->lane_1_release_rate == 0) {
		db_urms->lane_1_release_rate = db_urms_sav->lane_1_release_rate;
		db_urms->lane_1_action = db_urms_sav->lane_1_action;
		db_urms->lane_1_plan = db_urms_sav->lane_1_plan;
	}
	if(db_urms->lane_2_release_rate == 0) {
		db_urms->lane_2_release_rate = db_urms_sav->lane_2_release_rate;
		db_urms->lane_2_action = db_urms_sav->lane_2_action;
		db_urms->lane_2_plan = db_urms_sav->lane_2_plan;
	}
	if(db_urms->lane_3_release_rate == 0) {
		db_urms->lane_3_release_rate = db_urms_sav->lane_3_release_rate;
		db_urms->lane_3_action = db_urms_sav->lane_3_action;
		db_urms->lane_3_plan = db_urms_sav->lane_3_plan;
	}
	if(db_urms->lane_4_release_rate == 0) {
		db_urms->lane_4_release_rate = db_urms_sav->lane_4_release_rate;
		db_urms->lane_4_action = db_urms_sav->lane_4_action;
		db_urms->lane_4_plan = db_urms_sav->lane_4_plan;
	}
*/
	gen_mess.urmsctl.lane_1_action = db_urms->lane_1_action;
	gen_mess.urmsctl.lane_1_plan = db_urms->lane_1_plan;
	gen_mess.urmsctl.lane_1_release_rate_MSB = 
		(db_urms->lane_1_release_rate & 0xFF00) >> 8;
	gen_mess.urmsctl.lane_1_release_rate_LSB = 
		(db_urms->lane_1_release_rate & 0x00FF);
	gen_mess.urmsctl.lane_2_action = db_urms->lane_2_action;
	gen_mess.urmsctl.lane_2_plan = db_urms->lane_2_plan;
	gen_mess.urmsctl.lane_2_release_rate_MSB = 
		(db_urms->lane_2_release_rate & 0xFF00) >> 8;
	gen_mess.urmsctl.lane_2_release_rate_LSB = 
		(db_urms->lane_2_release_rate & 0x00FF);
	gen_mess.urmsctl.lane_3_action = db_urms->lane_3_action;
	gen_mess.urmsctl.lane_3_plan = db_urms->lane_3_plan;
	gen_mess.urmsctl.lane_3_release_rate_MSB = 
		(db_urms->lane_3_release_rate & 0xFF00) >> 8;
	gen_mess.urmsctl.lane_3_release_rate_LSB = 
		(db_urms->lane_3_release_rate & 0x00FF);
	gen_mess.urmsctl.lane_4_action = db_urms->lane_4_action;
	gen_mess.urmsctl.lane_4_plan = db_urms->lane_4_plan;
	gen_mess.urmsctl.lane_4_release_rate_MSB = 
		(db_urms->lane_4_release_rate & 0xFF00) >> 8;
	gen_mess.urmsctl.lane_4_release_rate_LSB = 
		(db_urms->lane_4_release_rate & 0x00FF);
	csum = 0;
	for(i=0; i<=20; i++)
		csum += msgbuf[i];
	msgbuf[22] = csum;
	printf("urms_set_meter: lane 1 action %d lane 2 action %d lne 3 action %d\n", gen_mess.urmsctl.lane_1_action, gen_mess.urmsctl.lane_2_action, gen_mess.urmsctl.lane_3_action);
	if (write(fd, msgbuf, sizeof(urmsctl_t)) != sizeof(urmsctl_t)) {
	  fprintf(stderr, "urms_set_meter: partial/failed write\n");
	  return -2;
	}

	nread = read(fd, buf, BUF_SIZE);
	if (nread == -1) {
	 perror("read");
	}
	if(verbose) {
	    get_current_timestamp(&ts);
	    print_timestamp(stdout, &ts);
	    printf(" Received %ld bytes:", (long) nread);
	    for(i = 0; i < nread; i++)	
	     printf(" %#.2hhX", buf[i]);
	    printf("\n");
	}

	if( (buf[0] + buf[1])!= buf[2])
		return -1;
	return 0;
}


int urms_get_status(int fd, gen_mess_t *gen_mess, char verbose) {

	char *msgbuf = (char *)&gen_mess->urms_status_poll;
	int csum;
	int i;
	int j;
	int nread;
	char *buf = &gen_mess->data[0];
	timestamp_t ts;
	struct timespec start_time;
	struct timespec end_time;
	int msg_len = 0;

	memset(gen_mess, 0, sizeof(gen_mess_t));
	gen_mess->urms_status_poll.msg_type = 0x51;
	gen_mess->urms_status_poll.drop_num = 2;
	gen_mess->urms_status_poll.num_bytes = 8;
	gen_mess->urms_status_poll.action = 6;
	gen_mess->urms_status_poll.rate_msb = 0;
	gen_mess->urms_status_poll.rate_lsb = 200;
	gen_mess->urms_status_poll.plan = 2;
	gen_mess->urms_status_poll.gp_out = 0;
	gen_mess->urms_status_poll.future_use = 0;
	gen_mess->urms_status_poll.enable = 0;
	gen_mess->urms_status_poll.checksum_msb = 0;
	gen_mess->urms_status_poll.checksum_lsb = 0;

	csum = 0;
	for(i=0; i < (sizeof(urms_status_poll_t) - 2); i++)
		csum += msgbuf[i];
	gen_mess->urms_status_poll.checksum_msb = ((csum >> 8) & 0xFF);
	gen_mess->urms_status_poll.checksum_lsb = (csum  & 0xFF);

	clock_gettime(CLOCK_REALTIME, &start_time);

//	Uncomment this block to use the URMSPOLL2 message instead of the URMS POLL
//	message.  The former doesn't work. It was supposed to keep the 2070 interconnect
//	channel from timing out, but is broken.
	gen_mess->urmspoll2[0] = 'U';
	gen_mess->urmspoll2[1] = 'R';
	gen_mess->urmspoll2[2] = 'M';
	gen_mess->urmspoll2[3] = 'S';
	gen_mess->urmspoll2[4] = 'P';
	gen_mess->urmspoll2[5] = 'O';
	gen_mess->urmspoll2[6] = 'L';
	gen_mess->urmspoll2[7] = 'L';
	gen_mess->urmspoll2[8] = '2';
	if (write(fd, &gen_mess->urmspoll2, 9) != 9) {

//	if (write(fd, &gen_mess->urms_status_poll, sizeof(urms_status_poll_t)) != sizeof(urms_status_poll_t)) {
	  fprintf(stderr, "urms_get_status: partial/failed write\n");
//	  exit(EXIT_FAILURE);
	}

	nread = read(fd, gen_mess, 417);
	clock_gettime(CLOCK_REALTIME, &end_time);
	if (nread == -1) {
	 perror("read");
	}

        // Now append the FCS.
	msg_len = 417 - 4;
//	check_modframe_string( (unsigned char *)(&gen_mess+1), &msg_len); 

	if(verbose) {
	    get_current_timestamp(&ts);
	    print_timestamp(stdout, &ts);
	    j = 0;
	    printf(" Received %ld bytes:\n", (long) nread);
	    for(i = 0; i < nread; i += 10){	
	    	printf("%3.3d ", i);
		for(j = 0; (j < 10) && ((j+i) < nread); j++)
	    		printf("%#2.2hhx ", buf[i+j]);
	    	printf("\n");
	    }
    	printf("urms_get_status: Time for function call %f sec\n", (end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - (start_time.tv_sec + (start_time.tv_nsec/1.0e9)));
	printf("ML1 speed %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[0].speed, 0.1 * ((gen_mess->urms_status_response.mainline_stat[0].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[0].lead_occ_lsb));
	printf("ML2 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[1].lead_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[1].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[1].lead_occ_lsb));
	printf("MT2 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[1].trail_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[1].trail_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[1].trail_occ_lsb));
	printf("ML3 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[2].lead_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[2].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[2].lead_occ_lsb));
	printf("MT3 vol %hhu occ %.1f \n", gen_mess->urms_status_response.mainline_stat[2].trail_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[2].trail_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[2].trail_occ_lsb));
	}
	return 0;
}

unsigned char rm_action_code_tos2; //	{SKIP = 0, REST_IN_DARK, REST_IN_GREEN,
				   //	0x15 – 0x90,	fixed rate in BCD vphpl * 10
				   //	0xD1 – 0xD6,	traffic responsive occupancy only
				   //	0xE1 – 0xE6,	traffic responsive flow only
				   //	0xF1 – 0xF6}	traffic responsive occupancy / flow
				   //	other values not assigned

int tos_set_action(int fd, gen_mess_t *gen_mess, char verbose, unsigned char *rm_action_code_tos2, unsigned char num_lanes) {

	int i;
	timestamp_t ts;
	struct timespec start_time;
	struct timespec end_time;
	int msg_len;


	memset(gen_mess, 0, sizeof(gen_mess_t));
	gen_mess->tos_set_action.start_flag = 0x7e;
	gen_mess->tos_set_action.address = 0x02;
	gen_mess->tos_set_action.comm_code = 'D';
	for(i = 0; i < num_lanes; i++) {
		gen_mess->tos_set_action.action[i] = rm_action_code_tos2[i];
		printf("rm_action_code_tos2[%d] %hhx\n", i, rm_action_code_tos2[i]);
	}
	gen_mess->tos_set_action.FCSmsb = 0x00;
	gen_mess->tos_set_action.FCSlsb = 0x00;
	gen_mess->tos_set_action.end_flag = 0x7e;

        // Now append the FCS.
	msg_len = sizeof(tos_set_action_t) - 4;
	fcs_hdlc(msg_len, &gen_mess->tos_set_action, verbose);

	clock_gettime(CLOCK_REALTIME, &start_time);
	if (write(fd, &gen_mess->tos_set_action, sizeof(tos_set_action_t)) != sizeof(tos_set_action_t)) {
	  fprintf(stderr, "tos_set_action: partial/failed write\n");
	  exit(EXIT_FAILURE);
	}
	clock_gettime(CLOCK_REALTIME, &end_time);

	if(verbose) {
	    get_current_timestamp(&ts);
	    print_timestamp(stdout, &ts);
	    printf("urms_get_status: Time for function call %f sec\n", (end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - (start_time.tv_sec + (start_time.tv_nsec/1.0e9)));
	    fflush(NULL);
	}

	return 0;
}

int tos_enable_detectors(int fd, gen_mess_t *gen_mess, char verbose, unsigned char *det_enable, unsigned char num_lanes, unsigned char station_type, unsigned char first_logical_lane) {

	char *msgbuf = (char *)&gen_mess->tos_enable_detectors;
	int csum;
	int i;
	int nread;
	char buf[BUF_SIZE];
	timestamp_t ts;
	struct timespec start_time;
	struct timespec end_time;


	memset(gen_mess, 0, sizeof(gen_mess_t));
	gen_mess->tos_enable_detectors.start_flag = 0x7e;
	gen_mess->tos_enable_detectors.address = 0x02;
	gen_mess->tos_enable_detectors.comm_code = 'E';
	gen_mess->tos_enable_detectors.station_type = station_type;
	gen_mess->tos_enable_detectors.first_logical_lane= first_logical_lane;
	gen_mess->tos_enable_detectors.num_lanes = num_lanes;
	for(i = 0; i < num_lanes; i++) {
		gen_mess->tos_enable_detectors.det_enable[i] = det_enable[i];
		printf("detector_enable[%d] %hhx\n", i, det_enable[i]);
	}
	gen_mess->tos_enable_detectors.FCSmsb = 0x00;
	gen_mess->tos_enable_detectors.FCSlsb = 0x00;
	gen_mess->tos_enable_detectors.end_flag = 0x7e;

	csum = 0;
	for(i=0; i < (sizeof(tos_enable_detectors_t) - 2); i++)
		csum += msgbuf[i];
	gen_mess->tos_enable_detectors.FCSmsb = ((csum >> 8) & 0xFF);
	gen_mess->tos_enable_detectors.FCSlsb = (csum  & 0xFF);

	clock_gettime(CLOCK_REALTIME, &start_time);

	if (write(fd, &gen_mess->tos_enable_detectors, sizeof(tos_enable_detectors_t)) != sizeof(tos_enable_detectors_t)) {
	  fprintf(stderr, "tos_enable_detectors: partial/failed write\n");
	  exit(EXIT_FAILURE);
	}

	nread = read(fd, buf, BUF_SIZE);
	clock_gettime(CLOCK_REALTIME, &end_time);
	if (nread == -1) {
	 perror("read");
	}
	if(verbose) {
	    get_current_timestamp(&ts);
//	    print_timestamp(stdout, &ts);
//	    printf(" Received %ld bytes:", (long) nread);
	    for(i = 0; i < 49; i++)	
	        printf("%.2hhX ", buf[i]);
//	    printf("%d %#.2hhX\n", i, buf[i]);
	    printf("\n");
//	    printf("tos_enable_detectors: Time for function call %f sec\n", (end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - (start_time.tv_sec + (start_time.tv_nsec/1.0e9)));
	}

	return 0;
}

/*
int tos_get_date ( int fd, gen_mess_typ *readBuff, char verbose) {

	tos_get_date_request_t tos_get_date_request;
        int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
        int ser_driver_retval;
        tsmss_get_msg_request_t tos_get_date_request;

        if(verbose != 0)
                printf("get_overlap 1: Starting get_overlap request\n");
        tos_get_date_request.get_hdr.start_flag = 0x7e;
        tos_get_date_request.get_hdr.address = 0x05;
        tos_get_date_request.get_hdr.comm_code = CODE_GET_DATE;
        tos_get_date_request.get_tail.FCSmsb = 0x00;
        tos_get_date_request.get_tail.FCSlsb = 0x00;

         Now append the FCS.
        msg_len = sizeof(tos_get_date_request_t) - 4;
        fcs_hdlc(msg_len, &tos_get_date_request, verbose);

        FD_ZERO(&writefds);
        FD_SET(fpout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select");
                outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
                printf("get_overlap 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
                return -3;
        }
        write ( fpout, &tos_get_date_request, sizeof(tos_get_date_request_t);
        fflush(NULL);
        sleep(2);

        ser_driver_retval = 100;

        if(readBuff) {
                FD_ZERO(&readfds);
                FD_SET(fpin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select");
                        inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
                        printf("get_overlap 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
                if(ser_driver_retval == 0) {
                        printf("get_overlap 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("get_overlap 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
        return 0;
}
*/
