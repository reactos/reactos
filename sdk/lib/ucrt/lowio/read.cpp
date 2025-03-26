//
// read.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _read(), which reads bytes from a file.
//
#include <corecrt_internal_lowio.h>

// Lookup table for UTF-8 lead bytes
// Probably preferable to just ask if the bits are set than use an entire
// table, however the macros using this were #defined in the header so
// removing this extern table would break apps compiled to an earlier verison.
//    1 for pattern 110xxxxx - 1 trailbyte
//    2 for pattern 1110xxxx - 2 trailbytes
//    3 for pattern 11110xxx - 3 trailbytes
//    0 for everything else, including invalid patterns.
// We return 0 for invalid patterns because we rely on MultiByteToWideChar to
// do the validations.
extern "C" { char _lookuptrailbytes[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0
}; }


static void store_lookahead(int const fh, char const c) throw()
{
    _pipe_lookahead(fh)[0] = c;
}

static void store_lookahead(int const fh, wchar_t const c) throw()
{
    char const* const byte_pointer = reinterpret_cast<char const*>(&c);
    _pipe_lookahead(fh)[0] = byte_pointer[0];
    _pipe_lookahead(fh)[1] = byte_pointer[1];
    _pipe_lookahead(fh)[2] = LF; // Mark as empty
}



static int __cdecl translate_utf16_from_console_nolock(
                            int      const  fh,
    _Inout_updates_(count)  wchar_t* const  buffer,
                            size_t   const  count
    ) throw()
{
    // The translation can be performend in-place, because we are converting
    // CRLF sequences into LF, so the resulting text will never be longer than
    // any corresponding source text.
    wchar_t* const buffer_end = buffer + count;

    wchar_t* source_it = buffer;
    wchar_t* result_it = buffer;

    while (source_it < buffer_end)
    {
        // If at any point during translation we encounter a Ctrl+Z, we stop
        // translating immediately:
        if (*source_it == CTRLZ)
        {
            _osfile(fh) |= FEOFLAG;
            break;
        }

        // When a CR character is encountered, we must check to see if the next
        // character is an LF.  If it is, then we skip the CR and copy only the
        // LF:
        if (*source_it == CR && source_it + 1 < buffer_end && *(source_it + 1) == LF)
        {
            source_it += 2;
            *result_it++ = LF;
            continue;
        }

        // Otherwise, we just copy the character:
        *result_it++ = *source_it++;
    }

    // Return the number of bytes that we translated:
    return static_cast<int>((result_it - buffer) * sizeof(wchar_t));
}



