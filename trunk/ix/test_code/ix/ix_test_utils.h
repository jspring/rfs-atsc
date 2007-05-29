/**\file
 * 	Header file for routines that read or write intersection
 *	message sets from/to a file for testing purposes. Actual code
 *	in an RSU will use the PATH "data bucket" code for interprocess
 *	communication (see ix_db_utils.h) 
 *
 *	Structures and types from the "data bucket" code that appear
 *	outside of ix_db_utils.c are given definitions compatible
 *	with file implementations, so identical source code can be used for
 *	off-line testing, with only a difference in the Makefile.
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */
#ifndef IX_TEST_UTILS_H
#define IX_TEST_UTILS_H

/* Type used for creating tables of database variables and their size
 */
typedef struct {
	int id;
	int size;
} db_id_t;

typedef FILE db_clt_typ;

// If not using the databucket, this is a no-op except for opening a file;
// the xport variable normally used to set the transport mode for the
// data bucket server is overloaded to indicate opening the file for write
// if true and read if false. snd_ix_msg_test will read from a file, 
// rcv_ix_msg_test will write to a file, no process will do both.

static inline db_clt_typ *clt_login(char * prog, char * hostname, char * domain,
				int xport)
{
	FILE *fp;
	if (xport)
		fp = fopen("rcv_test_phases.out", "w");
	else
		fp = fopen("snd_test_phases.in", "r");
}

static inline void clt_logout(db_clt_typ *pclt)
{
	if (pclt != NULL)
		fclose(pclt);
}

static inline db_clt_typ *db_list_init(char *prog, char * hostname,
	 char *domain, int xport, db_id_t *varlist, int numvars,
                        int *triglist, int numtrigs)
{
	FILE *fp;
	fp = clt_login(prog,hostname, domain, xport);
	return fp;
}
	

static inline void db_list_done(db_clt_typ *pclt, db_id_t *varlist, int numvars,
	int *triglist, int numtrigs)
{
	if (pclt != NULL)
		fclose(pclt);
}

// no-op
#define get_local_name(hostname, length) 

// database variable definitions added for compile compatibility, not
// used in TEST_ONLY mode
#define DB_IX_MSG_VAR 0
#define DB_IX_APPROACH1_VAR 0
#define DB_RX_IX_MSG_VAR 0
#define DB_RX_IX_APPROACH1_VAR 0
#define DB_RX_IX_APPROACH2_VAR 0
#define DB_RX_IX_APPROACH3_VAR 0
#define DB_RX_IX_APPROACH4_VAR 0
#define DB_RX_IX_APPROACH5_VAR 0
#define DB_RX_IX_APPROACH6_VAR 0
#define DB_RX_IX_APPROACH7_VAR 0
#define DB_RX_IX_APPROACH8_VAR 0
#define DB_RX_IX_APPROACH9_VAR 0
#define DB_RX_IX_APPROACH10_VAR 0
#define MAXHOSTNAMELEN	80
#define DEFAULT_SERVICE "none"
#define COMM_PSX_XPORT 3

// instead of using real-time timers, just use usleep
typedef int posix_timer_typ;
// interval is in millliseconds
static inline posix_timer_typ *timer_init(int interval, int dummy)
{
	int *ptmr = (int *) malloc(sizeof(int));
	*ptmr = interval;
	return(ptmr);
}
#define TIMER_WAIT(ptmr) usleep((*ptmr)*1000)
#endif
