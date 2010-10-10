/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/errno.c
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 *
 */
#include <precomp.h>
#include "doserrmap.h"
#include <errno.h>
#include <internal/wine/msvcrt.h>

static _invalid_parameter_handler invalid_parameter_handler = NULL;

/*
 * @implemented
 */
unsigned long* __doserrno(void)
{
  return (unsigned long*)(&GetThreadData()->tdoserrno);
}

/*
 * @implemented
 */
int *_errno(void)
{
  return(&GetThreadData()->terrno);
}


int __set_doserrno(int error)
{
  PTHREADDATA ThreadData;

  ThreadData = GetThreadData();
  if (ThreadData)
    ThreadData->tdoserrno = error;

  return(error);
}

int __set_errno(int error)
{
  PTHREADDATA ThreadData;

  ThreadData = GetThreadData();
  if (ThreadData)
    ThreadData->terrno = error;

  return(error);
}

/*
 * This function sets both doserrno to the passed in OS error code
 * and also maps this to an appropriate errno code.  The mapping
 * has been deduced automagically by running this functions, which
 * exists in MSVCRT but is undocumented, on all the error codes in
 * winerror.h.
 */
void _dosmaperr(unsigned long oserror)
{
	int pos, base, lim;

	__set_doserrno(oserror);

	/* Use binary chop to find the corresponding errno code */
	for (base=0, lim=sizeof(doserrmap)/sizeof(doserrmap[0]); lim; lim >>= 1) {
		pos = base+(lim >> 1);
		if (doserrmap[pos].winerr == oserror) {
			__set_errno(doserrmap[pos].en);
			return;
		} else if (doserrmap[pos].winerr < oserror) {
			base = pos + 1;
			--lim;
		}
	}
	/* EINVAL appears to be the default */
	__set_errno(EINVAL);
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
* TODO
*  This function does not have a proper implementation; the error mode is
*  never used.
*/
int CDECL _set_error_mode(int mode)
{
    static int current_mode = MSVCRT__OUT_TO_DEFAULT;

    const int old = current_mode;
    if ( MSVCRT__REPORT_ERRMODE != mode ) {
        current_mode = mode;

    }
    return old;
}

/******************************************************************************
*              _seterrormode (MSVCRT.@)
*/
void CDECL _seterrormode(int mode)
{
    SetErrorMode( mode );
}

/******************************************************************************
 *		_invalid_parameter (MSVCRT.@)
 */
void __cdecl _invalid_parameter(const wchar_t *expr, const wchar_t *func,
                                       const wchar_t *file, unsigned int line, uintptr_t arg)
{
    if (invalid_parameter_handler) invalid_parameter_handler( expr, func, file, line, arg );
    else
    {
        ERR( "%s:%u %s: %s %lx\n", debugstr_w(file), line, debugstr_w(func), debugstr_w(expr), arg );
        RaiseException( STATUS_INVALID_CRUNTIME_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, NULL );
    }
}

/* _get_invalid_parameter_handler - not exported in native msvcrt, added in msvcr80 */
_invalid_parameter_handler CDECL _get_invalid_parameter_handler(void)
{
    TRACE("\n");
    return invalid_parameter_handler;
}

/* _set_invalid_parameter_handler - not exproted in native msvcrt, added in msvcr80 */
_invalid_parameter_handler CDECL _set_invalid_parameter_handler(
        _invalid_parameter_handler handler)
{
    _invalid_parameter_handler old = invalid_parameter_handler;

    TRACE("(%p)\n", handler);

    invalid_parameter_handler = handler;
    return old;
}
/* EOF */