template <typename Character>
static int __cdecl translate_text_mode_nolock(
    _In_                                                         int        const fh,
    _Pre_writable_size_(count) _Post_readable_byte_size_(return) Character* const buffer,
    _In_                                                         size_t     const count
    ) throw()
{
    HANDLE const os_handle = reinterpret_cast<HANDLE>(_osfhnd(fh));

    // If there is an LF at the beginning of the buffer, set the CRLF flag:
    if (count != 0 && *buffer == LF)
    {
        _osfile(fh) |= FCRLF;
    }
    else
    {
        _osfile(fh) &= ~FCRLF;
    }

    // The translation can be performend in-place, because we are converting
    // CRLF sequences into LF, so the resulting text will never be longer than
    // any corresponding source text.
    Character* const buffer_end = buffer + count;

    Character* source_it = buffer;
    Character* result_it = buffer;

    while (source_it < buffer_end)
    {
        // If during translation we encounter a Ctrl+Z, we stop translating
        // immeidately.  For devices, we need to just set the Ctrl+Z flag;
        // for other files, we just copy the Ctrl+Z as a normal character
        // before returning:
        if (*source_it == CTRLZ)
        {
            if ((_osfile(fh) & FDEV) == 0)
            {
                _osfile(fh) |= FEOFLAG;
            }
            else
            {
                *result_it++ = *source_it++;
            }

            break;
        }

        // If the character is not a CR, then we can simply copy it:
        if (*source_it != CR)
        {
            *result_it++ = *source_it++;
            continue;
        }

        // Otherwise, the character is a CR.  We need to look-ahead to see if
        // the next character is an LF, so that we can perform the CRLF => LF
        // translation.  First, handle the easy case where the CR does not
        // appear at the end of the buffer:
        if (source_it + 1 < buffer_end)
        {
            if (*(source_it + 1) == LF)
            {
                source_it += 2;
                *result_it++ = LF; // Convert CRLF => LF
            }
            else
            {
                *result_it++ = *source_it++;
            }

            continue;
        }

        // This is the hard case:  The CR is at the end of the buffer.  We need
        // to peek ahead to see if the next character is an LF:
        ++source_it;

        Character peek;
        DWORD     peek_size;
        if (!ReadFile(os_handle, &peek, sizeof(peek), &peek_size, nullptr) || peek_size == 0)
        {
            // We couldn't peek ahead; just store the CR:
            *result_it++ = CR;
            continue;
        }

        // The peek succeeded.  What we do next depends on whether the file is
        // seekable or not.  First we handle the case where the file does not
        // allow seeking:
        if (_osfile(fh) & (FDEV | FPIPE))
        {
            // If the peek character is an LF, then we just need to copy that
            // character to the output buffer:
            if (peek == LF)
            {
                *result_it++ = LF;
            }
            // Otherwise, it was some other character.  We need to write the CR
            // to the output buffer, then we need to store the peek character
            // for later retrieval:
            else
            {
                *result_it++ = CR;
                store_lookahead(fh, peek);
            }
        }
        // If the file does allow seeking, then we handle the peek differently.
        // For seekable files, we translate the CRLF => LF by eliminating the
        // CR.  If the peek character is an LF, we simply do not write it to
        // the output buffer; instead, we will seek backwards to unpeek the
        // character, then let the LF get retrieved during the next call to
        // read().
        else
        {
            // However, if the buffer is currenty empty, then this is a one-
            // character read, so we store the LF in order that we make progress
            if (peek == LF && result_it == buffer)
            {
                *result_it++ = LF;
            }
            // Otherwise, we do what is described above:  we seek backwards and
            // write the CR if and only if the peek character was not an LF:
            else
            {
                _lseeki64_nolock(fh, -1 * static_cast<int>(sizeof(Character)), FILE_CURRENT);
                if (peek != LF)
                {
                    *result_it++ = CR;
                }
            }
        }
    }

    // Return the number of bytes that we translated:
    return static_cast<int>((result_it - buffer) * sizeof(Character));
}



