#if 0
#include <precomp.h>
#include <errno.h>

static _invalid_parameter_handler invalid_parameter_handler = NULL;

void
_invalid_parameter_default(
    const wchar_t * expr,
    const wchar_t * func, 
    const wchar_t * file, 
    unsigned int line,
    uintptr_t pReserved)
{
    if (invalid_parameter_handler) invalid_parameter_handler( expr, func, file, line, pReserved );
    else
    {
        ERR( "%s:%u %s: %s %lx\n", debugstr_w(file), line, debugstr_w(func), debugstr_w(expr), pReserved );
        RaiseException( STATUS_INVALID_CRUNTIME_PARAMETER, EXCEPTION_NONCONTINUABLE, 0, NULL );
    }
}


// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=98835
void
_invalid_parameter(
    const wchar_t * expression,
    const wchar_t * function, 
    const wchar_t * file, 
    unsigned int line,
    uintptr_t pReserved)
{
    _invalid_parameter_handler handler;

    if (invalid_parameter_handler)
    {
        handler = DecodePointer(invalid_parameter_handler);
    }
    else
    {
        handler = _invalid_parameter_default;
    }
    handler(expression, function, file, line, pReserved);
}

// http://cowboyprogramming.com/2007/02/22/what-is-_invalid_parameter_noinfo-and-how-do-i-get-rid-of-it/
void
invalid_parameter_noinfo(void)
{
    _invalid_parameter(0, 0, 0, 0, 0);
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
#endif
