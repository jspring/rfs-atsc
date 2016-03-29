/* urms.h - Header file for urms.c
*/

#ifndef URMS_H
#define URMS_H

#include <db_include.h>
#include "tos.h"

#define DB_URMS_TYPE		4321
#define DB_URMS_VAR		DB_URMS_TYPE
#define DB_URMS_STATUS_TYPE	4322
#define DB_URMS_STATUS_VAR	DB_URMS_STATUS_TYPE
#define URMS_ACTION_REST_IN_DARK	1
#define URMS_ACTION_REST_IN_GREEN	2
#define URMS_ACTION_FIXED_RATE		3
#define URMS_ACTION_SKIP		6

//Active interval status (interval zone)
#define INITIALIZATION		1
#define PRE_METERING_NONGREEN	2
#define PRE_METERING_GREEN	3 
#define STARTUP_ALERT		4
#define STARTUP_WARNING		5
#define STARTUP_GREEN		6
#define STARTUP_YELLOW		7
#define STARTUP_RED		8
#define METERING_RED		9
#define METERING_GREEN		10
#define METERING_YELLOW		11
#define SHUTDOWN_GREEN		12
#define SHUTDOWN_YELLOW		13
#define SHUTDOWN_RED		14
#define SHUTDOWN_WARNING	15
#define POST_METERING_GREEN	16

typedef struct {

	unsigned addr : 4; //Controller drop address
	unsigned hdr1 : 4; //Beginning of header (= 0xE)
	unsigned char hdr2; //(= 0x15)
	unsigned char hdr3; //(= 0x70)
	unsigned char hdr4; //(= 0x01)
	unsigned char hdr5; //End of header (= 0x11)
	unsigned char lane_1_action; //Metered lane 1 action (1=dark,2=rest in 
				     //green,3=fixed rate,
				     //6=skip(return to TOD metering)
	unsigned char lane_1_release_rate_MSB;//Metered lane 1 release rate(VPH)
	unsigned char lane_1_release_rate_LSB;//Metered lane 1 release rate(VPH)
	unsigned char lane_1_plan;//Metered lane 1 plan number
	unsigned char lane_2_action; //Metered lane 2 action
	unsigned char lane_2_release_rate_MSB;//Metered lane 2 release rate(VPH)
	unsigned char lane_2_release_rate_LSB;//Metered lane 2 release rate(VPH)
	unsigned char lane_2_plan;//Metered lane 2 plan number
	unsigned char lane_3_action; //Metered lane 3 action
	unsigned char lane_3_release_rate_MSB;//Metered lane 3 release rate(VPH)
	unsigned char lane_3_release_rate_LSB;//Metered lane 3 release rate(VPH)
	unsigned char lane_3_plan;//Metered lane 3 plan number
	unsigned char lane_4_action; //Metered lane 4 action
	unsigned char lane_4_release_rate_MSB;//Metered lane 4 release rate(VPH)
	unsigned char lane_4_release_rate_LSB;//Metered lane 4 release rate(VPH)
	unsigned char lane_4_plan;//Metered lane 4 plan number
	unsigned char checksum; //8 Bit Checksum ( Message Bytes 0-20 )
	unsigned char tail0; //(= 0x00)
	unsigned char tail1; //(= 0xAA)
	unsigned char tail2; //(= 0x55)
} IS_PACKED urmsctl_t;


typedef struct {
	unsigned addr : 4; //Controller drop address
	unsigned hdr1 : 4; //Beginning of header (= 0x4)
	unsigned char hdr2; //(= 0x03)
	unsigned char checksum; //8 Bit Checksum ( Message Bytes 0-1 )
	unsigned char tail1; //(= 0xAA)
	unsigned char tail2; //(= 0x55)
} IS_PACKED urmsrcv_t;


