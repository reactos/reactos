/* $Id: audit.c,v 1.1 2003/07/20 00:03:40 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/audit.c
 * PURPOSE:         Audit functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 07/19/2003
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
ObjectCloseAuditAlarmA (LPCSTR SubsystemName,
			LPVOID HandleId,
			BOOL GenerateOnClose)
{
  UNICODE_STRING Name;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz (&Name,
					     (PCHAR)SubsystemName);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = NtCloseObjectAuditAlarm (&Name,
				    HandleId,
				    GenerateOnClose);
  RtlFreeUnicodeString(&Name);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
ObjectCloseAuditAlarmW (LPCWSTR SubsystemName,
			LPVOID HandleId,
			BOOL GenerateOnClose)
{
  UNICODE_STRING Name;
  NTSTATUS Status;

  RtlInitUnicodeString (&Name,
			(PWSTR)SubsystemName);

  Status = NtCloseObjectAuditAlarm (&Name,
				    HandleId,
				    GenerateOnClose);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ObjectDeleteAuditAlarmA (LPCSTR SubsystemName,
			 LPVOID HandleId,
			 BOOL GenerateOnClose)
{
  UNICODE_STRING Name;
  NTSTATUS Status;

  Status = RtlCreateUnicodeStringFromAsciiz (&Name,
					     (PCHAR)SubsystemName);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  Status = NtDeleteObjectAuditAlarm (&Name,
				     HandleId,
				     GenerateOnClose);
  RtlFreeUnicodeString(&Name);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ObjectDeleteAuditAlarmW (LPCWSTR SubsystemName,
			 LPVOID HandleId,
			 BOOL GenerateOnClose)
{
  UNICODE_STRING Name;
  NTSTATUS Status;

  RtlInitUnicodeString (&Name,
			(PWSTR)SubsystemName);

  Status = NtDeleteObjectAuditAlarm (&Name,
				     HandleId,
				     GenerateOnClose);
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  return TRUE;
}


/* EOF */
