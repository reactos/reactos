/* rc_id.hh -- Private header file for stdcom communications driver module
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 1:02p $
 */

/* --- Resource IDs --- */

// Dialog control IDs
#define ID_CB_XSENDING	101
#define ID_CB_XRECVING	102
#define ID_CMB_XON		103
#define ID_CMB_XOFF 	104
#define ID_RB_RX_NONE	105
#define ID_RB_RX_RTS	106
#define ID_RB_RX_DTR	107
#define ID_RB_TX_NONE	108
#define ID_RB_TX_CTS	109
#define ID_RB_TX_DSR	110
#define ID_CMB_BREAK_DUR 111
#define ID_TEXT_PARITY	 112
#define ID_TEXT_FRAMING  113
#define ID_TEXT_OVERFLOW 114
#define ID_TEXT_OVERRUN  115


// String IDs
#define SID_DEVICE_NAME 	1
#define SID_ERR_VERSION 	2
#define SID_ERR_NOMEM		3
#define SID_ERR_NOPORT		4
#define SID_ERR_NOOPEN		5
#define SID_ERR_NOTIMER 	6
#define SID_ERR_WINDRIVER	7
#define SID_ERR_BADSETTING	8
#define SID_ERR_BADBAUD 	9
#define SID_ERR_BADCHARVAL	10
