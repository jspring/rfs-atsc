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

int write_atsc_db_var(db_clt_typ *pclt, unsigned char ediarr[]) {
	atsc_typ current_atsc;
	int i;
	unsigned char send_db_var = 0;
	static unsigned char last_greens[16] = {0};
	static unsigned char last_yellows[16] = {0};

	//initialize all fields to 0
	memset(&current_atsc, 0, sizeof(current_atsc));

	/// Figure out phase number and phase color from message buffer
#ifdef DEBUG_DBV
	printf("phase %d %c\n", p->phase_number, p->phase_color);
#endif

#define GREEN_OFFSET	1
#define RED_OFFSET	17
#define YELLOW_OFFSET	33

	send_db_var = 0;
	for( i = 0; i < 16 ; i++) {
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
#ifdef DEBUG_DBV
	printf("db write: reds %02hhx yellows %02hhx, greens %02hhx \n",
		current_atsc.phase_status_reds[0],
		current_atsc.phase_status_yellows[0],
		current_atsc.phase_status_greens[0]);
	fflush(stdout);
#endif
	if(send_db_var){
		db_clt_write(pclt, DB_ATSC_VAR, sizeof(atsc_typ), &current_atsc);
		printf("Writing atsc db var\n");
		printf("db write: reds %02hhx yellows %02hhx, greens %02hhx \n",
			current_atsc.phase_status_reds[1],
			current_atsc.phase_status_yellows[1],
			current_atsc.phase_status_greens[1]);
		printf("db write: reds %02hhx yellows %02hhx, greens %02hhx \n",
			current_atsc.phase_status_reds[3],
			current_atsc.phase_status_yellows[3],
			current_atsc.phase_status_greens[3]);
		fflush(stdout);
	}
#ifdef DEBUG_TRIG
	get_current_timestamp(&ts);
	print_timestamp(stdout, &ts);
	decrement_timestamp(&ts, &last_ts, &elapsed_ts);
	last_ts = ts;	
	printf(" current: 0x%2hhx last: 0x%2hhx elapsed %.3f\n",
		current_atsc.phase_status_greens[0], last_greens,
		TS_TO_MS(&elapsed_ts)/1000.0);
#endif
	return 0;
}

