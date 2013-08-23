#include "tos.h"
#include "ab3418_lib.h"

int tos_get_detector_request(int fd, int verbose) {
	int msg_len;
	int retval;
	int index;
	char rcvbuf[200];
	tos_get_detector_request_msg_t tos_get_detector_request;

	tos_get_detector_request.start_flag = 0x7e;
	tos_get_detector_request.address = 0x05;
	tos_get_detector_request.comm_code = 0x0d;
	tos_get_detector_request.FCSmsb = 0x00;
	tos_get_detector_request.FCSlsb = 0x00;
	tos_get_detector_request.end_flag = 0x7e;

	if(fd <= 0)
		return -1;
	/* Now append the FCS. */
	msg_len = sizeof(tos_get_detector_request_msg_t) - 4;
	fcs_hdlc(msg_len, &tos_get_detector_request, verbose);
	write ( fd, &tos_get_detector_request, sizeof(tos_get_detector_request_msg_t));

	retval = read(fd, rcvbuf, 200);
	if(retval <= 0) {
		perror("tos_get_detector_request read");
		return -2;
	}
	printf("\n\ntos_get_detector_request return message:"); 
	for(index = 0; index < retval; index++)
		printf(" %#hhx", rcvbuf[index]);
	printf("\n\n");	
	return 0;
}
