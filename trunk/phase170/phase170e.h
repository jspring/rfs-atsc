#define MAX_GE			10
#define Early_Green     2
#define Green_Extension 3

typedef struct{
	int year;
	int month;
	int day;
} date_stamp_typ;

typedef struct{
	int hour;
	int min;
	int sec;
	unsigned short millisec;
} time_stamp_typ;

typedef struct {
	unsigned char ringA_phase;
	unsigned char ringB_phase;
	unsigned char ringA_interval;
	unsigned char ringB_interval;
	unsigned char local_cycle_clock;
	unsigned char master_cycle_clock;
	char force_off_point[8];
	char pattern;
	unsigned char signal_state[8];
	timestamp_t ts;
	struct timespec tp;
} IS_PACKED signal_state_typ;

typedef struct {
	int  requested_busID;
	char priority_type;	
	char request_phase;
	char requested_forceoff;
	timestamp_t ts;
	struct timespec tp;	
} IS_PACKED priority_state_typ;

typedef struct {
	signal_state_typ signal;
	priority_state_typ prio;
	unsigned char priority_status;
	unsigned char bus_time_saved;
	float active_local_cycle_clock;
	float active_master_cycle_clock;	
	float *pactive_onsets;
	unsigned char *psignal_state; 
	float time2next[MAX_PHASES];
	struct timespec trace_time;
} IS_PACKED signal_trace_typ;

double ts2sec(struct timespec *ts);
void cycle_rounding(float *f, float cycle_len);
void get_signal_change_onset(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3]);
unsigned char update_head_color(unsigned char intv);
void update_signal_state(signal_trace_typ *ptracer, signal_status_typ *pinput);
void update_priority_state(signal_trace_typ *ptracer, signal_priority_request_typ *pinput);
void assem_ix_msg(ix_msg_t *pix, ix_approach_t *pappr, E170_timing_typ *ptiming,
	signal_trace_typ *psignal_trace, int verbose, db_clt_typ *pclt, char *progname);
void echo_cfg(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3]);


