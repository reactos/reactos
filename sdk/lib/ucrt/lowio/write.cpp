//
// write.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _write(), which writes a buffer to a file.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>



namespace
{
    struct write_result
    {
        DWORD error_code;
        DWORD char_count;
        DWORD lf_count;
    };
}



// This is the normal size of the LF => CRLF translation buffer.  The default
// buffer is 4K, plus extra room for LF characters.  Not all buffers are exactly
// this size, but this is used as the base size.
static size_t const BUF_SIZE = 5 * 1024;



// Writes a buffer to a file.  The way in which the buffer is written depends on
// the mode in which the file was opened (e.g., if the file is a text mode file,
// linefeed translation will take place).
//
// On success, this function returns the number of bytes actually written (note
// that "bytes" here is "bytes from the original buffer;" more or fewer bytes
// may have actually been written, due to linefeed translation, codepage
// translation, and other transformations).  On failure, this function returns 0
// and sets errno.
extern "C" int __cdecl _write_internal(int const fh, void const* const buffer, unsigned const size, __crt_cached_ptd_host& ptd)
{
    _UCRT_CHECK_FH_CLEAR_OSSERR_RETURN(ptd, fh, EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (fh >= 0 && (unsigned)fh < (unsigned)_nhandle), EBADF, -1);
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, (_osfile(fh) & FOPEN), EBADF, -1);

    __acrt_lowio_lock_fh(fh);
    int result = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            ptd.get_errno().set(EBADF);
            ptd.get_doserrno().set(0);
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            __leave;
        }

        result = _write_nolock(fh, buffer, size, ptd);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return result;
}

extern "C" int __cdecl _write(int const fh, void const* const buffer, unsigned const size)
{
    __crt_cached_ptd_host ptd;
    return _write_internal(fh, buffer, size, ptd);
}

static bool __cdecl write_requires_double_translation_nolock(int const fh, __crt_cached_ptd_host& ptd) throw()
{
    // Double translation is required if both [a] the current locale is not the C
    // locale or the file is open in a non-ANSI mode and [b] we are writing to the
    // console.

    // If this isn't a TTY or a text mode screen, then it isn't the console:
    if (!_isatty(fh))
    {
        return false;
    }

    if ((_osfile(fh) & FTEXT) == 0) {
        return false;
    }

    // Get the current locale.  If we're in the C locale and the file is open
    // in ANSI mode, we don't need double translation:
    bool const is_c_locale = ptd.get_locale()->locinfo->locale_name[LC_CTYPE] == nullptr;
    if (is_c_locale && _textmode(fh) == __crt_lowio_text_mode::ansi)
    {
        return false;
    }

    // If we can't get the console mode, it's not the console:
    DWORD mode;
    if (!GetConsoleMode(reinterpret_cast<HANDLE>(_osfhnd(fh)), &mode))
    {
        return false;
    }

    // Otherwise, double translation is required:
    return true;
}



