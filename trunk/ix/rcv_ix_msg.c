/*\file
 *	Receive an intersection set message and optionally write it
 *	to the data bucket or to a trace file.
 *
 *	When compiled in TEST_ONLY mode (as distributed to researchers 
 *	outside PATH who will be using OBUs only, not RSUs), the trace file
 *	will be written automatically to "rcv_ix_msg.out", since the data
 *	bucket code is replaced with file writing code. When compiled
 *	in an RSU environment with the data bucket code, use of the
 *	data bucket can still be turned off (since receive code does
 *	not need it for operation in isolation) and either verbose
 *	output written to stdout, or a trace file specified on the
 *	command line. The trace file will be in the correct format
 *	to use as input snd_ix_msg in TEST_ONLY mode. Note that if
 *	-n (to turn off database) is specified without specifying either
 *	-v (for verbose) or a filename with -f, rcv_ix_msg has no
 *	output. 
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */ 

#include "ix_msg.h"	// intersection message set header file
#include "timestamp.h"

// Normal operation in an RSU will interface with digital I/O and
// phase tracking processes using the PATH data bucket code.
// To test message send and receive off-line, file reading code
// is used instead; see Makefile to build test_rcv_ix_msg.

#include "ix_db_utils.h"

#include "udp_utils.h"

#undef DO_TRACE

jmp_buf exit_env;
static int sig_list[] = 
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGPIPE,
	ERROR,
};
static void sig_hand(int code);
void sig_ign(int *sig_list, void sig_hand(int sig));


static db_id_t db_vars_list[] = {
	{DB_RX_IX_MSG_VAR, sizeof(ix_msg_t)},
	{DB_RX_IX_APPROACH1_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH2_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH3_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH4_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH5_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH6_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH7_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH8_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH9_VAR, MAX_APPROACH_SIZE},
	{DB_RX_IX_APPROACH10_VAR, MAX_APPROACH_SIZE},
};

#define NUM_DB_VARS (sizeof(db_vars_list)/sizeof(db_id_t))

void do_usage(char *progname)
{
	fprintf(stderr, "Usage %s:\n", progname);
	fprintf(stderr, "-c print phases/countdown to stdout \n");
	fprintf(stderr, "-f specify filename to create a trace file \n");
	fprintf(stderr, "-n don't use database \n");
	fprintf(stderr, "-p port for input \n");
	fprintf(stderr, "-v verbose \n");
	fprintf(stderr, "\n");
	exit(1);
}
				 
