/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

#include <crtdll/wchar.h>


size_t wcslen(const wchar_t * s)
{
	const wchar_t *save;

	if (s == 0)
		return 0;
	for (save = s; *save; ++save);
	return save-s;
}

size_t wstrlen(const wchar_t *s)
{
    return wcslen(s);
}


