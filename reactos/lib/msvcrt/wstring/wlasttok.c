#include <msvcrt/internal/tls.h>

/*
 * This is an MSVCRT internal function to return the lasttoken
 * bit of data used by wcstok. The reason for it's existence is
 * so that CRTDLL can use the wcstok source code in the same
 * file.
 */
wchar_t** _wlasttoken()
{
	PTHREADDATA ptd = GetThreadData();
	_assert(ptd);
	return &(ptd->wlasttoken);
}
