/*\file
 *	
 *	Send Transit Signal Priority related message.
 *
 *	The message includes: signal phase and countdown time on bus approach, 
 *	and stop/go flag for driver advisory displaying
 *
 *	The program listens to 3 db_typ updates:
 *	1. ix_msg_tpy for signal phase countdown and TSP triggers(generated  by phase170e)
 *	2. bus_prediction_output_typ for generating driver advisory info
 *	3. gps_typ for time synchronization
 *
 *
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
#include "ix_msg.h" 
#include "ix_db_utils.h"
#include "udp_utils.h"
#include "veh_sig.h"
#include "tsp_clt_vars.h"

#define BUFFER_SIZE 200

typedef struct{
	int year;
	int month;
	int day;
} datestamp_t;

static int sig_list[]=
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGALRM,	// for timer
	ERROR
};
static jmp_buf exit_env;
static void sig_hand( int code );
static db_clt_typ *db_trig_set(char *pclient,char *phost,char *pserv,int xport, 
	 unsigned *var_list,int trignums); 
static void db_trig_unset(db_clt_typ *pclt,unsigned *var_list,int trignums);
void get_ds_ts(datestamp_t *pds,timestamp_t *pts);

void do_usage(char *progname)
{
	fprintf(stderr, "Usage %s:\n", progname);
	fprintf(stderr, "-o output destination IP address\n");
	fprintf(stderr, "\t (default to 128.32.129.87, i.e., tlab.path.berkeley.edu\n");
	fprintf(stderr, "-p port for send (default to 49888");
	fprintf(stderr, "-b bus_id in the datahub (default to 1\n");	
	fprintf(stderr, "-v verbose (default to 0)");
	fprintf(stderr, "-h print this usage");
	fprintf(stderr, "\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	// db variables
	db_clt_typ *pclt;	/// data bucket client pointer
	char hostname[MAXHOSTNAMELEN+1];
	char * domain = DEFAULT_SERVICE;
	int xport = COMM_PSX_XPORT;	/// transport mechanism for data bucket
	trig_info_typ trig_info;	/* Trigger info */
	unsigned var = 0;
	int recv_type;
	int trig_list[3];
	int trig_nums = 3;
	// socket variables
	int fdout;		/// output file descriptor
	char *foutname = "128.32.129.87";	/// default output address
	int port_out = 49888;	/// Default destination UDP port 	
	struct sockaddr_in snd_addr;	/// used by sendto
	// arguments
	int option;		/// for getopt
	int bus_id = 1;
	int verbose = 0;	/// extra output to stdout 	
	// local variables
	unsigned char send_buf[BUFFER_SIZE];
	int bytes_sent;
	int bytes_to_send;
	int bus_pred_len;
	int driver_adv_flag;
	float time2go;
	float time_left;
	datestamp_t ds;
	timestamp_t ts;
	timestamp_t bus_pred_ts;
	bus_prediction_output_typ bus_pred;
	bus_pred_len = sizeof(bus_prediction_output_typ);
	
	while ((option = getopt(argc, argv, "o:p:b:vh")) != EOF) 
	{
		switch(option) 
		{
		case 'o':
			foutname = strdup(optarg);
			break;
		case 'p':
			port_out = atoi(optarg);
			break;
		case 'b':
			bus_id = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			do_usage(argv[0]);
			break;		
		default:
			do_usage(argv[0]);
			break;
		}
	}

	// setup db trigger list 
	trig_list[0] = DB_IX_MSG_VAR;
	trig_list[1] = DB_BUS_PREDICTION_OUTPUT_VAR_BASE + bus_id;
	trig_nums = 2;	
	
	// udp socket
	fdout = udp_unicast();
	if (fdout < 0) 
		do_usage(argv[0]);
	set_inet_addr((struct sockaddr_in *) &snd_addr,
			 inet_addr(foutname), port_out);		

	 printf("Sending to %s, max packet size %d\n",
		foutname, BUFFER_SIZE);

	// Log in to the database (shared global memory).  Default to the current host. 
	get_local_name(hostname, MAXHOSTNAMELEN+1);
	if( (pclt = db_trig_set(argv[0],hostname,domain,xport,
		&trig_list[0],trig_nums )) == NULL )
	{
		fprintf(stderr, "%s: database initialization error\n",argv[0]);
	    db_trig_unset(pclt,&trig_list[0],trig_nums);
		close(fdout);
	    return (-1);
	}	
	// Exit code on receiving signal 
	if (setjmp(exit_env) != 0) 
	{
	    db_trig_unset(pclt,&trig_list[0],trig_nums);
		close(fdout);
		return (0);
	} 
	else
		sig_ign( sig_list, sig_hand );

	while (1) 
	{
		// now waite for a trigger
		recv_type = clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		get_ds_ts(&ds,&ts);
		var = DB_TRIG_VAR(&trig_info);
		if (var == trig_list[0])
		{
			// ix_msg_t trigger
			ix_msg_t *pmsg = (ix_msg_t *) malloc(sizeof(ix_msg_t));
			ix_msg_read(pclt,pmsg,DB_IX_MSG_VAR,DB_IX_APPROACH1_VAR); 
			if (verbose == 1)
				ix_msg_print(pmsg);		
			// determine the driver advisory flag for stpo-go
			ix_approach_t *pappr;
			pappr = pmsg->approach_array;
			if ( !(bus_pred.next_signal_number == pmsg->intersection_id &&
				bus_pred.next_signal_number == bus_pred.next_node_number) )
				driver_adv_flag = -1;
			else
			{
				driver_adv_flag = -1;				
				time2go = bus_pred.time_togo_signal - (TS_TO_MS(&ts)-TS_TO_MS(&bus_pred_ts));
				time_left = (float)pappr[0].time_to_next/10.0;
				switch (pappr[0].signal_state)
				{
				case SIGNAL_STATE_GREEN:
					if (time2go > time_left + 2.0) // within 2 seconds of yellow
						driver_adv_flag = 1;
					else
						driver_adv_flag = 0;
					break;
				case SIGNAL_STATE_YELLOW:
					if (time2go > time_left - 2.0) // within 2 seconds of yellow
						driver_adv_flag = 1;
					else
						driver_adv_flag = 0;
					break;
				case SIGNAL_STATE_RED:
					if (time2go > 0.0 && time2go < time_left)
						driver_adv_flag = 1;
					else
						driver_adv_flag = 0;
					break;
				default:
					driver_adv_flag = -1;				
					break;
				}
			}
			// form the packet and sendto
			sprintf(send_buf,"$TSPMSG,%04d-%02d-%02d,%02d:%02d:%02d,%d,%d,%d,%d,%d\n",
				ds.year,ds.month,ds.day,ts.hour,ts.min,ts.sec,ts.millisec,
				pappr[0].signal_state,pappr[0].time_to_next,pmsg->bus_priority_calls,
				pmsg->reserved[0],pmsg->reserved[1]);				
			bytes_to_send = (int)strlen(send_buf);
			bytes_sent = sendto(fdout,send_buf,bytes_to_send,0,
				(struct sockaddr *) &snd_addr,sizeof(snd_addr));
			if (bytes_sent != bytes_to_send) 
			{
				fprintf(stderr,"%s: bytes to send %d bytes sent %d\n", argv[0],
					bytes_to_send,bytes_sent);
			}
			ix_msg_free(&pmsg);
		}
		else if (var == trig_list[1])
		{
			// bus arrival time prediction update
			memset(&bus_pred,0,bus_pred_len);
			if (db_clt_read(pclt,var,bus_pred_len,&bus_pred) == FALSE )
			{
				memset(&bus_pred,0,bus_pred_len);
				continue;
			}
			bus_pred_ts.hour = 	bus_pred.hour;
			bus_pred_ts.min = 	bus_pred.min;
			bus_pred_ts.sec = 	bus_pred.sec;
			bus_pred_ts.millisec = 	bus_pred.millisec;			
		}		
	}
	fprintf(stderr, "%s exiting on error\n", argv[0]);
}

