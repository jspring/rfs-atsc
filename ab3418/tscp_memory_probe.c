/* ab3418test - eventually will be the main ab3418-to-database communicator
*/

#include <db_include.h>
#include "atsc_clt_vars.h"
#include "atsc.h"
#include "msgs.h"
#include "ab3418_lib.h"
#include "ab3418comm.h"

#define MAX_PHASES	7

const char *usage = "-P <port> -u (use db) -v (verbose)\n\nDisplay addresses and contents of memory range\n\t-l <low memory address> -h <high memory address>\n\nDisplay addresses within memory range whose contents match value\n\t-l <low memory address> -h <high memory address> -m <match value>\n\nDisplay addresses within memory range whose contents match value, and substitute new value for old value\n\t-l low memory address> -h <high memory address> -m <match value> -s <substitution value>\n\n";

int main(int argc, char *argv[]) {

	posix_timer_typ *ptmr;

	unsigned short i;
	int j = 0;
	int fpin = 0;
	int fpout = 0;
	db_timing_set_2070_t db_timing_set_2070;
	db_timing_get_2070_t db_timing_get_2070;
	int msg_len;
	int wait_for_data = 1;
	gen_mess_typ readBuff;
	int retval;
	char port[14] = "/dev/ttyUSB0";
	int opt;
	int interval = 200;
	int verbose = 0;

	unsigned short lomem = 0;
	unsigned short himem = 0;
	short num_left = 0;
	short num_bytes = 0;
	
	char match_val = 0;
	char do_match = 0;

	char subst_val = 0;
	char do_subst = 0;

        while ((opt = getopt(argc, argv, "P:vi:l:h:m:s:")) != -1)
        {
                switch (opt)
                {
                  case 'P':
			memset(port, 0, sizeof(port));
                        strncpy(&port[0], optarg, 13);
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  case 'i':
                        interval = atoi(optarg);
                        break;
                  case 'l':
                        lomem = (unsigned short)strtol(optarg, NULL, 0);
                        break;
                  case 'h':
                        himem = (unsigned short)strtol(optarg, NULL, 0);
                        break;
                  case 'm':
                        match_val = (char)strtol(optarg, NULL, 0);
			do_match = 1;
                        break;
                  case 's':
                        subst_val = (char)strtol(optarg, NULL, 0);
			do_subst = 1;
                        break;
		  default:
			fprintf(stderr, "\n\nUsage: %s %s", argv[0], usage);
			exit(EXIT_FAILURE);
		}
	}
	if( himem < lomem ) {
		fprintf(stderr, "\n\nError: High memory address < low memory address\nUsage: %s %s", argv[0], usage);
		exit(EXIT_FAILURE);
	}
		
	if ((ptmr = timer_init( interval, ChannelCreate(0))) == NULL) {
		fprintf(stderr, "Unable to initialize delay timer\n");
		exit(EXIT_FAILURE);
	}

        /* Initialize serial port. */
        fpin = open( port,  O_RDONLY );
	while(fpin <= 0) {
		sprintf(port, "/dev/ttyUSB%d", (j++ % 8));
		fprintf(stderr,"main 0: Trying to open %s\n", port);
       			fpin = open( port,  O_RDONLY );
		if ( fpin <= 0 ) {
			perror("main 00: serial inport open");
			fprintf(stderr, "main 000:Error opening device %s for input\n", port );
		}
		sleep(1);
	}

	/* Set up USB ports */
	char *portcfg = "for x in /dev/ttyUSB*; do /bin/stty -F $x raw 38400;done";
	system(portcfg);

        /* Initialize serial port. */
       fpout = open( port,  O_WRONLY );
        if ( fpout <= 0 ) {
		perror("serial outport open");
                printf( "Error opening device %s for output\n", port );
        }

	num_left = himem - lomem + 1;
#define MAX_BYTES	16	
		while(num_left > 0) {
			memset(&readBuff, 0, sizeof(readBuff));
			if( (num_left / MAX_BYTES) > 0)
				num_bytes = MAX_BYTES;
			else
				num_bytes = num_left % MAX_BYTES;
//printf("lomem %#x num_bytes %d\n", lomem, num_bytes);
			retval = get_mem(lomem, num_bytes, wait_for_data, &readBuff, fpin, fpout, verbose);
			for(i = lomem, j = 0; j < num_bytes; j++, i++) {
				if( (do_match == 0) && (do_subst == 0) ){
					printf("%#x\t%#x\t%d\n", i, readBuff.data[j+8], readBuff.data[j+8]);
				}
				if( (do_match != 0) && (do_subst == 0) ){
					if(readBuff.data[j+8] == match_val)
						printf("%#x\n", i);
				}
				if( (do_match != 0) && (do_subst != 0) ){
					if(readBuff.data[j+8] == match_val) {
						printf("%#x\n", i);
						memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070_t));
						db_timing_set_2070.cell_addr_data.cell_addr = (i & 0xff0f) + 0x10;
						db_timing_set_2070.cell_addr_data.data = subst_val;
						db_timing_set_2070.phase = (unsigned char)((i & 0xf0) >> 4);
						retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
					}
				}
				if( (do_match == 0) && (do_subst != 0) ){
					memset(&db_timing_set_2070, 0, sizeof(db_timing_set_2070_t));
					db_timing_set_2070.cell_addr_data.cell_addr = (i & 0xff0f) + 0x10;
					db_timing_set_2070.cell_addr_data.cell_addr = i;
					db_timing_set_2070.cell_addr_data.data = subst_val;
					db_timing_set_2070.phase = (unsigned char)((i & 0xf0) >> 4);
					db_timing_set_2070.phase = 0;
					retval = set_timing(&db_timing_set_2070, &msg_len, fpin, fpout, verbose);
				}
			}
			num_left -= num_bytes;
			lomem += num_bytes;
			usleep(1000000);
		}
		exit(EXIT_SUCCESS);

}
