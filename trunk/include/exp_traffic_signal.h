/**\file
 *
 *	Header file of traffic signal description types,
 *	originally part of CICAS experimental control
 *	but used by phases and phases170e
 */
#ifndef EXP_TRAFFIC_SIGNAL_H
#define EXP_TRAFFIC_SIGNAL_H 

// Signal phases, relative to SV:
#define SV_PHASE_GREEN      	0
#define SV_PHASE_AMBER      	1
#define SV_PHASE_RED        	2
#define SV_PHASE_FLASHING_RED	3
typedef unsigned char phase_typ;

// "Ring phases" refer to the NEMA concept of a phase as a set of
// concurrent lane movements, rather than to the color of the signal lamp
// in a particular direction. For now we will assume a maximum of 8 ring phases,
// with lanes numbered according to the NEMA standard from 1 to 8. At
// the RFS intersection, currently only phases 2 and 4 are active.

#define MAX_RING_PHASES		8

typedef struct
{
    phase_typ               phase;      // Current phase
    float                  time_used;  // time used in current phase, in sec.
    float                  time_left;  // time left in current phase, in sec.
} IS_PACKED signal_state_typ;

// database variable structure: ring_phase[0] corresponds to NEMA phase 1, etc.
// may add other fields with actuation info later
typedef struct
{
    signal_state_typ ring_phase[MAX_RING_PHASES];
} IS_PACKED traffic_signal_typ;

#endif
