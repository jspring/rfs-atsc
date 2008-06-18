#define MAX_SITES 2
#define MAX_PHASES 8
#define MAX_STOPLIES 2
#define MAX_PLANS 5
#define MAX_MOVEMENTS 10

typedef struct {
	int latitude;
	int longitude;
	short altitude;
} IS_PACKED ix_gps_fix;

typedef struct {
	int latitude; 
	int longitude; 
	unsigned short line_length;	//  cm 
	unsigned short orientation;	// degrees clockwise from north
} IS_PACKED ix_stopline_fix;

typedef struct {
	unsigned char approach_type;
	unsigned char control_phase;
	unsigned char no_stoplines;
	ix_stopline_fix stopline_cfg[MAX_STOPLIES];
} IS_PACKED ix_approach_fix;

typedef struct {
	unsigned char permitted_phase[MAX_PHASES];
	unsigned char phase_bf[MAX_PHASES];
	unsigned char phase_af[MAX_PHASES];
	unsigned char phase_swap[MAX_PHASES]; // at RFS_intersection, phase 2 within 170E is actually phase 4 output
	float yellow_intv[MAX_PHASES];
	float allred_intv[MAX_PHASES];
} IS_PACKED ix_phase_timing_fix;

typedef struct {
	unsigned char plan_activated; // 0 for false, 1 for ture 
	unsigned char control_type; // 0 for free, 1 for fixed-time, 2 for coordinated
	unsigned char synch_phase; // the phase that terminates at the yield point
	char *lag_phases;
	float cycle_length;
	float offset;
	float grn_splits[MAX_PHASES];
	float force_off[MAX_PHASES];
} IS_PACKED ix_control_plan_fix;
	
typedef struct{
	unsigned int list_id;
	char *name;
	unsigned int intersection_id;
	unsigned int map_node_id;
	ix_gps_fix center;
	ix_gps_fix antenna;
	unsigned char total_no_approaches;
	unsigned char total_no_stoplines;	
	ix_approach_fix approch[MAX_MOVEMENTS];
	ix_phase_timing_fix phase_timing;
	ix_control_plan_fix plan_timing[MAX_PLANS];
} IS_PACKED E170_timing_typ;
		
E170_timing_typ signal_timing_cfg[MAX_SITES] = {
	{1,"RFS_Intersection",1,1,{379155555,-1223348500,5},{379155555,-1223348500,6},4,4,
		{	{1,2,1,{{379156194,-1223348805,400,  0}}},{1,4,1,{{379159083,-1223350111,300,270}}},
			{1,2,1,{{379154972,-1223348777,400,180}}},{1,4,1,{{379155388,-1223347666,600, 90}}}
		},//approch_array
		{	{0,1,0,1,0,0,0,0},{0,4,0,2,0,0,0,0},{0,4,0,2,0,0,0,0},{0,4,0,2,0,0,0,0},
			{0.0,4.0,0.0,4.0,0.0,0.0,0.0,0.0},{0.0,2.0,0.0,2.0,0.0,0.0,0.0,0.0}
		}, // phase_timing
		{	{0,0,2,"2468",60,0}, // plan_timing
			{1,1,2,"2468",72,0,{0.0,15.0,0.0,45.0,0.0,0.0,0.0,0.0},{0.0,0.0,0.0,51.0,0.0,0.0,0.0,0.0}}
		}
	}	
};
