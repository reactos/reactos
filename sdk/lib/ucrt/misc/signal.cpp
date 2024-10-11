//
// signal.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// signal(), raise(), and related functions
//
#include <corecrt_internal.h>
#include <errno.h>
#include <excpt.h>
#include <float.h>
#include <malloc.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>



// Variables holding action codes (and code pointers) for SIGINT, SIGBRK,
// SIGABRT and SIGTERM.
//
// note that the disposition (i.e., action to be taken upon receipt) of
// these signals is defined on a per-process basis (not per-thread)!!
static __crt_state_management::dual_state_global<__crt_signal_handler_t> ctrlc_action;     // SIGINT
static __crt_state_management::dual_state_global<__crt_signal_handler_t> ctrlbreak_action; // SIGBREAK
static __crt_state_management::dual_state_global<__crt_signal_handler_t> abort_action;     // SIGABRT
static __crt_state_management::dual_state_global<__crt_signal_handler_t> term_action;      // SIGTERM

/*
 * flag indicated whether or not a handler has been installed to capture
 * ^C and ^Break events.
 */
static bool console_ctrl_handler_installed = false;

#define _SIGHUP_IGNORE 1
#define _SIGQUIT_IGNORE 3
#define _SIGPIPE_IGNORE 13
#define _SIGIOINT_IGNORE 16
#define _SIGSTOP_IGNORE 17



// Initializes the signal handler pointers to the encoded nullptr value.
extern "C" void __cdecl __acrt_initialize_signal_handlers(void* const encoded_nullptr)
{
    // The encoded nullptr is SIG_DFL
    ctrlc_action.initialize    (reinterpret_cast<__crt_signal_handler_t>(encoded_nullptr));
    ctrlbreak_action.initialize(reinterpret_cast<__crt_signal_handler_t>(encoded_nullptr));
    abort_action.initialize    (reinterpret_cast<__crt_signal_handler_t>(encoded_nullptr));
    term_action.initialize     (reinterpret_cast<__crt_signal_handler_t>(encoded_nullptr));
}



// Gets the address of the global action for one of the signals that has a
// global action.  Returns nullptr if the given signal does not have a global
// action.
static __crt_signal_handler_t* __cdecl get_global_action_nolock(int const signum) throw()
{
    switch (signum)
    {
    // CRT_REFACTOR TODO: PERFORMANCE: for OS mode, these might be able to be in the const data section, instead of in writeable page data.
    case SIGINT:         return &ctrlc_action.value();
    case SIGBREAK:       return &ctrlbreak_action.value();
    case SIGABRT:        return &abort_action.value();
    case SIGABRT_COMPAT: return &abort_action.value();
    case SIGTERM:        return &term_action.value();
    }

    return nullptr;
}



// Looks up the exception-action table entry for a signal.  This function finds
// the first entry in the 'action_table' whose _signal_number field is 'signum'.
// If no such entry is found, nullptr is returned.
static __crt_signal_action_t* __cdecl siglookup(
    int                    const signum,
    __crt_signal_action_t* const action_table
    ) throw()
{
    // Walk through the table looking for the proper entry.  Note that in the
    // case where more than one exception corresponds to the same signal, the
    // first such instance in the table is the one returned.
    for (__crt_signal_action_t* p = action_table; p != action_table + __acrt_signal_action_table_count; ++p)
        if (p->_signal_number == signum)
            return p;

    // If we reached the end of the table, return nullptr:
    return nullptr;
}



// Handles failures for signal().
static __crt_signal_handler_t __cdecl signal_failed(int const signum) throw()
{
    switch (signum)
    {
    case _SIGHUP_IGNORE:
    case _SIGQUIT_IGNORE:
    case _SIGPIPE_IGNORE:
    case _SIGIOINT_IGNORE:
    case _SIGSTOP_IGNORE:
        return SIG_ERR;

    default:
        errno = EINVAL;
        return SIG_ERR;
    }
}

// Enclaves have no console, thus also no signals specific to consoles.
#ifdef _UCRT_ENCLAVE_BUILD

__inline static BOOL is_unsupported_signal(int const signum, __crt_signal_handler_t const sigact)
{
    return (sigact == SIG_ACK || sigact == SIG_SGE || signum == SIGINT || signum == SIGBREAK);
}

__inline static BOOL is_console_signal(int const)
{
    return FALSE;
}

#else /* ^^^ _UCRT_ENCLAVE_BUILD ^^^ // vvv !_UCRT_ENCLAVE_BUILD vvv */

__inline static BOOL is_unsupported_signal(int const, __crt_signal_handler_t const sigact)
{
    return (sigact == SIG_ACK || sigact == SIG_SGE);
}

