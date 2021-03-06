/* urms.c - Controls URMS running on a 2070 via ethernet
**	This program runs in two distinct modes: 
**		1. Reading the controller status and sending it to the database
**		   Reading the database for commands to the controller and sending that on
**		2. Reading command line arguments for sending metering rates to the controller
**		   without the database running.
**	There is a third mode that is taken care of by a separate program, urms_db_test. That program assumes urms
**	running with the database, and writes command-line arguments to the database to check the database-to-controller
**	chain.
*/

#define ALLOW_SET_METER 1
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

const char *usage = "\n\t-r <controller IP address (def. 10.254.25.113)> \n\t-p <port (def. 1000)>\n\t-a <controller address, def. 1>\n\t-d <Database number (Modulo 4!)> \n\t-v (verbose) \n\t-s (standalone, no DB) \n\t-g (Get the controller status; use with -s and -v to just print out the status to screen without using the database)\n\nThe following tests are mutually exclusive, so don't mix the options by entering, for instance, a '-1' and a '-7'.  I don't check - just don't do it!\n\nFor standalone testing:\n\t-1 <lane 1 release rate (VPH)>\n\t-2 <lane 1 action (1=dark,2=rest in green,3=fixed rate,6=skip)>\n\t-3 <lane 1 plan>\n\t-4 <lane 2 release rate (VPH)>\n\t-5 <lane 2 action>\n\t-6 <lane 2 plan>\n\t-E <lane 3 release rate (VPH)>\n\t-F <lane 3 action>\n\t-G <lane 3 plan>\n\t-H <lane 4 release rate (VPH)>\n\t-I <lane 4 action>\n\t-J <lane 4 plan>\n\nFor TOS action code testing:\n\t-7 <lane 1 action code (0=skip,0x155=150 VPHPL)>\n\t-8 <lane 2 action code>\n\t-9 <lane 3 action code>\n\nTOS detector enable testing:\n\t-A <station type (1=mainline, 2=ramp,def.=2)>\n\t-B<number of lanes>\n\t-C <first logical lane (def.=1)>\n\t-D <detector enable code (mainline: 1=disabled,2=single lead,3=single trail,4=dual  ramp: recall=0,1=enable,2=red lock)>\n\n";


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
int urms_set_meter(int fd, db_urms_t *db_urms, db_urms_t *db_urms_sav, char verbose, char addr_2070);
int urms_get_status(int fd, gen_mess_t *gen_mess, char verbose);
int tos_set_action(int fd, gen_mess_t *gen_mess, char verbose, 
	unsigned char *rm_action_code_tos2, unsigned char num_lanes);
int tos_enable_detectors(int fd, gen_mess_t *gen_mess, char verbose, 
	unsigned char *det_enable, unsigned char num_lanes, 
	unsigned char station_type, unsigned char first_logical_lane);

typedef struct {
	char *name;
	float postmile;
	char *ipaddr;
	char *port;
}controller_def_t;

typedef struct{
	//Index,Onramp_Names,Controller_IP_Address,Days,Time_Interval_Start,Time_Interval_End,Minimum_RM_Rate,Maximum_RM_Rate
	//Elk_Grove_Blvd,10.253.28.97,Wednesday,1000,1200,111,212
	char *name;
	char *ipaddr;
	char *datestr;
	int interval_start;
	int interval_end;
	short min_rm_rate;
	short max_rm_rate;
} time_interval_config_t;

typedef struct{
	controller_def_t rm_controller;
	char *crm_enabled;
	controller_def_t data_controller[3];
}section_def_t;

typedef struct {
	short max_rate;
	short min_rate;
	char *am_start_time;
	char *am_end_time;
	char *pm_start_time;
	char *pm_end_time;
	char rt_vs_history;
	char q_override;
} config_t;

