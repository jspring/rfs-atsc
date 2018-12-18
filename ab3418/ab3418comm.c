/* ab3418comm - AB3418<-->database communicator
*/

#include <db_include.h>
#include "msgs.h"
#include "ab3418_lib.h"
#include "ab3418comm.h"
#include "urms.h"
#include <udp_utils.h>

#define MAX_PHASES	7

static jmp_buf exit_env;

static void sig_hand( int code )
{
        if( code == SIGALRM )
                return;
        else
                longjmp( exit_env, code );
}

static int sig_list[] =
{
        SIGINT,
        SIGQUIT,
        SIGTERM,
        SIGALRM,
        ERROR
};

int OpenTSCPConnection(char *controllerIP, char *port);
int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status);
int get_detector(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char detector, char verbose);
int set_detector(detector_msg_t *pdetector_set_request, int fpin, int fpout, char detector, char verbose);
int set_time_udp(int wait_for_data, gen_mess_typ *readBuff, int td_in, int td_out, struct sockaddr_in *time_addr, char verbose);
int get_status_udp(int wait_for_data, gen_mess_typ *readBuff, int sd_in, int sd_out, struct sockaddr_in *dst_addr, char verbose);
int get_short_status(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);
int set_pattern(int wait_for_data, gen_mess_typ *readBuff, unsigned char pattern, int fpin, int fpout, struct sockaddr_in *dst_addr, char verbose);
int set_time(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char verbose);

static db_id_t db_vars_ab3418comm[] = {
        {DB_2070_TIMING_GET_VAR, sizeof(db_timing_get_2070_t)},
        {DB_2070_TIMING_SET_VAR, sizeof(db_timing_set_2070_t)},
        {DB_TSCP_STATUS_VAR, sizeof(get_long_status8_resp_mess_typ)},
        {DB_SHORT_STATUS_VAR, sizeof(get_short_status_resp_t)},
        {DB_PHASE_1_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_2_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_3_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_4_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_5_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_6_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_7_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_8_TIMING_VAR, sizeof(phase_timing_t)},
        {DB_PHASE_STATUS_VAR, sizeof(phase_status_t)}
};

#define NUM_DB_VARS sizeof(db_vars_ab3418comm)/sizeof(db_id_t)
int db_trig_list[] =  {
       DB_2070_TIMING_SET_VAR
};

int NUM_TRIG_VARS = sizeof(db_trig_list)/sizeof(int);

static int db_status_var = 0;
static int db_set_pattern_var = 0;
static db_id_t db_vars_san_jose[] = {
        {0, sizeof(db_set_pattern_t)},
        {1, sizeof(get_long_status8_resp_mess_typ)},
};
#define NUM_SJ_VARS sizeof(db_vars_san_jose)/sizeof(db_id_t)

int db_trig_san_jose[] =  {
       0
};
int NUM_SJ_TRIG_VARS = sizeof(db_trig_san_jose)/sizeof(int);

const char *usage = "-p <port, (def. /dev/ttyS0)>\n\t\t -u (use db) -v (verbose)\n\t\t -i <loop interval>\n\t\t -c (create database variables)\n\t\t -n (no control)\n\t\t -d <detector number>\n\t\t -b (output binary SPaT message)\n\t\t -a <remote IP address>\n\t\t -A <local IP address>\n\t\t -o <UDP unicast port>\n\t\t -I <TCP/IP address>\n\t\t -P <pattern or plan number>\n\t\t -T <time port>";


