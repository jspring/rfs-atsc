/*FILE: rd3418.c   Read AB3418 messages from 2070 Controller
 *
 *
 * Copyright (c) 2006   Regents of the University of California
 *
 *	static char rcsid[] = "$Id: rd3418.c 1089 2007-10-04 05:23:17Z dickey $";
 *
 *
 *	$Log$
 *
 *
 *  Process to read AB3418 messages from 2070.  We'll expect
 *  to see message types:
 *  (0xcc) - Get long status8 message.
 *
 *  In order for this message to be sent, we must first request it by
 *  sending message type 0x8c (Get long status8 message).
 *
 *  The signals SIGINT, SIGQUIT, and SIGTERM are trapped, and cause the
 *  process to terminate.  Upon termination log out of the database.
 *
 */

#include <db_include.h>
#include "atsc_clt_vars.h"
#include "atsc.h"
#include "msgs.h"
#include "fcs.h"
#include "set_min_max_green.h"
#include "ab3418_lib.h"

#define DEBUG_TRIG

int fpin;	
int fpout;
static jmp_buf exit_env;
int verbose = 0; 
int veryverbose = 0; 

char *usage = "\nUsage: \n \
set_min_max_green -g <minimum green time> -p <phase> \n \
set_min_max_green -G <maximum green time> -p <phase> \n \
set_min_max_green -c <cell address> -d <data> \n \
set_min_max_green -t (to set controller time to system time) \n \
                  -v verbose \n \
                  -w very verbose \n \
                  -P <port (default \"/dev/ttyUSB0\")> \n \
                  -r read settings \n \
                  -s run standalone (no db)\n" ;

//static bool_typ ser_driver_read( gen_mess_typ *pMessagebuff);
int print_timing(get_long_status8_resp_mess_typ *status);
//int set_timing(mess_union_typ *writeBuff, db_2070_timing_set_t *db_2070_timing_set, int *msg_len); 

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
#undef COMPARE
#ifdef COMPARE
static db_id_t db_vars_list[] = {
        {DB_ATSC2_VAR, sizeof(atsc_typ)}};
#else
static db_id_t db_vars_list[] = {
        {DB_ATSC_VAR, sizeof(atsc_typ)},
        {DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t)}};
#endif 

#define NUM_DB_VARS sizeof(db_vars_list)/sizeof(db_id_t)
int db_trig_list[] =  {
       DB_2070_TIMING_SET_VAR 
};

int NUM_TRIG_VARS = sizeof(db_trig_list)/sizeof(int);

