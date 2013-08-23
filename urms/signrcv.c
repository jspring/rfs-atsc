#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <db_include.h>
#include <db_clt.h>
#include <db_utils.h>
#include "sign.h"
#include "aws.h"

#define BUFSIZE 17
#define TIMEOUT 50

static int sig_list[]=
{
   SIGINT,
   SIGQUIT,
   SIGTERM,
   SIGALRM,        // for timer
   ERROR
};
static jmp_buf exit_env;
static void sig_hand(int code)
{
   if (code == SIGALRM)
      return;
   else
      longjmp(exit_env, code);
}

int readmsg(int sd, char *buf) 
{
   int retval;
   int bytesread;
   
   for(bytesread = 0; bytesread < BUFSIZE-1; bytesread++) {
      if ((retval = read(sd, buf, 1)) <= 0)
         return retval;
      else {   
         if (*buf == '\n') {
            buf++;
            buf = '\0';
            return(bytesread+1);
         }  
      }
      buf++;
   }
   
   buf = '\0';
   return(bytesread+1);
}

void gettimestampstr(char *str)
{
   time_t ltime;
   struct tm *Tm;
   
   ltime=time(NULL);
   Tm=localtime(&ltime);
   
   sprintf(str, "[%04d-%02d-%02d %02d:%02d:%02d]:  ", Tm->tm_year+1900, Tm->tm_mon+1, Tm->tm_mday, Tm->tm_hour, Tm->tm_min, Tm->tm_sec);
}

void print_perror(char *msg)
{
   char str[30];
   
   memset(str, 0, 30);

   gettimestampstr(str);
   fprintf(stderr, "%s", str);
   perror(msg);
}

int parse_message(char *msg, int id, db_clt_typ *pclt) {
   sign_0_data_t sign_data;
   char *header = strtok(msg, ",");
   char str[30];

   if (header == NULL || 0 != strcmp("$SIGN", header)) {
      memset(str, 0, 30);
      gettimestampstr(str);
      fprintf(stderr, "%sparse_message: Could not tokenize!\n", str);
      return(-1);
   }
   
   unsigned char state = atoi(strtok(NULL, ","));
   unsigned char signid = atoi(strtok(NULL, ","));
   unsigned short int seqnum = atoi(strtok(NULL, ","));
   
   memset(&sign_data, 0, sizeof(sign_0_data_t));
   
   sign_data.state = state;
   
   if (state == STATE_ON || state == STATE_CONT || state == STATE_LOOP1 || state == STATE_LOOP2 || state == STATE_LOOP3)
      sign_data.state_simple = 0x1;
   else
      sign_data.state_simple = 0x0;
      
   sign_data.seqnum = seqnum;
   time(&sign_data.ts);
   
   if (id == 0)    
      db_clt_write(pclt, DB_SIGN_0_DATA_VAR, sizeof(sign_0_data_t), &sign_data);
   else if (id == 1)
      db_clt_write(pclt, DB_SIGN_1_DATA_VAR, sizeof(sign_1_data_t), &sign_data);   
   else if (id == 2)
      db_clt_write(pclt, DB_SIGN_2_DATA_VAR, sizeof(sign_2_data_t), &sign_data); 
   else if (id == 3)
      db_clt_write(pclt, DB_SIGN_3_DATA_VAR, sizeof(sign_3_data_t), &sign_data); 
   
   memset(str, 0, 30);
   gettimestampstr(str);
   fprintf(stderr, "%sWrote time=%u state=%hhu seqnum=%hu for sign=%hhu to database\n", str, (unsigned int)sign_data.ts, sign_data.state, sign_data.seqnum, signid);
   
   return(0);
}

#define LOCAL_IP "10.0.1.202"

int openconnection(char *remote_ip, unsigned short port)
{
   int sockfd;
   int newsockfd;
   struct sockaddr_in local_addr;
   struct sockaddr_in remote_addr;
   socklen_t localaddrlen = sizeof(local_addr);

   sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (sockfd < 0) {
      print_perror("ERROR opening socket");
      return -1;
   }

   memset(&local_addr, 0, sizeof(struct sockaddr_in));
   local_addr.sin_family = AF_INET;
   local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);    //htonl(INADDR_ANY);//
   local_addr.sin_port = htons(port);
   if ((bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr))) < 0) {
      print_perror("ERROR on binding");
      return -1;
   }

   listen(sockfd, 1);

    /** set up remote socket addressing and port */
   memset(&remote_addr, 0, sizeof(struct sockaddr_in));
   remote_addr.sin_family = AF_INET;
   remote_addr.sin_addr.s_addr = inet_addr(remote_ip);  //htonl(INADDR_ANY);
   remote_addr.sin_port = htons(port);
   localaddrlen = sizeof(remote_addr);

   if ((newsockfd = accept(sockfd, (struct sockaddr *) &remote_addr, &localaddrlen)) < 0) {
      print_perror("ERROR on accept");
      return -1;
   };
   
   close(sockfd);
   return newsockfd;
}
   