static write_result __cdecl write_double_translated_ansi_nolock(
    int                                 const fh,
    _In_reads_(buffer_size) char const* const buffer,
    unsigned                            const buffer_size,
    __crt_cached_ptd_host&                    ptd
    ) throw()
{
    HANDLE      const os_handle  = reinterpret_cast<HANDLE>(_osfhnd(fh));
    char const* const buffer_end = buffer + buffer_size;
    UINT        const console_cp = GetConsoleOutputCP();
    _locale_t   const locale     = ptd.get_locale();
    bool        const is_utf8    = locale->locinfo->_public._locale_lc_codepage == CP_UTF8;

    write_result result = { 0 };

    for (char const* source_it = buffer; source_it < buffer_end; )
    {
        char const c = *source_it;

        // We require double conversion, to convert from the source multibyte
        // to Unicode, then from Unicode back to multibyte, but in the console
        // codepage.
        //
        // Here, we have to take into account that _write() might be called
        // byte-by-byte, so when we see a lead byte without a trail byte, we
        // have to store it and return no error.  When this function is called
        // again, that byte will be combined with the next available character.
        wchar_t wc[2] = { 0 };
        int wc_used = 1;
        if (is_utf8)
        {
            _ASSERTE(!_dbcsBufferUsed(fh));
            const int mb_buf_size = sizeof(_mbBuffer(fh));
            int mb_buf_used;
            for (mb_buf_used = 0; mb_buf_used < mb_buf_size && _mbBuffer(fh)[mb_buf_used]; ++mb_buf_used)
            {}

            if (mb_buf_used > 0)
            {
                const int mb_len = _utf8_no_of_trailbytes(_mbBuffer(fh)[0]) + 1;
                _ASSERTE(1 < mb_len && mb_buf_used < mb_len);
                const int remaining_bytes = mb_len - mb_buf_used;
                if (remaining_bytes <= (buffer_end - source_it))
                {
                    // We now have enough bytes to complete the code point
                    char mb_buffer[MB_LEN_MAX];

                    for (int i = 0; i < mb_buf_used; ++i)
                    {
                        mb_buffer[i] = _mbBuffer(fh)[i];
                    }
                    for (int i = 0; i < remaining_bytes; ++i)
                    {
                        mb_buffer[i + mb_buf_used] = source_it[i];
                    }

                    // Clear out the temp buffer
                    for (int i = 0; i < mb_buf_used; ++i)
                    {
                        _mbBuffer(fh)[i] = 0;
                    }

                    mbstate_t state{};
                    const char* str = mb_buffer;
                    if (mb_len == 4)
                    {
                        wc_used = 2;
                    }
                    if (__crt_mbstring::__mbsrtowcs_utf8(wc, &str, wc_used, &state, ptd) == -1)
                    {
                        return result;
                    }
                    source_it += (remaining_bytes - 1);
                }
                else
                {
                    // Need to add some more bytes to the buffer for later
                    const auto bytes_to_add = buffer_end - source_it;
                    _ASSERTE(mb_buf_used + bytes_to_add < mb_buf_size);
                    for (int i = 0; i < bytes_to_add; ++i)
                    {
                        _mbBuffer(fh)[i + mb_buf_used] = source_it[i];
                    }
                    // Pretend we wrote the bytes, because this isn't an error *yet*.
                    result.char_count += static_cast<DWORD>(bytes_to_add);
                    return result;
                }
            }
            else
            {
                const int mb_len = _utf8_no_of_trailbytes(*source_it) + 1;
                const auto available_bytes = buffer_end - source_it;
                if (mb_len <= (available_bytes))
                {
                    // We have enough bytes to write the entire code point
                    mbstate_t state{};
                    const char* str = source_it;
                    if (mb_len == 4)
                    {
                        wc_used = 2;
                    }
                    if (__crt_mbstring::__mbsrtowcs_utf8(wc, &str, wc_used, &state, ptd) == -1)
                    {
                        return result;
                    }
                    source_it += (mb_len - 1);
                }
                else
                {
                    // Not enough bytes for this code point
                    _ASSERTE(available_bytes <= sizeof(_mbBuffer(fh)));
                    for (int i = 0; i < available_bytes; ++i)
                    {
                        _mbBuffer(fh)[i] = source_it[i];
                    }
                    // Pretend we wrote the bytes, because this isn't an error *yet*.
                    result.char_count += static_cast<DWORD>(available_bytes);
                    return result;
                }
            }
        }
        else if (_dbcsBufferUsed(fh))
        {
            // We already have a DBCS lead byte buffered.  Take the current
            // character, combine it with the lead byte, and convert:
            _ASSERTE(_isleadbyte_fast_internal(_dbcsBuffer(fh), locale));

            char mb_buffer[MB_LEN_MAX];
            mb_buffer[0] = _dbcsBuffer(fh);
            mb_buffer[1] = *source_it;

            _dbcsBufferUsed(fh) = false;

            if (_mbtowc_internal(wc, mb_buffer, 2, ptd) == -1)
            {
                return result;
            }
        }
        else
        {
            if (_isleadbyte_fast_internal(*source_it, locale))
            {
                if ((source_it + 1) < buffer_end)
                {
                    // And we have more bytes to read, just convert...
                    if (_mbtowc_internal(wc, source_it, 2, ptd) == -1)
                    {
                        return result;
                    }

                    // Increment the source_it to accomodate the DBCS character:
                    ++source_it;
                }
                else
                {
                    // And we ran out of bytes to read, so buffer the lead byte:
                    _dbcsBuffer(fh) = *source_it;
                    _dbcsBufferUsed(fh) = true;

                    // We lie here that we actually wrote the last character, to
                    // ensure we don't consider this an error:
                    ++result.char_count;
                    return result;
                }
            }
            else
            {
                // single character conversion:
                if (_mbtowc_internal(wc, source_it, 1, ptd) == -1)
                {
                    return result;
                }
            }
        }

        ++source_it;

        // Translate the Unicode character into Multibyte in the console codepage
        // and write the character to the file:
        char mb_buffer[MB_LEN_MAX];
        DWORD const size = static_cast<DWORD>(__acrt_WideCharToMultiByte(
            console_cp, 0, wc, wc_used, mb_buffer, sizeof(mb_buffer), nullptr, nullptr));

        if(size == 0)
            return result;

        DWORD written;
        if (!WriteFile(os_handle, mb_buffer, size, &written, nullptr))
        {
            result.error_code = GetLastError();
            return result;
        }

        // When we are converting, some conversions may result in:
        //
        //     2 MBCS characters => 1 wide character => 1 MBCS character.
        //
        // For example, when printing Japanese characters in the English console
        // codepage, each source character is transformed into a single question
        // mark.  Therefore, we want to track the number of bytes we converted,
        // plus the linefeed count, instead of how many bytes we actually wrote.
        result.char_count = result.lf_count + static_cast<DWORD>(source_it - buffer);

        // If the write succeeded but didn't write all of the characters, return:
        if (written < size)
        {
            return result;
        }

        // If the original character that we read was an LF, write a CR too:
        // CRT_REFACTOR TODO Doesn't this write LFCR instead of CRLF?
        if (c == LF)
        {
            wchar_t const cr = CR;
            if (!WriteFile(os_handle, &cr, 1, &written, nullptr))
            {
                result.error_code = GetLastError();
                return result;
            }

            if (written < 1)
            {
                return result;
            }

            ++result.lf_count;
            ++result.char_count;
        }
    }

    return result;
}