void get_ds_ts(datestamp_t *pds,timestamp_t *pts)
{
	struct timeb timeptr_raw;
	struct tm *time_converted;
	ftime ( &timeptr_raw );
	localtime_r ( &timeptr_raw.time, &time_converted );	
	pds->year = time_converted->tm_year + 1900;
	pds->month = time_converted->tm_mon + 1;
	pds->day = time_converted->tm_mday;
	pts->hour = time_converted->tm_hour;
	pts->min = time_converted->tm_min;
	pts->sec = time_converted->tm_sec;
	pts->millisec = timeptr_raw.millitm;			
	return;
}

static void sig_hand( int code )
{
	if( code == SIGALRM )
		return;
	else
		longjmp( exit_env, code );
}

static db_clt_typ *db_trig_set(char *pclient,char *phost,char *pserv,int xport, 
	unsigned *var_list,int trignums)
{
	db_clt_typ *pclt;
	unsigned var;
	int i,err=0;

	if( (pclt = clt_login( pclient, phost, pserv, xport)) == NULL )
	{
	    return( NULL );
	}
	 // set triggers
	for (i=0;i<trignums;i++)
	{
		var = *(var_list+i);
		if (clt_trig_set( pclt, var, var ) == FALSE )
		{
			fprintf(stderr,"%s: unable to set trigger %d\n",pclient,var);
			err++;
		}
	}
	if ( err != 0)
	{
		fprintf(stderr,"%s got %d errors to set triggers.\n",pclient,err);
	    clt_logout( pclt );
	    return( NULL );
	}
	else
	    return( pclt );
}

static void db_trig_unset(db_clt_typ *pclt,unsigned *var_list,int trignums)
{
	unsigned var;
	int i;
	if( pclt != NULL )
    {
		for (i=0;i<trignums;i++)
		{
			var = *(var_list+i);
			clt_trig_unset(pclt,var,var);
		}
	    clt_logout( pclt );
    }
}


