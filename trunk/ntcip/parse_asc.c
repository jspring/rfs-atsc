/**\file
 *
 *	parse_asc.c	Parsing routines for getting simple
 *			messages from the Management Information
 *			Base (MIB) of the 2070 Actuated Signal Controller
 *			(ASC) using NTCIP's Simple Transportation
 *			Management Framework (STMF) protocol.
 *
 * Copyright (c) 2006   Regents of the University of California
 *
 */

#include <sys_os.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include "local.h"
#include "sys_rt.h"
#include "db_clt.h"
#include "db_utils.h"
#include "clt_vars.h"
#include "timestamp.h"
#include "mibdefs.h"
#include "atsc.h"

/* change to define and remake to get debugging information */
#undef DEBUG

/** Called when an SNTP INTEGER type is expected.
 *  Returns value of integer. Index into buffer (pindex) is updated.
 */
unsigned int get_integer_val(unsigned char *buf, int *pindex)
{
	int i = *pindex;
	int j;
	int count;
	unsigned int val = 0;
	if (buf[i++] != SNMP_INTEGER)
		return -1;	/* CHECK STANDARD FOR RANGE */ 
	count = buf[i++];
	/* WILL FAIL FOR count > 4 */
	for (j = 0; j < count; j++)
		val = (val << 8) | buf[i++];
	*pindex = i;
	return val;
}

/** Called when an SNMP sequence is expected (also used for SNMP_GET_RESPONSE). 
 * Returns 0 if no SNMP_SEQUENCE code, or if the value of the
 * bytes remaining in the message, as passed into the parse 
 * routine from the higher-level message capture routine, is
 * less than the byte count for the sequence.
 * Otherwise returns the number of bytes in the sequence.
 */ 
unsigned int get_sequence_count(unsigned char *buf, int *pindex,
	int msg_byte_count)
{
	int i = *pindex;
	unsigned int retval = 0;
	unsigned char tmp;

	tmp = buf[i++];

	if ((tmp != SNMP_SEQUENCE) & (tmp != SNMP_GET_RESPONSE)) {
		 printf("not start of sequence: %02x\n", tmp);
		 return 0;
	}
	tmp = buf[i++];
	if (!(tmp & SNMP_MULTI_BYTE_COUNT)) {
		retval = tmp;
	} else if (tmp & 1) {
		retval = buf[i++];
	} else if (tmp & 2) { 
		retval = (buf[i] << 8) | buf[i+1];
		i += 2;
	}
	*pindex = i;
	if (retval > msg_byte_count - i) {
		printf("sequence count  %d > byte count %d - index %d\n",
			retval, msg_byte_count, i);
		return 0;
	}
	return retval;
}

/** Returns 0 if parse error, otherwise 1 */
int read_version_number(unsigned char * buf, int *pindex, int byte_count)
{
	unsigned int val;
	val = get_integer_val(buf, pindex);
#ifdef DEBUG
	printf("Version number %d\n", val);
#endif
	if (*pindex > byte_count)
		return 0;
	return 1;
}

/** Returns 0 if parse error, otherwise 1 */
int read_community_name(unsigned char *buf, int *pindex, int byte_count)
{
	int i = *pindex;
	int j;
	int count;
	char name[255];

	if (buf[i++] != SNMP_OCTET_STR)
		return 0;
	count = buf[i++];
	for (j = 0; j < count; j++)
		name[j] = buf[i++];
	name[j] = '\0';
#ifdef DEBUG
	printf("Community Name %s\n", name);
#endif
	if (i > byte_count)
		return 0;
	*pindex = i;
	return 1;
}

/** Error status and index are printed if non-zero, but otherwise ignored.
 *  Later may add error handling, request id matching, with 0 error return.
 *  Returns 1.
 */