int main(int argc, char **argv)
{
   db_clt_typ *pclt = NULL;
   char hostname[MAXHOSTNAMELEN+1];
   char *domain = DEFAULT_SERVICE;
   int xport = COMM_OS_XPORT;
   
   int signid = -1;
   int option;
   
   int retval;
   int sd;
   char *sign_ip;
   unsigned short sign_port;
   struct timeval tv;
   fd_set myfdset;
   char buf[BUFSIZE];
   char str[30];
   
   time_t now;
   aws_event_data_t aws_event_data;
   sign_0_data_t sign_data;
   int seconds_since_last_event;
   
   while ((option = getopt(argc, argv, "s:")) != EOF) {
      switch (option) {
         case 's':
            signid = atoi(optarg);
            break;
         default:
            fprintf(stderr, "Usage:  signrcv -s signid\n");
            exit(1);
      }
   }

   switch (signid) {
      case 0:
         sign_ip = strdup(SIGN_0_IP);
         sign_port = SIGN_0_PORT;
         break;
      case 1:
         sign_ip = strdup(SIGN_1_IP);
         sign_port = SIGN_1_PORT;
         break;
      case 2:
         sign_ip = strdup(SIGN_2_IP);
         sign_port = SIGN_2_PORT;
         break;
      case 3:
         sign_ip = strdup(SIGN_3_IP);
         sign_port = SIGN_3_PORT;
         break;
      default:
         fprintf(stderr, "Invalid sign id!\n");
         exit(1);                  
   }
   
   while ((sd = openconnection(sign_ip, sign_port)) < 0) {
      sleep(1);
   }
            
   if(setjmp(exit_env) != 0) {
      /* Log out from the database. */
     if (pclt != NULL)
        clt_logout(pclt);
     close(sd);   
     exit(0);
   }
   else
      sig_ign(sig_list, sig_hand);
      
   /* Log in to the database (shared global memory).  Default to the current host. */
   get_local_name(hostname, MAXHOSTNAMELEN);
   
   if ((pclt = clt_login(argv[0], hostname, domain, xport)) == NULL) {
      fprintf(stderr, "Database initialization error in %s.\n", argv[0]);
      longjmp(exit_env, 3);
   }
   while(1) {
      memset(buf, 0, BUFSIZE);
      FD_ZERO(&myfdset);
      FD_SET(sd, &myfdset);
      tv.tv_usec = TIMEOUT * 1000;
      tv.tv_sec = 0;
      
      retval = select(sd + 1, &myfdset, NULL, NULL, &tv);
      
      if (retval == 0) {
         seconds_since_last_event = -1;
         time(&now);
         memset(&sign_data, 0, sizeof(sign_data));
         memset(&aws_event_data, 0, sizeof(aws_event_data));
         
         db_clt_read(pclt, DB_STS_EVENT_DATA_VAR, sizeof(aws_event_data_t), &aws_event_data);
         seconds_since_last_event = now - aws_event_data.rcvr[0].ts;
         
         if (seconds_since_last_event <= 2) {    
            if (signid == 0)    
               db_clt_read(pclt, DB_SIGN_0_DATA_VAR, sizeof(sign_0_data_t), &sign_data);
            else if (signid == 1)
               db_clt_read(pclt, DB_SIGN_1_DATA_VAR, sizeof(sign_1_data_t), &sign_data);   
            else if (signid == 2)
               db_clt_read(pclt, DB_SIGN_2_DATA_VAR, sizeof(sign_2_data_t), &sign_data); 
            else if (signid == 3)
               db_clt_read(pclt, DB_SIGN_3_DATA_VAR, sizeof(sign_3_data_t), &sign_data);

            if ((now - sign_data.ts > seconds_since_last_event) || (sign_data.state != STATE_ON  && sign_data.state != STATE_CONT)) {
               memset(str, 0, 30);
               gettimestampstr(str);
   
               fprintf(stderr, "%sEvent detected, turning on sign!\n", str);
                     
               if ((retval = write(sd, "TURNON\n", 7)) < 0)
                  print_perror("ERROR on write");                  
            }
         }    
      } 
      else if (retval < 0) {
         print_perror("ERROR on select");
         close(sd);
         while ((sd = openconnection(sign_ip, sign_port)) < 0) {
            sleep(1);
         }
      } 
      else {
         if ((retval = readmsg(sd, buf)) <= 0) {
            print_perror("ERROR on read");
            close(sd);
            while ((sd = openconnection(sign_ip, sign_port)) < 0) {
               sleep(1);
            }
         }   
         else if (retval == 16) {
            memset(str, 0, 30);
            gettimestampstr(str);
            fprintf(stderr, "%sMessage received: %s\n", str, buf);
            
            parse_message(buf, signid, pclt);
         }
         else {
            memset(str, 0, 30);
            gettimestampstr(str);
            fprintf(stderr, "%sBad message received:  %s, %d bytes\n", str, buf, retval);
         }
      }       
   }
   
   longjmp(exit_env, 4);         
}