static write_result __cdecl write_double_translated_unicode_nolock(
    _In_reads_(buffer_size)                      char const* const buffer,
    _In_ _Pre_satisfies_((buffer_size % 2) == 0) unsigned    const buffer_size
    ) throw()
{
    // When writing to a Unicode file (UTF-8 or UTF-16LE) that corresponds to
    // the console, we don't actually need double translation.  We just need to
    // print each character to the console, one-by-one.  (This function is
    // named what it is because its use is guarded by the double translation
    // check, and to match the name of the corresponding ANSI function.)

    write_result result = { 0 };

    // Needed for SAL to clarify that buffer_size is even.
    _Analysis_assume_((buffer_size/2) != ((buffer_size-1)/2));
    char const* const buffer_end = buffer + buffer_size;
    for (char const* pch = buffer; pch < buffer_end; pch += 2)
    {
        wchar_t const c = *reinterpret_cast<wchar_t const*>(pch);

        // _putwch_nolock does not depend on global state, no PTD needed to be propagated.
        if (_putwch_nolock(c) == c)
        {
            result.char_count += 2;
        }
        else
        {
            result.error_code = GetLastError();
            return result;
        }

        // If the character was a carriage return, also emit a line feed.
        // CRT_REFACTOR TODO Doesn't this print LFCR instead of CRLF?
        if (c == LF)
        {
            // _putwch_nolock does not depend on global state, no PTD needed to be propagated.
            if (_putwch_nolock(CR) != CR)
            {
                result.error_code = GetLastError();
                return result;
            }

            ++result.char_count;
            ++result.lf_count;
        }
    }

    return result;
}



