/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */
#include <precomp.h>

#include <internal/tls.h>
#include <assert.h>

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