int main(int argc, char *argv[]) {

        db_clt_typ *pclt;              /* Database client pointer */
	char hostname[MAXHOSTNAMELEN];
	char *domain = DEFAULT_SERVICE;
	int xport = COMM_OS_XPORT;
	posix_timer_typ *ptmr;
	trig_info_typ trig_info;
	int ipc_message_error_ctr = 0;

        struct sockaddr_in snd_addr;    /// used in sendto call
	int i;
	int fpin = 0;
	int fpout = 0;
	int td = 0;
	FILE *fifofd = 0;
	FILE *fp = 0;
	char *datafilename;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	phase_status_t phase_status;
	int msg_len;
	int wait_for_data = 1;
	gen_mess_typ readBuff;
	char *preadBuff = (char *)&readBuff;
	phase_timing_t phase_timing[MAX_PHASES];
	overlap_msg_t overlap_sav;
	overlap_msg_t overlap;
	get_set_special_flags_t get_set_special_flags;
	detector_msg_t detector_block_sav;
	detector_msg_t detector_block;
	db_urms_status_t db_urms_status;
	db_urms_status2_t db_urms_status2;
	db_urms_status3_t db_urms_status3;
	db_urms_t db_urms;
	raw_signal_status_msg_t raw_signal_status_msg;
	db_set_pattern_t db_set_pattern;
	int retval;
	int check_retval;
	char port[30] = "/dev/ttyS0";
	char strbuf[1000] = {0};
	struct sockaddr_in dst_addr;
	struct sockaddr_in time_addr;
	char *local_ipaddr = NULL;       /// address of UDP destination
	char *remote_ipaddr = NULL;       /// address of UDP destination



	char *tcpip_addr = NULL;

	struct timespec start_time;
	struct timespec end_time;
	struct timespec tp;
	struct tm *ltime;
	timestamp_t ts;
	int dow;

        struct timeb timeptr_raw;
        struct tm time_converted;
	int opt;
	int use_db = 0;
	int interval = 100;
	int verbose = 0;
	unsigned char greens = 0;
	unsigned char create_db_vars = 0;
	unsigned char no_control = 0;
	unsigned char no_control_sav = 0;
	char db_urms_struct_null = 0;
	char detector = 0;
	unsigned int temp_addr;
	unsigned char output_spat_binary = 0;
	unsigned char read_spat_from_file = 0;
	short temp_port;

        int sd_out;             /// socket descriptor for UDP send
        short udp_port = 0;    /// set from command line option
	int pattern = -1;
	short time_port = 0;
	int exit_here = 0;
 
        while ((opt = getopt(argc, argv, "p:uvwi:cnd:a:bho:s:A:I:P:T:D:Ef:")) != -1)
        {
                switch (opt)
                {
                  case 'p':
			memset(port, 0, sizeof(port));
                        strncpy(&port[0], optarg, 29);
                        break;
                  case 'u':
                        use_db = 1;
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  case 'w':
                        verbose = 2;
                        break;
                  case 'i':
                        interval = atoi(optarg);
                        break;
                  case 'c':
                        create_db_vars = 1;
                        break;
                  case 'n':
                        no_control = 1;
                        break;
                  case 'd':
                        detector = atoi(optarg);
                        break;
                  case 'a':
			remote_ipaddr = strdup(optarg);
                        break;
                  case 'R':
                        read_spat_from_file  = 1;
                        break;
                  case 'b':
                        output_spat_binary = 1;
                        break;
                  case 'o':
                        udp_port = (short)atoi(optarg);
                        break;
		  case 'A': 
			local_ipaddr = strdup(optarg);
			break;
		  case 'I':
                        tcpip_addr = strdup(optarg);
                        break;
                  case 'P':
                        pattern = atoi(optarg);
                        break;
                  case 'T':
                        time_port = (short)atoi(optarg);
                        break;
                  case 'D':
                        db_status_var = atoi(optarg);
			db_set_pattern_var = db_status_var + 1;
                        break;
                  case 'E':
			exit_here = 1;
                        break;
		  case 'f':
                        datafilename = strdup(optarg);
//printf("datafilename %s\n", datafilename);
                        break;
		  case 'h':
		  default:
			fprintf(stderr, "Usage: %s %s\n", argv[0], usage);
			exit(EXIT_FAILURE);
		}
	}

	db_vars_san_jose[0].id = db_set_pattern_var;
	db_vars_san_jose[1].id = db_set_pattern_var + 1;
	db_trig_san_jose[0] = db_set_pattern_var;

	// Clear message structs
	memset(&detector_block, 0, sizeof(detector_msg_t));
	memset(&detector_block_sav, 0, sizeof(detector_msg_t));
	memset(&get_set_special_flags, 0, sizeof(get_set_special_flags_t));
	memset(&overlap, 0, sizeof(overlap_msg_t));
	memset(&overlap_sav, 0, sizeof(overlap_msg_t));
	memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070));
	memset(&raw_signal_status_msg, 0, sizeof(raw_signal_status_msg));
	memset(&snd_addr, 0, sizeof(snd_addr));
	memset(&dst_addr, 0, sizeof(dst_addr));
	memset(&time_addr, 0, sizeof(time_addr));

        /* Initialize port. */
	if( (udp_port != 0) && (remote_ipaddr != NULL) ) {
		if(verbose)
			printf("Opening UDP unicast to destination %s port %hu\n", remote_ipaddr, udp_port);
                if ( (sd_out = udp_unicast_init(&dst_addr, remote_ipaddr, udp_port)) < 0) {

               		 printf("1 Failure to initialize socket from %s to %s on port %d our error number %d\n",
               		         remote_ipaddr, local_ipaddr, udp_port, sd_out);
               		 longjmp(exit_env, 2);
        	}

		if (sd_out <= 0) {
			printf("failure opening socket on %s %d\n",
				remote_ipaddr, udp_port);
			exit(EXIT_FAILURE);
		}
		else {
			if(verbose) {
				printf("Success opening socket %hhu on %s %d\n",
					sd_out, remote_ipaddr, udp_port);
				printf("port %d addr 0x%08x\n", ntohs(dst_addr.sin_port), ntohl(dst_addr.sin_addr.s_addr));
			}
			temp_addr = dst_addr.sin_addr.s_addr;
			temp_port = dst_addr.sin_port;
		}
	}

	if( (time_port != 0) && (remote_ipaddr != NULL) ) {
		if(verbose)
			printf("Opening UDP unicast to destination %s time port %hu\n", remote_ipaddr, time_port);
		if ( (td = udp_peer2peer_init(&time_addr, remote_ipaddr, local_ipaddr, time_port, 0)) < 0) {
			printf("2 Failure to initialize socket from %s to %s on time port %d\n",
				remote_ipaddr, local_ipaddr, time_port);
			longjmp(exit_env, 2);
		}


		if (td <= 0) {
			printf("failure opening socket on %s %d\n",
				remote_ipaddr, time_port);
			exit(EXIT_FAILURE);
		}
		else {
			printf("Success opening socket %hhu on %s %d\n",
				td, remote_ipaddr, time_port);
			printf("port %d addr 0x%08x\n", ntohs(time_addr.sin_port), ntohl(time_addr.sin_addr.s_addr));
			temp_addr = time_addr.sin_addr.s_addr;
			temp_port = time_addr.sin_port;
		}
	}

	if( (udp_port != 0) && (remote_ipaddr != NULL) ) {
		fpin = fpout = sd_out;
		if(verbose)
			printf("Using UDP name %s and UDP port %d file descriptor %d time port file descriptor %d\n", remote_ipaddr, udp_port, sd_out, td);
		if(time_port != 0)
			retval = set_time_udp(wait_for_data, &readBuff, td, td, &time_addr, verbose);
		retval = get_status_udp(wait_for_data, &readBuff, sd_out, sd_out, &dst_addr, verbose);
		if(exit_here) {
			process_phase_status( (get_long_status8_resp_mess_typ *)&readBuff, verbose, greens, &phase_status);
			if(verbose) 
				print_status( NULL, NULL, (get_long_status8_resp_mess_typ *)&readBuff, verbose);
			if(retval > 0){
				fp = fopen(datafilename, "w");
				if(output_spat_binary) { 
					fwrite(preadBuff, retval, 1, stdout);
					fwrite(preadBuff, retval, 1, fp);
//					for(i=0; i<retval; i++)
//						printf("%hhx ", preadBuff[i]);
//						fprintf(fp, "%hhx ", preadBuff[i]);
//					fprintf(fp, "\n");
					fclose(fp);
					exit(EXIT_SUCCESS);
				}
				get_current_timestamp(&ts);
				print_timestamp(fp, &ts);
				struct tm *nowtime;
				struct timespec tspec;
				clock_gettime(CLOCK_REALTIME, &tspec);
				nowtime = localtime(&tspec.tv_sec);
				fprintf(fp, " %d/%d/%d ", nowtime->tm_mon+1, nowtime->tm_mday, nowtime->tm_year+1900);
				memset(strbuf, 0, 1000);
				retval = print_status(strbuf, fp, (get_long_status8_resp_mess_typ *)&readBuff, verbose);
				fprintf(fp, "\n");
				printf("strbuf: %s\n", strbuf);
				fclose(fp);
				if(verbose)
					printf("Exiting early, but it's OK\n");
				exit(EXIT_SUCCESS);
			}
			else {
				printf("get_status_udp return value %d\n", retval);
				exit(EXIT_FAILURE);
			}
		}
//		retval = get_short_status(wait_for_data, &readBuff, fpin, fpout, verbose);
//	for(i=0; i<MAX_PHASES; i++) {
//		retval = get_timing_udp(&db_timing_get_2070, wait_for_data, &phase_timing[i], sd_out, sd_out, &dst_addr, verbose);
//	}
		if(pattern >= 0) {
			retval = set_pattern(wait_for_data, &readBuff, pattern, fpin, fpout, &dst_addr, verbose);
			retval = get_status_udp(wait_for_data, &readBuff, sd_out, sd_out, &dst_addr, verbose);
			return 0;
		}
	}
	else
		if(tcpip_addr == NULL) {
			check_retval = check_and_reconnect_serial(0, &fpin, &fpout, port);
			retval = set_time(wait_for_data, &readBuff, fpin, fpout, verbose);
			retval = set_pattern(wait_for_data, &readBuff, pattern, fpin, fpout, &dst_addr, verbose);
			retval = get_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			retval = get_short_status(wait_for_data, &readBuff, fpin, fpout, verbose);
		}
	else
		fpout = fpin = OpenTSCPConnection(tcpip_addr, "2011");

	if(use_db) {
		get_local_name(hostname, MAXHOSTNAMELEN);
		if(create_db_vars) {
		    if ( ((pclt = db_list_init(argv[0], hostname,
			domain, xport, db_vars_ab3418comm, NUM_DB_VARS, 
			db_trig_list, NUM_TRIG_VARS)) == NULL))
			exit(EXIT_FAILURE);
		}
		if(db_set_pattern_var != 0) {
		    if ( ((pclt = db_list_init(argv[0], hostname,
			domain, xport, db_vars_san_jose, NUM_SJ_VARS, 
			db_trig_san_jose, NUM_SJ_TRIG_VARS)) == NULL))
			exit(EXIT_FAILURE);
		}
		else {
		    if ( ((pclt = db_list_init(argv[0], hostname,
			domain, xport, NULL, 0, 
			db_trig_list, NUM_TRIG_VARS)) == NULL)) {
			exit(EXIT_FAILURE);
		    }
		}
	}

		if (setjmp(exit_env) != 0) {

			// Tell ramp meter computer to stop controlling
			db_clt_read(pclt, DB_URMS_VAR, sizeof(db_urms_t), &db_urms);
			db_urms.no_control = 1;
			db_clt_write(pclt, DB_URMS_VAR, sizeof(db_urms_t), &db_urms);

			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
		    if(create_db_vars)
			db_list_done(pclt, db_vars_ab3418comm, NUM_DB_VARS, 
				db_trig_list, NUM_TRIG_VARS);
		    else
			db_list_done(pclt, NULL, 0, 
				db_trig_list, NUM_TRIG_VARS);
			printf("%s: %d IPC message errors\n", argv[0], 
				ipc_message_error_ctr);
                	if(fpin)
                       		close(fpin);
                	if(fpout)
                       		close(fpout);
			exit(EXIT_SUCCESS);
		} else
			sig_ign(sig_list, sig_hand);


	if ((ptmr = timer_init( interval, ChannelCreate(0))) == NULL) {
		fprintf(stderr, "Unable to initialize delay timer\n");
		exit(EXIT_FAILURE);
	}

	printf("main 1: getting timing settings before infinite loop\n");
	for(i=0; i<MAX_PHASES; i++) {
		db_timing_get_2070.phase = i+1;	
		db_timing_get_2070.page = 0x100; //phase timing page	
//		retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], &fpin, &fpout, verbose);
//		retval = get_timing_udp(&db_timing_get_2070, wait_for_data, &phase_timing[i], sd_out, sd_out, &dst_addr, verbose);
//		retval = get_status_udp(wait_for_data, &readBuff, sd_out, sd_out, &dst_addr, verbose);

//		if(use_db)
//		db_clt_write(pclt, DB_PHASE_1_TIMING_VAR + i, sizeof(phase_timing_t), &phase_timing[i]);
//		usleep(500000);
	}

	while(1) {
		if(use_db)
			retval = clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
//		if( DB_TRIG_VAR(&trig_info) == DB_2070_TIMING_SET_VAR ) 
		if( DB_TRIG_VAR(&trig_info) == db_set_pattern_var) {
//			db_clt_read(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_timing_set_2070_t), &db_timing_set_2070);
			db_clt_read(pclt, db_set_pattern_var, sizeof(db_set_pattern_t), &db_set_pattern);
			if(no_control == 0) {
//				retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
				retval = set_pattern(wait_for_data, &readBuff, db_set_pattern.pattern, fpin, fpout, &dst_addr, verbose);
			}
		}
		else {	
wait_for_data=1;
		if( (udp_port != 0) && (remote_ipaddr != NULL) ) {
			retval = get_status_udp(wait_for_data, &readBuff, sd_out, sd_out, &dst_addr, verbose);
			if(retval < 0) 
				printf("get_status_udp returned negative value: %d\n", retval);
			else {
				fp = fopen(datafilename, "w");
				get_current_timestamp(&ts);
				print_timestamp(fp, &ts);
				for(i=0; i<retval; i++)
					fprintf(fp, "%hhx ", preadBuff[i]);
				fclose(fp);
			}
		}
		else {
			if(read_spat_from_file) {
				fp = fopen(datafilename, "r");
				retval = fread(&readBuff, sizeof(get_long_status8_resp_mess_typ), 1, fp);
				fclose(fp);
			}
			else {
				retval = get_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			}
			if(retval < 0) 
				printf("get_status returned negative value: %d\n", retval);
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
		}
		if(use_db && (retval >= 0) ) {
//			db_clt_write(pclt, DB_TSCP_STATUS_VAR, sizeof(get_long_status8_resp_mess_typ), (get_long_status8_resp_mess_typ *)&readBuff);
			db_clt_write(pclt, db_status_var, sizeof(get_long_status8_resp_mess_typ), (get_long_status8_resp_mess_typ *)&readBuff);
			retval = process_phase_status( (get_long_status8_resp_mess_typ *)&readBuff, verbose, greens, &phase_status);
			db_clt_write(pclt, DB_PHASE_STATUS_VAR, sizeof(phase_status_t), &phase_status);
			fifofd = fopen("/tmp/blah", "w");
			for(i=0; i<8; i++) {
				fprintf(fifofd, "%d", phase_status.phase_status_colors[i]);
			}
			fclose(fifofd);
			if(verbose) 
				print_status( NULL, NULL, (get_long_status8_resp_mess_typ *)&readBuff, verbose);
		}
		usleep(80000);
		if(udp_port == 0)
			retval = get_short_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
			if(use_db && (retval == 0) ) {
			    greens = readBuff.data[5];	
			    if(verbose) 
				printf("ab3418comm: get_short_status: greens %#hhx\n", readBuff.data[5]);
				db_clt_write(pclt, DB_SHORT_STATUS_VAR, sizeof(get_short_status_resp_t), (get_short_status_resp_t *)&readBuff);
			}
			else 
				if(retval < 0)
					printf("get_short_status returned negative value: %d\n", retval);
		}
		if(verbose) {
			clock_gettime(CLOCK_REALTIME, &end_time);
			printf("%s: Time for function call %f sec\n", argv[0], 
				(end_time.tv_sec + (end_time.tv_nsec/1.0e9)) - 
				(start_time.tv_sec + (start_time.tv_nsec/1.0e9))
				);
			clock_gettime(CLOCK_REALTIME, &start_time);
		}

//###### The following block of code was used in the Taylor Street project.  It was a hard-coded time check that stopped control 
//###### when we were outside of the morning and afternoon peak hours. This control SHOULD be done elsewhere, but was required in that project.
//###### I'm commenting it out now, but not removing it.
//
//	if(use_db) {
//		db_clt_read(pclt, DB_URMS_STATUS_VAR, sizeof(db_urms_status_t), &db_urms_status);
//		db_clt_read(pclt, DB_URMS_STATUS2_VAR, sizeof(db_urms_status2_t), &db_urms_status2);
//		db_clt_read(pclt, DB_URMS_STATUS3_VAR, sizeof(db_urms_status3_t), &db_urms_status3);
//		clock_gettime(CLOCK_REALTIME, &tp);
//		ltime = localtime(&tp.tv_sec);
//		dow = ltime->tm_wday;
////		printf("dow=%d dow%%6=%d hour %d\n", dow, dow % 6, db_urms_status.hour);
//
//		if( ((dow % 6) == 0) || (db_urms_status.hour < 15) || (db_urms_status.hour >= 19) ) {
//			no_control = 1;
//			db_urms.no_control = 1;
//	
//			//When the software is first started, the db_urms_status struct is cleared
//			// to all zeroes, including the hour.  So we check a few of the other 
//			// struct members whose values should not be zero.
//			db_urms_struct_null = db_urms_status.num_meter + 
//						db_urms_status.num_main + 
//						db_urms_status3.num_opp + 
//						db_urms_status.mainline_stat[0].trail_stat + 
//						db_urms_status.mainline_stat[1].trail_stat + 
//						db_urms_status.mainline_stat[2].trail_stat;
//
//			if(( no_control_sav == 0) && (db_urms_struct_null != 0)){
//				// Set phase 3 max green 1 to 30 seconds (default) when we are outside of the TOD period
//				db_timing_set_2070.cell_addr_data.cell_addr = 0x118;
//				db_timing_set_2070.phase = 3;
//				db_timing_set_2070.cell_addr_data.data = 30;
////				retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
//				printf("%02d/%02d/%04d %02d:%02d:%02d Disabling control of arterial controller: hour=%d DOW=%d\n", 
//					ltime->tm_mon+1, 
//					ltime->tm_mday, 
//					ltime->tm_year+1900, 
//					ltime->tm_hour,  
//					ltime->tm_min, 
//					ltime->tm_sec, 
//					db_urms_status.hour, 
//					dow
//				);
//				no_control_sav = 1;
////				set_detector(&detector_block_sav, fpin, fpout, detector, verbose);
//			}
//		}
//		else {
//			no_control = 0;
//			db_urms.no_control = 0;
//			if( no_control_sav == 1) {
//				printf("%02d/%02d/%04d %02d:%02d:%02d Enabling control of arterial controller: hour=%d DOW=%d\n", 
//					ltime->tm_mon+1, 
//					ltime->tm_mday, 
//					ltime->tm_year+1900, 
//					ltime->tm_hour,  
//					ltime->tm_min, 
//					ltime->tm_sec, 
//					db_urms_status.hour, 
//					dow
//				);
//////				set_detector(&detector_block, fpin, fpout, detector, verbose);
//				no_control_sav = 0;
//			}
//		}
//	}
		if(!use_db)
			TIMER_WAIT(ptmr);
	}
	return retval;
}