static write_result __cdecl write_text_ansi_nolock(
    int                                 const fh,
    _In_reads_(buffer_size) char const* const buffer,
    unsigned                            const buffer_size
    ) throw()
{
    HANDLE      const os_handle  = reinterpret_cast<HANDLE>(_osfhnd(fh));
    char const* const buffer_end = buffer + buffer_size;

    write_result result = { 0 };

    for (char const* source_it = buffer; source_it < buffer_end; )
    {
        char lfbuf[BUF_SIZE]; // The LF => CRLF translation buffer

        // One-past-the-end of the translation buffer.  Note that we subtract
        // one to account for the case where we're pointing to the last element
        // in the buffer and we need to write both a CR and an LF.
        char* const lfbuf_end = lfbuf + sizeof(lfbuf) - 1;

        // Translate the source buffer into the translation buffer.  Note that
        // both source_it and lfbuf_it are incremented in the loop.
        char* lfbuf_it = lfbuf;
        while (lfbuf_it < lfbuf_end && source_it < buffer_end)
        {
            char const c = *source_it++;

            if (c == LF)
            {
                ++result.lf_count;
                *lfbuf_it++ = CR;
            }

            *lfbuf_it++ = c;
        }

        DWORD const lfbuf_length = static_cast<DWORD>(lfbuf_it - lfbuf);

        DWORD written;
        if (!WriteFile(os_handle, lfbuf, lfbuf_length, &written, nullptr))
        {
            result.error_code = GetLastError();
            return result;
        }

        result.char_count += written;
        if (written < lfbuf_length)
        {
            return result; // The write succeeded but didn't write everything
        }
    }

    return result;
}



static write_result __cdecl write_text_utf16le_nolock(
    int                                 const fh,
    _In_reads_(buffer_size) char const* const buffer,
    unsigned                            const buffer_size
    ) throw()
{
    HANDLE         const os_handle  = reinterpret_cast<HANDLE>(_osfhnd(fh));
    wchar_t const* const buffer_end = reinterpret_cast<wchar_t const*>(buffer + buffer_size);

    write_result result = { 0 };

    wchar_t const* source_it = reinterpret_cast<wchar_t const*>(buffer);
    while (source_it < buffer_end)
    {
        wchar_t lfbuf[BUF_SIZE / sizeof(wchar_t)]; // The translation buffer

        // One-past-the-end of the translation buffer.  Note that we subtract
        // one to account for the case where we're pointing to the last element
        // in the buffer and we need to write both a CR and an LF.
        wchar_t const* lfbuf_end = lfbuf + BUF_SIZE / sizeof(wchar_t) - 1;

        // Translate the source buffer into the translation buffer.  Note that
        // both source_it and lfbuf_it are incremented in the loop.
        wchar_t* lfbuf_it = lfbuf;
        while (lfbuf_it < lfbuf_end && source_it < buffer_end)
        {
            wchar_t const c = *source_it++;

            if (c == LF)
            {
                result.lf_count += 2;
                *lfbuf_it++ = CR;
            }

            *lfbuf_it++ = c;
        }

        // Note that this length is in bytes, not wchar_t elemnts, since we need
        // to tell WriteFile how many bytes (not characters) to write:
        DWORD const lfbuf_length = static_cast<DWORD>(lfbuf_it - lfbuf) * sizeof(wchar_t);


        // Attempt the write and return immediately if it fails:
        DWORD written;
        if (!WriteFile(os_handle, lfbuf, lfbuf_length, &written, nullptr))
        {
            result.error_code = GetLastError();
            return result;
        }

        result.char_count += written;
        if (written < lfbuf_length)
        {
            return result; // The write succeeded, but didn't write everything
        }
    }

    return result;
}



