/***
*wild.c - wildcard expander
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*        expands wildcards in argv
*
*        handles '*' (none or more of any char) and '?' (exactly one char)
*
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <ctype.h>
#include <corecrt_internal_traits.h>
#include <limits.h>
#include <mbstring.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wrl/wrappers/corewrappers.h>



namespace
{
    template <typename Character>
    class argument_list
    {
    public:

        argument_list() throw() : _first(nullptr), _last(nullptr), _end(nullptr) { }

        size_t      size()  const throw() { return _last - _first; }
        Character** begin() const throw() { return _first;         }
        Character** end()   const throw() { return _last;          }

        errno_t append(Character* const element) throw()
        {
            errno_t const expand_status = expand_if_necessary();
            if (expand_status != 0)
            {
                _free_crt(element);
                return expand_status;
            }

            *_last++ = element;
            return 0;
        }

        Character** detach() throw()
        {
            _last = nullptr;
            _end  = nullptr;

            Character** const first = _first;
            _first = nullptr;
            return first;
        }

        ~argument_list() throw()
        {
            for (auto it = _first; it != _last; ++it)
                _free_crt(*it);

            _free_crt(_first);
        }

    private:

        argument_list(argument_list const&) throw();            // not implemented
        argument_list& operator=(argument_list const&) throw(); // not implemented

        errno_t expand_if_necessary() throw()
        {
            // If there is already room for more elements, just return:
            if (_last != _end)
            {
                return 0;
            }
            // If the list has not yet had an array allocated for it, allocate one:
            if (!_first)
            {
                size_t const initial_count = 4;

                _first = _calloc_crt_t(Character*, initial_count).detach();
                if (!_first)
                    return ENOMEM;

                _last = _first;
                _end  = _first + initial_count;
                return 0;
            }
            // Otherwise, double the size of the array:
            else
            {
                size_t const old_count = _end - _first;
                if (old_count > SIZE_MAX / 2)
                    return ENOMEM;

                size_t const new_count = old_count * 2;
                __crt_unique_heap_ptr<Character*> new_array(_recalloc_crt_t(Character*, _first, new_count));
                if (!new_array)
                    return ENOMEM;

                _first = new_array.detach();
                _last = _first + old_count;
                _end  = _first + new_count;
                return 0;
            }
        }

        Character** _first;
        Character** _last;
        Character** _end;
    };
}

_Check_return_
static char*
previous_character(_In_reads_z_(current - first + 1) char* const first,
                   _In_z_ char* const current) throw()
{
    return reinterpret_cast<char*>(_mbsdec(
        reinterpret_cast<unsigned char*>(first),
        reinterpret_cast<unsigned char*>(current)));
}

static wchar_t* previous_character(_In_reads_(0) wchar_t*, _In_reads_(0) wchar_t* const current) throw()
{
    return current - 1;
}



template <typename Character>
static errno_t copy_and_add_argument_to_buffer(
    _In_z_ Character const*    const file_name,
    _In_z_ Character const*    const directory,
               size_t          const directory_length,
    argument_list<Character>&        buffer
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    size_t const file_name_count = traits::tcslen(file_name) + 1;
    if (file_name_count > SIZE_MAX - directory_length)
        return ENOMEM;

    size_t const required_count = directory_length + file_name_count + 1;
    __crt_unique_heap_ptr<Character> argument_buffer(_calloc_crt_t(Character, required_count));

    if (directory_length > 0)
    {
        _ERRCHECK(traits::tcsncpy_s(argument_buffer.get(), required_count, directory, directory_length));
    }

    _ERRCHECK(traits::tcsncpy_s(
        argument_buffer.get() + directory_length,
        required_count        - directory_length,
        file_name,
        file_name_count));

    return buffer.append(argument_buffer.detach());
}


static wchar_t * get_wide(__crt_internal_win32_buffer<wchar_t> * const dest, char * const source)
{
    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(
        source,
        *dest,
        __acrt_get_utf8_acp_compatibility_codepage()
    );

    if (cvt1 != 0)
    {
        return nullptr;
    }

    return dest->data();
};

static wchar_t * get_wide(__crt_internal_win32_buffer<wchar_t> *, wchar_t * const source)
{
    return source;
}

static char * get_file_name(__crt_internal_win32_buffer<char> * const dest, wchar_t * const source)
{
    errno_t const cvt = __acrt_wcs_to_mbs_cp(
        source,
        *dest,
        __acrt_get_utf8_acp_compatibility_codepage()
    );

    if (cvt != 0)
    {
        return nullptr;
    }

    return dest->data();
}

static wchar_t * get_file_name(__crt_internal_win32_buffer<wchar_t> *, wchar_t * const source)
{
    return source;
}

template <typename Character>
static errno_t expand_argument_wildcards(
    Character*          const argument,
    Character*          const wildcard,
    argument_list<Character>& buffer
    ) throw()
{
    typedef __crt_char_traits<Character>          traits;
    typedef typename traits::win32_find_data_type find_data_type;

    auto const is_directory_separator = [](Character const c) { return c == '/' || c == '\\' || c == ':'; };

    // Find the first slash or colon before the wildcard:
    Character* it = wildcard;
    while (it != argument && !is_directory_separator(*it))
    {
        it = previous_character(argument, it);
    }

    // If we found a colon that can't form a drive name (e.g. it can't be 'D:'),
    // then just add the argument as-is (we don't know how to expand it):
    if (*it == ':' && it != argument + 1)
    {
        return copy_and_add_argument_to_buffer(argument, static_cast<Character*>(nullptr), 0, buffer);
    }

    size_t const directory_length = is_directory_separator(*it)
        ? it - argument + 1 // it points to the separator, so add 1 to include it.
        : 0;

    // Try to begin the find operation:
    WIN32_FIND_DATAW findFileDataW;
    __crt_internal_win32_buffer<wchar_t> wide_file_name;

    __crt_findfile_handle const find_handle(::FindFirstFileExW(
        get_wide(&wide_file_name, argument),
        FindExInfoStandard,
        &findFileDataW,
        FindExSearchNameMatch,
        nullptr,
        0));

    // If the find operation failed, there was no match, so just add the argument:
    if (find_handle.get() == INVALID_HANDLE_VALUE)
    {
        return copy_and_add_argument_to_buffer(argument, static_cast<Character*>(nullptr), 0, buffer);
    }

    size_t const old_argument_count = buffer.size();

    do
    {
        __crt_internal_win32_buffer<Character> character_buffer;
        Character* const file_name = get_file_name(&character_buffer, findFileDataW.cFileName);
        // Skip . and ..:
        if (file_name[0] == '.' && file_name[1] == '\0')
        {
            continue;
        }

        if (file_name[0] == '.' && file_name[1] == '.' && file_name[2] == '\0')
        {
            continue;
        }

        errno_t const add_status = copy_and_add_argument_to_buffer(file_name, argument, directory_length, buffer);
        if (add_status != 0)
        {
            return add_status;
        }
    }
    while (::FindNextFileW(find_handle.get(), &findFileDataW));

    // If we didn't add any arguments to the buffer, then we're done:
    size_t const new_argument_count = buffer.size();
    if (old_argument_count == new_argument_count)
    {
        return 0;
    }

    // If we did add new arguments, let's helpfully sort them:
    qsort(
        buffer.begin()     + old_argument_count,
        new_argument_count - old_argument_count,
        sizeof(Character*),
        [](void const* lhs, void const* rhs) -> int
        {
            if (lhs < rhs) { return -1; }
            if (lhs > rhs) { return  1; }
            return 0;
        });

    return 0;
}


template <typename Character>
static errno_t common_expand_argv_wildcards(Character** const argv, Character*** const result) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);
    *result = nullptr;

    argument_list<Character> expansion_buffer;
    for (Character** it = argv; *it != nullptr; ++it)
    {
        Character const wildcard_characters[] = { '*', '?', '\0' };
        Character* const wildcard = traits::tcspbrk(*it, wildcard_characters);

        // If no wildcard characters were found in the argument string, just
        // append it to the list and continue on.  Otherwise, do the expansion:
        if (!wildcard)
        {
            errno_t const append_status = copy_and_add_argument_to_buffer(
                *it,
                static_cast<Character*>(nullptr),
                0,
                expansion_buffer);

            if (append_status != 0)
                return append_status;
        }
        else
        {
            errno_t const expand_status = expand_argument_wildcards(*it, wildcard, expansion_buffer);
            if (expand_status != 0)
                return expand_status;
        }
    }

    // Now that we've accumulated the expanded arguments into the expansion
    // buffer, we want to re-pack them in the form used by the argv parser,
    // in a single array, with everything "concatenated" together.
    size_t const argument_count  = expansion_buffer.size() + 1;
    size_t       character_count = 0;
    for (auto it = expansion_buffer.begin(); it != expansion_buffer.end(); ++it)
        character_count += traits::tcslen(*it) + 1;

    __crt_unique_heap_ptr<unsigned char> expanded_argv(__acrt_allocate_buffer_for_argv(
        argument_count,
        character_count,
        sizeof(Character)));

    if (!expanded_argv)
        return -1;

    Character** const argument_first  = reinterpret_cast<Character**>(expanded_argv.get());
    Character*  const character_first = reinterpret_cast<Character*>(
        expanded_argv.get() +
        argument_count * sizeof(Character*));

    Character** argument_it  = argument_first;
    Character*  character_it = character_first;
    for (auto it = expansion_buffer.begin(); it != expansion_buffer.end(); ++it)
    {
        size_t const count = traits::tcslen(*it) + 1;

        _ERRCHECK(traits::tcsncpy_s(
            character_it,
            character_count - (character_it - character_first),
            *it,
            count));

        *argument_it++ = character_it;
        character_it += count;
    }

    *result = reinterpret_cast<Character**>(expanded_argv.detach());
    return 0;
}

extern "C" errno_t __acrt_expand_narrow_argv_wildcards(char** const argv, char*** const result)
{
    return common_expand_argv_wildcards(argv, result);
}

extern "C" errno_t __acrt_expand_wide_argv_wildcards(wchar_t** const argv, wchar_t*** const result)
{
    return common_expand_argv_wildcards(argv, result);
}
