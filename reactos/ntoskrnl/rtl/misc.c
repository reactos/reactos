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
	return(NtGlobalFlag);
}


/*
* @implemented
*/
NTSTATUS NTAPI
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
   ULONG i, MaxLength;
   if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
       lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      lpVersionInformation->dwMajorVersion = NtMajorVersion;
      lpVersionInformation->dwMinorVersion = NtMinorVersion;
      lpVersionInformation->dwBuildNumber = NtBuildNumber;
      lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
      RtlZeroMemory(lpVersionInformation->szCSDVersion, sizeof(lpVersionInformation->szCSDVersion));
      if(((CmNtCSDVersion >> 8) & 0xFF) != 0)
      {
        MaxLength = (sizeof(lpVersionInformation->szCSDVersion) / sizeof(lpVersionInformation->szCSDVersion[0])) - 1;
        i = _snwprintf(lpVersionInformation->szCSDVersion,
                       MaxLength,
                       L"Service Pack %d",
                       ((CmNtCSDVersion >> 8) & 0xFF));
        if (i < 0)
        {
           /* null-terminate if it was overflowed */
           lpVersionInformation->szCSDVersion[MaxLength] = L'\0';
        }
      }
      if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)lpVersionInformation;
         InfoEx->wServicePackMajor = (USHORT)(CmNtCSDVersion >> 8) & 0xFF;
         InfoEx->wServicePackMinor = (USHORT)(CmNtCSDVersion & 0xFF);
         InfoEx->wSuiteMask = (USHORT)SharedUserData->SuiteMask;
         InfoEx->wProductType = SharedUserData->NtProductType;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

