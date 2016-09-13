/* urms_db_test.c - Sets URMS db variable
**
*/

#include "urms.h"

#define BUF_SIZE 500

const char *Usage = "-1 <lane 1 release rate (VPH)>\n\t-2 <lane 1 action>\n\t-3 <lane 1 plan>\n\t-4 <lane 2 release rate (VPH)>\n\t-5 <lane 2 action>\n\t-6 <lane 2 plan>\n\t";

extern int db_set_ramp_metering(db_clt_typ *pclt, db_urms_t *db_urms_msg);

int main(int argc, char *argv[]) {
	db_urms_t db_urms_msg;
	int db_var = 3202;

        int option;

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h

	memset(&db_urms_msg, 0, sizeof(db_urms_t));

        get_local_name(hostname, MAXHOSTNAMELEN);
        if ( (pclt = db_list_init(argv[0], hostname, domain,
            xport, NULL, 0, NULL, 0)) == NULL){ 
            exit(EXIT_FAILURE);
	}

	db_clt_read(pclt, db_var, sizeof(db_urms_t), &db_urms_msg);

        while ((option = getopt(argc, argv, "1:2:3:4:5:6:7:8:9:A:B:C:d:")) != EOF) {
                switch(option) {
                case '1':
			db_urms_msg.lane_1_release_rate = atoi(optarg);
                        break;
                case '2':
			db_urms_msg.lane_1_action = atoi(optarg);
                        break;
                case '3':
			db_urms_msg.lane_1_plan = atoi(optarg);
                        break;
                case '4':
			db_urms_msg.lane_2_release_rate = atoi(optarg);
                        break;
                case '5':
			db_urms_msg.lane_2_action = atoi(optarg);
                        break;
                case '6':
			db_urms_msg.lane_2_plan = atoi(optarg);
                        break;
                case '7':
			db_urms_msg.lane_3_release_rate = atoi(optarg);
                        break;
                case '8':
			db_urms_msg.lane_3_action = atoi(optarg);
                        break;
                case '9':
			db_urms_msg.lane_4_plan = atoi(optarg);
                        break;
                case 'A':
			db_urms_msg.lane_4_release_rate = atoi(optarg);
                        break;
                case 'B':
			db_urms_msg.lane_4_action = atoi(optarg);
                        break;
                case 'C':
			db_urms_msg.lane_3_plan = atoi(optarg);
                        break;
                case 'd':
			db_var = atoi(optarg) + 2;
                        break;
                default:
                        printf("Usage: %s %s\n", argv[0], Usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }
	printf("db_var %d Lane 1: rate %d action %d plan %d Lane 2: rate %d action %d plan %d\n",
		db_var,
		db_urms_msg.lane_1_release_rate,
		db_urms_msg.lane_1_action,
		db_urms_msg.lane_1_plan,
		db_urms_msg.lane_2_release_rate,
		db_urms_msg.lane_2_action,
		db_urms_msg.lane_2_plan
	);
	db_clt_write(pclt, db_var, sizeof(db_urms_t), &db_urms_msg);
}
