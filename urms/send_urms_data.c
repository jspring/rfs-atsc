/* send_urms.c - Controls URMS running on a 2070 via ethernet
**
*/

#include "urms.h"
#include "tos.h"

#define BUF_SIZE	sizeof(gen_mess_t)

static jmp_buf exit_env;
static int sig_list[] = {
        SIGINT,
        SIGQUIT,
        SIGTERM,
        SIGALRM,
        ERROR,
};
static void sig_hand(int code)
{
        if (code == SIGALRM)
                return;
        else
                longjmp(exit_env, code);
}

const char *usage = "-v (verbose) -r <controller IP address (def. 192.168.1.126)> -p <port (def. 10011)> -s (standalone, no DB) -l (print log)\n\nFor standalone testing:\n\t-1 <lane 1 release rate (VPH)>\n\t-2 <lane 1 action>\n\t-3 <lane 1 plan>\n\t-4 <lane 2 release rate (VPH)>\n\t-5 <lane 2 action>\n\t-6 <lane 2 plan>\n\t";

db_id_t db_vars_list[] =  {
        {DB_URMS_STATUS_VAR, sizeof(db_urms_status_t)}
};
int num_db_variables = sizeof(db_vars_list)/sizeof(db_id_t);

int db_trig_list[] =  {
        DB_URMS_STATUS_VAR
};

int num_trig_variables = sizeof(db_trig_list)/sizeof(int);

static int OpenSOBUConnection(char *SOBUIP, char *port);

int main(int argc, char *argv[]) {
	int urmsfd;
	gen_mess_t gen_mess;
	char *SOBUIP = "10.0.1.7";
	char *port = "4444";

        int option;

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        unsigned int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h
        posix_timer_typ *ptimer;
        trig_info_typ trig_info;
	db_urms_status_t db_urms_status;
	db_urms_t db_urms;
	int loop_interval = 5000; 	// Loop interval, ms
	int verbose = 0;
	int i;
	int get_status_err = 0;
	char readbuff[10];
	int nread = 0;
	fd_set readfds;
	int selectval = 1000;
	struct timeval timeout;
	char *inportisset = "not yet initialized";
	char *buf = &db_urms_status;

        while ((option = getopt(argc, argv, "r:vp:si:1:2:3:4:5:6:7:8:9:A:B:C:D:")) != EOF) {
                switch(option) {
                case 'r':
			SOBUIP = strdup(optarg);
                        break;
                case 'p':
                        port = strdup(optarg);
                        break;
                case 'v':
                        verbose = 1;
                        break;
                case 'i':
                        loop_interval = atoi(optarg);
                        break;
                default:
                        printf("Usage: %s %s\n", argv[0], usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

	// Open connection to URMS controller
	urmsfd = OpenSOBUConnection(SOBUIP, port);
	if(urmsfd < 0) {
		fprintf(stderr, "Could not open connection to SOBU\n");
		exit(EXIT_FAILURE);
	}

	// Connect to database
        get_local_name(hostname, MAXHOSTNAMELEN);
        if ( (pclt = db_list_init(argv[0], hostname, domain,
            xport, NULL, 0, db_trig_list,
            num_trig_variables)) == NULL) {
            exit(EXIT_FAILURE);
	}

        if (setjmp(exit_env) != 0) {
                close(urmsfd);
                db_list_done(pclt, NULL, 0, db_trig_list, num_trig_variables);
		printf("get_status_err %d\n", get_status_err);
                exit(EXIT_SUCCESS);
        } else
               sig_ign(sig_list, sig_hand);

        if ((ptimer = timer_init( loop_interval, ChannelCreate(0) )) == NULL) {
                fprintf(stderr, "Unable to initialize delay timer\n");
                exit(EXIT_FAILURE);
        }
	
	while(1) {
                // Now wait for a trigger. 
                // Data or timer, it doesn't matter - we read and write on both
		// BTW, if you're using clt_ipc_receive, you just
		// need to do the timer_init. Do NOT also use 
		// TIMER_WAIT; it'll add another timer interval to
		// the loop.

		clt_ipc_receive(pclt, &trig_info, sizeof(trig_info));
		if( DB_TRIG_VAR(&trig_info) == DB_URMS_STATUS_VAR ) {
			db_clt_read(pclt, DB_URMS_STATUS_VAR, sizeof(db_urms_status_t), &db_urms_status);
			printf("Got DB_URMS_STATUS_VAR trigger sizeof(db_urms_status_t) %d\n", sizeof(db_urms_status_t));
			printf("%#hhx %#hhx \n", db_urms_status.metered_lane_stat[0].metered_lane_rate_msb, db_urms_status.metered_lane_stat[0].metered_lane_rate_lsb);
			printf("%#hhx %#hhx \n", db_urms_status.queue_stat[0].occ_msb, db_urms_status.queue_stat[0].occ_lsb);
			write(urmsfd, &db_urms_status, sizeof(db_urms_status_t));
		}
		else {
			printf("Got another trigger urmsfd %d\n", urmsfd);
			if(urmsfd > 0) {
				FD_ZERO(&readfds);
				FD_SET(urmsfd, &readfds);
				timeout.tv_sec = 0;
				timeout.tv_usec = 1000;
				if( (selectval = select(urmsfd+1, &readfds, NULL, NULL, &timeout)) < 0) {
					if(errno != EINTR) {
						perror("select 1");
						inportisset = (FD_ISSET(urmsfd, &readfds)) == 0 ? "no" : "yes";
						printf("\n\nser_driver_read 1: urmsfd %d selectval %d inportisset %s\n\n", urmsfd, selectval, inportisset);
					}
				}
				if(selectval > 0) {
					printf("Got data from ac %d\n", urmsfd);
					nread = read(urmsfd, &db_urms, sizeof(db_urms_t));
					if(nread > 0) {
						db_clt_write(pclt, DB_URMS_VAR, sizeof(db_urms_t), &db_urms);
						printf("nread %d\n", nread);
						for(i = 0; i<nread; i++)
							printf("%#hhx ", readbuff[i]);
					}
					if(nread == 0) {
						if(urmsfd > 0)
							close(urmsfd);

						// Reopen connection to URMS controller (just the first time we lose connection;
						// after that, the file descriptor is less than zero)
						urmsfd = OpenSOBUConnection(SOBUIP, port);
						printf("1: urmsfd %d\n", urmsfd);
						if(urmsfd < 0) {
							fprintf(stderr, "1: Could not open connection to SOBU\n");
						}
					}
				}
			}
			else {
				// Reopen connection to URMS controller
				printf("2.1: urmsfd %d\n", urmsfd);
				urmsfd = OpenSOBUConnection(SOBUIP, port);
				printf("2.2: urmsfd %d\n", urmsfd);
				if(urmsfd < 0) {
					fprintf(stderr, "2: Could not open connection to SOBU\n");
				}
			}
		}
	}
}

static int OpenSOBUConnection(char *SOBUIP, char *port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;	   /* Any protocol */
	s = getaddrinfo(SOBUIP, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	Try each address until we successfully connect(2).
	If socket(2) (or connect(2)) fails, we (close the socket
	and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			perror("socket");
			continue;
		}
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;		    /* Success */
		}
		perror("connect");
		close(sfd);
	}
	if (rp == NULL) {		 /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		return -1;
	}
	freeaddrinfo(result);	    /* No longer needed */
	return sfd;
}
