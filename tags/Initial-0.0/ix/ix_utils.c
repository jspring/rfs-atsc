/**\file
 *
 *	Functions used by intersection message set send and receive routines
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */	

#include "ix_msg.h"
#undef DEBUG_EXTRACT
#undef DEBUG_FORMAT
#undef DEBUG_PRINT

// swap 2-byte and 4-byte fields in place
void ix_msg_swap(ix_msg_t *pix)
{
	pix->message_length = ntohs(pix->message_length);
	pix->intersection_id = ntohl(pix->intersection_id);
	pix->map_node_id = ntohl(pix->map_node_id);
	pix->ix_center_long = ntohl(pix->ix_center_long);
	pix->ix_center_lat = ntohl(pix->ix_center_lat);
	pix->ix_center_alt = ntohs(pix->ix_center_alt);
	pix->antenna_long = ntohl(pix->antenna_long);
	pix->antenna_lat = ntohl(pix->antenna_lat);
	pix->antenna_alt = ntohs(pix->antenna_alt);
	pix->seconds = ntohl(pix->seconds);
	pix->nanosecs = ntohl(pix->nanosecs);
}

void ix_approach_swap(ix_approach_t *pappr)
{
	pappr->time_to_next = ntohs(pappr->time_to_next);
}

void ix_stop_line_swap(ix_stop_line_t *pstop)
{
	pstop->longitude = ntohl(pstop->longitude);
	pstop->latitude = ntohl(pstop->latitude);
	pstop->line_length = ntohs(pstop->line_length); 
	pstop->orientation = ntohs(pstop->orientation);	
}

/* Code to convert to byte array is not working. Currently using
 * using two 4-byte fields of struct timespec instead.
 */

/** Converts ix_msg.ts array of characters to struct timespec.
 *  Use standard conversion functions like ctime to convert for printing, etc.
 */
void ix_ts_to_tm(unsigned char *ts, struct timespec *ptmspec)
{
	int i;
	unsigned long long time_in_millisec = ts[7];
	printf("ts_to_tm: ");
	printf("%hhu ", ts[7]);
	for (i = 6; i >= 0; i--) {
		time_in_millisec <<= 8;
		time_in_millisec |= ts[i];
		printf("%hhu ", ts[i]);
	}
	printf("\n");
	ptmspec->tv_sec = time_in_millisec / 1000;
	ptmspec->tv_nsec = (time_in_millisec % 1000) * 1000000;
	printf("sec %d nsec %d\n", ptmspec->tv_sec, ptmspec->tv_nsec);
}

/** Converts struct timespec into array
 */
void ix_tm_to_ts(struct timespec *ptmspec, unsigned char *ts)
{
	int i;
	unsigned long long time_in_millisec = 0;	
	time_in_millisec = ptmspec->tv_sec * 1000 + ptmspec->tv_nsec/1000000;
	printf("tm_to_ts: ");
	printf("sec %d nsec %d\n", ptmspec->tv_sec, ptmspec->tv_nsec);
	for (i = 0; i < 8; i++) {
		ts[i] = (time_in_millisec >> (i*8)) & 0xff;
		printf("%d:%hhu ", ts[i]);
	}
	printf("\n"); 
}

/** Converts type to string for printing
 */
const char *ix_approach_string(unsigned char approach_type)
{
	switch(approach_type) {
	case APPROACH_T_STRAIGHT_THROUGH:
		return "straight_through";
	case APPROACH_T_LEFT_TURN:
		return "left_turn";
	case APPROACH_T_RIGHT_TURN:
		return "right_turn";
	default:
		return "unknown";
	}
}

/** Converts signal state to string for printing
 */
const char *ix_signal_state_string(unsigned char signal_state)
{
	switch (signal_state) {
	case SIGNAL_STATE_GREEN:
		return "G";
	case SIGNAL_STATE_YELLOW:
		return "Y";
	case SIGNAL_STATE_RED:
		return "R";
	case SIGNAL_STATE_FLASHING_YELLOW:
		return "FY";
	case SIGNAL_STATE_FLASHING_RED:
		return "FR";
	case SIGNAL_STATE_GREEN_ARROW:
		return "GA";
	case SIGNAL_STATE_YELLOW_ARROW:
		return "YA";
	case SIGNAL_STATE_RED_ARROW:
		return "RA";
	default:
		return "?";
	}
}


/** Converts received byte array into message structure
 *  Allocates the message structure, initializing the pointer from
 *  the calling program
 */
