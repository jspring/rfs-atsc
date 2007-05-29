/**\file
 *
 *	Routines to read and write intersection message set structure
 *	from PATH "data bucket" server.
 *
 *	In TEST_ONLY mode, file reads/writes are used instead.
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */

#include "ix_msg.h"
#include "ix_db_utils.h"

#undef DEBUG_STOP
#ifdef TEST_ONLY
// To test send/receive only, can install on systems without
// PATH "data bucket" code, compile test programs using TEST_ONLY
// and do I/O to files, using a trace from a real run to provide
// the signal change data.

void ix_approach_update(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var_start)
{
	ix_approach_update_file(pclt, papp);
}

void ix_approach_read(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var_start)
{
	ix_approach_read_file(pclt, papp);
}

void ix_msg_update(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var_start)
{
	ix_msg_update_file(pclt, pmsg);
}

void ix_msg_read(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var_start)
{
	ix_msg_read_file(pclt, pmsg);
}

#else
// This section has code actually used in RSU.

// Writes approach values from an ix_approach_t structure
void ix_approach_update(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var_start)
{
	int i;
	ix_stop_line_t *pstop = &papp->stop_line_array[0];
	unsigned char nstop = papp->number_of_stop_lines;
	int approach_size = sizeof(ix_approach_t) - sizeof(pstop) + 
				nstop * sizeof(ix_stop_line_t);
	unsigned char tmp_buf[approach_size];
	int db_var = ix_approach_var_start + approach_num;
	unsigned char *cur = tmp_buf;
	memcpy(tmp_buf, papp, sizeof(ix_approach_t) - sizeof(pstop));
	cur += sizeof(ix_approach_t) - sizeof(pstop);
	for (i = 0; i < nstop; i++) {
		memcpy(cur, &pstop[i], sizeof(ix_stop_line_t));
		cur += sizeof(ix_stop_line_t);
#ifdef DEBUG_STOP
		printf("ix_approach_update: lat %d long %d line length %hd orient %hd\n", pstop[i].latitude, pstop[i].longitude, pstop[i].line_length, pstop[i].orientation);
		printf("cur-tmp_buf %d, sizeof(ix_approach_t) %d, sizeof(ix_stop_line_t) %d\n", cur-tmp_buf, sizeof(ix_approach_t), sizeof(ix_stop_line_t));
#endif
	}
	db_clt_write(pclt, db_var, cur-tmp_buf, tmp_buf); 
}

// Reads approach values from an IX_APPROACH database variable 
// The database variable has flattened the structure to hold a
// variable number of stop lines, while the ix_approach_t structure
// will contain a variable to the malloced number of stop lines.
void ix_approach_read(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var_start)
{
	int i;
	ix_stop_line_t *pstop;	// malloc pointer to stop array
	unsigned char nstop;	// number of stop lines
	// array must be big enough to hold maximum number of stop lines
	unsigned char tmp_buf[MAX_APPROACH_SIZE];
	int db_var = ix_approach_var_start + approach_num;
	unsigned char *cur = tmp_buf;

	db_clt_read(pclt, db_var, MAX_APPROACH_SIZE, tmp_buf); 

	memcpy(papp, tmp_buf, sizeof(ix_approach_t) - IX_POINTER_SIZE);
	cur += sizeof(ix_approach_t) - sizeof(pstop);

	nstop = papp->number_of_stop_lines;
	pstop = (ix_stop_line_t *) malloc(sizeof(ix_stop_line_t) * nstop);
	papp->stop_line_array = pstop;

	for (i = 0; i < nstop; i++) {
		memcpy(&pstop[i], cur,  sizeof(ix_stop_line_t));
		cur += sizeof(ix_stop_line_t);
#ifdef DEBUG_STOP
		printf("ix_approach_read: lat %d long %d line length %hd orient %hd\n", pstop[i].latitude, pstop[i].longitude, pstop[i].line_length, pstop[i].orientation);
		printf("cur %d\n", cur);
#endif

	}
}

// Writes intersection message and approach variables from pmsg structure
void ix_msg_update(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var_start)
{
	int i;
	db_clt_write(pclt, ix_db_var, 
		(sizeof(ix_msg_t) - IX_POINTER_SIZE), pmsg);
	for (i = 0; i < pmsg->num_approaches; i++) 
		ix_approach_update(pclt, &pmsg->approach_array[i],
					i, ix_approach_var_start);
}

// Reads the specified database variables into the pmsg pointer
// Assumes base pmsg structure has already been malloced.
// Need to call ix_msg_free when done with it.
void ix_msg_read(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var_start)
{
	int i;
	struct timespec trig_time;
	db_clt_read(pclt, ix_db_var,	
		(sizeof(ix_msg_t) - IX_POINTER_SIZE), pmsg);

	pmsg->approach_array = (ix_approach_t *) malloc(
					sizeof (ix_approach_t) *
					pmsg->num_approaches);
	for (i = 0; i < pmsg->num_approaches; i++) 
		ix_approach_read(pclt, &pmsg->approach_array[i],
					i, ix_approach_var_start);
}
#endif //TEST_ONLY
	
