//
// getwch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _getwch(), _getwche(), and _ungetwch(), which get and unget
// characters directly from the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>



namespace
{
    struct CharPair
    {
        unsigned char LeadChar;
        unsigned char SecondChar;
    };
}


static wint_t wchbuf = WEOF; // The push-back buffer

extern "C" intptr_t __dcrt_lowio_console_input_handle;

extern "C" CharPair const* __cdecl _getextendedkeycode(KEY_EVENT_RECORD*);



// Reads one character directly from the console.  The _getwche() form also
// echoes the character back to the console.  If the push-back buffer is
// nonempty, then its value is returned and the push-back buffer is marked as
// empty.
//
// Returns the character that is read on success; returns WEOF on failure.
extern "C" wint_t __cdecl _getwch()
{
    __acrt_lock(__acrt_conio_lock);
    wint_t result = 0;
    __try
    {
        result = _getwch_nolock();
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}


extern "C" wint_t __cdecl _getwche()
{
    __acrt_lock(__acrt_conio_lock);
    wint_t result = 0;
    __try
    {
        result = _getwche_nolock();
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}



extern "C" wint_t __cdecl _getwch_nolock()
{
    // First check the pushback buffer for a character.  If it has one, return
    // it without echoing and reset the buffer:
    if (wchbuf != WEOF)
    {
        wchar_t const buffered_wchar = static_cast<wchar_t>(wchbuf & 0xFFFF);
        wchbuf = WEOF;
        return buffered_wchar;
    }

    // The console input handle is created the first time that _getwch(),
    // _cgetws(), or _kbhit() is called:
    if (__dcrt_lowio_ensure_console_input_initialized() == FALSE)
        return WEOF;

    // Switch to raw mode (no line input, no echo input):
    DWORD old_console_mode;
    __dcrt_get_input_console_mode(&old_console_mode);
    __dcrt_set_input_console_mode(0);
    wint_t result = 0;
    __try
    {
        for ( ; ; )
        {
            // Get a console input event:
            INPUT_RECORD input_record;
            DWORD num_read;
            if (__dcrt_read_console_input(&input_record, 1, &num_read) == FALSE)
            {
                result = WEOF;
                __leave;
            }

            if (num_read == 0)
            {
                result = WEOF;
                __leave;
            }

            // Look for, and decipher, key events.
            if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
            {
                // Easy case:  if UnicodeChar is non-zero, we can just return it:
                wchar_t const c = static_cast<wchar_t>(input_record.Event.KeyEvent.uChar.UnicodeChar);
                if (c != 0)
                {
                    result = c;
                    __leave;
                }

                // Hard case:  either it is an extended code or an event which
                // should not be recognized.  Let _getextendedkeycode do the work:
                CharPair const* const cp = _getextendedkeycode(&input_record.Event.KeyEvent);
                if (cp != nullptr)
                {
                    wchbuf = cp->SecondChar;
                    result = cp->LeadChar;
                    __leave;
                }
            }
        }
    }
    __finally
    {
        // Restore the previous console mode:
        __dcrt_set_input_console_mode(old_console_mode);
    }
    __endtry
    return result;
}



extern "C" wint_t __cdecl _getwche_nolock()
{
    // First check the pushback buffer for a character.  If it has one, return
    // it without echoing and reset the buffer:
    if (wchbuf != WEOF)
    {
        wchar_t const buffered_wchar = static_cast<wchar_t>(wchbuf & 0xFFFF);
        wchbuf = WEOF;
        return buffered_wchar;
    }

    // Othwrwise, read the character, echo it, and return it.  If anything fails,
    // we immediately return WEOF.
    wchar_t const gotten_wchar = _getwch_nolock();
    if (gotten_wchar == WEOF)
        return WEOF;

    if (_putwch_nolock(gotten_wchar) == WEOF)
        return WEOF;

    return gotten_wchar;
}



// Pushes back ("ungets") one character to be read next by _getwch() or
// _getwche().  On success, returns the character that was pushed back; on
// failure, returns EOF.
extern "C" wint_t __cdecl _ungetwch(wint_t const c)
{
    __acrt_lock(__acrt_conio_lock);
    wint_t result = 0;
    __try
    {
        result = _ungetwch_nolock(c);
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}



extern "C" wint_t __cdecl _ungetwch_nolock(wint_t const c)
{
    // Fail if the char is EOF or the pushback buffer is non-empty:
    if (c == WEOF || wchbuf != WEOF)
        return static_cast<wint_t>(EOF);

    wchbuf = (c & 0xFF);
    return wchbuf;
}