void ix_msg_extract(unsigned char *buf, ix_msg_t **ppix)
{
	ix_approach_t *pappr;	// pointer to variable-size array of approaches 
	ix_stop_line_t *pstop;	// ponter to variable-size array of stop lines
	unsigned char *cur;     // current extraction point in buffer 
	int i,j;		// temporaries for array processing 
	ix_msg_t *pix;		// pointer to message

	*ppix = malloc(sizeof(ix_msg_t));
	pix = *ppix;
	ix_msg_swap((ix_msg_t *) buf);	// swaps bytes in place in buf
	memcpy(pix, buf, sizeof(ix_msg_t) - IX_POINTER_SIZE);
	cur = buf + sizeof(ix_msg_t) - IX_POINTER_SIZE;
	
	pappr = malloc(pix->num_approaches * sizeof(ix_approach_t)); 
	pix->approach_array = pappr;
#ifdef DEBUG_EXTRACT
	printf("extract: pix 0x%x pappr 0x%x, approaches\n", pix, pappr,
			pix->num_approaches);
	fflush(stdout);
#endif
		
	for (i = 0; i < pix->num_approaches; i++) {
		ix_approach_swap((ix_approach_t *) cur); // swap bytes in place
		memcpy(&pappr[i], cur, sizeof(ix_approach_t) - IX_POINTER_SIZE);
				
		cur += sizeof(ix_approach_t) - IX_POINTER_SIZE;
		pstop = malloc(pappr[i].number_of_stop_lines *
						sizeof(ix_stop_line_t));
		pappr[i].stop_line_array = pstop;
#ifdef DEBUG_EXTRACT
	printf("extract: &pappr[%d] 0x%x, pstop 0x%x, signal_state %d\n",
			i, &pappr[i], pstop, pappr[i].signal_state);
	printf("signal state offset: %d\n", 
			(int)&pappr[i].signal_state - (int)&pappr[i]);
				
		fflush(stdout);
#endif
		for (j = 0; j < pappr[i].number_of_stop_lines; j++){
			ix_stop_line_swap((ix_stop_line_t *) cur);
			memcpy(&pstop[j], cur, sizeof(ix_stop_line_t));
			cur += sizeof(ix_stop_line_t);
		}
	}
}

void ix_print_hdr_offsets(ix_msg_t *pix)
{
	int i;
	printf("ix_msg_t offsets:\n");
	printf("flag %d\n", (int)&pix->flag - (int) pix);
	printf("version %d\n", (int)&pix->version - (int) pix);
	printf("message_length %d\n", (int)&pix->message_length - (int) pix);
	printf("message_type %d\n", (int)&pix->message_type - (int) pix);
	printf("control_field %d\n", (int)&pix->control_field - (int) pix);
	printf("ipi_byte %d\n", (int)&pix->ipi_byte - (int) pix);
	printf("intersection_id %d\n", (int)&pix->intersection_id - (int) pix);
	printf("map %d\n", (int)&pix->map_node_id - (int) pix);
	printf("ix_center_long %d\n", (int)&pix->ix_center_long - (int) pix);
	printf("ix_center_lat %d\n", (int)&pix->ix_center_lat - (int) pix);
	printf("ix_center_alt %d\n", (int)&pix->ix_center_alt - (int) pix);
	printf("antenna_long %d\n", (int)&pix->antenna_long - (int) pix);
	printf("antenna_lat %d\n", (int)&pix->antenna_lat - (int) pix);
	printf("antenna_alt %d\n", (int)&pix->antenna_alt - (int) pix);
	printf("seconds %d\n", (int)&pix->seconds - (int) pix);
	printf("nanosecs %d\n", (int)&pix->nanosecs - (int) pix);
	printf("cabinet %d\n", (int)&pix->cabinet_err - (int) pix);
	printf("preempt %d\n", (int)&pix->preempt_calls - (int) pix);
	printf("bus %d\n", (int)&pix->bus_priority_calls - (int) pix);
	printf("preempt %d\n", (int)&pix->preempt_state - (int) pix);
	printf("special %d\n", (int)&pix->special_alarm - (int) pix);
	for (i = 0; i < 4; i++)
		printf("reserved %d\n", (int)&pix->reserved[i] - (int) pix); 
//	printf("next %d\n", (int)&pix->next_approach - (int) pix);
	printf("number %d\n", (int)&pix->num_approaches - (int) pix);
}

/** Assumes that message is being flattenened into buffer that begins
 * at the pix address, print the offsets for the current approach
 * being placed into the buffer at pappr.
 *
 * By supplying pappr as first argument, can print offset within approach
 * of each field.
 */
