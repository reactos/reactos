/* $Id: handle.c,v 1.16 2004/01/23 21:16:03 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/handle.c
 * PURPOSE:         Object  functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* GLOBALS *******************************************************************/

BOOL STDCALL
InternalGetProcessId (HANDLE hProcess, LPDWORD lpProcessId);

HANDLE STDCALL
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD   dwDesiredAccess,
			BOOL	bInheritHandle,
			DWORD	dwOptions);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL WINAPI
GetHandleInformation (HANDLE hObject,
		      LPDWORD lpdwFlags)
{
  OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
  ULONG BytesWritten;
  NTSTATUS Status;

  Status = NtQueryObject (hObject,
			  ObjectHandleInformation,
			  &HandleInfo,
			  sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
			  &BytesWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
	return FALSE;
    }

  if (HandleInfo.Inherit)
    *lpdwFlags &= HANDLE_FLAG_INHERIT;

  if (HandleInfo.ProtectFromClose)
    *lpdwFlags &= HANDLE_FLAG_PROTECT_FROM_CLOSE;

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetHandleInformation (HANDLE hObject,
		      DWORD dwMask,
		      DWORD dwFlags)
{
  OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
  ULONG BytesWritten;
  NTSTATUS Status;

  Status = NtQueryObject (hObject,
			  ObjectHandleInformation,
			  &HandleInfo,
			  sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
			  &BytesWritten);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  if (dwMask & HANDLE_FLAG_INHERIT)
    {
      HandleInfo.Inherit = dwFlags & HANDLE_FLAG_INHERIT;
    }

  if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
    {
      HandleInfo.ProtectFromClose = dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE;
    }

  Status = NtSetInformationObject (hObject,
				   ObjectHandleInformation,
				   &HandleInfo,
				   sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL CloseHandle(HANDLE  hObject)
/*
 * FUNCTION: Closes an open object handle
 * PARAMETERS:
 *       hObject = Identifies an open object handle
 * RETURNS: If the function succeeds, the return value is nonzero
 *          If the function fails, the return value is zero
 */
{
   NTSTATUS errCode;
   
   if (IsConsoleHandle(hObject))
     {
	return(CloseConsoleHandle(hObject));
     }
   
   errCode = NtClose(hObject);
   if (!NT_SUCCESS(errCode)) 
     {     
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   
   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL DuplicateHandle(HANDLE hSourceProcessHandle,
				HANDLE hSourceHandle,
				HANDLE hTargetProcessHandle,
				LPHANDLE lpTargetHandle,
				DWORD dwDesiredAccess,
				BOOL bInheritHandle,
				DWORD dwOptions)
{
   NTSTATUS errCode;
   DWORD SourceProcessId, TargetProcessId;
   if (IsConsoleHandle(hSourceHandle))
   {
      if (FALSE == InternalGetProcessId(hSourceProcessHandle, &SourceProcessId) || 
	  FALSE == InternalGetProcessId(hTargetProcessHandle, &TargetProcessId) ||
	  SourceProcessId != TargetProcessId ||
	  SourceProcessId != GetCurrentProcessId())
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
      }

      *lpTargetHandle = DuplicateConsoleHandle(hSourceHandle, dwDesiredAccess, bInheritHandle, dwOptions);
      return *lpTargetHandle != INVALID_HANDLE_VALUE ? TRUE : FALSE;
   }
      
   errCode = NtDuplicateObject(hSourceProcessHandle,
			       hSourceHandle,
			       hTargetProcessHandle,
			       lpTargetHandle, 
			       dwDesiredAccess, 
			       (BOOLEAN)bInheritHandle,
			       dwOptions);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   
   return TRUE;
}


/*
 * @implemented
 */
UINT STDCALL SetHandleCount(UINT nCount)
{
   return(nCount);
}

/* EOF */
