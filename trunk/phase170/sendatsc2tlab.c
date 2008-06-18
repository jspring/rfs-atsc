/*
 * SendATSC2tlab.c
 * This program reads real-time traffic signal status data (atsc),  packs a packet and sends it to tlab via socket
 *  
 * Linux version
 *
 * Last Update: May 28, 2008
 *
 */
#include "sys_os.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/time.h"
#include "signal.h"
#include "local.h"
#include "sys_rt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "db_clt.h"
#include "db_utils.h"
#include "timestamp.h"
#include "atsc_clt_vars.h"
#include "atsc.h"	// actuated traffic signal controller header file 
#include "assem_msg.h"
#include "assem_atsc.h"

#define BUFFER_SIZE 150
#define destAddr "128.32.129.87"
#define destport 13000

static int sig_list[]=
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGALRM,	// for timer
	ERROR
};

static jmp_buf exit_env;

static void sig_hand( int code )
{
	if (code == SIGALRM)
		return;
	else
		longjmp( exit_env, code );
}

static db_clt_typ *database_init(char *pclient, char *phost, char *pserv, int xport, 
	 unsigned *var_list, unsigned *typ_list, int trignums);
static void tsp_done( db_clt_typ *pclt, unsigned *var_list, unsigned *typ_list, int trignums );
 
int main( int argc, char *argv[] )
{
	// database variables
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = DEFAULT_SERVICE;
	db_clt_typ *pclt;
	int  xport = COMM_PSX_XPORT;	// Posix message queus, Linux
	trig_info_typ trig_info;
	int  recv_type,trignums;	
	db_data_typ	db_data;
	unsigned trig_var_list,trig_typ_list;	// ATSC 
	unsigned var = 0,typ = 0;	
	atsc_typ *patsc;		// ATSC info
	// socket variable 
	int sock;
	struct sockaddr_in their_addr;
	struct hostent *he;
	unsigned char buffer[BUFFER_SIZE];
	int lenSendMsg = 0,lenMsgSend;
	int scn_prn = 0;

	//argument inputs
	if (argc != 2)
	{
		fprintf(stderr,"Usage: %s screen_prn_flag\n",argv[0]);
		exit (EXIT_FAILURE);
	}
	scn_prn = atoi(argv[1]);
	// form database trig list (need change)
	trignums = 1;	
	trig_var_list = DB_ATSC_VAR;
	trig_typ_list = DB_ATSC_TYPE;	
	// creat UDP socket 
	if ( ( he = gethostbyname(destAddr)) == NULL) 
	{
		fprintf(stderr,"%s gethostbyname error\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	if ( (sock = socket(PF_INET,SOCK_DGRAM,0)) == -1)
	{
		fprintf(stderr,"%s create socket error\n",argv[0]);
		exit (EXIT_FAILURE);
	}
	// Initial sending address structure 
	their_addr.sin_family = AF_INET;		// host byte order
	their_addr.sin_port = htons(destport);	// short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct
	// Log in to the database (shared global memory) . Default to the the current host.
	get_local_name(hostname, MAXHOSTNAMELEN);
	if( (pclt = database_init(argv[0], hostname, DEFAULT_SERVICE,
			COMM_PSX_XPORT, &trig_var_list, &trig_typ_list, trignums )) == NULL )
	{
		fprintf(stderr,"Database initialization error in %s\n",argv[0]);
		tsp_done( pclt, &trig_var_list, &trig_typ_list, trignums );
		exit( EXIT_FAILURE );
	}
	// exit 
	if( setjmp( exit_env ) != 0 )
	{
		tsp_done( pclt, &trig_var_list, &trig_typ_list, trignums );
		close(sock);
		exit( EXIT_SUCCESS );
	}
	else
		sig_ign( sig_list, sig_hand );

	// main loop 
	for( ;; )
	{
		// Now wait for a trigger. 
		recv_type= clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		var = DB_TRIG_VAR(&trig_info);
		typ = DB_TRIG_TYPE(&trig_info);
		if ( !(var == trig_var_list && typ == trig_typ_list) )
			continue;
		// read database
		if (clt_read(pclt,var,typ,&db_data) == FALSE )
		{
			fprintf(stderr,"%s clt_read ( %d ) failed.\n",argv[0],var);
			continue;
		}
		patsc = (atsc_typ *) db_data.value.user;						
		if (scn_prn == 1)
		{
			fprintf(stderr,"%s recv trigger %d\n",argv[0],var);
		}			
		// form a atsc package
		memset(buffer,0,BUFFER_SIZE);
		lenSendMsg = assemATSC(buffer,patsc);
		if ( (lenMsgSend = sendto(sock,buffer,lenSendMsg,0,
				(struct sockaddr *)&their_addr,sizeof(struct sockaddr)) ) == -1)
		{
			fprintf(stderr,"%s sendto error\n",argv[0]);
		}
		else if (scn_prn == 1)
		{
			fprintf(stderr,"%s sent msg at %02d:%02d:%02d.%03d\n",
				argv[0],patsc->ts.hour,patsc->ts.min,patsc->ts.sec,
				patsc->ts.millisec);
		}		
	}
}

static db_clt_typ *database_init(char *pclient, char *phost, char *pserv, int xport, 
	 unsigned *var_list, unsigned *typ_list, int trignums)
{
	db_clt_typ *pclt;
	unsigned var,typ;
	int i,err=0;
	if( (pclt = clt_login( pclient, phost, pserv, xport)) == NULL )
	{
	    return( NULL );
	}
	// Set triggers 		
	for (i=0;i<trignums;i++)
	{
		var = *(var_list+i);
		typ = *(typ_list+i);
		if (clt_trig_set( pclt, var, typ ) == FALSE )
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

static void tsp_done( db_clt_typ *pclt, unsigned *var_list, unsigned *typ_list, int trignums)
{
	unsigned var,typ;	
	int i;
	if( pclt != NULL )
    {
		for (i=0;i<trignums;i++)
		{
			var = *(var_list+i);
			typ = *(typ_list+i);
			clt_trig_unset( pclt, var, typ );
		}

		clt_logout( pclt );
    }
}
