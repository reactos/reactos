/* $Id: misc.c,v 1.2 2004/03/13 20:49:07 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include <ntos.h>
#include <windows.h>
#include <string.h>

#include "internal.h"


/* FUNCTIONS ***************************************************************/

LPWSTR
AppendBackslash (LPWSTR String)
{
  ULONG Length;

  Length = lstrlenW (String);
  if (String[Length - 1] != L'\\')
    {
      String[Length] = L'\\';
      Length++;
      String[Length] = (WCHAR)0;
    }

  return &String[Length];
}


BOOL
GetUserSidFromToken (HANDLE hToken,
		     PUNICODE_STRING SidString)
{
  PSID_AND_ATTRIBUTES SidBuffer;
  ULONG Length;
  NTSTATUS Status;

  Length = 256;
  SidBuffer = LocalAlloc (0,
			  Length);
  if (SidBuffer == NULL)
    return FALSE;

  Status = NtQueryInformationToken (hToken,
				    TokenUser,
				    (PVOID)SidBuffer,
				    Length,
				    &Length);
  if (Status == STATUS_BUFFER_TOO_SMALL)
    {
      SidBuffer = LocalReAlloc (SidBuffer,
				Length,
				0);
      if (SidBuffer == NULL)
	return FALSE;

      Status = NtQueryInformationToken (hToken,
					TokenUser,
					(PVOID)SidBuffer,
					Length,
					&Length);
    }

  if (!NT_SUCCESS (Status))
    {
      LocalFree (SidBuffer);
      return FALSE;
    }

  DPRINT ("SidLength: %lu\n", RtlLengthSid (SidBuffer[0].Sid));

  Status = RtlConvertSidToUnicodeString (SidString,
					 SidBuffer[0].Sid,
					 TRUE);

  LocalFree (SidBuffer);

  if (!NT_SUCCESS (Status))
    return FALSE;

  DPRINT ("SidString.Length: %lu\n", SidString->Length);
  DPRINT ("SidString.MaximumLength: %lu\n", SidString->MaximumLength);
  DPRINT ("SidString: '%wZ'\n", SidString);

  return TRUE;
}

/* EOF */
