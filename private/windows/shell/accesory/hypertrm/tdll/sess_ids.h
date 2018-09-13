/*	File: D:\WACKER\tdll\sess_ids.h (Created: 30-Dec-1993)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:37p $
 */

/*
 *	This file contains the IDs for items that are in the session file.
 *
 *	The following guidelines are suggested for using this list:
 *
 *	1.  New guidelines may be added as necessary.  It is the responsibility
 *		of the person adding the new guideline to make sure that old entries
 *		conform to the new guideline.
 *
 *	2.	All IDs should be prefixed with SFID_ to indicate what they are.
 *
 *	3.	All IDs should be entered in HEX and in increasing numerical order.
 *
 *	4.	If a block of IDs are needed, the first and last ID in the block
 *		should be entered in the file and a comment added to indicate that
 *		the values in between should not be used for other reasons.
 *
 *	Thank you for your support.
 */

#define SFID_ICON_DEFAULT			0x00000001
#define SFID_ICON_EXTERN			0x00000002

#define SFID_INTERNAL_TAG			0x00000005

#define SFID_PRINTSET_NAME			0x00000010
#define SFID_EMU_SETTINGS			0x00000011
#define SFID_EMU_TEXTCOLOR_SETTING	0x00000013
#define SFID_EMU_BKGRNDCOLOR_SETTING 0x00000014
#define SFID_EMU_SCRNROWS_SETTING	0x00000015
#define SFID_EMU_SCRNCOLS_SETTING	0x00000016

#define SFID_PRINTSET_DEVMODE		0x00000020
#define SFID_PRINTSET_DEVNAMES		0x00000021
#define SFID_PRINTSET_FONT  		0x00000022
#define SFID_PRINTSET_MARGINS		0x00000023
#define SFID_PRINTSET_FONT_HEIGHT	0x00000024
#define SFID_KEY_MACRO_LIST         0x00000025

#define SFID_CNCT					0x00000040
#define SFID_CNCT_CC				0x00000041
#define SFID_CNCT_AREA				0x00000042
#define SFID_CNCT_DEST				0x00000043
#define SFID_CNCT_LINE				0x00000044
#define SFID_CNCT_TAPICONFIG		0x00000045
#define SFID_CNCT_USECCAC			0x00000046
#define SFID_CNCT_REDIAL            0x00000047
#define SFID_CNCT_COMDEVICE         0x00000048
#define SFID_CNCT_END				0x00000050

#define SFID_CNCT_IPDEST			0x00000051
#define SFID_CNCT_IPPORT			0x00000052

#define SFID_XFER_PARAMS			0x00000100

#define	SFID_PROTO_PARAMS			0x00000101
/* This block is used for SFID_PROTO_PARAMS */
#define	SFID_PROTO_PARAMS_END		0x00000111

#define	SFID_XFR_RECV_DIR			0x00000120
#define	SFID_XFR_SEND_DIR			0x00000121
#define	SFID_XFR_USE_BPS			0x00000122

#define	SFID_CPF_FILENAME			0x00000128
#define	SFID_CPF_MODE				0x00000129
#define	SFID_CPF_FILE				0x0000012A

#define	SFID_PRE_MODE				0x0000012C
#define	SFID_PRE_METHOD				0x0000012D

/* Backscroll region size and saved data (text) */
#define SFID_BKSC					0x00000130
#define	SFID_BKSC_SIZE				0x00000131
#define	SFID_BKSC_TEXT				0x00000132
#define SFID_BKSC_ULINES			0x00000133
#define SFID_BKSC_END				0x00000134

#define	SFID_TLBR_VISIBLE			0x00000135
#define	SFID_STBR_VISIBLE			0x00000136

#define SFID_CLOOP					0x0000200
/* This block is used for SFID_CLOOP values */
#define SFID_CLOOP_END				0x000021F

#define SFID_STDCOM 				0x00001011
/* This block is used for SFID_STDCOM values */
#define SFID_STDCOM_END 			0x0000101F

#define SFID_SESS_SOUND				0x00001020
#define SFID_SESS_NAME				0x00001021
#define SFID_SESS_LEFT				0x00001022
#define SFID_SESS_TOP				0x00001023
#define SFID_SESS_RIGHT				0x00001024
#define SFID_SESS_BOTTOM			0x00001025
#define SFID_SESS_SHOWCMD			0x00001026
#define SFID_SESS_EXIT				0x00001027

#define SFID_TERM_LOGFONT			0x00001030
/* Another block, please don't use these values */
#define SFID_TERM_END				0x0000103F

#define	SFID_TRANS_FIRST			0x00001040
/* This block is used for CHARACTER_TRANSLATION features */
#define	SFID_TRANS_END				0x0000107F
