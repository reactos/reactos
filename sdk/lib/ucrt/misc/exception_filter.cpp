//
// exception_filter.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The exception filter used by the startup code to transform various exceptions
// into C signals.
//
#include <corecrt_internal.h>
#include <float.h>
#include <signal.h>
#include <stddef.h>



// The default exception action table.  Exceptions corresponding to the same
// signal (e.g. SIGFPE) must be grouped together.  If any _action field is
// changed in this table, that field must be initialized with an encoded
// function pointer during CRT startup.
extern "C" extern struct __crt_signal_action_t const __acrt_exception_action_table[] =
{
//    _exception_number                          _signal_number   _action
//  --------------------------------------------------------
    { STATUS_ACCESS_VIOLATION,         SIGSEGV, SIG_DFL },
    { STATUS_ILLEGAL_INSTRUCTION,      SIGILL,  SIG_DFL },
    { STATUS_PRIVILEGED_INSTRUCTION,   SIGILL,  SIG_DFL },
    { STATUS_FLOAT_DENORMAL_OPERAND,   SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_DIVIDE_BY_ZERO,     SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_INEXACT_RESULT,     SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_INVALID_OPERATION,  SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_OVERFLOW,           SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_STACK_CHECK,        SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_UNDERFLOW,          SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_MULTIPLE_FAULTS,    SIGFPE,  SIG_DFL },
    { STATUS_FLOAT_MULTIPLE_TRAPS,     SIGFPE,  SIG_DFL },
};

// WARNING!!! This constant must be the integer value for which
// __acrt_exception_action_table[__acrt_signal_action_first_fpe_index] is the very FIRST entry in the table
// corresponding to a floating point exception.
extern "C" extern size_t const __acrt_signal_action_first_fpe_index = 3;

// There are __acrt_signal_action_fpe_count entries in XcptActTab for floating point exceptions:
extern "C" extern size_t const __acrt_signal_action_fpe_count = 9;

// Size of the exception-action table in bytes
extern "C" extern size_t const __acrt_signal_action_table_size = sizeof(__acrt_exception_action_table);

// Number of entries in the exception-action table
extern "C" extern size_t const __acrt_signal_action_table_count = sizeof(__acrt_exception_action_table) / sizeof(__acrt_exception_action_table[0]);



// Finds an exception-action entry by exception number
static __crt_signal_action_t* __cdecl xcptlookup(
    unsigned long const xcptnum,
    __crt_signal_action_t* const action_table
    ) throw()
{
    __crt_signal_action_t* const first = action_table;
    __crt_signal_action_t* const last  = first + __acrt_signal_action_table_count;

    // Search for the matching entry and return it if we find it:
    for (__crt_signal_action_t* it = first; it != last; ++it)
        if (it->_exception_number == xcptnum)
            return it;

    // Otherwise, return nullptr on failure:
    return nullptr;
}



// Wrapper that only calls the exception filter for C++ exceptions
extern "C" int __cdecl _seh_filter_dll(
    unsigned long       const xcptnum,
    PEXCEPTION_POINTERS const pxcptinfoptrs
    )
{
    if (xcptnum != ('msc' | 0xE0000000))
        return EXCEPTION_CONTINUE_SEARCH;

    return _seh_filter_exe(xcptnum,pxcptinfoptrs);
}



