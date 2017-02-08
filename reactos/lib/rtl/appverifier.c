/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            lib/rtl/appverifier.c
 * PURPOSE:         RTL Application Verifier Routines
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlApplicationVerifierStop(
    _In_ ULONG_PTR Code,
    _In_ PCSTR Message,
    _In_ PVOID Value1,
    _In_ PCSTR Description1,
    _In_ PVOID Value2,
    _In_ PCSTR Description2,
    _In_ PVOID Value3,
    _In_ PCSTR Description3,
    _In_ PVOID Value4,
    _In_ PCSTR Description4)
{
    PTEB Teb = NtCurrentTeb();

    DbgPrint("**************************************************\n");
    DbgPrint("VERIFIER STOP %08Ix: pid %04Ix:  %s\n",
             Code, (ULONG_PTR)Teb->ClientId.UniqueProcess, Message);
    DbgPrint("    %p : %s\n", Value1, Description1);
    DbgPrint("    %p : %s\n", Value2, Description2);
    DbgPrint("    %p : %s\n", Value3, Description3);
    DbgPrint("    %p : %s\n", Value4, Description4);
    DbgPrint("**************************************************\n");
    DbgBreakPoint();
}