static write_result __cdecl write_text_utf8_nolock(
    int                                 const fh,
    _In_reads_(buffer_size) char const* const buffer,
    unsigned                            const buffer_size
    ) throw()
{
    HANDLE         const os_handle  = reinterpret_cast<HANDLE>(_osfhnd(fh));
    wchar_t const* const buffer_end = reinterpret_cast<wchar_t const*>(buffer + buffer_size);

    write_result result = { 0 };

    wchar_t const* source_it = reinterpret_cast<wchar_t const*>(buffer);
    while (source_it < buffer_end)
    {
        // The translation buffer.  We use two buffers:  the first is used to
        // store the UTF-16 LF => CRLF translation (this is that buffer here).
        // The second is used for storing the conversion to UTF-8 (defined
        // below).  The sizes are selected to handle the worst-case scenario
        // where each UTF-8 character is four bytes long.
        wchar_t utf16_buf[BUF_SIZE / 6];

        // One-past-the-end of the translation buffer.  Note that we subtract
        // one to account for the case where we're pointing to the last element
        // in the buffer and we need to write both a CR and an LF.
        wchar_t const* utf16_buf_end = utf16_buf + (BUF_SIZE / 6 - 1);

        // Translate the source buffer into the translation buffer.  Note that
        // both source_it and lfbuf_it are incremented in the loop.
        wchar_t* utf16_buf_it = utf16_buf;
        while (utf16_buf_it < utf16_buf_end && source_it < buffer_end)
        {
            wchar_t const c = *source_it++;

            if (c == LF)
            {
                // No need to count the number of line-feeds translated; we
                // track the number of written characters by counting the total
                // number of characters written from the UTF8 buffer (see below
                // where we update the char_count).
                *utf16_buf_it++ = CR;
            }

            *utf16_buf_it++ = c;
        }

        // Note that this length is in characters, not bytes.
        DWORD const utf16_buf_length = static_cast<DWORD>(utf16_buf_it - utf16_buf);


        // This is the second translation, where we translate the UTF-16 text to
        // UTF-8, into the UTF-8 buffer:
        char utf8_buf[(BUF_SIZE * 2) / 3];
        DWORD const bytes_converted = static_cast<DWORD>(__acrt_WideCharToMultiByte(
                CP_UTF8,
                0,
                utf16_buf,
                utf16_buf_length,
                utf8_buf,
                sizeof(utf8_buf),
                nullptr,
                nullptr));

        if (bytes_converted == 0)
        {
            result.error_code = GetLastError();
            return result;
        }

        // Here, we need to make every attempt to write all of the converted
        // characters to avoid corrupting the stream.  If, for example, we write
        // only half of the bytes of a UTF-8 character, the stream may be
        // corrupted.
        //
        // This loop will ensure that we exit only if either (a) all of the
        // bytes are written, ensuring that no partial MBCSes are written, or
        // (b) there is an error in the stream.
        for (DWORD bytes_written = 0; bytes_written < bytes_converted; )
        {
            char const* const current      = utf8_buf + bytes_written;
            DWORD       const current_size = bytes_converted - bytes_written;

            DWORD written;
            if (!WriteFile(os_handle, current, current_size, &written, nullptr))
            {
                result.error_code = GetLastError();
                return result;
            }

            bytes_written += written;
        }

        // If this chunk was committed successfully, update the character count:
        result.char_count = static_cast<DWORD>(reinterpret_cast<char const*>(source_it) - buffer);
    }

    return result;
}



static write_result __cdecl write_binary_nolock(
    int                                 const fh,
    _In_reads_(buffer_size) char const* const buffer,
    unsigned                            const buffer_size
    ) throw()
{
    HANDLE const os_handle = reinterpret_cast<HANDLE>(_osfhnd(fh));

    // Compared to text files, binary files are easy...
    write_result result = { 0 };
    if (!WriteFile(os_handle, buffer, buffer_size, &result.char_count, nullptr))
    {
        result.error_code = GetLastError();
    }

    return result;
}



