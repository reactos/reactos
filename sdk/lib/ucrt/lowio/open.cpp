//
// open.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _open() and its friends, who are used to open or create files.
//
// These functions are used to open a file.
//
// oflag:   The primary file open flags are passed via this parameter.  It may
//          have a combination of the following flags:
//           * _O_APPEND:     Reposition file ptr to end before every write
//           * _O_BINARY:     Open in binary mode
//           * _O_CREAT:      Create a new file* no effect if file already exists
//           * _O_EXCL:       Return error if file exists, only use with O_CREAT
//           * _O_RDONLY:     Open for reading only
//           * _O_RDWR:       Open for reading and writing
//           * _O_TEXT:       Open in text mode
//           * _O_TRUNC:      Open and truncate to 0 length (must have write permission)
//           * _O_WRONLY:     Open for writing only
//           * _O_NOINHERIT:  Handle will not be inherited by child processes.
//          Exactly one of _O_RDONLY, _O_WRONLY, and _O_RDWR must be present.
//
// shflag:  Specifies the sharing options with which the file is to be opened.
//          This parameter is only supported by the sharing-enabled open
//          functions (_tsopen, _tsopen_s, etc.). The following flags are
//          supported:
//           * _SH_COMPAT:  Set compatability mode
//           * _SH_DENYRW:  Deny read and write access to the file
//           * _SH_DENYWR:  Deny write access to the file
//           * _SH_DENYRD:  Deny read access to the file
//           * _SH_DENYNO:  Permit read and write access
//
// pmode:   The pmode argument is only required when _O_CREAT is specified.  Its
//          flags are as follows:
//           * _S_IWRITE:
//           * _S_IREAD:
//          These flags may be combined (_S_IWRITE | _S_IREAD) to enable both
//          reading and writing.  The current file permission mask is applied to
//          pmode before setting the permission (see umask).
//
// Functions that return an errno_t return 0 on success and an error code on
// failure.  Functions that return an int return the file handle on success, and
// return -1 and set errno on failure.
//
#include <corecrt_internal_lowio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>



namespace
{
    DWORD const GENERIC_READ_WRITE = (GENERIC_READ | GENERIC_WRITE);

    struct file_options
    {
        // These are the flags that are used for the osflag of the CRT file
        // object that is created.
        char  crt_flags;

        // These are the flags that are eventually passed to CreateFile to tell
        // the Operating System how to create the file:
        DWORD access;
        DWORD create;
        DWORD share;
        DWORD attributes;
        DWORD flags;
    };
}



#define UTF16LE_BOM     0xFEFF      // UTF16 Little Endian Byte Order Mark
#define UTF16BE_BOM     0xFFFE      // UTF16 Big Endian Byte Order Mark
#define BOM_MASK        0xFFFF      // Mask for testing Byte Order Mark
#define UTF8_BOM        0xBFBBEF    // UTF8 Byte Order Mark
#define UTF16_BOMLEN    2           // No of Bytes in a UTF16 BOM
#define UTF8_BOMLEN     3           // No of Bytes in a UTF8 BOM

template <typename Character>
static int __cdecl common_open(
    _In_z_  Character const* const  path,
            int              const  oflag,
            int              const  pmode
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(path != nullptr, EINVAL, -1);

    int fh = -1;
    int unlock_flag = 0;
    errno_t error_code = 0;
    __try
    {
        error_code = traits::tsopen_nolock(&unlock_flag, &fh, path, oflag, _SH_DENYNO, pmode, 0);
    }
    __finally
    {
        if (unlock_flag)
        {
            if (error_code)
            {
                _osfile(fh) &= ~FOPEN;
            }

            __acrt_lowio_unlock_fh(fh);
        }
    }
    __endtry

    if (error_code != 0)
    {
        errno = error_code;
        return -1;
    }

    return fh;
}

extern "C" int _open(char const* const path, int const oflag, ...)
{
    va_list arglist;
    va_start(arglist, oflag);
    int const pmode = va_arg(arglist, int);
    va_end(arglist);

    return common_open(path, oflag, pmode);
}

extern "C" int _wopen(wchar_t const* const path, int const oflag, ...)
{
    va_list arglist;
    va_start(arglist, oflag);
    int const pmode = va_arg(arglist, int);
    va_end(arglist);

    return common_open(path, oflag, pmode);
}



