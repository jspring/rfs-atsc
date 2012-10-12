// Bit assignment for RFS
#include "nema_asg.h"

bit_to_nema_phase_t nema_asg[] = {
	{'G', 2},
	{'G', 4},
	{'Y', 2},
	{'Y', 4},
};

#define NUM_BITS_ASSIGNED sizeof(nema_asg)/sizeof(bit_to_nema_phase_t)

int num_bits_to_nema = NUM_BITS_ASSIGNED;
