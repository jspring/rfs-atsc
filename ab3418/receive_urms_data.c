/* urms.c - Controls URMS running on a 2070 via ethernet
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
        {DB_URMS_VAR, sizeof(db_urms_t)},
        {DB_URMS_STATUS_VAR, sizeof(db_urms_status_t)}
};
int num_db_variables = sizeof(db_vars_list)/sizeof(db_id_t);

unsigned int db_trig_list[] =  {
        DB_URMS_VAR,
        DB_URMS_STATUS_VAR
};
int num_trig_variables = sizeof(db_trig_list)/sizeof(int);

static int OpenSOBUListener(unsigned short serv_port);

int main(int argc, char *argv[]) {
	int urmsfd;
	gen_mess_t gen_mess;
	unsigned short port = 4466;

        int option;

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        unsigned int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h
        posix_timer_typ *ptimer;
        trig_info_typ trig_info;
	db_urms_status_t db_urms_status;
	db_urms_t db_urms;
	int loop_interval = 500; 	// Loop interval, ms
	int verbose = 0;
	int i;
	int get_status_err = 0;
	char *readbuff = (char *)&db_urms_status;
	int nread = 0;
	fd_set readfds;
	int selectval = 1000;
	struct timeval timeout;
	char *inportisset = "not yet initialized";

        while ((option = getopt(argc, argv, "vp:i:")) != EOF) {
                switch(option) {
                case 'p':
                        port = (unsigned short)atoi(optarg);
                        break;
                case 'i':
                        loop_interval = (unsigned short)atoi(optarg);
                        break;
                case 'v':
                        verbose = 1;
                        break;
                default:
                        printf("Usage: %s %s\n", argv[0], usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

	// Open connection to URMS controller
	urmsfd = OpenSOBUListener(port);
	if(urmsfd < 0) {
		fprintf(stderr, "Could not open connection to SOBU\n");
		exit(EXIT_FAILURE);
	}

	// Connect to database
        get_local_name(hostname, MAXHOSTNAMELEN);
        if ( (pclt = db_list_init(argv[0], hostname, domain,
            xport, db_vars_list, num_db_variables, db_trig_list,
            num_trig_variables)) == NULL) {
            exit(EXIT_FAILURE);
	}

        if (setjmp(exit_env) != 0) {
                close(urmsfd);
                db_list_done(pclt, db_vars_list, num_db_variables , db_trig_list, num_trig_variables);
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
		db_clt_read(pclt, DB_URMS_VAR, sizeof(db_urms_t), &db_urms);
		if( DB_TRIG_VAR(&trig_info) == DB_URMS_VAR ) {
			printf("Got DB_URMS_VAR trigger\n");
			write(urmsfd, &db_urms, sizeof(db_urms_t));
		}
		else {
//                        printf("Got another trigger urmsfd %d\n", urmsfd);
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
                                        nread = read(urmsfd, &db_urms_status, sizeof(db_urms_status_t));
                                        if(nread > 0) {
                                                printf("receive_urms_data: nread %d\n", nread);
                                                for(i = 0; i<nread; i++)
                                                        printf("%#hhx ", readbuff[i]);
						printf("\n");
						db_clt_write(pclt, DB_URMS_STATUS_VAR, sizeof(db_urms_status_t), &db_urms_status);
                                        }
                                        if(nread == 0) {
                                                if(urmsfd > 0)
                                                        close(urmsfd);

                                                // Reopen connection to URMS controller (just the first time we lose connection;
                                                // after that, the file descriptor is less than zero)
                                		urmsfd = OpenSOBUListener(port);
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
                                urmsfd = OpenSOBUListener(port);
                                printf("2.2: urmsfd %d\n", urmsfd);
                                if(urmsfd < 0) {
                                        fprintf(stderr, "2: Could not open connection to SOBU\n");
				}
			}
		}
	}
}


#define MAXPENDING 3

static int OpenSOBUListener(unsigned short serv_port) {

	int serv_sd = -1;
	int client_sd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	char receive_buffer[1500];
	int rcvd;               /// count received from a call to recv
	int byte_count = 0;     /// count received in total from a client
	int fd;                 /// file descriptor for ouput
	char *file_name = "port.out";
	int loop_count = 0;
	int perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	int opt;
	int verbose = 0;

	printf("Echoing port %d\n", serv_port);

	if ((serv_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("TCP socket create failed");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(serv_port);
	if (verbose)
		printf("s_addr 0x%x, sin_port 0x%x\n",
		serv_addr.sin_addr.s_addr,
		serv_addr.sin_port);
	if (bind(serv_sd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0){
			perror("TCP socket bind failed");
			return -2;
	}
	if (listen(serv_sd, MAXPENDING) < 0) {
		perror("TCP listen failed");
		return -3;
	}

	unsigned int accept_length = sizeof(client_addr);
	printf("Waiting for client\n");
	if ((client_sd = accept(serv_sd,
		(struct sockaddr *) &client_addr,
		&accept_length)) < 0) {
			perror("Client accept failed");
			return -4;
		}
	printf("Handling client %s\n", inet_ntoa(client_addr.sin_addr));
	return client_sd;
}
