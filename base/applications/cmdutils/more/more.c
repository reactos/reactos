/*
 * PROJECT:     ReactOS More Command
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Displays text stream from STDIN or from an arbitrary number
 *              of files to STDOUT, with screen capabilities (more than CAT,
 *              but less than LESS ^^).
 * COPYRIGHT:   Copyright 1999 Paolo Pantaleo
 *              Copyright 2003 Timothy Schepens
 *              Copyright 2016-2021 Hermes Belusca-Maito
 *              Copyright 2021 Katayama Hirofumi MZ
 */
/*
 * MORE.C - external command.
 *
 * clone from 4nt more command
 *
 * 26 Sep 1999 - Paolo Pantaleo <paolopan@freemail.it>
 *     started
 *
 * Oct 2003 - Timothy Schepens <tischepe at fastmail dot fm>
 *     use window size instead of buffer size.
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winnls.h>
#include <winreg.h>
#include <winuser.h>

#include <conutils.h>
#include <strsafe.h>

#include "resource.h"

/* PagePrompt statistics for the current file */
DWORD dwFileSize; // In bytes
DWORD dwSumReadBytes, dwSumReadChars;
// The average number of bytes per character is equal to
// dwSumReadBytes / dwSumReadChars. Note that dwSumReadChars
// will never be == 0 when ConWritePaging (and possibly PagePrompt)
// is called.

/* Handles for file and console */
HANDLE hFile = INVALID_HANDLE_VALUE;
HANDLE hStdIn, hStdOut;
HANDLE hKeyboard;

/* Enable/Disable extensions */
BOOL bEnableExtensions = TRUE; // FIXME: By default, it should be FALSE.

/* Parser flags */
#define FLAG_HELP   (1 << 0)
#define FLAG_E      (1 << 1)
#define FLAG_C      (1 << 2)
#define FLAG_P      (1 << 3)
#define FLAG_S      (1 << 4)
#define FLAG_Tn     (1 << 5)
#define FLAG_PLUSn  (1 << 6)

/* Prompt flags */
#define PROMPT_PERCENT  (1 << 0)
#define PROMPT_LINE_AT  (1 << 1)
#define PROMPT_OPTIONS  (1 << 2)
#define PROMPT_LINES    (1 << 3)

static DWORD s_dwFlags = 0;
static LONG s_nTabWidth = 8;
static DWORD s_nNextLineNo = 0;
static BOOL s_bPrevLineIsBlank = FALSE;
static WORD s_fPrompt = 0;
static BOOL s_bDoNextFile = FALSE;

static BOOL IsBlankLine(IN PCWCH line, IN DWORD cch)
{
    DWORD ich;
    WORD wType;
    for (ich = 0; ich < cch; ++ich)
    {
        /*
         * Explicitly exclude FORM-FEED from the check,
         * so that the pager can handle it.
         */
        if (line[ich] == L'\f')
            return FALSE;

        /*
         * Otherwise do the extended blanks check.
         * Note that MS MORE.COM only checks for spaces (\x20) and TABs (\x09).
         * See http://archives.miloush.net/michkap/archive/2007/06/11/3230072.html
         * for more information.
         */
        wType = 0;
        GetStringTypeW(CT_CTYPE1, &line[ich], 1, &wType);
        if (!(wType & (C1_BLANK | C1_SPACE)))
            return FALSE;
    }
    return TRUE;
}

static BOOL
__stdcall
MorePagerLine(
    IN OUT PCON_PAGER Pager,
    IN PCWCH line,
    IN DWORD cch)
{
    if (s_dwFlags & FLAG_PLUSn) /* Skip lines */
    {
        if (Pager->lineno < s_nNextLineNo)
        {
            s_bPrevLineIsBlank = FALSE;
            return TRUE; /* Handled */
        }
        s_dwFlags &= ~FLAG_PLUSn;
    }

    if (s_dwFlags & FLAG_S) /* Shrink blank lines */
    {
        if (IsBlankLine(line, cch))
        {
            if (s_bPrevLineIsBlank)
                return TRUE; /* Handled */

            /*
             * Display a single blank line, independently of the actual size
             * of the current line, by displaying just one space: this is
             * especially needed in order to force line wrapping when the
             * ENABLE_VIRTUAL_TERMINAL_PROCESSING or DISABLE_NEWLINE_AUTO_RETURN
             * console modes are enabled.
             * Then, reposition the cursor to the next line, first column.
             */
            if (Pager->PageColumns > 0)
                ConStreamWrite(Pager->Screen->Stream, TEXT(" "), 1);
            ConStreamWrite(Pager->Screen->Stream, TEXT("\n"), 1);
            Pager->iLine++;
            Pager->iColumn = 0;

            s_bPrevLineIsBlank = TRUE;
            s_nNextLineNo = 0;

            return TRUE; /* Handled */
        }
        else
        {
            s_bPrevLineIsBlank = FALSE;
        }
    }

    s_nNextLineNo = 0;
    /* Not handled, let the pager do the default action */
    return FALSE;
}