template <typename Character>
static errno_t __cdecl common_sopen_dispatch(
    _In_z_  Character const* const path,
            int              const oflag,
            int              const shflag,
            int              const pmode,
            int*             const pfh,
            int              const secure
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN_ERRCODE(pfh != nullptr, EINVAL);
    *pfh = -1;

    _VALIDATE_RETURN_ERRCODE(path != nullptr, EINVAL);

    if(secure)
    {
        _VALIDATE_RETURN_ERRCODE((pmode & (~(_S_IREAD | _S_IWRITE))) == 0, EINVAL);
    }


    int unlock_flag = 0;
    errno_t error_code = 0;
    __try
    {
        error_code = traits::tsopen_nolock(&unlock_flag, pfh, path, oflag, shflag, pmode, secure);
    }
    __finally
    {
        if (unlock_flag)
        {
            if (error_code)
            {
                _osfile(*pfh) &= ~FOPEN;
            }
            __acrt_lowio_unlock_fh(*pfh);
        }
    }
    __endtry

    if (error_code != 0)
    {
        *pfh = -1;
    }

    return error_code;
}

extern "C" errno_t __cdecl _sopen_dispatch(
    char const* const path,
    int         const oflag,
    int         const shflag,
    int         const pmode,
    int*        const pfh,
    int         const secure
    )
{
    return common_sopen_dispatch(path, oflag, shflag, pmode, pfh, secure);
}

extern "C" errno_t __cdecl _wsopen_dispatch(
    wchar_t const* const path,
    int            const oflag,
    int            const shflag,
    int            const pmode,
    int*           const pfh,
    int            const secure
    )
{
    return common_sopen_dispatch(path, oflag, shflag, pmode, pfh, secure);
}



static HANDLE __cdecl create_file(
    PCWSTR               const path,
    SECURITY_ATTRIBUTES* const security_attributes,
    file_options         const options
    ) throw()
{
    return CreateFileW(
        path,
        options.access,
        options.share,
        security_attributes,
        options.create,
        options.flags | options.attributes,
        nullptr);
}



static DWORD decode_access_flags(int const oflag) throw()
{
    switch (oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR))
    {
    case _O_RDONLY:
            return GENERIC_READ;

    case _O_WRONLY:
        // If the file is being opened in append mode, we give read access as
        // well because in append (a, not a+) mode, we need to read the BOM to
        // determine the encoding (ANSI, UTF-8, or UTF-16).
        if ((oflag & _O_APPEND) && (oflag & (_O_WTEXT | _O_U16TEXT | _O_U8TEXT)) != 0)
            return GENERIC_READ | GENERIC_WRITE;

        return GENERIC_WRITE;

    case _O_RDWR:
        return GENERIC_READ | GENERIC_WRITE;
    }

    // This is unreachable, but the compiler can't tell.
    _VALIDATE_RETURN(("Invalid open flag", 0), EINVAL, static_cast<DWORD>(-1));
    return 0;
}

static DWORD decode_open_create_flags(int const oflag) throw()
{
    switch (oflag & (_O_CREAT | _O_EXCL | _O_TRUNC))
    {
    case 0:
    case _O_EXCL: // ignore EXCL w/o CREAT
        return OPEN_EXISTING;

    case _O_CREAT:
        return OPEN_ALWAYS;

    case _O_CREAT | _O_EXCL:
    case _O_CREAT | _O_TRUNC | _O_EXCL:
        return CREATE_NEW;

    case _O_TRUNC:
    case _O_TRUNC | _O_EXCL: // ignore EXCL w/o CREAT
        return TRUNCATE_EXISTING;

    case _O_CREAT | _O_TRUNC:
        return CREATE_ALWAYS;
    }

    // This is unreachable, but the compiler can't tell.
    _VALIDATE_RETURN(("Invalid open flag", 0), EINVAL, static_cast<DWORD>(-1));
    return 0;
}

static DWORD decode_sharing_flags(int const shflag, int const access) throw()
{
    switch (shflag)
    {
    case _SH_DENYRW:
        return 0;

    case _SH_DENYWR:
        return FILE_SHARE_READ;

    case _SH_DENYRD:
        return FILE_SHARE_WRITE;

    case _SH_DENYNO:
        return FILE_SHARE_READ | FILE_SHARE_WRITE;

    case _SH_SECURE:
        if (access == GENERIC_READ)
            return FILE_SHARE_READ;
        else
            return 0;
    }

    _VALIDATE_RETURN(("Invalid sharing flag", 0), EINVAL, static_cast<DWORD>(-1));
    return 0;
}

