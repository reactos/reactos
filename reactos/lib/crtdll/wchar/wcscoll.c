

#include <crtdll/wchar.h>

int wcscoll(const wchar_t *a1,const wchar_t *a2)
{
	/* FIXME: handle collates */
	return wcscmp(a1,a2);
}


int _wcsicoll(const wchar_t *a1,const wchar_t *a2)
{
	/* FIXME: handle collates */
	return _wcsicmp(a1,a2);
}

