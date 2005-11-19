/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/exfuncs.h
 * PURPOSE:         Prototypes for exported Executive Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _EXFUNCS_H
#define _EXFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex);

VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(PFAST_MUTEX FastMutex);

#endif