__inline static BOOL is_console_signal(int const signum)
{
    return (signum == SIGINT || signum == SIGBREAK);
}

#endif /* _UCRT_ENCLAVE_BUILD */

/***
*static BOOL WINAPI ctrlevent_capture(DWORD ctrl_type) - capture ^C and ^Break events
*
*Purpose:
*       Capture ^C and ^Break events from the console and dispose of them
*       according the values in ctrlc_action and ctrlbreak_action, resp.
*       This is the routine that evokes the user-defined action for SIGINT
*       (^C) or SIGBREAK (^Break) installed by a call to signal().
*
*Entry:
*       DWORD ctrl_type  - indicates type of event, two values:
*                               CTRL_C_EVENT
*                               CTRL_BREAK_EVENT
*
*Exit:
*       Returns TRUE to indicate the event (signal) has been handled.
*       Otherwise, returns FALSE.
*
*Exceptions:
*
*******************************************************************************/

static BOOL WINAPI ctrlevent_capture(DWORD const ctrl_type) throw()
{
    __crt_signal_handler_t ctrl_action = nullptr;
    int signal_code = 0;

    __acrt_lock(__acrt_signal_lock);
    __try
    {
        __crt_signal_handler_t* pctrl_action;

        // Identify the type of event and fetch the corresponding action
        // description:
        if (ctrl_type == CTRL_C_EVENT)
        {
            pctrl_action = &ctrlc_action.value();
            ctrl_action = __crt_fast_decode_pointer(*pctrl_action);
            signal_code = SIGINT;
        }
        else
        {
            pctrl_action = &ctrlbreak_action.value();
            ctrl_action = __crt_fast_decode_pointer(*pctrl_action);
            signal_code = SIGBREAK;
        }

        if (ctrl_action != SIG_DFL && ctrl_action != SIG_IGN)
        {
            // Reset the action to be SIG_DFL:
            *pctrl_action = __crt_fast_encode_pointer(nullptr);
        }
    }
    __finally
    {
        __acrt_unlock(__acrt_signal_lock);
    }
    __endtry

    // The default signal action leaves the event unhandled, so return false to
    // indicate such:
    if (ctrl_action == SIG_DFL)
        return FALSE;

    // If the action is not to ignore the signal, then invoke the action:
    if (ctrl_action != SIG_IGN)
        (*ctrl_action)(signal_code);

    // Then return TRUE to indicate the event has been handled (this may mean
    // that the even is being ignored):
    return TRUE;
}


/***
*__crt_signal_handler_t signal(signum, sigact) - Define a signal handler
*
*Purpose:
*       The signal routine allows the user to define what action should
*       be taken when various signals occur. The Win32/Dosx32 implementation
*       supports seven signals, divided up into three general groups
*
*       1. Signals corresponding to OS exceptions. These are:
*                       SIGFPE
*                       SIGILL
*                       SIGSEGV
*          Signal actions for these signals are installed by altering the
*          _action and SigAction fields for the appropriate entry in the
*          exception-action table (XcptActTab[]).
*
*       2. Signals corresponding to ^C and ^Break. These are:
*                       SIGINT
*                       SIGBREAK
*          Signal actions for these signals are installed by altering the
*          _ctrlc_action and _ctrlbreak_action variables.
*
*       3. Signals which are implemented only in the runtime. That is, they
*          occur only as the result of a call to raise().
*                       SIGABRT
*                       SIGTERM
*
*
*Entry:
*       int signum      signal type. recognized signal types are:
*
*                       SIGABRT         (ANSI)
*                       SIGBREAK
*                       SIGFPE          (ANSI)
*                       SIGILL          (ANSI)
*                       SIGINT          (ANSI)
*                       SIGSEGV         (ANSI)
*                       SIGTERM         (ANSI)
*                       SIGABRT_COMPAT
*
*       __crt_signal_handler_t sigact  signal handling function or action code. the action
*                       codes are:
*
*                       SIG_DFL - take the default action, whatever that may
*                       be, upon receipt of this type type of signal.
*
*                       SIG_DIE - *** ILLEGAL ***
*                       special code used in the _action field of an
*                       XcptActTab[] entry to indicate that the runtime is
*                       to terminate the process upon receipt of the exception.
*                       not accepted as a value for sigact.
*
*                       SIG_IGN - ignore this type of signal
*
*                       [function address] - transfer control to this address
*                       when a signal of this type occurs.
*
*Exit:
*       Good return:
*       Signal returns the previous value of the signal handling function
*       (e.g., SIG_DFL, SIG_IGN, etc., or [function address]). This value is
*       returned in DX:AX.
*
*       Error return:
*       Signal returns -1 and errno is set to EINVAL. The error return is
*       generally taken if the user submits bogus input values.
*
*Exceptions:
*       None.
*
*******************************************************************************/

