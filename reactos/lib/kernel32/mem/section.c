/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/section.c
 * PURPOSE:              Implementing file mapping
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

/* FUNCTIONS *****************************************************************/

HANDLE CreationFileMappingA(HANDLE hFile,
			    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
			    DWORD flProtect,
			    DWORD dwMaximumSizeHigh,
			    DWORD dwMaximumSizeLow,
			    LPCSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   LARGE_INTEGER MaximumSize;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ANSI_STRING AnsiName;
   UNICODE_STRING UnicodeName;
   
   SET_LARGE_INTEGER_LOW_PART(MaximumSize, dwMaximumSizeLow);
   SET_LARGE_INTEGER_HIGH_PART(MaximumSize, dwMaximumSizeHigh);
   RtlInitAnsiString(&AnsiString, lpName);
   RtlAnsiStringToUnicodeString(&UnicodeName, &AnsiName, TRUE);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      NULL,
			      lpFileMappingAttributes);
   Status = ZwCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes,
			    &MaximumSize,
			    flProtect,
			    0,
			    hFile);
   if (!NT_SUCCESS(Status))
     {
	return(SectionHandle);
     }
   return(NULL);
}

HANDLE CreationFileMappingW(HANDLE hFile,
			    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
			    DWORD flProtect,
			    DWORD dwMaximumSizeHigh,
			    DWORD dwMaximumSizeLow,
			    LPCWSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   LARGE_INTEGER MaximumSize;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UnicodeName;
   
   SET_LARGE_INTEGER_LOW_PART(MaximumSize, dwMaximumSizeLow);
   SET_LARGE_INTEGER_HIGH_PART(MaximumSize, dwMaximumSizeHigh);
   RtlInitUnicodeString(&UnicodeString, lpName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      NULL,
			      lpFileMappingAttributes);
   Status = ZwCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes,
			    &MaximumSize,
			    flProtect,
			    0,
			    hFile);
   if (!NT_SUCCESS(Status))
     {
	return(SectionHandle);
     }
   return(NULL);
}

LPVOID MapViewOfFileEx(HANDLE hFileMappingObject,
		       DWORD dwDesiredAccess,
		       DWORD dwFileOffsetHigh,
		       DWORD dwFileOffsetLow,
		       DWORD dwNumberOfBytesToMap,
		       LPVOID lpBaseAddress)
{
   NTSTATUS Status;
   
   Status = ZwMapViewOfSection(hFileMappingObject,
			       NtCurrentProcess(),
			       &lpBaseAddress,
			       0,
			       dwNumberOfBytesToMap,
			       
}
