/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNtProductType
 *
 * DESCRIPTION
 *	Retrieves the OS product type.
 *
 * ARGUMENTS
 *	ProductType	Pointer to the product type variable.
 *
 * RETURN VALUE
 *	TRUE if successful, otherwise FALSE
 *
 * NOTE
 *	ProductType can be one of the following values:
 *	  1	Workstation (WinNT)
 *	  2	Server (LanmanNT)
 *	  3	Advanced Server (ServerNT)
 *
 * REVISIONS
 * 	2000-08-10 ekohl
 *
 * @implemented
 */

BOOLEAN NTAPI
RtlGetNtProductType(PNT_PRODUCT_TYPE ProductType)
{
  *ProductType = SharedUserData->NtProductType;
  return(TRUE);
}

/**********************************************************************
 * NAME							EXPORTED
 *	RtlGetNtVersionNumbers
 *
 * DESCRIPTION
 *	Get the version numbers of the run time library.
 *
 * ARGUMENTS
 *	pdwMajorVersion [OUT]	Destination for the Major version
 *	pdwMinorVersion [OUT]	Destination for the Minor version
 *	pdwBuildNumber  [OUT]	Destination for the Build version
 *
 * RETURN VALUE
 *	Nothing.
 *
 * NOTES
 *	- Introduced in Windows XP (NT 5.1)
 *	- Since this call didn't exist before XP, we report at least the version
 *	  5.1. This fixes the loading of msvcrt.dll as released with XP Home,
 *	  which fails in DLLMain() if the major version isn't 5.
 *
 * @implemented
 */

VOID NTAPI
RtlGetNtVersionNumbers(OUT LPDWORD pdwMajorVersion,
                       OUT LPDWORD pdwMinorVersion,
                       OUT LPDWORD pdwBuildNumber)
{
    PPEB pPeb = NtCurrentPeb();

    if (pdwMajorVersion)
    {
        *pdwMajorVersion = pPeb->OSMajorVersion < 5 ? 5 : pPeb->OSMajorVersion;
    }

    if (pdwMinorVersion)
    {
        if ( (pPeb->OSMajorVersion  < 5) ||
            ((pPeb->OSMajorVersion == 5) && (pPeb->OSMinorVersion < 1)) )
            *pdwMinorVersion = 1;
        else
            *pdwMinorVersion = pPeb->OSMinorVersion;
    }

    if (pdwBuildNumber)
    {
        /* Windows really does this! */
        *pdwBuildNumber = (0xF0000000 | pPeb->OSBuildNumber);
    }
}

/*
* @implemented
*/
NTSTATUS NTAPI
RtlGetVersion(RTL_OSVERSIONINFOW *Info)
{
   LONG i, MaxLength;

   if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
       Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      PPEB Peb = NtCurrentPeb();

      Info->dwMajorVersion = Peb->OSMajorVersion;
      Info->dwMinorVersion = Peb->OSMinorVersion;
      Info->dwBuildNumber = Peb->OSBuildNumber;
      Info->dwPlatformId = Peb->OSPlatformId;
      RtlZeroMemory(Info->szCSDVersion, sizeof(Info->szCSDVersion));
      if(((Peb->OSCSDVersion >> 8) & 0xFF) != 0)
      {
        MaxLength = (sizeof(Info->szCSDVersion) / sizeof(Info->szCSDVersion[0])) - 1;
        i = _snwprintf(Info->szCSDVersion,
                       MaxLength,
                       L"Service Pack %d",
                       ((Peb->OSCSDVersion >> 8) & 0xFF));
        if (i < 0)
        {
           /* null-terminate if it was overflowed */
           Info->szCSDVersion[MaxLength] = L'\0';
        }
      }
      if (Info->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)Info;
         InfoEx->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
         InfoEx->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
         InfoEx->wSuiteMask = SharedUserData->SuiteMask & 0xFFFF;
         InfoEx->wProductType = SharedUserData->NtProductType;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

/* EOF */
