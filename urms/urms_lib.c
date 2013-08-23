/* urms_lib.c - contains definition of db_set_ramp_metering, the database
** function for setting URMS parameters
**
*/

#include "urms.h"

int db_set_ramp_metering(db_clt_typ *pclt, db_urms_t *db_urms);


int db_set_ramp_metering(db_clt_typ *pclt, db_urms_t *db_urms) {

	int retval;

	retval = db_clt_write(pclt, DB_URMS_VAR, sizeof(db_urms_t), db_urms);
	if(retval == FALSE) {
		printf("db_set_ramp_metering: db_clt_write failed\n");
		return -1;
	}
	return 0;
}
