/* FILE:  msgs.h  Header file for AB3418 format messagew
 *
 * Copyright (c) 2006  Regents of the University of California
 *
 * static char rcsid[] = "$Id$";
 *
 *
 * $Log$
 * Revision 1.1  2007/01/22 16:01:20  lou
 * Initial revision
 *
 *
 */


#define SERIAL_DEVICE_NAME "/dev/ttyS0"

/* Following is format for GetLongStatus8 message. */

/* Message sent from RSU to 2070 controller
 * has the following format. */
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
} get_long_status8_mess_typ;

/* Message sent from 2070 to RSU has the following
 * format. */
typedef struct
{
	char          start_flag;   /* 0x7e */
	char          address;      /* 0x05 Requester PC */
	char          control;      /* 0x13 - unnumbered information, individual address */
	char          ipi;          /* 0xc0 - NTCIP Class B Protocol */
	char          mess_type;    /* 0xcc - get long status8 response */
	unsigned char flags;        /* Additional flags; bit 0:focus (default 0 = no focus),
	                             * bits 1-7: reserved, unused. */
	unsigned char status;       /* Bit 7 = critical alarm; bit 6 = non-critical alarm;
	                             * bit 5 = detector fault; bit 4 = coordination alarm;
	                             * bit 3 = local override; bit 2 = passed local zero;
	                             * bit 1 = cabinet flash; bit 0 = preempt. */
	unsigned char pattern;      /* Pattern (0-250, 251-253 reserved, 254 flash, 255 free) */
	unsigned char green_yellow_overlap; /* Bits 0-3 green overlaps A-D;
	                              * bits 4-7 yellow overlaps A-D */
	unsigned char preemption;    /* Bits 0-1 EV A-D; bits 4-6 RR 1-2;
	                              * bit 6 = pattern transition; bit 7 unused */
	unsigned char phase_call;    /* Phase call 1-8; (bit 7 = phase 8; bit 0 = phase 1) */
	unsigned char ped_call;      /* Ped call 1-8; (bit 7 = ped 8; bit 0 = ped 1) */
	unsigned char active_phase;  /* Bits 0-7 -> phases 1-8; bit set true for phase active */
	unsigned char interval;      /* Bits 0-3: ring 0 interval; bits 4-7: ring 1 interval.
	                              * Interval encoding is as follows:
                                  * 0X00 = walk, 0x01 = don't walk, 0x02 = min green,
	                              * 0x03 = unused, 0x04 = added initial, 0x05 = passage -resting,
	                              * 0x06 = max gap, 0x07 = min gap, 0x08 = red rest,
	                              * 0x09 = preemption, 0x0a = stop time, 0x0b = red revert,
	                              * 0x0c = max termination, 0x0d = gap termination,
	                              * 0x0e = force off, 0x0f = red clearance */
	unsigned char presence1;     /* Bits 0-7: detector 1-8. Presence bits set true for
	                              * positive presence. */
	unsigned char presence2;     /* Bits 0-7: detector 9-16 */
	unsigned char presence3;     /* Bits 0-7: detector 17-24 */
	unsigned char presence4;     /* Bits 0-3: detector 25-28, bits 4-7 unused */
	unsigned char master_clock;  /* Master background cycle clock.  Counts up to cycle length */
	unsigned char local_clock;   /* Local cycle clock.  Counts up to cycle length. */
	unsigned char seq_number;    /* Sample sequence number */
	unsigned char volume1;       /* System detector 1 */
	unsigned char occupancy1;    /* System detector 1.  Value 0-200 = detector occupancy in
	                              * 0.5% increments, 201-209 = reserved, 210 = stuck ON fault,
	                              * 211 = stuck OFF fault, 212 = open loop fault,
	                              * 213 = shorted loop fault, 214 = excessive inductance fault,
	                              * 215 = overcount fault. */
	unsigned char volume2;       /* System detector 2 */
	unsigned char occupancy2;    /* system detector 2 */
	unsigned char volume3;       /* System detector 3 */
	unsigned char occupancy3;    /* system detector 3 */
	unsigned char volume4;       /* System detector 4 */
	unsigned char occupancy4;    /* system detector 4 */
	unsigned char volume5;       /* System detector 5 */
	unsigned char occupancy5;    /* system detector 5 */
	unsigned char volume6;       /* System detector 6 */
	unsigned char occupancy6;    /* system detector 6 */
	unsigned char volume7;       /* System detector 7 */
	unsigned char occupancy7;    /* system detector 7 */
	unsigned char volume8;       /* System detector 8 */
	unsigned char occupancy8;    /* system detector 8 */
	unsigned char FCSmsb;        /* FCS (Frame Checking Sequence) MSB */
	unsigned char FCSlsb;        /* FCS least significant byte */
	char          end_flag;      /* 0x7e */
} get_long_status8_resp_mess_typ;


typedef struct
{
	/* Set for maximum length of expanded message after converting
	 * any 0x7e to the combo 0x7d, 0x5e. */
	unsigned char    data[100];
} gen_mess_typ;

typedef union
{
	get_long_status8_mess_typ         get_long_status8_mess;
	get_long_status8_resp_mess_typ    get_long_status8_resp_mess;
	gen_mess_typ                      gen_mess;
} mess_union_typ;

