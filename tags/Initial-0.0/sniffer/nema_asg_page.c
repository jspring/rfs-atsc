// Bit assignment for Page Mill Road
#include "nema_asg.h"

bit_to_nema_phase_t nema_asg[] = {
	{'G', 2},
	{'G', 6},
	{'G', 4},
	{'G', 8},
	{'G', 5},
	{'G', 1},
	{'G', 7},
	{'G', 3},
};

#define NUM_BITS_ASSIGNED sizeof(nema_asg)/sizeof(bit_to_nema_phase_t)

int num_bits_to_nema = NUM_BITS_ASSIGNED;
