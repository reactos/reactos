/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS More Command
 * FILE:            base/applications/cmdutils/more/more.c
 * PURPOSE:         Displays text stream from STDIN or from an arbitrary number
 *                  of files to STDOUT, with screen capabilities (more than CAT,
 *                  but less than LESS ^^).
 * PROGRAMMERS:     Paolo Pantaleo
 *                  Timothy Schepens
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
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

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h>

#include <conutils.h>

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


static BOOL
__stdcall
PagePrompt(PCON_PAGER Pager, DWORD Done, DWORD Total)
{
    HANDLE hInput = ConStreamGetOSHandle(StdIn);
    DWORD dwMode;
    KEY_EVENT_RECORD KeyEvent;

    /*
     * Just use the simple prompt if the file being displayed is the STDIN,
     * otherwise use the prompt with progress percentage.
     *
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
    if (hFile == hStdIn)
    {
        ConResPuts(Pager->Screen->Stream, IDS_CONTINUE);
    }
    else
    {
        ConResPrintf(Pager->Screen->Stream, IDS_CONTINUE_PROGRESS,
                     // (dwSumReadChars - Total + Done) * 100 / dwFileSize
                     (dwSumReadBytes - (Total - Done) *
                        (dwSumReadBytes / dwSumReadChars)) * 100 / dwFileSize
                     );
    }

    // TODO: Implement prompt read line!

    // FIXME: Does not support TTY yet!

    /* RemoveBreakHandler */
    SetConsoleCtrlHandler(NULL, TRUE);
    /* ConInDisable */
    GetConsoleMode(hInput, &dwMode);
    dwMode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);

    do
    {
        // FIXME: Does not support TTY yet!

        // ConInKey(&KeyEvent);
        INPUT_RECORD ir;
        DWORD dwRead;
        do
        {
            ReadConsoleInput(hInput, &ir, 1, &dwRead);
        }
        while ((ir.EventType != KEY_EVENT) || (!ir.Event.KeyEvent.bKeyDown));

        /* Got our key, return to caller */
        KeyEvent = ir.Event.KeyEvent;
    }
    while ((KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
           (KeyEvent.wVirtualKeyCode == VK_MENU) ||
           (KeyEvent.wVirtualKeyCode == VK_CONTROL));

    /* AddBreakHandler */
    SetConsoleCtrlHandler(NULL, FALSE);
    /* ConInEnable */
    GetConsoleMode(hInput, &dwMode);
    dwMode |= ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hInput, dwMode);

    /*
     * Erase the full line where the cursor is, and move
     * the cursor back to the beginning of the line.
     */
    ConClearLine(Pager->Screen->Stream);

    if ((KeyEvent.wVirtualKeyCode == VK_ESCAPE) ||
        ((KeyEvent.wVirtualKeyCode == L'C') &&
         (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))))
    {
        /* We break, output a newline */
        WCHAR ch = L'\n';
        ConStreamWrite(Pager->Screen->Stream, &ch, 1);
        return FALSE;
    }

    return TRUE;
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

#define FileCacheBufferSize 4096
    PVOID FileCacheBuffer = NULL;
    PWCHAR StringBuffer = NULL;
    DWORD StringBufferLength = 0;
    DWORD dwReadBytes, dwReadChars;

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

    // FIXME: Parse all the remaining parameters.
    // Then the file list can be found at the very end.
    // FIXME2: Use the PARSER api that can be found in EVENTCREATE.

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

    /* Special case where we run 'MORE' without any argument: we use STDIN */
    if (argc <= 1)
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

        bContinue = ConPutsPaging(&Pager, PagePrompt, TRUE, L"");
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
                goto Quit;
        }
        while (bRet && dwReadBytes > 0);
        goto Quit;
    }

    /* We have files: read them and output them to STDOUT */
    for (i = 1; i < argc; i++)
    {
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
            continue;
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

        /* Update the statistics for PagePrompt */
        dwSumReadBytes = dwSumReadChars = 0;

        bContinue = ConPutsPaging(&Pager, PagePrompt, TRUE, L"");
        if (!bContinue)
        {
            CloseHandle(hFile);
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
                 * We failed at reading the file, bail out and
                 * continue with the other files.
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
            /* If we Ctrl-C/Ctrl-Break, stop everything */
            if (!bContinue)
            {
                CloseHandle(hFile);
                goto Quit;
            }
        }
        while (bRet && dwReadBytes > 0);

        CloseHandle(hFile);
    }

Quit:
    if (StringBuffer) HeapFree(GetProcessHeap(), 0, StringBuffer);
    HeapFree(GetProcessHeap(), 0, FileCacheBuffer);
    CloseHandle(hKeyboard);
    return 0;
}

/* EOF */