int main (int argc, char **argv)
{
	int option;		/// for getopt
	int fdin;		/// input file or socket descriptor
	int port_in = 6053;	/// port number to bind to 
	unsigned char buf[MAX_IX_MSG_PKT_SIZE];	/// input buffer 
	struct sockaddr_in src_addr;	/// filled in by recvfrom
	int bytes_received;	/// returned from recvfrom
	int verbose = 0;	/// echo entire message  to stdout
	int check_phases = 0;	/// print phase/countdown summary to stdout
	int check_tsp = 0;	/// print TSP info from header 
	int count = 0;		/// character count for formatting
	int msg_count = 0;	/// incremented with each receive
	int i;			/// local counter for for loops
	unsigned int socklen;	/// holds sockaddr_in size for sendto call
	ix_msg_t *pix;		/// Pointer, malloced and freed for each
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = DEFAULT_SERVICE;	/// Set to use database
	db_clt_typ *pclt = NULL;	/// Data bucket client pointer

	/** in TEST_ONLY, with use_db, file rcv_test_phases.out
	 *  is automatically created
	 *  if not compiled as TEST_ONLY, must have db_slv running
	 *  or set use_db to 0 with the -n flag.
	 */ 
	int use_db = 1;		
	int do_trace = 0;	/// if 1, create a trace file
	char *foutname = NULL;  /// must set trace file name on command line
	FILE *fp;		/// trace file pointer

#ifdef TEST_ONLY
	int xport = 1;			/// Write the file
#else
	/// Normal setting to use data bucket on Linux
	int xport = COMM_PSX_XPORT;	/// Change if porting to another OS
#endif

        while ((option = getopt(argc, argv, "cd:f:gnp:v")) != EOF) {
                switch( option ) {
			case 'c':
				check_phases = 1; 
				break;
			case 'd':
				domain = strdup(optarg);
				break;
			case 'f':
				foutname = strdup(optarg);
				do_trace = 1;
				break;
			case 'g':
				check_tsp = 1; 
				break;
                        case 'n':
                                use_db = 0;
				break;
                        case 'p':
                                port_in = atoi(optarg);
				break;
                        case 'v':
                                verbose = 1;
				break;
                        default:
				do_usage(argv[0]);
                                break;
                }
        }

	fprintf(stderr, "Receive Port: %d\n", port_in);
	fdin = udp_allow_all(port_in);

	if (fdin < 0) 
		do_usage(argv[0]);

	if (do_trace) {
		fp = fopen(foutname, "w"); 	
		if (fp == NULL) {
			printf("Could not open %s for writing\n", foutname);
			do_usage(argv[0]);
		}
	}

        /* Log in to the database (shared global memory).  Default to the
	 * the current host. */
	get_local_name(hostname, MAXHOSTNAMELEN);
	if (use_db) {
		if (( pclt = db_list_init( argv[0], hostname, domain, xport,
			db_vars_list, NUM_DB_VARS, NULL, 0)) == NULL ) {
			printf("%s: Database initialization error \n", argv[0]);
			exit( EXIT_FAILURE );
		}
	}

	if (setjmp(exit_env) != 0) {
		if (use_db) {
			if (pclt != NULL) {
				db_list_done(pclt, db_vars_list, 
					NUM_DB_VARS, NULL, 0);
			}
		}
		printf("%s exited, received %d messages\n", argv[0], msg_count);
		exit(EXIT_SUCCESS);
	} else 
		sig_ign(sig_list, sig_hand);

	socklen = sizeof(src_addr);
	memset(&src_addr, 0, socklen);

	while (1) {
		count = 0;	// for formatting output
		bytes_received = recvfrom(fdin, buf,
				 MAX_IX_MSG_PKT_SIZE, 0,
				(struct sockaddr *) &src_addr,
				 &socklen);

		if (bytes_received  < 0) {
			perror("recvfrom failed\n");
			continue;
		}
		msg_count++;
		ix_msg_extract(buf, &pix);  // places message in structure
		if (use_db) {
			ix_msg_update(pclt, pix, DB_RX_IX_MSG_VAR,
					DB_RX_IX_APPROACH1_VAR);
		}
		if (check_phases) {
			struct tm tmval;
			time_t time_t_secs = pix->seconds;

			localtime_r(&time_t_secs, &tmval);	
			printf("%2d:%02d:%02d.%03d: ", tmval.tm_hour,
				tmval.tm_min, tmval.tm_sec, 
				pix->nanosecs/1000000);
			for (i = 0; i < pix->num_approaches; i++) {
				ix_approach_t *pappr = &pix->approach_array[i];
				printf("%d:%2s:%5.1f ", i+1,
				   ix_signal_state_string(pappr->signal_state),
				   pappr->time_to_next/10.0);
			}
			printf("\n");
		}
		if (check_tsp) {
			timestamp_t ts;
			short int *pshort;
			get_current_timestamp(&ts);
			print_timestamp(stdout, &ts);
			printf(" showT2G %c sig_bf %c sig_af %c showTS %c ",
				pix->preempt_calls,
				pix->bus_priority_calls,
				pix->preempt_state,
				pix->special_alarm);
			pshort = (short *) &pix->reserved[0];
			printf(" T2G %hd ", pshort);
			pshort = (short *) &pix->reserved[2];
			printf(" TS %hd ", pshort);
		}
		if (do_trace)
			ix_msg_update_file(fp, pix);
		if (verbose)
			ix_msg_print(pix);	
		ix_msg_free(&pix);    // frees structure and internal pointers
	}
	fprintf(stderr, "%s exiting on error\n", argv[0]);
}

static void sig_hand(int code)
{
	longjmp(exit_env, code);
}

void sig_ign(int *sig_list, void sig_hand(int sig))
{
        int i = 0;
        while (sig_list[i] != ERROR) {
                signal(sig_list[i], sig_hand);
                i++;
        }
}

