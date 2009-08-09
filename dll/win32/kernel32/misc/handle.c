/* $Id$
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
#include <debug.h>

/* GLOBALS *******************************************************************/

HANDLE WINAPI
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD   dwDesiredAccess,
			BOOL	bInheritHandle,
			DWORD	dwOptions);

/* FUNCTIONS *****************************************************************/

HANDLE FASTCALL
TranslateStdHandle(HANDLE hHandle)
{
  PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;

  switch ((ULONG)hHandle)
    {
      case STD_INPUT_HANDLE:  return Ppb->StandardInput;
      case STD_OUTPUT_HANDLE: return Ppb->StandardOutput;
      case STD_ERROR_HANDLE:  return Ppb->StandardError;
    }

  return hHandle;
}

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
  DWORD Flags;

  hObject = TranslateStdHandle(hObject);

  Status = NtQueryObject (hObject,
			  ObjectHandleFlagInformation,
			  &HandleInfo,
			  sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
			  &BytesWritten);
  if (NT_SUCCESS(Status))
  {
    Flags = 0;
    if (HandleInfo.Inherit)
      Flags |= HANDLE_FLAG_INHERIT;
    if (HandleInfo.ProtectFromClose)
      Flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;

    *lpdwFlags = Flags;

    return TRUE;
  }
  else
  {
    SetLastErrorByStatus (Status);
    return FALSE;
  }
}


/*
 * @implemented
 */
BOOL WINAPI
SetHandleInformation (HANDLE hObject,
		      DWORD dwMask,
		      DWORD dwFlags)
{
  OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
  ULONG BytesWritten;
  NTSTATUS Status;

  hObject = TranslateStdHandle(hObject);

  Status = NtQueryObject (hObject,
			  ObjectHandleFlagInformation,
			  &HandleInfo,
			  sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
			  &BytesWritten);
  if (NT_SUCCESS(Status))
  {
    if (dwMask & HANDLE_FLAG_INHERIT)
      HandleInfo.Inherit = (dwFlags & HANDLE_FLAG_INHERIT) != 0;
    if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
      HandleInfo.ProtectFromClose = (dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;

    Status = NtSetInformationObject (hObject,
				     ObjectHandleFlagInformation,
				     &HandleInfo,
				     sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION));
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

    return TRUE;
  }
  else
  {
    SetLastErrorByStatus (Status);
    return FALSE;
  }
}


/*
 * @implemented
 */
BOOL WINAPI CloseHandle(HANDLE  hObject)
/*
 * FUNCTION: Closes an open object handle
 * PARAMETERS:
 *       hObject = Identifies an open object handle
 * RETURNS: If the function succeeds, the return value is nonzero
 *          If the function fails, the return value is zero
 */
{
   NTSTATUS Status;

   hObject = TranslateStdHandle(hObject);

   if (IsConsoleHandle(hObject))
     {
	return(CloseConsoleHandle(hObject));
     }

   Status = NtClose(hObject);
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
BOOL WINAPI DuplicateHandle(HANDLE hSourceProcessHandle,
				HANDLE hSourceHandle,
				HANDLE hTargetProcessHandle,
				LPHANDLE lpTargetHandle,
				DWORD dwDesiredAccess,
				BOOL bInheritHandle,
				DWORD dwOptions)
{
   DWORD SourceProcessId, TargetProcessId;
   NTSTATUS Status;

   hSourceHandle = TranslateStdHandle(hSourceHandle);

   if (IsConsoleHandle(hSourceHandle))
   {
      SourceProcessId = GetProcessId(hSourceProcessHandle);
      TargetProcessId = GetProcessId(hTargetProcessHandle);
      if (!SourceProcessId || !TargetProcessId ||
	  SourceProcessId != TargetProcessId ||
	  SourceProcessId != GetCurrentProcessId())
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
      }

      *lpTargetHandle = DuplicateConsoleHandle(hSourceHandle, dwDesiredAccess, bInheritHandle, dwOptions);
      return *lpTargetHandle != INVALID_HANDLE_VALUE;
   }

   Status = NtDuplicateObject(hSourceProcessHandle,
			      hSourceHandle,
			      hTargetProcessHandle,
			      lpTargetHandle,
			      dwDesiredAccess,
			      bInheritHandle ? OBJ_INHERIT : 0,
			      dwOptions);
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
UINT WINAPI SetHandleCount(UINT nCount)
{
   return(nCount);
}

/* EOF */