extern "C" __crt_signal_handler_t __cdecl signal(int signum, __crt_signal_handler_t sigact)
{
    // Check for signal actions that are supported on other platforms but not on
    // this one, and make sure the action is not SIG_DIE:
    if (is_unsupported_signal(signum, sigact))
        return signal_failed(signum);

    // First, handle the case where the signal does not correspond to an
    // exception in the host OS:
    if (signum == SIGINT         ||
        signum == SIGBREAK       ||
        signum == SIGABRT        ||
        signum == SIGABRT_COMPAT ||
        signum == SIGTERM)
    {
        bool set_console_ctrl_error = false;
        __crt_signal_handler_t old_action = nullptr;

        __acrt_lock(__acrt_signal_lock);
        __try
        {
            // If the signal is SIGINT or SIGBREAK make sure the handler is
            // installed to capture ^C and ^Break events:
            // C4127: conditional expression is constant
#pragma warning( suppress: 4127 )
            if (is_console_signal(signum) && !console_ctrl_handler_installed)
            {
                if (SetConsoleCtrlHandler(ctrlevent_capture, TRUE))
                {
                    console_ctrl_handler_installed = true;
                }
                else
                {
                    _doserrno = GetLastError();
                    set_console_ctrl_error = true;
                }
            }

            __crt_signal_handler_t* const action_pointer = get_global_action_nolock(signum);
            if (action_pointer != nullptr)
            {
                old_action = __crt_fast_decode_pointer(*action_pointer);
                if (sigact != SIG_GET)
                    *action_pointer = __crt_fast_encode_pointer(sigact);
            }
        }
        __finally
        {
            __acrt_unlock(__acrt_signal_lock);
        }
        __endtry

        if (set_console_ctrl_error)
            return signal_failed(signum);

        return old_action;
    }


    // If we reach here, signum is supposed to be one of the signals which
    // correspond to exceptions on the host OS.  If it's not one of these,
    // fail and return immediately:
    if (signum != SIGFPE && signum != SIGILL && signum != SIGSEGV)
        return signal_failed(signum);

    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (ptd == nullptr)
        return signal_failed(signum);

    // Check that there is a per-thread instance of the exception-action table
    // for this thread.  If there isn't, create one:
    if (ptd->_pxcptacttab == __acrt_exception_action_table)
    {
        // Allocate space for an exception-action table:
        ptd->_pxcptacttab = static_cast<__crt_signal_action_t*>(_malloc_crt(__acrt_signal_action_table_size));
        if (ptd->_pxcptacttab == nullptr)
            return signal_failed(signum);

        // Initialize the table by copying the contents of __acrt_exception_action_table:
        memcpy(ptd->_pxcptacttab, __acrt_exception_action_table, __acrt_signal_action_table_size);
    }

    // Look up the proper entry in the exception-action table. Note that if
    // several exceptions are mapped to the same signal, this returns the
    // pointer to first such entry in the exception action table. It is assumed
    // that the other entries immediately follow this one.
    __crt_signal_action_t* const xcpt_action = siglookup(signum, ptd->_pxcptacttab);
    if (xcpt_action == nullptr)
        return signal_failed(signum);

    // SIGSEGV, SIGILL and SIGFPE all have more than one exception mapped to
    // them.  The code below depends on the exceptions corresponding to the same
    // signal being grouped together in the exception-action table.

    __crt_signal_handler_t const old_action = xcpt_action->_action;

    // If we are not just getting the currently installed action, loop through
    // all the entries corresponding to the given signal and update them as
    // appropriate:
    if (sigact != SIG_GET)
    {
        __crt_signal_action_t* const last = ptd->_pxcptacttab + __acrt_signal_action_table_count;

        // Iterate until we reach the end of the table or we reach the end of
        // the range of actions for this signal, whichever comes first:
        for (__crt_signal_action_t* p = xcpt_action; p != last && p->_signal_number == signum; ++p)
        {
            p->_action = sigact;
        }
    }

    return old_action;
}



