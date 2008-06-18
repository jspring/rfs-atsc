int assemATSC(unsigned char *packet_array,atsc_typ *pInput)
{
	int nbuff = 0,i;	
	// insert vehicle_detector_status
	for (i=0;i<MAX_VEHICLE_DETECTOR_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->vehicle_detector_status[i]);
	}
	// insert phase_status_reds
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_reds[i]);
	}
	// insert phase_status_yellows
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_yellows[i]);
	}
	// insert phase_status_greens
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_greens[i]);
	}
	// insert phase_status_flashing
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_flashing[i]);
	}
	// insert phase_status_ons
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_ons[i]);
	}
	// insert phase_status_next
	for (i=0;i<MAX_PHASE_STATUS_GROUPS;i++)
	{
		nbuff = InsertChar(packet_array,nbuff,pInput->phase_status_next[i]);
	}
	// insert info_source
	nbuff = InsertIntergerValue(packet_array,nbuff,pInput->info_source);
	// insert hour
	nbuff = InsertChar(packet_array,nbuff,pInput->ts.hour);
	// insert min
	nbuff = InsertChar(packet_array,nbuff,pInput->ts.min);
	// insert sec
	nbuff = InsertChar(packet_array,nbuff,pInput->ts.sec);
	// insert millisec
	nbuff = InsertShortValue(packet_array,nbuff,pInput->ts.millisec);	
	return(nbuff);
}
