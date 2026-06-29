//
// corecrt_internal_string_templates.h
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This internal header defines template implementations of several secure
// string functions that have identical implementations for both narrow and
// wide character strings.
//
#pragma once

#include <corecrt_internal_securecrt.h>



// _strcat_s() and _wcscat_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcscat_s(
    _Inout_updates_z_(size_in_elements) Character* const destination,
    size_t                                         const size_in_elements,
    _In_z_  Character const*                       const source
    ) throw()
{
    _VALIDATE_STRING(destination, size_in_elements);
    _VALIDATE_POINTER_RESET_STRING(source, destination, size_in_elements);

    Character* destination_it = destination;
    size_t available = size_in_elements;
    while (available > 0 && *destination_it != 0)
    {
        ++destination_it;
        --available;
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_DEST_NOT_NULL_TERMINATED(destination, size_in_elements);
    }

    Character const* source_it = source;
    while ((*destination_it++ = *source_it++) != 0 && --available > 0)
    {
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_BUFFER_TOO_SMALL(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}



// _strcpy_s() and _wcscpy_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcscpy_s(
    _Out_writes_z_(size_in_elements) Character* const destination,
    _In_                                 size_t const size_in_elements,
    _In_z_                     Character const* const source
    ) throw()
{
    _VALIDATE_STRING(destination, size_in_elements);
    _VALIDATE_POINTER_RESET_STRING(source, destination, size_in_elements);

    Character*       destination_it = destination;
    Character const* source_it      = source;

    size_t available = size_in_elements;
    while ((*destination_it++ = *source_it++) != 0 && --available > 0)
    {
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_BUFFER_TOO_SMALL(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}



// _strncat_s() and _wcsncat_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcsncat_s(
    _Inout_updates_z_(size_in_elements) Character* const destination,
    _In_                                    size_t const size_in_elements,
    _In_reads_or_z_(count)        Character const* const source,
    _In_                                    size_t const count
    ) throw()
{
    if (count == 0 && destination == nullptr && size_in_elements == 0)
    {
        // This case is allowed; nothing to do:
        _RETURN_NO_ERROR;
    }

    _VALIDATE_STRING(destination, size_in_elements);
    if (count != 0)
    {
        _VALIDATE_POINTER_RESET_STRING(source, destination, size_in_elements);
    }

    Character* destination_it = destination;

    size_t available = size_in_elements;
    size_t remaining = count;
    while (available > 0 && *destination_it != 0)
    {
        ++destination_it;
        --available;
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_DEST_NOT_NULL_TERMINATED(destination, size_in_elements);
    }

    Character const* source_it = source;
    if (count == _TRUNCATE)
    {
        while ((*destination_it++ = *source_it++) != 0 && --available > 0)
        {
        }
    }
    else
    {
        while (remaining > 0 && (*destination_it++ = *source_it++) != 0 && --available > 0)
        {
            remaining--;
        }

        if (remaining == 0)
        {
            *destination_it = 0;
        }
    }

    if (available == 0)
    {
        if (count == _TRUNCATE)
        {
            destination[size_in_elements - 1] = 0;
            _RETURN_TRUNCATE;
        }
        _RESET_STRING(destination, size_in_elements);
        _RETURN_BUFFER_TOO_SMALL(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}



// _strncpy_s() and _wcsncpy_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcsncpy_s(
    _Out_writes_z_(size_in_elements) Character* const destination,
    _In_                                 size_t const size_in_elements,
    _In_reads_or_z_(count)     Character const* const source,
    _In_                                 size_t const count
    ) throw()
{
    if (count == 0 && destination == nullptr && size_in_elements == 0)
    {
        // this case is allowed; nothing to do:
        _RETURN_NO_ERROR;
    }

    _VALIDATE_STRING(destination, size_in_elements);
    if (count == 0)
    {
        // Notice that the source string pointer can be nullptr in this case:
        _RESET_STRING(destination, size_in_elements);
        _RETURN_NO_ERROR;
    }
    _VALIDATE_POINTER_RESET_STRING(source, destination, size_in_elements);

    Character*       destination_it = destination;
    Character const* source_it      = source;

    size_t available = size_in_elements;
    size_t remaining = count;
    if (count == _TRUNCATE)
    {
        while ((*destination_it++ = *source_it++) != 0 && --available > 0)
        {
        }
    }
    else
    {
        while ((*destination_it++ = *source_it++) != 0 && --available > 0 && --remaining > 0)
        {
        }
        if (remaining == 0)
        {
            *destination_it = 0;
        }
    }

    if (available == 0)
    {
        if (count == _TRUNCATE)
        {
            destination[size_in_elements - 1] = 0;
            _RETURN_TRUNCATE;
        }
        _RESET_STRING(destination, size_in_elements);
        _RETURN_BUFFER_TOO_SMALL(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}



// _strnset_s() and _wcsnset_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcsnset_s(
    _Inout_updates_z_(size_in_elements) Character* const destination,
    _In_                                    size_t const size_in_elements,
    _In_                                 Character const value,
    _In_                                    size_t const count
    ) throw()
{
    if (count == 0 && destination == nullptr && size_in_elements == 0)
    {
        // This case is allowed; nothing to do:
        _RETURN_NO_ERROR;
    }
    _VALIDATE_STRING(destination, size_in_elements);

    Character* destination_it = destination;

    size_t available = size_in_elements;
    size_t remaining = count;
    while (*destination_it != 0 && remaining > 0 && --available > 0)
    {
        *destination_it++ = value;
        --remaining;
    }

    if (remaining == 0)
    {
        // Ensure the string is null-terminated:
        while (*destination_it != 0 && --available > 0)
        {
            ++destination_it;
        }
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_DEST_NOT_NULL_TERMINATED(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}



// _strset_s() and _wcsset_s()
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_tcsset_s(
    _Inout_updates_z_(size_in_elements) Character* const destination,
    _In_                                    size_t const size_in_elements,
    _In_                                 Character const value
    ) throw()
{
    _VALIDATE_STRING(destination, size_in_elements);

    Character* destination_it = destination;

    size_t available = size_in_elements;
    while (*destination_it != 0 && --available > 0)
    {
        *destination_it++ = value;
    }

    if (available == 0)
    {
        _RESET_STRING(destination, size_in_elements);
        _RETURN_DEST_NOT_NULL_TERMINATED(destination, size_in_elements);
    }
    _FILL_STRING(destination, size_in_elements, size_in_elements - available + 1);
    _RETURN_NO_ERROR;
}
