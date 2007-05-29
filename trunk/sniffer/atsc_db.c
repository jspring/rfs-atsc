// Link in this routine to write atsc_t to database from digio.
// Must be linked with site-specific .o that sets up mapping in nema_asg.
#include <sys_os.h>
#include "sys_rt.h"
#include "timestamp.h"
#include "db_clt.h"
#include "db_utils.h"
#include "clt_vars.h"
#include "atsc.h"
#include "nema_asg.h"

#undef DEBUG_DBV
#undef DEBUG_DBV_VERBOSE

int write_atsc_db_var(db_clt_typ *pclt, unsigned char digio_byte)
{
	atsc_typ current_atsc;
	int bit_set = 0;
	int i;

	//initialize all fields to 0
	memset(&current_atsc, 0, sizeof(current_atsc));
#ifdef DEBUG_DBV
	printf("byte 0x%02x num_bits %d \n", digio_byte, num_bits_to_nema);
#endif
	for (i = 0; i < num_bits_to_nema; i++) {
		bit_to_nema_phase_t *p = &nema_asg[i];
		// 0 means on
		if (digio_byte & (1 << i))	
			bit_set = 0;
		else
			bit_set = 1;
#ifdef DEBUG_DBV_VERBOSE
		printf("NEMA %d %c %d\n", i, p->phase_color, 
				p->phase_number);
		printf("i %d digio_byte 0x%2hhx, bit_set %d\n", i,
				digio_byte, bit_set);
#endif
		if (bit_set) {
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
	db_clt_write(pclt, DB_ATSC_VAR, sizeof(atsc_typ), &current_atsc);
}

