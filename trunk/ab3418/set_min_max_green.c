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

#include <sys_os.h>
#include <sys_rt.h>
#include <timestamp.h>

#include "db_clt.h"
#include "db_utils.h"
#include "atsc_clt_vars.h"
#include "atsc.h"
#include "msgs.h"
#include "fcs.h"

#define DEBUG_TRIG

int fpin;	
int fpout;
static jmp_buf exit_env;

static bool_typ ser_driver_read( gen_mess_typ *pMessagebuff);

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

#ifdef COMPARE
static db_id_t db_vars_list[] = {
        {DB_ATSC2_VAR, sizeof(atsc_typ)}};
#else
static db_id_t db_vars_list[] = {
        {DB_ATSC_VAR, sizeof(atsc_typ)}};
#endif 

#define NUM_DB_VARS sizeof(db_vars_list)/sizeof(db_id_t)

int main( int argc, char *argv[] )
{
	char *domain = DEFAULT_SERVICE;
	int opt;
	int xport = COMM_PSX_XPORT;
        int interval = 25;    // number of milliseconds between calls
	int i;
	gen_mess_typ    readBuff;
	char hostname[MAXHOSTNAMELEN];
	db_clt_typ *pclt;              /* Database client pointer */
	atsc_typ atsc;
	mess_union_typ writeBuff;
	int msg_len;
	char *pchar;
	int j;
	int chid;
	posix_timer_typ *ptmr;	
	get_long_status8_resp_mess_typ *pget_long_status8_resp_mess;
	struct timeb timeptr_raw;
	struct tm time_converted;
	unsigned char interval_mask;
	static unsigned char last_greens = 0;
	timestamp_t ts, last_ts, elapsed_ts;
	int max_green = -1;
	int min_green = -1;
	int phase = -1; 

int jjj;

	while ((opt = getopt(argc, argv, "d:i:p:G:g:")) != -1)
	{
		switch (opt)
		{
		  case 'd':
			domain = strdup(optarg);
			break;
                  case 'i':
                        interval = atoi(optarg);
                        break;
                  case 'p':
			phase = atoi(optarg);	
			break;
                  case 'G':
			max_green = (int)strtol(optarg, NULL, 0);	
			break;
                  case 'g':
			min_green = (int)strtol(optarg, NULL, 0);	
			break;
		  default:
			printf( "Usage %s: clt_update -d [domain] ", argv[0]);
			printf("-x [xport]\n");
			exit(1);
		}
	}
	if( (phase < 0) || ( (min_green < 0) && (max_green < 0)) ) {
		printf("Usage: %s -g <minimum green time> -G <maximum green time> -p <phase>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
//	get_local_name(hostname, MAXHOSTNAMELEN+1);
//printf("hostname %s\n",hostname);
//	if ((pclt = db_list_init( argv[0], hostname, domain, xport,
//			db_vars_list, NUM_DB_VARS, NULL, 0))
//			== NULL)
//	{
//		printf("Database initialization error in %s\n", argv[0]);
//		exit(EXIT_FAILURE);
//	}

	/* Initialize serial port. */  
	fpin = open( SERIAL_DEVICE_NAME,  O_RDONLY );
	if ( fpin <= 0 ) {
		printf( "Error opening device %s for input\n", SERIAL_DEVICE_NAME );
	}

	/* Initialize serial port. */  
	fpout = open( SERIAL_DEVICE_NAME,  O_WRONLY | O_NONBLOCK );
	if ( fpout <= 0 ) {
		printf( "Error opening device %s for output\n", SERIAL_DEVICE_NAME );
	    return -1;
	}

	chid = ChannelCreate(0);
	printf("chid %d\n", chid);

	if ((ptmr = timer_init(interval, chid)) == NULL) {
		printf("timer_init failed\n");
		exit(1);
	}

	/*	set jmp */
	if ( setjmp(exit_env) != 0)
	    {
	    /* Log out from the database. */
	    if (pclt != NULL)
#ifdef COMPARE
//		clt_destroy( pclt, DB_ATSC2_VAR, DB_ATSC2_VAR );
#else
//		clt_destroy( pclt, DB_ATSC_VAR, DB_ATSC_VAR );
#endif
//	        clt_logout( pclt );

		exit( EXIT_SUCCESS);
	}
	else 
		sig_ign( sig_list, sig_hand);

//	while(1)
//	    {
printf("Starting SetControllerTimingData request\n");
	    /* Send the message to request GetLongStatus8 from 2070. */
	writeBuff.set_controller_timing_data_mess.start_flag = 0x7e;
	writeBuff.set_controller_timing_data_mess.address = 0x05;
	writeBuff.set_controller_timing_data_mess.control = 0x13;
	writeBuff.set_controller_timing_data_mess.ipi = 0xc0;
	writeBuff.set_controller_timing_data_mess.mess_type = 0x99;
	writeBuff.set_controller_timing_data_mess.num_cells = 0x2;
	writeBuff.set_controller_timing_data_mess.cell1_addr[0] = 0x01;
	writeBuff.set_controller_timing_data_mess.cell1_addr[1] = 0x92;
	writeBuff.set_controller_timing_data_mess.cell1_data = 0x02;
	writeBuff.set_controller_timing_data_mess.cell2_addr[0] = 0x01;
	if(max_green >= 0) {
		writeBuff.set_controller_timing_data_mess.cell2_addr[1] = 
			((phase << 4) + 8) & 0xff;
		writeBuff.set_controller_timing_data_mess.cell2_data= max_green;
	}
	if(min_green >= 0) {
		writeBuff.set_controller_timing_data_mess.cell2_addr[1] = 
			((phase << 4) + 2) & 0xff;
		writeBuff.set_controller_timing_data_mess.cell2_data= min_green;
	}
printf("phase %x cell2_addr[1] %hhx\n", phase, writeBuff.set_controller_timing_data_mess.cell2_addr[1]);
	writeBuff.set_controller_timing_data_mess.FCSmsb = 0x00;
	writeBuff.set_controller_timing_data_mess.FCSlsb = 0x00;

	    /* Now append the FCS. */
	    msg_len = sizeof(set_controller_timing_data_t) - 4;
	    pchar = (char *) &writeBuff;
	    get_modframe_string( pchar+1, &msg_len );

	    /* Check for any 0x7e in message and replace with the
	     * combo 0x7d, 0x5e.  Replace any 0x7d with 0x7d, 0x5d. */
	    for( i=1; i <= msg_len; i++)
	        {
	        /* Within a message, replace any 0x7e with 0x7d, 0x5e. */
	        if ( writeBuff.gen_mess.data[i] == 0x7e )
	            {
	            for ( j=msg_len; j > i; j-- )
	                {
	                writeBuff.gen_mess.data[j+1] = writeBuff.gen_mess.data[j];
	                }
	            writeBuff.gen_mess.data[i] = 0x7d;
	            writeBuff.gen_mess.data[i+1] = 0x5e;
	            msg_len++;
	            i++;
	            }

	        /* Within a message, replace any 0x7d with 0x7d, 0x5d. */
	        if ( writeBuff.gen_mess.data[i] == 0x7d )
	            {
	            for ( j=msg_len; j > i; j-- )
	                {
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
printf("New msg request\n");
for (jjj=0; jjj<msg_len+2; jjj++)
 printf("%x ",writeBuff.gen_mess.data[jjj]);
printf("\n");
	        write ( fpout, &writeBuff, msg_len+2 );
fflush(NULL);
/*
	    if (ser_driver_read ( &readBuff ) )
	        {
printf("msg %x ",readBuff.data[4]);
	        switch( readBuff.data[4] )
	            {
	            case 0xcc:    // GetLongStatus8 message
	                /* Get time of day and save in the database. 
	                ftime ( &timeptr_raw );
	                localtime_r ( &timeptr_raw.time, &time_converted );
	                atsc.ts.hour = time_converted.tm_hour;
	                atsc.ts.min = time_converted.tm_min;
	                atsc.ts.sec = time_converted.tm_sec;
	                atsc.ts.millisec = timeptr_raw.millitm;

printf("%02d:%02d:%02d:%03d\n",atsc.ts.hour,atsc.ts.min,atsc.ts.sec,atsc.ts.millisec );
	                pget_long_status8_resp_mess = (get_long_status8_resp_mess_typ *) &readBuff;
		
			atsc.phase_status_greens[0] = pget_long_status8_resp_mess->active_phase;
			interval_mask = pget_long_status8_resp_mess->interval;
			interval_mask = interval_mask & 0x0f;
printf("%x ",interval_mask);
			if (( interval_mask == 0x0c) || (interval_mask == 0x0d) ||
			    (interval_mask == 0x0e ) || (interval_mask == 0x0f))
			atsc.phase_status_greens[0] = atsc.phase_status_greens[0] & 0xf0;
			interval_mask = pget_long_status8_resp_mess->interval;
			interval_mask = interval_mask & 0xf0;
printf("%x ",interval_mask);
			if (( interval_mask == 0xc0) || (interval_mask == 0xd0) ||
			    (interval_mask == 0xe0 ) || (interval_mask == 0xf0))
			atsc.phase_status_greens[0] = atsc.phase_status_greens[0] & 0x0f;
printf("%x %x\n", atsc.phase_status_greens[0],pget_long_status8_resp_mess->interval);

	                atsc.info_source = ATSC_SOURCE_AB3418;
	    		printf("write to database\n");
			if (atsc.phase_status_greens[0] != last_greens) {

#ifdef COMPARE
//				db_clt_write(pclt, DB_ATSC2_VAR,
//					 sizeof(atsc_typ), &atsc);
#else
//				db_clt_write(pclt, DB_ATSC_VAR,
//					 sizeof(atsc_typ), &atsc);
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
	                break;

	            default:
	                printf("Unknown message type : 0x%x\n", readBuff.data[4] );
	                break;
	            }  
		TIMER_WAIT(ptmr);
	        }
*/
//	    }
}

static bool_typ ser_driver_read( gen_mess_typ *pMessagebuff) 
{
unsigned char msgbuf [100];
int i;
int ii;
unsigned short oldfcs;
unsigned short newfcs;

	/* Read from serial port. */
	/* Blocking read is used, so control doesn't return unless data is
	 * available.  Keep reading until the beginning of a message is 
	 * determined by reading the start flag 0x7e.  */
	memset( msgbuf, 0x0, 100 );
	while ( msgbuf[0] != 0x7e )
	    {
printf("1: Ready to read:\n");
	    read ( fpin, &msgbuf[0], 1);
printf("%x \n",msgbuf[0]);
fflush(stdout);
	    }

printf("\n");

	/* Read next character.  If this is 0x7e, then this is really
	 * the start of new message, previous 0x7e was end of previous message. */
	read ( fpin, &msgbuf[1], 1 );
printf("%x ", msgbuf[1] );
	if ( msgbuf[1] != 0x7e )
	{
	    ii=2;
		printf("%x ",msgbuf[1]);
	}
	else 
	{
	    ii=1;
		printf("\n");
	}

	/* Header found, now read remainder of message. Continue reading
	 * until end flag is found (0x7e).  If more than 95 characters are
	 * read, this message is junk so just take an error return. */
	for ( i=ii; i<100; i++ )
	    {
	    read ( fpin, &msgbuf[i], 1);
printf("%x ", msgbuf[i]);
fflush(stdout);
	    if ( i>95 )
	        return( FALSE );
	    if ( msgbuf[i] == 0x7e )
	        break;
	    /* If the byte read was 0x7d read the next byte.  If the next
	     * byte is 0x5e, convert the first byte to 0x7e.  If the next
	     * byte is 0x5d, the first byte really should be 0x7d.  If
	     * the next byte is neither 0x5e nor 0x5d, take an error exit. */
	    if ( msgbuf[i] == 0x7d )
	        {
	        read ( fpin, &msgbuf[i+1], 1 );
printf("%x ", msgbuf[i+1] );
	        if ( msgbuf[i+1] == 0x5e )
	            msgbuf[i] = 0x7e;
	        else if ( msgbuf[i+1] != 0x5d )
	            {
	            printf("Illegal 0x7d\n");
	            return (FALSE);
	            }
	        }
	    }

	memcpy( pMessagebuff->data, &msgbuf[0], 100);

	oldfcs = ~(msgbuf[i-2] << 8) | ~msgbuf[i-1];
	newfcs = pppfcs( oldfcs, &msgbuf[1], i-1 );
printf("newfcs=%x\n",newfcs);
	if ( newfcs != 0xf0b8 )
	    {
		printf( "FCS error, msg type %x\n", msgbuf[4] );
	    return (FALSE);
	    }
	else
	    {
	    return (TRUE);
	    }
}
