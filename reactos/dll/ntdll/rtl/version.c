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
 *	  1	Workstation (Winnt)
 *	  2	Server (Lanmannt)
 *	  3	Advanced Server (Servernt)
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
 *	major [OUT]	Destination for the Major version
 *	minor [OUT]	Destination for the Minor version
 *	build [OUT]	Destination for the Build version
 *
 * RETURN VALUE
 *	Nothing.
 *
 * NOTE
 *	Introduced in Windows XP (NT5.1)
 *
 * @implemented
 */

void NTAPI
RtlGetNtVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build)
{
	PPEB pPeb = NtCurrentPeb();

	if (major)
	{
		/* msvcrt.dll as released with XP Home fails in DLLMain() if the
		 * major version is not 5. So, we should never set a version < 5 ...
		 * This makes sense since this call didn't exist before XP anyway.
		 */
		*major = pPeb->OSMajorVersion < 5 ? 5 : pPeb->OSMajorVersion;
	}

	if (minor)
	{
		if (pPeb->OSMinorVersion <= 5)
			*minor = pPeb->OSMinorVersion < 1 ? 1 : pPeb->OSMinorVersion;
		else
			*minor = pPeb->OSMinorVersion;
	}

	if (build)
	{
		/* FIXME: Does anybody know the real formula? */
		*build = (0xF0000000 | pPeb->OSBuildNumber);
	}
}

/*
* @implemented
*/
NTSTATUS NTAPI
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
