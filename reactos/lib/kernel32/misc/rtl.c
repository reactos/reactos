/* $Id: rtl.c,v 1.4 2001/02/10 22:51:07 dwelch Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/misc/rtl.c
 * PURPOSE:              
 * PROGRAMMER:           Boudewijn Dekker
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

typedef DWORD 
(*RtlFillMemoryType) (PVOID Destination, ULONG Length, UCHAR Fill);

#undef FillMemory
VOID STDCALL
RtlFillMemory (PVOID Destination, ULONG	Length, UCHAR Fill)
{
  HINSTANCE hModule;
  RtlFillMemoryType FillMemory; 

  hModule = LoadLibraryA("ntdll.dll");
  if (hModule == NULL)
    return;
  FillMemory = (RtlFillMemoryType)GetProcAddress(hModule, "RtlFillMemory");
  if ( FillMemory == NULL )
    return;
  FillMemory(Destination, Length, Fill);
}

typedef DWORD 
(*RtlMoveMemoryType) (PVOID Destination, CONST VOID* Source, ULONG Length);
#undef MoveMemory
VOID STDCALL
RtlMoveMemory (PVOID Destination, CONST VOID* Source, ULONG Length)
{
  HINSTANCE hModule;
  RtlMoveMemoryType MoveMemory; 

  hModule = LoadLibraryA("ntdll.dll");
  if (hModule == NULL)
    return;
  MoveMemory = (RtlMoveMemoryType)GetProcAddress(hModule, "RtlMoveMemory");
  if (MoveMemory == NULL)
    return;
  MoveMemory(Destination, Source, Length);
}

typedef DWORD ( *RtlZeroMemoryType) (PVOID Destination, ULONG Length);
#undef ZeroMemory
VOID STDCALL
RtlZeroMemory (PVOID Destination, ULONG Length)
{
  HINSTANCE hModule;
  RtlZeroMemoryType ZeroMemory; 

  hModule = LoadLibraryA("ntdll.dll");
  if (hModule == NULL)
    return;
  ZeroMemory = (RtlZeroMemoryType)GetProcAddress(hModule, "RtlZeroMemory");
  if (ZeroMemory == NULL)
    return;
  ZeroMemory(Destination, Length);
}

typedef DWORD ( *RtlUnwindType) (DWORD	Unknown0, DWORD	Unknown1, DWORD	Unknown2, DWORD Unknown3 );
#undef Unwind
VOID
STDCALL
RtlUnwind (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2,
        ULONG	Unknown3
	)
{
	HINSTANCE hModule;
	RtlUnwindType Unwind; 
	hModule = LoadLibraryA("ntdll.dll");
	if ( hModule == NULL )
		return;
	Unwind = (RtlUnwindType)GetProcAddress(hModule, "RtlUnwind");
	if ( Unwind == NULL )
		return;
	Unwind(Unknown0, Unknown1, Unknown2, Unknown3);
}


/* EOF */
