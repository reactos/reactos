//
// new_handler.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Implementation of the new handler and related functions..
//
#include <corecrt_internal.h>
#include <new.h>

// The global new handler.  This pointer should only be accessed via the
// functions defined in this source file.
static __crt_state_management::dual_state_global<_PNH> __acrt_new_handler;



// Initializes the new handler to the encoded nullptr value.
extern "C" void __cdecl __acrt_initialize_new_handler(void* const encoded_null)
{
    __acrt_new_handler.initialize(reinterpret_cast<_PNH>(encoded_null));
}

// Sets the new handler to the new new handler and returns the old handler.
_PNH __cdecl _set_new_handler(_PNH new_handler)
{
    _PNH result = nullptr;

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        result = __crt_fast_decode_pointer(__acrt_new_handler.value());
        __acrt_new_handler.value() = __crt_fast_encode_pointer(new_handler);
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return result;
}

// Sets the new handler to nullptr and returns the old handler.  The argument
// must be zero.  This overload is used to enable a caller to pass nullptr.
_PNH __cdecl _set_new_handler(int const new_handler)
{
    // This is referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(new_handler);

    _ASSERTE(new_handler == 0);

    return _set_new_handler(__crt_fast_encode_pointer(nullptr));
}

// Gets the current new handler.
_PNH __cdecl _query_new_handler()
{
    _PNH result = nullptr;

    __acrt_lock(__acrt_heap_lock);
    __try
    {
        result = __crt_fast_decode_pointer(__acrt_new_handler.value());
    }
    __finally
    {
        __acrt_unlock(__acrt_heap_lock);
    }
    __endtry

    return result;
}

// Calls the currently registered new handler with the provided size.  If the
// new handler succeeds, this function returns 1.  This function returns 0 on
// failure.  The new handler may throw std::bad_alloc.
extern "C" int __cdecl _callnewh(size_t const size)
{
    _PNH const pnh = _query_new_handler();

    if (pnh == nullptr || (*pnh)(size) == 0)
        return 0;

    return 1;
}
