/**\file
 *
 *	Prototypes for functions in ix_utils.c
 *
 *	Copyright (c) 2006   Regents of the University of California
 *
 */	

// swap 2-byte and 4-byte fields in place
extern void ix_msg_swap(ix_msg_t *pix);
extern void ix_approach_swap(ix_approach_t *pappr);
extern void ix_stop_line_swap(ix_stop_line_t *pstop);
extern void ix_ts_to_tm(unsigned char *ts, struct timespec *ptmspec);
extern void ix_tm_to_ts(struct timespec *ptmspec, unsigned char *ts);
extern const char *ix_approach_string(unsigned char approach_type);
extern const char *ix_signal_state_string(unsigned char signal_state);
extern void ix_msg_extract(unsigned char *buf, ix_msg_t **ppix);
extern void ix_print_hdr_offsets(ix_msg_t *pix);
extern void ix_print_approach_offsets(ix_msg_t *pix, ix_approach_t *pappr, 
	int num);
extern void ix_print_stopline_offsets(ix_msg_t *pix, ix_stop_line_t *pstop,
	int num);
extern int ix_msg_format(ix_msg_t *pix, unsigned char *buf, int buf_size, 
			int do_print);
extern void ix_msg_free(ix_msg_t **ppix);
extern void ix_msg_print(ix_msg_t *pix);
