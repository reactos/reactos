/*****************************************************************/
/**		      	Copyright (C) Microsoft Corp., 1996				**/
/*****************************************************************/ 

/* CONVTIME.H -- Header file for time conversion subroutines.
 *
 * History:
 *	gregj	04/22/96	Created for subroutines snarfed from vredir.
 */


#ifndef _INC_CONVTIME
#define _INC_CONVTIME

#ifndef RC_INVOKED
#ifdef __cplusplus
extern "C" {
#endif


#define _70_to_80_bias	0x012CEA600L
#define SECS_IN_DAY (60L*60L*24L)
#define SEC2S_IN_DAY (30L*60L*24L)
#define FOURYEARS	(3*365+366)

extern WORD MonTotal[];


typedef struct tagdos_time {
	WORD dt_date;
	WORD dt_time;
} dos_time;

void
NetToDosDate(
DWORD time,
dos_time *pinfo);

DWORD
DosToNetDate(dos_time dt);

void
NetToSystemDate(
DWORD time,
LPSYSTEMTIME pinfo);

DWORD
SystemToNetDate(LPSYSTEMTIME pinfo);

DWORD
GetCurrentNetDate(void);

DWORD
GetLocalNetDate(void);


#ifdef __cplusplus
};	/* extern "C" */
#endif

#endif	/* RC_INVOKED */

#endif	/* _INC_CONVTIME */
