/*
 * Reactos Session Manager
 *
 *
 */

#include <ddk/ntddk.h>


#include "smss.h"



VOID STDCALL
SmApiThread(HANDLE Port)
{
    ULONG count;

    DisplayString (L"SmApiThread running...\n");

#if 0
    NtSuspendThread (NtCurrentThread(), &count);

    while (TRUE)
    {


    }

#endif
    DisplayString (L"SmApiThread terminating...\n");

    NtTerminateThread(NtCurrentThread(), 0);
}

/* EOF */