typedef struct {
	int lane_1_action; //Metered lane 1 action (1=dark,2=rest in 
				     //green,3=fixed rate,
				     //6=skip(return to TOD metering)
	int lane_1_release_rate;//Metered lane 1 release rate(VPH)
	int lane_1_plan;//Metered lane 1 plan number
	int lane_2_action; //Metered lane 2 action
	int lane_2_release_rate;//Metered lane 2 release rate(VPH)
	int lane_2_plan;//Metered lane plan number
	int lane_3_action; //Metered lane 3 action
	int lane_3_release_rate;//Metered lane 3 release rate(VPH)
	int lane_3_plan;//Metered lane 3 plan number
	int lane_4_action; //Metered lane 4 action
	int lane_4_release_rate;//Metered lane 4 release rate(VPH)
	int lane_4_plan;//Metered lane 4 plan number
	char	no_control; //Request by arterial computer to stop controlling
} IS_PACKED db_urms_t;

typedef struct {
//URMS Poll From TMC  
//				// Dec Hex Parameter
	char	msg_type;	// 0 Type 51 - URMS Poll 1 [0x51]
	char	drop_num;	// 1 Drop Number
	char	num_bytes;	// 2 Byte Count to Checksum [0x08]
	char	action;		// 3 Action Number 
				//     1 - Rest in Dark
				//     2 - Rest in Green
				//     3 - Fixed Rate
				//     4 - Traffic Responsive
				//     6 - Skip (Normal Case)
				//     7 - Slave Responsive (Slave data being used from a master controller)
	char	rate_msb;	// 4 Rate (MSB_
	char	rate_lsb;	// 5 Rate (LSB)
	char	plan;		// 6 Traffic Responsive Plan Number ( 1 to 10)
	char	gp_out;		// 7 General Purpose Outputs
	short	future_use;	// 8 Future Use - Set to Zero
	char	enable; 	// 10 A Enable Bits
				//	Bit 1 - Reset Controller
				//	Bit 2 - Output Detector Reset Pulse
				//	3 - Enable General Purpose Outputs Remotely
				//	4 - Future Use - Set to Zero
				//	5 - Future Use - Set to Zero
				//	6 - Future Use - Set to Zero
				//	7 - Future Use - Set to Zero
				//	8 - Future Use - Set to Zero
	char	checksum_msb;	// 11 B FCS-16 Checksum (MSB)
	char	checksum_lsb;	// 12 C FCS-16 Checksum (MSB)
} IS_PACKED urms_status_poll_t;

struct mainline_stat {
	char speed;	//25 19 Mainline 1 Speed 
	char lead_vol;	//26 1A Mainline 1 Leading Volume 
	char lead_occ_msb;	//27 1B Mainline 1 Leading Occupancy (MSB) 
	char lead_occ_lsb;	//28 1C Mainline 1 Leading Occupancy (LSB) 
	char lead_stat;	/*29 1D Mainline 1 Leading Status 
			 for Mainline, Queue and Ramp Detectors
			DETECTOR_DISABLED = 1,
			DETECTOR_WORKING = 2,
			DETECTOR_OTHER_ERROR = 3,
			DETECTOR_ERRATIC_COUNT = 4,
			DETECTOR_MAX_PRESENCE = 5,
			DETECTOR_NO_ACTIVITY = 6,
			DETECTOR_ERROR_AT_SENSOR = 7,
			DETECTOR_DEPENDENT_NO_ACTIVITY = 8,
			DETECTOR_DEPENDENT_MAX_PRESENCE = 9 */
	char trail_vol;	//30 1E Mainline 1 Trailing Volume 
	char trail_occ_msb;	//31 1F Mainline 1 Trailing Occupancy (MSB) 
	char trail_occ_lsb;	//32 20 Mainline 1 Trailing Occupancy (LSB) 
	char trail_stat;	//33 21 Mainline 1 Trailing Status 
	char stat;	//34 22 Mainline 1 Lane Status 
};

struct addl_det_stat {
	char volume;	//187 BB Additional Detector 1 Volume 
	char occ_msb;	//188 BC Additional Delector 1 Occupancy(MSB) 
	char occ_lsb;	//189 BD Additional Delector 1 Occupancy(MLB) 
	char stat;	//190 BE Additional Delector 1 Status 
};