extern "C" int __cdecl _write_nolock(int const fh, void const* const buffer, unsigned const buffer_size, __crt_cached_ptd_host& ptd)
{
    // If the buffer is empty, there is nothing to be written:
    if (buffer_size == 0)
    {
        return 0;
    }

    // If the buffer is null, though... well, that is not allowed:
    _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, buffer != nullptr, EINVAL, -1);

    __crt_lowio_text_mode const fh_textmode = _textmode(fh);

    // If the file is open for Unicode, the buffer size must always be even:
    if (fh_textmode == __crt_lowio_text_mode::utf16le || fh_textmode == __crt_lowio_text_mode::utf8)
    {
        _UCRT_VALIDATE_CLEAR_OSSERR_RETURN(ptd, buffer_size % 2 == 0, EINVAL, -1);
    }

    // If the file is opened for appending, seek to the end of the file.  We
    // ignore errors because the underlying file may not allow seeking.
    if (_osfile(fh) & FAPPEND)
    {
        (void)_lseeki64_nolock_internal(fh, 0, FILE_END, ptd);
    }

    char const* const char_buffer = static_cast<char const*>(buffer);

    // Dispatch the actual writing to one of the helper routines based on the
    // text mode of the file and whether or not the file refers to the console.
    //
    // Note that in the event that the handle belongs to the console, WriteFile
    // will generate garbage output.  To print to the console correctly, we need
    // to print ANSI.  Also note that when printing to the console, we need to
    // convert the characters to the console codepge.
    write_result result = { 0 };
    if (write_requires_double_translation_nolock(fh, ptd))
    {
        switch (fh_textmode)
        {
        case __crt_lowio_text_mode::ansi:
            result = write_double_translated_ansi_nolock(fh, char_buffer, buffer_size, ptd);
            break;

        case __crt_lowio_text_mode::utf16le:
        case __crt_lowio_text_mode::utf8:
            _Analysis_assume_((buffer_size % 2) == 0);
            result = write_double_translated_unicode_nolock(char_buffer, buffer_size);
            break;
        }
    }
    else if (_osfile(fh) & FTEXT)
    {
        switch (fh_textmode)
        {
        case __crt_lowio_text_mode::ansi:
            result = write_text_ansi_nolock(fh, char_buffer, buffer_size);
            break;

        case __crt_lowio_text_mode::utf16le:
            result = write_text_utf16le_nolock(fh, char_buffer, buffer_size);
            break;

        case __crt_lowio_text_mode::utf8:
            result = write_text_utf8_nolock(fh, char_buffer, buffer_size);
            break;
        }
    }
    else
    {
        result = write_binary_nolock(fh, char_buffer, buffer_size);
    }


    // Why did we not write anything?  Lettuce find out...
    if (result.char_count == 0)
    {
        // If nothing was written, check to see if it was due to an OS error:
        if (result.error_code != 0)
        {
            // An OS error occurred.  ERROR_ACCESS_DENIED should be mapped in
            // this case to EBADF, not EACCES.  All other errors are mapped
            // normally:
            if (result.error_code == ERROR_ACCESS_DENIED)
            {
                ptd.get_errno().set(EBADF);
                ptd.get_doserrno().set(result.error_code);
            }
            else
            {
                __acrt_errno_map_os_error_ptd(result.error_code, ptd);
            }

            return -1;
        }

        // If this file is a device and the first character was Ctrl+Z, then
        // writing nothing is the expected behavior and is not an error:
        if ((_osfile(fh) & FDEV) && *char_buffer == CTRLZ)
        {
            return 0;
        }

        // Otherwise, the error is reported as ENOSPC:
        ptd.get_errno().set(ENOSPC);
        ptd.get_doserrno().set(0);
        return -1;
    }

    // The write succeeded.  Return the adjusted number of bytes written:
    return result.char_count - result.lf_count;
}