static bool is_text_mode(int const oflag) throw()
{
    if (oflag & _O_BINARY)
        return false;

    if (oflag & (_O_TEXT | _O_WTEXT | _O_U16TEXT | _O_U8TEXT))
        return true;

    // Finally, check the global default mode:
    int fmode;
    _ERRCHECK(_get_fmode(&fmode));
    if (fmode != _O_BINARY)
        return true;

    return false;
}

static file_options decode_options(int const oflag, int const shflag, int const pmode) throw()
{
    file_options result;
    result.crt_flags  = 0;
    result.access     = decode_access_flags(oflag);
    result.create     = decode_open_create_flags(oflag);
    result.share      = decode_sharing_flags(shflag, result.access);
    result.attributes = FILE_ATTRIBUTE_NORMAL;
    result.flags      = 0;

    if (oflag & _O_NOINHERIT)
    {
        result.crt_flags |= FNOINHERIT;
    }

    if (is_text_mode(oflag))
    {
        result.crt_flags |= FTEXT;
    }

    if (oflag & _O_CREAT)
    {
        if (((pmode & ~_umaskval) & _S_IWRITE) == 0)
            result.attributes = FILE_ATTRIBUTE_READONLY;
    }

    if (oflag & _O_TEMPORARY)
    {
        result.flags  |= FILE_FLAG_DELETE_ON_CLOSE;
        result.access |= DELETE;
        result.share  |= FILE_SHARE_DELETE;
    }

    if (oflag & _O_SHORT_LIVED)
    {
        result.attributes |= FILE_ATTRIBUTE_TEMPORARY;
    }

    if (oflag & _O_OBTAIN_DIR)
    {
        result.flags |= FILE_FLAG_BACKUP_SEMANTICS;
    }

    if (oflag & _O_SEQUENTIAL)
    {
        result.flags |= FILE_FLAG_SEQUENTIAL_SCAN;
    }
    else if (oflag & _O_RANDOM)
    {
        result.flags |= FILE_FLAG_RANDOM_ACCESS;
    }

    return result;
}



// If we open a text mode file for writing, and the file ends in Ctrl+Z, we need
// to remove the Ctrl+Z character so that appending will work.  We do this by
// seeking to the end of the file, testing if the last character is a Ctrl+Z,
// truncating the file if it is, then rewinding back to the beginning.
static errno_t truncate_ctrl_z_if_present(int const fh) throw()
{
    // No truncation is possible for devices and pipes:
    if (_osfile(fh) & (FDEV | FPIPE))
        return 0;

    // No truncation is necessary for binary files:
    if ((_osfile(fh) & FTEXT) == 0)
        return 0;

    // Find the end of the file:
    __int64 const last_char_position = _lseeki64_nolock(fh, -1, SEEK_END);

    // If the seek failed, either the file is empty or an error occurred.
    // (It's not an error if the file is empty.)
    if (last_char_position == -1)
    {
        if (_doserrno == ERROR_NEGATIVE_SEEK)
            return 0;

        return errno;
    }

    // Read the last character.  If the read succeeds and the character
    // is a Ctrl+Z, remove the character from the file by shortening:
    wchar_t c = 0;
    if (_read_nolock(fh, &c, 1) == 0 && c == 26)
    {
        if (_chsize_nolock(fh, last_char_position) == -1)
            return errno;
    }

    // Now, rewind the file pointer back to the beginning:
    if (_lseeki64_nolock(fh, 0, SEEK_SET) == -1)
        return errno;

    return 0;
}



