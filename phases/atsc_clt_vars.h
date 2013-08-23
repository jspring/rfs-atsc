/*	FILE
 *	atsc_clt_vars.h 
 *
 *	This version is intended for the ATSC software
 *	All values in 3000s to avoid conflict with other projects
 *	that may want to coexist with this code.
 *
 *	Every atsc data server var is in the range 3000 to avoid conflicts,
 *	since this code may be used in concert with many other variables
 */

/* cicas references this*/
#define DB_TRAFFIC_SIGNAL_TYPE      3264
#define DB_TRAFFIC_SIGNAL_VAR       3264

/* for communication between driver and phase countdown */
#define DB_ATSC_TYPE                3300
#define DB_ATSC_VAR                 3300
/* for communication between phase countdown and message send */
#define DB_ATSC_BCAST_TYPE          3301
#define DB_ATSC_BCAST_VAR           3301
/* for timing comparison between ntcip and sniffer */
#define DB_ATSC2_TYPE               3302
#define DB_ATSC2_VAR                3302

/* for transmission of an intersection message type */
#define DB_IX_MSG_TYPE              3400
#define DB_IX_APPROACH1_TYPE        3401
#define DB_IX_APPROACH2_TYPE        3402
#define DB_IX_APPROACH3_TYPE        3403
#define DB_IX_APPROACH4_TYPE        3404
#define DB_IX_APPROACH5_TYPE        3405
#define DB_IX_APPROACH6_TYPE        3406
#define DB_IX_APPROACH7_TYPE        3407
#define DB_IX_APPROACH8_TYPE        3408
#define DB_IX_APPROACH9_TYPE        3409
#define DB_IX_APPROACH10_TYPE       3410

#define DB_IX_MSG_VAR               3400
#define DB_IX_APPROACH1_VAR         3401
#define DB_IX_APPROACH2_VAR         3402
#define DB_IX_APPROACH3_VAR         3403
#define DB_IX_APPROACH4_VAR         3404
#define DB_IX_APPROACH5_VAR         3405
#define DB_IX_APPROACH6_VAR         3406
#define DB_IX_APPROACH7_VAR         3407
#define DB_IX_APPROACH8_VAR         3408
#define DB_IX_APPROACH9_VAR         3409
#define DB_IX_APPROACH10_VAR        3410

/* for received intersection message type */
#define DB_RX_IX_MSG_TYPE           3420
#define DB_RX_IX_APPROACH1_TYPE     3421
#define DB_RX_IX_APPROACH2_TYPE     3422
#define DB_RX_IX_APPROACH3_TYPE     3423
#define DB_RX_IX_APPROACH4_TYPE     3424
#define DB_RX_IX_APPROACH5_TYPE     3425
#define DB_RX_IX_APPROACH6_TYPE     3426
#define DB_RX_IX_APPROACH7_TYPE     3427
#define DB_RX_IX_APPROACH8_TYPE     3428
#define DB_RX_IX_APPROACH9_TYPE     3429
#define DB_RX_IX_APPROACH10_TYPE    3430

#define DB_RX_IX_MSG_VAR            3420
#define DB_RX_IX_APPROACH1_VAR      3421
#define DB_RX_IX_APPROACH2_VAR      3422
#define DB_RX_IX_APPROACH3_VAR      3423
#define DB_RX_IX_APPROACH4_VAR      3424
#define DB_RX_IX_APPROACH5_VAR      3425
#define DB_RX_IX_APPROACH6_VAR      3426
#define DB_RX_IX_APPROACH7_VAR      3427
#define DB_RX_IX_APPROACH8_VAR      3428
#define DB_RX_IX_APPROACH9_VAR      3429
#define DB_RX_IX_APPROACH10_VAR     3430
