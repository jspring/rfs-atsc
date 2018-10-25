/* FILE:  msgs.h  Header file for AB3418 format messages
 *
 * Copyright (c) 2012  Regents of the University of California
 *
 */

#ifndef MSGS_H
#define MSGS_H
#define DB_AB3418_CMD_TYPE	2222
#define DB_AB3418_CMD_VAR	DB_AB3418_CMD_TYPE

typedef struct {

	char	start_flag;	/* 0x7e */
	char	address;	/* 0x05 2070 controller */
	char	control;	/* 0x13 - unnumbered information, individual address */
	char	ipi;		/* 0xc0 - NTCIP Class B Protocol */
} ab3418_hdr_t;

typedef struct {

	ab3418_hdr_t ab3428_hdr;
	unsigned char msg_request;
	

} ab3418_frame_t;

/* Following is format for GetLongStatus8 message. */

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char      start_flag;   /* 0x7e */
	char      address;      /* 0x05 2070 controller */
	char      control;      /* 0x13 - unnumbered information, individual address */
	char      ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char      mess_type;    /* 0x84 - get short status */
	char      FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char      FCSlsb;       /* FCS least significant byte */
	char      end_flag;     /* 0x7e */
} IS_PACKED get_short_status_request_t;

/* Message sent from 2070 to RSU has the following
 * format. */
