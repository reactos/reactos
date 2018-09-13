/***
*share.h - defines file sharing modes for sopen
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the file sharing modes for sopen().
*
****/

#ifndef _INC_SHARE

#define _SH_COMPAT  0x00    /* compatibility mode */
#define _SH_DENYRW  0x10    /* deny read/write mode */
#define _SH_DENYWR  0x20    /* deny write mode */
#define _SH_DENYRD  0x30    /* deny read mode */
#define _SH_DENYNO  0x40    /* deny none mode */

#ifndef __STDC__
/* Non-ANSI names for compatibility */
#define SH_COMPAT _SH_COMPAT
#define SH_DENYRW _SH_DENYRW
#define SH_DENYWR _SH_DENYWR
#define SH_DENYRD _SH_DENYRD
#define SH_DENYNO _SH_DENYNO
#endif 

#define _INC_SHARE
#endif 
