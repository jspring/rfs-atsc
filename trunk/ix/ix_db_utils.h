/**\file
 * 	Header file for routines that read or write intersection
 *	message sets from/to the PATH "data bucket" (a process, db_slv
 *	handles interprocess communication with simple in-memory
 *	publish/subscribe semantics).
 *
 *	Kept separate because code samples for customer testing may not
 *	want to include the database.
 *
 *	The corresponding functions in ix_file_utils.[ch] are used for this
 *	test only situation, substituting file reads and writes for the
 *	data bucket client log in and reads and writes.
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */
#include "ix_msg.h"
#ifdef TEST_ONLY
#include "ix_test_utils.h"
#else
#include "db_clt.h"
#include "db_utils.h"
#include "clt_vars.h"
#endif

// Writes approach values from an ix_approach_t structure
// ix_approach_var is the database variable number of the 
// first approach database variable
extern void ix_approach_update(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var);

// Reads approach values from an IX_APPROACH database variable 
extern void ix_approach_read(db_clt_typ *pclt, ix_approach_t * papp, 
				int approach_num, int ix_approach_var);

// Updates the database variables as specified from the structure pmsg
// May be different variable numbers on transmit and receive
extern void ix_msg_update(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var);

// Reads the specified database variables into the pmsg structure
extern void ix_msg_read(db_clt_typ *pclt, ix_msg_t *pmsg, int ix_db_var,
		int ix_approach_var);

extern void ix_approach_update_file(FILE *fp, ix_approach_t *papp);
extern void ix_approach_read_file(FILE *fp, ix_approach_t *papp);
extern void ix_msg_update_file(FILE *fp, ix_msg_t *pmsg);
extern void ix_msg_read_file(FILE *fp, ix_msg_t *pmsg);


