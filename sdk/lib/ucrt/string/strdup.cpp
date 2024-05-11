//
// strdup.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _strdup() and _strdup_dbg(), which dynamically allocate a buffer and
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

extern "C" char* __cdecl _strdup(char const* const string)
{
    return _strdup_dbg(string, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" char* __cdecl _strdup_dbg(
    char const* const string,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )

#else // ^^^ _DEBUG ^^^ // vvv !_DEBUG vvv //

extern "C" char* __cdecl _strdup(
    char const* string
    )

#endif // !_DEBUG
{
    if (string == nullptr)
        return nullptr;

    size_t const size = strlen(string) + 1;

#ifdef _DEBUG
    char* const memory = static_cast<char*>(_malloc_dbg(
        size,
        block_use,
        file_name,
        line_number));
#else
    char* const memory = static_cast<char*>(malloc(size));
#endif
    
    if (memory == nullptr)
        return nullptr;

    _ERRCHECK(strcpy_s(memory, size, string));
    return memory;
}