static BOOL
__stdcall
PagePrompt(PCON_PAGER Pager, DWORD Done, DWORD Total)
{
    HANDLE hInput = ConStreamGetOSHandle(StdIn);
    HANDLE hOutput = ConStreamGetOSHandle(Pager->Screen->Stream);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD orgCursorPosition;
    DWORD dwMode;

    KEY_EVENT_RECORD KeyEvent;
    BOOL fCtrl;
    DWORD nLines;
    WCHAR chSubCommand = 0;

    /* Prompt strings (small size since the prompt should
     * hold ideally on one <= 80-character line) */
    static WCHAR StrPercent[80] = L"";
    static WCHAR StrLineAt[80]  = L"";
    static WCHAR StrOptions[80] = L"";
    static WCHAR StrLines[80]   = L"";

    WCHAR szPercent[80] = L"";
    WCHAR szLineAt[80]  = L"";

    /* Load the prompt strings */
    if (!*StrPercent)
        K32LoadStringW(NULL, IDS_CONTINUE_PERCENT, StrPercent, ARRAYSIZE(StrPercent));
    if (!*StrLineAt)
        K32LoadStringW(NULL, IDS_CONTINUE_LINE_AT, StrLineAt, ARRAYSIZE(StrLineAt));
    if (!*StrOptions)
        K32LoadStringW(NULL, IDS_CONTINUE_OPTIONS, StrOptions, ARRAYSIZE(StrOptions));
    if (!*StrLines)
        K32LoadStringW(NULL, IDS_CONTINUE_LINES, StrLines, ARRAYSIZE(StrLines));

    /*
     * Check whether the pager is prompting, but we have actually finished
     * to display a given file, or no data is present in STDIN anymore.
     * In this case, skip the prompt altogether. The only exception is when
     * we are displaying other files.
     */
    // TODO: Implement!

Restart:
    nLines = 0;

    /* Do not show the progress percentage when STDIN is being displayed */
    if (s_fPrompt & PROMPT_PERCENT) // && (hFile != hStdIn)
    {
        /*
         * The progress percentage is evaluated as follows.
         * So far we have read a total of 'dwSumReadBytes' bytes from the file.
         * Amongst those is the latest read chunk of 'dwReadBytes' bytes, to which
         * correspond a number of 'dwReadChars' characters with which we have called
         * ConWritePaging who called PagePrompt. We then have: Total == dwReadChars.
         * During this ConWritePaging call the PagePrompt was called after 'Done'
         * number of characters over 'Total'.
         * It should be noted that for 'dwSumReadBytes' number of bytes read it
         * *roughly* corresponds 'dwSumReadChars' number of characters. This is
         * because there may be some failures happening during the conversion of
         * the bytes read to the character string for a given encoding.
         * Therefore the number of characters displayed on screen is equal to:
         *   dwSumReadChars - Total + Done ,
         * but the best corresponding approximed number of bytes would be:
         *   dwSumReadBytes - (Total - Done) * (dwSumReadBytes / dwSumReadChars) ,
         * where the ratio is the average number of bytes per character.
         * The percentage is then computed relative to the total file size.
         */
        DWORD dwPercent = (dwSumReadBytes - (Total - Done) *
                           (dwSumReadBytes / dwSumReadChars)) * 100 / dwFileSize;
        StringCchPrintfW(szPercent, ARRAYSIZE(szPercent), StrPercent, dwPercent);
    }
    if (s_fPrompt & PROMPT_LINE_AT)
    {
        StringCchPrintfW(szLineAt, ARRAYSIZE(szLineAt), StrLineAt, Pager->lineno);
    }

    /* Suitably format and display the prompt */
    ConResMsgPrintf(Pager->Screen->Stream, 0, IDS_CONTINUE_PROMPT,
                    (s_fPrompt & PROMPT_PERCENT ? szPercent  : L""),
                    (s_fPrompt & PROMPT_LINE_AT ? szLineAt   : L""),
                    (s_fPrompt & PROMPT_OPTIONS ? StrOptions : L""),
                    (s_fPrompt & PROMPT_LINES   ? StrLines   : L""));

    /* Reset the prompt to a default state */
    s_fPrompt &= ~(PROMPT_LINE_AT | PROMPT_OPTIONS | PROMPT_LINES);

    /* RemoveBreakHandler */
    SetConsoleCtrlHandler(NULL, TRUE);
    /* ConInDisable */
    GetConsoleMode(hInput, &dwMode);
    dwMode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);

    // FIXME: Does not support TTY yet!
    ConGetScreenInfo(Pager->Screen, &csbi);
    orgCursorPosition = csbi.dwCursorPosition;
    for (;;)
    {
        INPUT_RECORD ir = {0};
        DWORD dwRead;
        WCHAR ch;

        do
        {
            ReadConsoleInput(hInput, &ir, 1, &dwRead);
        }
        while ((ir.EventType != KEY_EVENT) || (!ir.Event.KeyEvent.bKeyDown));

        /* Got our key */
        KeyEvent = ir.Event.KeyEvent;

        /* Ignore any unsupported keyboard press */
        if ((KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
            (KeyEvent.wVirtualKeyCode == VK_MENU)  ||
            (KeyEvent.wVirtualKeyCode == VK_CONTROL))
        {
            continue;
        }

        /* Ctrl key is pressed? */
        fCtrl = !!(KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));

        /* Ctrl+C or Ctrl+Esc? */
        if (fCtrl && ((KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
                      (KeyEvent.wVirtualKeyCode == L'C')))
        {
            chSubCommand = 0;
            break;
        }

        /* If extended features are unavailable, or no
         * pending commands, don't do more processing. */
        if (!(s_dwFlags & FLAG_E) || (chSubCommand == 0))
            break;

        ch = KeyEvent.uChar.UnicodeChar;
        if (L'0' <= ch && ch <= L'9')
        {
            nLines *= 10;
            nLines += ch - L'0';
            ConStreamWrite(Pager->Screen->Stream, &ch, 1);
            continue;
        }
        else if (KeyEvent.wVirtualKeyCode == VK_RETURN)
        {
            /* Validate the line number */
            break;
        }
        else if (KeyEvent.wVirtualKeyCode == VK_ESCAPE)
        {
            /* Cancel the current command */
            chSubCommand = 0;
            break;
        }
        else if (KeyEvent.wVirtualKeyCode == VK_BACK)
        {
            if (nLines != 0)
                nLines /= 10;

            /* Erase the current character */
            ConGetScreenInfo(Pager->Screen, &csbi);
            if ( (csbi.dwCursorPosition.Y  > orgCursorPosition.Y) ||
                ((csbi.dwCursorPosition.Y == orgCursorPosition.Y) &&
                 (csbi.dwCursorPosition.X  > orgCursorPosition.X)) )
            {
                if (csbi.dwCursorPosition.X > 0)
                {
                    csbi.dwCursorPosition.X = csbi.dwCursorPosition.X - 1;
                }
                else if (csbi.dwCursorPosition.Y > 0)
                {
                    csbi.dwCursorPosition.Y = csbi.dwCursorPosition.Y - 1;
                    csbi.dwCursorPosition.X = (csbi.dwSize.X ? csbi.dwSize.X - 1 : 0);
                }

                SetConsoleCursorPosition(hOutput, csbi.dwCursorPosition);

                ch = L' ';
                ConStreamWrite(Pager->Screen->Stream, &ch, 1);
                SetConsoleCursorPosition(hOutput, csbi.dwCursorPosition);
            }

            continue;
        }
    }

    /* AddBreakHandler */
    SetConsoleCtrlHandler(NULL, FALSE);
    /* ConInEnable */
    GetConsoleMode(hInput, &dwMode);
    dwMode |= ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);

    /* Refresh the screen information, as the console may have been
     * redimensioned. Update also the default number of lines to scroll. */
    ConGetScreenInfo(Pager->Screen, &csbi);
    Pager->ScrollRows = csbi.srWindow.Bottom - csbi.srWindow.Top;

    /*
     * Erase the full line where the cursor is, and move
     * the cursor back to the beginning of the line.
     */
    ConClearLine(Pager->Screen->Stream);

    /* Ctrl+C or Ctrl+Esc: Control Break */
    if (fCtrl && ((KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
                  (KeyEvent.wVirtualKeyCode == L'C')))
    {
        /* We break, output a newline */
        WCHAR ch = L'\n';
        ConStreamWrite(Pager->Screen->Stream, &ch, 1);
        return FALSE;
    }

    switch (chSubCommand)
    {
        case L'P':
        {
            /* If we don't display other lines, just restart the prompt */
            if (nLines == 0)
            {
                chSubCommand = 0;
                goto Restart;
            }
            /* Otherwise tell the pager to display them */
            Pager->ScrollRows = nLines;
            return TRUE;
        }
        case L'S':
        {
            s_dwFlags |= FLAG_PLUSn;
            s_nNextLineNo = Pager->lineno + nLines;
            /* Use the default Pager->ScrollRows value */
            return TRUE;
        }
        default:
            chSubCommand = 0;
            break;
    }

    /* If extended features are available */
    if (s_dwFlags & FLAG_E)
    {
        /* Ignore any key presses if Ctrl is pressed */
        if (fCtrl)
        {
            chSubCommand = 0;
            goto Restart;
        }

        /* 'Q': Quit */
        if (KeyEvent.wVirtualKeyCode == L'Q')
        {
            /* We break, output a newline */
            WCHAR ch = L'\n';
            ConStreamWrite(Pager->Screen->Stream, &ch, 1);
            return FALSE;
        }

        /* 'F': Next file */
        if (KeyEvent.wVirtualKeyCode == L'F')
        {
            s_bDoNextFile = TRUE;
            return FALSE;
        }

        /* '?': Show Options */
        if (KeyEvent.uChar.UnicodeChar == L'?')
        {
            s_fPrompt |= PROMPT_OPTIONS;
            goto Restart;
        }

        /* [Enter] key: Display one line */
        if (KeyEvent.wVirtualKeyCode == VK_RETURN)
        {
            Pager->ScrollRows = 1;
            return TRUE;
        }

        /* [Space] key: Display one page */
        if (KeyEvent.wVirtualKeyCode == VK_SPACE)
        {
            if (s_dwFlags & FLAG_C)
            {
                /* Clear the screen */
                ConClearScreen(Pager->Screen);
            }
            /* Use the default Pager->ScrollRows value */
            return TRUE;
        }

        /* 'P': Display n lines */
        if (KeyEvent.wVirtualKeyCode == L'P')
        {
            s_fPrompt |= PROMPT_LINES;
            chSubCommand = L'P';
            goto Restart;
        }

        /* 'S': Skip n lines */
        if (KeyEvent.wVirtualKeyCode == L'S')
        {
            s_fPrompt |= PROMPT_LINES;
            chSubCommand = L'S';
            goto Restart;
        }

        /* '=': Show current line number */
        if (KeyEvent.uChar.UnicodeChar == L'=')
        {
            s_fPrompt |= PROMPT_LINE_AT;
            goto Restart;
        }

        chSubCommand = 0;
        goto Restart;
    }
    else
    {
        /* Extended features are unavailable: display one page */
        /* Use the default Pager->ScrollRows value */
        return TRUE;
    }
}

/*
 * See base/applications/cmdutils/clip/clip.c!IsDataUnicode()
 * and base/applications/notepad/text.c!ReadText() for more details.
 * Also some good code example can be found at:
 * https://github.com/AutoIt/text-encoding-detect
 */
typedef enum
{
    ENCODING_ANSI    =  0,
    ENCODING_UTF16LE =  1,
    ENCODING_UTF16BE =  2,
    ENCODING_UTF8    =  3
} ENCODING;

static BOOL
IsDataUnicode(
    IN PVOID Buffer,
    IN DWORD BufferSize,
    OUT ENCODING* Encoding OPTIONAL,
    OUT PDWORD SkipBytes OPTIONAL)
{
    PBYTE pBytes = Buffer;
    ENCODING encFile = ENCODING_ANSI;
    DWORD dwPos = 0;

    /*
     * See http://archives.miloush.net/michkap/archive/2007/04/22/2239345.html
     * for more details about the algorithm and the pitfalls behind it.
     * Of course it would be actually great to make a nice function that
     * would work, once and for all, and put it into a library.
     */

    /* Look for Byte Order Marks */
    if ((BufferSize >= 2) && (pBytes[0] == 0xFF) && (pBytes[1] == 0xFE))
    {
        encFile = ENCODING_UTF16LE;
        dwPos = 2;
    }
    else if ((BufferSize >= 2) && (pBytes[0] == 0xFE) && (pBytes[1] == 0xFF))
    {
        encFile = ENCODING_UTF16BE;
        dwPos = 2;
    }
    else if ((BufferSize >= 3) && (pBytes[0] == 0xEF) && (pBytes[1] == 0xBB) && (pBytes[2] == 0xBF))
    {
        encFile = ENCODING_UTF8;
        dwPos = 3;
    }
    else
    {
        /*
         * Try using statistical analysis. Do not rely on the return value of
         * IsTextUnicode as we can get FALSE even if the text is in UTF-16 BE
         * (i.e. we have some of the IS_TEXT_UNICODE_REVERSE_MASK bits set).
         * Instead, set all the tests we want to perform, then just check
         * the passed tests and try to deduce the string properties.
         */

/*
 * This mask contains the 3 highest bits from IS_TEXT_UNICODE_NOT_ASCII_MASK
 * and the 1st highest bit from IS_TEXT_UNICODE_NOT_UNICODE_MASK.
 */
#define IS_TEXT_UNKNOWN_FLAGS_MASK  ((7 << 13) | (1 << 11))

        /* Flag out the unknown flags here, the passed tests will not have them either */
        INT Tests = (IS_TEXT_UNICODE_NOT_ASCII_MASK   |
                     IS_TEXT_UNICODE_NOT_UNICODE_MASK |
                     IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_UNICODE_MASK)
                        & ~IS_TEXT_UNKNOWN_FLAGS_MASK;
        INT Results;

        IsTextUnicode(Buffer, BufferSize, &Tests);
        Results = Tests;

        /*
         * As the IS_TEXT_UNICODE_NULL_BYTES or IS_TEXT_UNICODE_ILLEGAL_CHARS
         * flags are expected to be potentially present in the result without
         * modifying our expectations, filter them out now.
         */
        Results &= ~(IS_TEXT_UNICODE_NULL_BYTES | IS_TEXT_UNICODE_ILLEGAL_CHARS);

        /*
         * NOTE: The flags IS_TEXT_UNICODE_ASCII16 and
         * IS_TEXT_UNICODE_REVERSE_ASCII16 are not reliable.
         *
         * NOTE2: Check for potential "bush hid the facts" effect by also
         * checking the original results (in 'Tests') for the absence of
         * the IS_TEXT_UNICODE_NULL_BYTES flag, as we may presumably expect
         * that in UTF-16 text there will be at some point some NULL bytes.
         * If not, fall back to ANSI. This shows the limitations of using the
         * IsTextUnicode API to perform such tests, and the usage of a more
         * improved encoding detection algorithm would be really welcome.
         */
        if (!(Results & IS_TEXT_UNICODE_NOT_UNICODE_MASK) &&
            !(Results & IS_TEXT_UNICODE_REVERSE_MASK)     &&
             (Results & IS_TEXT_UNICODE_UNICODE_MASK)     &&
             (Tests   & IS_TEXT_UNICODE_NULL_BYTES))
        {
            encFile = ENCODING_UTF16LE;
            dwPos = (Results & IS_TEXT_UNICODE_SIGNATURE) ? 2 : 0;
        }
        else
        if (!(Results & IS_TEXT_UNICODE_NOT_UNICODE_MASK) &&
            !(Results & IS_TEXT_UNICODE_UNICODE_MASK)     &&
             (Results & IS_TEXT_UNICODE_REVERSE_MASK)     &&
             (Tests   & IS_TEXT_UNICODE_NULL_BYTES))
        {
            encFile = ENCODING_UTF16BE;
            dwPos = (Results & IS_TEXT_UNICODE_REVERSE_SIGNATURE) ? 2 : 0;
        }
        else
        {
            /*
             * Either 'Results' has neither of those masks set, as it can be
             * the case for UTF-8 text (or ANSI), or it has both as can be the
             * case when analysing pure binary data chunk. This is therefore
             * invalid and we fall back to ANSI encoding.
             * FIXME: In case of failure, assume ANSI (as long as we do not have
             * correct tests for UTF8, otherwise we should do them, and at the
             * very end, assume ANSI).
             */
            encFile = ENCODING_ANSI; // ENCODING_UTF8;
            dwPos = 0;
        }
    }

    if (Encoding)
        *Encoding = encFile;
    if (SkipBytes)
        *SkipBytes = dwPos;

    return (encFile != ENCODING_ANSI);
}

