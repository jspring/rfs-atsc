/* set_min_max_green_lib.c - contains function call for setting URMS parameters
**
*/

#include <db_include.h>
#include "set_min_max_green.h"

int db_set_min_max_green(db_clt_typ *pclt, int phase, int min_green, int max_green, int yellow, int all_red, int verbose) {

        db_2070_timing_set_t db_2070_timing_set;
	int retval = -1;

	if(phase <=0 ) {
		fprintf(stderr, "phase argument to db_set_min_max_green must be > 0\n");
		return retval;
	}
        db_2070_timing_set.phase = phase;
	memset(&db_2070_timing_set.cell_addr_data[0], 0, sizeof(db_2070_timing_set.cell_addr_data));
        if(min_green >= 0) {
		db_2070_timing_set.cell_addr_data[0].cell_addr = (unsigned short) MINIMUM_GREEN;
	        db_2070_timing_set.cell_addr_data[0].data = min_green;
	}
        if(max_green >= 0) {
	        db_2070_timing_set.cell_addr_data[1].cell_addr = (unsigned short) MAXIMUM_GREEN_1;
        	db_2070_timing_set.cell_addr_data[1].data = max_green;
	}
        if(yellow >= 0) {
        	db_2070_timing_set.cell_addr_data[2].cell_addr = (unsigned short) YELLOW;
        	db_2070_timing_set.cell_addr_data[2].data = yellow;
	}
        if(all_red >= 0) {
        	db_2070_timing_set.cell_addr_data[3].cell_addr = (unsigned short) ALL_RED;
        	db_2070_timing_set.cell_addr_data[3].data = all_red;
	}
	if(verbose) {
		printf("Writing: phase %d min_green %d max_green %d yellow %d all_red %d\n",
			db_2070_timing_set.phase,
			db_2070_timing_set.cell_addr_data[0].data,
			db_2070_timing_set.cell_addr_data[1].data,
			db_2070_timing_set.cell_addr_data[2].data,
			db_2070_timing_set.cell_addr_data[3].data
		);
		printf("Writing: cell0 addr %#x cell1 addr %#x cell2 addr %#x cell3 addr %#x \n",
			db_2070_timing_set.cell_addr_data[0].cell_addr,
			db_2070_timing_set.cell_addr_data[1].cell_addr,
			db_2070_timing_set.cell_addr_data[2].cell_addr,
			db_2070_timing_set.cell_addr_data[3].cell_addr
		);
	}
        db_clt_write(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t), &db_2070_timing_set);
	if(retval == FALSE) {
		printf(stderr, "db_set_min_max_green: db_clt_write failed\n");
		return -1;
	}
	memset(&db_2070_timing_set, 0, sizeof(db_2070_timing_set_t));
        db_clt_read(pclt, DB_2070_TIMING_SET_VAR, sizeof(db_2070_timing_set_t), &db_2070_timing_set);
	if(verbose) {
		printf("Reading: phase %d min_green %d max_green %d yellow %d all_red %d\n",
			db_2070_timing_set.phase,
			db_2070_timing_set.cell_addr_data[0].data,
			db_2070_timing_set.cell_addr_data[1].data,
			db_2070_timing_set.cell_addr_data[2].data,
			db_2070_timing_set.cell_addr_data[3].data
		);
		printf("Reading: cell0 addr %#x cell1 addr %#x cell2 addr %#x cell3 addr %#x \n",
			db_2070_timing_set.cell_addr_data[0].cell_addr,
			db_2070_timing_set.cell_addr_data[1].cell_addr,
			db_2070_timing_set.cell_addr_data[2].cell_addr,
			db_2070_timing_set.cell_addr_data[3].cell_addr
		);
	}
	return 0;
}

int db_get_timing_request(db_clt_typ *pclt, int phase, unsigned short page) {

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
