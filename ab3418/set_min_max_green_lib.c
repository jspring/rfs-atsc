/* set_min_max_green_lib.c - contains function call for setting URMS parameters
**
*/

#include <db_include.h>
#include "ab3418comm.h"

int db_set_phase3_max_green1(db_clt_typ *pclt, int max_green, int verbose) {

        db_2070_timing_set_t db_2070_timing_set;
	int retval = -1;

        db_2070_timing_set.phase = 3;
	memset(&db_2070_timing_set.cell_addr_data, 0, sizeof(db_2070_timing_set.cell_addr_data));
        if(max_green > 0) {
	        db_2070_timing_set.cell_addr_data.cell_addr = (unsigned short) MAXIMUM_GREEN_1;
        	db_2070_timing_set.cell_addr_data.data = max_green;
	}
	if(verbose) {
		printf("Writing: phase %d min_green %d\n",
			db_2070_timing_set.phase,
			db_2070_timing_set.cell_addr_data.data
		);
		printf("Writing: cell0 addr %#x\n",
			db_2070_timing_set.cell_addr_data.cell_addr
		);
	}
        db_clt_write(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t), &db_2070_timing_set);
	if(retval == FALSE) {
		fprintf(stderr, "db_set_min_max_green: db_clt_write failed\n");
		return -1;
	}
	memset(&db_2070_timing_set, 0, sizeof(db_2070_timing_set_t));
        db_clt_read(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t), &db_2070_timing_set);
	if(verbose) {
		printf("Reading: phase %d max_green %d\n",
			db_2070_timing_set.phase,
			db_2070_timing_set.cell_addr_data.data
		);
		printf("Reading: cell addr %#x\n",
			db_2070_timing_set.cell_addr_data.cell_addr
		);
	}
	return 0;
}

int db_get_timing_request(db_clt_typ *pclt, unsigned char phase, unsigned short page) {

        db_2070_timing_get_t db_2070_timing_get;
	int retval = -1;

        db_2070_timing_get.phase = phase;
        db_2070_timing_get.page = page;
        printf("Writing: phase %d page %d\n",
                db_2070_timing_get.phase,
        	db_2070_timing_get.page
                );
        db_clt_write(pclt, DB_2070_TIMING_GET_VAR, sizeof(db_2070_timing_get_t), &db_2070_timing_get);
	if(retval == FALSE) {
		printf("db_get_min_max_green: db_clt_write failed\n");
		return -1;
	}
        db_clt_read(pclt, DB_2070_TIMING_GET_VAR, sizeof(db_2070_timing_get_t), &db_2070_timing_get);
        printf("Reading: phase %d page %d\n",
                db_2070_timing_get.phase,
        	db_2070_timing_get.page
                );
	return 0;
}
