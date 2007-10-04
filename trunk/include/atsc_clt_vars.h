/*	FILE
 *	atsc_clt_vars.h 
 *
 *	This version is intended for the ATSC software
 *
 *	As a convention, the variable name/type space is partitioned as
 *	follows:
 *
 *  0    to 99     Used by the system.
 *  100  to 199    Reserved.
 *  200  to 299    Permanent variables.
 *  1000 to 1099   Temporary variables.
 */

/* same as in /home/cicas/src */
#define DB_TRAFFIC_SIGNAL_TYPE      264
#define DB_TRAFFIC_SIGNAL_VAR       264

/* for communication between driver and phase countdown */
#define DB_ATSC_TYPE                300
#define DB_ATSC_VAR                 300
/* for communication between phase countdown and message send */
#define DB_ATSC_BCAST_TYPE          301
#define DB_ATSC_BCAST_VAR           301
/* for timing comparison between ntcip and sniffer */
#define DB_ATSC2_TYPE               302
#define DB_ATSC2_VAR                302

/* for transmission of an intersection message type */
#define DB_IX_MSG_TYPE              400
#define DB_IX_APPROACH1_TYPE        401
#define DB_IX_APPROACH2_TYPE        402
#define DB_IX_APPROACH3_TYPE        403
#define DB_IX_APPROACH4_TYPE        404
#define DB_IX_APPROACH5_TYPE        405
#define DB_IX_APPROACH6_TYPE        406
#define DB_IX_APPROACH7_TYPE        407
#define DB_IX_APPROACH8_TYPE        408
#define DB_IX_APPROACH9_TYPE        409
#define DB_IX_APPROACH10_TYPE       410

#define DB_IX_MSG_VAR               400
#define DB_IX_APPROACH1_VAR         401
#define DB_IX_APPROACH2_VAR         402
#define DB_IX_APPROACH3_VAR         403
#define DB_IX_APPROACH4_VAR         404
#define DB_IX_APPROACH5_VAR         405
#define DB_IX_APPROACH6_VAR         406
#define DB_IX_APPROACH7_VAR         407
#define DB_IX_APPROACH8_VAR         408
#define DB_IX_APPROACH9_VAR         409
#define DB_IX_APPROACH10_VAR        410

/* for received intersection message type */
#define DB_RX_IX_MSG_TYPE           420
#define DB_RX_IX_APPROACH1_TYPE     421
#define DB_RX_IX_APPROACH2_TYPE     422
#define DB_RX_IX_APPROACH3_TYPE     423
#define DB_RX_IX_APPROACH4_TYPE     424
#define DB_RX_IX_APPROACH5_TYPE     425
#define DB_RX_IX_APPROACH6_TYPE     426
#define DB_RX_IX_APPROACH7_TYPE     427
#define DB_RX_IX_APPROACH8_TYPE     428
#define DB_RX_IX_APPROACH9_TYPE     429
#define DB_RX_IX_APPROACH10_TYPE    430

#define DB_RX_IX_MSG_VAR            420
#define DB_RX_IX_APPROACH1_VAR      421
#define DB_RX_IX_APPROACH2_VAR      422
#define DB_RX_IX_APPROACH3_VAR      423
#define DB_RX_IX_APPROACH4_VAR      424
#define DB_RX_IX_APPROACH5_VAR      425
#define DB_RX_IX_APPROACH6_VAR      426
#define DB_RX_IX_APPROACH7_VAR      427
#define DB_RX_IX_APPROACH8_VAR      428
#define DB_RX_IX_APPROACH9_VAR      429
#define DB_RX_IX_APPROACH10_VAR     430
