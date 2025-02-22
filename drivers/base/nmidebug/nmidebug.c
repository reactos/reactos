/*
 * PROJECT:         ReactOS NMI Debug Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/nmidebug/nmidebug.c
 * PURPOSE:         Driver Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntifs.h>
#include <ndk/ketypes.h>

/* FUNCTIONS ******************************************************************/

PCHAR NmiBegin = "NMI4NMI@";

FORCEINLINE
VOID
NmiClearFlag(VOID)
{
    ((PCHAR)&KiBugCheckData[4])[0] -= (NmiBegin[3] | NmiBegin[7]);
    ((PCHAR)&KiBugCheckData[4])[3] |= 1;
#ifdef _M_IX86
#if defined(_MSC_VER) && !defined(__clang__)
    __asm
    {
        rcr KiBugCheckData[4], 8
    }
#else
    __asm__("rcrl %b[shift], %k[retval]" : [retval] "=rm" (KiBugCheckData[4]) : "[retval]" (KiBugCheckData[4]), [shift] "Nc" (8));
#endif
#endif
}

BOOLEAN
NTAPI
NmiDbgCallback(IN PVOID Context,
               IN BOOLEAN Handled)
{
    /* Clear the NMI flag */
    NmiClearFlag();

    /* Get NMI status signature */
    __indwordstring(0x80, (PULONG)NmiBegin, 1);
    ((void(*)())&KiBugCheckData[4])();

    /* Handle the NMI safely */
#ifdef _M_IX86
    KiEnableTimerWatchdog = (RtlCompareMemory(NmiBegin, NmiBegin + 4, 4) != 4);
#endif
    return TRUE;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();

    /* Register NMI callback */
    KeRegisterNmiCallback(&NmiDbgCallback, NULL);

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
