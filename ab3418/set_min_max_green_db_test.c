/* set_min_max_green_db_test.c - command line arguments write the maximum and
** minimum green times for a given phase, to the database
**
**
*/

#include <db_include.h>
#include "set_min_max_green.h"

const char *usage = " -p <phase> -g <minimum green> -G <maximum green> -y <yellow> -R <all_red>\n";

int main( int argc, char *argv[] )
{
        char *domain = DEFAULT_SERVICE;
        int opt;
        int xport = COMM_PSX_XPORT;
        char hostname[MAXHOSTNAMELEN];
        db_clt_typ *pclt;              /* Database client pointer */
	int phase = 0;
	int min_green = -1;
	int max_green = -1;
	int yellow = -1;
	int all_red = -1;
	int verbose = 0;
	unsigned short page = 0;

        while ((opt = getopt(argc, argv, "p:G:g:r:y:R:v")) != -1)
        {
                switch (opt)
                {
                  case 'p':
                        phase = (int)strtol(optarg, NULL, 0);
                        break;
                  case 'G':
                        max_green = (int)strtol(optarg, NULL, 0);
                        break;
                  case 'g':
                        min_green = (int)strtol(optarg, NULL, 0);
                        break;
                  case 'y':
                        yellow = (int)strtol(optarg, NULL, 0);
                        break;
                  case 'R':
                        all_red = (int)strtol(optarg, NULL, 0);
                        break;
                  case 'r':
                        page = strcmp(optarg, "plan") == 0 ? 0x300 : 0x100;
printf("page %hx\n", page);
                        break;
                  case 'v':
                        verbose = 1;
                        break;
                  default:
                        printf("Usage: %s %s", argv[0], usage);
                        exit(1);
                }
        }
	if(phase == 0) {
		printf("Usage: %s %s", argv[0], usage);
		exit(EXIT_FAILURE);
	}
	get_local_name(hostname, MAXHOSTNAMELEN+1);
	if ((pclt = db_list_init( argv[0], hostname, domain, xport,
		NULL, 0, NULL, 0))
		== NULL)
	{
		fprintf(stderr, "Database initialization error in %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if(page != 0) 
		db_get_timing_request(pclt, phase, page);
	else 
		db_set_min_max_green(pclt, phase, min_green, max_green, yellow, all_red, verbose);
	printf("\n");
	clt_logout(pclt);
}