int read_request_status(unsigned char *buf, int *pindex, int byte_count)
{
	unsigned int val;

	val = get_integer_val(buf, pindex);
#ifdef DEBUG
	printf("Request id %d\n", val);
#endif

	val = get_integer_val(buf, pindex);
	if (val) printf("Error status %d\n", val);

	val = get_integer_val(buf, pindex);
	if (val) printf("Error index %d\n", val);

	return 1;
}

/** Parses an object id, checking for known ids in the list, and
 * returns the type of the object.
 */
int parse_oid(unsigned char *buf, int *pindex, asc_oid *list, 
	int num_oids, int *pinstance)
{
	int i = *pindex;
	int j;
	int object_type = 0;
	int count;
	char name[255];	/* buffer for string comparisons */

	*pinstance = 0;
	if (buf[i++] != SNMP_OBJECT_ID)
		return 0;
	count = buf[i++];	

	/* First read the prefix common to all ASC objects */
	for (j = 0; j < ASC_PREFIX_LENGTH; j++)
		name[j] = buf[i++];
	name[j] = '\0';
	if (strncmp(name, asc_oid_prefix, 255) != 0)
		return UNKNOWN_OBJECT_TYPE; 

	/* Then read the rest */
	for (j = 0; j < count - ASC_PREFIX_LENGTH; j++)
		name[j] = buf[i++];
	name[j] = '\0';

	/* Finished reading from buffer */
	*pindex = i;

	/* Search for object id suffix on list */ 
	for (j = 0; j < num_oids; j++) {
		if (strncmp(name, list[j].id, 255) == 0) {
			object_type = list[j].type;
			//set instance to value of last byte 
			//this will be don't care if not group type
			*pinstance = list[j].id[list[j].id_length - 1];
			break;
		}	
	}
	if (j == num_oids)
		return UNKNOWN_OBJECT_TYPE;
	return object_type;
}
	
void parse_asc_integer_group_entry(unsigned char *buf, int *pindex,
	int object_type, int instance, atsc_typ *asc)
{
	unsigned int val;

	val = get_integer_val(buf, pindex);

	switch (object_type) 
	{
	case DETECTOR_STATUS_GROUP:
		asc->vehicle_detector_status[instance-1] = val;
#ifdef DEBUG
		printf("detector status group: instance %d, val 0x%02x\n",
				instance, val);
#endif
		break;
	case PHASE_STATUS_GROUP_NUMBER:
		/* don't store for now, may decide we need later */
		break;
	case PHASE_STATUS_GROUP_REDS:
		asc->phase_status_reds[instance-1] = val;
		break;
	case PHASE_STATUS_GROUP_YELLOWS:
		asc->phase_status_yellows[instance-1] = val;
		break;
	case PHASE_STATUS_GROUP_GREENS:
		asc->phase_status_greens[instance-1] = val;
		break;
	case PHASE_STATUS_GROUP_ONS:
		asc->phase_status_ons[instance-1] = val;
		break;
	case PHASE_STATUS_GROUP_NEXT:
		asc->phase_status_next[instance-1] = val;
		break;
	default:
		fprintf(stderr, "Unrecognized object ID\n");
		break;
	}
}

/** Parses an object id and calls the appropriate function to parse
 *  and write to the database if the object type is recognized.
 *  Returns 0 if parse error, 1 otherwise.
 */
int parse_asc_object(unsigned char *buf, int *pindex, int byte_count,
	asc_oid *list, int num_oids, atsc_typ *asc)
{
	int asc_object_type;
	int instance;	/* instance number of object */ 
	int sequence_count;

	sequence_count = get_sequence_count(buf, pindex, byte_count);
	if (!sequence_count)
		return 0;

	asc_object_type = parse_oid(buf, pindex, list, num_oids, &instance);
	if (asc_object_type == UNKNOWN_OBJECT_TYPE) {
		printf("parsed unknown object type\n");
		return 0;
	}

	/* for now we are only parsing group entries of type integer,
	 * later can add non-group integer and other types, check
	 * object_type to decide which to call.
	 */
	parse_asc_integer_group_entry(buf, pindex, asc_object_type,
			 instance, asc);
	return 1;
}
	
