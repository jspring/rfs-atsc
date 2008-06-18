int assemSMS(unsigned char *packet_array,smsobjarr_t *pInput)
{
	int nbuff = 0,i;	
	// loop to insert object structure
	for (i=0;i<MAXDBOBJS;i++)
	{
		// insert xrange (float)		
		nbuff = InsertFloatValue(packet_array,nbuff,pInput->object[i].xrange);
		// insert yrange (float)		
		nbuff = InsertFloatValue(packet_array,nbuff,pInput->object[i].yrange);
		// insert xvel (float)		
		nbuff = InsertFloatValue(packet_array,nbuff,pInput->object[i].xvel);
		// insert yvel (float)		
		nbuff = InsertFloatValue(packet_array,nbuff,pInput->object[i].yvel);
		// insert obj char)		
		nbuff = InsertChar(packet_array,nbuff,pInput->object[i].obj);
	}
	return(nbuff);
}
