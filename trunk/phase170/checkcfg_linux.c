#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int_cfg.h"

void get_signal_change_onset(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3]);
void echo_cfg(E170_timing_typ *ptiming, float onsets[MAX_PLANS][MAX_PHASES][3]);

int main(void)
{
	float signal_state_onset[MAX_PLANS][MAX_PHASES][3];
	
	E170_timing_typ *prfs_timing;
	prfs_timing = &signal_timing_cfg[0];
	
	memset(signal_state_onset,0,sizeof(signal_state_onset));
	get_signal_change_onset(prfs_timing, signal_state_onset);	
	echo_cfg(prfs_timing, signal_state_onset);
	
	exit(1);
}

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
			if ( f > ptiming->plan_timing[i].cycle_length )
				f -= ptiming->plan_timing[i].cycle_length;
			onsets[i][j][0] = f; // green start
			onsets[i][j][1] = ptiming->plan_timing[i].force_off[j]; // yellow start
			onsets[i][j][2] = onsets[i][j][1] + ptiming->phase_timing.yellow_intv[j];
			if ( onsets[i][j][2] > ptiming->plan_timing[i].cycle_length )
				onsets[i][j][2] -= ptiming->plan_timing[i].cycle_length; // red start
		}		
	}
	return;
}	

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