/** Parses a Get Response packet from the ASC looking for a list of OIDS.
 *  Writes values of objects which are recognized to variables in the
 *  PATH publish/subscribe database.
 *  	buf -- must be start of SNMP message
 *	byte_count -- bytes found by higher-level in the message
 *	list -- list of object id structures
 *	numoids -- number of objects in the list
 *  Assumes any occurrences of escape characters have been collapsed
 *  to original values.
 *  Returns 0 if any parse errors are found.
 *  Returns 1 otherwise.
 *  Side effects: database variable summarizing signal type is updated
 */ 
int parse_asc_get_response(unsigned char *buf, int byte_count, 
	asc_oid *list, int num_oids, db_clt_typ *pclt)
{
	int i = 0;
	int sequence_count;
	atsc_typ asc;		/* to use for parsing */
	int j;

	for (j = 0; j < MAX_VEHICLE_DETECTOR_GROUPS; j++)
		asc.vehicle_detector_status[j] = 0;
	for (j = 0; j < MAX_PHASE_STATUS_GROUPS; j++) {
		asc.phase_status_reds[j] = 0;
		asc.phase_status_yellows[j] = 0;
		asc.phase_status_greens[j] = 0;
		asc.phase_status_ons[j] = 0;
		asc.phase_status_next[j] = 0;
	}
 
	/* SNMP header */
	sequence_count = get_sequence_count(buf, &i, byte_count);
	if (!sequence_count) {
		printf("parse get response: error first sequence count\n");
		return 0;
	}
	if (!read_version_number(buf, &i, byte_count)){
		printf("parse get response: error version number\n");
		return 0;
	}
	if (!read_community_name(buf, &i, byte_count)){
		printf("parse get response: error community name \n");
		return 0;
	}

	/* Expect Get Response */
	if (buf[i] != SNMP_GET_RESPONSE) {
		printf("parse get response: error get response \n");
		return 0;
	}
	sequence_count = get_sequence_count(buf, &i, byte_count);
	if (!sequence_count) {
		printf("parse get response: error second sequence count\n");
		return 0;
	}
	if (!read_request_status(buf, &i, byte_count)) {
		printf("parse get response: error request status\n");
		return 0;
	}

	/* Start of variable binding of objects */
	sequence_count = get_sequence_count(buf, &i, byte_count);
	if (!sequence_count) {
		printf("parse get response: error third sequence count\n");
		return 0;
	}
	
	while (i < byte_count) {
		if (!parse_asc_object(buf, &i, byte_count, list, num_oids,
				&asc)) {
			printf("parse get response: error asc object\n");
			return 0;
		}
	}
#ifdef DEBUG
	printf("phase status reds 0x%x\n", asc.phase_status_reds[0]);	
	printf("phase status yellows 0x%x\n", asc.phase_status_yellows[0]);	
	printf("phase status greens 0x%x\n", asc.phase_status_greens[0]);	
	printf("phase status ons 0x%x\n", asc.phase_status_ons[0]);	
	printf("phase status next 0x%x\n", asc.phase_status_next[0]);	
	fflush(stdout);
#endif

	asc.info_source = ATSC_SOURCE_NTCIP;	
	//write_asc_to_database(pclt, &asc);
#ifdef COMPARE
	if (clt_update(pclt, DB_ATSC2_VAR, DB_ATSC2_TYPE,
                        sizeof(atsc_typ), (void *) &asc) == FALSE)
                fprintf(stderr, "clt_update(DB_ATSC2_VAR)\n" );
#else
	if (clt_update(pclt, DB_ATSC_VAR, DB_ATSC_TYPE,
                        sizeof(atsc_typ), (void *) &asc) == FALSE)
                fprintf(stderr, "clt_update(DB_ATSC_VAR)\n" );
#endif

	return 1;	
}

