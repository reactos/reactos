/*
 * internal executive prototypes
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

#include <ddk/ntddk.h>
#include <ntos/time.h>

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION SystemTimeZoneInfo;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID 
ExInit (VOID);
VOID 
ExInitTimeZoneInfo (VOID);
VOID 
ExInitializeWorkerThreads(VOID);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */


