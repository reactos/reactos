//
// getch.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _getch(), _getche(), and _ungetch(), which get and unget
// characters directly from the console.
//
#include <conio.h>
#include <corecrt_internal_lowio.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>



namespace
{
    struct CharPair
    {
        unsigned char LeadChar;
        unsigned char SecondChar;
    };

    struct EnhKeyVals
    {
        unsigned short ScanCode;
        CharPair RegChars;
        CharPair ShiftChars;
        CharPair CtrlChars;
        CharPair AltChars;
    };

    struct NormKeyVals
    {
        CharPair RegChars;
        CharPair ShiftChars;
        CharPair CtrlChars;
        CharPair AltChars;
    };
}



// Table of enhanced key values:
static EnhKeyVals const EnhancedKeys[] =
{
    { 28, {  13,   0 }, {  13,   0 }, {  10,   0 }, {   0, 166 } },
    { 53, {  47,   0 }, {  63,   0 }, {   0, 149 }, {   0, 164 } },
    { 71, { 224,  71 }, { 224,  71 }, { 224, 119 }, {   0, 151 } },
    { 72, { 224,  72 }, { 224,  72 }, { 224, 141 }, {   0, 152 } },
    { 73, { 224,  73 }, { 224,  73 }, { 224, 134 }, {   0, 153 } },
    { 75, { 224,  75 }, { 224,  75 }, { 224, 115 }, {   0, 155 } },
    { 77, { 224,  77 }, { 224,  77 }, { 224, 116 }, {   0, 157 } },
    { 79, { 224,  79 }, { 224,  79 }, { 224, 117 }, {   0, 159 } },
    { 80, { 224,  80 }, { 224,  80 }, { 224, 145 }, {   0, 160 } },
    { 81, { 224,  81 }, { 224,  81 }, { 224, 118 }, {   0, 161 } },
    { 82, { 224,  82 }, { 224,  82 }, { 224, 146 }, {   0, 162 } },
    { 83, { 224,  83 }, { 224,  83 }, { 224, 147 }, {   0, 163 } }
};

// The number of elements in EnhancedKeys:
#define NUM_EKA_ELTS (sizeof(EnhancedKeys) / sizeof(EnhKeyVals))



// Table of key values for normal keys.  Note that the table is padded so that
// the key scan code serves as an index into the table.
static NormKeyVals const NormalKeys[] =
{
    /* padding */
    { /*  0 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /*  1 */ {  27,   0 }, {  27,   0 }, {  27,   0 }, {   0,   1 } },
    { /*  2 */ {  49,   0 }, {  33,   0 }, {   0,   0 }, {   0, 120 } },
    { /*  3 */ {  50,   0 }, {  64,   0 }, {   0,   3 }, {   0, 121 } },
    { /*  4 */ {  51,   0 }, {  35,   0 }, {   0,   0 }, {   0, 122 } },
    { /*  5 */ {  52,   0 }, {  36,   0 }, {   0,   0 }, {   0, 123 } },
    { /*  6 */ {  53,   0 }, {  37,   0 }, {   0,   0 }, {   0, 124 } },
    { /*  7 */ {  54,   0 }, {  94,   0 }, {  30,   0 }, {   0, 125 } },
    { /*  8 */ {  55,   0 }, {  38,   0 }, {   0,   0 }, {   0, 126 } },
    { /*  9 */ {  56,   0 }, {  42,   0 }, {   0,   0 }, {   0, 127 } },
    { /* 10 */ {  57,   0 }, {  40,   0 }, {   0,   0 }, {   0, 128 } },
    { /* 11 */ {  48,   0 }, {  41,   0 }, {   0,   0 }, {   0, 129 } },
    { /* 12 */ {  45,   0 }, {  95,   0 }, {  31,   0 }, {   0, 130 } },
    { /* 13 */ {  61,   0 }, {  43,   0 }, {   0,   0 }, {   0, 131 } },
    { /* 14 */ {   8,   0 }, {   8,   0 }, { 127,   0 }, {   0,  14 } },
    { /* 15 */ {   9,   0 }, {   0,  15 }, {   0, 148 }, {   0,  15 } },
    { /* 16 */ { 113,   0 }, {  81,   0 }, {  17,   0 }, {   0,  16 } },
    { /* 17 */ { 119,   0 }, {  87,   0 }, {  23,   0 }, {   0,  17 } },
    { /* 18 */ { 101,   0 }, {  69,   0 }, {   5,   0 }, {   0,  18 } },
    { /* 19 */ { 114,   0 }, {  82,   0 }, {  18,   0 }, {   0,  19 } },
    { /* 20 */ { 116,   0 }, {  84,   0 }, {  20,   0 }, {   0,  20 } },
    { /* 21 */ { 121,   0 }, {  89,   0 }, {  25,   0 }, {   0,  21 } },
    { /* 22 */ { 117,   0 }, {  85,   0 }, {  21,   0 }, {   0,  22 } },
    { /* 23 */ { 105,   0 }, {  73,   0 }, {   9,   0 }, {   0,  23 } },
    { /* 24 */ { 111,   0 }, {  79,   0 }, {  15,   0 }, {   0,  24 } },
    { /* 25 */ { 112,   0 }, {  80,   0 }, {  16,   0 }, {   0,  25 } },
    { /* 26 */ {  91,   0 }, { 123,   0 }, {  27,   0 }, {   0,  26 } },
    { /* 27 */ {  93,   0 }, { 125,   0 }, {  29,   0 }, {   0,  27 } },
    { /* 28 */ {  13,   0 }, {  13,   0 }, {  10,   0 }, {   0,  28 } },

    /* padding */
    { /* 29 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 30 */ {  97,   0 }, {  65,   0 }, {   1,   0 }, {   0,  30 } },
    { /* 31 */ { 115,   0 }, {  83,   0 }, {  19,   0 }, {   0,  31 } },
    { /* 32 */ { 100,   0 }, {  68,   0 }, {   4,   0 }, {   0,  32 } },
    { /* 33 */ { 102,   0 }, {  70,   0 }, {   6,   0 }, {   0,  33 } },
    { /* 34 */ { 103,   0 }, {  71,   0 }, {   7,   0 }, {   0,  34 } },
    { /* 35 */ { 104,   0 }, {  72,   0 }, {   8,   0 }, {   0,  35 } },
    { /* 36 */ { 106,   0 }, {  74,   0 }, {  10,   0 }, {   0,  36 } },
    { /* 37 */ { 107,   0 }, {  75,   0 }, {  11,   0 }, {   0,  37 } },
    { /* 38 */ { 108,   0 }, {  76,   0 }, {  12,   0 }, {   0,  38 } },
    { /* 39 */ {  59,   0 }, {  58,   0 }, {   0,   0 }, {   0,  39 } },
    { /* 40 */ {  39,   0 }, {  34,   0 }, {   0,   0 }, {   0,  40 } },
    { /* 41 */ {  96,   0 }, { 126,   0 }, {   0,   0 }, {   0,  41 } },

    /* padding */
    { /* 42 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 43 */ {  92,   0 }, { 124,   0 }, {  28,   0 }, {   0,   0 } },
    { /* 44 */ { 122,   0 }, {  90,   0 }, {  26,   0 }, {   0,  44 } },
    { /* 45 */ { 120,   0 }, {  88,   0 }, {  24,   0 }, {   0,  45 } },
    { /* 46 */ {  99,   0 }, {  67,   0 }, {   3,   0 }, {   0,  46 } },
    { /* 47 */ { 118,   0 }, {  86,   0 }, {  22,   0 }, {   0,  47 } },
    { /* 48 */ {  98,   0 }, {  66,   0 }, {   2,   0 }, {   0,  48 } },
    { /* 49 */ { 110,   0 }, {  78,   0 }, {  14,   0 }, {   0,  49 } },
    { /* 50 */ { 109,   0 }, {  77,   0 }, {  13,   0 }, {   0,  50 } },
    { /* 51 */ {  44,   0 }, {  60,   0 }, {   0,   0 }, {   0,  51 } },
    { /* 52 */ {  46,   0 }, {  62,   0 }, {   0,   0 }, {   0,  52 } },
    { /* 53 */ {  47,   0 }, {  63,   0 }, {   0,   0 }, {   0,  53 } },

    /* padding */
    { /* 54 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 55 */ {  42,   0 }, {   0,   0 }, { 114,   0 }, {   0,   0 } },

    /* padding */
    { /* 56 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 57 */ {  32,   0 }, {  32,   0 }, {  32,   0 }, {  32,   0 } },

    /* padding */
    { /* 58 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 59 */ {   0,  59 }, {   0,  84 }, {   0,  94 }, {   0, 104 } },
    { /* 60 */ {   0,  60 }, {   0,  85 }, {   0,  95 }, {   0, 105 } },
    { /* 61 */ {   0,  61 }, {   0,  86 }, {   0,  96 }, {   0, 106 } },
    { /* 62 */ {   0,  62 }, {   0,  87 }, {   0,  97 }, {   0, 107 } },
    { /* 63 */ {   0,  63 }, {   0,  88 }, {   0,  98 }, {   0, 108 } },
    { /* 64 */ {   0,  64 }, {   0,  89 }, {   0,  99 }, {   0, 109 } },
    { /* 65 */ {   0,  65 }, {   0,  90 }, {   0, 100 }, {   0, 110 } },
    { /* 66 */ {   0,  66 }, {   0,  91 }, {   0, 101 }, {   0, 111 } },
    { /* 67 */ {   0,  67 }, {   0,  92 }, {   0, 102 }, {   0, 112 } },
    { /* 68 */ {   0,  68 }, {   0,  93 }, {   0, 103 }, {   0, 113 } },

    /* padding */
    { /* 69 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 70 */ {    0,  0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 71 */ {   0,  71 }, {  55,   0 }, {   0, 119 }, {   0,   0 } },
    { /* 72 */ {   0,  72 }, {  56,   0 }, {   0, 141 }, {   0,   0 } },
    { /* 73 */ {   0,  73 }, {  57,   0 }, {   0, 132 }, {   0,   0 } },
    { /* 74 */ {   0,   0 }, {  45,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 75 */ {   0,  75 }, {  52,   0 }, {   0, 115 }, {   0,   0 } },
    { /* 76 */ {   0,   0 }, {  53,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 77 */ {   0,  77 }, {  54,   0 }, {   0, 116 }, {   0,   0 } },
    { /* 78 */ {   0,   0 }, {  43,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 79 */ {   0,  79 }, {  49,   0 }, {   0, 117 }, {   0,   0 } },
    { /* 80 */ {   0,  80 }, {  50,   0 }, {   0, 145 }, {   0,   0 } },
    { /* 81 */ {   0,  81 }, {  51,   0 }, {   0, 118 }, {   0,   0 } },
    { /* 82 */ {   0,  82 }, {  48,   0 }, {   0, 146 }, {   0,   0 } },
    { /* 83 */ {   0,  83 }, {  46,   0 }, {   0, 147 }, {   0,   0 } },

    /* padding */
    { /* 84 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 85 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },
    { /* 86 */ {   0,   0 }, {   0,   0 }, {   0,   0 }, {   0,   0 } },

    { /* 87 */ { 224, 133 }, { 224, 135 }, { 224, 137 }, { 224, 139 } },
    { /* 88 */ { 224, 134 }, { 224, 136 }, { 224, 138 }, { 224, 140 } }
};

// The primary purpose of the pushback buffer is so that if a
// multi-byte character or extended key code is read, we can return
// the first byte and store the rest of the data in the buffer for
// subsequent calls. UTF-8 characters can be up to 4 bytes long, so
// the pushback buffer must be able to store the 3 remaining bytes.
size_t const getch_pushback_buffer_capacity = 3;

static int getch_pushback_buffer[getch_pushback_buffer_capacity];
static int getch_pushback_buffer_index = 0;
static int getch_pushback_buffer_current_size = 0;

static bool is_getch_pushback_buffer_full()
{
    return getch_pushback_buffer_current_size >= getch_pushback_buffer_capacity;
}

static void add_to_getch_pushback_buffer(int const c)
{
    _ASSERTE(!is_getch_pushback_buffer_full());
    getch_pushback_buffer[getch_pushback_buffer_current_size++] = c;
}

static int peek_next_getch_pushback_buffer()
{
    if (getch_pushback_buffer_current_size == 0) {
        return EOF;
    }

    return getch_pushback_buffer[getch_pushback_buffer_index];
}

static int get_next_getch_pushback_buffer()
{
    if (getch_pushback_buffer_current_size == 0) {
        return EOF;
    }

    int const ret_val = getch_pushback_buffer[getch_pushback_buffer_index++];

    if (getch_pushback_buffer_index == getch_pushback_buffer_current_size) {
        getch_pushback_buffer_index = 0;
        getch_pushback_buffer_current_size = 0;
    }

    return ret_val;
}

extern "C" intptr_t __dcrt_lowio_console_input_handle;

extern "C" CharPair const* __cdecl _getextendedkeycode(KEY_EVENT_RECORD*);
extern "C" int __cdecl _kbhit_nolock();



// These functions read a single character from the console.  _getch() does not
// echo the character; _getche() does echo the character.  If the push-back
// buffer is nonempty, the buffered character is returned immediately, without
// being echoed.
//
// On success, the read character is returned; on failure, EOF is returned.
extern "C" int __cdecl _getch()
{
    __acrt_lock(__acrt_conio_lock);
    int result = 0;
    __try
    {
        result = _getch_nolock();
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}

extern "C" int __cdecl _getche()
{
    __acrt_lock(__acrt_conio_lock);
    int result = 0;
    __try
    {
        result = _getche_nolock();
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}

extern "C" int __cdecl _getch_nolock()
{
    // Check the pushback buffer for a character.  If one is present, return it:
    int const pushback = get_next_getch_pushback_buffer();
    if (pushback != EOF)
    {
        return pushback;
    }

    if (__dcrt_lowio_ensure_console_input_initialized() == FALSE) {
        return EOF;
    }

    // Switch console to raw mode:
    DWORD old_console_mode;
    __dcrt_get_input_console_mode(&old_console_mode);
    __dcrt_set_input_console_mode(0);

    int result = 0;

    __try
    {
        for ( ; ; )
        {
            // Get a console input event:
            INPUT_RECORD input_record;
            DWORD num_read;

            if (__dcrt_read_console_input(&input_record, 1, &num_read) == FALSE || num_read == 0)
            {
                result = EOF;
                __leave;
            }

            // Look for, and decipher, key events.
            if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
            {
                // Simple case:  if UnicodeChar is non-zero, we can convert it to char and return it.
                wchar_t const c = input_record.Event.KeyEvent.uChar.UnicodeChar;
                if (c != 0)
                {
                    wchar_t const c_buffer[2] = {c, L'\0'};
                    char mb_chars[4];

                    size_t const amount_written = __acrt_wcs_to_mbs_cp_array(
                        c_buffer,
                        mb_chars,
                        GetConsoleCP()
                        );

                    // Mask with 0xFF to just get lowest byte
                    if (amount_written >= 1) {
                        result = mb_chars[0] & 0xFF;
                    }

                    if (amount_written >= 2) {
                        for (size_t i = 1; i < amount_written; ++i) {
                            add_to_getch_pushback_buffer(mb_chars[i] & 0xFF);
                        }
                    }
                    __leave;
                }

                // Hard case:  either it is an extended code or an event which
                // should not be recognized.  Let _getextendedkeycode do the work:
                CharPair const* const cp = _getextendedkeycode(&input_record.Event.KeyEvent);
                if (cp != nullptr)
                {
                    // Mask with 0xFF to just get lowest byte
                    add_to_getch_pushback_buffer(cp->SecondChar & 0xFF);
                    result = cp->LeadChar & 0xFF;
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



extern "C" int __cdecl _getche_nolock()
{
    // Check the pushback buffer for a character.  If one is present, return
    // it without echoing:
    int const pushback = get_next_getch_pushback_buffer();
    if (pushback != EOF)
    {
        return pushback;
    }

    // Otherwise, read the next character from the console and echo it:
    int const c = _getch_nolock();
    if (c == EOF)
        return EOF;

    if (_putch_nolock(c) == EOF)
        return EOF;

    return c;
}



// Returns nonzero if a keystroke is waiting to be read; otherwise returns zero.
extern "C" int __cdecl _kbhit()
{
    __acrt_lock(__acrt_conio_lock);
    int result = 0;
    __try
    {
        result = _kbhit_nolock();
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}



extern "C" int __cdecl _kbhit_nolock()
{
    // If a character has been pushed back, return TRUE:
    if (peek_next_getch_pushback_buffer() != EOF) {
        return TRUE;
    }

    if (__dcrt_lowio_ensure_console_input_initialized() == FALSE) {
        return FALSE;
    }

    // Peek at all pending console events:
    DWORD num_pending;
    if (__dcrt_get_number_of_console_input_events(&num_pending) == FALSE) {
        return FALSE;
    }

    if (num_pending == 0) {
        return FALSE;
    }

    __crt_scoped_stack_ptr<INPUT_RECORD> const input_buffer(_malloca_crt_t(INPUT_RECORD, num_pending));
    if (input_buffer.get() == nullptr) {
        return FALSE;
    }

    DWORD num_peeked;
    // AsciiChar is not read, so using the narrow Win32 API is permitted.
    if (__dcrt_peek_console_input_a(input_buffer.get(), num_pending, &num_peeked) == FALSE) {
        return FALSE;
    }

    if (num_peeked == 0 || num_peeked > num_pending) {
        return FALSE;
    }

    // Scan all of the peeked events to determine if any is a key event
    // that should be recognized:
    for (INPUT_RECORD* p = input_buffer.get(); num_peeked > 0; --num_peeked, ++p)
    {
        if (p->EventType != KEY_EVENT)
            continue;

        if (!p->Event.KeyEvent.bKeyDown)
            continue;

        if (p->Event.KeyEvent.uChar.AsciiChar == 0 &&
            _getextendedkeycode(&p->Event.KeyEvent) == nullptr)
            continue;

        return TRUE;
    }

    return FALSE;
}



// Pushes back ("ungets") one character to be read next by _getwch() or
// _getwche().  On success, returns the character that was pushed back; on
// failure, returns EOF.
extern "C" int __cdecl _ungetch(int const c)
{
    __acrt_lock(__acrt_conio_lock);
    int result = 0;
    __try
    {
        result = _ungetch_nolock(c);
    }
    __finally
    {
        __acrt_unlock(__acrt_conio_lock);
    }
    __endtry
    return result;
}



extern "C" int __cdecl _ungetch_nolock(int const c)
{
    // Fail if the character is EOF or the pusback buffer is nonempty:
    if (c == EOF || is_getch_pushback_buffer_full()) {
        return EOF;
    }

    add_to_getch_pushback_buffer(c);
    return c;
}



// Returns the extended code (if there is one) for a key event.  This is the
// core function for the _getch() and _getche() functions and their wide
// character equivalents, and is essential to _kbhit().  This is the function
// that determines whether or not a key event NOT accompanied by an ASCII
// character has an extended code and returns that code.
//
// On success, a pointer to a CharPair value holding the lead and second
// characters of the extended code is returned.  On failure, nullptr is returned.
extern "C" CharPair const* __cdecl _getextendedkeycode(KEY_EVENT_RECORD* const pKE)
{
    DWORD const CKS = pKE->dwControlKeyState;

    if (CKS & ENHANCED_KEY)
    {
        // Find the appropriate entry in EnhancedKeys[]:
        for (int i = 0 ; i < NUM_EKA_ELTS; ++i)
        {
            if (EnhancedKeys[i].ScanCode != pKE->wVirtualScanCode)
            {
                continue;
            }

            // We found a match!  Determine which pair to return:
            if (CKS & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
            {
                return &EnhancedKeys[i].AltChars;
            }
            else if (CKS & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                return &EnhancedKeys[i].CtrlChars;
            }
            else if (CKS & SHIFT_PRESSED)
            {
                return &EnhancedKeys[i].ShiftChars;
            }
            else
            {
                return &EnhancedKeys[i].RegChars;
            }
        }

        return nullptr;
    }
    else
    {
        // Regular key or keyboard event which shouldn't be recognized.
        // Determine which by getting the proper field of the proper entry in
        // NormalKeys[] and examining the extended code.
        CharPair const* pCP;

        if (CKS & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
        {
            pCP = &NormalKeys[pKE->wVirtualScanCode].AltChars;
        }
        else if (CKS & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
        {
            pCP = &NormalKeys[pKE->wVirtualScanCode].CtrlChars;
        }
        else if (CKS & SHIFT_PRESSED)
        {
            pCP = &NormalKeys[pKE->wVirtualScanCode].ShiftChars;
        }
        else
        {
            pCP = &NormalKeys[pKE->wVirtualScanCode].RegChars;
        }

        // Make sure it wasn't a keyboard event which should not be recognized
        // (e.g. the shift key was pressed):
        if ((pCP->LeadChar != 0 && pCP->LeadChar != 224) || pCP->SecondChar == 0)
        {
            return nullptr;
        }

        return pCP;
    }
}
