/**\file
 *	signal_get.c	Send repeated requests for a set of
 * 			2070 Actuated Signal Controller (ASC) signal state
 *			object ids and record responses to database.
 *
 * Copyright (c) 2005   Regents of the University of California
 *
 */

#include <sys_qnx6.h>
#include <sys/select.h>
#include "local.h"
#include "sys_rt.h"
#include "sys_list.h"
#include "db_clt.h"
#include "timing.h"
#include "clt_vars.h"
#include "ids_io.h"
#include "mibdefs.h"

#undef DEBUG
#undef DEBUG_SCENARIO

jmp_buf env;

static int sig_list[] = 
{
	SIGINT,
	SIGQUIT,
	SIGTERM,
	ERROR,
};

static void sig_hand(int code)	//sig)
{
	longjmp(env, code);	//1);
}


/* Object ID prefix for ASC 1.3.6.1.4.1.1206.4.2.1, null-terminated */
unsigned char asc_oid_prefix[] = {
	0x2b, 0x06, 0x01, 0x04, 0x01, 0x89, 0x36, 0x04, 0x02, 0x01, '\0',
};

/** List of ASC object IDs to request and parse in the response.
 * The "id" character array in the asc_oid structure has the suffix of
 * the full object id, omitting the prefix common to all objects within asc.
 *
 * Later we may want to initialize the lists that are sent to 
 * the format and parse routines from a configuration file, for
 * now they are hard-coded into this array, structure of code assumes
 * only one type of get-request is being sent.
 *
 * Note: last byte in group object ids is instance number, null char
 * termination is added so that standard null-terminated string
 * functions can be used for search. All objects in this list
 * use the full "id" character array, but some objects may be shorter,
 * in that case fill with null characters, set the second "id_length"
 * field correctly to be the number of characters without the null.
 */
