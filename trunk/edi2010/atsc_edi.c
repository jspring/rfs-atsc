/**\file
 *	Updates ATSC variable after reading Eberle conflict monitor.
 *
 * (C) Copyright University of California 2008.  All rights reserved.
 *
 */

#include	<sys_os.h>
#include	"sys_rt.h"
#include	"timestamp.h"
#include	"db_clt.h"
#include	"db_utils.h"
#include	"atsc_clt_vars.h"
#include	"atsc.h"
#include	<udp_utils.h>

extern int write_atsc_db_var(db_clt_typ *pclt, unsigned char *buf, unsigned char verbos);
extern int print_log(unsigned char *);


#define TRACE           1
#define USE_DATABASE    2
#define MYSQL           4
int output_mask = 0;    // 1 trace, 2 DB server, 4 MySQL


int newsockfd;
struct sockaddr_storage edibcast;
db_clt_typ *pclt = NULL;
#define BUFSIZE 50

static int sig_list[]=
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	ERROR
};

static jmp_buf exit_env;
static void sig_hand( int code )
{
    longjmp( exit_env, code );
}

static db_id_t db_vars_list[] = {
	{DB_ATSC_VAR, sizeof(atsc_typ)}};

#define NUM_DB_VARS sizeof(db_vars_list)/sizeof(db_id_t)

int main (int argc, char** argv)
{
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = DEFAULT_SERVICE;
	int xport = COMM_PSX_XPORT;
	int opt;
	int verbose = 0;
	socklen_t msglen;
	int retval;
	unsigned char buf[BUFSIZE];
	int error = 0;
    	int tcp_input = 1;     // 1 TCP socket, 0 file or stdin redirect
	unsigned char debug = 0;

	while ((opt = getopt(argc, argv, "x:D:o:fvd")) != -1) {
		switch (opt) {
	  case 'D':
		domain = strdup(optarg);
		break;
	  case 'v':
		verbose = 1;
		break;
	  case 'x':
		xport = atoi(optarg);	
		break;
          case 'd' :
            	debug = 1;
        	break;
          case 'o':
            	output_mask = atoi(optarg);
            	break;
          case 'f' :
            	tcp_input = 0;
            	break;
         default:
            printf("Usage: %s -d (debug)\n\t\t    -v (verbose)\n\t\t    -o (output mask, 1=trace file 2=use DB 4=MySQL)\n\t\t    -f < \"input file\" >\n", argv[0]);
            exit(EXIT_FAILURE);
		}
	}

    if(tcp_input){
        newsockfd = udp_allow_all(6060);
        if( newsockfd < 0) {
            printf("udp_allow_all failed. Exiting...\n");
            exit(EXIT_FAILURE);
            }
        }
    else
        newsockfd = STDIN_FILENO;

    if (output_mask & USE_DATABASE) {
        get_local_name(hostname, MAXHOSTNAMELEN);
        if ((pclt = db_list_init(argv[0], hostname, domain, xport, db_vars_list, NUM_DB_VARS, NULL, 0)) == NULL) {
                printf("Database initialization error in %s\n", argv[0] );
                exit(EXIT_FAILURE);
            }
        }

	/* Catch the signals SIGINT, SIGQUIT, and SIGTERM.  If signal occurs,
	 * log out from database and exit. */
	if( setjmp( exit_env ) != 0 ) {
		close(newsockfd);
		/* Log out from the database. */
		if (pclt != NULL)
			clt_destroy( pclt, DB_ATSC_VAR, DB_ATSC_VAR); 
			clt_logout( pclt );
		exit( EXIT_SUCCESS );
	} else
		sig_ign( sig_list, sig_hand );

    msglen = sizeof(edibcast);
    memset(&edibcast, 0, msglen);
			
	while (1) {
    	if( (retval=recvfrom( newsockfd, buf, BUFSIZE, 0, (struct sockaddr *)&edibcast, &msglen) ) < 0) {
             perror("recv");
             printf("RECV ERROR: %d\n", retval);
             error++;
             continue;
        }
        if(debug != 0)
            print_log(buf);

    	if (output_mask & USE_DATABASE)
	    write_atsc_db_var(pclt, buf, verbose);
	}
}
