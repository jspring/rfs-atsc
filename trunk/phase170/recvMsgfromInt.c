/*
 * recvMsgfromInt.c
 * This program receives real-time atsc message and sms message, and save it in data file 
 *
 * Linux version
 *
 * Last updated: May 28, 2008
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/timeb.h>

#include "recvMsgfromInt.h"
#define BUFFER_SIZE 150

void saveATSCdata(FILE *fp,atsc_typ *theInput,int hour,int min,int sec,short int millisec);

int main(int argc,char* argv[])
{
	FILE *fpATC,*fpSMS;	
	// timer structure
	struct timeb timeptr_raw;
	struct tm time_converted;
	int date,year,month,day,hour,min,sec;
	short int millisec;	
	// file flags
	char fileName[100];
	int fpATC_exist = 0, fpSMS_exist = 0;
	int prev_hr = -1,prev_date = 0;
	// local variables
	atsc_typ atc;	
	// Linux socket
	int sockfd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	unsigned char recvBuffer[BUFFER_SIZE];	
	unsigned int nCharRecv,lenRecvSockAddr;							
	int myport,msg_typ,srn_prn = 0;
	
	//argument inputs
	if (argc != 4)
	{
		fprintf(stderr,"Usage: %s port msg_type <1/2 for ATSC/SMS> sereen_prn_flag\n",argv[0]);
		exit (EXIT_FAILURE);
	}
	myport = atoi(argv[1]);
	msg_typ = atoi(argv[2]);
	srn_prn = atoi(argv[3]);
	
	// open socket
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(myport);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0',8);
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
	{
		fprintf(stderr,"Socket creation error\n");
		exit(EXIT_FAILURE);
	}
	if ( bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr,"Binding error\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// recv socket
		nCharRecv = recvfrom(sockfd,(char *)recvBuffer,BUFFER_SIZE,0,
			(struct sockaddr *)&their_addr,&lenRecvSockAddr);			
		if (srn_prn == 1)
		{
			fprintf(stderr,"recv %d byte\n",nCharRecv);
		}
		// get date and time
	    ftime ( &timeptr_raw );
	    localtime_r ( &timeptr_raw.time, &time_converted );
		year = time_converted.tm_year-100; // year since 2000
		month = time_converted.tm_mon+1;
		day = time_converted.tm_mday;
		hour = time_converted.tm_hour;
		min = time_converted.tm_min;
		sec = time_converted.tm_sec;
		millisec = timeptr_raw.millitm;				
		date = year*10000+month*100+day;
		// Parse message 
		if ( msg_typ == 1)
		{
			// ATSC message
			nCharRecv = parseATSC(recvBuffer,&atc);
			// determine whether needs to re-open a file			
			if ( (hour != prev_hr || date != prev_date) && fpATC_exist == 1)
			{
				prev_hr = hour;
				prev_date = date;
				fclose(fpATC);
				fpATC_exist = 0;
			}
			if (fpATC_exist == 0)
			{
				sprintf(fileName,"signal/%02d%02d%02d.%02d.dat",month,day,year,hour);
				fpATC = fopen(fileName,"a+");
				fpATC_exist = 1;
			}
			if (srn_prn == 1)
			{
				fprintf(stderr,"write to file\n");
			}
			saveATSCdata(fpATC,&atc,hour,min,sec,millisec);
		}
	}
	close(sockfd);
	if (fpATC_exist == 1) fclose(fpATC);
	if (fpSMS_exist == 1) fclose(fpSMS);
	exit(EXIT_SUCCESS);
}

void saveATSCdata(FILE *fp,atsc_typ *theInput,int hour,int min,int sec,short int millisec)
{
	int i;
	fprintf(fp,"%02d:%02d:%02d.%03d",hour,min,sec,millisec);
	fprintf(fp," %02d:%02d:%02d.%03d",theInput->hour,theInput->min,
		theInput->sec,theInput->millisec);
	for (i=0;i<MAX_VEHICLE_DETECTOR_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->vehicle_detector_status[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_reds[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_yellows[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_greens[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_flashing[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_ons[i]);
	}
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		fprintf(fp," %d",theInput->phase_status_next[i]);
	}
	fprintf(fp," %d",theInput->info_source);	
	fprintf(fp,"\n");
	fflush(fp);
	
	return;
}
