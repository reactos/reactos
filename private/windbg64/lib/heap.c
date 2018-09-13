/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This file contains a heap validation routines.

Author:

    Wesley Witt (wesw) 2-Feb-94

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"


//
// used to globally enable or disable heap checking
//
DWORD fHeapCheck = TRUE;



VOID
ValidateTheHeap(
    LPSTR fName,
    DWORD dwLine
    )
/*++

Routine Description:

    Validate the process's heap.  If the heap is found to be
    invalid then a messagebox is displayed that indicates the
    caller's file & line number.  If the BO button is pressed
    on the message box then DebugBreak is called.

Arguments:

   fName   - caller's source filename
   dwLine  - caller's source line number

Return Value:

   None.

--*/
{
    CHAR buf[256];
    INT  id;

    //
    // we don't really want to allocate memory here, since the
    // whole point was to look for heap corruption.  Instead,
    // just preallocate an unreasonable number of slots...
    //

    PVOID ProcessHeaps[64];
    DWORD NumberOfHeaps = 64;
    DWORD Heap;


    if (fHeapCheck) {

        NumberOfHeaps = GetProcessHeaps(NumberOfHeaps, ProcessHeaps);

        for (Heap = 0; Heap < NumberOfHeaps; Heap++) {

            if (!HeapValidate( ProcessHeaps[Heap], 0, 0 )) {

                _snprintf( buf, sizeof(buf),
                           "Heap corruption detected in heap 0x%p at line %d in %s.\n",
                           ProcessHeaps[Heap], dwLine, fName);

                id = MessageBox( NULL, buf, "WinDbg Error",
                                 MB_YESNO | MB_ICONHAND |
                                 MB_TASKMODAL | MB_SETFOREGROUND );

                OutputDebugString( buf );
                OutputDebugString("\n\r");

                if (id != IDYES) {
                    DebugBreak();
                }

            }

        }

    }
}
