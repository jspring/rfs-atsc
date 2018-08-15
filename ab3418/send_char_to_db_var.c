/* send_char_to_db_var.c - well what do you think it does?
*/

#include <db_include.h>
#include "ab3418comm.h"

static db_id_t db_vars_list[] =  {
        {0, sizeof(db_set_pattern_t)},
};
int num_db_vars = sizeof(db_vars_list)/sizeof(db_id_t);

int main(int argc, char *argv[]) {

        char hostname[MAXHOSTNAMELEN];
        char *domain = DEFAULT_SERVICE; /// on Linux sets DB q-file directory
        db_clt_typ *pclt;               /// data server client pointer
        unsigned int xport = COMM_OS_XPORT;      /// value set correctly in sys_os.h
	db_set_pattern_t db_set_pattern;

	int db_var;
	char val;
	
	db_var = atoi(argv[1]);
	db_set_pattern.pattern = (char)atoi(argv[2]);

	db_vars_list[0].id = db_var;

        get_local_name(hostname, MAXHOSTNAMELEN);
        if ( (pclt = db_list_init(argv[0], hostname, domain, xport, NULL, 0, NULL, 0)) == NULL) {
            exit(EXIT_FAILURE);
        }

	printf("db_var %d val %d\n", db_var, db_set_pattern.pattern);

	db_clt_write(pclt, db_vars_list[0].id, db_vars_list[0].size, &db_set_pattern);


	db_list_done(pclt, NULL, 0, NULL, 0);


return 0;
}
