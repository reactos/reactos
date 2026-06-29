/***
*splitpath.c - break down path name into components
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       To provide support for accessing the individual components of an
*       arbitrary path name
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <mbctype.h>
#include <stdlib.h>
#include <corecrt_internal_securecrt.h>
#include <corecrt_internal_traits.h>

namespace
{
    template <typename Character>
    struct component_buffers
    {
        _Null_terminated_ _Maybenull_
        Character* _drive;
        size_t     _drive_count;
        _Null_terminated_ _Maybenull_
        Character* _directory;
        size_t     _directory_count;
        _Null_terminated_ _Maybenull_
        Character* _file_name;
        size_t     _file_name_count;
        _Null_terminated_ _Maybenull_
        Character* _extension;
        size_t     _extension_count;
    };
}

template <typename Character, typename ResetPolicy>
static void __cdecl reset_buffers(
    component_buffers<Character>* const components,
    ResetPolicy                   const reset_buffer
    ) throw()
{
    reset_buffer(components->_drive,     components->_drive_count    );
    reset_buffer(components->_directory, components->_directory_count);
    reset_buffer(components->_file_name, components->_file_name_count);
    reset_buffer(components->_extension, components->_extension_count);
}

// is_lead_byte helper 
// these functions are only used to ensure that trailing bytes that might
// look like slashes or periods aren't misdetected.
// UTF-8/UTF-16 don't have that problem as trail bytes never look like \ or .
static bool __cdecl needs_trail_byte(char const c) throw()
{
    // UTF-8 is OK here as the caller is really only concerned about trail
    // bytes that look like . or \ and UTF-8 trail bytes never will.
    return _ismbblead(c) != 0;
}

static bool __cdecl needs_trail_byte(wchar_t) throw()
{
    // UTF-16 is OK here as the caller is really only concerned about trail
    // characters that look like . or \ and UTF-16 surrogate pairs never will.
    return false;
}

template <typename Character, typename ResetPolicy, typename BufferCountTransformer>
static errno_t __cdecl common_splitpath_internal(
    Character const*              const path,
    component_buffers<Character>* const components,
    ResetPolicy                   const reset_buffer,
    BufferCountTransformer        const transform_buffer_count
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    if (!path || !components)
    {
        reset_buffers(components, reset_buffer);
        _VALIDATE_RETURN_ERRCODE(false, EINVAL);
    }

    if ((components->_drive     == nullptr) != (components->_drive_count     == 0) ||
        (components->_directory == nullptr) != (components->_directory_count == 0) ||
        (components->_file_name == nullptr) != (components->_file_name_count == 0) ||
        (components->_extension == nullptr) != (components->_extension_count == 0))
    {
        reset_buffers(components, reset_buffer);
        _VALIDATE_RETURN_ERRCODE(false, EINVAL);
    }

    Character const* path_it = path;

    // Extract drive letter and ':', if any:
    {
        size_t skip = _MAX_DRIVE - 2;
        Character const* p = path_it;
        while (skip > 0 && *p != '\0')
        {
            --skip;
            ++p;
        }

        if (*p == ':')
        {
            if (components->_drive)
            {
                if (components->_drive_count < _MAX_DRIVE)
                {
                    reset_buffers(components, reset_buffer);
                    return errno = ERANGE;
                }

                traits::tcsncpy_s(components->_drive, transform_buffer_count(components->_drive_count), path_it, _MAX_DRIVE - 1);
            }

            path_it = p + 1;
        }
        else
        {
            reset_buffer(components->_drive, components->_drive_count);
        }
    }

    // Extract the path string, if any.  The path iterator now points to the first
    // character of the path, if there is one, or to the filename or extension if
    // no path was specified.  Scan ahead for the last occurence, if any, of a '/'
    //  or '\' path separator character.  If none is found, there is no path.  We
    // will also note the last '.' character found, if any, to aid in handling the
    // extension.
    Character const* p          = path_it;
    Character const* last_slash = nullptr;
    Character const* last_dot   = nullptr;
    for (; *p != '\0'; ++p)
    {
        // UTF-8 will never look like slashes or periods so this will be OK for UTF-8
        if (needs_trail_byte(*p))
        {
            // For narrow character strings, skip any multibyte characters to avoid
            // matching trail bytes that "look like" slashes or periods.  This ++p
            // will skip the lead byte; the ++p in the for loop will skip the trail
            // byte.
            ++p;

            // If we've reached the end of the string, there is no trail byte.
            // (Technically, the string is malformed.)
            if (*p == '\0')
            {
                break;
            }
        }
        else if (*p == '/' || *p == '\\')
        {
            last_slash = p + 1; // Point one past for later copy
        }
        else if (*p == '.')
        {
            last_dot = p;
        }
    }

    if (last_slash)
    {
        if (components->_directory)
        {
            size_t const length = static_cast<size_t>(last_slash - path_it);
            if (components->_directory_count <= length)
            {
                reset_buffers(components, reset_buffer);
                return errno = ERANGE;
            }

            traits::tcsncpy_s(components->_directory, transform_buffer_count(components->_directory_count), path_it, length);
        }

        path_it = last_slash;
    }
    else
    {
        reset_buffer(components->_directory, components->_directory_count);
    }

    // Extract the file name and extension, if any.  The path iterator now points
    // to the first character of the file name, if any, or the extension if no
    // file name was given.  The dot points to the '.' beginning the extension,
    // if any.
    if (last_dot && last_dot >= path_it)
    {
        // We found a dot; it separates the file name from the extension:
        if (components->_file_name)
        {
            size_t const length = static_cast<size_t>(last_dot - path_it);
            if (components->_file_name_count <= length)
            {
                reset_buffers(components, reset_buffer);
                return errno = ERANGE;
            }

            traits::tcsncpy_s(components->_file_name, transform_buffer_count(components->_file_name_count), path_it, length);
        }

        if (components->_extension)
        {
            size_t const length = static_cast<size_t>(p - last_dot);
            if (components->_extension_count <= length)
            {
                reset_buffers(components, reset_buffer);
                return errno = ERANGE;
            }

            traits::tcsncpy_s(components->_extension, transform_buffer_count(components->_extension_count), last_dot, length);
        }
    }
    else
    {
        // No extension found; reset the extension and treat the remaining text
        // as the file name:
        if (components->_file_name)
        {
            size_t const length = static_cast<size_t>(p - path_it);
            if (components->_file_name_count <= length)
            {
                reset_buffers(components, reset_buffer);
                return errno = ERANGE;
            }

            traits::tcsncpy_s(components->_file_name, transform_buffer_count(components->_file_name_count), path_it, length);
        }

        if (components->_extension)
        {
            reset_buffer(components->_extension, components->_extension_count);
        }
    }

    return 0;
}

template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_splitpath_s(
    Character const*              const path,
    component_buffers<Character>* const components
    ) throw()
{
    return common_splitpath_internal(path, components, [](_Out_writes_z_(buffer_count) Character* const buffer, size_t const buffer_count)
    {
        UNREFERENCED_PARAMETER(buffer);
        UNREFERENCED_PARAMETER(buffer_count);
        if (buffer)
        {
            _RESET_STRING(buffer, buffer_count);
        }
    },
    [](size_t const n) { return n; });
}


extern "C" errno_t __cdecl _splitpath_s(
    char const* const path,
    char*       const drive,
    size_t      const drive_count,
    char*       const directory,
    size_t      const directory_count,
    char*       const file_name,
    size_t      const file_name_count,
    char*       const extension,
    size_t      const extension_count
    )
{
    component_buffers<char> components =
    {
        drive,     drive_count,
        directory, directory_count,
        file_name, file_name_count,
        extension, extension_count
    };

    return common_splitpath_s(path, &components);
}

extern "C" errno_t __cdecl _wsplitpath_s(
    wchar_t const* const path,
    wchar_t*       const drive,
    size_t         const drive_count,
    wchar_t*       const directory,
    size_t         const directory_count,
    wchar_t*       const file_name,
    size_t         const file_name_count,
    wchar_t*       const extension,
    size_t         const extension_count
    )
{
    component_buffers<wchar_t> components =
    {
        drive,     drive_count,
        directory, directory_count,
        file_name, file_name_count,
        extension, extension_count
    };

    return common_splitpath_s(path, &components);
}

template <typename Character>
static void __cdecl common_splitpath(
    _In_z_                   Character const* const path,
    _Pre_maybenull_ _Post_z_ Character*       const drive,
    _Pre_maybenull_ _Post_z_ Character*       const directory,
    _Pre_maybenull_ _Post_z_ Character*       const file_name,
    _Pre_maybenull_ _Post_z_ Character*       const extension
    ) throw()
{
    component_buffers<Character> components =
    {
        drive,     drive     ? _MAX_DRIVE : 0,
        directory, directory ? _MAX_DIR   : 0,
        file_name, file_name ? _MAX_FNAME : 0,
        extension, extension ? _MAX_EXT   : 0
    };

    common_splitpath_internal(path, &components, [](Character* const buffer, size_t const buffer_count)
    {
        if (buffer && buffer_count != 0)
        {
            buffer[0] = '\0';
        }
    },
    [](size_t){ return static_cast<size_t>(-1); });
}

extern "C" void __cdecl _splitpath(
    char const* const path,
    char*       const drive,
    char*       const directory,
    char*       const file_name,
    char*       const extension
    )
{
    return common_splitpath(path, drive, directory, file_name, extension);
}

extern "C" void __cdecl _wsplitpath(
    wchar_t const* const path,
    wchar_t*       const drive,
    wchar_t*       const directory,
    wchar_t*       const file_name,
    wchar_t*       const extension
    )
{
    return common_splitpath(path, drive, directory, file_name, extension);
}