// Computes the text mode to be used for a file, using a combination of the
// options passed into the open function and the BOM read from the file.
static errno_t configure_text_mode(
    int              const fh,
    file_options     const options,
    int                    oflag,
    __crt_lowio_text_mode& text_mode
    ) throw()
{
    // The text mode is ANSI by default:
    text_mode = __crt_lowio_text_mode::ansi;

    // If the file is open in binary mode, it gets the default text mode:
    if ((_osfile(fh) & FTEXT) == 0)
        return 0;

    // Set the default text mode per the oflag.  The BOM may change the default,
    // if one is present.  If oflag does not specify a text mode, use the _fmode
    // default:
    DWORD const text_mode_mask = (_O_TEXT | _O_WTEXT | _O_U16TEXT | _O_U8TEXT);
    if ((oflag & text_mode_mask) == 0)
    {
        int fmode = 0;
        _ERRCHECK(_get_fmode(&fmode));

        if ((fmode & text_mode_mask) == 0)
            oflag |= _O_TEXT; // Default to ANSI.
        else
            oflag |= fmode & text_mode_mask;
    }

    // Now oflags should be set to one of the text modes:
    _ASSERTE((oflag & text_mode_mask) != 0);

    switch (oflag & text_mode_mask)
    {
    case _O_TEXT:
        text_mode = __crt_lowio_text_mode::ansi;
        break;

    case _O_WTEXT:
    case _O_WTEXT | _O_TEXT:
        if ((oflag & (_O_WRONLY | _O_CREAT | _O_TRUNC)) == (_O_WRONLY | _O_CREAT | _O_TRUNC))
            text_mode = __crt_lowio_text_mode::utf16le;
        break;

    case _O_U16TEXT:
    case _O_U16TEXT | _O_TEXT:
        text_mode = __crt_lowio_text_mode::utf16le;
        break;

    case _O_U8TEXT:
    case _O_U8TEXT | _O_TEXT:
        text_mode = __crt_lowio_text_mode::utf8;
        break;
    }


    // If the file hasn't been opened with the UNICODE flags then we have
    // nothing to do:  the text mode is the default mode that we just set:
    if ((oflag & (_O_WTEXT | _O_U16TEXT | _O_U8TEXT)) == 0)
        return 0;

    // If this file refers to a device, we cannot check the BOM, so we have
    // nothing to do:  the text mode is the default mode that we just set:
    if ((options.crt_flags & FDEV) != 0)
        return 0;


    // Determine whether we need to check or write the BOM, by testing the
    // access with which the file was opened and whether the file already
    // existed or was just created:
    int check_bom = 0;
    int write_bom = 0;
    switch (options.access & GENERIC_READ_WRITE)
    {
    case GENERIC_READ:
        check_bom = 1;
        break;

    case GENERIC_WRITE:
    case GENERIC_READ_WRITE:
        switch (options.create)
        {
        // If this file was opened, we will read the BOM if the file was opened
        // with read/write access.  We will write the BOM if and only if the
        // file is empty:
        case OPEN_EXISTING:
        case OPEN_ALWAYS:
        {
            if (_lseeki64_nolock(fh, 0, SEEK_END) != 0)
            {
                if (_lseeki64_nolock(fh, 0, SEEK_SET) == -1)
                    return errno;

                // If we have read access, then we need to check the BOM.  Note
                // that we've taken a shortcut here:  if the file is empty, then
                // we do not set this flag because the file doesn't have a BOM
                // to be read.
                check_bom = (options.access & GENERIC_READ) != 0;
            }
            else
            {
                write_bom = 1;
                break;
            }
            break;
        }

        // If this is a new or truncated file, then we always write the BOM:
        case CREATE_NEW:
        case CREATE_ALWAYS:
        case TRUNCATE_EXISTING:
        {
            write_bom = 1;
            break;
        }
        }
        break;
    }

    if (check_bom)
    {
        int bom = 0;
        int const count = _read_nolock(fh, &bom, UTF8_BOMLEN);

        // Intrernal validation:  This branch should never be taken if write_bom
        // is true and count > 0:
        if (count > 0 && write_bom == 1)
        {
            _ASSERTE(0 && "Internal Error");
            write_bom = 0;
        }

        switch (count)
        {
        case -1:
            return errno;

        case UTF8_BOMLEN:
            if (bom == UTF8_BOM)
            {
                text_mode = __crt_lowio_text_mode::utf8;
                break;
            }

        case UTF16_BOMLEN:
            if((bom & BOM_MASK) == UTF16BE_BOM)
            {
                _ASSERTE(0 && "Only UTF-16 little endian & UTF-8 is supported for reads");
                errno = EINVAL;
                return errno;
            }

            if((bom & BOM_MASK) == UTF16LE_BOM)
            {
                // We have read three bytes, so we should seek back one byte:
                if(_lseeki64_nolock(fh, UTF16_BOMLEN, SEEK_SET) == -1)
                    return errno;

                text_mode = __crt_lowio_text_mode::utf16le;
                break;
            }

            // Fall through to default case to lseek to beginning of file

        default:
            // The file has no BOM, so we seek back to the beginning:
            if (_lseeki64_nolock(fh, 0, SEEK_SET) == -1)
                return errno;

            break;
        }
    }

    if (write_bom)
    {
        // If we are creating a new file, we write a UTF-16LE or UTF8 BOM:
        int bom_length = 0;
        int bom = 0;
        switch (text_mode)
        {
        case __crt_lowio_text_mode::utf16le:
        {
            bom        = UTF16LE_BOM;
            bom_length = UTF16_BOMLEN;
            break;
        }
        case __crt_lowio_text_mode::utf8:
        {
            bom        = UTF8_BOM;
            bom_length = UTF8_BOMLEN;
            break;
        }
        }

        for (int total_written = 0; bom_length > total_written; )
        {
            char const* const bom_begin = reinterpret_cast<char const*>(&bom);

            // Note that the call to write may write less than bom_length
            // characters but not really fail.  We retry until the write fails
            // or we have written all of the characters:
            int const written = _write(fh, bom_begin + total_written, bom_length - total_written);
            if (written == -1)
                return errno;

            total_written += written;
        }
    }

    return 0; // Success!
}