struct metered_lane_stat{
	char demand_vol;	//251 FB Demand 1 Volume 
	char demand_stat;	//252 FC Demand 1 Status 
				//	0=Non Metering
				//	1=Metering Startup
				//	2=Metering
				//	3=Metering Shutdown Active
				//	4=Metering Shutdown
	char passage_vol;	//253 FD Passage 1 Volume 
	char passage_stat;	//254 FF Passage 1 Status 
	char metered_lane_interval_zone;	//255 100 Metered Lane 1 Interval Zone 
	char passage_viol;	//256 FE Passage 1 Violations 
	char metered_lane_rate_msb;	//257 101 Metered Lane 1 Release Rate (MSB) 
	char metered_lane_rate_lsb;	//258 102 Metered Lane 1 Release Rate (LSB) 
};

struct metered_lane_ctl {
	char cmd_src;	//291 123 Metered Lane 1 Command Source 
	char action;	//292 124 Metered Lane 1 Action 
	char release_rate_msb;	//293 125 Metered Lane 1 Release Rate (MSB) 
	char release_rate_lsb;	//294 126 Metered Lane 1 Release Rate (LSB) 
	char plan;	//295 127 Metered Lane 1 Plan Number 
	char plan_base_lvl;	//296 128 Metered Lane 1 Plan Base Level 
	char plan_adj_lvl;	//297 129 Metered Lane 1 Plan Adjustment Level 
	char plan_final_lvl;	//298 12A Metered Lane 1 Plan Final Level 
	char base_meter_rate_msb;	//299 12B Metered Lane 1 Base Meter Rate (MSB) 
	char base_meter_rate_lsb;	//300 12C Metered Lane 1 Base Meter Rate  (LSB) 
	char master_queue_flag;	//301 12D Metered Lane 1 Master Queue Flag 
};

struct queue_stat{
	char vol;	// Queue Volume
	char occ_msb;	// Queue Occupancy (MSB)
	char occ_lsb;	// Queue Occupancy (LSB)
	char stat;	/* Queue Status 
			 for Mainline, Queue and Ramp Detectors
			DETECTOR_DISABLED = 1,
			DETECTOR_WORKING = 2,
			DETECTOR_OTHER_ERROR = 3,
			DETECTOR_ERRATIC_COUNT = 4,
			DETECTOR_MAX_PRESENCE = 5,
			DETECTOR_NO_ACTIVITY = 6,
			DETECTOR_ERROR_AT_SENSOR = 7,
			DETECTOR_DEPENDENT_NO_ACTIVITY = 8 */
	char flag;	// Queue Flag
};

typedef struct {

		// URMS Poll Response   
		// Dec Hex Parameter 
	char header1;	//0 0 U [0x55] 
	char header2;	//1 1 R [0x52] 
	char header3;	//2 2 M [0x4D] 
	char header4;	//3 3 S [0x53] 
	char header5;	//4 4 2 [0x32] 
	char numbytes1;	//5 5 Bytes to Checksum (MSB) [0x01] 
	char numbytes2;	//6 6 Bytes to Checksum (LSB)  [0x9A] 
	char id4;	//7 7 Station ID Digit 4 
	char id3;	//8 8 Station ID Digit 3 
	char id2;	//9 9 Station ID Digit 2 
	char id1;	//10 A Station ID Digit 1 
	char spare1;	//11 B Spare Byte 1 - Always set to zero 
	char spare2;	//12 C Spare Byte 2 - Always set to zero 
	char spare3;	//13 D Spare Byte 3 - Always set to zero 
	char day;	//14 E Day 
	char month;	//15 F Month 
	char year_msb;	//16 10 Year (MSB)  
	char year_lsb;	//17 11 Year (LSB) 
	char hour;	//18 12 Hour 
	char minute;	//19 13 Minute 
	char second;	//20 14 Second 
	char num_meter;	//21 15 Number of Metered Lanes 
	char num_main;	//22 16 Number of Mainline Lanes 
	char num_opp;	//23 17 Number of Opposite Mainline Lanes 
	char num_addl_det;	//24 18 Number of Additional Detectors 
	struct mainline_stat mainline_stat[8]; //25-104
	char mainline_dir;	//105 69 Mainline Direction Bits (Each Lane 0=Normal, 1=Reverse) 
	struct mainline_stat opposite_stat[8]; //106-185
	char opposite_dir;	//186 BA Opposite Mainline Direction Bits (Each Lane 0=Normal, 1=Reverse) 
	struct addl_det_stat additional_det[16]; //187-250
	struct metered_lane_stat metered_lane_stat[4]; //251-282
	char is_metering;	//283 11B Is Metering (1 = YES) 
	char traffic_responsive_vol_msb;	//284 11C Traffic Responsive Volume (VPH)(MSB) 
	char traffic_responsive_vol_lsb;	//285 11D Traffic Responsive Volume (VPH)(LSB) 
	char 	traffic_responsive_occ_msb;	//286 11E Traffic Responsive Occupancy (VPH)(MSB) 
	char 	traffic_responsive_occ_lsb; 	//287 11F Traffic Responsive Occupancy (VPH)(LSB) Each bit is one General Purpose output sign
	char 	traffic_responsive_speed;	//288 120 Traffic Responsive Speed (VPH) 
	char 	spare;	//289 121 Spare 
	char	gen_purpose_out_stat;	//290 122 General Purpose Output Status 
	struct metered_lane_ctl metered_lane_ctl[4]; //291-334
	struct queue_stat queue_stat[4][4]; //335-414
	char checksum_msb;	//415 19F CRC-16 Checksum (MSB) 
	char checksum_lsb;	//416 1A0 CRC-16 Checksum (LSB) 
} IS_PACKED urms_status_response_t;