// Identifies an exception and the action to be taken with it
//
// This function is called by the exception filter expression of the __try-
// __except statement in the startup code, which guards the call to the user-
// provided main (or DllMain) function.  This function consults the per-thread
// exception-action table to identify the exception and determine its
// disposition.  The disposition of an exception corresponding to a C signal may
// be modified by a call to signal().  There are three broad cases:
//
// (1) Unrecognized exceptions and exceptions for which the action is SIG_DFL
//
//     In both of these cases, EXCEPTION_CONTINUE_SEARCH is returned to cause
//     the OS exception dispatcher to pass the exception on to the next exception
//     handler in the chain--usually a system default handler.
//
// (2) Exceptions corresponding to C signals with an action other than SIG_DFL
//
//     These are the C signals whose disposition has been affected by a call to
//     signal() or whose default semantics differ slightly from the corresponding
//     OS exception.  In all cases, the appropriate disposition of the C signal is
//     made by the function (e.g., calling a user-specified signal handler).
//     Then, EXCEPTION_CONTINUE_EXECUTION is returned to cause the OS exception
//     dispatcher to dismiss the exception and resume execution at the point
//     where the exception occurred.
//
// (3) Exception for which the action is SIG_DIE
//
//     These are the exceptions corresponding to fatal C Runtime errors. For
//     these, EXCEPTION_EXECUTE_HANDLER is returned to cause control to pass
//     into the __except block of the __try-__except statement.  There, the
//     runtime error is identified, an appropriate error message is printed out,
//     and the program is terminated.
extern "C" int __cdecl _seh_filter_exe(
    unsigned long       const xcptnum,
    PEXCEPTION_POINTERS const pxcptinfoptrs
    )
{
    // Get the PTD and find the action for this exception.  If either of these
    // fails, return to let the handler search continue.
    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (ptd == nullptr)
        return EXCEPTION_CONTINUE_SEARCH;

    __crt_signal_action_t* const pxcptact = xcptlookup(xcptnum, ptd->_pxcptacttab);
    if (pxcptact == nullptr)
        return EXCEPTION_CONTINUE_SEARCH;

    __crt_signal_handler_t const phandler = pxcptact->_action;

    // The default behavior (SIG_DFL) is not to handle the exception:
    if (phandler == SIG_DFL)
        return EXCEPTION_CONTINUE_SEARCH;

    // If the behavior is to die, execute the handler:
    if (phandler == SIG_DIE)
    {
        // Reset the _action (in case of recursion) and enter the __except:
        pxcptact->_action = SIG_DFL;
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // If an exception is ignored, just ignore it:
    if (phandler == SIG_IGN)
        return EXCEPTION_CONTINUE_EXECUTION;

    // The remaining exceptions all correspond to C signals which have signal
    // handlers associated with them.  For some, special setup is required
    // before the signal handler is called.  In all cases, if the signal handler
    // returns, -1 is returned by this function to resume execution at the point
    // where the exception occurred.

    // Save the old exception pointers in case this is a nested exception/signal.
    PEXCEPTION_POINTERS const old_pxcptinfoptrs = ptd->_tpxcptinfoptrs;
    ptd->_tpxcptinfoptrs = pxcptinfoptrs;

    // Call the user-supplied signal handler.  Floating point exceptions must be
    // handled specially since, from the C point of view, there is only one
    // signal.  The exact identity of the exception is passed in the per-thread
    // variable _tfpecode.
    if (pxcptact->_signal_number == SIGFPE)
    {
        // Reset the _action field to the default for all SIGFPE entries:
        __crt_signal_action_t* const first = ptd->_pxcptacttab + __acrt_signal_action_first_fpe_index;
        __crt_signal_action_t* const last  = first + __acrt_signal_action_fpe_count;

        for (__crt_signal_action_t* it = first; it != last; ++it)
            it->_action = SIG_DFL;

        // Save the current _tfpecode in case this is a nested floating point
        // exception.  It's not clear that we need to support this, but it's
        // easy to support.
        int const old_fpecode = ptd->_tfpecode;

        // Note:  There are no exceptions corresponding to _FPE_UNEMULATED and
        // _FPE_SQRTNEG.  Furthermore, STATUS_FLOATING_STACK_CHECK is raised for
        // both stack overflow and underflow, thus the exception does not
        // distinguish between _FPE_STACKOVERFLOW and _FPE_STACKUNDERFLOW.
        // Arbitrarily, _tfpecode is set to the formr value.
        switch (pxcptact->_exception_number)
        {
        case STATUS_FLOAT_DIVIDE_BY_ZERO:    ptd->_tfpecode = _FPE_ZERODIVIDE;      break;
        case STATUS_FLOAT_INVALID_OPERATION: ptd->_tfpecode = _FPE_INVALID;         break;
        case STATUS_FLOAT_OVERFLOW:          ptd->_tfpecode = _FPE_OVERFLOW;        break;
        case STATUS_FLOAT_UNDERFLOW:         ptd->_tfpecode = _FPE_UNDERFLOW;       break;
        case STATUS_FLOAT_DENORMAL_OPERAND:  ptd->_tfpecode = _FPE_DENORMAL;        break;
        case STATUS_FLOAT_INEXACT_RESULT:    ptd->_tfpecode = _FPE_INEXACT;         break;
        case STATUS_FLOAT_STACK_CHECK:       ptd->_tfpecode = _FPE_STACKOVERFLOW;   break;
        case STATUS_FLOAT_MULTIPLE_TRAPS:    ptd->_tfpecode = _FPE_MULTIPLE_TRAPS;  break;
        case STATUS_FLOAT_MULTIPLE_FAULTS:   ptd->_tfpecode = _FPE_MULTIPLE_FAULTS; break;
        }

        // Call the SIGFPE handler.  Note that we cast to the given function
        // type to support old MS C programs whose SIGFPE handlers expect two
        // arguments.
        //
        // CRT_REFACTOR TODO Can we remove this support?  It's not clear that
        // this is correct for all architectures (e.g. x64 and ARM).
        reinterpret_cast<void (__cdecl *)(int, int)>(phandler)(SIGFPE, ptd->_tfpecode);

        // Restore the stored value of _tfpecode:
        ptd->_tfpecode = old_fpecode;
    }
    else
    {
        // Reset the _action field to the default, then call the user-
        // supplied handler:
        pxcptact->_action = SIG_DFL;
        phandler(pxcptact->_signal_number);
    }

    // Restore the stored value of _pxcptinfoptrs:
    ptd->_tpxcptinfoptrs = old_pxcptinfoptrs;

    return EXCEPTION_CONTINUE_EXECUTION; 
}
