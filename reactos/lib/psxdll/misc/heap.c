/* $Id: heap.c,v 1.1 1999/09/12 21:54:16 ea Exp $
 *
 * reactos/lib/psxdll/misc/heap.c
 *
 * ReactOS Operating System
 *
 */
#define NTOS_MODE_USER
#include <ntos.h>

HANDLE
STDCALL
GetProcessHeap (
	VOID
	)
{
	/* FIXME: Right? */
	return RtlGetProcessHeap ();
}


/*
 * NOTE: These functions are forwarded programmatically
 * to NTDLL: they should be imported automatically by the
 * linker instead.
 */
LPVOID
STDCALL
HeapAlloc (
	HANDLE	hHeap,
	DWORD	dwFlags,
	DWORD	dwBytes
	)
{
	return RtlAllocateHeap (
		hHeap,
		dwFlags,
		dwBytes
		);
}


WINBOOL
STDCALL
HeapFree (
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	lpMem
	)
{
	return RtlFreeHeap (
		hHeap,
		dwFlags,
		lpMem
		);
}


LPVOID
STDCALL
HeapReAlloc (
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPVOID	lpMem,
	DWORD	dwBytes
	)
{
	return RtlReAllocateHeap (
		hHeap,
		dwFlags,
		lpMem,
		dwBytes
		);
}


DWORD
STDCALL
HeapSize(
	HANDLE	hHeap,
	DWORD	dwFlags,
	LPCVOID	lpMem
	)
{
	return RtlSizeHeap (
		hHeap,
		dwFlags,
		lpMem
		);
}


/* EOF */
