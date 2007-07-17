/*FILE: rd3418.c   Read AB3418 messages from 2070 Controller
 *
 *
 * Copyright (c) 2006   Regents of the University of California
 *
 *	static char rcsid[] = "$Id$";
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
#include "clt_vars.h"
#include "atsc.h"
#include "msgs.h"
#include "fcs.h"

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
//int jjj;

	while ((opt = getopt(argc, argv, "d:x:")) != -1)
	{
		switch (opt)
		{
		  case 'd':
			domain = strdup(optarg);
			break;
		  case 'x':
			xport = atoi(optarg);	
			break;
		  default:
			printf( "Usage %s: clt_update -d [domain] ", argv[0]);
			printf("-x [xport]\n");
			exit(1);
		}
	}
	get_local_name(hostname, MAXHOSTNAMELEN+1);
	if ((pclt = db_list_init( argv[0], hostname, domain, xport,
			db_vars_list, NUM_DB_VARS, NULL, 0))
			== NULL)
	{
		printf("Database initialization error in %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Initialize serial port. */  
	fpin = open( SERIAL_DEVICE_NAME,  O_RDONLY );
	if ( fpin <= 0 ) {
		printf( "Error opening device %s for input\n", SERIAL_DEVICE_NAME );
	}

	/* Initialize serial port. */  
	fpout = open( SERIAL_DEVICE_NAME,  O_WRONLY | O_NONBLOCK );
	if ( fpout <= 0 ) {
		printf( "Error opening device %s for output\n", SERIAL_DEVICE_NAME );
	    return;
	}

	chid = ChannelCreate(0);
//	printf("chid %d\n", chid);

	if ((ptmr = timer_init(25, chid)) == NULL) {
		printf("timer_init failed\n");
		exit(1);
	}

	/*	set jmp */
	if ( setjmp(exit_env) != 0)
	    {
	    /* Log out from the database. */
	    if (pclt != NULL)
#ifdef COMPARE
		clt_destroy( pclt, DB_ATSC2_VAR, DB_ATSC2_VAR );
#else
		clt_destroy( pclt, DB_ATSC_VAR, DB_ATSC_VAR );
#endif
	        clt_logout( pclt );

		exit( EXIT_SUCCESS);
	}
	else 
		sig_ign( sig_list, sig_hand);

	while(1)
	    {
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
//printf("New msg request\n");
//for (jjj=0; jjj<msg_len+2; jjj++)
// printf("%x ",writeBuff.gen_mess.data[jjj]);
//printf("\n");
	        write ( fpout, &writeBuff, msg_len+2 );

	    if (ser_driver_read ( &readBuff ) )
	        {
//printf("msg %x ",readBuff.data[4]);
	        switch( readBuff.data[4] )
	            {
	            case 0xcc:    // GetLongStatus8 message
	                /* Get time of day and save in the database. */
	                ftime ( &timeptr_raw );
	                localtime_r ( &timeptr_raw.time, &time_converted );
	                atsc.ts.hour = time_converted.tm_hour;
	                atsc.ts.min = time_converted.tm_min;
	                atsc.ts.sec = time_converted.tm_sec;
	                atsc.ts.millisec = timeptr_raw.millitm;

//printf("%02d:%02d:%02d:%03d\n",atsc.ts.hour,atsc.ts.min,atsc.ts.sec,atsc.ts.millisec );
	                pget_long_status8_resp_mess = (get_long_status8_resp_mess_typ *) &readBuff;
		
			atsc.phase_status_greens[0] = pget_long_status8_resp_mess->active_phase;
			interval_mask = pget_long_status8_resp_mess->interval;
			interval_mask = interval_mask & 0x0f;
//printf("%x ",interval_mask);
			if (( interval_mask == 0x0c) || (interval_mask == 0x0d) ||
			    (interval_mask == 0x0e ) || (interval_mask == 0x0f))
			atsc.phase_status_greens[0] = atsc.phase_status_greens[0] & 0xf0;
			interval_mask = pget_long_status8_resp_mess->interval;
			interval_mask = interval_mask & 0xf0;
//printf("%x ",interval_mask);
			if (( interval_mask == 0xc0) || (interval_mask == 0xd0) ||
			    (interval_mask == 0xe0 ) || (interval_mask == 0xf0))
			atsc.phase_status_greens[0] = atsc.phase_status_greens[0] & 0x0f;
//printf("%x %x\n", atsc.phase_status_greens[0],pget_long_status8_resp_mess->interval);

	                atsc.info_source = ATSC_SOURCE_AB3418;
//	    		printf("write to database\n");
#ifdef COMPARE
	    		db_clt_write(pclt, DB_ATSC2_VAR, sizeof(atsc_typ), &atsc);
#else
	    		db_clt_write(pclt, DB_ATSC_VAR, sizeof(atsc_typ), &atsc);
#endif
	                break;

	            default:
	                printf("Unknown message type : 0x%x\n", readBuff.data[4] );
	                break;
	            }  
		TIMER_WAIT(ptmr);
	        }
	    }

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
//printf("Ready to read:\n");
	    read ( fpin, &msgbuf[0], 1);
//printf("%x \n",msgbuf[0]);
	    }

//printf("\n");

	/* Read next character.  If this is 0x7e, then this is really
	 * the start of new message, previous 0x7e was end of previous message. */
	read ( fpin, &msgbuf[1], 1 );
//printf("%x ", msgbuf[1] );
	if ( msgbuf[1] != 0x7e )
	{
	    ii=2;
//		printf("%x ",msgbuf[1]);
	}
	else 
	{
	    ii=1;
//		printf("\n");
	}

	/* Header found, now read remainder of message. Continue reading
	 * until end flag is found (0x7e).  If more than 95 characters are
	 * read, this message is junk so just take an error return. */
	for ( i=ii; i<100; i++ )
	    {
	    read ( fpin, &msgbuf[i], 1);
//printf("%x ", msgbuf[i]);
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
//printf("%x ", msgbuf[i+1] );
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
//printf("newfcs=%x\n",newfcs);
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