/*
 * Adapted from base/shell/cmd/misc.c!FileGetString(), but with correct
 * text encoding support. Also please note that similar code should be
 * also used in the CMD.EXE 'TYPE' command.
 * Contrary to CMD's FileGetString() we do not stop at new-lines.
 *
 * Read text data from a file and convert it from a given encoding to UTF-16.
 *
 *   IN OUT PVOID pCacheBuffer and IN DWORD CacheBufferLength :
 *     Implementation detail so that the function uses an external user-provided
 *     buffer to store the data temporarily read from the file. The function
 *     could have used an internal buffer instead. The length is in number of bytes.
 *
 *   IN OUT PWSTR* pBuffer and IN OUT PDWORD pnBufferLength :
 *     Reallocated buffer containing the string data converted to UTF-16.
 *     In input, contains a pointer to the original buffer and its length.
 *     In output, contains a pointer to the reallocated buffer and its length.
 *     The length is in number of characters.
 *
 *     At first call to this function, pBuffer can be set to NULL, in which case
 *     when the function returns the pointer will point to a valid buffer.
 *     After the last call to this function, free the pBuffer pointer with:
 *     HeapFree(GetProcessHeap(), 0, *pBuffer);
 *
 *     If Encoding is set to ENCODING_UTF16LE or ENCODING_UTF16BE, since we are
 *     compiled in UNICODE, no extra conversion is performed and therefore
 *     pBuffer is unused (remains unallocated) and one can directly use the
 *     contents of pCacheBuffer as it is expected to contain valid UTF-16 text.
 *
 *   OUT PDWORD pdwReadBytes : Number of bytes read from the file (optional).
 *   OUT PDWORD pdwReadChars : Corresponding number of characters read (optional).
 */
