/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trace.h

Abstract:

    This module contains definitions of the trace functions

Author:
    Michael Tsang (MikeTs) 24-Sep-1998

Environment:

    Kernel mode


Revision History:


--*/

#ifndef _TRACE_H
#define _TRACE_H

//
// Macros
//
#ifdef TRACING
  #ifndef PROCNAME
    #define PROCNAME(s) static PSZ ProcName = s
  #endif
  #define ENTER(n,p)    {                                               \
                            if (IsTraceOn(n, ProcName))                 \
                            {                                           \
                                KdPrint(p);                             \
                            }                                           \
                            ++IoepIndentLevel;                          \
                        }
  #define EXIT(n,p)     {                                               \
                            --IoepIndentLevel;                          \
                            if (IsTraceOn(n, ProcName))                 \
                            {                                           \
                                KdPrint(p);                             \
                            }                                           \
                        }
#else
  #define ENTER(n,p)
  #define EXIT(n,p)
#endif

//
// Exported data
//
extern int IoepIndentLevel;

//
// Exported function prototypes
//
#ifdef TRACING
BOOLEAN
IsTraceOn(
    IN UCHAR   n,
    IN PSZ     ProcName
    );
#endif

#endif  //ifndef _TRACE_H
