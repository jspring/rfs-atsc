/**\file
 *
 *	Routines to read and write intersection message set structure
 *	from files. May be used for debugging and traces in actual
 *	operation. In test code that does not use the PATH data bucket,
 *	these routines are called by replacement routines for the
 #	db utilities.
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */

#include "ix_msg.h"
#include "ix_db_utils.h"

// Writes approach values from an ix_approach_t structure to a file as
// text information, assumes multi-byte fields are already in correct
// byte order.
void ix_approach_update_file(FILE *fp, ix_approach_t * papp) 
{
	int i;
	ix_stop_line_t *pstop = &papp->stop_line_array[0];
	unsigned char nstop = papp->number_of_stop_lines;
	fprintf(fp, "%hhu %hhu %hd %hhu %hhu %hhu %hhu %hhu %hhu ",
		papp->approach_type, papp->signal_state,
		papp->time_to_next, papp->vehicles_detected, 
		papp->ped_signal_state,
		papp->seconds_to_ped_signal_state_change,
		papp->ped_detected,
		papp->seconds_since_ped_detect,
		papp->seconds_since_ped_phase_started);
	fprintf(fp, "%hhu %hhu %hhu %hhu %hhu %hhu %hhu \n",
		papp->emergency_vehicle_approach,
		papp->seconds_until_light_rail,
		papp->high_priority_freight_train,
		papp->vehicle_stopped_in_ix,
		papp->reserved[0],
		papp->reserved[1],
		papp->number_of_stop_lines);
	for (i = 0; i < nstop; i++) {
		fprintf(fp, "%d %d %hu %hu \n", pstop[i].longitude,
			pstop[i].latitude, pstop[i].line_length, 
			pstop[i].orientation);
	}
}

// Reads approach values from a file
// Initializes pointer to be used to hold stop array
// Pointer papp to ix_approach_t structure must already by defined.
// First line has overall approach info, 
// followed by variable number of stop lines
void ix_approach_read_file(FILE *fp, ix_approach_t * papp) 
{
	int i;
	ix_stop_line_t *pstop;	// malloc pointer to stop array
	int nstop;

	// array must be big enough to hold text line
	unsigned char tmp_buf[256];
	unsigned char *cur = tmp_buf;

	if (fgets(tmp_buf, 256, fp) == NULL) {
		fprintf(stderr, "Error reading approach line\n");
		return;
	}

	sscanf(tmp_buf,
"%hhu %hhu %hd %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu ",
		&papp->approach_type, &papp->signal_state,
		&papp->time_to_next, &papp->vehicles_detected, 
		&papp->ped_signal_state,
		&papp->seconds_to_ped_signal_state_change,
		&papp->ped_detected,
		&papp->seconds_since_ped_detect,
		&papp->seconds_since_ped_phase_started,
		&papp->emergency_vehicle_approach,
		&papp->seconds_until_light_rail,
		&papp->high_priority_freight_train,
		&papp->vehicle_stopped_in_ix,
		&papp->reserved[0],
		&papp->reserved[1],
		&papp->number_of_stop_lines);

	nstop = papp->number_of_stop_lines;
	pstop = (ix_stop_line_t *) malloc(sizeof(ix_stop_line_t) * nstop);
	papp->stop_line_array = pstop;

	for (i = 0; i < nstop; i++) {
		if (fgets(tmp_buf, 256, fp) == NULL) {
			fprintf(stderr, "Error reading stop line\n");
			return;
		}
		sscanf(tmp_buf, "%d %d %hu %hu \n", &pstop[i].longitude,
			&pstop[i].latitude, &pstop[i].line_length, 
			&pstop[i].orientation);
	}
}

// Writes intersection message and approach variables from pmsg structure
// to file, with message header on one line, followed by multi-line
// groups for each approach
void ix_msg_update_file(FILE *fp, ix_msg_t *pmsg) 
{
	struct timespec tval;
	struct tm tmval;
	int i;

 	fprintf(fp, "%hhu ", pmsg->flag);
 	fprintf(fp, "%hhu ", pmsg->version);
	fprintf(fp, "%hu ", pmsg->message_length);
	fprintf(fp, "%hhu ", pmsg->message_type);
	fprintf(fp, "%hhu ", pmsg->control_field);
	fprintf(fp, "%hhu ", pmsg->ipi_byte);
	fprintf(fp, "%u ", pmsg->intersection_id);
	fprintf(fp, "%u ", pmsg->map_node_id);
	fprintf(fp, "%d ", pmsg->ix_center_lat);
	fprintf(fp, "%d ", pmsg->ix_center_long);
	fprintf(fp, "%hd ", pmsg->ix_center_alt);
	fprintf(fp, "%d ", pmsg->antenna_lat);
	fprintf(fp, "%d ", pmsg->antenna_long);
	fprintf(fp, "%hd ", pmsg->antenna_alt);
	fprintf(fp, "%d %d ", pmsg->seconds, pmsg->nanosecs);
	fprintf(fp, "%hhu ", pmsg->cabinet_err);
	fprintf(fp, "%hhu ", pmsg->preempt_calls);
	fprintf(fp, "%hhu ", pmsg->bus_priority_calls);
	fprintf(fp, "%hhu ", pmsg->preempt_state);
	fprintf(fp, "%hhu ", pmsg->special_alarm);
	for (i = 0; i < 4; i++)
		fprintf(fp, "%hhu ", pmsg->reserved[i]); 
//	fprintf(fp, "%hhu ", pmsg->next_approach);
	fprintf(fp, "%hhu ", pmsg->num_approaches);
	fprintf(fp, "\n");
	
	for (i = 0; i < pmsg->num_approaches; i++) 
		ix_approach_update_file(fp, &pmsg->approach_array[i]);
}

// Reads from already opened file into the pmsg pointer
// Assumes base pmsg structure has already been malloced.
// Needs to malloc approach_array once num_approaches has been read
// Need to call ix_msg_free when done with it.
void ix_msg_read_file(FILE *fp, ix_msg_t *pmsg)
{
	int i;
	unsigned long long time_in_millisec = 0;
	int millisec;
	struct timespec tval;
	unsigned char tmp_buf[256];
	if (fgets(tmp_buf, 256, fp) == NULL) {
		fprintf(stderr, "Error reading message header\n");
		return;
	}
 	sscanf(tmp_buf,
"%hhu %hhu %hu %hhu %hhu %hhu %u %u %d %d %hd %d %d %hd %d %d %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu", 
	&pmsg->flag, &pmsg->version, &pmsg->message_length, 
	&pmsg->message_type, &pmsg->control_field, &pmsg->ipi_byte, 
	&pmsg->intersection_id, &pmsg->map_node_id, &pmsg->ix_center_lat, 
	&pmsg->ix_center_long, &pmsg->ix_center_alt, &pmsg->antenna_lat, 
	&pmsg->antenna_long, &pmsg->ix_center_alt, 
	&pmsg->seconds, &pmsg->nanosecs,
	&pmsg->cabinet_err, &pmsg->preempt_calls, &pmsg->bus_priority_calls, 
	&pmsg->preempt_state, &pmsg->special_alarm,
	&pmsg->reserved[0],  &pmsg->reserved[1],  
	&pmsg->reserved[2],  &pmsg->reserved[3],  
	&pmsg->num_approaches); 
//	&pmsg->next_approach, &pmsg->num_approaches); 

	pmsg->approach_array = (ix_approach_t *) malloc( sizeof(ix_approach_t)
					* pmsg->num_approaches);

	for (i = 0; i < pmsg->num_approaches; i++) 
		ix_approach_read_file(fp, &pmsg->approach_array[i]);
}
