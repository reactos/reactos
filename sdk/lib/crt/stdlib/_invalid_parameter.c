/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     _invalid_parameter implementation
 * COPYRIGHT:   Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <precomp.h>

#ifdef _MSVCRTEX_
#undef TRACE
#undef ERR
#define TRACE(...)
#define ERR(...)
#endif

static _invalid_parameter_handler invalid_parameter_handler = NULL;

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
#if _MSVCR_VER > 0 // FIXME: possible improvement: use a global variable in the DLL
        RaiseException( STATUS_INVALID_CRUNTIME_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, NULL );
#endif
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
