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
#include	"atsc_clt_vars.h"
#include	"atsc.h"
#include	"nema_asg.h"

#undef DEBUG_DBV
#undef NO_HARDWARE

#define TEST_DIR	"/home/atsc/test/"

//variable that stores number of bits to be converted and written to atsc_t
int num_bits_to_nema;

//array that stores conversion information
//assume sniffing 2 colors on 8 approaches max
bit_to_nema_phase_t nema_asg[16];	

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
	FILE *fp_nema;
	char *site = "rfs";
	char filename_nema[80];
	char lineinput[512];
	posix_timer_typ *ptmr;
	db_clt_typ *pclt;
	int opt;
	int xport = COMM_PSX_XPORT;
	int interval = 1000;	// number of milliseconds between calls
	int counter = 0;
	int i, line = 1;
	int no_db = 0;
	int verbose = 0;

	while ((opt = getopt(argc, argv, "d:s:i:x:nv")) != -1) {
		switch (opt) {
		  case 'd':
			domain = strdup(optarg);
			break;
		  case 'n':
			no_db = 1;
			break;
		  case 's':
			site = strdup(optarg);
			break;
		  case 'i':
			interval = atoi(optarg);
			break;
		  case 'v':
			verbose = 1;
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
	if (!no_db) {
		if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			       db_vars_list, NUM_DB_VARS, NULL, 0))
			       == NULL ) {
		   printf("Database initialization error in %s\n", argv[0]);
		   exit( EXIT_FAILURE );
		}
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

        /*******************************/
        /* Read the configuration file */
        /*******************************/

        /* Specify the whole path */
        strcpy( filename_nema, TEST_DIR);
        strcat( filename_nema, site);
        strcat( filename_nema, ".nema.cfg");

        /* Open the NEMA configuration file for that site */
        fp_nema = fopen( filename_nema, "r");
        if( fp_nema == NULL )
        {
                printf("Could not open %s\n", filename_nema);
                exit( EXIT_FAILURE );
        }

        /* Read the NEMA configuration file */
        printf("Reading %s\n", filename_nema);
        fflush(stdout);
        while( fgets( lineinput, 100, fp_nema) != NULL )
        {
                if( strncmp(lineinput,"#",1) == 0 )
                        continue;
                /* Scan the file without the # */
                switch (line)
		{
                case 1: sscanf(lineinput, "%d", &num_bits_to_nema);
			i = 1;
                        break;
		default:
			sscanf(lineinput, "%c %d",
				&nema_asg[i-1].phase_color, 
				&nema_asg[i-1].phase_number);	
			i++;
			break;
		}
		if( i > num_bits_to_nema )	// nothing left to read
			break;
		line += 1;
	}
	fclose( fp_nema );

 	printf("Number of bits: %d\n", num_bits_to_nema);
        for (i = 1; i <= num_bits_to_nema; i++) {
                printf("NEMA phase color: %c phase number: %d\n",
                        nema_asg[i-1].phase_color, 
                        nema_asg[i-1].phase_number); 
	}
	fflush(stdout);

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
		if (verbose)
			printf("dio: status 0x%02hhx\n", status);
		if (!no_db)
			write_atsc_db_var(pclt, status);
		TIMER_WAIT(ptmr);
	}
}
