/**\file
 *	Header file with constants for packet formats and Management
 *	Information Base definitions.
 *	See NTCIP 9001 v03.02 Chapter 6 for a summary description
 *	of packet structure and frame check sequence. 
 *	See NTCIP 1202 (formerly NEMA TS 3.5) for MIB definitions
 *	for Actuated Traffic Signal Controller Units. 
 *	Used logs from the Exerciser software from NTCIP, which runs
 *	on a Windows PC, to elucidate and vlidate details of packet
 *	formats.
 *	Get documents and Exerciser from ntcip.org.
 *
 * Copyright (c) 2003   Regents of the University of California
 *
 */

/* STMF maximum packet size */
#define STMF_MAX_PACKET	1500

/* index into PMPP message header */
#define PMPP_HEADER_SIZE 4
#define PMPP_FLAG_INDEX 0
#define PMPP_ADDR_INDEX 1	/* assume no two bytes addresses for now */
#define PMPP_CTRL_INDEX 2
#define PMPP_IPI_INDEX  3	/* protocol field, should be STMF */	

#define PMPP_FLAG_VAL	0x7e
#define PMPP_ADDR_VAL(n)	(((n) << 2) | 1)	
#define PMPP_CTRL_VAL	0x13	/* Unnumbered Information, P/F set to 1 */	
#define PMPP_STMF_VAL	0xc1	/* STMF: only value with ASC */

/* "magic number" for Frame Check Sequence, see NTCIP 9001 */
#define PMPP_FCS_MAGIC (unsigned short) 0xf0b8

#define SNMP_HEADER_SIZE 32
/* if first byte of STMF packet is 0x30, protocol is SNMP */
#define SNMP_SEQUENCE	0x30
/* OR with number of bytes needed to represent count */
#define SNMP_MULTI_BYTE_COUNT	0x80

#define SNMP_INTEGER	0x02
#define SNMP_OCTET_STR	0x04
#define SNMP_NULL	0x05
#define SNMP_OBJECT_ID	0x06

#define SNMP_GET_REQUEST	0xa0
#define SNMP_GET_RESPONSE	0xa2
#define SNMP_SET_REQUEST	0xa3

/* Maximum depth of substructures, including table entries, in 1202 ASC
 * standard MIB
 */
#define ASC_MAX_ID	6
#define ASC_PREFIX_LENGTH	10

extern unsigned char asc_oid_prefix[];

/* structure to hold object ids for objects in ASC subtree
 */
typedef struct {
	unsigned char id[ASC_MAX_ID];	
	int id_length;
	int type;	/* for use of parsing routine */
	int val;	/* for use in set requests, only setting int for now */
} asc_oid;

/* identifiers for types of objects to decode */

#define UNKNOWN_OBJECT_TYPE		9999	/* for all types not on list */
#define PHASE_STATUS_GROUP_NUMBER	1	/*phaseStatusGroupNumber */
#define PHASE_STATUS_GROUP_REDS		2	/*phaseStatusGroupReds */
#define PHASE_STATUS_GROUP_YELLOWS	3	/*phaseStatusGroupYellows */
#define PHASE_STATUS_GROUP_GREENS	4	/*phaseStatusGroupGreens */
#define PHASE_STATUS_GROUP_ONS		5	/*phaseStatusGroupPhaseOns */
#define PHASE_STATUS_GROUP_NEXT		6	/*phaseStatusGroupPhaseNexts */
#define DETECTOR_STATUS_GROUP		7	/*vehicleDetectorStatusGroup */
#define PREEMPT_CONTROL_STATE		9	/*preemptControlState */

/* for use on unsigned short variables */
#define HIGH_BYTE(x)	((unsigned char) (((x) & (unsigned short) 0xff00) >> 8))
#define LOW_BYTE(x)	((unsigned char) ((x) & (unsigned short) 0xff))	

/* function definitions for fcs.c, format_asc.c and parse_asc.c */
extern unsigned short compute_fcs(unsigned char *buf, int length);
extern int fmt_asc_get_request(unsigned char *buf, int bufsize,
        unsigned char request_id, asc_oid *list, int num_oids);
extern int fmt_asc_set_request(unsigned char *buf, int bufsize,
         unsigned char request_id, asc_oid *list, int num_oids);
extern int fmt_pmpp_header(int dest_addr, char * buf);
extern int parse_asc_get_response(unsigned char *buf, int byte_count,
	asc_oid *list, int num_oids, db_clt_typ *pclt);
