/* $Id: rtl.c,v 1.3 2000/07/01 17:07:00 ea Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/misc/rtl.c
 * PURPOSE:              
 * PROGRAMMER:           Boudewijn Dekker
 */

/* INCLUDES ******************************************************************/

#include <windows.h>

typedef DWORD ( *RtlFillMemoryType) (DWORD Unknown0, DWORD Unknown1, DWORD Unknown2 );
#undef FillMemory
DWORD
STDCALL
RtlFillMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	HINSTANCE hModule;
	RtlFillMemoryType FillMemory; 
	hModule = LoadLibraryA("ntdll.dll");
	if ( hModule == NULL )
		return -1;
	FillMemory = (RtlFillMemoryType)GetProcAddress(hModule, "RtlFillMemory");
	if ( FillMemory == NULL )
		return -1;
	return FillMemory(Unknown0, Unknown1, Unknown2);
   

}

typedef DWORD ( *RtlMoveMemoryType) (DWORD Unknown0, DWORD Unknown1, DWORD Unknown2 );
#undef MoveMemory
DWORD
STDCALL
RtlMoveMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	HINSTANCE hModule;
	RtlMoveMemoryType MoveMemory; 
	hModule = LoadLibraryA("ntdll.dll");
	if ( hModule == NULL )
		return -1;
	MoveMemory = (RtlMoveMemoryType)GetProcAddress(hModule, "RtlMoveMemory");
	if ( MoveMemory == NULL )
		return -1;
	return MoveMemory(Unknown0, Unknown1, Unknown2);
}

typedef DWORD ( *RtlZeroMemoryType) (DWORD Unknown0, DWORD Unknown1 );
#undef ZeroMemory
DWORD
STDCALL
RtlZeroMemory (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	HINSTANCE hModule;
	RtlZeroMemoryType ZeroMemory; 
	hModule = LoadLibraryA("ntdll.dll");
	if ( hModule == NULL )
		return -1;
	ZeroMemory = (RtlZeroMemoryType)GetProcAddress(hModule, "RtlZeroMemory");
	if ( ZeroMemory == NULL )
		return -1;
	return ZeroMemory(Unknown0, Unknown1);
}

typedef DWORD ( *RtlUnwindType) (DWORD	Unknown0, DWORD	Unknown1, DWORD	Unknown2, DWORD Unknown3 );
#undef Unwind
DWORD
STDCALL
RtlUnwind (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	HINSTANCE hModule;
	RtlUnwindType Unwind; 
	hModule = LoadLibraryA("ntdll.dll");
	if ( hModule == NULL )
		return -1;
	Unwind = (RtlUnwindType)GetProcAddress(hModule, "RtlUnwind");
	if ( Unwind == NULL )
		return -1;
	return Unwind(Unknown0, Unknown1, Unknown2, Unknown3);
}


/* EOF */
