#define MAX_GE			10
#define Early_Green     2
#define Green_Extension 3

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

// function to roud the clock when passing the cycle end
void cycle_rounding(float *f, float cycle_len)
{
	if ( *f >= cycle_len)
		*f -= cycle_len;
	return;
}
				  
// function to convert struct timespec into double
double ts2sec(struct timespec *ts)
{
	return ( ts->tv_sec + ((double) ts->tv_nsec/1000000000L) );
}

// function to calculate the onset of signal state change for each phase and each control plan
void get_signal_change_onset(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3])
{
	int i,j,k;
	float f;
	// plan 0 is running free, and there is no pre-determined onset
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 0)
			continue; // not in use plan
		for (j=0;j<MAX_PHASES;j++)
		{
			if (ptiming->phase_timing.permitted_phase[j] == 0)
				continue; // not in use phase
			k = ptiming->phase_timing.phase_bf[j] - 1; // k is the previous phase
			f = ptiming->plan_timing[i].force_off[k] +
				ptiming->phase_timing.yellow_intv[k] +
				ptiming->phase_timing.allred_intv[k];
			cycle_rounding(&f, ptiming->plan_timing[i].cycle_length);	
			onsets[i][j][0] = f; // green start
			onsets[i][j][1] = ptiming->plan_timing[i].force_off[j]; // yellow start
			onsets[i][j][2] = onsets[i][j][1] + ptiming->phase_timing.yellow_intv[j];
			cycle_rounding(&onsets[i][j][2], ptiming->plan_timing[i].cycle_length);
		}		
	}
	return;
}	

// function to get the signal head color for active phase
unsigned char update_head_color(unsigned char intv)
{
	if ( intv <= 7) return (SIGNAL_STATE_GREEN);
	else if ( intv >= 12 && intv <= 14) return (SIGNAL_STATE_YELLOW);
	else return (SIGNAL_STATE_RED);
}

// function to update signal status upon db trigger
void update_signal_state(signal_trace_typ *ptracer, signal_status_typ *pinput)
{
	int i,active_phase,active_intv;
	int active_color;
	ptracer->signal.ringA_phase = pinput->ringA_phase;
	ptracer->signal.ringB_phase = pinput->ringB_phase;
	ptracer->signal.ringA_interval = pinput->ringA_interval;
	ptracer->signal.ringB_interval = pinput->ringB_interval;
	ptracer->signal.local_cycle_clock = pinput->local_cycle_clock;
	ptracer->signal.master_cycle_clock = pinput->master_cycle_clock;
	for (i=0;i<8;i++)
	{
		ptracer->signal.force_off_point[i] = pinput->force_off_point[i];
	}	
	ptracer->signal.pattern = pinput->pattern;
	ptracer->signal.ts.hour = pinput->hour;
	ptracer->signal.ts.min = pinput->min;
	ptracer->signal.ts.sec = pinput->sec;
	ptracer->signal.ts.millisec = pinput->millisec;
	// get the head color for each phase
	for (i=0;i<8;i++)
	{
		ptracer->signal.signal_state[i] = SIGNAL_STATE_RED;
	}
	if (pinput->ringA_phase > 0)
	{
		ptracer->signal.signal_state[pinput->ringA_phase - 1] = 
			update_head_color(pinput->ringA_interval);
	}
	if (pinput->ringB_phase > 0)
	{
		ptracer->signal.signal_state[pinput->ringB_phase - 1] = 
			update_head_color(pinput->ringB_interval);
	}
	return;
}

// function to update priority status upon db trigger
void update_priority_state(signal_trace_typ *ptracer, signal_priority_request_typ *pinput)
{
	ptracer->prio.requested_busID = pinput->requested_busID;
	ptracer->prio.priority_type = pinput->requested_type;
	ptracer->prio.request_phase = pinput->execution_phase;
	ptracer->prio.requested_forceoff = pinput->desired_force_off;
	ptracer->prio.ts.hour = pinput->hour;
	ptracer->prio.ts.min = pinput->min;
	ptracer->prio.ts.sec = pinput->sec;
	ptracer->prio.ts.millisec = pinput->millisec;
	ptracer->priority_status = pinput->requested_type;
	return;
}