int main( int argc, char *argv[] )
{
	char *domain = DEFAULT_SERVICE;
	int opt;
	int xport = COMM_PSX_XPORT;
        int interval = 1000;    // number of milliseconds between calls
	int i;
	gen_mess_typ    readBuff;
	char hostname[MAXHOSTNAMELEN];
	db_clt_typ *pclt;              /* Database client pointer */
	atsc_typ atsc;
	mess_union_typ writeBuff;
	db_timing_set_2070_t db_2070_timing_set;
	trig_info_typ trig_info;
	int retval;
	int msg_len;
	unsigned char *pchar;
	int j;
	int chid;
	posix_timer_typ *ptmr;	
	get_long_status8_resp_mess_typ *pget_long_status8_resp_mess;
	struct timeb timeptr_raw;
	struct tm time_converted;
	unsigned char interval_mask;
	static unsigned char last_greens = 0;
	timestamp_t ts, last_ts, elapsed_ts;
	char max_green = -1;
	char min_green = -1;
	int phase = -1; 
	int cell_addr = 0; 
	char data = -1;
	int jjj;
	int do_set_time = 0;
	int do_set_controller_timing = 0;
	int do_read_controller_timing = 0;
	int use_db = 1;
	char *port = "/dev/ttyUSB0";
	char portcfg[100];
	
	memset(portcfg, 0, sizeof(portcfg));

	while ((opt = getopt(argc, argv, "d:i:c:p:G:g:vwtsrP:")) != -1)
	{
		switch (opt)
		{
                  case 'd':
			data = (char)strtol(optarg, NULL, 0);	
                        break;
                  case 'i':
                        interval = atoi(optarg);
                        break;
                  case 'c':
			cell_addr = (int)strtol(optarg, NULL, 0);	
			do_set_controller_timing = 1;
			break;
                  case 'p':
			phase = (int)strtol(optarg, NULL, 0);	
			do_set_controller_timing = 1;
			break;
                  case 'G':
			max_green = (char)strtol(optarg, NULL, 0);	
			break;
                  case 'g':
			min_green = (char)strtol(optarg, NULL, 0);	
			break;
                  case 'v':
			verbose = 1;	
			break;
                  case 'w':
			veryverbose = 1;	
			break;
		  case 't':
			do_set_time = 1;
			break;
                  case 's':
                        use_db = 0;
                        break;
		  case 'r':
			do_read_controller_timing = 1;
			break;
                  case 'P':
                        port = strdup(optarg);
                        break;
		  default:
			printf("%s", usage);
			exit(1);
		}
	}
	if(   (do_set_time == 0) && 
	      (do_read_controller_timing == 0) && 
	      ((cell_addr == 0) || (data == -99999999)) && 
	      ((phase < 0) || ((min_green < 0) && (max_green < 0)))
	)
	{
		printf("%s", usage);
		exit(EXIT_FAILURE);
	}
	if(use_db) {
		get_local_name(hostname, MAXHOSTNAMELEN+1);
		if ((pclt = db_list_init( argv[0], hostname, domain, xport,
				db_vars_list, NUM_DB_VARS, db_trig_list, NUM_TRIG_VARS))
				== NULL)
		{
			printf("Database initialization error in %s\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	/* Initialize serial port. */  
	fpin = open( port,  O_RDONLY );
	if ( fpin <= 0 ) {
		printf( "Error opening device %s for input\n", port );
	}

	/* Initialize serial port. */  
	fpout = open( port,  O_WRONLY | O_NONBLOCK );
	if ( fpout <= 0 ) {
		printf( "Error opening device %s for output\n", port );
	    return -1;
	}
	sprintf(portcfg, "/bin/stty -F %s raw 38400", port);
printf("portcfg %s\n", portcfg);
	system(portcfg);

        if(!use_db) {
		if(cell_addr != 0) {
			db_2070_timing_set.cell_addr_data[0].cell_addr = cell_addr;
			db_2070_timing_set.cell_addr_data[0].data = data;
			retval = set_timing(&writeBuff, &db_2070_timing_set, &msg_len, fpin, fpout);
		}
		else {
			db_2070_timing_set.cell_addr_data[0].cell_addr = MINIMUM_GREEN;
			db_2070_timing_set.cell_addr_data[0].data = (unsigned char)min_green;
			db_2070_timing_set.cell_addr_data[1].cell_addr = MAXIMUM_GREEN_1;
			db_2070_timing_set.cell_addr_data[1].data = (unsigned char)max_green;
			db_2070_timing_set.phase = phase;
			retval = set_timing(&writeBuff, &db_2070_timing_set, &msg_len, fpin, fpout);
                if(verbose) {
                        printf("Starting SetControllerTimingData request\n");
                        printf("SetControllerTimingData: phase %d max_green %d min_green %d\n",
                        db_2070_timing_set.phase,
                        db_2070_timing_set.max_green,
                        db_2070_timing_set.min_green
                        );
                        printf("msg_len %d\n", msg_len);
                        printf("main2: New msg request\n");
                        for (jjj=0; jjj<msg_len+2; jjj++)
                                printf("%x ",writeBuff.gen_mess.data[jjj]);
                        printf("\n");
                }
                write ( fpout, &writeBuff, msg_len+2 );
                perror("write set_min_max_timing");
                exit(EXIT_SUCCESS);
		}
        }

	chid = ChannelCreate(0);
//	printf("chid %d\n", chid);

	if ((ptmr = timer_init(interval, chid)) == NULL) {
		printf("timer_init failed\n");
		exit(1);
	}

	/*	set jmp */
	if ( setjmp(exit_env) != 0)
	    {
	    /* Log out from the database. */
	    if (pclt != NULL)
		db_list_done(pclt, db_vars_list, NUM_DB_VARS, db_trig_list, NUM_TRIG_VARS);
		if(fpin)
			close(fpin);
		if(fpout)
			close(fpout);
		exit( EXIT_SUCCESS);
	}
	else 
		sig_ign( sig_list, sig_hand);

	while(1)
	    {
              clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
              if( DB_TRIG_VAR(&trig_info) == DB_2070_TIMING_SET_VAR ) {
		db_clt_read(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t), &db_2070_timing_set);
		if(do_set_controller_timing) {
			if(verbose) {
				printf("Starting SetControllerTimingData request\n");
			}
			retval = set_timing(&writeBuff, &db_2070_timing_set, &msg_len, fpin, fpout);
			if(verbose) {
				printf("main2: New msg request\n");
				for (jjj=0; jjj<msg_len+2; jjj++)
					printf("%x ",writeBuff.gen_mess.data[jjj]);
				printf("\n");
			}
			write ( fpout, &writeBuff, msg_len+2 );
			perror("write set_min_max_timing");
		}
    	      }
	      else {
        	if(do_set_time) {
		if(verbose)
                        printf("Starting SetTime request\n");
                /* Send the message to request GetLongStatus8 from 2070. */
                writeBuff.set_time_mess.start_flag = 0x7e;
                writeBuff.set_time_mess.address = 0x05;
                writeBuff.set_time_mess.control = 0x13;
                writeBuff.set_time_mess.ipi = 0xc0;
                writeBuff.set_time_mess.mess_type = 0x92;

                ftime ( &timeptr_raw );
                localtime_r ( &timeptr_raw.time, &time_converted );
                writeBuff.set_time_mess.day_of_week = time_converted.tm_wday + 1
;
                writeBuff.set_time_mess.month = time_converted.tm_mon + 1;
                writeBuff.set_time_mess.day_of_month = time_converted.tm_mday;
                writeBuff.set_time_mess.year = time_converted.tm_year - 100;
                writeBuff.set_time_mess.hour = time_converted.tm_hour;
                writeBuff.set_time_mess.minute = time_converted.tm_min;
                writeBuff.set_time_mess.second = time_converted.tm_sec;
                writeBuff.set_time_mess.tenths = timeptr_raw.millitm / 100;

                if(verbose) {

                        printf("set_time_mess.day_of_week %d\n",
                                writeBuff.set_time_mess.day_of_week);
                        printf("set_time_mess.month %d\n",
                                writeBuff.set_time_mess.month);
                        printf("set_time_mess.day_of_month %d\n",
                                writeBuff.set_time_mess.day_of_month);
                        printf("set_time_mess.year %d\n",
                                writeBuff.set_time_mess.year);
                        printf("set_time_mess.hour %d\n",
                                writeBuff.set_time_mess.hour);
                        printf("set_time_mess.minute %d\n",
                                writeBuff.set_time_mess.minute);
                        printf("set_time_mess.second %d\n",
                                writeBuff.set_time_mess.second);
                        printf("set_time_mess.tenths %d\n",
                                writeBuff.set_time_mess.tenths);
                }
                writeBuff.set_time_mess.FCSmsb = 0x00;
                writeBuff.set_time_mess.FCSlsb = 0x00;

                /* Now append the FCS. */
                msg_len = sizeof(set_time_t) - 4;
		do_set_time = 0;
        }
	else {
               	 if(verbose)
                        printf("Starting ReadControllerTimingData request\n");
               	 /* Send the message to request GetLongStatus8 from 2070. */
               	 writeBuff.get_long_status8_mess.start_flag = 0x7e;
               	 writeBuff.get_long_status8_mess.address = 0x05;
               	 writeBuff.get_long_status8_mess.control = 0x13;
               	 writeBuff.get_long_status8_mess.ipi = 0xc0;
               	 writeBuff.get_long_status8_mess.mess_type = 0x8c;
               	 writeBuff.get_long_status8_mess.FCSmsb = 0x00;
               	 writeBuff.get_long_status8_mess.FCSlsb = 0x00;

               	 /* Now append the FCS. */
               	 msg_len = sizeof(get_long_status8_mess_typ) - 4;
	}
	pchar = (unsigned char *) &writeBuff;
	get_modframe_string( pchar+1, &msg_len );

	/* Check for any 0x7e in message and replace with the
	* combo 0x7d, 0x5e.  Replace any 0x7d with 0x7d, 0x5d. */
	for( i=1; i <= msg_len; i++) {
	/* Within a message, replace any 0x7e with 0x7d, 0x5e. */
		if ( writeBuff.gen_mess.data[i] == 0x7e ) {
			for ( j=msg_len; j > i; j-- ) {
				writeBuff.gen_mess.data[j+1] = writeBuff.gen_mess.data[j];
			}
			writeBuff.gen_mess.data[i] = 0x7d;
			writeBuff.gen_mess.data[i+1] = 0x5e;
			msg_len++;
			i++;
		}

		/* Within a message, replace any 0x7d with 0x7d, 0x5d. */
		if ( writeBuff.gen_mess.data[i] == 0x7d ) {
			for ( j=msg_len; j > i; j-- ) {
				writeBuff.gen_mess.data[j+1] = writeBuff.gen_mess.data[j];
				writeBuff.gen_mess.data[i] = 0x7d;
				writeBuff.gen_mess.data[i+1] = 0x5d;
				msg_len++;
				i++;
			}
		}
	}

	/* Now add the end flag and send message. */
	writeBuff.gen_mess.data[msg_len+1] = 0x7e;
	if(verbose) {
		printf("New msg request\n");
		for (jjj=0; jjj<msg_len+2; jjj++)
		 printf("%x ",writeBuff.gen_mess.data[jjj]);
		printf("\n");
	}
	        write ( fpout, &writeBuff, msg_len+2 );
		fflush(NULL);
	}
//    if (ser_driver_read ( &readBuff ) ) {
	if(verbose) 
		printf("msg %x ",readBuff.data[4]);
        switch( readBuff.data[4] ) {
            case 0xcc:    // GetLongStatus8 message
		// Get time of day and save in the database. 
                ftime ( &timeptr_raw );
                localtime_r ( &timeptr_raw.time, &time_converted );
                atsc.ts.hour = time_converted.tm_hour;
                atsc.ts.min = time_converted.tm_min;
                atsc.ts.sec = time_converted.tm_sec;
                atsc.ts.millisec = timeptr_raw.millitm;

		if(verbose) 
			printf("%02d:%02d:%02d:%03d\n",atsc.ts.hour,atsc.ts.min,
				atsc.ts.sec,atsc.ts.millisec );
                pget_long_status8_resp_mess = 
			(get_long_status8_resp_mess_typ *) &readBuff;
	
		atsc.phase_status_greens[0] = 
			pget_long_status8_resp_mess->active_phase;
		interval_mask = pget_long_status8_resp_mess->interval;
		interval_mask = interval_mask & 0x0f;
		if(verbose) 
			printf("%x ",interval_mask);
		if (( interval_mask == 0x0c) || (interval_mask == 0x0d) ||
		    (interval_mask == 0x0e ) || (interval_mask == 0x0f))
		atsc.phase_status_greens[0] = 
			atsc.phase_status_greens[0] & 0xf0;
		interval_mask = pget_long_status8_resp_mess->interval;
		interval_mask = interval_mask & 0xf0;
		if(verbose) 
			printf("%x ",interval_mask);
		if (( interval_mask == 0xc0) || (interval_mask == 0xd0) ||
		    (interval_mask == 0xe0 ) || (interval_mask == 0xf0))
		atsc.phase_status_greens[0] = 
			atsc.phase_status_greens[0] & 0x0f;
		if(verbose) 
			printf("%x %x\n", atsc.phase_status_greens[0],
			pget_long_status8_resp_mess->interval);
                atsc.info_source = ATSC_SOURCE_AB3418;
		if (atsc.phase_status_greens[0] != last_greens) {

#ifdef COMPARE
			if(use_db)
			db_clt_write(pclt, DB_ATSC2_VAR,
			 sizeof(atsc_typ), &atsc);
#else
			if(use_db)
			db_clt_write(pclt, DB_ATSC_VAR,
			 sizeof(atsc_typ), &atsc);
#endif
#ifdef DEBUG_TRIG
			get_current_timestamp(&ts);
			print_timestamp(stdout, &ts);
			decrement_timestamp(&ts, &last_ts, &elapsed_ts);
			last_ts = ts;
			printf(" current: 0x%2hhx last: 0x%2hhx elapsed %.3f\n",
				 atsc.phase_status_greens[0], last_greens,
				 TS_TO_MS(&elapsed_ts)/1000.0);
#endif
		}
		last_greens = atsc.phase_status_greens[0];
		if(veryverbose)
			print_timing(pget_long_status8_resp_mess);
		break;
	    case 0xd9:
		printf("SetControllerTiming returned OK\n");
		break;
	    case 0xf9:
		printf("SetControllerTiming error: %hhd index %hhd\n",
			readBuff.data[5], readBuff.data[6]);
		break;
	    default:
	    	printf("Unknown message type : 0x%x\n", readBuff.data[4] );
	    	break;
	}  
//    }
//	TIMER_WAIT(ptmr);
	    }
	return 0;
}