void ix_print_approach_offsets(ix_msg_t *pix, ix_approach_t *pappr, int num)
{
	printf("Offsets for approach %d \n", num);
    	printf("approach_type %d\n", (int) &pappr->approach_type - (int) pix);	
   	printf("signal_state %d\n", (int) &pappr->signal_state - (int) pix);	
   	printf("time_to_next %d\n", (int)&pappr->time_to_next - (int) pix);	
   	printf("vehicles_detected %d\n", (int)&pappr->vehicles_detected - (int) pix); 
   	printf("ped_signal_state %d\n", (int)&pappr->ped_signal_state - (int) pix);	
   	printf("seconds_to_ped_signal_state_change %d\n", (int)&pappr->seconds_to_ped_signal_state_change - (int) pix);
   	printf("ped_detected %d\n", (int)&pappr->ped_detected - (int) pix);;
   	printf("seconds_since_ped_detect %d\n", (int)&pappr->seconds_since_ped_detect - (int) pix);
   	printf("seconds_since_ped_phase_started %d\n", (int)&pappr->seconds_since_ped_phase_started - (int) pix);
   	printf("emergency_vehicle_approach %d\n", (int)&pappr->emergency_vehicle_approach - (int) pix); 
   	printf("seconds_until_light_rail %d\n", (int)&pappr->seconds_until_light_rail - (int) pix); 
   	printf("high_priority_freight_train %d\n", (int)&pappr->high_priority_freight_train - (int) pix); 
   	printf("vehicle_stopped_in_ix %d\n", (int)&pappr->vehicle_stopped_in_ix - (int) pix); 
   	printf("reserved[0] %d\n", (int)&pappr->reserved[0] - (int) pix);
   	printf("reserved[1] %d\n", (int)&pappr->reserved[1] - (int) pix);
   	printf("number_of_stop_lines %d\n", (int)&pappr->number_of_stop_lines - (int) pix);
}

void ix_print_stopline_offsets(ix_msg_t *pix, ix_stop_line_t *pstop, int num)
{
	printf("Offsets for stop line %d\n", num);
	printf("latitude %d\n", (int) &pstop->latitude - (int) pix); 
	printf("longitude %d\n", (int) &pstop->longitude - (int) pix); 
	printf("line_length %d\n", (int) &pstop->line_length - (int) pix); 
	printf("orientation %d\n", (int) &pstop->orientation - (int) pix); 
}

/** Converts message structure into byte array to send
 *  Buffer buf is not allocated, must be large enough to hold
 *  all approaches.
 *
 *  Returns number of characters put in buffer, or -1 on error.
 *  Do_print option is for debugging, and for standalone program
 *  ix_byte_count that can be used to print offsets as an aid in
 *  writing and debugging receive code in another environment.
 */
int ix_msg_format(ix_msg_t *pix, unsigned char *buf, int buf_size, 
			int do_print)
{
	ix_approach_t *pappr;    // pointer to elements of approach array
	ix_stop_line_t *pstop;	// ponter to variable-size array of stop lines
	unsigned char *cur;     // current formatting point in buffer 
	int i,j;		// temporaries for array processing 
	int retval = 0;		// number of bytes added to buffer so far

	retval += sizeof(ix_msg_t) - IX_POINTER_SIZE;
	if (retval > buf_size)
		return -1;
	memcpy(buf, (unsigned char *) pix, sizeof(ix_msg_t) - IX_POINTER_SIZE);
	ix_msg_swap((ix_msg_t *) buf);	// swaps bytes in place in buf
	if (do_print) {
		ix_print_hdr_offsets((ix_msg_t *) buf);
		printf("msg header offset: %d\n", retval);
	}
	cur = buf + retval;
	pappr = pix->approach_array;	
	for (i = 0; i < pix->num_approaches; i++) {
		retval += sizeof(ix_approach_t) - IX_POINTER_SIZE;
		if (retval  > buf_size)
			return -1;
		memcpy(cur, &pappr[i], sizeof(ix_approach_t) - IX_POINTER_SIZE);
		if (do_print)
			ix_print_approach_offsets(
				(ix_msg_t *) buf, (ix_approach_t *) cur, i); 

		ix_approach_swap((ix_approach_t *) cur); // swap bytes in place

		cur += sizeof(ix_approach_t) - IX_POINTER_SIZE;
		pstop = pappr[i].stop_line_array;
		for (j = 0; j < pappr[i].number_of_stop_lines; j++){
			retval += sizeof(ix_stop_line_t);
			memcpy(cur, &pstop[j], sizeof(ix_stop_line_t));
			ix_stop_line_swap((ix_stop_line_t *) cur);
			if (do_print)
				ix_print_stopline_offsets(
				(ix_msg_t *) buf, (ix_stop_line_t *) cur, j); 
			cur += sizeof(ix_stop_line_t);
		}
	}
	return retval;
}

/** Frees internal pointers, and then frees message structure
 *  Sets pointer passed from main program to NULL.
 */