static BOOL
FileGetString(
    IN HANDLE hFile,
    IN ENCODING Encoding,
    IN OUT PVOID pCacheBuffer,
    IN DWORD CacheBufferLength,
    IN OUT PWCHAR* pBuffer,
    IN OUT PDWORD pnBufferLength,
    OUT PDWORD pdwReadBytes OPTIONAL,
    OUT PDWORD pdwReadChars OPTIONAL)
{
    BOOL Success;
    UINT CodePage = (UINT)-1;
    DWORD dwReadBytes;
    INT len;

    // ASSERT(pCacheBuffer && (CacheBufferLength > 0));
    // ASSERT(CacheBufferLength % 2 == 0); // Cache buffer length MUST BE even!
    // ASSERT(pBuffer && pnBufferLength);

    /* Always reset the retrieved number of bytes/characters */
    if (pdwReadBytes) *pdwReadBytes = 0;
    if (pdwReadChars) *pdwReadChars = 0;

    Success = ReadFile(hFile, pCacheBuffer, CacheBufferLength, &dwReadBytes, NULL);
    if (!Success || dwReadBytes == 0)
        return FALSE;

    if (pdwReadBytes) *pdwReadBytes = dwReadBytes;

    if ((Encoding == ENCODING_ANSI) || (Encoding == ENCODING_UTF8))
    {
        /* Conversion is needed */

        if (Encoding == ENCODING_ANSI)
            CodePage = GetConsoleCP(); // CP_ACP; // FIXME: Cache GetConsoleCP() value.
        else // if (Encoding == ENCODING_UTF8)
            CodePage = CP_UTF8;

        /* Retrieve the needed buffer size */
        len = MultiByteToWideChar(CodePage, 0, pCacheBuffer, dwReadBytes,
                                  NULL, 0);
        if (len == 0)
        {
            /* Failure, bail out */
            return FALSE;
        }

        /* Initialize the conversion buffer if needed... */
        if (*pBuffer == NULL)
        {
            *pnBufferLength = len;
            *pBuffer = HeapAlloc(GetProcessHeap(), 0, *pnBufferLength * sizeof(WCHAR));
            if (*pBuffer == NULL)
            {
                // *pBuffer = NULL;
                *pnBufferLength = 0;
                // WARN("DEBUG: Cannot allocate memory for *pBuffer!\n");
                // ConErrFormatMessage(GetLastError());
                return FALSE;
            }
        }
        /* ... or reallocate only if the new length is greater than the old one */
        else if (len > *pnBufferLength)
        {
            PWSTR OldBuffer = *pBuffer;

            *pnBufferLength = len;
            *pBuffer = HeapReAlloc(GetProcessHeap(), 0, *pBuffer, *pnBufferLength * sizeof(WCHAR));
            if (*pBuffer == NULL)
            {
                /* Do not leak old buffer */
                HeapFree(GetProcessHeap(), 0, OldBuffer);
                // *pBuffer = NULL;
                *pnBufferLength = 0;
                // WARN("DEBUG: Cannot reallocate memory for *pBuffer!\n");
                // ConErrFormatMessage(GetLastError());
                return FALSE;
            }
        }

        /* Now perform the conversion proper */
        len = MultiByteToWideChar(CodePage, 0, pCacheBuffer, dwReadBytes,
                                  *pBuffer, len);
        dwReadBytes = len;
    }
    else
    {
        /*
         * No conversion needed, just convert from big to little endian if needed.
         * pBuffer and pnBufferLength are left untouched and pCacheBuffer can be
         * directly used.
         */
        PWCHAR pWChars = pCacheBuffer;
        DWORD i;

        dwReadBytes /= sizeof(WCHAR);

        if (Encoding == ENCODING_UTF16BE)
        {
            for (i = 0; i < dwReadBytes; i++)
            {
                /* Equivalent to RtlUshortByteSwap: reverse high/low bytes */
                pWChars[i] = MAKEWORD(HIBYTE(pWChars[i]), LOBYTE(pWChars[i]));
            }
        }
        // else if (Encoding == ENCODING_UTF16LE), we are good, nothing to do.
    }

    /* Return the number of characters (dwReadBytes is converted) */
    if (pdwReadChars) *pdwReadChars = dwReadBytes;

    return TRUE;
}

