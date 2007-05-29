/**\file
 *	Updates ATSC variable after reading USB digio. 
 *
 * (C) Copyright University of California 2006.  All rights reserved.
 *
 */

#include	<sys_os.h>
#include	"sys_rt.h"
#include	"timestamp.h"
#include	"db_clt.h"
#include	"db_utils.h"
#include	"clt_vars.h"
#include	"atsc.h"

#define DEBUG_DBV
//#define NO_HARDWARE

extern int write_atsc_db_var(db_clt_typ *pclt, unsigned char digio_byte);

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

static db_id_t db_vars_list[] = {
	{DB_ATSC_VAR, sizeof(atsc_typ)}};

#define NUM_DB_VARS sizeof(db_vars_list)/sizeof(db_id_t)

int main (int argc, char** argv)
{
	char *domain = DEFAULT_SERVICE;
	char hostname[MAXHOSTNAMELEN];
	db_clt_typ *pclt;
	int opt;
	int xport = COMM_PSX_XPORT;
	posix_timer_typ *ptmr;
	int interval = 1000;	// number of milliseconds between calls
	int counter = 0;
	while ((opt = getopt(argc, argv, "d:i:x:")) != -1) {
		switch (opt) {
		  case 'd':
			domain = strdup(optarg);
			break;
		  case 'i':
			interval = atoi(optarg);
			break;
		  case 'x':
			xport = atoi(optarg);	
			break;
		  default:
			printf( "Usage %s: clt_update -d [domain] ", argv[0]);
			printf("-x [xport]]\n");
			exit(1);
		}
	}
	get_local_name(hostname, MAXHOSTNAMELEN+1);
	if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			       db_vars_list, NUM_DB_VARS, NULL, 0))
			       == NULL ) {
		   printf("Database initialization error in %s\n", argv[0]);
		   exit( EXIT_FAILURE );
	}
	if ((ptmr = timer_init(interval, 0)) == NULL) {
		printf("timer_init failed\n");
		exit(EXIT_FAILURE);
	}

#ifdef NO_HARDWARE
	if ((ptmr = timer_init(1000, 0)) == NULL) {
		printf("timer_init failed\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* Catch the signals SIGINT, SIGQUIT, and SIGTERM.  If signal occurs,
	 * log out from database and exit. */
	if( setjmp( exit_env ) != 0 ) {
		/* Log out from the database. */
		if (pclt != NULL)
			clt_destroy( pclt, DB_ATSC_VAR, DB_ATSC_VAR); 
			clt_logout( pclt );
		exit( EXIT_SUCCESS );
	} else
		sig_ign( sig_list, sig_hand );

	init_usb_digio();

	while (1) {
		unsigned char status;
		status = read_digio_byte();
#ifdef NO_HARDWARE
		if (counter >=  0) status = 0x01;
		if (counter >= 20) status = 0x04;
		if (counter >= 24) status = 0x00;
                if (counter >= 26) status = 0x02;
                if (counter >= 54) status = 0x80;
		if (counter >= 58) status = 0x00;
		if (counter >= 60) counter = -1;
                counter++;
		printf("fake status: 0x%2hhx\n", status);
#endif
//		printf("dio: status 0x%02hhx\n", status);
		write_atsc_db_var(pclt, status);
		TIMER_WAIT(ptmr);
	}
}
