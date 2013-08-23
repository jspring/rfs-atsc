#ifndef SET_MIN_MAX_GREEN_H
#define SET_MIN_MAX_GREEN_H

#include "msgs.h"

#define DB_2070_TIMING_SET_TYPE     5501
#define DB_2070_TIMING_SET_VAR      DB_2070_TIMING_SET_TYPE
#define DB_2070_TIMING_GET_TYPE     5502
#define DB_2070_TIMING_GET_VAR      DB_2070_TIMING_GET_TYPE

// Cell Addresses of various timing parameters
// These are the addresses for phase 1 parameters; 
// for phase N, change the second numeral to N.  So
// for "Flash Dont Walk" phase 2, change the cell address
// from 0x111 to 0x121. 
#define	WALK_1  	0x0110
#define FLASH_DONT_WALK 0x0111
#define MINIMUM_GREEN   0x0112
#define DETECTOR_LIMIT  0x0113
#define ADD_PER_VEH     0x0114
#define EXTENSION       0x0115
#define MAXIMUM_GAP     0x0116
#define MINIMUM_GAP     0x0117
#define MAXIMUM_GREEN_1 0x0118
#define MAXIMUM_GREEN_2 0x0119
#define MAXIMUM_GREEN_3 0x011a
#define MAXIMUM_INITIAL 0x011b
#define REDUCE_GAP_BY   0x011c
#define REDUCE_EVERY    0x011d
#define YELLOW          0x011e
#define ALL_RED		0x011f

#define	CYCLE_LENGTH	0x0310 
#define GREEN_FACTOR_1	0x0311 
#define GREEN_FACTOR_2	0x0312 
#define GREEN_FACTOR_3	0x0313 
#define GREEN_FACTOR_4	0x0314 
#define GREEN_FACTOR_5	0x0315 
#define GREEN_FACTOR_6	0x0316 
#define GREEN_FACTOR_7	0x0317 
#define GREEN_FACTOR_8	0x0318 
#define MULTIPLIER	0x0319 
#define PLAN_A          0x031a 
#define OFFSET_B        0x031b 
#define NO_LABEL_C      0x031c 

#define PAGE_TIMING	0X100
#define PAGE_LOCAL_PLAN	0X300

typedef struct {
	int message_type;	//Set to 0x1234
	int phase;			// phase
	int max_green;
	int min_green;
	int yellow;
	int all_red;
	cell_addr_data_t cell_addr_data[4];	// Determines parameters to change
} IS_PACKED db_2070_timing_set_t;


typedef struct {
	int message_type;	//Set to 0x1235
	int phase;			// phase
	unsigned short page;	// timing settings=0x100, local plan settings=0x300
} IS_PACKED db_2070_timing_get_t;

int db_set_min_max_green(db_clt_typ *pclt, int phase, int min_green, int max_green, 
	int yellow, int all_red, int verbose);
int db_get_timing_request(db_clt_typ *pclt, int phase, unsigned short page);

#endif
