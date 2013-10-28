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
	cell_addr_data_t cell_addr_data[4]; // Allow 4 parameter changes at a time 
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

#endif


