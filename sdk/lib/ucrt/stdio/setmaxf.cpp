//
// setmaxf.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _setmaxstdio() and _getmaxstdio(), which control the maximum number
// of stdio streams that may be open simultaneously.
//
#include <corecrt_internal_stdio.h>



// Sets the maximum number of stdio streams that may be simultaneously open.
// Note that the maximum must be at least _IOB_ENTRIES, and must be no larger
// than _N_HANDLE_.  However, it may be either larger or smaller than the current
// maximum.
//
// Returns the new maximum value on success; returns -1 on failure.
extern "C" int __cdecl _setmaxstdio(int const new_maximum)
{
    // Make sure the request is reasonable:
    _VALIDATE_RETURN(new_maximum >= _IOB_ENTRIES && new_maximum <= _NHANDLE_, EINVAL, -1);

    return __acrt_lock_and_call(__acrt_stdio_index_lock, [&]
    {
        // If the new maximum is the same as our current maximum, no work to do:
        if (new_maximum == _nstream)
            return new_maximum;

        // If the new maximum is smaller than the current maximum, attempt to
        // free up any entries that are beyond the new maximum:
        if (new_maximum < _nstream)
        {
            __crt_stdio_stream_data** const first_to_remove = __piob + new_maximum;
            __crt_stdio_stream_data** const last_to_remove  = __piob + _nstream;
            for (__crt_stdio_stream_data** rit = last_to_remove; rit != first_to_remove; --rit)
            {
                __crt_stdio_stream_data* const entry = *(rit - 1);
                if (entry == nullptr)
                    continue;

                // If the entry is still in use, stop freeing entries and return
                // failure to the caller:
                if (__crt_stdio_stream(entry).is_in_use())
                    return -1;

                _free_crt(entry);
            }
        }

        // Enlarge or shrink the array, as required:
        __crt_stdio_stream_data** const new_piob = _recalloc_crt_t(__crt_stdio_stream_data*, __piob, new_maximum).detach();
        if (new_piob == nullptr)
            return -1;

        _nstream = new_maximum;
        __piob = new_piob;
        return new_maximum;
    });
}



// Gets the maximum number of stdio streams that may be open at any one time.
extern "C" int __cdecl _getmaxstdio()
{
    return __acrt_lock_and_call(__acrt_stdio_index_lock, []
    {
        return _nstream;
    });
}
