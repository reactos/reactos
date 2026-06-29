//
// wcsdup.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _wcsdup() and _wcsdup_dbg(), which dynamically allocate a buffer and
// duplicate a string into it.
//
// These functions allocate storage via malloc() or _malloc_dbg().  The caller
// is responsible for free()ing the returned array.  If the input string is null
// or if sufficient memory could not be allocated, these functions return null.
//
#include <corecrt_internal.h>
#include <malloc.h>
#include <string.h>

#ifdef _DEBUG

extern "C" wchar_t* __cdecl _wcsdup(wchar_t const* const string)
{
    return _wcsdup_dbg(string, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" wchar_t* __cdecl _wcsdup_dbg(
    wchar_t const* const string,
    int            const block_use,
    char const*    const file_name,
    int            const line_number
    )

#else // ^^^ _DEBUG ^^^ // vvv !_DEBUG vvv //

extern "C" wchar_t* __cdecl _wcsdup(
    wchar_t const* string
    )

#endif // !_DEBUG
{
    if (string == nullptr)
        return nullptr;

    size_t const size_in_elements = wcslen(string) + 1;

#ifdef _DEBUG
    wchar_t* const memory = static_cast<wchar_t*>(_malloc_dbg(
        size_in_elements * sizeof(wchar_t),
        block_use,
        file_name,
        line_number));
#else
    wchar_t* const memory = static_cast<wchar_t*>(malloc(
        size_in_elements * sizeof(wchar_t)));
#endif

    if (memory == nullptr)
        return nullptr;

    _ERRCHECK(wcscpy_s(memory, size_in_elements, string));
    return memory;
}
