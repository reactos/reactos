/*
 * internal executive prototypes
 */

#ifndef _INCLUDE_INTERNAL_EXECUTIVE_H
#define _INCLUDE_INTERNAL_EXECUTIVE_H

#include <ddk/ntddk.h>
#include <ntos/time.h>

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION SystemTimeZoneInfo;

/* INITIALIZATION FUNCTIONS *************************************************/

VOID ExInit (VOID);
VOID ExInitTimeZoneInfo (VOID);
VOID ExInitializeWorkerThreads(VOID);

#endif /* _INCLUDE_INTERNAL_EXECUTIVE_H */