// function to assemble ix_msg and write to database 
void assem_ix_msg(ix_msg_t *pix, ix_approach_t *pappr, E170_timing_typ *ptiming,
				  signal_trace_typ *psignal_trace, int verbose, db_clt_typ *pclt, char *progname)
{
	ix_stop_line_t *pstop;	// Stop line info 
	int i,j,k;
	// Fill in the message structure
	memset(pix,0,sizeof(ix_msg_t));
	pix->flag = 0x7e;	// identify this as a message
	pix->version = 0x2;	// message version
	pix->message_length = (unsigned short)( sizeof(ix_msg_t)-IX_POINTER_SIZE +
		ptiming->total_no_approaches * (sizeof(ix_approach_t) - IX_POINTER_SIZE) +
		ptiming->total_no_stoplines * sizeof(ix_stop_line_t) );
	pix->message_type = 0;
	pix->control_field = 0;
	pix->ipi_byte = 0;
	pix->intersection_id = ptiming->intersection_id;
	pix->map_node_id = ptiming->map_node_id;
	pix->ix_center_lat = ptiming->center.latitude;		
	pix->ix_center_long = ptiming->center.longitude; 
	pix->ix_center_alt = ptiming->center.altitude;
	pix->antenna_lat = ptiming->antenna.latitude;    		
	pix->antenna_long = ptiming->antenna.longitude;
	pix->antenna_alt = ptiming->antenna.altitude;
	pix->seconds = (unsigned int)psignal_trace->trace_time.tv_sec;
	pix->nanosecs = psignal_trace->trace_time.tv_nsec;
	pix->cabinet_err = 0;
	pix->preempt_calls = 0;
	pix->bus_priority_calls = psignal_trace->priority_status;
	pix->preempt_state = 0;
	pix->special_alarm = 0;
	for( i=0;i<4;i++ )
	{
		pix->reserved[i] = 0;
	}
	// fill in reserve[0] with bus phase and reserve[1] with bus time saved
	// when the traffic controller is under priority
	if ( psignal_trace->priority_status != 0)
	{
		pix->reserved[0] = 2;
		pix->reserved[1] = psignal_trace->bus_time_saved;
	}	
	pix->num_approaches = ptiming->total_no_approaches;
	// Fill in the approach structure 
	memset(pappr,0,pix->num_approaches * sizeof(ix_approach_t));
	pix->approach_array = pappr;
	for (i=0;i<pix->num_approaches;i++) 
	{
		pappr[i].approach_type = ptiming->approch[i].approach_type;
		j = ptiming->approch[i].control_phase-1; // signal phase that controls the approach
		k = ptiming->phase_timing.phase_swap[j] - 1;
		pappr[i].signal_state = *(psignal_trace->psignal_state + k);
		pappr[i].time_to_next = (short)(10*psignal_trace->time2next[k]);
		pappr[i].vehicles_detected = 0;
		pappr[i].ped_signal_state = 0;  
		pappr[i].seconds_to_ped_signal_state_change = 255;
		pappr[i].ped_detected = 0;    
		pappr[i].seconds_since_ped_detect = 255;
		pappr[i].seconds_since_ped_phase_started = 255;
		pappr[i].emergency_vehicle_approach = 0;
		pappr[i].seconds_until_light_rail = 255;
		pappr[i].high_priority_freight_train = 0;
		pappr[i].vehicle_stopped_in_ix = 0;
		for(j=0;j<2;j++)
		{
			pappr[i].reserved[j] = 0;
		}
		pappr[i].number_of_stop_lines = ptiming->approch[i].no_stoplines;
		// Fill in the stop line structure 
		pstop = malloc(pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
		memset(pstop,0,pappr[i].number_of_stop_lines * sizeof(ix_stop_line_t));
		pappr[i].stop_line_array = pstop;
		for( j=0;j<pappr[i].number_of_stop_lines;j++ )
		{
			pstop[j].latitude = ptiming->approch[i].stopline_cfg[j].latitude;
			pstop[j].longitude = ptiming->approch[i].stopline_cfg[j].longitude;
			pstop[j].line_length = ptiming->approch[i].stopline_cfg[j].line_length;	// cm
			pstop[j].orientation = ptiming->approch[i].stopline_cfg[j].orientation;	// degrees clockwise from north
		}
	}
	if ( verbose == 2 )
		ix_msg_print(pix);		
	// write ix_msg to database
	ix_msg_update(pclt,pix,DB_IX_MSG_VAR,DB_IX_APPROACH1_VAR);
	// Free the pstop pointers 
	for( i=0;i<pix->num_approaches;i++ )
	{
		free(pix->approach_array[i].stop_line_array);
		pix->approach_array[i].stop_line_array = NULL;
	}
	return;
}

// function to echo intersection configurations
void echo_cfg(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3])
{
	int i,j,k;
	printf("Welcome to %s\n\n",ptiming->name);
	printf("Approach configurations:\n");
	printf("\tIntersection ID %d\n",ptiming->intersection_id);
	printf("\tMap node ID %d\n",ptiming->map_node_id);
	printf("\tIntersection center gps %d %d %d\n",
		ptiming->center.latitude,ptiming->center.longitude,ptiming->center.altitude);
	printf("\tAntenna gps %d %d %d\n",
		ptiming->antenna.latitude,ptiming->antenna.longitude,ptiming->antenna.altitude);
	printf("\tNumber of approaches %d\n",ptiming->total_no_approaches);
	printf("\tNumber of stoplines %d\n",ptiming->total_no_stoplines);
	printf("\tStopline configurations:\n");
	printf("\t\t(approach_type, control_phase, no_stoplines)\n");
	printf("\t\t(stopline_lat, stopline_long, stopline_length, stopline_orient)\n");
	for (i=0;i<ptiming->total_no_approaches;i++)
	{
		printf("\t\t%d %d %d\n",
			ptiming->approch[i].approach_type,ptiming->approch[i].control_phase,
			ptiming->approch[i].no_stoplines);
		for (j=0;j<ptiming->approch[i].no_stoplines;j++)
		{
			printf("\t\t%d %d %d %d\n",
				ptiming->approch[i].stopline_cfg[j].latitude,
				ptiming->approch[i].stopline_cfg[j].longitude,
				ptiming->approch[i].stopline_cfg[j].line_length,
				ptiming->approch[i].stopline_cfg[j].orientation);
		}
	}
	printf("\n");
	printf("Phase configurations:\n");
	j = 0;
	for (i=0;i<MAX_PHASES;i++)
	{
		if (ptiming->phase_timing.permitted_phase[i] == 1) 
			j+= 1;
	}
	printf("\tNumber of activated phases %d\n",j);
	// plan 0 is running free
	j = 0;
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 1)
			j+= 1;
	}
	printf("\tNumber of activated plans %d\n",j);
	for (i=1;i<MAX_PLANS;i++)
	{
		if (ptiming->plan_timing[i].plan_activated == 0)
			continue;
		printf("\tPlan_no, Synch_phase, Cycle_length\n");
		k = ptiming->plan_timing[i].synch_phase-1;
		printf("\t\t%d\t%d\t%d\n",
			ptiming->plan_timing[i].control_type,
			ptiming->phase_timing.phase_swap[k],
			(int)ptiming->plan_timing[i].cycle_length);
		printf("\tPlan_no, phase, green_start, yellow_start, red_start\n");
		for (j=0;j<MAX_PHASES;j++)
		{
			if (ptiming->phase_timing.permitted_phase[j] == 0)
				continue;
			printf("\t\t%d %d %5.1f %5.1f %5.1f\n",
				i,ptiming->phase_timing.phase_swap[j],
				onsets[i][j][0],onsets[i][j][1],onsets[i][j][2]);
		}		
	}
	fflush(stdout);
	return;
}
