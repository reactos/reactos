//
// onexit.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _onexit registry, which stores pointers to functions to be called when
// the program terminates or, when the CRT is statically linked into a DLL, when
// the DLL is unloaded.
//
// When the CRT is statically linked into an EXE or a DLL, this registry is used
// to hold the on-exit functions for the module (EXE or DLL) into which it is
// linked.
//
// When the dynamic CRT is used, this object is part of the AppCRT DLL and this
// registry stores the on-exit functions for the VCRuntime.  If the EXE for the
// process uses this VCRuntime, then this registry also stores the on-exit
// functions for that EXE.  If a DLL uses the dynamic CRT, then that DLL has its
// own registry, defined in the statically-linked VCStartup library.
//
#include <corecrt_internal.h>



// The global atexit and at_quick_exit registries
extern "C" { _onexit_table_t __acrt_atexit_table{}; }
extern "C" { _onexit_table_t __acrt_at_quick_exit_table{}; }



enum : size_t
{
    initial_table_count     = 32,
    minimum_table_increment = 4,
    maximum_table_increment = 512
};



// Registers a function to be executed on exit.  This function modifies the global
// onexit table.
extern "C" int __cdecl _crt_atexit(_PVFV const function)
{
    return _register_onexit_function(&__acrt_atexit_table, reinterpret_cast<_onexit_t>(function));
}

extern "C" int __cdecl _crt_at_quick_exit(_PVFV const function)
{
    return _register_onexit_function(&__acrt_at_quick_exit_table, reinterpret_cast<_onexit_t>(function));
}



extern "C" int __cdecl _initialize_onexit_table(_onexit_table_t* const table)
{
    if (!table)
    {
        return -1;
    }

    // If the table has already been initialized, do not do anything.  Note that
    // this handles both the case where the table was value initialized and where
    // the table was initialized with encoded null pointers.
    if (table->_first != table->_end)
    {
        return 0;
    }

    _PVFV* const encoded_nullptr = __crt_fast_encode_pointer(nullptr);

    table->_first = encoded_nullptr;
    table->_last  = encoded_nullptr;
    table->_end   = encoded_nullptr;

    return 0;
}



// Appends the given 'function' to the given onexit 'table'.  Returns 0 on
// success; returns -1 on failure.  In general, failures are considered fatal
// in calling code.
extern "C" int __cdecl _register_onexit_function(_onexit_table_t* const table, _onexit_t const function)
{
    return __acrt_lock_and_call(__acrt_select_exit_lock(), [&]
    {
        if (!table)
        {
            return -1;
        }

        _PVFV* first = __crt_fast_decode_pointer(table->_first);
        _PVFV* last  = __crt_fast_decode_pointer(table->_last);
        _PVFV* end   = __crt_fast_decode_pointer(table->_end);

        // If there is no room for the new entry, reallocate a larger table:
        if (last == end)
        {
            size_t const old_count = end - first;

            size_t const increment = old_count > maximum_table_increment ? maximum_table_increment : old_count;

            // First, try to double the capacity of the table:
            size_t new_count = old_count + increment;
            if (new_count == 0)
            {
                new_count = initial_table_count;
            }

            _PVFV* new_first = nullptr;
            if (new_count >= old_count)
            {
                new_first = _recalloc_crt_t(_PVFV, first, new_count).detach();
            }

            // If that didn't work, try to allocate a smaller increment:
            if (new_first == nullptr)
            {
                new_count = old_count + minimum_table_increment;
                new_first = _recalloc_crt_t(_PVFV, first, new_count).detach();
            }

            if (new_first == nullptr)
            {
                return -1;
            }

            first = new_first;
            last  = new_first + old_count;
            end   = new_first + new_count;

            // The "additional" storage obtained from recalloc is sero-initialized.
            // The array holds encoded function pointers, so we need to fill the
            // storage with encoded nullptrs:
            _PVFV const encoded_nullptr = __crt_fast_encode_pointer(nullptr);
            for (auto it = last; it != end; ++it)
            {
                *it = encoded_nullptr;
            }
        }

        *last++ = reinterpret_cast<_PVFV>(__crt_fast_encode_pointer(function));

        table->_first = __crt_fast_encode_pointer(first);
        table->_last  = __crt_fast_encode_pointer(last);
        table->_end   = __crt_fast_encode_pointer(end);

        return 0;
    });
}



// This function executes a table of _onexit()/atexit() functions.  The
// terminators are executed in reverse order, to give the required LIFO
// execution order.  If the table is uninitialized, this function has no
// effect.  After executing the terminators, this function resets the table
// so that it is uninitialized.  Returns 0 on success; -1 on failure.
extern "C" int __cdecl _execute_onexit_table(_onexit_table_t* const table)
{
    return __acrt_lock_and_call(__acrt_select_exit_lock(), [&]
    {
        if (!table)
        {
            return -1;
        }

        _PVFV* first = __crt_fast_decode_pointer(table->_first);
        _PVFV* last  = __crt_fast_decode_pointer(table->_last);
        if (!first || first == reinterpret_cast<_PVFV*>(-1))
        {
            return 0;
        }

        // This loop calls through caller-provided function pointers.  We must
        // save and reset the global state mode before calling them, to maintain
        // proper mode nesting.  (These calls to caller-provided function pointers
        // are the only non-trivial calls, so we can do this once for the entire
        // loop.)
        {
            __crt_state_management::scoped_global_state_reset saved_state;

            _PVFV const encoded_nullptr = __crt_fast_encode_pointer(nullptr);

            _PVFV* saved_first = first;
            _PVFV* saved_last  = last;
            for (;;)
            {
                // Find the last valid function pointer to call:
                while (--last >= first && *last == encoded_nullptr)
                {
                    // Keep going backwards
                }

                if (last < first)
                {
                    // There are no more valid entries in the list; we are done:
                    break;
                }

                // Store the function pointer and mark it as visited in the list:
                _PVFV const function = __crt_fast_decode_pointer(*last);
                *last = encoded_nullptr;

                function();

                _PVFV* const new_first = __crt_fast_decode_pointer(table->_first);
                _PVFV* const new_last  = __crt_fast_decode_pointer(table->_last);

                // Reset iteration if either the begin or end pointer has changed:
                if (new_first != saved_first || new_last != saved_last)
                {
                    first = saved_first = new_first;
                    last  = saved_last  = new_last;
                }
            }
        }

        if (first != reinterpret_cast<_PVFV*>(-1))
        {
            _free_crt(first);
        }

        _PVFV* const encoded_nullptr = __crt_fast_encode_pointer(nullptr);

        table->_first = encoded_nullptr;
        table->_last  = encoded_nullptr;
        table->_end   = encoded_nullptr;

        return 0;
    });
}
