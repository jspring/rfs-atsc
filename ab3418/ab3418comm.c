/* ab3418comm - AB3418<-->database communicator
*/

#include <db_include.h>
#include "msgs.h"
#include "ab3418_lib.h"
#include "ab3418comm.h"

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
        ERROR,
};

int OpenTSCPConnection(char *controllerIP, char *port);
int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status);

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

int main(int argc, char *argv[]) {

        db_clt_typ *pclt;              /* Database client pointer */
	char hostname[MAXHOSTNAMELEN];
	char *domain = DEFAULT_SERVICE;
	int xport = COMM_OS_XPORT;
	posix_timer_typ *ptmr;
	trig_info_typ trig_info;
	int ipc_message_error_ctr = 0;

	int i;
	int fpin = 0;
	int fpout = 0;
	FILE *fifofd = 0;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	phase_status_t phase_status;
	int msg_len;
	int wait_for_data = 1;
	gen_mess_typ readBuff;
	phase_timing_t phase_timing[MAX_PHASES];
	int retval;
	int check_retval;
	char port[14] = "/dev/ttyS0";
	struct timespec start_time;
	struct timespec end_time;
	int opt;
	int use_db = 0;
	int interval = 100;
	int verbose = 0;
	unsigned char greens = 0;
	unsigned char create_db_vars = 0;

        while ((opt = getopt(argc, argv, "p:uvi:c")) != -1)
        {
                switch (opt)
                {
                  case 'p':
			memset(port, 0, sizeof(port));
                        strncpy(&port[0], optarg, 13);
                        break;
                  case 'u':
                        use_db = 1;
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  case 'i':
                        interval = atoi(optarg);
                        break;
                  case 'c':
                        create_db_vars = 1;
                        break;
		  default:
			fprintf(stderr, "Usage: %s -p <port, (def. /dev/ttyS0)> -u (use db) -v (verbose) -i <loop interval>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if(use_db) {
		get_local_name(hostname, MAXHOSTNAMELEN);
		if(create_db_vars) {
		    if ( ((pclt = db_list_init(argv[0], hostname,
			domain, xport, db_vars_ab3418comm, NUM_DB_VARS, 
			db_trig_list, NUM_TRIG_VARS)) == NULL))
			exit(EXIT_FAILURE);
		}
		else {
		    if ( ((pclt = db_list_init(argv[0], hostname,
			domain, xport, NULL, 0, 
			db_trig_list, NUM_TRIG_VARS)) == NULL))
			exit(EXIT_FAILURE);
		}

		if (setjmp(exit_env) != 0) {
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

	}

	if ((ptmr = timer_init( interval, ChannelCreate(0))) == NULL) {
		fprintf(stderr, "Unable to initialize delay timer\n");
		exit(EXIT_FAILURE);
	}

        /* Initialize serial port. */
	check_retval = check_and_reconnect_serial(0, &fpin, &fpout, port);

//	memset(&overlapreadBuff, 0, sizeof(overlapreadBuff));
//	retval = get_overlap(wait_for_data, &overlapreadBuff, fpin, fpout, verbose);
//	if(retval < 0) 
//		check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
//
//	retval = set_overlap( (overlap_msg_t *)&overlapreadBuff, fpin, fpout, verbose);
//	if(retval < 0) 
//		check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);

	memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070));

	printf("main 1: getting timing settings before infinite loop\n");
	for(i=0; i<MAX_PHASES; i++) {
		db_timing_get_2070.phase = i+1;	
		db_timing_get_2070.page = 0x100; //phase timing page	
		retval = get_timing(&db_timing_get_2070, wait_for_data, &phase_timing[i], &fpin, &fpout, verbose);
		db_clt_write(pclt, DB_PHASE_1_TIMING_VAR + i, sizeof(phase_timing_t), &phase_timing[i]);
		usleep(500000);
	}

	while(1) {
		if(use_db)
			retval = clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		if( DB_TRIG_VAR(&trig_info) == DB_2070_TIMING_SET_VAR ) {
			db_clt_read(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_timing_set_2070_t), &db_timing_set_2070);
			retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
		}
		else {	
			retval = get_status(wait_for_data, &readBuff, fpin, fpout, verbose);
			if(retval < 0) 
				check_retval = check_and_reconnect_serial(retval, &fpin, &fpout, port);
			if(use_db && (retval == 0) ) {
				db_clt_write(pclt, DB_TSCP_STATUS_VAR, sizeof(get_long_status8_resp_mess_typ), (get_long_status8_resp_mess_typ *)&readBuff);
				retval = process_phase_status( (get_long_status8_resp_mess_typ *)&readBuff, verbose, greens, &phase_status);
				db_clt_write(pclt, DB_PHASE_STATUS_VAR, sizeof(phase_status_t), &phase_status);
				fifofd = fopen("/tmp/blah", "w");
				for(i=0; i<8; i++) {
					fprintf(fifofd, "%d", phase_status.phase_status_colors[i]);
				}
				fclose(fifofd);
			if(verbose) 
				print_status( (get_long_status8_resp_mess_typ *)&readBuff);
			}
			else 
				if(retval < 0) 
					printf("get_status returned negative value: %d\n", retval);
			usleep(80000);
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

/*
int process_phase_status( get_long_status8_resp_mess_t *pstatus, get_short_status_resp_t *pshort_status, phase_status_t *pphase_status, int verbose) {

        struct timeb timeptr_raw;
        struct tm time_converted;
        int i;
        unsigned char interval_temp = 0;

        memset(pphase_status, 0, sizeof(phase_status_t));
        pphase_status->greens = pshort_status->greens;
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

//                Get time of day and save in the database.
        ftime ( &timeptr_raw );
        localtime_r ( &timeptr_raw.time, &time_converted );
        pphase_status->ts.hour = time_converted.tm_hour;
        pphase_status->ts.min = time_converted.tm_min;
        pphase_status->ts.sec = time_converted.tm_sec;
        pphase_status->ts.millisec = timeptr_raw.millitm;

        return 0;
}
*/

int process_phase_status( get_long_status8_resp_mess_typ *pstatus, int verbose, unsigned char greens, phase_status_t *pphase_status) {


        struct timeb timeptr_raw;
        struct tm time_converted;
        int i;
        unsigned char interval_temp = 0;

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

//                Get time of day and save in the database.
        ftime ( &timeptr_raw );
        localtime_r ( &timeptr_raw.time, &time_converted );
        pphase_status->ts.hour = time_converted.tm_hour;
        pphase_status->ts.min = time_converted.tm_min;
        pphase_status->ts.sec = time_converted.tm_sec;
        pphase_status->ts.millisec = timeptr_raw.millitm;

        return 0;
}	
