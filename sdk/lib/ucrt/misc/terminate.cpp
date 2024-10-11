//
// terminate.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The terminate handler
//
#include <corecrt_internal.h>
#include <corecrt_terminate.h>



static terminate_handler __cdecl get_terminate_or_default(
    __acrt_ptd const* const ptd
    ) throw()
{
    return ptd->_terminate ? ptd->_terminate : &abort;
}

extern "C" terminate_handler __cdecl _get_terminate()
{
    return get_terminate_or_default(__acrt_getptd());
}

extern "C" terminate_handler __cdecl set_terminate(
    terminate_handler const new_handler
    ) throw()
{
    __acrt_ptd* const ptd = __acrt_getptd();

    terminate_handler const old_handler = get_terminate_or_default(ptd);

    ptd->_terminate = new_handler;

    return old_handler;
}

extern "C" void __cdecl terminate() throw()
{
    terminate_handler const handler = __acrt_getptd()->_terminate;
    if (handler)
    {
        // Note:  We cannot allow any exceptions to propagate from a user-
        // registered terminate handler, so if any structured exception escapes
        // the user handler we abort.
        __try
        {
            handler();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ; // Deliberately do nothing
        }
        __endtry
    }

    // If the terminate handler returned, faulted, or otherwise failed to end
    // execution, we will do it:
    abort();
}
