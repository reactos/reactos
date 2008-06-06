#include <precomp.h>
#include "internal/wine/msvcrt.h"
#include "internal/wine/cppexcept.h"

typedef void (*sighandler_t)(int);
static sighandler_t sighandlers[NSIG] = { SIG_DFL };

/* The exception codes are actually NTSTATUS values */
static const struct
{
    NTSTATUS status;
    int signal;
} float_exception_map[] = {
 { EXCEPTION_FLT_DENORMAL_OPERAND, _FPE_DENORMAL },
 { EXCEPTION_FLT_DIVIDE_BY_ZERO, _FPE_ZERODIVIDE },
 { EXCEPTION_FLT_INEXACT_RESULT, _FPE_INEXACT },
 { EXCEPTION_FLT_INVALID_OPERATION, _FPE_INVALID },
 { EXCEPTION_FLT_OVERFLOW, _FPE_OVERFLOW },
 { EXCEPTION_FLT_STACK_CHECK, _FPE_STACKOVERFLOW },
 { EXCEPTION_FLT_UNDERFLOW, _FPE_UNDERFLOW },
};

/*
 * @implemented
 */
int
_XcptFilter(DWORD ExceptionCode,
            struct _EXCEPTION_POINTERS *  except)
{
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    sighandler_t handler;

    if (!except || !except->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    switch (except->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if ((handler = sighandlers[SIGSEGV]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                sighandlers[SIGSEGV] = SIG_DFL;
                handler(SIGSEGV);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    /* According to msdn,
     * the FPE signal handler takes as a second argument the type of
     * floating point exception.
     */
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        if ((handler = sighandlers[SIGFPE]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                int i, float_signal = _FPE_INVALID;

                sighandlers[SIGFPE] = SIG_DFL;
                for (i = 0; i < sizeof(float_exception_map) /
                         sizeof(float_exception_map[0]); i++)
                {
                    if (float_exception_map[i].status ==
                        except->ExceptionRecord->ExceptionCode)
                    {
                        float_signal = float_exception_map[i].signal;
                        break;
                    }
                }
                ((float_handler)handler)(SIGFPE, float_signal);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        if ((handler = sighandlers[SIGILL]) != SIG_DFL)
        {
            if (handler != SIG_IGN)
            {
                sighandlers[SIGILL] = SIG_DFL;
                handler(SIGILL);
            }
            ret = EXCEPTION_CONTINUE_EXECUTION;
        }
        break;
    }
    return ret;
}

int CDECL __CppXcptFilter(NTSTATUS ex, PEXCEPTION_POINTERS ptr)
{
     /* only filter c++ exceptions */
     if (ex != CXX_EXCEPTION) return EXCEPTION_CONTINUE_SEARCH;
     return _XcptFilter( ex, ptr );
}