extern "C" errno_t __cdecl _wsopen_nolock(
    int*           const punlock_flag,
    int*           const pfh,
    wchar_t const* const path,
    int            const oflag,
    int            const shflag,
    int            const pmode,
    int            const secure
    )
{
    UNREFERENCED_PARAMETER(secure);

    // First, do the initial parse of the options.  The only thing that can fail
    // here is the parsing of the share options, in which case -1 is returned
    // and errno is set.
    file_options options = decode_options(oflag, shflag, pmode);
    if (options.share == static_cast<DWORD>(-1))
    {
        _doserrno = 0;
        *pfh = -1;
        return errno;
    }

    // Allocate the CRT file handle.  Note that if a handle is allocated, it is
    // locked when it is returned by the allocation function.  It is our caller's
    // responsibility to unlock the file handle (we do not unlock it before
    // returning).
    *pfh = _alloc_osfhnd();
    if (*pfh == -1)
    {
        _doserrno = 0;
        *pfh = -1;
        errno = EMFILE;
        return errno;
    }

    // Beyond this point, do not change *pfh, even if an error occurs.  Our
    // caller requires the handle in order to release its lock.
    *punlock_flag = 1;



    SECURITY_ATTRIBUTES security_attributes;
    security_attributes.nLength = sizeof(security_attributes);
    security_attributes.lpSecurityDescriptor = nullptr;
    security_attributes.bInheritHandle = (oflag & _O_NOINHERIT) == 0;


    // Try to open or create the file:
    HANDLE os_handle = create_file(path, &security_attributes, options);
    if (os_handle == INVALID_HANDLE_VALUE)
    {
        if ((options.access & GENERIC_READ_WRITE) == GENERIC_READ_WRITE && (oflag & _O_WRONLY))
        {
            // The call may have failed because we may be trying to open
            // something for reading that does not allow reading (e.g. a pipe or
            // a device).  So, we try again with just GENERIC_WRITE.  If this
            // succeeds, we will have to assume the default encoding because we
            // will have no way to read the BOM.
            options.access &= ~GENERIC_READ;

            os_handle = create_file(path, &security_attributes, options);
        }
    }

    if (os_handle == INVALID_HANDLE_VALUE)
    {
        // We failed to open the file.  We need to free the CRT file handle, but
        // we do not release the lock--our caller releases the lock.
        _osfile(*pfh) &= ~FOPEN;
        __acrt_errno_map_os_error(GetLastError());
        return errno;
    }

    // Find out what type of file this is (e.g., file, device, pipe, etc.)
    DWORD const file_type = GetFileType(os_handle);

    if (file_type == FILE_TYPE_UNKNOWN)
    {
        DWORD const last_error = GetLastError();
        __acrt_errno_map_os_error(last_error);

        _osfile(*pfh) &= ~FOPEN;
        CloseHandle(os_handle);

        // If GetFileType returns FILE_TYPE_UNKNOWN but doesn't fail, the file
        // type really is unknown.  This function is not designed to handle
        // unknown types of files, so we must return an error.
        if (last_error == ERROR_SUCCESS)
            errno = EACCES;

        return errno;
    }

    if (file_type == FILE_TYPE_CHAR)
    {
        options.crt_flags |= FDEV;
    }
    else if (file_type == FILE_TYPE_PIPE)
    {
        options.crt_flags |= FPIPE;
    }

    // The file is open and valid.  Set the OS handle:
    __acrt_lowio_set_os_handle(*pfh, reinterpret_cast<intptr_t>(os_handle));


    // Mark the handle as open, and store the flags we gathered so far:
    options.crt_flags |= FOPEN;
    _osfile(*pfh) = options.crt_flags;


    // The text mode is set to ANSI by default.  If we find a BOM, then we will
    // reset this to the appropriate type (this check happens below).
    _textmode(*pfh) = __crt_lowio_text_mode::ansi;


    // If the text mode file is opened for writing and allows reading, remove
    // any trailing Ctrl+Z character, if present, to ensure appending works:
    if (oflag & _O_RDWR)
    {
        errno_t const result = truncate_ctrl_z_if_present(*pfh);
        if (result != 0)
        {
            _close_nolock(*pfh);
            return result;
        }
    }

    // Configure the text mode:
    __crt_lowio_text_mode text_mode = __crt_lowio_text_mode::ansi;
    errno_t const text_mode_result = configure_text_mode(*pfh, options, oflag, text_mode);
    if (text_mode_result != 0)
    {
        _close_nolock(*pfh);
        return text_mode_result;
    }

    _textmode(*pfh)   = text_mode;
    _tm_unicode(*pfh) = (oflag & _O_WTEXT) != 0;


    // Set FAPPEND flag if appropriate. Don't do this for devices or pipes:
    if ((options.crt_flags & (FDEV | FPIPE)) == 0 && (oflag & _O_APPEND))
        _osfile(*pfh) |= FAPPEND;


    // Finally, if we were asked only to open the file with write access but we
    // opened it with read and write access in order to read the BOM, close the
    // file and re-open it with only write access:
    if ((options.access & GENERIC_READ_WRITE) == GENERIC_READ_WRITE && (oflag & _O_WRONLY))
    {
        CloseHandle(os_handle);
        options.access &= ~GENERIC_READ;
        os_handle = create_file(path, &security_attributes, options);

        if (os_handle == INVALID_HANDLE_VALUE)
        {
            // Note that we can't use the normal close function here because the
            // file isn't really open anymore.  We need only release the file
            // handle by unsetting the FOPEN flag:
            __acrt_errno_map_os_error(GetLastError());
            _osfile(*pfh) &= ~FOPEN;
            _free_osfhnd(*pfh);
            return errno;
        }
        else
        {
            // We were able to open the file successfully, set the file
            // handle in the _ioinfo structure, then we are done.  All
            // the options.crt_flags should have been set properly already.
            _osfhnd(*pfh) = reinterpret_cast<intptr_t>(os_handle);
        }
    }

    return 0; // Success!
}