int strarr2sectiondef(char strarr[][20], section_def_t *section_def, int verbose) {

	printf("strarr2sectiondef: Got to 1\n");
	printf("strarr2sectiondef: Got to 2 strarr[0][0] %s\n", &strarr[0][0]);

	//Ramp meter controller
	section_def->rm_controller.name = &strarr[0][0];
	if(section_def->rm_controller.name == NULL) {
		fprintf(stderr, "No onramp name\n");
		return -1;
	}
	
	section_def->rm_controller.postmile = strtof(&strarr[1][0], NULL);

	section_def->rm_controller.ipaddr = &strarr[2][0];
	if(inet_addr(section_def->rm_controller.ipaddr) == INADDR_NONE) {
		fprintf(stderr, "Invalid ramp meter controller IP address\n");
		return -1;
	}	

	section_def->rm_controller.port = &strarr[3][0];
	if(section_def->rm_controller.port == NULL) {
		fprintf(stderr, "No port string\n");
		return -1;
	}

	section_def->crm_enabled = &strarr[4][0];
	if(section_def->crm_enabled == NULL) {
		fprintf(stderr, "No crm enable string\n");
		return -1;
	}

	//Data controller #1
	section_def->data_controller[0].name = &strarr[5][0];
	if(section_def->data_controller[0].name == NULL) {
		fprintf(stderr, "No onramp name\n");
	}
	
	section_def->data_controller[0].postmile = strtof(&strarr[6][0], NULL);

	section_def->data_controller[0].ipaddr = &strarr[7][0];
	if(inet_addr(section_def->data_controller[0].ipaddr) == INADDR_NONE) {
		printf("Invalid IP address\n");
	}	

	section_def->data_controller[0].port = &strarr[8][0];
	if(section_def->data_controller[0].port == NULL) {
		fprintf(stderr, "No port string\n");
	}

	//Data controller #2
	section_def->data_controller[1].name = &strarr[9][0];
	if(section_def->data_controller[1].name == NULL) {
		fprintf(stderr, "No onramp name\n");
	}
	
	section_def->data_controller[1].postmile = strtof(&strarr[10][0], NULL);

	section_def->data_controller[1].ipaddr = &strarr[11][0];
	if(inet_addr(section_def->data_controller[1].ipaddr) == INADDR_NONE) {
		printf("Invalid IP address\n");
	}	

	section_def->data_controller[1].port = &strarr[12][0];
	if(section_def->data_controller[1].port == NULL) {
		fprintf(stderr, "No port string\n");
	}

	//Data controller #3
	section_def->data_controller[2].name = &strarr[13][0];
	if(section_def->data_controller[2].name == NULL) {
		fprintf(stderr, "No onramp name\n");
	}
	
	section_def->data_controller[2].postmile = strtof(&strarr[14][0], NULL);

	section_def->data_controller[2].ipaddr = &strarr[15][0];
	if(inet_addr(section_def->data_controller[2].ipaddr) == INADDR_NONE) {
		printf("Invalid IP address\n");
	}	

	section_def->data_controller[2].port = &strarr[16][0];
	if(section_def->data_controller[2].port == NULL) {
		fprintf(stderr, "No port string\n");
	}
/*
	section_def->rt_vs_history = atoi(&strarr[11][0]);
	section_def->q_override = atoi(&strarr[12][0]);
*/
	if(verbose) {
		printf("section_def->rm_controller.name %s\n \
			section_def->rm_controller.postmile %f\n \
			section_def->rm_controller.ipaddr %s\n \
			section_def->rm_controller.port %s\n \
			section_def->crm_enabled %s\n \
			section_def->data_controller[0].name %s\n \
			section_def->data_controller[0].postmile %f\n \
			section_def->data_controller[0].ipaddr %s\n \
			section_def->data_controller[0].port %s\n \
			section_def->data_controller[1].name %s\n \
			section_def->data_controller[1].postmile %f\n \
			section_def->data_controller[1].ipaddr %s\n \
			section_def->data_controller[1].port %s\n \
			section_def->data_controller[2].name %s\n \
			section_def->data_controller[2].postmile %f\n \
			section_def->data_controller[2].ipaddr %s\n \
			section_def->data_controller[2].port %s\n",

			section_def->rm_controller.name,
			section_def->rm_controller.postmile,
			section_def->rm_controller.ipaddr,
			section_def->rm_controller.port,
			section_def->crm_enabled,
			section_def->data_controller[0].name,
			section_def->data_controller[0].postmile,
			section_def->data_controller[0].ipaddr,
			section_def->data_controller[0].port,
			section_def->data_controller[1].name,
			section_def->data_controller[1].postmile,
			section_def->data_controller[1].ipaddr,
			section_def->data_controller[1].port,
			section_def->data_controller[2].name,
			section_def->data_controller[2].postmile,
			section_def->data_controller[2].ipaddr,
			section_def->data_controller[2].port
		);		
	}
	return 0;
}
/*
			section_def->rt_vs_history %d\n \
			section_def->q_override %d\n", 
			section_def->rt_vs_history,
			section_def->q_override
*/

int strarr2timeinterval(char strarr[][20], time_interval_config_t *time_interval_config, int verbose) {

	time_interval_config->name = &strarr[0][0];
	if(time_interval_config->name == NULL) {
		fprintf(stderr, "No onramp name\n");
		return -1;
	}
	time_interval_config->ipaddr = &strarr[1][0];
	if(inet_addr(time_interval_config->ipaddr) == INADDR_NONE) {
		printf("Invalid IP address\n");
		return -1;
	}	
	time_interval_config->datestr = &strarr[2][0];
	time_interval_config->interval_start = atoi(&strarr[3][0]);
	time_interval_config->interval_end = atoi(&strarr[4][0]);
	time_interval_config->min_rm_rate = atoi(&strarr[5][0]);
	time_interval_config->max_rm_rate = atoi(&strarr[6][0]);
	if(verbose) {
		printf("time_interval_config->name %s\n \
			time_interval_config->ipaddr %s\n \
			time_interval_config->interval_start %d\n \
			time_interval_config->interval_end %d\n \
			time_interval_config->datestr %s\n \
			time_interval_config->min_rm_rate %d\n \
			time_interval_config->max_rm_rate %d\n ",

			time_interval_config->name,
			time_interval_config->ipaddr,
			time_interval_config->interval_start,
			time_interval_config->interval_end,
			time_interval_config->datestr,
			time_interval_config->min_rm_rate,
			time_interval_config->max_rm_rate
		);		
	}
	return 0;
};

