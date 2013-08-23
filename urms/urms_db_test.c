/* urms_db_test.c - Sets URMS db variable
**
*/

#include "urms.h"

#define BUF_SIZE 500

const char *Usage = "-1 <lane 1 release rate (VPH)>\n\t-2 <lane 1 action>\n\t-3 <lane 1 plan>\n\t-4 <lane 2 release rate (VPH)>\n\t-5 <lane 2 action>\n\t-6 <lane 2 plan>\n\t";

extern int db_set_ramp_metering(db_clt_typ *pclt, db_urms_t *db_urms_msg);

int main(int argc, char *argv[]) {
	db_urms_t db_urms_msg;

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
	db_clt_read(pclt, DB_URMS_VAR, sizeof(db_urms_t), &db_urms_msg);


        while ((option = getopt(argc, argv, "1:2:3:4:5:6:")) != EOF) {
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
                default:
                        printf("Usage: %s %s\n", argv[0], Usage);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

	return db_set_ramp_metering(pclt, &db_urms_msg);
}
