/**\file
 *      Writes ATSC variable to the database, after parsing information
 *	out of buffer. 
 *
 * (C) Copyright University of California 2006.  All rights reserved.
 *
 */

#include <sys_os.h>
#include "sys_rt.h"
#include "timestamp.h"
#include "db_clt.h"
#include "db_utils.h"
#include "atsc_clt_vars.h"
#include "atsc.h"

int write_atsc_db_var(db_clt_typ *pclt, unsigned char ediarr[], unsigned char verbose) {
	atsc_typ current_atsc;
	int i;
	unsigned char send_db_var = 0;
	static unsigned char last_greens[MAX_PHASE_STATUS_GROUPS] = {0};
	static unsigned char last_yellows[MAX_PHASE_STATUS_GROUPS] = {0};
        timestamp_t elapsed_ts;
        timestamp_t ts;
        static timestamp_t last_ts;

	//initialize all fields to 0
	memset(&current_atsc, 0, sizeof(current_atsc));
	current_atsc.info_source = ATSC_SOURCE_EDI;

#define GREEN_OFFSET	1
#define YELLOW_OFFSET	17
#define RED_OFFSET	33

	send_db_var = 0;
	for( i = 0; i < MAX_PHASE_STATUS_GROUPS; i++) {
	    current_atsc.phase_status_greens[i] = ediarr[i + GREEN_OFFSET];
	    current_atsc.phase_status_reds[i] = ediarr[i + RED_OFFSET];
	    current_atsc.phase_status_yellows[i] = ediarr[i + YELLOW_OFFSET];
	    if ( (current_atsc.phase_status_greens[i] != last_greens[i]) ||
	       (current_atsc.phase_status_yellows[i] != last_yellows[i]) )
			send_db_var = 1;
	    last_greens[i] = current_atsc.phase_status_greens[i];
	    last_yellows[i] = current_atsc.phase_status_yellows[i];
	}
	current_atsc.info_source = ATSC_SOURCE_SNIFF;

	if(send_db_var){
		db_clt_write(pclt, DB_ATSC_VAR, sizeof(atsc_typ), &current_atsc);
	   if(verbose) {
		printf("Writing atsc db var\n");
		printf("db write phase 2: reds %02hhx yellows %02hhx, greens %02hhx \n",
			current_atsc.phase_status_reds[1],
			current_atsc.phase_status_yellows[1],
			current_atsc.phase_status_greens[1]);
		printf("db write phase 4: reds %02hhx yellows %02hhx, greens %02hhx \n",
			current_atsc.phase_status_reds[3],
			current_atsc.phase_status_yellows[3],
			current_atsc.phase_status_greens[3]);
		fflush(stdout);
		get_current_timestamp(&ts);
		print_timestamp(stdout, &ts);
		decrement_timestamp(&ts, &last_ts, &elapsed_ts);
		last_ts = ts;	
		printf("elapsed %.3f\n", TS_TO_MS(&elapsed_ts)/1000.0);
	   }
	}
	return 0;
}

