/*
 * internal executive prototypes
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

#define NTOS_MODE_KERNEL
#include <ntos.h>

#ifndef __WIN32K__
typedef PVOID PUSER_HANDLE_TABLE;
#endif

typedef struct _WINSTATION_OBJECT
{
  UNICODE_STRING Name;
  KSPIN_LOCK Lock;

  /* desktops */
  LIST_ENTRY DesktopListHead;
  struct _DESKTOP_OBJECT* ActiveDesktop;

  /* atom table */
  PRTL_ATOM_TABLE AtomTable;
  
  /* user object handle table */
  PUSER_HANDLE_TABLE HandleTable;
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

typedef struct _DESKTOP_OBJECT
{
  UNICODE_STRING Name;
  KSPIN_LOCK Lock;
  
  /* link to the desktop list */
  LIST_ENTRY ListEntry;
  /* desktop owner */
  struct _WINSTATION_OBJECT *WindowStation;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;


typedef VOID (*PLOOKASIDE_MINMAX_ROUTINE)(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth);

/* GLOBAL VARIABLES *********************************************************/

extern TIME_ZONE_INFORMATION ExpTimeZoneInfo;
extern LARGE_INTEGER ExpTimeZoneBias;
extern ULONG ExpTimeZoneId;

extern POBJECT_TYPE ExEventPairObjectType;


/* INITIALIZATION FUNCTIONS *************************************************/

VOID
ExpWin32kInit(VOID);

VOID
ExInit2(VOID);
VOID
ExInit3(VOID);
VOID
ExpInitTimeZoneInfo(VOID);
VOID
ExInitializeWorkerThreads(VOID);
VOID
ExpInitLookasideLists(VOID);
VOID
ExpInitializeCallbacks(VOID);

/* OTHER FUNCTIONS **********************************************************/

#ifdef _ENABLE_THRDEVTPAIR
VOID
ExpSwapThreadEventPair(
	IN struct _ETHREAD* Thread,
	IN struct _KEVENT_PAIR* EventPair
	);
#endif /* _ENABLE_THRDEVTPAIR */

LONGLONG
FASTCALL
ExfpInterlockedExchange64(LONGLONG volatile * Destination,
                          PLONGLONG Exchange);


#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */
