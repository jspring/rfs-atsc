/**\file
 *
 *	nema_asg.h
 */

// Structure for assigning bit position in digital I/O to NEMA phase
typedef struct {
	char phase_color; 		// R, G, Y, N
	unsigned char phase_number;	// NEMA phase number
} bit_to_nema_phase_t;
