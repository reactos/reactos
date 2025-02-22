/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/wstring/wcsnicmp.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
int CDECL _wcsnicmp (const wchar_t *cs, const wchar_t *ct, size_t count)
{
	if (count == 0)
		return 0;
	do {
		if (towupper(*cs) != towupper(*ct++))
			return towupper(*cs) - towupper(*--ct);
		if (*cs++ == 0)
			break;
	} while (--count != 0);
	return 0;
}
