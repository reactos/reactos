/* $Id$
 *
 * version.dll stubs: remove from this file if
 * you implement one of these functions.
 */
#include <windows.h>

#ifndef HAVE_DLL_FORWARD

/* VerQueryValueIndex seems undocumented */

/*
 * @unimplemented
 */
DWORD
STDCALL
VerQueryValueIndexA (
	DWORD Unknown0,
	DWORD Unknown1,
	DWORD Unknown2,
	DWORD Unknown3,
	DWORD Unknown4,
	DWORD Unknown5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VerQueryValueIndexW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}
#endif
/* EOF */
