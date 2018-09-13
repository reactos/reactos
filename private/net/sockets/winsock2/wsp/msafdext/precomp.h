/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Common header file for MSAFDEXT NTSD Debugger Extension DLL.

Author:

    Keith Moore (keithmo) 20-May-1996

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_


#include <winsockp.h>
#include <stdio.h>
#include <ntsdexts.h>
#include <ntverp.h>

#include "cons.h"
#include "type.h"
#include "data.h"
#include "proc.h"


//
// Stuff stolen from NTSDEXTP.H.
//

#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PNTSD_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disassm                 (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )


#endif  // _PRECOMP_H_

