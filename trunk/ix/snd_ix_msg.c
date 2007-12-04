/*\file
 *	
 *	Send intersection message.
 *
 *	Input normally comes from reading the "data bucket".
 *	In normal use at PATH, this is a server for interprocess
 *	communication using a publish/subscribe database.
 *	The required real-time data for the intersection message  
 *	may be produced in an RSU by processes reading a sniffer
 *	circuit, and tracking it using information from the phase
 *	timing plan, or from NTCIP or AB348 processes. Any of these
 *	methods for acquiring data will write the data to the
 *	"data bucket" server in the same format to be read by this
 *	process.
 *
 *	For testing of OBU code outside of PATH, this code can be
 *	compiled in TEST_ONLY mode to read a trace file of the
 *	intersection message information instead of real-time
 *	information from the data bucket. If you are using this
 *	code outside of PATH, the Makefile has been configured
 *	for TEST_ONLY, and this program will automatically read
 *	from the file "snd_test_phases.in".
 *
 *	Copyright (c) 2006 Regents of the University of California
 *
 */ 

/*
 * Useless comment by LK to test uploading 070718
 *
 */

#include "ix_msg.h" 
#include "ix_db_utils.h"
#include "udp_utils.h"
#include "../../tsp/src/veh_sig.h"
#include "../../tsp/src/clt_vars.h"

static int sig_list[]=
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	SIGALRM,	// for timer
	ERROR
};

static jmp_buf exit_env;
static void sig_hand(int code);
void sig_ign(int *sig_list, void sig_hand(int sig));

void do_usage(char *progname)
{
	fprintf(stderr, "Usage %s:\n", progname);
	fprintf(stderr, "-i send interval (milliseconds)");
	fprintf(stderr, "-n no broadcast (default is broadcast)");
	fprintf(stderr, "-o output destination IP address ");
	fprintf(stderr, "-p port for send ");
	fprintf(stderr, "-v verbose ");
	fprintf(stderr, "\n");
	exit(1);
}

int main (int argc, char **argv)
{
	int option;		/// for getopt
	int fdout;		/// output file descriptor
	char *foutname = "192.168.255.255";	/// default output address
	int no_broadcast = 0;	/// by default, send to broadcast address
	int port_out = 6053;	/// Default destination UDP port 
	struct sockaddr_in snd_addr;	/// used by sendto
	int bytes_sent;		/// returned from sendto
	int verbose = 0;	/// extra output to stdout 
	posix_timer_typ *ptmr;
	char * domain = DEFAULT_SERVICE;
#ifdef TEST_ONLY
	int xport = 0;		/// read file, data bucket not active 
#else
	int xport = COMM_PSX_XPORT;	/// transport mechanism for data bucket
#endif
	db_clt_typ *pclt;	/// data bucket client pointer
	unsigned char send_buf[MAX_IX_MSG_PKT_SIZE];
	char hostname[MAXHOSTNAMELEN+1];
	int interval = 100;
	int getbusdisplay=0;	// put Transit Signal Priority info in header

        while ((option = getopt(argc, argv, "gi:no:p:v")) != EOF) {
                switch(option) {
                        case 'g':
                                getbusdisplay = 1;
				break;
                        case 'i':
                                interval = atoi(optarg);
				break;
                        case 'n':
                                no_broadcast = 1;
                                break;
                        case 'o':
                                foutname = strdup(optarg);
                                break;
                        case 'p':
                                port_out = atoi(optarg);
				break;
                        case 'v':
                                verbose = 1;
				break;
                        default:
				do_usage(argv[0]);
                                break;
                }
        }
	printf("Sending to %s, max packet size %d\n",
		foutname, MAX_IX_MSG_PKT_SIZE);

	get_local_name(hostname, MAXHOSTNAMELEN+1);

	// only reads database variables, so does not do create
	// process that writes the variable must be started first
	if ((pclt = clt_login(argv[0], hostname, domain, xport))
			       == NULL) {
		   printf("Database initialization error in %s\n", argv[0]);
		   exit(EXIT_FAILURE);
	}

        if (setjmp(exit_env) != 0) {
		if (pclt != NULL) 
			clt_logout(pclt);
                exit(EXIT_SUCCESS);
        } else
                sig_ign(sig_list, sig_hand);

	if ((ptmr = timer_init(interval, 0)) == NULL) {
		printf("timer_init failed\n");
		exit(EXIT_FAILURE);
	}

	if (no_broadcast)	// may want to set when testing
		fdout = udp_unicast();
	else
		fdout = udp_broadcast();

	if (fdout < 0) 
		do_usage(argv[0]);

	set_inet_addr((struct sockaddr_in *) &snd_addr,
			 inet_addr(foutname), port_out);		
	
	while (1) {
		ix_msg_t *pmsg = (ix_msg_t *) malloc(sizeof(ix_msg_t));
		int bytes_to_send;
		to_disp_typ to_disp;
		ix_msg_read(pclt, pmsg, DB_IX_MSG_VAR, DB_IX_APPROACH1_VAR); 
		if (verbose)
			ix_msg_print(pmsg);
		if (getbusdisplay) {
			short *pshort;
			db_clt_read(pclt, DB_TO_DISP_VAR, sizeof(to_disp_typ),
					&to_disp);
			pmsg->preempt_calls = to_disp.showT2G;
			pmsg->bus_priority_calls = to_disp.signalFace_bf;
			pmsg->preempt_state = to_disp.signalFace_af;
			pmsg->special_alarm = to_disp.showTimeSave;
			pshort = (short *) &pmsg->reserved[0];
			*pshort = to_disp.T2G;			
			pshort = (short *) &pmsg->reserved[2];
			*pshort = to_disp.busTimeSave;			
		}
		bytes_to_send = ix_msg_format(pmsg, send_buf,
					MAX_IX_MSG_PKT_SIZE, 0);

		if (bytes_to_send < 0) {
			printf("%s: Message formatting error \n", argv[0]);
			continue;
		}			

		bytes_sent = sendto(fdout, send_buf, bytes_to_send , 0,
				 (struct sockaddr *) &snd_addr,
				 sizeof(snd_addr));

		if (bytes_sent != bytes_to_send) {
			printf("%s: bytes to send %d bytes sent %d\n", argv[0],
					bytes_to_send, bytes_sent);
		}
		TIMER_WAIT(ptmr);
		ix_msg_free(&pmsg);
	}
	fprintf(stderr, "%s exiting on error\n", argv[0]);
}

static void sig_hand(int code)
{
	longjmp(exit_env, code);
}

#ifdef TEST_ONLY
void sig_ign(int *sig_list, void sig_hand(int sig))
{
        int i = 0;
        while (sig_list[i] != ERROR) {
                signal(sig_list[i], sig_hand);
                i++;
        }
}
#endif
