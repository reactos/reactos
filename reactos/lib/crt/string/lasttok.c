#ifdef __USE_W32API
#undef __USE_W32API
#endif

#include <msvcrt/internal/tls.h>
#include <msvcrt/assert.h>

/*
 * This is an MSVCRT internal function to return the lasttoken
 * bit of data used by strtok. The reason for it's existence is
 * so that CRTDLL can use the strtok source code in the same
 * file.
 */
char** _lasttoken()
{
	PTHREADDATA ptd = GetThreadData();
	assert(ptd);
	return &(ptd->lasttoken);
}
