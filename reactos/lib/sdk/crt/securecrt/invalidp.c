#include <precomp.h>

void * __pInvalidArgHandler = NULL;

void
_invalid_parameter_default(
    const wchar_t * expression,
    const wchar_t * function, 
    const wchar_t * file, 
    unsigned int line,
    uintptr_t pReserved)
{
    // TODO: launch Doc Watson or something like that
    UNIMPLEMENTED;
    ExitProcess(-1);
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

    if (__pInvalidArgHandler)
    {
        handler = DecodePointer(__pInvalidArgHandler);
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