int OpenTSCPConnection(char *controllerIP, char *port) {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int sfd, s;
        /* Obtain address(es) matching host/port */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* TCP socket */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;     /* Any protocol */
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
                        break;              /* Success */
                }
                perror("connect");
                close(sfd);
        }
        if (rp == NULL) {                /* No address succeeded */
                fprintf(stderr, "Could not connect\n");
                return -1;
        }
        freeaddrinfo(result);       /* No longer needed */
        return sfd;
}

#define RED	0
#define YELOW	1
#define GREEN	2

char interval_color[] = {
	GREEN,
	RED,
	GREEN,
	RED,
	GREEN,
	GREEN,
	GREEN,
	GREEN,
	RED,
	RED,
	RED,
	RED,
	YELOW,
	YELOW,
	RED,
	RED
};

int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status) {


        struct timeb timeptr_raw;
        struct tm time_converted;
        static struct timespec start_time;
        struct timespec end_time;
        int i;
        unsigned char interval_temp = 0;
	static unsigned char greens_sav = 0;

        memset(pphase_status, 0, sizeof(phase_status_t));
        pphase_status->greens = greens;
        for(i=0; i<8; i++) {
                if(pstatus->active_phase & (1 << i)) {
                        if( (pstatus->active_phase & (1 << i)) <= 8) {
                                interval_temp = pstatus->interval & 0xf;
                                pphase_status->phase_status_colors[i] = interval_color[interval_temp];
                                if( (interval_temp == 0xc) || (interval_temp == 0xd) )
                                        pphase_status->yellows |= pstatus->active_phase & 0xf;
                        }
                        else
                                interval_temp = (pstatus->interval >> 4) & 0xf;
                                pphase_status->phase_status_colors[i] = interval_color[interval_temp];
                                if( (interval_temp == 0xc) || (interval_temp == 0xd) )
                                        pphase_status->yellows |= pstatus->active_phase & 0xf0;
                }
        }
        pphase_status->reds = ~(pphase_status->greens | pphase_status->yellows);

	if( (greens_sav == 0x0) && ((greens  == 0x44 )) )
		clock_gettime(CLOCK_REALTIME, &start_time);
	if( (greens_sav == 0x44) && ((greens  == 0x40 )) ) {
		clock_gettime(CLOCK_REALTIME, &end_time);
		printf("process_phase_status: Green time for phase 3 %f sec\n\n", 
			(end_time.tv_sec + (end_time.tv_nsec/1.0e9)) -
			(start_time.tv_sec +(start_time.tv_nsec/1.0e9))
		);
	}
	if( (greens_sav == 0x22) && ((greens & greens_sav) == 0 )) {
		printf("\n\nPhases 2 and 6 should be yellow now\n");
		pphase_status->barrier_flag = 1;
	}
	else
		pphase_status->barrier_flag = 0;
	greens_sav = greens;
		

//                Get time of day and save in the database.
        ftime ( &timeptr_raw );
        localtime_r ( &timeptr_raw.time, &time_converted );
        pphase_status->ts.hour = time_converted.tm_hour;
        pphase_status->ts.min = time_converted.tm_min;
        pphase_status->ts.sec = time_converted.tm_sec;
        pphase_status->ts.millisec = timeptr_raw.millitm;

        return 0;
}	

