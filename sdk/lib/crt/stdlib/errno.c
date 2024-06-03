/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/stdlib/errno.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 *
 */
#include <precomp.h>
#include "doserrmap.h"
#include <errno.h>
#include <internal/wine/msvcrt.h>

/*********************************************************************
 *		_errno (MSVCRT.@)
 */
int* CDECL _errno(void)
{
    return &(msvcrt_get_thread_data()->thread_errno);
}

/*********************************************************************
 *		__doserrno (MSVCRT.@)
 */
unsigned long* CDECL __doserrno(void)
{
    return &(msvcrt_get_thread_data()->thread_doserrno);
}

/*********************************************************************
 *		_get_errno (MSVCRT.@)
 */
errno_t CDECL _get_errno(int *pValue)
{
    if (!pValue)
        return EINVAL;

    *pValue = *_errno();
    return 0;
}

/*********************************************************************
 *		_get_doserrno (MSVCRT.@)
 */
errno_t CDECL _get_doserrno(unsigned long *pValue)
{
    if (!pValue)
        return EINVAL;

    *pValue = *__doserrno();
    return 0;
}

/*********************************************************************
 *		_set_errno (MSVCRT.@)
 */
errno_t CDECL _set_errno(int value)
{
    *_errno() = value;
    return 0;
}

/*********************************************************************
 *		_set_doserrno (MSVCRT.@)
 */
errno_t CDECL _set_doserrno(unsigned long value)
{
    *__doserrno() = value;
    return 0;
}

/*
 * This function sets both doserrno to the passed in OS error code
 * and also maps this to an appropriate errno code.  The mapping
 * has been deduced automagically by running this functions, which
 * exists in MSVCRT but is undocumented, on all the error codes in
 * winerror.h.
 */
void CDECL _dosmaperr(unsigned long oserror)
{
	int pos, base, lim;

	_set_doserrno(oserror);

	/* Use binary chop to find the corresponding errno code */
	for (base=0, lim=sizeof(doserrmap)/sizeof(doserrmap[0]); lim; lim >>= 1) {
		pos = base+(lim >> 1);
		if (doserrmap[pos].winerr == oserror) {
			_set_errno(doserrmap[pos].en);
			return;
		} else if (doserrmap[pos].winerr < oserror) {
			base = pos + 1;
			--lim;
		}
	}
	/* EINVAL appears to be the default */
	_set_errno(EINVAL);
}

/******************************************************************************
*              _set_error_mode (MSVCRT.@)
*
* Set the error mode, which describes where the C run-time writes error
* messages.
*
* PARAMS
*   mode - the new error mode
*
* RETURNS
*   The old error mode.
*
*/
int msvcrt_error_mode = MSVCRT__OUT_TO_DEFAULT;

int CDECL _set_error_mode(int mode)
{
    const int old = msvcrt_error_mode;
    if ( MSVCRT__REPORT_ERRMODE != mode ) {
        msvcrt_error_mode = mode;
    }
    return old;
}

/******************************************************************************
 *		_seterrormode (MSVCRT.@)
 */
void CDECL _seterrormode(int mode)
{
    SetErrorMode( mode );
}
