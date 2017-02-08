/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/misc.c
 * PURPOSE:         Various functions
 *
 * PROGRAMMERS:
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ULONG NtGlobalFlag;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern ULONG NtOSCSDVersion;

/* FUNCTIONS *****************************************************************/

/*
* @implemented
*/
ULONG
NTAPI
RtlGetNtGlobalFlags(VOID)
{
    return NtGlobalFlag;
}

/*
 * @implemented
 */
NTSTATUS 
NTAPI
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    PAGED_CODE();

    /* Return the basics */
    lpVersionInformation->dwMajorVersion = NtMajorVersion;
    lpVersionInformation->dwMinorVersion = NtMinorVersion;
    lpVersionInformation->dwBuildNumber = NtBuildNumber & 0x3FFF;
    lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;

    /* Check if this is the extended version */
    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        PRTL_OSVERSIONINFOEXW InfoEx = (PRTL_OSVERSIONINFOEXW)lpVersionInformation;
        InfoEx->wServicePackMajor = (USHORT)(CmNtCSDVersion >> 8) & 0xFF;
        InfoEx->wServicePackMinor = (USHORT)(CmNtCSDVersion & 0xFF);
        InfoEx->wSuiteMask = (USHORT)(SharedUserData->SuiteMask & 0xFFFF);
        InfoEx->wProductType = SharedUserData->NtProductType;
        InfoEx->wReserved = 0;
    }

    /* Always succeed */
    return STATUS_SUCCESS;
}

#if !defined(_M_IX86)
//
// Stub for architectures which don't have this implemented
//
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(IN PVOID Source,
                             IN SIZE_T Length)
{
    //
    // Do nothing
    //
    UNREFERENCED_PARAMETER(Source);
    UNREFERENCED_PARAMETER(Length);
}
#endif

/* EOF */
