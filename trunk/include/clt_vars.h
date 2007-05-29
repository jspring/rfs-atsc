/*	FILE
 *	clt_vars.h  This version is intended for Intersection Decidion
 *              Support (IDS) project
 *
 *	As a convention, the variable name/type space is partitioned as
 *	follows:
 *
 *  0    to 99     Used by the system.
 *  100  to 199    Reserved.
 *  200  to 299    Permanent variables.
 *  1000 to 1099   Temporary variables.
 */

/*  Permanent variables
 */

#define DB_DII_OUT_TYPE             200  /* dii_out_typ */
#define DB_LONG_RADAR1_TYPE         201  /* long_radar_typ */
#define DB_LONG_RADAR2_TYPE         202  /* long_radar_typ */
#define DB_LONG_LIDAR1A_TYPE        203  /* long_lidarA_typ */
#define DB_LONG_LIDAR2A_TYPE        204  /* long_lidarA_typ */
#define DB_LONG_LIDAR1B_TYPE        205  /* long_lidarB_typ */
#define DB_LONG_LIDAR2B_TYPE        206  /* long_lidarB_typ */
#define DB_GPS1_GGA_TYPE            207  /* gps_gga_typ */
#define DB_GPS1_VTG_TYPE            208  /* gps_vtg_typ */

#define DB_DII_OUT_VAR              200
#define DB_LONG_RADAR1_VAR          201
#define DB_LONG_RADAR2_VAR          202
#define DB_LONG_LIDAR1A_VAR         203
#define DB_LONG_LIDAR2A_VAR         204
#define DB_LONG_LIDAR1B_VAR         205
#define DB_LONG_LIDAR2B_VAR         206
#define DB_GPS1_GGA_VAR             207
#define DB_GPS1_VTG_VAR             208

/* for communication between driver and phase countdown */
#define DB_ATSC_TYPE                300
#define DB_ATSC_VAR                 300
/* for communication between phase countdown and message send */
#define DB_ATSC_BCAST_TYPE          301
#define DB_ATSC_BCAST_VAR           301
/* for timing comparison between ntcip and sniffer */
#define DB_ATSC2_TYPE               302
#define DB_ATSC2_VAR                302

// used for transmission of an intersection message type
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

//used for received intersection message type
#define DB_RX_IX_MSG_TYPE              420
#define DB_RX_IX_APPROACH1_TYPE        421
#define DB_RX_IX_APPROACH2_TYPE        422
#define DB_RX_IX_APPROACH3_TYPE        423
#define DB_RX_IX_APPROACH4_TYPE        424
#define DB_RX_IX_APPROACH5_TYPE        425
#define DB_RX_IX_APPROACH6_TYPE        426
#define DB_RX_IX_APPROACH7_TYPE        427
#define DB_RX_IX_APPROACH8_TYPE        428
#define DB_RX_IX_APPROACH9_TYPE        429
#define DB_RX_IX_APPROACH10_TYPE       430

#define DB_RX_IX_MSG_VAR               420
#define DB_RX_IX_APPROACH1_VAR         421
#define DB_RX_IX_APPROACH2_VAR         422
#define DB_RX_IX_APPROACH3_VAR         423
#define DB_RX_IX_APPROACH4_VAR         424
#define DB_RX_IX_APPROACH5_VAR         425
#define DB_RX_IX_APPROACH6_VAR         426
#define DB_RX_IX_APPROACH7_VAR         427
#define DB_RX_IX_APPROACH8_VAR         428
#define DB_RX_IX_APPROACH9_VAR         429
#define DB_RX_IX_APPROACH10_VAR        430

