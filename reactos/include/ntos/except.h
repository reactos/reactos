/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/ntos/except.h
 * PURPOSE:      Exception handling structures
 * PROGRAMMER:   Casper S. Hornstrup <chorns@users.sourceforge.net>
 */

#ifndef __INCLUDE_EXCEPT_H
#define __INCLUDE_EXCEPT_H

#include <excpt.h>

// FIXME: Figure out if this is needed
#define ExceptionDismiss (EXCEPTION_DISPOSITION)((ULONG)ExceptionCollidedUnwind + 1)

typedef DWORD (CDECL *PSCOPE_EXCEPTION_FILTER)(VOID);
typedef VOID (CDECL *PSCOPE_EXCEPTION_HANDLER)(VOID);

typedef struct _SCOPETABLE_ENTRY {
  DWORD PreviousTryLevel;
  PSCOPE_EXCEPTION_FILTER FilterRoutine;
  PSCOPE_EXCEPTION_HANDLER HandlerRoutine;
} SCOPETABLE_ENTRY, *PSCOPETABLE_ENTRY;

/*
 * Other structures preceeding this structure:
 *   ULONG_PTR StandardESPInFrame;
 *   LPEXCEPTION_POINTERS ExceptionPointers;
 */
typedef struct _RTL_EXCEPTION_REGISTRATION_I386 {
  EXCEPTION_REGISTRATION OS;
  PSCOPETABLE_ENTRY ScopeTable;
  DWORD TryLevel;
  /* Value of EBP before the EXCEPTION_REGISTRATION was created on the stack */
  ULONG_PTR Ebp;
} RTL_EXCEPTION_REGISTRATION_I386, *PRTL_EXCEPTION_REGISTRATION_I386;

#define TRYLEVEL_NONE -1

typedef RTL_EXCEPTION_REGISTRATION_I386 RTL_EXCEPTION_REGISTRATION;
typedef PRTL_EXCEPTION_REGISTRATION_I386 PRTL_EXCEPTION_REGISTRATION;

#endif /* __INCLUDE_EXCEPT_H */