_Success_(return != -1)
static int __cdecl translate_ansi_or_utf8_nolock(
                                                                        int      const fh,
    _In_reads_(source_count)                                            char*    const source_buffer,
                                                                        size_t   const source_count,
    _Pre_writable_size_(result_count) _Post_readable_byte_size_(return) wchar_t* const result_buffer,
                                                                        size_t   const result_count
    ) throw()
{
    int const text_mode_translation_result_size = translate_text_mode_nolock(fh, source_buffer, source_count);

    // If we read no characters, then we are done:
    if (text_mode_translation_result_size == 0)
    {
        return 0;
    }

    // If the file is open in ANSI mode, then no further translation is
    // required; we can simply return the number of bytes that we read.
    // Even though there is no translation, there may still be
    // characters in the buffer due to CRLF translation (a CR without
    // a LF would 'unget' the would-be LF).
    // text_mode_translation_result_size has already been adjusted for
    // CRLF translation by translate_text_mode_nolock().
    if (_textmode(fh) == __crt_lowio_text_mode::ansi)
    {
        return text_mode_translation_result_size;
    }

    // Otherwise, the file is open in UTF-8 mode and we read a nonzero number
    // of characters.  We need to translate from UTF-8 to UTF-16.  To do this,
    // we first need to hunt for the end of the translatable buffer.  This may
    // not be result_it, because we may have read a partial multibyte UTF-8
    // character.
    char* result_it = source_buffer + text_mode_translation_result_size - 1;

    // If the last character is an independent character, then we can
    // translate the entire buffer:
    if (_utf8_is_independent(*result_it))
    {
        ++result_it; // Reset the result_it
    }
    // Otherwise, we have to find the end of the last full UTF-8 character
    // that was read:
    else
    {
        // Walk backwards from the end of the buffer until we find a lead byte:
        unsigned counter = 1;
        while (!_utf8_is_leadbyte(*result_it) && counter <= 4 && result_it >= source_buffer)
        {
            --result_it;
            ++counter;
        }

        // Now that we've found the last lead byte, determine whether the
        // character is complete or incomplete.  We compute the number of
        // trailbytes...
        unsigned const trailbyte_count = _utf8_no_of_trailbytes(static_cast<const unsigned char>(*result_it));
        if (trailbyte_count == 0)
        {
            // Oh, apparently that wasn't a lead byte; the file contains invalid
            // UTF-8 character sequences:
            errno = EILSEQ;
            return -1;
        }

        // If the lead byte plus the remaining bytes form a full set, then we
        // can translate the entire buffer:
        if (trailbyte_count + 1 == counter)
        {
            result_it += counter;
        }
        // Otherwise, the last character is incomplete, so we will not include
        // this character in the result.  We unget the last characters, either
        // by seeking backwards if the file is seekable, or by buffering the
        // characters.  Note that result_it currently points one-past-the-end
        // of the translatable buffer, because it points to the lead byte of
        // the partially read character.
        else
        {
            // If the file does not support seeking, buffer the characters:
            if (_osfile(fh) & (FDEV | FPIPE))
            {
                _pipe_lookahead(fh)[0] = *result_it++;

                if (counter >= 2)
                {
                    _pipe_lookahead(fh)[1] = *result_it++;
                }

                if (counter == 3)
                {
                    _pipe_lookahead(fh)[2] = *result_it++;
                }

                // Now that we've buffered the characters, seek the end iterator
                // back to the actual end of the translatable sequence:
                result_it -= counter;

            }
            // If the file does support seeking, we can just seek backwards so
            // that the next read will get the characters directly:
            else
            {
                _lseeki64_nolock(fh, -static_cast<int>(counter), FILE_CURRENT);
            }
        }
    }

    // Finally, we can translate the characters into the result buffer:
    int const characters_translated = static_cast<int>(__acrt_MultiByteToWideChar(
            CP_UTF8,
            0,
            source_buffer,
            static_cast<DWORD>(result_it - source_buffer),
            result_buffer,
            static_cast<DWORD>(result_count)));

    if (characters_translated == 0)
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    _utf8translations(fh) = (characters_translated != static_cast<int>(result_it - source_buffer));

    // MultiByteToWideChar returns the number of wide characters that
    // it produced; we need to return the number of bytes:
    return characters_translated * sizeof(wchar_t);
}



// Reads bytes from a file.  This function attempts to read enough bytes to fill
// the provided buffer.  If the file is in text mode, CRLF sequences are mapped
// to LF, thus affecting the number of characters read.  This mapping does not
// affect the file pointer.
//
// Returns the number of bytes read, which may be less than the number of bytes
// requested if EOF was reached or if the file is in text mode.  Returns -1 and
// sets errno on failure.
extern "C" int __cdecl _read(int const fh, void* const buffer, unsigned const buffer_size)
{
    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(buffer_size <= INT_MAX, EINVAL, -1);

    __acrt_lowio_lock_fh(fh);
    int result = -1;
    __try
    {
        if ((_osfile(fh) & FOPEN) == 0)
        {
            errno = EBADF;
            _doserrno = 0;
            _ASSERTE(("Invalid file descriptor. File possibly closed by a different thread",0));
            __leave;
        }

        result = _read_nolock(fh, buffer, buffer_size);
    }
    __finally
    {
        __acrt_lowio_unlock_fh(fh);
    }
    __endtry
    return result;
}



