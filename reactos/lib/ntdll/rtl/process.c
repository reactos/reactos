/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <napi/i386/segment.h>
#include <ntdll/ldr.h>
#include <ntdll/base.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/


PPEB
STDCALL
RtlpCurrentPeb(VOID)
{
    return NtCurrentPeb();
}


/*
 * @implemented
 */
VOID STDCALL
RtlAcquirePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebLockRoutine (Peb->FastPebLock);
}


/*
 * @implemented
 */
VOID STDCALL
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebUnlockRoutine (Peb->FastPebLock);
}


/*
* @implemented
*/
NTSTATUS STDCALL
RtlGetVersion(RTL_OSVERSIONINFOW *Info)
{
   if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
       Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      PPEB Peb = NtCurrentPeb();

      Info->dwMajorVersion = Peb->OSMajorVersion;
      Info->dwMinorVersion = Peb->OSMinorVersion;
      Info->dwBuildNumber = Peb->OSBuildNumber;
      Info->dwPlatformId = Peb->OSPlatformId;
      if(((Peb->OSCSDVersion >> 8) & 0xFF) != 0)
      {
        int i = _snwprintf(Info->szCSDVersion,
                           (sizeof(Info->szCSDVersion) / sizeof(Info->szCSDVersion[0])) - 1,
                           L"Service Pack %d",
                           ((Peb->OSCSDVersion >> 8) & 0xFF));
        Info->szCSDVersion[i] = L'\0';
      }
      else
      {
        RtlZeroMemory(Info->szCSDVersion, sizeof(Info->szCSDVersion));
      }
      if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)Info;
         InfoEx->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
         InfoEx->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
         InfoEx->wSuiteMask = SharedUserData->SuiteMask;
         InfoEx->wProductType = SharedUserData->NtProductType;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

/* EOF */