static VOID
LoadRegistrySettings(HKEY hKeyRoot)
{
    LONG lRet;
    HKEY hKey;
    DWORD dwType, len;
    /*
     * Buffer big enough to hold the string L"4294967295",
     * corresponding to the literal 0xFFFFFFFF (MAXULONG) in decimal.
     */
    WCHAR Buffer[sizeof("4294967295")];
    C_ASSERT(sizeof(Buffer) >= sizeof(DWORD));

    lRet = RegOpenKeyExW(hKeyRoot,
                         L"Software\\Microsoft\\Command Processor",
                         0,
                         KEY_QUERY_VALUE,
                         &hKey);
    if (lRet != ERROR_SUCCESS)
        return;

    len = sizeof(Buffer);
    lRet = RegQueryValueExW(hKey,
                            L"EnableExtensions",
                            NULL,
                            &dwType,
                            (PBYTE)&Buffer,
                            &len);
    if (lRet == ERROR_SUCCESS)
    {
        /* Overwrite the default setting */
        if (dwType == REG_DWORD)
            bEnableExtensions = !!*(PDWORD)Buffer;
        else if (dwType == REG_SZ)
            bEnableExtensions = (_wtol((PWSTR)Buffer) == 1);
    }
    // else, use the default setting set globally.

    RegCloseKey(hKey);
}

static BOOL IsFlag(PCWSTR param)
{
    PCWSTR pch;
    PWCHAR endptr;

    if (param[0] == L'/')
        return TRUE;

    if (param[0] == L'+')
    {
        pch = param + 1;
        if (*pch)
        {
            (void)wcstol(pch, &endptr, 10);
            return (*endptr == 0);
        }
    }
    return FALSE;
}

