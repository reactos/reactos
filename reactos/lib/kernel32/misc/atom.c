/* $Id: atom.c,v 1.13 2001/05/27 15:40:31 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMER:      Eric Kohl ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Full rewrite 27/05/2001
 */

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/error.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBALS *******************************************************************/

static PRTL_ATOM_TABLE LocalAtomTable = NULL;

static PRTL_ATOM_TABLE GetLocalAtomTable(VOID);


/* FUNCTIONS *****************************************************************/

ATOM STDCALL
GlobalAddAtomA(LPCSTR lpString)
{
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);

   Status = NtAddAtom(AtomName.Buffer,
		      &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
GlobalAddAtomW(LPCWSTR lpString)
{
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   Status = NtAddAtom((LPWSTR)lpString,
		      &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
GlobalDeleteAtom(ATOM nAtom)
{
   NTSTATUS Status;
   
   if (nAtom < 0xC000)
     {
	return 0;
     }
   
   Status = NtDeleteAtom(nAtom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return nAtom;
     }
   
   return 0;
}


ATOM STDCALL
GlobalFindAtomA(LPCSTR lpString)
{
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);
   Status = NtFindAtom(AtomName.Buffer,
		       &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
GlobalFindAtomW(LPCWSTR lpString)
{
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   Status = NtFindAtom((LPWSTR)lpString,
		       &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


UINT STDCALL
GlobalGetAtomNameA(ATOM nAtom,
		   LPSTR lpBuffer,
		   int nSize)
{
   PATOM_BASIC_INFORMATION Buffer;
   UNICODE_STRING AtomNameU;
   ANSI_STRING AtomName;
   ULONG BufferSize;
   ULONG ReturnLength;
   NTSTATUS Status;

   BufferSize = sizeof(ATOM_BASIC_INFORMATION) + nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    BufferSize);

   Status = NtQueryInformationAtom(nAtom,
				   AtomBasicInformation,
				   (PVOID)&Buffer,
				   BufferSize,
				   &ReturnLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
	return 0;
     }

   RtlInitUnicodeString(&AtomNameU,
			Buffer->Name);
   AtomName.Buffer = lpBuffer;
   AtomName.Length = 0;
   AtomName.MaximumLength = nSize;
   RtlUnicodeStringToAnsiString(&AtomName,
				&AtomNameU,
				FALSE);

   ReturnLength = AtomName.Length;
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return ReturnLength;
}


UINT STDCALL
GlobalGetAtomNameW(ATOM nAtom,
		   LPWSTR lpBuffer,
		   int nSize)
{
   PATOM_BASIC_INFORMATION Buffer;
   ULONG BufferSize;
   ULONG ReturnLength;
   NTSTATUS Status;

   BufferSize = sizeof(ATOM_BASIC_INFORMATION) + nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    BufferSize);

   Status = NtQueryInformationAtom(nAtom,
				   AtomBasicInformation,
				   (PVOID)&Buffer,
				   BufferSize,
				   &ReturnLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
	return 0;
     }

   wcscpy(lpBuffer, Buffer->Name);
   ReturnLength = Buffer->NameLength / sizeof(WCHAR);
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return ReturnLength;
}


static PRTL_ATOM_TABLE
GetLocalAtomTable(VOID)
{
   if (LocalAtomTable != NULL)
     {
	return LocalAtomTable;
     }
   RtlCreateAtomTable(37,
		      &LocalAtomTable);
   return LocalAtomTable;
}


BOOL STDCALL
InitAtomTable(DWORD nSize)
{
   NTSTATUS Status;
   
   /* nSize should be a prime number */
   
   if ( nSize < 4 || nSize >= 512 )
     {
	nSize = 37;
     }
   
   if (LocalAtomTable == NULL)
    {
	Status = RtlCreateAtomTable(nSize,
				    &LocalAtomTable);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return FALSE;
	  }
    }

  return TRUE;
}


ATOM STDCALL
AddAtomA(LPCSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);

   Status = RtlAddAtomToAtomTable(AtomTable,
				  AtomName.Buffer,
				  &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
AddAtomW(LPCWSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlAddAtomToAtomTable(AtomTable,
				  (LPWSTR)lpString,
				  &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
DeleteAtom(ATOM nAtom)
{
   PRTL_ATOM_TABLE AtomTable;
   NTSTATUS Status;
   
   if (nAtom < 0xC000)
     {
	return 0;
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlDeleteAtomFromAtomTable(AtomTable,
				       nAtom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return nAtom;
     }
   
   return 0;
}


ATOM STDCALL
FindAtomA(LPCSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();
   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);
   Status = RtlLookupAtomInAtomTable(AtomTable,
				     AtomName.Buffer,
				     &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


ATOM STDCALL
FindAtomW(LPCWSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlLookupAtomInAtomTable(AtomTable,
				     (LPWSTR)lpString,
				     &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


UINT STDCALL
GetAtomNameA(ATOM nAtom,
	     LPSTR lpBuffer,
	     int nSize)
{
   PRTL_ATOM_TABLE AtomTable;
   PWCHAR Buffer;
   UNICODE_STRING AtomNameU;
   ANSI_STRING AtomName;
   ULONG NameLength;
   NTSTATUS Status;

   AtomTable = GetLocalAtomTable();

   NameLength = nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    NameLength);

   Status = RtlQueryAtomInAtomTable(AtomTable,
				    nAtom,
				    NULL,
				    NULL,
				    Buffer,
				    &NameLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
	return 0;
     }

   RtlInitUnicodeString(&AtomNameU,
			Buffer);
   AtomName.Buffer = lpBuffer;
   AtomName.Length = 0;
   AtomName.MaximumLength = nSize;
   RtlUnicodeStringToAnsiString(&AtomName,
				&AtomNameU,
				FALSE);

   NameLength = AtomName.Length;
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return NameLength;
}


UINT STDCALL
GetAtomNameW(ATOM nAtom,
	     LPWSTR lpBuffer,
	     int nSize)
{
   PRTL_ATOM_TABLE AtomTable;
   ULONG NameLength;
   NTSTATUS Status;

   AtomTable = GetLocalAtomTable();

   NameLength = nSize * sizeof(WCHAR);
   Status = RtlQueryAtomInAtomTable(AtomTable,
				    nAtom,
				    NULL,
				    NULL,
				    lpBuffer,
				    &NameLength);
   if (!NT_SUCCESS(Status))
     {
	return 0;
     }

   return(NameLength / sizeof(WCHAR));
}

/* EOF */
