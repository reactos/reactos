/*
 * PROJECT:         MSVC runtime check support library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Provides support functions for MSVC runtime checks
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <rtcapi.h>

extern _RTC_error_fnW _RTC_pErrorFuncW;

int
__cdecl
_RTC_DefaultErrorFuncW(
    int errType,
    const wchar_t *file,
    int line,
    const wchar_t *module,
    const wchar_t *format,
    ...);

static
char*
_RTC_ErrorDescription[] =
{
    "The stack pointer was wrong after returning from a function call.", /* _RTC_CHKSTK */
    "Data was lost when a type was converted to a smaller type.",        /* _RTC_CVRT_LOSS_INFO */
    "The stack near a local variable was corrupted.",                    /* _RTC_CORRUPT_STACK */
    "An uninitialized local variable was used.",                         /* _RTC_UNINIT_LOCAL_USE */
    "The stack around an alloca was corrupted.",                         /* _RTC_CORRUPTED_ALLOCA */
};

int
__cdecl
_RTC_NumErrors(void)
{
    /* Not supported yet */
    __debugbreak();
    return 0;
}

const char *
__cdecl
_RTC_GetErrDesc(
    _RTC_ErrorNumber _Errnum)
{
    if (_Errnum < (sizeof(_RTC_ErrorDescription) / sizeof(_RTC_ErrorDescription[0])))
    {
        return _RTC_ErrorDescription[_Errnum];
    }

    return "Invalid/Unknown error.";
}

int
__cdecl
_RTC_SetErrorType(
    _RTC_ErrorNumber _Errnum,
    int _ErrType)
{
    /* Not supported yet */
    __debugbreak();
    return 0;
}

_RTC_error_fn
__cdecl
_RTC_SetErrorFunc(
    _RTC_error_fn new_fn)
{
    /* Not supported yet */
    __debugbreak();
    return 0;
}

_RTC_error_fnW
__cdecl
_RTC_SetErrorFuncW(_RTC_error_fnW new_fn)
{
    _RTC_error_fnW old_fn;

    /* Get the current error func */
    old_fn = _RTC_pErrorFuncW;

    /* Set the new function or reset when 0 was passed */
    _RTC_pErrorFuncW = new_fn ? new_fn : _RTC_DefaultErrorFuncW;

    /* Return the old error func, or 0, if none was set */
    return old_fn != _RTC_DefaultErrorFuncW ? old_fn : 0;
}

