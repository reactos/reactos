/*
 * internal executive prototypes
 */

#ifndef _INCLUDE_INTERNAL_EXECUTIVE_H
#define _INCLUDE_INTERNAL_EXECUTIVE_H


#include <ddk/ntddk.h>

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION SystemTimeZoneInfo;


/* INTERNAL EXECUTIVE FUNCTIONS *********************************************/

VOID ExUnmapPage(PVOID Addr);
PVOID ExAllocatePage(VOID);


/* INITIALIZATION FUNCTIONS *************************************************/

VOID ExInit (VOID);
VOID ExInitTimeZoneInfo (VOID);


#endif /* _INCLUDE_INTERNAL_EXECUTIVE_H */

/*EOF */

