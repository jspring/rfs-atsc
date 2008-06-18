#define MAX_VEHICLE_DETECTOR_GROUPS 8
#define MAX_PHASE_STATUS_GROUPS 	4

typedef struct 
{
    unsigned char vehicle_detector_status[MAX_VEHICLE_DETECTOR_GROUPS];
    unsigned char phase_status_reds[MAX_PHASE_STATUS_GROUPS];
    unsigned char phase_status_yellows[MAX_PHASE_STATUS_GROUPS];
    unsigned char phase_status_greens[MAX_PHASE_STATUS_GROUPS];
    unsigned char phase_status_flashing[MAX_PHASE_STATUS_GROUPS];
  	unsigned char phase_status_ons[MAX_PHASE_STATUS_GROUPS];
    unsigned char phase_status_next[MAX_PHASE_STATUS_GROUPS];
	int info_source;	// indicate whether NTCIP, AB348, sniffer, etc. 
	char hour;	/// Timestamp of last database update
	char min;
	char sec;
	short int millisec;
} atsc_typ;

// Unions for assembling and extracting a packet // 
union pack_short
{
	short s;
	unsigned char c[2];
};
union pack_interger
{
	int i;
	unsigned char c[4];
};
union pack_float
{
	float f;
	unsigned char c[4];
};
// Functions to extract information from a packet //
int ExtractChar(unsigned char *packet_array, int next, char *pvalue)
{
	int i = next;
	*pvalue = packet_array[i++];
	return(i);
}
int ExtractShortValue(unsigned char *packet_array, int next, int *pvalue)
{
	union pack_short pp;
	int i = next;
	pp.c[0] = packet_array[i++];
	pp.c[1] = packet_array[i++];
	*pvalue = (int)pp.s;
	return(i);
}
int ExtractIntergerValue(unsigned char *packet_array, int next, int *pvalue)
{
	union pack_interger pp;
	int i = next,j;
	for (j=0;j<4;j++)
	{
		pp.c[j] = packet_array[i++];
	}
	*pvalue = (int)pp.i;
	return(i);
}
int ExtractFloatValue(unsigned char *packet_array, int next, float *pvalue)
{
	union pack_float pp;
	int i = next,j;
	for (j=0;j<4;j++) 
	{
		pp.c[j] = packet_array[i++];
	}
	*pvalue = pp.f;
	return(i);
}
// Parsing ATSC message
int parseATSC(unsigned char *packet_array,atsc_typ *pInput)
{
	int nbuff = 0,i;
	int k;
	char c;
	// extract vehicle_detector_status
	for (i=0;i<MAX_VEHICLE_DETECTOR_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->vehicle_detector_status[i] = c;
	}
	// extract phase_status_reds
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_reds[i] = c;
	}
	// extract phase_status_yellows
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_yellows[i] = c;
	}
	// extract phase_status_greens
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_greens[i] = c;
	}
	// extract phase_status_flashing
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_flashing[i] = c;
	}
	// extract phase_status_ons
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_ons[i] = c;
	}
	// extract phase_status_next
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = ExtractChar(packet_array,nbuff,&c);
		pInput->phase_status_next[i] = c;
	}
	// extract info_source
	nbuff = ExtractIntergerValue(packet_array,nbuff,&k);
	pInput->info_source = k;
	// extract hour
	nbuff = ExtractChar(packet_array,nbuff,&c);
	pInput->hour = c;
	// extract min
	nbuff = ExtractChar(packet_array,nbuff,&c);
	pInput->min = c;
	// extract sec
	nbuff = ExtractChar(packet_array,nbuff,&c);
	pInput->sec = c;
	// insert millisec
	nbuff = ExtractShortValue(packet_array,nbuff,&k);
	pInput->millisec = k;
	return(nbuff);
}