int get_detector(int wait_for_data, gen_mess_typ *readBuff, int fpin, int fpout, char detector, char verbose) {
        int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
        int ser_driver_retval;
        tsmss_get_msg_request_t detector_get_request;

        if(verbose != 0)
                printf("get_detector 1: Starting get_detector request\n");
        detector_get_request.get_hdr.start_flag = 0x7e;
        detector_get_request.get_hdr.address = 0x05;
        detector_get_request.get_hdr.control = 0x13;
        detector_get_request.get_hdr.ipi = 0xc0;
        detector_get_request.get_hdr.mess_type = 0x87;
        detector_get_request.get_hdr.page_id = 0x07;
//        detector_get_request.get_hdr.block_id = (detector/NUM_DET_PER_BLOCK) + 1;//Block ID 1=det 1-4
        detector_get_request.get_tail.FCSmsb = 0x00;
        detector_get_request.get_tail.FCSlsb = 0x00;

        /* Now append the FCS. */
        msg_len = sizeof(tsmss_get_msg_request_t) - 4;
        fcs_hdlc(msg_len, &detector_get_request, verbose);

        FD_ZERO(&writefds);
        FD_SET(fpout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 56");
                outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
                printf("get_detector 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
                return -3;
        }
        write ( fpout, &detector_get_request, msg_len+4 );
        fflush(NULL);
        sleep(2);


        ser_driver_retval = 100;

        if(wait_for_data && readBuff) {
                FD_ZERO(&readfds);
                FD_SET(fpin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 57");
                        inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
                        printf("get_detector 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(readBuff, fpin, verbose);
                if(ser_driver_retval == 0) {
                        printf("get_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("get_detector 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
        return 0;
}

int set_detector(detector_msg_t *pdetector_set_request, int fpin, int fpout, char detector, char verbose) {
        int msg_len;
        fd_set readfds;
        fd_set writefds;
        int selectval = 1000;
        struct timeval timeout;
        char *inportisset = "not yet initialized";
        char *outportisset = "not yet initialized";
        int ser_driver_retval;
        int wait_for_data = 1;
        gen_mess_typ readBuff;
        char *tempbuf = (char *)pdetector_set_request;
        int i;
printf("set_detector input message: ");
for(i=0; i<sizeof(detector_msg_t); i++)
        printf("%#hhx ", tempbuf[i]);
printf("\n");

        if(verbose != 0)
                printf("set_detector 1: Starting set_detector request\n");
        pdetector_set_request->detector_hdr.start_flag = 0x7e;
        pdetector_set_request->detector_hdr.address = 0x05;
        pdetector_set_request->detector_hdr.control = 0x13;
        pdetector_set_request->detector_hdr.ipi = 0xc0;
        pdetector_set_request->detector_hdr.mess_type = 0x96;
        pdetector_set_request->detector_hdr.page_id = 0x07;
//        pdetector_set_request->detector_hdr.block_id = (detector/NUM_DET_PER_BLOCK) + 1;//Block ID 1=det 1-4
        pdetector_set_request->detector_tail.FCSmsb = 0x00;
        pdetector_set_request->detector_tail.FCSlsb = 0x00;

        // Now append the FCS. 
        msg_len = sizeof(detector_msg_t) - 4;
        fcs_hdlc(msg_len, pdetector_set_request, verbose);

printf("set_detector output message: ");
for(i=0; i<sizeof(detector_msg_t); i++)
        printf("%#hhx ", tempbuf[i]);
printf("\n");

        FD_ZERO(&writefds);
        FD_SET(fpout, &writefds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if( (selectval = select(fpout+1, NULL, &writefds, NULL, &timeout)) <=0) {
                perror("select 14");
                outportisset = (FD_ISSET(fpout, &writefds)) == 0 ? "no" : "yes";
                printf("set_detector 2: fpout %d selectval %d outportisset %s\n", fpout, selectval, outportisset);
                return -3;
        }
        write ( fpout, pdetector_set_request, sizeof(detector_msg_t));
        fflush(NULL);
        sleep(2);

        ser_driver_retval = 100;
        if(wait_for_data) {
                FD_ZERO(&readfds);
                FD_SET(fpin, &readfds);
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                if( (selectval = select(fpin+1, &readfds, NULL, NULL, &timeout)) <=0) {
                        perror("select 15");
                        inportisset = (FD_ISSET(fpin, &readfds)) == 0 ? "no" : "yes";
                        printf("set_detector 3: fpin %d selectval %d inportisset %s\n", fpin, selectval, inportisset);
                        return -2;
                }
                ser_driver_retval = ser_driver_read(&readBuff, fpin, verbose);
                if(ser_driver_retval == 0) {
                        printf("set_detector 4: Lost USB connection\n");
                        return -1;
                }
        }
        if(verbose != 0)
                printf("set_detector 5-end: fpin %d selectval %d inportisset %s fpout %d selectval %d outportisset %s ser_driver_retval %d\n", fpin, selectval, inportisset, fpout, selectval, outportisset, ser_driver_retval);
        return 0;
}
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>
       #include <stdio.h>
       #include <stdlib.h>
       #include <unistd.h>
       #include <string.h>

       #define BUF_SIZE 500

       int
       OpenUDP(char *ipaddr, char *port)
       {
           struct addrinfo hints;
           struct addrinfo *result, *rp;
           int sfd, s, j;
           size_t len;
           ssize_t nread;
           char buf[BUF_SIZE];

           /* Obtain address(es) matching host/port */

           memset(&hints, 0, sizeof(struct addrinfo));
           hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
           hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
           hints.ai_flags = 0;
           hints.ai_protocol = 0;          /* Any protocol */

           s = getaddrinfo(ipaddr, port, &hints, &result);
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
               if (sfd == -1)
                   continue;

               if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
                   break;                  /* Success */

           }

           if (rp == NULL) {               /* No address succeeded */
               fprintf(stderr, "Could not connect\n");
               return -1;
           }
	   else
		return sfd;

       }
