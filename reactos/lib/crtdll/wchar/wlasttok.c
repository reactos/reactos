#include <msvcrt/wchar.h>

/*
 * This is a CRTDLL internal function to return the lasttoken
 * bit of data used by wcstok. The reason for it's existence is
 * so that CRTDLL can use MSVCRT's implementation of wcstok.
 */
wchar_t** _wlasttoken()
{
	static wchar_t *wlasttoken = NULL;
	return &wlasttoken;
}
