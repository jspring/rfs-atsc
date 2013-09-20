/* receive_urms_data.c - Listens for a connection from SOBU46, opens it and writes data
** from the ramp meter computer to database
**
** receive_urms_data is a TCP listener.  If the connection is dropped from the other 
** end of the socket pair, bind thinks the original address is still bound, and 
** throws an error. You cannot "un-bind" the address, so what I've done is to exit
** with an exit status of 1 (EXIT_FAILURE). An external while loop restarts 
** receive_urms_data in that case.
*/

#include "urms.h"
#include "tos.h"
#include "/home/art_coord_ramp_metering/src/wrfiles_ac_rm.h"

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

const char *usage = "-v (verbose) -p <port (def. 4444)> -i <loop interval (ms)>";

db_id_t db_vars_list[] =  {
        {DB_URMS_VAR, sizeof(db_urms_t)},
        {DB_URMS_STATUS_VAR, sizeof(db_urms_status_t)}
};
int num_db_variables = sizeof(db_vars_list)/sizeof(db_id_t);

int db_trig_list[] =  {
        DB_URMS_VAR,
        DB_URMS_STATUS_VAR
};
int num_trig_variables = sizeof(db_trig_list)/sizeof(int);

static int OpenSOBUListener(unsigned short serv_port);

int main(int argc, char *argv[]) {
	int urmsfd;
	unsigned short port = 4444;

        int option;

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        unsigned int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h
        posix_timer_typ *ptimer;
        trig_info_typ trig_info;
	db_urms_status_t db_urms_status;
	unsigned char *inmsg = (unsigned char *) &db_urms_status;
	unsigned short checksum = 0;
	db_urms_t db_urms;
	urms_datafile_t urms_datafile;
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
		fprintf(stderr, "Could not open listener to SOBU\n");
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
 //                   printf("Got another trigger urmsfd %d\n", urmsfd);
			if(urmsfd > 0) {
                                FD_ZERO(&readfds);
                                FD_SET(urmsfd, &readfds);
                                timeout.tv_sec = 10;
                                timeout.tv_usec = 1000;
                                if( (selectval = select(urmsfd+1, &readfds, NULL, NULL, &timeout)) < 0) {
                                        if(errno != EINTR) {
                                                perror("select 1");
                                                inportisset = (FD_ISSET(urmsfd, &readfds)) == 0 ? "no" : "yes";
                                                printf("\n\nser_driver_read 1: urmsfd %d selectval %d inportisset %s\n\n", urmsfd, selectval, inportisset);
						fprintf(stderr, "Error on connection to SOBU selectval %d Exiting....\n", selectval);
						db_list_done(pclt, db_vars_list, num_db_variables , db_trig_list, num_trig_variables);
//						db_list_done(pclt, NULL, 0, db_trig_list, num_trig_variables);
						close(urmsfd);
						exit(EXIT_FAILURE);
					}
                                }
                                if(selectval == 0) {
					fprintf(stderr, "Connection to SOBU timed out selectval %d Exiting....\n", selectval);
					db_list_done(pclt, db_vars_list, num_db_variables , db_trig_list, num_trig_variables);
					close(urmsfd);
					exit(EXIT_FAILURE);
				}
                                if(selectval > 0) {
                                        nread = read(urmsfd, &db_urms_status, sizeof(db_urms_status_t));
//					fprintf(stderr, "Everything should be OK. selectval %d nread %d\n", selectval, nread);

                                        if(nread == sizeof(db_urms_status)) {
					    if(verbose) {
                                                printf("receive_urms_data: nread %d\n", nread);
                                                for(i = 0; i<nread; i++)
                                                        printf("%d:%#hhx ", i, readbuff[i]);
						printf("\n");
					    }
					    checksum = 0;
					    for(i=0; i < (sizeof(db_urms_status_t) - 2); i++)
					   	 checksum += inmsg[i];
					    if(checksum == db_urms_status.checksum) {
						db_clt_write(pclt, DB_URMS_STATUS_VAR, sizeof(db_urms_status_t), &db_urms_status);
						for(i=0; i<3; i++) {
							urms_datafile.mainline_lead_occ[i] = 0.1 * ((db_urms_status.mainline_stat[i].lead_occ_msb << 8) 
											         + (unsigned char)(db_urms_status.mainline_stat[i].lead_occ_lsb));
							urms_datafile.mainline_trail_occ[i] = 0.1 * ((db_urms_status.mainline_stat[i].trail_occ_msb << 8) 
												 + (unsigned char)(db_urms_status.mainline_stat[i].trail_occ_lsb));
							urms_datafile.queue_occ[i] = 0.1 * ((db_urms_status.queue_stat[i].occ_msb << 8) 
											 + (unsigned char)(db_urms_status.queue_stat[i].occ_lsb));
							urms_datafile.metering_rate[i] = ((db_urms_status.metered_lane_stat[i].metered_lane_rate_msb << 8) 
											 + (unsigned char)(db_urms_status.metered_lane_stat[i].metered_lane_rate_lsb));
							if(verbose) {
							    printf("2:ML%d occ %.1f\n", i+1, urms_datafile.mainline_lead_occ[i]);
							    printf("2:MT%d occ %.1f\n", i+1, urms_datafile.mainline_trail_occ[i]);
							    printf("2:Q%d-1 occ %.1f\n", i+1, urms_datafile.queue_occ[i]);
							}
						}
						db_clt_write(pclt, DB_URMS_DATAFILE_VAR, sizeof(urms_datafile_t), &urms_datafile);
					    }
                                        }
                                        if(nread == 0) {
                                                fprintf(stderr, "Lost connection to SOBU. Exiting....\n");
						exit(EXIT_FAILURE);
                                                }
                                        }
			}
		}
//		fprintf(stderr, "End of loop selectval %d nread %d\n", selectval, nread);
		nread = -500;
		selectval = -500;
	}
}


#define MAXPENDING 3

static int OpenSOBUListener(unsigned short serv_port) {

	int serv_sd = -1;
	int client_sd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
	int verbose = 0;
	int reuse = 1;

	printf("Echoing port %d\n", serv_port);

	if ((serv_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("TCP socket create failed");
		return -1;
	}

	if(setsockopt(serv_sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
		perror("setsockopt");
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
