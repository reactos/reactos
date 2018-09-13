/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1997			**/
/*****************************************************************/ 

/* NPSTUB.H -- Definitions for example hooking network provider DLL.
 *
 * History:
 *	06/02/94	lens	Created
 */

#include <windows.h>
#include <netspi.h>

// Macros to define process local storage:

#define PROCESS_LOCAL_BEGIN data_seg(".PrcLcl","INSTANCE")
#define PROCESS_LOCAL_END data_seg()

