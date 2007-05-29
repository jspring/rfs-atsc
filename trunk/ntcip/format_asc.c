/**\file
 *
 *	format_asc.c	Formatting routines for sending simple
 *			messages to the Management Information
 *			Base (MIB) of the 2070 Actuated Signal Controller
 *			(ASC) using NTCIP's Simple Transportation
 *			Management Framework (STMF).protocol..
 *
 *
 *
 * Copyright (c) 2003   Regents of the University of California
 *
 */

#include <sys_os.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "local.h"
#include "sys_rt.h"
#include "sys_list.h"
#include "db_clt.h"
#include "mibdefs.h"

/* change to define and remake for debugging messages */

#undef DEBUG

/* "public" */
static unsigned char community_name[]  = {
	0x70, 0x75, 0x62, 0x6C, 0x69, 0x63
};

static void store_byte_count(char *buf, int index, int count_size, int byte_count);

/* returns number of bytes in header */
int fmt_pmpp_header(int dest_addr, char * buf)
{
	buf[PMPP_FLAG_INDEX] = PMPP_FLAG_VAL;
	buf[PMPP_ADDR_INDEX] = PMPP_ADDR_VAL(dest_addr);
	buf[PMPP_CTRL_INDEX] = PMPP_CTRL_VAL;
	buf[PMPP_IPI_INDEX] = PMPP_STMF_VAL;
	return 4;
}

/** Places a list of OIDs (object ids) from the asc (automated signal
 *  controller) subtree into the supplied buffer as a Get Request packet.
 *  Returns the number of bytes placed in buf.
 *  Simple implementation for clarity, could pre-fill headers for efficiency.
 */ 

int fmt_asc_get_request(unsigned char *buf, int bufsize,
	 unsigned char request_id, asc_oid *list, int num_oids)
{
	int i = 0;
	int oid_count;
	int byte_count;
	int pdu_byte_count_index;
	int get_request_byte_count_index;
	int sequence_byte_count_index;
	int count_size=2; /* can make one if all get-requests less than 255 bytes */

#ifdef DEBUG
	printf("buf 0x%x, bufsize %d, request_id %d, list 0x%x, num_oids %d\n",
		buf, bufsize, request_id, list, num_oids);
#endif

	if (bufsize < SNMP_HEADER_SIZE)
		return 0; 

	/* Byte counts for PDU SEQUENCE,  GET REQUEST and SEQUENCE of 
	 * GET REQUEST are always coded as count_size "multi bytes", so it
	 * need not change for larger requests.
	 */   
	buf[i++] = SNMP_SEQUENCE; 
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
	pdu_byte_count_index = i;
	i += count_size;
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;	
	buf[i++] = 0;	
	buf[i++] = SNMP_OCTET_STR;
	buf[i++] = sizeof(community_name);
	memcpy(&buf[i], community_name, sizeof(community_name));
	i += sizeof(community_name);
	buf[i++] = SNMP_GET_REQUEST; 
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
#ifdef DEBUG
	printf("get_request_byte_count_index %d\n", i);
#endif
	get_request_byte_count_index = i;
	i += count_size;
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = request_id; 
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = 0;	/* no error */ 
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = 0;	/* null error index */ 
	buf[i++] = SNMP_SEQUENCE;
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
#ifdef DEBUG
	printf("sequence_byte_count_index %d\n", i);
#endif
	sequence_byte_count_index = i;
	i += count_size;
	for (oid_count = 0; oid_count < num_oids; oid_count++) {
		int len = list[oid_count].id_length;
		int j;
#ifdef DEBUG
		printf("object id: ");
		for (j = 0; j < len; j++)
			printf("%d.", list[oid_count].id[j]);
		printf("\n");
#endif

		if (i + len + ASC_PREFIX_LENGTH + 4 > bufsize) {
			printf("i %d, len %d, asc_length %d, bufsize %d\n",
			i, len, ASC_PREFIX_LENGTH, bufsize);
			return 0; 
		}
		buf[i++] = SNMP_SEQUENCE;
		/* 2 bytes + OID length + 2 bytes NULL */
		buf[i++] = 2 + ASC_PREFIX_LENGTH + len + 2; 
		buf[i++] = SNMP_OBJECT_ID;
		buf[i++] = ASC_PREFIX_LENGTH + len;		
		memcpy(&buf[i], asc_oid_prefix, ASC_PREFIX_LENGTH); 
		i += ASC_PREFIX_LENGTH;
		memcpy(&buf[i], list[oid_count].id, len);
		i += len;
		buf[i++] = SNMP_NULL;
		buf[i++] = 0;
	}	
#ifdef DEBUG
	printf("total byte count %d\n", i);
#endif
	byte_count = i - (pdu_byte_count_index + count_size);
	store_byte_count(buf, pdu_byte_count_index, count_size, byte_count);
	byte_count = i - (get_request_byte_count_index + count_size);
	store_byte_count(buf, get_request_byte_count_index, count_size, byte_count);
	byte_count = i - (sequence_byte_count_index + count_size);
	store_byte_count(buf, sequence_byte_count_index, count_size, byte_count);
	return (i);
}

