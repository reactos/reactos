/*	File: C:\WACKER\TDLL\VU_METER.HH (Created: 10-Jan-1994)
 *	Created from:
 *	File: C:\HA5G\ha5g\s_text.hh (Created: 27-SEP-1991)
 *
 *	Copyright 1990 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:40p $
 */

// #define	STXT_DEF_DEPTH		3
#define	STXT_DEF_DEPTH		1

struct s_vumeter
	{
	ULONG	ulCheck;			/* Validity check field */
	ULONG	cBackGround;		/* Fill color for background */
	ULONG	cFillColor; 		/* Color to use for progress display */
	ULONG	cRefillColor;		/* Color used in retries progress display */
	ULONG	cMarkColor; 		/* Color used when retries are going */
	ULONG	cUpperEdge; 		/* Upper and left edge 3D border color */
	ULONG	cLowerEdge; 		/* Lower and right edge 3D border color */
	ULONG	ulMaxRange; 		/* Maximum range value */
	ULONG	ulHiValue;			/* Highest value so far */
	ULONG	ulCurValue; 		/* Current value */
	USHORT	usDepth;
	};

#define VUMETER_VALID	0x744D7556

typedef struct s_vumeter VUMETER;
typedef VUMETER FAR *LPVM;

#define VUMETER_OK(x)	((x!=NULL)&&(x->ulCheck==VUMETER_VALID))