extern "C" errno_t __cdecl _sopen_nolock(
    int*        const punlock_flag,
    int*        const pfh,
    char const* const path,
    int         const oflag,
    int         const shflag,
    int         const pmode,
    int         const secure
    )
{
    // At this point we know path is not null already
    __crt_internal_win32_buffer<wchar_t> wide_path;

    errno_t const cvt = __acrt_mbs_to_wcs_cp(path, wide_path, __acrt_get_utf8_acp_compatibility_codepage());

    if (cvt != 0) {
        return -1;
    }

    return _wsopen_nolock(punlock_flag, pfh, wide_path.data(), oflag, shflag, pmode, secure);
}



extern "C" int __cdecl _sopen(char const* const path, int const oflag, int const shflag, ...)
{
    va_list ap;
    va_start(ap, shflag);
    int const pmode = va_arg(ap, int);
    va_end(ap);

    // The last argument is 0 so thta the pmode is not validated in open_s:
    int fh = -1;
    errno_t const result = _sopen_dispatch(path, oflag, shflag, pmode, &fh, FALSE);
    return result ? -1 : fh;
}

extern "C" int __cdecl _wsopen(wchar_t const* const path, int const oflag, int const shflag, ...)
{
    va_list ap;
    va_start(ap, shflag);
    int const pmode = va_arg(ap, int);
    va_end(ap);

    // The last argument is 0 so thta the pmode is not validated in open_s:
    int fh = -1;
    errno_t const result = _wsopen_dispatch(path, oflag, shflag, pmode, &fh, FALSE);
    return result ? -1 : fh;
}



extern "C" errno_t __cdecl _sopen_s(
    int*        const pfh,
    char const* const path,
    int         const oflag,
    int         const shflag,
    int         const pmode
    )
{
    // The last argument is 1 so that pmode is validated in open_s:
    return _sopen_dispatch(path, oflag, shflag, pmode, pfh, TRUE);
}

extern "C" errno_t __cdecl _wsopen_s(
    int*           const pfh,
    wchar_t const* const path,
    int            const oflag,
    int            const shflag,
    int            const pmode
    )
{
    // The last argument is 1 so that pmode is validated in open_s:
    return _wsopen_dispatch(path, oflag, shflag, pmode, pfh, TRUE);
}