static BOOL ParseArgument(PCWSTR arg, BOOL* pbHasFiles)
{
    PWCHAR endptr;

    if (arg[0] == L'/')
    {
        switch (towupper(arg[1]))
        {
            case L'?':
                if (arg[2] == 0)
                {
                    s_dwFlags |= FLAG_HELP;
                    return TRUE;
                }
                break;
            case L'E':
                if (arg[2] == 0)
                {
                    s_dwFlags |= FLAG_E;
                    return TRUE;
                }
                break;
            case L'C':
                if (arg[2] == 0)
                {
                    s_dwFlags |= FLAG_C;
                    return TRUE;
                }
                break;
            case L'P':
                if (arg[2] == 0)
                {
                    s_dwFlags |= FLAG_P;
                    return TRUE;
                }
                break;
            case L'S':
                if (arg[2] == 0)
                {
                    s_dwFlags |= FLAG_S;
                    return TRUE;
                }
                break;
            case L'T':
                if (arg[2] != 0)
                {
                    s_dwFlags |= FLAG_Tn;
                    s_nTabWidth = wcstol(&arg[2], &endptr, 10);
                    if (*endptr == 0)
                        return TRUE;
                }
                break;
            default:
                break;
        }
    }
    else if (arg[0] == L'+')
    {
        if (arg[1] != 0)
        {
            s_dwFlags |= FLAG_PLUSn;
            s_nNextLineNo = wcstol(&arg[1], &endptr, 10) + 1;
            if (*endptr == 0)
                return TRUE;
        }
    }

    if (IsFlag(arg))
    {
        ConResPrintf(StdErr, IDS_BAD_FLAG, arg);
        return FALSE;
    }
    else
    {
        *pbHasFiles = TRUE;
    }

    return TRUE;
}

static BOOL ParseMoreVariable(BOOL* pbHasFiles)
{
    BOOL ret = TRUE;
    PWSTR psz;
    PWCHAR pch;
    DWORD cch;

    cch = GetEnvironmentVariableW(L"MORE", NULL, 0);
    if (cch == 0)
        return TRUE;

    psz = (PWSTR)malloc((cch + 1) * sizeof(WCHAR));
    if (!psz)
        return TRUE;

    if (!GetEnvironmentVariableW(L"MORE", psz, cch + 1))
    {
        free(psz);
        return TRUE;
    }

    for (pch = wcstok(psz, L" "); pch; pch = wcstok(NULL, L" "))
    {
        ret = ParseArgument(pch, pbHasFiles);
        if (!ret)
            break;
    }

    free(psz);
    return ret;
}

