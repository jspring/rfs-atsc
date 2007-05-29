
// Structure for assigning bit position in digital I/O to NEMA phase
typedef struct {
	char phase_color; // R, G, Y, N
	unsigned char phase_number;	// NEMA phase number
} bit_to_nema_phase_t;

// Reference to variable that stores number of bits to be converted
// and written to atsc_t and array that stores conversion information
extern int num_bits_to_nema;
extern bit_to_nema_phase_t nema_asg[];