int main(int argc, char *argv[]) {
	section_def_t section_def;
	time_interval_config_t  time_interval_config;
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
	int db_urms_var = 0;
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
	urms_status_response_t *purms_status_response;

	int standalone = 0;
	int loop_interval = 5000; 	// Loop interval, ms
	int verbose = 0;
	int set_urms = 0;
	int get_urms = 0;
	char addr_2070 = 2;
	char use_db = 0;
	int set_tos_action = 0;
	int set_tos_det = 0;
	int i;
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
	unsigned char no_control_startup = 0;
	unsigned char no_control_runtime = 1;
	unsigned char no_control_runtime_sav = 0;
	char *section_def_str = NULL;
	char *time_config_str = NULL;
	char str_seg[20][20] = {0};
	char *tempstr;

	memset(&db_urms, 0, sizeof(db_urms_t));

//printf("sizeof(db_urms_status_t) %d sizeof(db_urms_status2_t) %d sizeof(db_urms_status3_t) %d mainline_stat %d metered_lane_stat %d queue_stat %d addl_det_stat %d\n",
//	sizeof(db_urms_status_t),
//	sizeof(db_urms_status2_t),
//	sizeof(db_urms_status3_t),
//	sizeof(struct mainline_stat),
//	sizeof(struct metered_lane_stat),
//	sizeof(struct queue_stat),
//	sizeof(struct addl_det_stat)
//);
        while ((option = getopt(argc, argv, "d:r:vgsp:a:i:n1:2:3:4:5:6:7:8:9:A:B:C:D:E:F:G:H:I:J:T:c:")) != EOF) {
	    switch(option) {
                case 'd':
                        db_urms_status_var = atoi(optarg);
                        db_urms_var = db_urms_status_var + 2;
			use_db = 1;
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
                case 'p':
                        port = strdup(optarg);
                        break;
                case 'a':
			addr_2070 = atoi(optarg);
                        break;
                case 'i':
                        loop_interval = atoi(optarg);
                        break;
                case 'n':
                        no_control_startup = 1;
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
                case 'T':
                        time_config_str = strdup(optarg);
			for(i=0; i<7; i++) {
				tempstr = strstr( time_config_str, ",");
				if(tempstr == time_config_str){
					str_seg[i][0] = -1;
					printf("No string %d\n", i);
					time_config_str += 1;
				}
				else {
					tempstr = strtok(time_config_str, ",");
					strcpy(&str_seg[i][0], tempstr);
					printf("String %d %s\n", i, &str_seg[i][0]);
					time_config_str += strlen(tempstr) + 1;
				}	
			}	
			if(strarr2timeinterval(str_seg, &time_interval_config, verbose) < 0) {
				printf("Configuration string is wrong. Exiting....\n");
                       		exit(EXIT_FAILURE);
			}
                        break;
                case 'c':
                        section_def_str = strdup(optarg);
			for(i=0; i<17; i++) {
				tempstr = strstr( section_def_str, ",");
				if(tempstr == section_def_str){
					str_seg[i][0] = -1;
					printf("No string %d\n", i);
					section_def_str += 1;
				}
				else {
					tempstr = strtok(section_def_str, ",");
					strcpy(&str_seg[i][0], tempstr);
					printf("String %d %s\n", i, &str_seg[i][0]);
					section_def_str += strlen(tempstr) + 1;
				}	
			}	
			if(strarr2sectiondef (str_seg, &section_def, verbose) < 0) {
				printf("Configuration string is wrong. Exiting....\n");
                       		exit(EXIT_FAILURE);
			}
			else {
				controllerIP = strdup(section_def.rm_controller.ipaddr);
				port = strdup(section_def.rm_controller.port);
                        	no_control_startup = ((section_def.crm_enabled == NULL) ? 0:1);
			}
                        break;
                default:
                        printf("\n\nUsage: %s %s\n", argv[0], usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

printf("Got to 0\n");
	if(use_db) {
		db_vars_list[0].id = db_urms_status_var;
		db_vars_list[1].id = db_urms_status_var + 1;
		db_vars_list[2].id = db_urms_status_var + 2;
		db_vars_list[3].id = db_urms_status_var + 3;
		db_vars_list[4].id = db_urms_status_var + 4;
		db_trig_list[0] = db_urms_var;
	}
printf("Got to 1 config.ipaddr %s port %s crm_enabled %s\n",
	section_def.rm_controller.ipaddr, section_def.rm_controller.port, section_def.crm_enabled);
	if(section_def_str != NULL) {
		controllerIP = section_def.rm_controller.ipaddr;
		port = section_def.rm_controller.port;
		if(strcmp(section_def.crm_enabled, "on") == 0)
			no_control_startup = 0;
		else
			no_control_startup = 1;
	}

	printf("Starting urms.c: IP address %s port %s db_urms_status_var %d db_urms_var %d db_trig_list[0] %d num_trig_variables %d no_control_startup %d no_control_runtime %d\n",
		controllerIP,
		port,
		db_urms_status_var,
		db_urms_var,
		db_trig_list[0],
		num_trig_variables,
		no_control_startup,
		no_control_runtime
	);
	if(set_tos_action)
	for(i=0 ; i<num_lanes; i++)
		printf("top: rm_action_code_tos2[%d] %#hx\n", i, rm_action_code_tos2[i]);

	// Open connection to URMS controller
	printf("1: Opening connection to %s on port %s\n",
		controllerIP,
		port
	);

	urmsfd = OpenURMSConnection(controllerIP, port);
	if(urmsfd < 0) {
		fprintf(stderr, "2: Could not open connection to URMS controller\n");
		exit(EXIT_FAILURE);
	}

	// If just testing in standalone, write message to controller and exit
	if(standalone) {
		if(set_urms) {
			if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
				fprintf(stderr, "Bad status command\n");
				exit(EXIT_FAILURE);
			}
			purms_status_response = &gen_mess.urms_status_response;
			if( ( (purms_status_response->metered_lane_ctl[0].cmd_src < 3) && (purms_status_response->metered_lane_ctl[0].cmd_src > 0) ) || 
				( (purms_status_response->metered_lane_ctl[1].cmd_src < 3) && (purms_status_response->metered_lane_ctl[1].cmd_src > 0) ) || 
				( (purms_status_response->metered_lane_ctl[2].cmd_src < 3) && (purms_status_response->metered_lane_ctl[2].cmd_src > 0) ) || 
				( (purms_status_response->metered_lane_ctl[3].cmd_src < 3) && (purms_status_response->metered_lane_ctl[3].cmd_src > 0) ) 
				) {
					printf("Higher priority command source (i.e. cmd_src < 3) is controlling the meter:\n");
					printf("\tLane 1 command source %d\n", purms_status_response->metered_lane_ctl[0].cmd_src); 
					printf("\tLane 2 command source %d\n", purms_status_response->metered_lane_ctl[1].cmd_src); 
					printf("\tLane 3 command source %d\n", purms_status_response->metered_lane_ctl[2].cmd_src); 
					printf("\tLane 4 command source %d\n", purms_status_response->metered_lane_ctl[3].cmd_src); 
					exit(EXIT_FAILURE);
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
			if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose, addr_2070) < 0) {
				fprintf(stderr, "1:Bad meter setting command\n");
//				exit(EXIT_FAILURE);
			}
#else
			printf("ALLOW_SET_METER is not defined, i.e. I'm not going to actually send a rate to the controller.\n");
#endif
		}
		if(get_urms) {
			if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
				fprintf(stderr, "Bad status command\n");
//				exit(EXIT_FAILURE);
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
	if(use_db) {
		get_local_name(hostname, MAXHOSTNAMELEN);
		if ( (pclt = db_list_init(argv[0], hostname, domain, xport, NULL, 0, db_trig_list, num_trig_variables)) == NULL)
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
		if(no_control_startup == 0)
			urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose, addr_2070);
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
		if(use_db)
			clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));

		// Check for command received via db
		if( DB_TRIG_VAR(&trig_info) == db_urms_var ) {
			db_clt_read(pclt, db_urms_var, sizeof(db_urms_t), &db_urms);
			printf("db_clt_read: db_urms_var %d\n", db_urms_var);
			if(verbose)
				printf("Got db_urms_var trigger\n");
			if( (no_control_startup == 0) && (no_control_runtime == 0) ) {
#ifdef ALLOW_SET_METER
				printf("2: Opening connection to %s on port %s\n",
					controllerIP,
					port
				);
			
				if(urmsfd < 0) 		//Returned a negative number from a previous I/O operation, which should have been dealt with.
					urmsfd = 0;
//				if(urmsfd > 0)
//					close(urmsfd);
				urmsfd = OpenURMSConnection(controllerIP, port);
				if(urmsfd < 0) {
					fprintf(stderr, "Could not open connection to URMS controller\n");
					exit(EXIT_FAILURE);
				}
				if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose, addr_2070) < 0) {
					fprintf(stderr, "2:Bad meter setting command\n");
					exit(EXIT_FAILURE);
				}
//				close(urmsfd);
//				if(urmsfd < 0) {
//					perror("close2");
//				}
#endif
			}
		}
		// If no command, assume timer interrupt and read controller status.
		// Also check time on controller and set our control to SKIP if we're
		// outside of the metering windows (i.e. not AM or PM metering times).
		else {
				close(urmsfd);
				urmsfd = OpenURMSConnection(controllerIP, port);
				if(urmsfd < 0) {
					fprintf(stderr, "OpenURMSConnection failed with error %d: exiting\n", urmsfd);
					exit(EXIT_FAILURE);
				}
			if( urms_get_status(urmsfd, &gen_mess, verbose) < 0) {
				fprintf(stderr, "Bad status command\n");
				close(urmsfd);
				get_status_err++;
				urmsfd = OpenURMSConnection(controllerIP, port);
				if(urmsfd < 0) {
					fprintf(stderr, "OpenURMSConnection failed with error %d: exiting\n", urmsfd);
					exit(EXIT_FAILURE);
				}
	    		}
			else {
				if( clock_gettime(CLOCK_REALTIME, &curr_timespec) < 0)
					perror("urms clock_gettime");
				curr_time = curr_timespec.tv_sec + (curr_timespec.tv_nsec / 1000000000.0);

				memcpy(&db_urms_status.mainline_stat[0], &gen_mess.urms_status_response.mainline_stat[0], sizeof(struct mainline_stat) * MAX_MAINLINES);
				memcpy(&db_urms_status.metered_lane_stat[0], &gen_mess.urms_status_response.metered_lane_stat[0], sizeof(struct metered_lane_stat) * MAX_METERED_LANES);
				memcpy(&db_urms_status2.queue_stat[0][0], &gen_mess.urms_status_response.queue_stat[0][0], sizeof(struct queue_stat) * MAX_METERED_LANES*MAX_QUEUE_LOOPS);
				memcpy(&db_urms_status3.additional_det[0], &gen_mess.urms_status_response.additional_det[0], sizeof(struct addl_det_stat) * MAX_METERED_LANES*MAX_QUEUE_LOOPS);

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
				printf("dow=%d dow%%6=%d hour %d\n", dow, dow % 6, db_urms_status.hour);

				// Check metering windows
				if( (dow == 0) || 		//Disable control if it's Sunday, ...
				    (dow == 6) || 		//Disable control if it's Saturday, ...
				    (db_urms_status.hour < 7) || //or if it's before 7 AM, ...
				    (db_urms_status.hour >= 10) || //or if it's after 10 AM
// Coordinated Ramp Metering for    ((db_urms_status.hour >= 9) && (db_urms_status.hour < 15) ) || //or if it's between 9 AM and 3 PM, ...
// the PM peak hours was disabled   (db_urms_status.hour >= 18) || 	//or if it's after 6 PM, ...
// on 9/27/2017
				    (db_urms.no_control != 0)	||	//or if the database no_control flag has been set, ...
				    (no_control_startup != 0)) { 	//or if the startup no_control flag has been set!
					no_control_runtime = 1;
					if( no_control_runtime_sav == 0) {
						printf("%02d/%02d/%04d %02d:%02d:%02d Disabling control of ramp meter controller: hour=%d db_urms.no_control %d DOW=%d\n",
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
					}
					no_control_runtime_sav = 1;
					db_urms.lane_1_action = URMS_ACTION_SKIP;
					db_urms.lane_2_action = URMS_ACTION_SKIP;
					db_urms.lane_3_action = URMS_ACTION_SKIP;
					db_urms.lane_4_action = URMS_ACTION_SKIP;
#ifdef ALLOW_SET_METER
					if( urms_set_meter(urmsfd, &db_urms, &db_urms_sav, verbose, addr_2070) < 0) {
						fprintf(stderr, "3:Bad meter setting command for IP %s\n", controllerIP);
					}
#endif
				}
				else {
					no_control_runtime = 0;
					if( no_control_runtime_sav == 1) {
						printf("%02d/%02d/%04d %02d:%02d:%02d Enabling control of ramp meter controller: hour=%d db_urms.no_control %d DOW=%d\n",
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
						no_control_runtime_sav = 0;
					}
				}

				for(i = 0; i < MAX_METERED_LANES; i++) {
					db_urms_status3.cmd_src[i] = gen_mess.urms_status_response.metered_lane_ctl[i].cmd_src;
					db_urms_status3.action[i] = gen_mess.urms_status_response.metered_lane_ctl[i].action;
					db_urms_status3.plan[i] = gen_mess.urms_status_response.metered_lane_ctl[i].plan;
					db_urms_status3.plan_base_lvl[i] = gen_mess.urms_status_response.metered_lane_ctl[i].plan_base_lvl;
					db_urms_status3.metering_rate[i] = 
						(int)((gen_mess.urms_status_response.metered_lane_stat[i].metered_lane_rate_msb << 8) +
						(unsigned char)(gen_mess.urms_status_response.metered_lane_stat[i].metered_lane_rate_lsb));
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
					if(verbose) {
						printf("IP %s Lane %d rate %d cmd_src %d action %d plan %d plan_base level %d\n",
							controllerIP,
							i+1, 
							db_urms_status3.metering_rate[i],
							db_urms_status3.cmd_src[i],
							db_urms_status3.action[i],
							db_urms_status3.plan[i],
							db_urms_status3.plan_base_lvl[i]
						);
				    	}
				}

				for(i = 0; i < MAX_MAINLINES; i++) {
				    urms_datafile.mainline_lead_occ[i] = 0.1 * ((gen_mess.urms_status_response.mainline_stat[i].lead_occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.mainline_stat[i].lead_occ_lsb));
				    urms_datafile.mainline_trail_occ[i] = 0.1 * ((gen_mess.urms_status_response.mainline_stat[i].trail_occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.mainline_stat[i].trail_occ_lsb));
				    urms_datafile.queue_occ[i] = 0.1 * ((gen_mess.urms_status_response.queue_stat[0][i].occ_msb << 8) + (unsigned char)(gen_mess.urms_status_response.queue_stat[0][i].occ_lsb));
				    if(verbose) {
					printf("1:ML%d occ %.1f ", i+1, urms_datafile.mainline_lead_occ[i]);
				 	printf("1:MT%d occ %.1f ", i+1, urms_datafile.mainline_trail_occ[i]);
					printf("1:Q%d-1 occ %.1f ", i+1, urms_datafile.queue_occ[i]);
					printf("1:lane %d cmd_src %hhu ", i+1, db_urms_status3.cmd_src[i]);
					printf("1:lane %d action %hhu\n", i+1, db_urms_status3.action[i]);
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
				if(use_db) {
					db_clt_write(pclt, db_urms_status_var, sizeof(db_urms_status_t), &db_urms_status);
					db_clt_write(pclt, db_urms_status_var + 1, sizeof(urms_datafile_t), &urms_datafile);
					printf("db_urms_status2.queue_stat[0][0].occ_msb %hhu db_urms_status2.queue_stat[0][0].occ_lsb %hhu\n", db_urms_status2.queue_stat[0][0].occ_msb, db_urms_status2.queue_stat[0][0].occ_lsb );
					db_clt_write(pclt, db_urms_status_var + 3, sizeof(db_urms_status2_t), &db_urms_status2);
					db_clt_write(pclt, db_urms_status_var + 4, sizeof(db_urms_status3_t), &db_urms_status3);
				}
	    		}
		}
		if(!use_db)
			TIMER_WAIT(ptimer);
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
		fprintf(stderr, "1: Could not connect\n");
		return -1;
	}
	freeaddrinfo(result);	    /* No longer needed */
	return sfd;
}


int urms_set_meter(int fd, db_urms_t *db_urms, db_urms_t *db_urms_sav, char verbose, char addr_2070) {
	gen_mess_t gen_mess;
	char *msgbuf = (char *)&gen_mess;
	int csum;
	int i;
	int nread;
	char buf[BUF_SIZE];
	timestamp_t ts;
	int ret;

	// Initialize URMS message
	// Lane 1: 900 VPH release rate, fixed rate, plan 1
	// Lane 2: 900 VPH release rate, fixed rate, plan 1
	memset(&gen_mess, 0, sizeof(gen_mess_t));
	gen_mess.urmsctl.addr = addr_2070;
	gen_mess.urmsctl.hdr1 = 0xe;
	gen_mess.urmsctl.hdr2 = 0x15;
	gen_mess.urmsctl.hdr3 = 0x70;
	gen_mess.urmsctl.hdr4 = 0x01;
	gen_mess.urmsctl.hdr5 = 0x11;
	gen_mess.urmsctl.tail0 = 0x00;
	gen_mess.urmsctl.tail1 = 0xaa;
	gen_mess.urmsctl.tail2 = 0x55;

//	memset(gen_mess, 0, sizeof(gen_mess_t));
//	gen_mess->urms_status_poll.msg_type = 0x51;
//	gen_mess->urms_status_poll.drop_num = 2;
//	gen_mess->urms_status_poll.num_bytes = 8;
//	gen_mess->urms_status_poll.action = db_urms->lane_1_action;
//	gen_mess->urms_status_poll.rate_msb = (db_urms->lane_1_release_rate & 0xFF00) >> 8;
//	gen_mess->urms_status_poll.rate_lsb = (db_urms->lane_1_release_rate & 0x00FF);
//	gen_mess->urms_status_poll.plan = db_urms->lane_1_plan;
//	gen_mess->urms_status_poll.gp_out = 0;
//	gen_mess->urms_status_poll.future_use = 0;
//	gen_mess->urms_status_poll.enable = 0x08;
//	gen_mess->urms_status_poll.checksum_msb = 0;
//	gen_mess->urms_status_poll.checksum_lsb = 0;

//	csum = 0;
//	for(i=0; i < (sizeof(urms_status_poll_t) - 2); i++)
//		csum += msgbuf[i];
//	gen_mess->urms_status_poll.checksum_msb = ((csum >> 8) & 0xFF);
//	gen_mess->urms_status_poll.checksum_lsb = (csum  & 0xFF);

	// If requested metering rate is 0, assume no change
	// ALL of the parameters MUST be set!

	if( (db_urms->lane_1_release_rate < 150) || (db_urms->lane_1_action < 1) ||
	   (db_urms->lane_1_release_rate > 1100) || (db_urms->lane_1_action > 7) ){
		fprintf(stderr, "lane 1 release rate %d (out of range) action %d: (invalid action)!!\n",
			db_urms->lane_1_release_rate,
			db_urms->lane_1_action
		);
		if(db_urms->lane_1_release_rate < 150)
			db_urms->lane_1_release_rate = 150;
		if(db_urms->lane_1_release_rate > 1100)
			db_urms->lane_1_release_rate = 1100;
		if( (db_urms->lane_1_action < 1) ||(db_urms->lane_1_action > 7) )
			db_urms->lane_1_action = URMS_ACTION_SKIP;
		fprintf(stderr, "Setting lane 1 release rate to %d and action to %d\n",
			db_urms->lane_1_release_rate,
			db_urms->lane_1_action
		);
	}

	if( (db_urms->lane_2_release_rate < 150) || (db_urms->lane_2_action < 1) ||
	   (db_urms->lane_2_release_rate > 1100) || (db_urms->lane_2_action > 7) ){
		fprintf(stderr, "lane 2 release rate %d (out of range) action %d: (invalid action)!!\n",
			db_urms->lane_2_release_rate,
			db_urms->lane_2_action
		);
		if(db_urms->lane_2_release_rate < 150)
			db_urms->lane_2_release_rate = 150;
		if(db_urms->lane_2_release_rate > 1100)
			db_urms->lane_2_release_rate = 1100;
		if( (db_urms->lane_2_action < 1) ||(db_urms->lane_2_action > 7) )
			db_urms->lane_2_action = URMS_ACTION_SKIP;
		fprintf(stderr, "Setting lane 2 release rate to %d and action to %d\n",
			db_urms->lane_2_release_rate,
			db_urms->lane_2_action
		);
	}

	if( (db_urms->lane_3_release_rate < 150) || (db_urms->lane_3_action < 1) ||
	   (db_urms->lane_3_release_rate > 1100) || (db_urms->lane_3_action > 7) ){
		fprintf(stderr, "lane 3 release rate %d (out of range) action %d: (invalid action)!!\n",
			db_urms->lane_3_release_rate,
			db_urms->lane_3_action
		);
		if(db_urms->lane_3_release_rate < 150)
			db_urms->lane_3_release_rate = 150;
		if(db_urms->lane_3_release_rate > 1100)
			db_urms->lane_3_release_rate = 1100;
		if( (db_urms->lane_3_action < 1) ||(db_urms->lane_3_action > 7) )
			db_urms->lane_3_action = URMS_ACTION_SKIP;
		fprintf(stderr, "Setting lane 3 release rate to %d and action to %d\n",
			db_urms->lane_3_release_rate,
			db_urms->lane_3_action
		);
	}

	if( (db_urms->lane_4_release_rate < 150) || (db_urms->lane_4_action < 1) ||
	   (db_urms->lane_4_release_rate > 1100) || (db_urms->lane_4_action > 7) ){
		fprintf(stderr, "lane 4 release rate %d (out of range) action %d: (invalid action)!!\n",
			db_urms->lane_4_release_rate,
			db_urms->lane_4_action
		);
		if(db_urms->lane_4_release_rate < 150)
			db_urms->lane_4_release_rate = 150;
		if(db_urms->lane_4_release_rate > 1100)
			db_urms->lane_4_release_rate = 1100;
		if( (db_urms->lane_4_action < 1) ||(db_urms->lane_4_action > 7) )
			db_urms->lane_4_action = URMS_ACTION_SKIP;
		fprintf(stderr, "Setting lane 4 release rate to %d and action to %d\n",
			db_urms->lane_4_release_rate,
			db_urms->lane_4_action
		);
	}

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
	printf("urms_set_meter: lane 1 action %d rate %d plan %d lane 2 action %d rate %d plan %d lane 3 action %d rate %d plan %d  lane 4 action %d rate %d plan %d \n", 
		gen_mess.urmsctl.lane_1_action, 
		(gen_mess.urmsctl.lane_1_release_rate_MSB << 8) + gen_mess.urmsctl.lane_1_release_rate_LSB , 
		gen_mess.urmsctl.lane_1_plan, 
		gen_mess.urmsctl.lane_2_action, 
		(gen_mess.urmsctl.lane_2_release_rate_MSB << 8) + gen_mess.urmsctl.lane_2_release_rate_LSB , 
		gen_mess.urmsctl.lane_2_plan, 
		gen_mess.urmsctl.lane_3_action, 
		(gen_mess.urmsctl.lane_3_release_rate_MSB << 8) + gen_mess.urmsctl.lane_3_release_rate_LSB , 
		gen_mess.urmsctl.lane_3_plan,
		gen_mess.urmsctl.lane_4_action, 
		(gen_mess.urmsctl.lane_4_release_rate_MSB << 8) + gen_mess.urmsctl.lane_4_release_rate_LSB , 
		gen_mess.urmsctl.lane_4_plan
	);

	if ((ret = write(fd, msgbuf, sizeof(urmsctl_t))) != sizeof(urmsctl_t)) {
	  perror("urms_set_meter write");
	  fprintf(stderr, "urms_set_meter: partial/failed write, ret %d sizeof(urmsctl_t) %ld\n",
		ret,
		sizeof(urmsctl_t)
	  );
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

	int i;
	int j;
	int nread;
	char *buf = &gen_mess->data[0];
	timestamp_t ts;
	struct timespec start_time;
	struct timespec end_time;
	int msg_len = 0;

//	memset(gen_mess, 0, sizeof(gen_mess_t));
//	gen_mess->urms_status_poll.msg_type = 0x51;
//	gen_mess->urms_status_poll.drop_num = 2;
//	gen_mess->urms_status_poll.num_bytes = 8;
//	gen_mess->urms_status_poll.action = 6;
//	gen_mess->urms_status_poll.rate_msb = 0;
//	gen_mess->urms_status_poll.rate_lsb = 200;
//	gen_mess->urms_status_poll.plan = 2;
//	gen_mess->urms_status_poll.gp_out = 0;
//	gen_mess->urms_status_poll.future_use = 0;
//	gen_mess->urms_status_poll.enable = 0;
//	gen_mess->urms_status_poll.checksum_msb = 0;
//	gen_mess->urms_status_poll.checksum_lsb = 0;
//	MUST DO FCS16 CHECKSUM HERE!!
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
	if (write(fd, &gen_mess->urmspoll2, 9) < 0) {

//	if (write(fd, &gen_mess->urms_status_poll, sizeof(urms_status_poll_t)) != sizeof(urms_status_poll_t)) 
	  perror("urms_get_status1: partial/failed write");
	  fprintf(stderr, "urms_get_status2: partial/failed write\n");
//	  exit(EXIT_FAILURE);
	  return -1;
	}

	nread = read(fd, gen_mess, 417);
	if (nread == -1) {
	  perror("urms_get_status: read error");
	  return -2;
	}
	clock_gettime(CLOCK_REALTIME, &end_time);

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
	printf("Q1-1 occ %.1f ", 0.1 * ((gen_mess->urms_status_response.queue_stat[0][0].occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.queue_stat[0][0].occ_lsb));
	printf("Q2-1 occ %.1f ", 0.1 * ((gen_mess->urms_status_response.queue_stat[0][1].occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.queue_stat[0][1].occ_lsb));
/*
	printf("ML1 speed %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[0].speed, 0.1 * ((gen_mess->urms_status_response.mainline_stat[0].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[0].lead_occ_lsb));
	printf("ML2 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[1].lead_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[1].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[1].lead_occ_lsb));
	printf("MT2 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[1].trail_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[1].trail_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[1].trail_occ_lsb));
	printf("ML3 vol %hhu occ %.1f ", gen_mess->urms_status_response.mainline_stat[2].lead_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[2].lead_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[2].lead_occ_lsb));
	printf("MT3 vol %hhu occ %.1f \n", gen_mess->urms_status_response.mainline_stat[2].trail_vol, 0.1 * ((gen_mess->urms_status_response.mainline_stat[2].trail_occ_msb << 8) + (unsigned char)gen_mess->urms_status_response.mainline_stat[2].trail_occ_lsb));
	printf("Metering rate 1 %hu 2 %hu 3 %hu 4 %hu\n",
		((gen_mess->urms_status_response.metered_lane_stat[0].metered_lane_rate_msb << 8) +
		(unsigned char)(gen_mess->urms_status_response.metered_lane_stat[0].metered_lane_rate_lsb)),
		((gen_mess->urms_status_response.metered_lane_stat[1].metered_lane_rate_msb << 8) +
		(unsigned char)(gen_mess->urms_status_response.metered_lane_stat[1].metered_lane_rate_lsb)),
		((gen_mess->urms_status_response.metered_lane_stat[2].metered_lane_rate_msb << 8) +
		(unsigned char)(gen_mess->urms_status_response.metered_lane_stat[3].metered_lane_rate_lsb)),
		((gen_mess->urms_status_response.metered_lane_stat[3].metered_lane_rate_msb << 8) +
		(unsigned char)(gen_mess->urms_status_response.metered_lane_stat[3].metered_lane_rate_lsb))
	);
*/
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
		printf("rm_action_code_tos2[%d] %hx\n", i, rm_action_code_tos2[i]);
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
	    printf("tos_set_action: Time for function call %f sec\n", (end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - (start_time.tv_sec + (start_time.tv_nsec/1.0e9)));
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
