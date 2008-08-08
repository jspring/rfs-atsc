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

int write_atsc_db_var(db_clt_typ *pclt, unsigned char *buf)
{
	atsc_typ current_atsc;
	int bit_set = 0;
	timestamp_t ts;
	static timestamp_t last_ts;
	int i;

	//initialize all fields to 0
	memset(&current_atsc, 0, sizeof(current_atsc));
/// Figure out phase number and phase color from message buffer
#ifdef DEBUG_DBV
			printf("phase %d %c\n", 
				p->phase_number, p->phase_color);
#endif
			switch(p->phase_color) {
			case 'G':
			    set_atsc_phase(current_atsc.phase_status_greens,
					p->phase_number);
			    break;
			case 'R':
			    set_atsc_phase(current_atsc.phase_status_reds, 
					p->phase_number);
			    break;
			case 'Y':
			    set_atsc_phase(current_atsc.phase_status_yellows, 
					p->phase_number);
			    break;
			case 'N':	// not assigned
			default:
			    break;
			}
		}
	}
	current_atsc.info_source = ATSC_SOURCE_SNIFF;
#ifdef DEBUG_DBV
	printf("db write: reds %02hhx yellows %02hhx, greens %02hhx \n",
		current_atsc.phase_status_reds[0],
		current_atsc.phase_status_yellows[0],
		current_atsc.phase_status_greens[0]);
	fflush(stdout);
#endif
	if (current_atsc.phase_status_greens[0] != last_greens) {
		timestamp_t elapsed_ts;
		db_clt_write(pclt, DB_ATSC_VAR, sizeof(atsc_typ),
				 &current_atsc);
#ifdef DEBUG_TRIG
		get_current_timestamp(&ts);
		print_timestamp(stdout, &ts);
		decrement_timestamp(&ts, &last_ts, &elapsed_ts);
		last_ts = ts;	
		printf(" current: 0x%2hhx last: 0x%2hhx elapsed %.3f\n",
			 current_atsc.phase_status_greens[0], last_greens,
			 TS_TO_MS(&elapsed_ts)/1000.0);
#endif
		last_greens = current_atsc.phase_status_greens[0];
	}
}