// INT CommandMore(LPTSTR cmd, LPTSTR param)
int wmain(int argc, WCHAR* argv[])
{
    // FIXME this stuff!
    CON_SCREEN Screen = {StdOut};
    CON_PAGER Pager = {&Screen, 0};

    int i;

    BOOL bRet, bContinue;

    ENCODING Encoding;
    DWORD SkipBytes = 0;
    BOOL HasFiles;

#define FileCacheBufferSize 4096
    PVOID FileCacheBuffer = NULL;
    PWCHAR StringBuffer = NULL;
    DWORD StringBufferLength = 0;
    DWORD dwReadBytes = 0, dwReadChars = 0;

    TCHAR szFullPath[MAX_PATH];

    hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Initialize the Console Standard Streams */
    ConStreamInit(StdIn , GetStdHandle(STD_INPUT_HANDLE) , UTF8Text, INVALID_CP);
    ConStreamInit(StdOut, GetStdHandle(STD_OUTPUT_HANDLE), UTF8Text, INVALID_CP);
    ConStreamInit(StdErr, GetStdHandle(STD_ERROR_HANDLE) , UTF8Text, INVALID_CP);

    /*
     * Bad usage (too much options) or we use the /? switch.
     * Display help for the MORE command.
     */
    if (argc > 1 && wcscmp(argv[1], L"/?") == 0)
    {
        ConResPuts(StdOut, IDS_USAGE);
        return 0;
    }

    /* Load the registry settings */
    LoadRegistrySettings(HKEY_LOCAL_MACHINE);
    LoadRegistrySettings(HKEY_CURRENT_USER);
    if (bEnableExtensions)
        s_dwFlags |= FLAG_E;

    // NOTE: We might try to duplicate the ConOut for read access... ?
    hKeyboard = CreateFileW(L"CONIN$", GENERIC_READ|GENERIC_WRITE,
                            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);
    FlushConsoleInputBuffer(hKeyboard);
    ConStreamSetOSHandle(StdIn, hKeyboard);

    FileCacheBuffer = HeapAlloc(GetProcessHeap(), 0, FileCacheBufferSize);
    if (!FileCacheBuffer)
    {
        ConPuts(StdErr, L"Error: no memory\n");
        CloseHandle(hKeyboard);
        return 1;
    }

    /* First, load the "MORE" environment variable and parse it,
     * then parse the command-line parameters. */
    HasFiles = FALSE;
    if (!ParseMoreVariable(&HasFiles))
        return 1;
    for (i = 1; i < argc; i++)
    {
        if (!ParseArgument(argv[i], &HasFiles))
            return 1;
    }

    if (s_dwFlags & FLAG_HELP)
    {
        ConResPuts(StdOut, IDS_USAGE);
        return 0;
    }

    Pager.PagerLine = MorePagerLine;
    Pager.dwFlags |= CON_PAGER_EXPAND_TABS | CON_PAGER_CACHE_INCOMPLETE_LINE;
    if (s_dwFlags & FLAG_P)
        Pager.dwFlags |= CON_PAGER_EXPAND_FF;
    Pager.nTabWidth = s_nTabWidth;

    /* Special case where we run 'MORE' without any argument: we use STDIN */
    if (!HasFiles)
    {
        /*
         * Assign STDIN handle to hFile so that the page prompt function will
         * know the data comes from STDIN, and will take different actions.
         */
        hFile = hStdIn;

        /* Update the statistics for PagePrompt */
        dwFileSize = 0;
        dwSumReadBytes = dwSumReadChars = 0;

        /* We suppose we read text from the file */

        /* For STDIN we always suppose we are in ANSI mode */
        // SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        Encoding = ENCODING_ANSI; // ENCODING_UTF8;

        /* Start paging */
        bContinue = ConWritePaging(&Pager, PagePrompt, TRUE, NULL, 0);
        if (!bContinue)
            goto Quit;

        do
        {
            bRet = FileGetString(hFile, Encoding,
                                 FileCacheBuffer, FileCacheBufferSize,
                                 &StringBuffer, &StringBufferLength,
                                 &dwReadBytes, &dwReadChars);
            if (!bRet || dwReadBytes == 0 || dwReadChars == 0)
            {
                /* We failed at reading the file, bail out */
                break;
            }

            /* Update the statistics for PagePrompt */
            dwSumReadBytes += dwReadBytes;
            dwSumReadChars += dwReadChars;

            bContinue = ConWritePaging(&Pager, PagePrompt, FALSE,
                                       StringBuffer, dwReadChars);
            /* If we Ctrl-C/Ctrl-Break, stop everything */
            if (!bContinue)
                break;
        }
        while (bRet && dwReadBytes > 0);

        /* Flush any cached pager buffers */
        if (bContinue)
            bContinue = ConWritePaging(&Pager, PagePrompt, FALSE, NULL, 0);

        goto Quit;
    }

    /* We have files: read them and output them to STDOUT */
    for (i = 1; i < argc; i++)
    {
        if (IsFlag(argv[i]))
            continue;

        GetFullPathNameW(argv[i], ARRAYSIZE(szFullPath), szFullPath, NULL);
        hFile = CreateFileW(szFullPath,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0, // FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            ConResPrintf(StdErr, IDS_FILE_ACCESS, szFullPath);
            goto Quit;
        }

        /* We currently do not support files too big */
        dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == INVALID_FILE_SIZE)
        {
            ConPuts(StdErr, L"ERROR: Invalid file size!\n");
            CloseHandle(hFile);
            continue;
        }

        /* We suppose we read text from the file */

        /* Check whether the file is UNICODE and retrieve its encoding */
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        bRet = ReadFile(hFile, FileCacheBuffer, FileCacheBufferSize, &dwReadBytes, NULL);
        IsDataUnicode(FileCacheBuffer, dwReadBytes, &Encoding, &SkipBytes);
        SetFilePointer(hFile, SkipBytes, NULL, FILE_BEGIN);

        /* Reset state for paging */
        s_nNextLineNo = 0;
        s_bPrevLineIsBlank = FALSE;
        s_fPrompt = PROMPT_PERCENT;
        s_bDoNextFile = FALSE;

        /* Update the statistics for PagePrompt */
        dwSumReadBytes = dwSumReadChars = 0;

        /* Start paging */
        bContinue = ConWritePaging(&Pager, PagePrompt, TRUE, NULL, 0);
        if (!bContinue)
        {
            /* We stop displaying this file */
            CloseHandle(hFile);
            if (s_bDoNextFile)
            {
                /* Bail out and continue with the other files */
                continue;
            }

            /* We Ctrl-C/Ctrl-Break, stop everything */
            goto Quit;
        }

        do
        {
            bRet = FileGetString(hFile, Encoding,
                                 FileCacheBuffer, FileCacheBufferSize,
                                 &StringBuffer, &StringBufferLength,
                                 &dwReadBytes, &dwReadChars);
            if (!bRet || dwReadBytes == 0 || dwReadChars == 0)
            {
                /*
                 * We failed at reading the file, bail out
                 * and continue with the other files.
                 */
                break;
            }

            /* Update the statistics for PagePrompt */
            dwSumReadBytes += dwReadBytes;
            dwSumReadChars += dwReadChars;

            if ((Encoding == ENCODING_UTF16LE) || (Encoding == ENCODING_UTF16BE))
            {
                bContinue = ConWritePaging(&Pager, PagePrompt, FALSE,
                                           FileCacheBuffer, dwReadChars);
            }
            else
            {
                bContinue = ConWritePaging(&Pager, PagePrompt, FALSE,
                                           StringBuffer, dwReadChars);
            }
            if (!bContinue)
            {
                /* We stop displaying this file */
                break;
            }
        }
        while (bRet && dwReadBytes > 0);

        /* Flush any cached pager buffers */
        if (bContinue)
            bContinue = ConWritePaging(&Pager, PagePrompt, FALSE, NULL, 0);

        CloseHandle(hFile);

        /* Check whether we should stop displaying this file */
        if (!bContinue)
        {
            if (s_bDoNextFile)
            {
                /* Bail out and continue with the other files */
                continue;
            }

            /* We Ctrl-C/Ctrl-Break, stop everything */
            goto Quit;
        }
    }

Quit:
    if (StringBuffer) HeapFree(GetProcessHeap(), 0, StringBuffer);
    HeapFree(GetProcessHeap(), 0, FileCacheBuffer);
    CloseHandle(hKeyboard);
    return 0;
}

/* EOF */