/** Places a list of OIDs (object ids) from the asc (automated signal
 *  controller) subtree into the supplied buffer as a Set Request packet.
 *  Returns the number of bytes placed in buf.
 */ 

int fmt_asc_set_request(unsigned char *buf, int bufsize,
	 unsigned char request_id, asc_oid *list, int num_oids)
{
	int i = 0;
	int oid_count;
	int byte_count;
	int pdu_byte_count_index;
	int get_request_byte_count_index;
	int sequence_byte_count_index;
	int count_size=2; /* can make one if all get-requests less than 255 bytes */

#ifdef DEBUG
	printf("buf 0x%x, bufsize %d, request_id %d, list 0x%x, num_oids %d\n",
		buf, bufsize, request_id, list, num_oids);
#endif

	if (bufsize < SNMP_HEADER_SIZE)
		return 0; 

	/* Byte counts for PDU SEQUENCE,  GET REQUEST and SEQUENCE of 
	 * GET REQUEST are always coded as count_size "multi bytes", so it
	 * need not change for larger requests.
	 */   
	buf[i++] = SNMP_SEQUENCE; 
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
	pdu_byte_count_index = i;
	i += count_size;
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;	
	buf[i++] = 0;	
	buf[i++] = SNMP_OCTET_STR;
	buf[i++] = sizeof(community_name);
	memcpy(&buf[i], community_name, sizeof(community_name));
	i += sizeof(community_name);
	buf[i++] = SNMP_SET_REQUEST; 
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
#ifdef DEBUG
	printf("get_request_byte_count_index %d\n", i);
#endif
	get_request_byte_count_index = i;
	i += count_size;
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = request_id; 
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = 0;	/* no error */ 
	buf[i++] = SNMP_INTEGER;
	buf[i++] = 1;
	buf[i++] = 0;	/* null error index */ 
	buf[i++] = SNMP_SEQUENCE;
	buf[i++] = SNMP_MULTI_BYTE_COUNT | count_size;
#ifdef DEBUG
	printf("sequence_byte_count_index %d\n", i);
#endif
	sequence_byte_count_index = i;
	i += count_size;
	for (oid_count = 0; oid_count < num_oids; oid_count++) {
		int len = list[oid_count].id_length;
		int j;
#ifdef DEBUG
		printf("object id: ");
		for (j = 0; j < len; j++)
			printf("%d.", list[oid_count].id[j]);
		printf("\n");
#endif

		if (i + len + ASC_PREFIX_LENGTH + 4 > bufsize) {
			printf("i %d, len %d, asc_length %d, bufsize %d\n",
			i, len, ASC_PREFIX_LENGTH, bufsize);
			return 0; 
		}
		buf[i++] = SNMP_SEQUENCE;
		/* 2 bytes + OID length + 2 bytes NULL */
		buf[i++] = 2 + ASC_PREFIX_LENGTH + len + 2; 
		buf[i++] = SNMP_OBJECT_ID;
		buf[i++] = ASC_PREFIX_LENGTH + len;		
		memcpy(&buf[i], asc_oid_prefix, ASC_PREFIX_LENGTH); 
		i += ASC_PREFIX_LENGTH;
		memcpy(&buf[i], list[oid_count].id, len);
		i += len;
		buf[i++] = SNMP_INTEGER;
		buf[i++] = 1;	/* only 1 byte integer for now */
		buf[i++] = list[oid_count].val;
	}	
#ifdef DEBUG
	printf("total byte count %d\n", i);
#endif
	byte_count = i - (pdu_byte_count_index + count_size);
	store_byte_count(buf, pdu_byte_count_index, count_size, byte_count);
	byte_count = i - (get_request_byte_count_index + count_size);
	store_byte_count(buf, get_request_byte_count_index, count_size, byte_count);
	byte_count = i - (sequence_byte_count_index + count_size);
	store_byte_count(buf, sequence_byte_count_index, count_size, byte_count);
	return (i);
}

static void store_byte_count(char *buf, int index, int count_size, int byte_count)
{
	switch (count_size) {
	case 1:
		buf[index] = byte_count;
		break;
	case 2:
		buf[index] = HIGH_BYTE(byte_count);
		buf[index+1] = LOW_BYTE(byte_count);
		break;
	default:
		fprintf(stderr,"Error: store byte count size greater than 2!\n");
	}
} 
