//
// tempnam.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _tempnam(), which generates temporary file names.
//
#include <corecrt_internal_stdio.h>



// Strips quotes from a string.  If the source string contains no quotes, it is
// returned.  If the source string contains quotes, a new string is allocated
// and the string is copied into it, character by character, skipping "'s.  If
// a non-null pointer is returned, the caller is responsible for freeing it.
template <typename Character>
static Character const* __cdecl strip_quotes(Character const* const source) throw()
{
    // Count the number of quotation marks in the string, and compute the length
    // of the string, in case we need to allocate a new string:
    size_t quote_count   = 0;
    size_t source_length = 0;
    for (Character const* it = source; *it; ++it)
    {
        if (*it == '\"')
            ++quote_count;

        ++source_length;
    }

    // No quotes?  No problem!
    if (quote_count == 0)
        return nullptr;

    size_t const destination_length = source_length - quote_count + 1;
    __crt_unique_heap_ptr<Character> destination(_calloc_crt_t(Character, destination_length));
    if (destination.get() == nullptr)
        return nullptr;

    // Copy the string, stripping quotation marks:
    Character* destination_it = destination.get();
    for (Character const* source_it = source; *source_it; ++source_it)
    {
        if (*source_it == '\"')
            continue;

        *destination_it++ = *source_it;
    }

    *destination_it = '\0';
    return destination.detach();
}

// Gets the path to the %TMP% directory
template <typename Character>
static Character const* __cdecl get_tmp_directory() throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    static Character const tmp_name[] = { 'T', 'M', 'P', '\0' };

    Character* tmp_value = nullptr;
    if (_ERRCHECK_EINVAL(stdio_traits::tdupenv_s_crt(&tmp_value, nullptr, tmp_name)) != 0)
        return nullptr;

    return tmp_value;
}



// Gets the directory relative to which the temporary name should be formed.
// The directory to be used is returned via the 'result' out parameter.  The
// '*result' pointer is never null on return.  If '*result' needs to be freed
// by the caller, the function also returns the pointer; if null is returned,
// no caller cleanup is required.
template <typename Character>
static Character const* __cdecl get_directory(
    Character const*  const alternative,
    Character const** const result
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    __crt_unique_heap_ptr<Character const> tmp(get_tmp_directory<Character>());
    if (tmp.get() != nullptr)
    {
        if (stdio_traits::taccess_s(tmp.get(), 0) == 0)
            return *result = tmp.detach();

        // Otherwise, try stripping quotes out of the TMP path and check again:
        __crt_unique_heap_ptr<Character const> unquoted_tmp(strip_quotes(tmp.get()));
        if (unquoted_tmp.get() != nullptr && stdio_traits::taccess_s(unquoted_tmp.get(), 0) == 0)
            return *result = unquoted_tmp.detach();
    }

    // Otherwise, the TMP path is not usable; use the alternative path if one
    // was provided and is accessible:
    if (alternative != nullptr && stdio_traits::taccess_s(alternative, 0) == 0)
        return (*result = alternative), nullptr;

    // Otherwise, fall back to \ or .:
    static Character const root_fallback[] = { '\\', '\0' };
    static Character const cwd_fallback [] = { '.',  '\0' };
    
    if (stdio_traits::taccess_s(root_fallback, 0) == 0)
        return (*result = root_fallback), nullptr;

    return (*result = cwd_fallback), nullptr;
}



// The path_buffer is a pointer to the beginning of the buffer into which we
// are formatting the temporary path.  The suffix_pointer is a pointer into
// that same buffer, to the position where the unique identifier is to be
// written.  The suffix_count is the number of characters that can be written
// to the suffix_pointer.  The prefix_length is the length of the prefix that
// appears before the suffix in the path_buffer already.
template <typename Character>
static bool __cdecl compute_name(
    Character const* const path_buffer,
    Character*       const suffix_pointer,
    size_t           const suffix_count,
    size_t           const prefix_length
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    // Re-initialize _tempoff if necessary.  If we don't re-init _tempoff, we
    // can get into an infinate loop (e.g., (a) _tempoff is a big number on
    // entry, (b) prefix is a long string (e.g., 8 chars) and all tempfiles
    // with that prefix exist, (c) _tempoff will never equal first and we'll
    // loop forever).

    // [NOTE: To avoid a conflict that causes the same bug as that discussed
    // above, _tempnam() uses _tempoff; tmpnam() uses _tmpoff]

    bool return_value = false;

    __acrt_lock(__acrt_tempnam_lock);
    __try
    {
        if (_old_pfxlen < prefix_length)
            _tempoff = 1;

        _old_pfxlen = static_cast<unsigned int>(prefix_length);

        unsigned const first = _tempoff;

        errno_t const saved_errno = errno;
        do
        {
            ++_tempoff;
            if (_tempoff - first > _TMP_MAX_S)
            {
                errno = saved_errno;
                __leave;
            }

            // The maximum length string returned by the conversion is ten
            // characters, assuming a 32-bit unsigned integer, so there is
            // sufficient room in the result buffer for it.
            _ERRCHECK(stdio_traits::ultot_s(_tempoff, suffix_pointer, suffix_count, 10));
            errno = 0;
        }
        while (stdio_traits::taccess_s(path_buffer, 0) == 0 || errno == EACCES);

        errno = saved_errno;
        return_value = true;
    }
    __finally
    {
        __acrt_unlock(__acrt_tempnam_lock);
    }

    return return_value;
}