/***
*int raise(signum) - Raise a signal
*
*Purpose:
*       This routine raises a signal (i.e., performs the action currently
*       defined for this signal). The action associated with the signal is
*       evoked directly without going through intermediate dispatching or
*       handling.
*
*Entry:
*       int signum - signal type (e.g., SIGINT)
*
*Exit:
*       returns 0 on good return, -1 on bad return.
*
*Exceptions:
*       May not return.  Raise has no control over the action
*       routines defined for the various signals.  Those routines may
*       abort, terminate, etc.  In particular, the default actions for
*       certain signals will terminate the program.
*
*******************************************************************************/
extern "C" int __cdecl raise(int const signum)
{
    __acrt_ptd* ptd = nullptr;
    int old_fpecode = 0;

    __crt_signal_handler_t* action_pointer = nullptr;
    bool action_is_global = true;
    switch (signum)
    {
    case SIGINT:
    case SIGBREAK:
    case SIGABRT:
    case SIGABRT_COMPAT:
    case SIGTERM:
        action_pointer = get_global_action_nolock(signum);;
        break;

    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    {
        ptd = __acrt_getptd_noexit();
        if (ptd == nullptr)
            return -1;

        __crt_signal_action_t* const local_action = siglookup(signum, ptd->_pxcptacttab);
        _VALIDATE_RETURN(local_action != nullptr, EINVAL, -1);
        action_pointer = &local_action->_action;
        action_is_global = false;
        break;
    }
    default:
        // unsupported signal, return an error
        _VALIDATE_RETURN(("Invalid signal or error", 0), EINVAL, -1);
    }


    PEXCEPTION_POINTERS old_pxcptinfoptrs = nullptr;

    // If the action is global, we must acquire the lock before accessing it:
    if (action_is_global)
        __acrt_lock(__acrt_signal_lock);

    __crt_signal_handler_t action = nullptr;
    bool return0 = false;
    __try
    {
        // Global function pointers are encoded; per-thread pointers are not:
        action = action_is_global ? __crt_fast_decode_pointer(*action_pointer) : *action_pointer;

        // If the current action is SIG_IGN, just return:
        return0 = action == SIG_IGN;
        if (return0)
            __leave;

        // If the current action is SIG_DFL, take the default action.  The current
        // default action for all of the supported signals is to terminate with an
        // exit code of 3:
        if (action == SIG_DFL)
        {
            // Be sure to unlock before entering the exit code.  The exit function
            // does not return.
            if (action_is_global)
                __acrt_unlock(__acrt_signal_lock);

            _exit(3);
        }

        // For signals that correspond to exceptions, set the pointer to the
        // EXCEPTION_POINTERS structure to nullptr:
        if (signum == SIGFPE  || signum == SIGSEGV || signum == SIGILL)
        {
            old_pxcptinfoptrs = ptd->_tpxcptinfoptrs;
            ptd->_tpxcptinfoptrs = nullptr;

            // If the signal is SIGFPE, also set _fpecode to _FPE_EXPLICITGEN:
            if ( signum == SIGFPE )
            {
                old_fpecode = _fpecode;
                _fpecode = _FPE_EXPLICITGEN;
            }
        }

        // Reset the action to SIG_DFL before calling the user-specified handler.
        // For SIGFPE, we must reset the action for all of the FP exceptions:
        if (signum == SIGFPE)
        {
            __crt_signal_action_t* const first = ptd->_pxcptacttab + __acrt_signal_action_first_fpe_index;
            __crt_signal_action_t* const last = first + __acrt_signal_action_fpe_count;

            for (__crt_signal_action_t* p = first; p != last; ++p)
            {
                p->_action = SIG_DFL;
            }
        }
        else
        {
            *action_pointer = __crt_fast_encode_pointer(nullptr);
        }
    }
    __finally
    {
        if (action_is_global)
            __acrt_unlock(__acrt_signal_lock);
    }
    __endtry

    if (return0)
        return 0;

    // Call the user-specified handler routine.  For SIGFPE, we have special
    // code to support old handlers which expect the value of _fpecode as the
    // second argument:
    if (signum == SIGFPE)
    {
        reinterpret_cast<void(__cdecl*)(int,int)>(action)(SIGFPE, _fpecode);
    }
    else
    {
        action(signum);
    }


    // For signals that correspond to exceptions, restore the pointer to the
    // EXCEPTION_POINTERS structure:
    if (signum == SIGFPE  ||  signum == SIGSEGV || signum == SIGILL)
    {
        ptd->_tpxcptinfoptrs = old_pxcptinfoptrs;

        // If signum is SIGFPE, also restore _fpecode
        if (signum == SIGFPE)
            _fpecode = old_fpecode;
    }

    return 0;
}


// Gets the SIGABRT signal handling function
extern "C" __crt_signal_handler_t __cdecl __acrt_get_sigabrt_handler()
{
    return __acrt_lock_and_call(__acrt_signal_lock, []
    {
        return __crt_fast_decode_pointer(abort_action.value());
    });
}

// Gets the FPE code for the current thread
extern "C" int* __cdecl __fpecode()
{
    return &__acrt_getptd()->_tfpecode;
}

// Returns a pointer to the signal handlers for the current thread
extern "C" void** __cdecl __pxcptinfoptrs()
{
    return reinterpret_cast<void**>(&__acrt_getptd()->_tpxcptinfoptrs);
}