asc_oid get_list[] = {
  {{0x01, 0x04, 0x01, 0x01, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_NUMBER, 0},	
  {{0x01, 0x04, 0x01, 0x02, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_REDS, 0},
  {{0x01, 0x04, 0x01, 0x03, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_YELLOWS, 0},
  {{0x01, 0x04, 0x01, 0x04, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_GREENS, 0},	
  {{0x01, 0x04, 0x01, 0x0A, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_ONS, 0},
  {{0x01, 0x04, 0x01, 0x0B, 0x01, '\0'}, 5, PHASE_STATUS_GROUP_NEXT, 0}
};

#define NUM_GET_ASC_OIDS sizeof(get_list)/sizeof(asc_oid)

asc_oid set_list[] = {
  {{0x06, 0x03, 0x01, 0x02, 0x01, '\0'}, 5, PREEMPT_CONTROL_STATE, 0},
};

#define NUM_SET_ASC_OIDS sizeof(set_list)/sizeof(asc_oid)

/* single drop operation for now */
#define DEST_ADDR 	1

#define SET_PREEMPT	1
#define GET_STATE	0

static struct avcs_timing tmg;

/* use to hold message on transmission and reception */
static unsigned char msg_buf[STMF_MAX_PACKET];

/* used by send_message, receive_message and collapse_escape */
static unsigned char delimiter_flag = 0x7e;
static unsigned char escape_char = 0x7d;
static unsigned char escape_delimiter = 0x5e;
static unsigned char escape_escape = 0x5d;

/** Formats a message and sends it out, following PMPP standard for
 *  escaping delimiter bytes..
 */
void send_message(int fd, int set_message)
{
        static int request_id = 0;      /* 0-31, then wrap */
        int data_length = 0;
        int msg_length = 0;
        int i = 0;
        unsigned short check_sum;

        msg_length += fmt_pmpp_header(DEST_ADDR, msg_buf);
#ifdef DEBUG
        printf("pmpp header length %d\n", msg_length);
#endif
        if (set_message)
        {
                data_length = fmt_asc_set_request(
                        &msg_buf[PMPP_HEADER_SIZE],
                        STMF_MAX_PACKET - PMPP_HEADER_SIZE,
                        request_id, set_list, NUM_SET_ASC_OIDS);
        }
        else
        {
                data_length = fmt_asc_get_request(
                        &msg_buf[PMPP_HEADER_SIZE],
                        STMF_MAX_PACKET - PMPP_HEADER_SIZE,
                        request_id, get_list, NUM_GET_ASC_OIDS);
        }
        if (data_length == 0)
        {
                printf("send_message: fmt_asc_get_request failed\n");
                return;
        }
        else
                msg_length += data_length;
#ifdef DEBUG
        printf("msg length before check sum %d\n", msg_length);
#endif

        /* Frame check sequence is computed on all bytes in PMPP
         * message after starting flag and before check sum placement
         */
        check_sum = compute_fcs(&msg_buf[PMPP_ADDR_INDEX], msg_length - 1);
        check_sum ^= 0xffff;
        msg_buf[msg_length++] = LOW_BYTE(check_sum);
        msg_buf[msg_length++] = HIGH_BYTE(check_sum);

        /* write flag byte */
        write(fd, &msg_buf[0], 1);

        /* escape 0x7e and 0x7d when writing to port; for simplicity
         * written byte by byte to serial port
         */
        for (i = 1; i < msg_length; i++)
        {
                if (msg_buf[i] == delimiter_flag)
                {
                        write(fd, &escape_char, 1);
                        write(fd, &escape_delimiter, 1);
                }
                else if (msg_buf[i] == escape_char)
                {
                        write(fd, &escape_char, 1);
                        write(fd, &escape_escape, 1);
                }
                 else
                        write(fd, &msg_buf[i], 1);
        }
        write(fd, &delimiter_flag, 1);
        if (request_id++ == 32)
                request_id = 0;
}

/** Places correct original value of escape sequence in val;
 * improve error handling later, for now don't know what to do
 * after an error if we get one.
 */
void collapse_escape(int fd, unsigned char *val)
{
	unsigned char tmp;

	read(fd, &tmp, 1);
	if (tmp == escape_delimiter)
		*val = delimiter_flag;
	else if (tmp = escape_escape)
		*val = escape_char;
	else	/* ERROR */
		*val = 0xff;
}
	
/* Read in a message, collapsing PMPP escape characters, validating check
 * sum and calling parse routine. Put the entire message, including
 * delimiter flags, into the array buf. Call the parse_asc_response
 * routine only on the SNMP message. Escape characters are possible
 * for the entire PMPP message except the delimiter flags, including
 * the check sum. Check sum is computed on entire preceding bytes
 * in PMPP message, excluding delimiter flags.
 */
int receive_message(int fd, db_clt_typ *pclt)
{
	int i= 0;
	int j;
	unsigned char buf[STMF_MAX_PACKET];
	unsigned char tmp = 0;
	unsigned char byte_code; 
	int byte_count = 0;
	int header_size;
	unsigned short check_sum;

	/* Read delimiter flag */ 
	while (tmp != delimiter_flag)
		read(fd, &tmp, 1);
	buf[i++] = tmp;
	read(fd, &tmp, 1);

	if (tmp == delimiter_flag) {	/* resynced, caught end of previous */
		read(fd, &tmp, 1);	/* read again to get second byte */
	}	
	if (tmp == escape_char)
		collapse_escape(fd, &tmp);
	buf[i++] = tmp;

	while (i < PMPP_HEADER_SIZE)
	{
		read(fd, &tmp, 1);
		if (tmp == escape_char)
			collapse_escape(fd, &tmp);
		buf[i++] = tmp;
	}
#ifdef DEBUG
	printf("PMPP flag %02X, ctrl %02x, ipi %02x\n",
		buf[PMPP_FLAG_INDEX], buf[PMPP_CTRL_INDEX],
		buf[PMPP_IPI_INDEX]);
#endif

	if ((buf[PMPP_FLAG_INDEX] != PMPP_FLAG_VAL) ||
		(buf[PMPP_CTRL_INDEX] != PMPP_CTRL_VAL) ||
		(buf[PMPP_IPI_INDEX] != PMPP_STMF_VAL))
		return 0;

	/* Read SNMP Message header, get byte count for rest of message
	 * excluding check sum */
	read(fd, &tmp, 1);
	if (tmp != SNMP_SEQUENCE)
		return 0;
	buf[i++] = tmp;
	read(fd, &byte_code, 1);
	if (byte_code == escape_char) collapse_escape(fd, &byte_code);
#ifdef DEBUG
	printf("byte_code %02X\n", byte_code);
#endif
	buf[i++] = byte_code;
	if (!(byte_code & SNMP_MULTI_BYTE_COUNT))
	{
                byte_count = byte_code;
		printf("byte_code is byte_count\n");
        }
	else if (byte_code & 1)
	{
		read(fd, &tmp, 1);
		if (tmp == escape_char) collapse_escape(fd, &tmp);
		printf("byte count is %d (%02x)\n", tmp, tmp);
		buf[i++] = tmp;
                byte_count = tmp;
        }
	else if (byte_code & 2)
	{ 
		read(fd, &tmp, 1);
		if (tmp == escape_char) collapse_escape(fd, &tmp);
#ifdef DEBUG
		printf("first byte of count %02x\n", tmp);
#endif
		buf[i++] = tmp;
		read(fd, &tmp, 1);
		if (tmp == escape_char) collapse_escape(fd, &tmp);
#ifdef DEBUG
		printf("second byte of count %02x\n", tmp);
#endif
		buf[i++] = tmp;
                byte_count = (buf[i-2] << 8) | buf[i-1];
        }
	/* Read SNMP packet, using byte count, and two-byte check sum */ 
	header_size = i;	
#ifdef DEBUG
	printf("byte count %d, header_size %d\n", byte_count, header_size);
#endif
	while (i < byte_count + header_size + 2)
	{
		read(fd, &tmp, 1);
		if (tmp == escape_char) collapse_escape(fd, &tmp);
		buf[i++] = tmp;
	}
	check_sum = compute_fcs(&buf[1], byte_count + header_size + 1);
	if (check_sum != PMPP_FCS_MAGIC) 
		printf("2070: check sum error\n");
	else
	{
#ifdef DEBUG
		for (j = 0; j < byte_count + header_size +2; j++)
		{
			if (buf[j] == 0x30)
				printf("\n");
			printf("%02x ", buf[j]);
		}
		printf("\n");
			
#endif
		/* later may parse other message types */
		parse_asc_get_response(&buf[PMPP_HEADER_SIZE],
			byte_count + header_size - PMPP_HEADER_SIZE,
			get_list, NUM_GET_ASC_OIDS, pclt);
	}

	read(fd, &tmp, 1);
	if (tmp != delimiter_flag) 
		printf("2070: no delimiter flag\n");
	return 1;
}

static db_clt_typ *database_init(char *prog, char *domain)
{
        db_clt_typ *pclt;
	asc_2070_typ asc;
	int i;

	if( (pclt = clt_login(prog, NULL, domain, 0 )) == NULL )
		return( NULL );

	printf("Database log in returns %08x\n", pclt);
	if ((clt_create(pclt, DB_ASC_2070_VAR, DB_ASC_2070_TYPE,
                sizeof(asc_2070_typ)) == FALSE))
	{
		printf("asc_2070_var: could not create, already created?\n");
		clt_logout(pclt);
		return(NULL);
        }
	for (i = 0; i < 4; i++)
		asc.signal_light[i] = 0;
	asc.loop_detectors = 0;

        if (clt_update(pclt, DB_ASC_2070_VAR, DB_ASC_2070_TYPE,
                        sizeof(asc_2070_typ), (void *) &asc) == FALSE)
                fprintf(stderr, "error clt_update(DB_ASC_2070_VAR)\n" );
	return(pclt);
}

int main(int argc, char *argv[])
{
	db_clt_typ *pclt;      /* Database pointer */
	int db_num;
	int serial_in;
	int serial_out;
	struct timeval tv;
	int initiator = 1;	/* by default, send first */
	int keep_sending = 1;	/* by default, keep sending after reception */
	int preempt_interval = 500;	/* measured in time for "get" message */
	int preempt_count = 0;
	char ser_name[80];
	fd_set rfd;
	int n;
	int ch;
	int time_out = 5000;	/* in milliseconds */
	char hostname[MAXHOSTNAMELEN+1];
	char *domain = "ids";
	db_data_typ db_data;
	ca_or_minn_typ ca_or_minn;
	int current_scenario = MN_SCENARIO;
	int last_scenario = NO_SCENARIO;	
	int rcv_success;

	strncpy(ser_name, "/dev/ser2", 80);

        while ((ch = getopt(argc, argv, "i:s:t:x:d:")) != EOF)
	{
                switch (ch)
		{
		case 'i': initiator = atoi(optarg);
			  break;
                case 's': strncpy(ser_name, optarg, 80);
                          break;
                case 't': time_out = atoi(optarg);
                          break;
                case 'x': keep_sending = atoi(optarg);
                          break;
		case 'd': domain = strdup(optarg);
			  break;
                default: printf("Usage: %s -s serial port  \n", argv[0]);
                          break;
                }
        }

	printf("%s started, %s\n", argv[0], ser_name); 
	fflush(stdout);

        /* Log in to the database (shared global memory).  Default to the
         * the current host. */
        if ((pclt = database_init(argv[0],domain)) == NULL)
	{
		printf("Database initialization error in %s\n", argv[0]);
		exit (EXIT_FAILURE);;
	}

	avcs_start_timing(&tmg);

	/* Set jmp */
	if (setjmp(env) != 0)
	{
		fflush(stdout);
		/* Log out from the database. */
		if (pclt != NULL)
			clt_logout(pclt);

		avcs_end_timing(&tmg);
		avcs_print_timing("asc_get.tmg", &tmg);
		exit (EXIT_SUCCESS);
	}
	else 
		sig_ign(sig_list, sig_hand);

	if ((serial_in  = open(ser_name, O_RDONLY)) == -1)
	{
		perror("open");
		exit(EXIT_FAILURE);
	}

	if ((serial_out  = open(ser_name, O_WRONLY)) == -1)
	{
		perror("open");
		exit(EXIT_FAILURE);
	}

	tv.tv_sec = time_out / 1000;
	tv.tv_usec = (time_out % 1000) *1000;
	printf("Time out %d sec, %d microsec\n", tv.tv_sec, tv.tv_usec);

	if (initiator == 1) {
		printf("send message to get state\n");
		fflush(stdout);
		send_message(serial_out, GET_STATE);
	}
		
	do
	{
		tmg.exec_num++;
		FD_ZERO(&rfd);
		FD_SET(serial_in, &rfd);
#ifdef DEBUG_SCENARIO
		printf("rfd before select 0x%x\n", rfd);
		fflush(stdout);
#endif
		switch (n = select(1 + serial_in, &rfd, 0, 0, &tv))
		{
		case -1:
			perror("select");
			exit(EXIT_FAILURE);
		case  0:
			//printf("select timed out\n");
			send_message(serial_out, GET_STATE);
			break;
		default:
#ifdef DEBUG_SCENARIO
			printf("data on serial port\n");
			fflush(stdout);
#endif
			if (FD_ISSET(serial_in, &rfd)) {
				//printf("received message\n");
				 rcv_success = receive_message(serial_in, pclt);
				 if (!rcv_success)
					printf("ASC receive error\n");
			} else {				
				send_message(serial_out, GET_STATE);
#ifdef DEBUG_SCENARIO
				printf("Request loop info\n");
				fflush(stdout);
#endif
			}
		}
	} while (keep_sending);

	exit(EXIT_SUCCESS);
}