void ix_msg_free(ix_msg_t **ppix)
{
	ix_approach_t *pappr;	// pointer to variable-size array of approaches 
	int i,j;		// temporaries for array processing 
	ix_msg_t *pix = *ppix;

	pappr = pix->approach_array;
	for (i = 0; i < pix->num_approaches; i++) 
		free(pappr[i].stop_line_array);

	free(pappr);
	free(pix);
	*ppix = NULL;
}

/** Prints message from structure with field labels and formatting.
 */
void ix_msg_print(ix_msg_t *pix)
{
	struct timespec tval;	// convert 8 byte timestamp to this
	struct tm tmval;	// use this for formatted output
	ix_approach_t *pappr;	// pointer to variable-size array of approaches 
	ix_stop_line_t *pstop;	// pointer to variable-size array of stop lines
	int i,j;		// temporaries for array processing 

 	printf("flag 0x%02hhx\n", pix->flag);
 	printf("version 0x%02hhx\n", pix->version);
	printf("message length %hd\n", pix->message_length);
	printf("message type 0x%02hhx\n", pix->message_type);
	printf("control field 0x%02hhx\n", pix->control_field);
	printf("ipi byte 0x%02hhx\n", pix->ipi_byte);
	printf("intersection id %d\n", pix->intersection_id);
	printf("map node id %d\n", pix->map_node_id);
	printf("ix center lat %d\n", pix->ix_center_lat);
	printf("ix center long %d\n", pix->ix_center_long);
	printf("ix center alt %d\n", pix->ix_center_alt);
	printf("antenna lat %d\n", pix->antenna_lat);
	printf("antenna long %d\n", pix->antenna_long);
	printf("antenna alt %d\n", pix->antenna_alt);
	localtime_r(&pix->seconds, &tmval);	// use gmtime to print UTC
	printf("local time %2d:%02d:%02d.%03d\n", tmval.tm_hour,
			tmval.tm_min, tmval.tm_sec, pix->nanosecs / 1000000);
	printf("cabinet err 0x%02x\n", pix->cabinet_err);
	printf("preempt calls 0x%02x\n", pix->preempt_calls);
	printf("bus priority calls 0x%02x\n", pix->bus_priority_calls);
	printf("preempt state 0x%02x\n", pix->preempt_state);
	printf("special alarm 0x%02x\n", pix->special_alarm);
	for (i = 0; i < 4; i++)
		printf("reserved 0x%02x\n", pix->reserved[i]); 
//	printf("next approach 0x%02x\n", pix->next_approach);
	printf("number of approaches %d\n", pix->num_approaches);
	
	pappr = pix->approach_array;
#ifdef DEBUG_PRINT
	printf("print: pix 0x%x, pappr 0x%x\n", pix, pappr);
	fflush(stdout);
	for (i = 0; i < pix->num_approaches; i++)
		printf("signal_state: %0hhd\n", pappr[i].signal_state);
#endif
		
	for (i = 0; i < pix->num_approaches; i++)
	{
		printf("\n");
		printf("Approach %d: type %s\n",
			 i+1, ix_approach_string(pappr[i].approach_type));
		printf("\tsignal_state %s\n", 
			ix_signal_state_string(pappr[i].signal_state));
        	printf("\ttime_to_next %d\n", pappr[i].time_to_next);
        	printf("\tvehicles_detected %d\n", pappr[i].vehicles_detected);
        	printf("\tped_signal_state %d\n", pappr[i].ped_signal_state);
        	printf("\tped_detected %d\n", pappr[i].ped_detected);
        	printf("\tseconds_since_ped_detect %d\n",
			 pappr[i].seconds_since_ped_detect);
        	printf("\tseconds_since_ped_phase_started %d\n",
			 pappr[i].seconds_since_ped_phase_started);
        	printf("\temergency_vehicle_approach %d\n",
			 pappr[i].emergency_vehicle_approach);
        	printf("\tseconds_until_light_rail %d\n",
			 pappr[i].seconds_until_light_rail);
        	printf("\thigh_priority_freight_train %d\n",
			 pappr[i].high_priority_freight_train);
        	printf("\tvehicle_stopped_in_ix %d\n",
			 pappr[i].vehicle_stopped_in_ix);
		for (j = 0; j < 2; j++)
			printf("\treserved %d 0x%02x\n", pappr[i].reserved[j]); 
		printf("\tnumber_of_stop_lines %d\n",
			 pappr[i].number_of_stop_lines);

		pstop = pappr[i].stop_line_array; 

		for (j = 0; j < pappr[i].number_of_stop_lines; j++)
		{
			printf("\tStop line %d: %d %d %d %d\n", j+1, 
				pstop[j].latitude, pstop[j].longitude, 
				pstop[j].line_length, pstop[j].orientation);
		}
	}
	printf("\n");
}
