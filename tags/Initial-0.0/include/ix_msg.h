/**\file
 *
 *	Header file for intersection set message parsing.
 *
 *	Packed structures are designed to map over top of fixed sized
 *	sections of buffer received from UDP, to get pointer to
 *	variable sized sections when parsing.
 *
 *	Last three bytes of message are check sum and closing flag.
 *
 *	Sue Dickey, October 24, 2005
 *	Revised July 25, 2006 for new message set at Page Mill.
 *
 *	Copyright (c) 2005 Regents of the University of California
 *
 */
#ifndef IX_MSG_H
#define IX_MSG_H

#ifdef TEST_ONLY
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/types.h>	// The next two lines for message queses
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/select.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#define IS_PACKED	__attribute__((__packed__))
#define ERROR 			-1
#else
// above includes are all part of sys_os.h in /home/path/local
#include <sys_os.h>
#include "local.h"
#include "sys_rt.h"
#endif

#define MAX_APPROACHES 		10
#define MAX_STOP_LINES 		2
#define MAX_TIMINGS    		5	// different timing plans
#define MAX_INTERVALS		10	// different periods, some with same timing

// Values defined for approach type
#define APPROACH_T_UNKNOWN		0
#define APPROACH_T_STRAIGHT_THROUGH	1 
#define APPROACH_T_LEFT_TURN		2
#define APPROACH_T_RIGHT_TURN		3

// Values defined for signal state
#define SIGNAL_STATE_UNKNOWN		0
#define SIGNAL_STATE_GREEN		1
#define SIGNAL_STATE_YELLOW		2
#define SIGNAL_STATE_RED		3
#define SIGNAL_STATE_FLASHING_YELLOW	4
#define SIGNAL_STATE_FLASHING_RED	5
#define SIGNAL_STATE_GREEN_ARROW	6
#define SIGNAL_STATE_YELLOW_ARROW	7
#define SIGNAL_STATE_RED_ARROW		8

// Note that int is assumed to be 32 bits, short 16 and char 8
// For portability these fields should be typedefed
typedef struct {
	unsigned char phase_index; 
	unsigned short green_offset; 
	unsigned short green_length; 
} IS_PACKED ix_phase_t;

typedef struct {
	unsigned int start; 
	unsigned int end; 
	unsigned short cycle_length;
	ix_phase_t *phase_array;	// variable sized array
} IS_PACKED ix_timing_plan_t;

typedef struct {
	int latitude; 
	int longitude; 
	unsigned short line_length;	//  cm 
	unsigned short orientation;	// degrees clockwise from north
} IS_PACKED ix_stop_line_t;

typedef struct {
	unsigned char approach_type;	
	unsigned char signal_state;	// not NEMA terminology
	short time_to_next;	
	unsigned char vehicles_detected; // boolean?	
	unsigned char ped_signal_state;	// pedestrian
	unsigned char seconds_to_ped_signal_state_change;
	unsigned char ped_detected;;
	unsigned char seconds_since_ped_detect;
	unsigned char seconds_since_ped_phase_started;
	unsigned char emergency_vehicle_approach; 
	unsigned char seconds_until_light_rail; 
	unsigned char high_priority_freight_train; 
	unsigned char vehicle_stopped_in_ix; 
	unsigned char reserved[2];
	unsigned char number_of_stop_lines;
	ix_stop_line_t *stop_line_array;	// variable sized array
} IS_PACKED ix_approach_t; 

typedef struct {
	unsigned char flag;
	unsigned char version;
	unsigned short message_length;
	unsigned char message_type;
	unsigned char control_field;
	unsigned char ipi_byte;
	unsigned int intersection_id;
	unsigned int map_node_id;
	int ix_center_lat;	// intersection center location
	int ix_center_long;	
	short ix_center_alt;
	int antenna_lat;	// antenna location
	int antenna_long;	
	short antenna_alt;
	// time fields corresponds to basic POSIX struct timespec
	unsigned int seconds;	// seconds since UTC Jan 1, 1970
	unsigned int nanosecs;	// nanoseconds within second
	unsigned char cabinet_err;
	unsigned char preempt_calls;
	unsigned char bus_priority_calls;
	unsigned char preempt_state;
	unsigned char special_alarm;
	unsigned char reserved[4];
	unsigned char num_approaches; 
	ix_approach_t *approach_array;	//each approach variable sized 
} IS_PACKED ix_msg_t;

#define IX_POINTER_SIZE  sizeof(ix_approach_t *)

// Maximum size of flattened approach structure 
// where pointer field in ix_approach_t is replaced by stop line array
#define MAX_APPROACH_SIZE (sizeof(ix_approach_t) - IX_POINTER_SIZE  + MAX_STOP_LINES * sizeof(ix_stop_line_t))

// number of bytes needed to flatten the maximum size ix message structure
#define MAX_IX_MSG_PKT_SIZE ((sizeof(ix_msg_t)-IX_POINTER_SIZE)+MAX_APPROACHES*MAX_APPROACH_SIZE)   

// Extern declarations for preempt message send to Econolite controller
// Used for World Congress demonstration, may be needed later when
// implementing transit signal priority

extern char *preempt_strings[];
extern int econo_fill_buf(unsigned char *buf, char *str, int max_size);

#define PREEMPT_PORT		161
#define PREEMPT_MIN_INTERVAL	100

#include "ix_utils.h"
#endif
