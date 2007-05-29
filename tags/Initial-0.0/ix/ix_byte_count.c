/*\file
 *
 *	Calculates offsets within message, based on flattening
 *	structures into buffer as done in ix_msg_format, for an
 *	intersection with number of approaches and stop lines
 *	specified on the command line. This program for simplicity
 *	assumes that all approaches have the same number of stop
 *	lines. Writes info to stdout.	
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */ 

#include "ix_msg.h"	// intersection message set header file
#include "udp_utils.h"

jmp_buf exit_env;
static int sig_list[] = 
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	ERROR,
};
static void sig_hand(int code);
void sig_ign(int *sig_list, void sig_hand(int sig));


void do_usage(char *progname)
{
	fprintf(stderr, "Usage %s:\n", progname);
	fprintf(stderr, "-a number of approaches ");
	fprintf(stderr, "-s number fo stop lines ");
	fprintf(stderr, "\n");
	exit(1);
}
				 
int main (int argc, char **argv)
{
	int option;		/// for getopt
	unsigned char buf[MAX_IX_MSG_PKT_SIZE];	/// input buffer 
	int i;			/// local counter for for loops
	ix_msg_t *pix;		/// Pointer, malloced
	int num_approach = 8;
	int num_stop = 1;
	ix_approach_t *pappr;
	ix_stop_line_t *pstop;

        while ((option = getopt(argc, argv, "a:s:")) != EOF) {
                switch( option ) {
                        case 'a':
                                num_approach = atoi(optarg);
				break;
                        case 's':
                                num_stop = atoi(optarg);
				break;
                        default:
				do_usage(argv[0]);
                                break;
                }
        }

	if (setjmp(exit_env) != 0) {
		exit(EXIT_SUCCESS);
	} else 
		sig_ign(sig_list, sig_hand);

	// initialize structures of the correct size
	// no actual data, except for pointers and size fields
	pix = (ix_msg_t *) malloc(sizeof(ix_msg_t));
	pappr = (ix_approach_t * )malloc(sizeof(ix_approach_t) * num_approach);
	pix->num_approaches = num_approach;
	pix->approach_array = pappr;
	for (i = 0; i < num_approach; i++) {
		pstop = (ix_stop_line_t *)
			 malloc(sizeof(ix_stop_line_t) * num_stop);  
		pix->approach_array[i].stop_line_array = pstop;
		pix->approach_array[i].number_of_stop_lines = num_stop;
	}
	// called with do_print options set to 1
	ix_msg_format(pix, buf, MAX_IX_MSG_PKT_SIZE, 1);	
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