// Generates a unique file name within the temporary directory.  If the TMP
// environment variable is defined and is accessible, that directory is used.
// Otherwise, the given 'alternative' directory is used, if that is non-null and
// the path is accessible.  Otherwise, the root of the drive is used if it is
// accessible; otherwise, the local directory is used.
//
// Returns a pointer to the resulting file name on success; returns nullptr on
// failure.  If a non-null pointer is returned, the caller is responsible for
// freeing it by calling 'free()'.
template <typename Character>
_Success_(return != 0)
static Character* __cdecl common_tempnam(
    Character const* const alternative,
    Character const* const prefix,
    int              const block_use,
    char const*      const file_name,
    int              const line_number
    ) throw()
{
    // These are referenced only in the Debug CRT build
    UNREFERENCED_PARAMETER(block_use);
    UNREFERENCED_PARAMETER(file_name);
    UNREFERENCED_PARAMETER(line_number);

    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    Character const* directory = nullptr;
    __crt_unique_heap_ptr<Character const> const directory_cleanup(get_directory(alternative, &directory));

    unsigned const prefix_length = prefix != nullptr
        ? static_cast<unsigned>(stdio_traits::tcslen(prefix))
        : 0;

    // The 12 allows for a backslash, a ten character temporary string, and a
    // null terminator.
    unsigned const buffer_size = static_cast<unsigned>(stdio_traits::tcslen(directory)) + prefix_length + 12;

    __crt_unique_heap_ptr<Character, __crt_public_free_policy> result(
            static_cast<Character*>(_calloc_dbg(
                buffer_size,
                sizeof(Character),
                block_use,
                file_name,
                line_number)));

    if (!result)
        return nullptr;

    *result.get() = 0;
    _ERRCHECK(stdio_traits::tcscat_s(result.get(), buffer_size, directory));

    if (__crt_stdio_path_requires_backslash(directory))
    {
        static Character const backslash[] = { '\\', '\0' };
        _ERRCHECK(stdio_traits::tcscat_s(result.get(), buffer_size, backslash));
    }

    if (prefix != nullptr)
    {
        _ERRCHECK(stdio_traits::tcscat_s(result.get(), buffer_size, prefix));
    }

    Character* const ptr = result.get() + stdio_traits::tcslen(result.get());
    size_t     const ptr_size = buffer_size - (ptr - result.get());

    if (!compute_name(result.get(), ptr, ptr_size, prefix_length))
        return nullptr;

    return result.detach();
}



extern "C" char* __cdecl _tempnam(
    char const* const alternative,
    char const* const prefix
    )
{
    return common_tempnam(alternative, prefix, _NORMAL_BLOCK, nullptr, 0);
}

extern "C" wchar_t* __cdecl _wtempnam(
    wchar_t const* const alternative,
    wchar_t const* const prefix
    )
{
    return common_tempnam(alternative, prefix, _NORMAL_BLOCK, nullptr, 0);
}

#ifdef _DEBUG

extern "C" char* __cdecl _tempnam_dbg(
    char const* const alternative,
    char const* const prefix,
    int         const block_use,
    char const* const file_name,
    int         const line_number
    )
{
    return common_tempnam(alternative, prefix, block_use, file_name, line_number);
}

extern "C" wchar_t* __cdecl _wtempnam_dbg(
    wchar_t const* const alternative,
    wchar_t const* const prefix,
    int            const block_use,
    char const*    const file_name,
    int            const line_number
    )
{
    return common_tempnam(alternative, prefix, block_use, file_name, line_number);
}

#endif