typedef struct
{
	char          start_flag;   /* 0x7e */
	char          address;      /* 0x05 Requester PC */
	char          control;      /* 0x13 - unnumbered information, individual address */
	char          ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char          mess_type;    /* 0xc4 - get short status response */
	unsigned char greens;
	unsigned char status;       /* Bit 7 = critical alarm; bit 6 = non-critical alarm;
	                             * bit 5 = detector fault; bit 4 = coordination alarm;
	                             * bit 3 = local override; bit 2 = passed local zero;
	                             * bit 1 = cabinet flash; bit 0 = preempt. */
	unsigned char pattern;      /* Pattern (0-250, 251-253 reserved, 254 flash, 255 free) */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED get_short_status_resp_t;

typedef struct
{
	char      start_flag;   /* 0x7e */
	char      address;      /* 0x05 2070 controller */
	char      control;      /* 0x13 - unnumbered information, individual address */
	char      ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char      mess_type;    /* 0x8c - get long status8 */
	char      FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char      FCSlsb;       /* FCS least significant byte */
	char      end_flag;     /* 0x7e */
} IS_PACKED get_long_status8_mess_typ;

/* Message sent from 2070 to RSU has the following
 * format. */
typedef struct
{
	char          start_flag;   /* 0x7e */
	char          address;      /* Byte 2 0x05 Requester PC */
	char          control;      /* Byte 3 0x13 - unnumbered information, individual address */
	char          ipi;          /* Byte 4 0xc0 - NTCIP Class B Protocol */
	char          mess_type;    /* Byte 5 0xcc - get long status8 response */
	unsigned char flags;        /* Byte 6 Additional flags; bit 0:focus (default 0 = no focus),
	                             * bits 1-7: reserved, unused. */
	unsigned char status;       /* Byte 7 Bit 7 = critical alarm; bit 6 = non-critical alarm;
	                             * bit 5 = detector fault; bit 4 = coordination alarm;
	                             * bit 3 = local override; bit 2 = passed local zero;
	                             * bit 1 = cabinet flash; bit 0 = preempt. */
	unsigned char pattern;      /* Byte 8 Pattern (0-250, 251-253 reserved, 254 flash, 255 free) */
	unsigned char green_yellow_overlap; /* Byte 9 Bits 0-3 green overlaps A-D;
	                              * bits 4-7 yellow overlaps A-D */
	unsigned char preemption;    /* Byte 10 Bits 0-1 EV A-D; bits 4-6 RR 1-2;
	                              * bit 6 = pattern transition; bit 7 unused */
	unsigned char phase_call;    /* Byte 11 Phase call 1-8; (bit 7 = phase 8; bit 0 = phase 1) */
	unsigned char ped_call;      /* Byte 12 Ped call 1-8; (bit 7 = ped 8; bit 0 = ped 1) */
	unsigned char active_phase;  /* Byte 13 Bits 0-7 -> phases 1-8; bit set true for phase active */
	unsigned char interval;      /* Byte 14 Bits 0-3: ring 0 interval; bits 4-7: ring 1 interval.
	                              * Interval encoding is as follows:
                                      * 0X00 = walk, 0x01 = don't walk, 0x02 = min green,
	                              * 0x03 = unused, 0x04 = added initial, 0x05 = passage -resting,
	                              * 0x06 = max gap, 0x07 = min gap, 0x08 = red rest,
	                              * 0x09 = preemption, 0x0a = stop time, 0x0b = red revert,
	                              * 0x0c = max termination, 0x0d = gap termination,
	                              * 0x0e = force off, 0x0f = red clearance */
	unsigned char presence1;     /* Byte 15 Bits 0-7: detector 1-8. Presence bits set true for
	                              * positive presence. */
	unsigned char presence2;     /* Byte 16 Bits 0-7: detector 9-16 */
	unsigned char presence3;     /* Byte 17 Bits 0-7: detector 17-24 */
	unsigned char presence4;     /* Byte 18 Bits 0-3: detector 25-28, bits 4-7 unused */
	unsigned char master_clock;  /* Byte 19 Master background cycle clock.  Counts up to cycle length */
	unsigned char local_clock;   /* Byte 20 Local cycle clock.  Counts up to cycle length. */
	unsigned char seq_number;    /* Byte 21 Sample sequence number */
	unsigned char volume1;       /* Byte 22 System detector 1 */
	unsigned char occupancy1;    /* Byte 23 System detector 1.  Value 0-200 = detector occupancy in
	                              * 0.5% increments, 201-209 = reserved, 210 = stuck ON fault,
	                              * 211 = stuck OFF fault, 212 = open loop fault,
	                              * 213 = shorted loop fault, 214 = excessive inductance fault,
	                              * 215 = overcount fault. */
	unsigned char volume2;       /* Byte 24 System detector 2 */
	unsigned char occupancy2;    /* Byte 25 system detector 2 */
	unsigned char volume3;       /* Byte 26 System detector 3 */
	unsigned char occupancy3;    /* Byte 27 system detector 3 */
	unsigned char volume4;       /* Byte 28 System detector 4 */
	unsigned char occupancy4;    /* Byte 29 system detector 4 */
	unsigned char volume5;       /* Byte 30 System detector 5 */
	unsigned char occupancy5;    /* Byte 31 system detector 5 */
	unsigned char volume6;       /* Byte 32 System detector 6 */
	unsigned char occupancy6;    /* Byte 33 system detector 6 */
	unsigned char volume7;       /* Byte 34 System detector 7 */
	unsigned char occupancy7;    /* Byte 35 system detector 7 */
	unsigned char volume8;       /* Byte 36 System detector 8 */
	unsigned char occupancy8;    /* Byte 37 system detector 8 */
	unsigned char FCSmsb;        /* Byte 38 FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* Byte 39 FCS least significant byte */
	char          end_flag;      /* Byte 40 0x7e */
} IS_PACKED get_long_status8_resp_mess_typ;

typedef struct {
	unsigned short cell_addr;
	unsigned char data;
} IS_PACKED cell_addr_data_t;

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x99 - set controller timing data*/
	char	num_cells;    /* 0x02 - number of cells to set */
        cell_addr_data_t magic_num;     // Magic number (=0x0192) that James 
                                        // Lau gave me. I still don't know what
                                        // it means, but it's necessary to get 
                                        // the parameters to change.
	cell_addr_data_t cell_addr_data; // Allow 1 parameter change at a time 
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED set_controller_timing_data_t;

/* Message sent from RSU to 2070 controller
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x89 - get controller timing data*/
	unsigned short offset;    /* Start address - 0x110 for timing settings, 0x310 for local plan settings*/
	char	num_bytes;    /* Number of bytes to get; 32 is the max, set to 16 for timing or plan settings for a single phase  */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_controller_timing_data_request_t;

typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x89 - get controller timing data*/
	char	page;         /* Page number */
	char	block;        /* Block number */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_block_request_t;

/* Message sent from 2070 to RSU
 * has the following format. */
typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0xc9 - controller timing data response code*/
	unsigned short offset;    /* Start address - 0x110 for timing settings, 0x310 for local plan settings*/
	char	num_bytes;    /* Number of bytes received ; 32 is the max */
	char	data[16];     /* Data */
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED get_controller_timing_data_response_t;

typedef struct
{
	/* Set for maximum length of expanded message after converting
	 * any 0x7e to the combo 0x7d, 0x5e. */
	unsigned char    data[200];
} gen_mess_typ;

typedef struct
{
	char	start_flag;	/* 0x7e */
	char	address;	/* 0x05 2070 controller */
	char	control;	/* 0x13 - unnumbered information, individual address */
	char	ipi;		/* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;	/* 0x92 - set time request */
	char	day_of_week;	/* Day of week (1-7) 1=Sunday */	
	char	month;		/* Month (1-12) 1=January */
	char	day_of_month;	/* Day of month (1-31) */
	char	year;		/* Last two digits of year (00-99) 95=1995, 0=2000, 94=2094 */
	char 	hour;		/* Hour (0-23) */
	char 	minute;		/* Minute (0-59) */
	char 	second;		/* Second (0-59) */
	char 	tenths;		/* Tenth second (0-9) */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED set_time_t;

typedef struct {
	char	start_flag;	/* 0x7e */
	char	address;	/* 0x05 2070 controller */
	char	control;	/* 0x13 - unnumbered information, individual address */
	char	ipi;		/* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;	/* 0x93 - set pattern request */
	unsigned char	pattern;	/* Pattern number (0-255); 0 Standby, 251-253 reserved, 254 Flash, 255 Free */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED set_pattern_t;


/*************************************************************************************************
TSMSS Support for TSCP

05/20/2013 - JAS

TSMSS GET request, GET response, and SET request messages for different types of data, all have
the same message code - 0x87=GET request, 0xC7=GET response, 0x96=SET request. The payload 
type is differentiated by the page and block IDs.
*************************************************************************************************/
typedef struct {
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller drop address (0x05 = address #1)*/
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0x87=GET request, 0xC7=GET response, 0x96=SET request overlap flags code*/
	char	page_id;	// 0x02 - overlap Page ID
	char	block_id;	// 0x04 - overlap Block ID
} IS_PACKED tsmss_get_set_hdr_t;

typedef struct {
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} IS_PACKED tsmss_get_set_tail_t;

typedef struct {
	tsmss_get_set_hdr_t get_hdr; //get_hdr.mess_type = 0x87, page & block ID - see manual
	tsmss_get_set_tail_t get_tail;
} IS_PACKED tsmss_get_msg_request_t;

typedef struct {
	tsmss_get_set_hdr_t special_flags_hdr ; // special_flags_hdr.mess_type=0xC7 GET response
					 // special_flags_hdr.mess_type=0x96 SET request
					 // special_flags_hdr.page_id=0x02
					 // special_flags_hdr.block_id=0x02

	unsigned char call_to_phase_1;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_2;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_3;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_4;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_5;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_6;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_7;	// Bits 0-7 <-> phases 1-8
	unsigned char call_to_phase_8;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_1;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_2;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_3;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_4;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_5;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_6;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_7;	// Bits 0-7 <-> phases 1-8
	unsigned char omit_on_green_8;	// Bits 0-7 <-> phases 1-8
	unsigned char yellow_flash_phases; // Bits 0-7 <-> phases 1-8
	unsigned char yellow_flash_overlaps; // Bits 0-7 <-> overlaps A-F
	unsigned char flash_in_red_phases; // Bits 0-7 <-> phases 1-8
	unsigned char flash_in_red_overlaps; // Bits 0-7 <-> overlapss A-F
	unsigned char single_exit_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char driveway_signal_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char driveway_signal_overlaps;	// Bits 0-7 <-> overlaps A-F
	unsigned char leading_ped_phases;	// Bits 0-7 <-> phases 1-8
	unsigned char protected_permissive;	//
	unsigned char cabinet_type;		//
	unsigned char cabinet_config;		//

	tsmss_get_set_tail_t special_flags_tail;

} IS_PACKED get_set_special_flags_t;

typedef struct {
	tsmss_get_set_hdr_t overlap_hdr; // overlap_hdr.mess_type=0xC7 GET response
					 // overlap_hdr.mess_type=0x96 SET request
					 // overlap_hdr.page_id=0x02
					 // overlap_hdr.block_id=0x04
	char	overlapA_parent;  //Overlap A – On With Phases bits 0-7 = phases 1-8
	char	overlapA_omit;	  //Overlap A – Omit Phases bits 0-7 = phases 1-8
	char	overlapA_nostart; //Overlap A – No Start Phases bits 0-7 = phases 1-8
	char	overlapA_not;	  //Overlap A – Not On With Phases bits 0-7 = phases 1-8
	char	overlapB_parent;  //Overlap B – On With Phases bits 0-7 = phases 1-8
	char	overlapB_omit;	  //Overlap B – Omit Phases bits 0-7 = phases 1-8
	char	overlapB_nostart; //Overlap B – No Start Phases bits 0-7 = phases 1-8
	char	overlapB_not;	  //Overlap B – Not On With Phases bits 0-7 = phases 1-8
	char	overlapC_parent;  //Overlap C – On With Phases bits 0-7 = phases 1-8
	char	overlapC_omit;	  //Overlap C – Omit Phases bits 0-7 = phases 1-8
	char	overlapC_nostart; //Overlap C – No Start Phases bits 0-7 = phases 1-8
	char	overlapC_not;	  //Overlap C – Not On With Phases bits 0-7 = phases 1-8
	char	overlapD_parent;  //Overlap D – On With Phases bits 0-7 = phases 1-8
	char	overlapD_omit;	  //Overlap D – Omit Phases bits 0-7 = phases 1-8
	char	overlapD_nostart; //Overlap D – No Start Phases bits 0-7 = phases 1-8
	char	overlapD_not;	  //Overlap D – Not On With Phases bits 0-7 = phases 1-8
	char	overlapE_parent;  //Overlap E – On With Phases bits 0-7 = phases 1-8
	char	overlapE_omit;	  //Overlap E – Omit Phases bits 0-7 = phases 1-8
	char	overlapE_nostart; //Overlap E – No Start Phases bits 0-7 = phases 1-8
	char	overlapE_not;	  //Overlap E – Not On With Phases bits 0-7 = phases 1-8
	char	overlapF_parent;  //Overlap F – On With Phases bits 0-7 = phases 1-8
	char	overlapF_omit;	  //Overlap F – Omit Phases bits 0-7 = phases 1-8
	char	overlapF_nostart; //Overlap F – No Start Phases bits 0-7 = phases 1-8
	char	overlapF_not;	  //Overlap F – Not On With Phases bits 0-7 = phases 1-8
	tsmss_get_set_tail_t overlap_tail;
} IS_PACKED overlap_msg_t;

typedef struct {
	//	Detector Attributes = Det Attrib
	//	Detector Configuration = Det Config
	//	X = detector 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41
	//	X+1 = detector 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42
	//	X+2 = detector 3, 7, 11, 15, 19, 23, 27, 31, 35, 39, 43
	//	X+3 = detector 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44

    char det_type; //Byte 8 Det X Attrib – Detector Type
	//	0 = None
	//	1 = Count
	//	2 = Call
	//	3 = Extend
	//	4 = Count + Call
	//	5 = Call + Extend
	//	6 = Count + Call + Extend
	//	7 = Count + Extend
	//	8 = Limited
	//	9 = Bicycle
	//	10 = Pedestrian

    char phase_assignment; //Byte 9 Det X Attrib – Phases Assignment
	//	Phases that are assigned to the detector
	//	Bits 0-7 <-> phases 1-8

    char lock;	//Byte 10 Det X Attrib – Lock
	//	0 = No
	//	1 = Red
	//	2 = Yellow

    char delay_time;	//Byte 11 Det X Attrib – Delay Time 0-255
    char extend_time;	//Byte 11 Det X Attrib – Extend Time (0.1 sec) 0-255
    char recall_time;	//Byte 11 Det X Attrib – Recall Time 0-255
    char input_port;	//Byte 11 Det X Attrib – Input Port (0x13 = 1.3)

} IS_PACKED detector_attr_t;

typedef struct {
    tsmss_get_set_hdr_t detector_hdr; 
	// detector_hdr.mess_type=0xC7 GET response
	// detector_hdr.mess_type=0x96 SET request
	// detector_hdr.page_id=0x07
	// detector_hdr.block_id=0x01=det 1-4
	//	Block 	Block Description	Bytes	Timing Chart 
	//	ID#					Reference
	//	1 	Detector 1 to 4		38
	//	2 	Detector 5 to 8		38
	//	3 	Detector 9 to 12	38
	//	4 	Detector 12 to 16	38
	//	5 	Detector 17 to 20	38
	//	6 	Detector 21 to 24	38
	//	7 	Detector 25 to 28	38
	//	8 	Detector 29 to 32	38
	//	9 	Detector 33 to 36	38
	//	10 	Detector 37 to 40	38
	//	11 	Detector 41 to 44	38
	//	12	Failure Times		34	Failure Times (5-3)
	//		Failure Override		Failure Override (5-4)
	//		System Det. Assignment		System Detectory Assignment (5-5)
	//	13 	CIC Operation		35	CIC Operation (5-6-1)		
	//		CIC Values			CIC Values (5-6-2)
	//		Det.-to-Phase Ass't		Det.-to-Phase Ass't (5-6-3)

    detector_attr_t	detector_attr[4];
    tsmss_get_set_tail_t detector_tail;

} IS_PACKED detector_msg_t;

typedef struct {
  unsigned char start_flag;    // 0x7e
  unsigned char address;       // 0x01 
  unsigned char control;       // 0x13 - unnumbered information, individual address
  unsigned char ipi;           // 0xc0 - NTCIP Class B Protocol 
  unsigned char mess_type;     // 0xCE - raw spat message	
  unsigned char active_phase;  // Bits 1-8 <=> phase 1-8
  unsigned char interval_A;    // interval on ringA phase
  unsigned char interval_B;   // interval on ringB phase
  unsigned char intvA_timer;   // countdown timer for active ringA interval, in 10th sec
  unsigned char intvB_timer;   // countdown timer for active ringB interval, in 10th sec
  unsigned char next_phase;    // Bits 1-8 <==> phase 1 - 8 (same format as active phase)
  unsigned char ped_call;      // Bits 1-8 <=> Pedestrian call on phase 1-8
  unsigned char veh_call;      // Bits 1-8 <=> vehicle call on phase 1-8
  unsigned char plan_num;      // control plan
  unsigned char local_cycle_clock;	
  unsigned char master_cycle_clock;	
  unsigned char preempt;       // preemption
  unsigned char permissive[8]; // permissive period for phase 1 to 8
  unsigned char force_off_A;   // force-off point for ringA phase
  unsigned char force_off_B;   // force-off point for ringB phase	
  unsigned char ped_permissive[8]; // ped permissive period for phase 1 to 8
  unsigned char dummy;
  unsigned char FCSmsb;       // FCS (Frame Checking Sequence) MSB 
  unsigned char FCSlsb;       // FCS least significant byte
  unsigned char end_flag;     // 0x7e

} raw_signal_status_msg_t;

typedef union
{
	get_long_status8_mess_typ         get_long_status8_mess;
	get_long_status8_resp_mess_typ    get_long_status8_resp_mess;
	gen_mess_typ                      gen_mess;
	set_controller_timing_data_t      set_controller_timing_data_mess;
	set_time_t			  set_time_mess;
	get_controller_timing_data_request_t get_controller_timing_data_request_mess;
	get_controller_timing_data_response_t get_controller_timing_data_response_mess;
	overlap_msg_t			overlap_msg;
	get_short_status_request_t	get_short_status_resp;
	detector_msg_t			detector_msg_t;

} IS_PACKED mess_union_typ;

int *GetControllerID(void);
int *SetTime(void);
int *SetPattern(void);
int *GetShortStatus(void);
int *GetSystemDetectorData(void);
int *GetStatus8(void);
int *SetLoginAccess(void);
int *SetMasterPolling(void);
int *GetControllerTimingData(void);
int *SetControllerTimingData(void);
int *GetStatus16(void);
int *SetControllerTimingDataOffset(void);
int *GetLongStatus8(void);
int *SetMasterTrafficResponsive(void);


typedef struct {
	char Walk1;	//08, 40, 72, 104, 136, 168, 200, 232
	char PedClr;	//09, 41, 73, 105, 137, 169, 201, 233
	char MinGrn;	//10, 42, 74, 106, 138, 170, 202, 234
	char Added;	//11, 43, 75, 107, 139, 171, 203, 235
	char MaxInt;	//12, 44, 76, 108, 140, 172, 204, 236
	char Vext;	//13, 45, 77, 109, 141, 173, 205, 237
	char Walk2;	//14, 46, 78, 110, 142, 174, 206, 238
	char MinGap;	//15, 47, 79, 111, 143, 175, 207, 239
	char BikeMG;	//16, 48, 80, 112, 144, 176, 208, 240
	char RedAft;	//17, 49, 81, 113, 145, 177, 209, 241
	char TTRed;	//18, 50, 82, 114, 146, 178, 210, 242
	char const01;	//19, 51, 83, 115, 147, 179, 211, 243
	char const0;	//20, 52, 84, 116, 148, 180, 212, 244
	char Max1;	//21, 53, 85, 117, 149, 181, 213, 245
	char Max2;	//22, 54, 86, 118, 150, 182, 214, 246
	char Max3;	//23, 55, 87, 119, 151, 183, 215, 247
	char Yel;	//24, 56, 88, 120, 152, 184, 216, 248
	char RedClr;	//25, 57, 89, 121, 153, 185, 217, 249
	char RedRevert;	//26, 58, 90, 122, 154, 186, 218, 250
	char MaxExt;	//27, 59, 91, 123, 155, 187, 219, 251
	char EarlyWlk;	//28, 60, 92, 124, 156, 188, 220, 252
	char DlyWlk;	//29, 61, 93, 125, 157, 189, 221, 253
	char SolDW;	//30, 62, 94, 126, 158, 190, 222, 254
	char CSMinGrn;	//31, 63, 95, 127, 159, 191, 223, 255
	char NegPed;	//32, 64, 96, 128, 160, 192, 224, 256
	char APDisc;	//33, 65, 97, 129, 161, 193, 225, 257
	char PmtGrn;	//34, 66, 98, 130, 162, 194, 226, 258
	char PmtWalk;	//35, 67, 99, 131, 163, 195, 227, 259
	char PmtPedClr;	//36, 68, 100, 132, 164, 196, 228, 260
	char CSMaxGrn;	//37, 69, 101, 133, 165, 197, 229, 261
	char ReturnGrn;	//38, 70, 102, 134, 166, 198, 230, 262
	char AdvFls;	//39, 71, 103, 135, 167, 199, 231, 263
} IS_PACKED san_jose_timing_single_phase_t; 

typedef struct
{
	char	start_flag;   /* 0x7e */
	char	address;      /* 0x05 2070 controller */
	char	control;      /* 0x13 - unnumbered information, individual address */
	char	ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char	mess_type;    /* 0xc7 - set controller timing data*/
	char	const1_1;
	char	const2_0;
	char	const3_ff;
	san_jose_timing_single_phase_t san_jose_timing_single_phase[8]; 
	char	FCSmsb;       /* FCS (Frame Checking Sequence) MSB */
	char	FCSlsb;       /* FCS least significant byte */
	char	end_flag;     /* 0x7e */
} IS_PACKED san_jose_timing_msg_t;

/*
      0000           0        e0        4c        5c             53        f2        10        10           b6            0        5a        af         8          0          45             0
      0010           1        27         0         0             40         0        40        11           1d           c2        0a        c0        83          9          0a            c0
      0020          83        7b         0        a2             fa        da         1        13           a9           95        7e         5        13         c0          c7             1

      0030           0        ff   Walk1-1   PedClr1        MinGrn1    Added1   MaxInt1    Vext 1      Walk2-1      MinGap1   BikeMG1   RedAft1    TTRed1          0           0       Max 1-1
      0040      Max2-1    Max3-1      Yel1   RedClr1     RedRevert1   MaxExt1 EarlyWlk1   DlyWlk1       SolDW1    CSMinGrn1   NegPed1   APDisc1   PmtGrn1   PmtWalk1   PmtPedClr     CSMaxGrn1
      0050  ReturnGrn1   AdvFls1   

      0050                         Walk1-2   PedClr2        MinGrn2    Added2   MaxInt2     Vext2      Walk2-2      MinGap2   BikeMG2   RedAft2    TTRed2          0           0        Max1-2
      0060      Max2-2    Max3-2      Yel2   RedClr2     RedRevert2   MaxExt2 EarlyWlk2   DlyWlk2       SolDW2    CSMinGrn2   NegPed2   APDisc2   PmtGrn2   PmtWalk2   PmtPedClr     CSMaxGrn2
      0070  ReturnGrn2   AdvFls2   

      0070                         Walk1-3   PedClr3        MinGrn3    Added3   MaxInt3     Vext3      Walk2-3      MinGap3   BikeMG3   RedAft3    TTRed3          0           0        Max1-3
      0080      Max2-3    Max3-3      Yel3   RedClr3     RedRevert3   MaxExt3 EarlyWlk3   DlyWlk3       SolDW3    CSMinGrn3   NegPed3   APDisc3   PmtGrn3   PmtWalk3   PmtPedClr     CSMaxGrn3
      0090  ReturnGrn3   AdvFls3   

      0090                         Walk1-4   PedClr4        MinGrn4    Added4   MaxInt4     Vext4      Walk2-4      MinGap4   BikeMG4   RedAft4    TTRed4          0           0        Max1-4
      00a0      Max2-4    Max3-4      Yel4   RedClr4     RedRevert4   MaxExt4 EarlyWlk4   DlyWlk4       SolDW4    CSMinGrn4   NegPed4   APDisc4   PmtGrn4   PmtWalk4   PmtPedClr     CSMaxGrn4
      00b0  ReturnGrn4   AdvFls4   

      00b0                         Walk1-5   PedClr5        MinGrn5    Added5   MaxInt5     Vext5      Walk2-5      MinGap5   BikeMG5   RedAft5    TTRed5          0           0        Max1-5
      00c0      Max2-5    Max3-5      Yel5   RedClr5     RedRevert5   MaxExt5 EarlyWlk5   DlyWlk5       SolDW5    CSMinGrn5   NegPed5   APDisc5   PmtGrn5   PmtWalk5   PmtPedClr     CSMaxGrn5
      00d0  ReturnGrn5   AdvFls5   

      00d0                         Walk1-6   PedClr6        MinGrn6    Added6   MaxInt6     Vext6      Walk2-6      MinGap6   BikeMG6   RedAft6    TTRed6          0           0        Max1-6
      00e0      Max2-6    Max3-6      Yel6   RedClr6     RedRevert6   MaxExt6 EarlyWlk6   DlyWlk6       SolDW6    CSMinGrn6   NegPed6   APDisc6   PmtGrn6   PmtWalk6   PmtPedClr     CSMaxGrn6
      00f0  ReturnGrn6   AdvFls6   

      00f0                         Walk1-7   PedClr7        MinGrn7    Added7   MaxInt7     Vext7      Walk2-7      MinGap7   BikeMG7   RedAft7    TTRed7          0           0        Max1-7
      0100      Max2-7    Max3-7      Yel7   RedClr7     RedRevert7   MaxExt7 EarlyWlk7   DlyWlk7       SolDW7    CSMinGrn7   NegPed7   APDisc7   PmtGrn7   PmtWalk7   PmtPedClr     CSMaxGrn7
      0110  ReturnGrn7   AdvFls7   

      0110                         Walk1-8   PedClr8        MinGrn8    Added8   MaxInt8     Vext8      Walk2-8      MinGap8   BikeMG8   RedAft8    TTRed8          0           0        Max1-8
      0120      Max2-8    Max3-8      Yel8   RedClr8     RedRevert8   MaxExt8 EarlyWlk8   DlyWlk8       SolDW8    CSMinGrn8   NegPed8   APDisc8   PmtGrn8   PmtWalk8   PmtPedClr     CSMaxGrn8
      0130  ReturnGrn8   AdvFls8   

      0130                          CkSum1    CkSum2             7e 
*/
#endif
