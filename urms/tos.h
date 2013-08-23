//proposed communication packet description
#include <db_include.h>

#ifndef TOS_H
#define TOS_H

#define BYTE	unsigned char
#define BOOL	unsigned char

#define	MAX_PEAK_PERIODS   2
#define	MAX_RMETERS        4
#define	MAX_HOLIDAYS      16
#define	MAX_TOD_ENTRIES   16
#define	MAX_PLAN_TABLES    6
#define	MAX_PLAN_ENTRIES  16

#define CODE_GET_DATE		c
typedef struct {
        char	start_flag;   // 0x7e
        char	address;      // 0x05 2070 controller
        char	comm_code;    // c = get clock/calendar
        char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
        char	FCSlsb;       /* FCS least significant byte */
        char	end_flag;     /* 0x7e */
} IS_PACKED tos_get_date_request_msg_t;

#define CODE_GET_DETECTOR	d
typedef struct {
        char	start_flag;   // 0x7e
        char	address;      // 0x05 2070 controller
        char	comm_code;    // d = get detector data (both RM and MLM)
        char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
        char	FCSlsb;       /* FCS least significant byte */
        char	end_flag;     /* 0x7e */
} IS_PACKED tos_get_detector_request_msg_t;

#define CODE_SET_ACTION		D
typedef struct {
        char	start_flag;   // 0x7e
        char	address;      // 0x05 2070 controller
        char	comm_code;    // d = get detector data (both RM and MLM)
        char	action[4];      // Array of actions; depends on number of configured 
			      // metered lanes. 0=skip,0x15=150 VPHPL
        char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
        char	FCSlsb;       /* FCS least significant byte */
        char	end_flag;     /* 0x7e */
} IS_PACKED tos_set_action_t;

#define CODE_ENABLE_DETECTORS	E
typedef struct {
        char	start_flag;   // 0x7e
        char	address;      // 0x05 2070 controller
        char	comm_code;    // E = set detector enables (both RM and MLM)
	char	station_type; // mainline=1, ramp=2
	char	first_logical_lane; //If setting just one lane, 
			      // set first_logical_lane 
	char	num_lanes;
        char	det_enable[8];// Array of detector enables
			      // Mainline
			      // DISABLED = 1, SINGLE_LEAD, SINGLE_TRAIL, DUAL
			      // Metered lane
			      // RECALL = 0, ENABLE, RED LOCK
        char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
        char	FCSlsb;       /* FCS least significant byte */
        char	end_flag;     /* 0x7e */
} IS_PACKED tos_enable_detectors_t;

int tos_get_detector_request(int fd, int verbose);

#endif