typedef struct {
		// URMS Poll Response   
		// Dec Hex Parameter 
	char num_meter;	//0 Number of Metered Lanes 
	char num_main;	//1 Number of Mainline Lanes 
	char num_opp;	//2 Number of Opposite Mainline Lanes 
	char num_addl_det;	//3 Number of Additional Detectors 
	struct mainline_stat mainline_stat[3]; //4-33
	char mainline_dir;	//34 Mainline Direction Bits (Each Lane 0=Normal, 1=Reverse) 
	struct metered_lane_stat metered_lane_stat[3]; //35-58
	char is_metering;	//59 Is Metering (1 = YES) 
	struct queue_stat queue_stat[3]; //60-74
	char	cmd_src[3]; //75-77
	char	action[3]; //78-80
	unsigned char	plan[3];   //81-83
	unsigned char	computation_finished; //84
	char	plan_base_lvl[3]; //85-87
	char	no_control; //88
        struct addl_det_stat additional_det[2]; //89-96
	unsigned char	rm2rmc_ctr; //97
	char	hour; //98
	char	minute; //99
	char	second; //100
	unsigned short  checksum; //101-102
} IS_PACKED db_urms_status_t;

/*
				URMS	URMS
				Poll	Poll	db_urms_status
Det	File	Port	Pin	Vol	Occ	Vol	Occ
ML1	I1U	1.8	46	26	27-8	4	5-6
MT1	I1L	1.1	39	30	31-2	9	10-1		
ML2	I2U	2.4	50	36	37-8
MT2	I2L	2.1	47	40	41-2
ML3	I3U	2.3	49	46	47-8
MT3	I3L	2.2	48	
ML4	I4U	3.1	55
MT4	I4L	3.2	56	
D1	I9U	6.7	81	251
D2	I11U	2.7	53	259
D3	I13U	1.5	43	267
P1	I9L	6.6	80	253	???
P2	I11L	2.8	54	264	???
P3	I13L	1.4	42	270	???
Q1-1	I10U	6.5	79	335	336-7
Q2-1	I12U	1.3	41	340	341-2
Q3-1	I14U	1.7	45	345	346-7

OL1	I5U	2.5	51
OT1	I5L	2.6	52	
OL2	I6U	3.3	57
OT2	I6L	3.4	58	
OL3	I7U	3.5	59
OT3	I7L	3.6	60	
OL4	I8U	3.7	61
OT4	I8L	3.8	62	
F1	I10L	6.8	82	
F2	I12L	1.2	40	
F3	I14L	1.6	44	

*/

typedef union {
	urms_status_response_t urms_status_response;
	urms_status_poll_t urms_status_poll;
	urmsctl_t urmsctl;
	urmsrcv_t urmsrcv;
	tos_set_action_t tos_set_action;
	tos_enable_detectors_t tos_enable_detectors;
	char data[1000];
	char urmspoll2[9];
} IS_PACKED gen_mess_t;

#endif
