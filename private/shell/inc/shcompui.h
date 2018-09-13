#ifndef __SHCOMPUI_H
#define __SHCOMPUI_H
///////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  FILE: SHCOMPUI.H
//
//  DESCRIPTION:
//
//    Header for users of SHCOMPUI.DLL.
//
//    REVISIONS:
//
//    Date       Description                                         Programmer
//    ---------- --------------------------------------------------- ----------
//    09/15/95   Initial creation.                                   brianau
//    09/21/95   Changes per first code review.                      brianau
//    09/28/95   Added SCCA_CONTEXT structure.                       brianau
//
///////////////////////////////////////////////////////////////////////////////
#ifdef WINNT

#include <windows.h>

//
// Define a context structure for keeping track of what's going on during the
// compression/uncompression operations.
//
typedef struct {
   BOOL bIgnoreAllErrors;       // User wants to ignore all errors.
   DWORD uCompletionReason;     // Reason operation completed.
   DWORD cErrors;               // Number of errors on last call.
   DWORD cCummErrors;           // Cummulative error count.
} SCCA_CONTEXT, *LPSCCA_CONTEXT;


//
// Values for uCompletionReason member of SCCA_CONTEXT.
//
#define SCCA_REASON_NORMAL         0  // No problems.
#define SCCA_REASON_USERCANCEL     1  // User cancel in confirm dlg.
#define SCCA_REASON_USERABORT      2  // User abort in error dlg.
#define SCCA_REASON_IOERROR        3  // Device IO error.
#define SCCA_REASON_DISKFULL       4  // Disk full on uncompression.

//
// Initialize a new context structure.
// Use address of structure as argument.
//
#define SCCA_CONTEXT_INIT(c)  { memset((LPVOID)c, 0, sizeof(SCCA_CONTEXT)); }

//
// To call this function, create a SCCA_CONTEXT variable, initialize
// it with SCCA_CONTEXT_INIT( ) and pass it's address in the "context"
// argument.  The context structure maintains information that must
// persist between successive calls while processing an Explorer
// selection set.  The structure is also used to collect performance
// data such as error counts and completion status that may be queried
// once the function returns.
//
BOOL ShellChangeCompressionAttribute(HWND hActiveWnd, LPTSTR szNameSpec,
                      LPSCCA_CONTEXT context, BOOL bCompress, BOOL bShowUI);

#define SZ_SHCOMPUI_DLLNAME    __TEXT("SHCOMPUI.DLL")

//
// Note:  This must be an ANSI string for GetProcAddress( ).
//
#define SZ_COMPRESS_PROCNAME   "ShellChangeCompressionAttribute"


#endif  // ifdef WINNT

#endif  // ifdef __SHCOMPUI_H