extern "C" int __cdecl _read_nolock(
    int      const fh,
    void*    const result_buffer,
    unsigned const result_buffer_size
    )
{
    _CHECK_FH_CLEAR_OSSERR_RETURN(fh, EBADF, -1 );
    _VALIDATE_CLEAR_OSSERR_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(_osfile(fh) & FOPEN, EBADF, -1);
    _VALIDATE_CLEAR_OSSERR_RETURN(result_buffer_size <= INT_MAX, EINVAL, -1);

    // If there is no data to be written or if the file is at EOF, no work to do:
    if (result_buffer_size == 0 || (_osfile(fh) & FEOFLAG))
        return 0;

    _VALIDATE_CLEAR_OSSERR_RETURN(result_buffer != nullptr, EINVAL, -1);


    HANDLE const os_handle = reinterpret_cast<HANDLE>(_osfhnd(fh));
    __crt_lowio_text_mode const text_mode = _textmode(fh);


    __crt_unique_heap_ptr<char> owned_internal_buffer;

    char*    internal_buffer;
    unsigned internal_buffer_remaining;
    switch (text_mode)
    {
    case __crt_lowio_text_mode::utf8:
        // For UTF-8 files, we need two buffers, because after reading we need
        // to convert the text into Unicode.  MultiByteToWideChar doesn't do
        // in-place conversions.
        //
        // The multibyte to wide character conversion may double the size of the
        // text, hence we halve the size here.
        //
        // Since we are reading a UTF-8 stream, the number of bytes read may
        // vary from 'size' characters to 'size/4' characters.  For this reason,
        // if we need to read 'size' characters, we will allocate an MBCS buffer
        // of size 'size'.  In case the size is zero, we will use four as a
        // minimum value.  This will make sure we don't overflow when we read
        // from a pipe.
        //
        // In this case, the number of wide characters that we can read is
        // size / 2.  This means that we require a buffer of size size / 2.

        // For UTF-8 the count always needs to be an even number:
        _VALIDATE_CLEAR_OSSERR_RETURN(result_buffer_size % 2 == 0, EINVAL, -1);

        internal_buffer_remaining = (result_buffer_size / 2) < 4
            ? 4
            : (result_buffer_size/2);

        owned_internal_buffer = _malloc_crt_t(char, internal_buffer_remaining);
        internal_buffer = owned_internal_buffer.get();
        if (!internal_buffer)
        {
            errno = ENOMEM;
            _doserrno = ERROR_NOT_ENOUGH_MEMORY;
            return -1;
        }

        _startpos(fh) = _lseeki64_nolock(fh, 0, FILE_CURRENT);
        break;

    case __crt_lowio_text_mode::utf16le:
        // For UTF-16 the count always needs to be an even number:
        _VALIDATE_CLEAR_OSSERR_RETURN((result_buffer_size % 2) == 0, EINVAL, -1);

        // For UTF-16 files, we can directly use the input buffer:
        internal_buffer_remaining = result_buffer_size;
        internal_buffer           = static_cast<char*>(result_buffer);
        break;

    default:
        // For ANSI files, we can directly use the input buffer:
        internal_buffer_remaining = result_buffer_size;
        internal_buffer           = static_cast<char*>(result_buffer);
        break;
    }

    wchar_t* wide_internal_buffer = reinterpret_cast<wchar_t*>(internal_buffer);

    int bytes_read = 0;

    // We may have buffered look-ahead characters during the last read.  If
    // so, read them into the buffer and set the look-ahead buffers back to
    // empty state (with the value of LF):
    //
    // CRT_REFACTOR This look-ahead buffering could use additional work, but
    // will require nonlocal changes, so that work is not included in this
    // changeset.
    if ((_osfile(fh) & (FPIPE | FDEV)) &&
        _pipe_lookahead(fh)[0] != LF &&
        internal_buffer_remaining != 0)
    {
        *internal_buffer++ = _pipe_lookahead(fh)[0];
        ++bytes_read;
        --internal_buffer_remaining;
        _pipe_lookahead(fh)[0] = LF;

        // For UTF-16, there may be an additional look-ahead character
        // bufferred.  For UTF-8, there may be two more:
        if (text_mode != __crt_lowio_text_mode::ansi &&
            _pipe_lookahead(fh)[1] != LF &&
            internal_buffer_remaining != 0)
        {
            *internal_buffer++ = _pipe_lookahead(fh)[1];
            ++bytes_read;
            --internal_buffer_remaining;
            _pipe_lookahead(fh)[1] = LF;

            if (text_mode == __crt_lowio_text_mode::utf8 &&
                _pipe_lookahead(fh)[2] != LF &&
                internal_buffer_remaining != 0)
            {
                *internal_buffer++ = _pipe_lookahead(fh)[2];
                ++bytes_read;
                --internal_buffer_remaining;
                _pipe_lookahead(fh)[2] = LF;
            }
        }
    }

    DWORD console_mode;
    bool const from_console =
        _isatty(fh) &&
        (_osfile(fh) & FTEXT) &&
        GetConsoleMode(os_handle, &console_mode);

    // Read the data directly from the console:
    if (from_console && text_mode == __crt_lowio_text_mode::utf16le)
    {
        DWORD console_characters_read;
        if (!ReadConsoleW(
                os_handle,
                internal_buffer,
                internal_buffer_remaining / sizeof(wchar_t),
                &console_characters_read,
                nullptr))
        {
            __acrt_errno_map_os_error(GetLastError());
            return -1;
        }

        // In UTF-16 mode, the return value is the actual number of wide
        // characters read; we need the number of bytes:
        bytes_read += console_characters_read * sizeof(wchar_t);
    }
    // Otherwise, read the data from the file normally:
    else
    {
        DWORD bytes_read_from_file;
        if (!ReadFile(
                os_handle,
                internal_buffer,
                internal_buffer_remaining,
                &bytes_read_from_file,
                nullptr
            ) || bytes_read_from_file > result_buffer_size)
        {
            DWORD const last_error = GetLastError();
            if (last_error == ERROR_ACCESS_DENIED)
            {
                // ERROR_ACCESS_DENIED occurs if the file is open with the wrong
                // read/write mode.  For this error, we should return EBADF, not
                // the EACCES that will be set by __acrt_errno_map_os_error:
                errno = EBADF;
                _doserrno = last_error;
                return -1;

            }
            else if (last_error == ERROR_BROKEN_PIPE)
            {
                // Return 0 if ERROR_BROKEN_PIPE occurs.  It means the handle is
                // a read handle on a pipe for which all write handles have been
                // closed and all data has been read:
                return 0;
            }
            else
            {
                // Otherwise, map the error normally and return:
                __acrt_errno_map_os_error(last_error);
                return -1;
            }
        }

        bytes_read += bytes_read_from_file;
    }


    // If the file is open in binary mode, no translation is required, so we
    // can skip all of the rest of this function:
    if ((_osfile(fh) & FTEXT) == 0)
        return bytes_read;


    // Perform the CRLF => LF translation and convert to the required
    // encoding (UTF-8 must be converted to UTF-16).  This first case
    // handles UTF-8 and ANSI:
    if (text_mode != __crt_lowio_text_mode::utf16le)
    {
        return translate_ansi_or_utf8_nolock(
            fh,
            internal_buffer,
            bytes_read,
            static_cast<wchar_t*>(result_buffer),
            result_buffer_size / sizeof(wchar_t));
    }

    // The text mode is __crt_lowio_text_mode::utf16le and we are reading from the
    // console:
    else if (from_console)
    {
        return translate_utf16_from_console_nolock(
            fh,
            wide_internal_buffer,
            bytes_read / sizeof(wchar_t));
    }
    // Otherwise, the text mode is __crt_lowio_text_mode::utf16le and we are NOT
    // reading from the console:
    else
    {
        return translate_text_mode_nolock(
            fh,
            wide_internal_buffer,
            bytes_read / sizeof(wchar_t));
    }
}
