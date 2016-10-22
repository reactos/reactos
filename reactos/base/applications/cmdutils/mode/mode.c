/*
 *  ReactOS mode console command
 *
 *  mode.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Mode Utility
 * FILE:            base/applications/cmdutils/mode/mode.c
 * PURPOSE:         Provides fast mode setup for DOS devices.
 * PROGRAMMERS:     Robert Dickenson
 *                  Hermes Belusca-Maito
 */

#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>

#include <conutils.h>

#include "resource.h"

#define MAX_PORTNAME_LEN 20
#define MAX_COMPORT_NUM  10

#define ASSERT(a)

/*** For fixes, see also network/net/main.c ***/
// VOID PrintPadding(...)
INT
__cdecl
UnderlinedResPrintf(
    IN PCON_STREAM Stream,
    IN UINT uID,
    ...)
{
    INT Len;
    va_list args;

#define MAX_BUFFER_SIZE 4096
    INT i;
    WCHAR szMsgBuffer[MAX_BUFFER_SIZE];

    va_start(args, uID);
    Len = ConResPrintfV(Stream, uID, args);
    va_end(args);

    ConPuts(Stream, L"\n");
    for (i = 0; i < Len; i++)
         szMsgBuffer[i] = L'-';
    szMsgBuffer[Len] = UNICODE_NULL;

    // FIXME: Use ConStreamWrite instead.
    ConPuts(Stream, szMsgBuffer);

    return Len;
}

int QueryDevices(VOID)
{
    WCHAR buffer[20240];
    WCHAR* ptr = buffer;

    *ptr = L'\0';
    // FIXME: Dynamically allocate 'buffer' in a loop.
    if (QueryDosDeviceW(NULL, buffer, ARRAYSIZE(buffer)))
    {
        while (*ptr != L'\0')
        {
            if (wcsstr(ptr, L"COM"))
            {
                ConResPrintf(StdOut, IDS_QUERY_SERIAL_FOUND, ptr);
            }
            else if (wcsstr(ptr, L"PRN"))
            {
                ConResPrintf(StdOut, IDS_QUERY_PRINTER_FOUND, ptr);
            }
            else if (wcsstr(ptr, L"LPT"))
            {
                ConResPrintf(StdOut, IDS_QUERY_PARALLEL_FOUND, ptr);
            }
            else if (wcsstr(ptr, L"AUX") || wcsstr(ptr, L"NUL"))
            {
                ConResPrintf(StdOut, IDS_QUERY_DOSDEV_FOUND, ptr);
            }
            else
            {
                // ConResPrintf(StdOut, IDS_QUERY_MISC_FOUND, ptr);
            }
            ptr += (wcslen(ptr) + 1);
        }
    }
    else
    {
        wprintf(L"ERROR: QueryDosDeviceW(...) failed: 0x%lx\n", GetLastError());
    }
    return 1;
}

int ShowParallelStatus(INT nPortNum)
{
    WCHAR buffer[250];
    WCHAR szPortName[MAX_PORTNAME_LEN];

    swprintf(szPortName, L"LPT%d", nPortNum);

    ConPuts(StdOut, L"\n");
    UnderlinedResPrintf(StdOut, IDS_DEVICE_STATUS_HEADER, szPortName);
    ConPuts(StdOut, L"\n");

    if (QueryDosDeviceW(szPortName, buffer, ARRAYSIZE(buffer)))
    {
        WCHAR* ptr = wcsrchr(buffer, L'\\');
        if (ptr != NULL)
        {
            if (_wcsicmp(szPortName, ++ptr) == 0)
                ConResPuts(StdOut, IDS_PRINTER_OUTPUT_NOT_REROUTED);
            else
                ConResPrintf(StdOut, IDS_PRINTER_OUTPUT_REROUTED_SERIAL, ptr);

            return 0;
        }
        else
        {
            wprintf(L"    QueryDosDeviceW(%s) returned unrecognised form %s.\n", szPortName, buffer);
        }
    }
    else
    {
        wprintf(L"ERROR: QueryDosDeviceW(%s) failed: 0x%lx\n", szPortName, GetLastError());
    }

    return 1;
}

int SetParallelState(INT nPortNum)
{
    WCHAR szPortName[MAX_PORTNAME_LEN];
    WCHAR szTargetPath[MAX_PORTNAME_LEN];

    swprintf(szPortName, L"LPT%d", nPortNum);
    swprintf(szTargetPath, L"COM%d", nPortNum);
    if (!DefineDosDeviceW(DDD_REMOVE_DEFINITION, szPortName, szTargetPath))
    {
        wprintf(L"SetParallelState(%d) - DefineDosDevice(%s) failed: 0x%lx\n", nPortNum, szPortName, GetLastError());
    }

    ShowParallelStatus(nPortNum);
    return 0;
}


static PCWSTR
ParseNumber(PCWSTR argStr, PDWORD Number)
{
    INT value, skip = 0;

    value = swscanf(argStr, L"%lu%n", Number, &skip);
    if (!value) return NULL;
    argStr += skip;
    return argStr;
}


/*
    \??\COM1
    \Device\NamedPipe\Spooler\LPT1
BOOL DefineDosDevice(
  DWORD dwFlags,         // options
  LPCTSTR lpDeviceName,  // device name
  LPCTSTR lpTargetPath   // path string
);
DWORD QueryDosDevice(
  LPCTSTR lpDeviceName, // MS-DOS device name string
  LPTSTR lpTargetPath,  // query results buffer
  DWORD ucchMax         // maximum size of buffer
);
 */


/*****************************************************************************\
 **                      C O N S O L E   H E L P E R S                      **
\*****************************************************************************/

int ShowConsoleStatus(VOID)
{
    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwKbdDelay, dwKbdSpeed;

    ConPuts(StdOut, L"\n");
    UnderlinedResPrintf(StdOut, IDS_DEVICE_STATUS_HEADER, L"CON");
    ConPuts(StdOut, L"\n");

    if (GetConsoleScreenBufferInfo(hConOut, &csbi))
    {
        ConResPrintf(StdOut, IDS_CONSOLE_STATUS_LINES, csbi.dwSize.Y);
        ConResPrintf(StdOut, IDS_CONSOLE_STATUS_COLS , csbi.dwSize.X);
    }
    if (SystemParametersInfoW(SPI_GETKEYBOARDSPEED, 0, &dwKbdSpeed, 0))
    {
        ConResPrintf(StdOut, IDS_CONSOLE_KBD_RATE, dwKbdSpeed);
    }
    if (SystemParametersInfoW(SPI_GETKEYBOARDDELAY, 0, &dwKbdDelay, 0))
    {
        ConResPrintf(StdOut, IDS_CONSOLE_KBD_DELAY, dwKbdDelay);
    }
    ConResPrintf(StdOut, IDS_CONSOLE_CODEPAGE, GetConsoleOutputCP());
    return 0;
}

int ShowConsoleCPStatus(VOID)
{
    ConPuts(StdOut, L"\n");
    UnderlinedResPrintf(StdOut, IDS_DEVICE_STATUS_HEADER, L"CON");
    ConPuts(StdOut, L"\n");

    ConResPrintf(StdOut, IDS_CONSOLE_CODEPAGE, GetConsoleOutputCP());
    return 0;
}

static VOID
ClearScreen(
    IN HANDLE hConOut,
    IN PCONSOLE_SCREEN_BUFFER_INFO pcsbi)
{
    COORD coPos;
    DWORD dwWritten;

    coPos.X = 0;
    coPos.Y = 0;
    FillConsoleOutputAttribute(hConOut, pcsbi->wAttributes,
                               pcsbi->dwSize.X * pcsbi->dwSize.Y,
                               coPos, &dwWritten);
    FillConsoleOutputCharacterW(hConOut, L' ',
                                pcsbi->dwSize.X * pcsbi->dwSize.Y,
                                coPos, &dwWritten);
    SetConsoleCursorPosition(hConOut, coPos);
}

/*
 * See, or adjust if needed, subsystems/mvdm/ntvdm/console/video.c!ResizeTextConsole()
 * for more information.
 */
static BOOL
ResizeTextConsole(
    IN HANDLE hConOut,
    IN OUT PCONSOLE_SCREEN_BUFFER_INFO pcsbi,
    IN COORD Resolution)
{
    BOOL Success;
    SHORT Width, Height;
    SMALL_RECT ConRect;

    /*
     * Use this trick to effectively resize the console buffer and window,
     * because:
     * - SetConsoleScreenBufferSize fails if the new console screen buffer size
     *   is smaller than the current console window size, and:
     * - SetConsoleWindowInfo fails if the new console window size is larger
     *   than the current console screen buffer size.
     */

    /* Resize the screen buffer only if needed */
    if (Resolution.X != pcsbi->dwSize.X || Resolution.Y != pcsbi->dwSize.Y)
    {
        Width  = pcsbi->srWindow.Right  - pcsbi->srWindow.Left + 1;
        Height = pcsbi->srWindow.Bottom - pcsbi->srWindow.Top  + 1;

        /*
         * If the current console window is too large for
         * the new screen buffer, resize it first.
         */
        if (Width > Resolution.X || Height > Resolution.Y)
        {
            /*
             * NOTE: This is not a problem if we move the window back to (0,0)
             * because when we resize the screen buffer, the window will move back
             * to where the cursor is. Or, if the screen buffer is not resized,
             * when we readjust again the window, we will move back to a correct
             * position. This is what we wanted after all...
             */
            ConRect.Left   = ConRect.Top = 0;
            ConRect.Right  = ConRect.Left + min(Width , Resolution.X) - 1;
            ConRect.Bottom = ConRect.Top  + min(Height, Resolution.Y) - 1;

            Success = SetConsoleWindowInfo(hConOut, TRUE, &ConRect);
            if (!Success) return FALSE;
        }

        /*
         * Now resize the screen buffer.
         *
         * SetConsoleScreenBufferSize automatically takes into account the current
         * cursor position when it computes starting which row it should copy text
         * when resizing the screen buffer, and scrolls the console window such that
         * the cursor is placed in it again. We therefore do not need to care about
         * the cursor position and do the maths ourselves.
         */
        Success = SetConsoleScreenBufferSize(hConOut, Resolution);
        if (!Success) return FALSE;

        /*
         * Setting a new screen buffer size can change other information,
         * so update the console screen buffer information.
         */
        GetConsoleScreenBufferInfo(hConOut, pcsbi);
    }

    /* Always resize the console window within the permitted maximum size */
    Width  = min(Resolution.X, pcsbi->dwMaximumWindowSize.X);
    Height = min(Resolution.Y, pcsbi->dwMaximumWindowSize.Y);
    ConRect.Left   = 0;
    ConRect.Right  = ConRect.Left + Width - 1;
    ConRect.Bottom = max(pcsbi->dwCursorPosition.Y, Height - 1);
    ConRect.Top    = ConRect.Bottom - Height + 1;

    SetConsoleWindowInfo(hConOut, TRUE, &ConRect);

    /* Update the console screen buffer information */
    GetConsoleScreenBufferInfo(hConOut, pcsbi);
    return TRUE;
}

int SetConsoleStateOld(IN PCWSTR ArgStr)
{
    PCWSTR argStr = ArgStr;

    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD Resolution;
    DWORD value;

    if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
    {
        // TODO: Error message?
        return 0;
    }

    Resolution = csbi.dwSize;

    /* Parse the column number (only MANDATORY argument) */
    value = 0;
    argStr = ParseNumber(argStr, &value);
    if (!argStr) goto invalid_parameter;
    Resolution.X = (SHORT)value;

    /* Parse the line number (OPTIONAL argument) */
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;
    if (*argStr++ != L',') goto invalid_parameter;
    while (*argStr == L' ') argStr++;

    value = 0;
    argStr = ParseNumber(argStr, &value);
    if (!argStr) goto invalid_parameter;
    Resolution.Y = (SHORT)value;

    /* This should be the end of the string */
    while (*argStr == L' ') argStr++;
    if (*argStr) goto invalid_parameter;

Quit:
    ClearScreen(hConOut, &csbi);
    if (!ResizeTextConsole(hConOut, &csbi, Resolution))
        wprintf(L"The screen cannot be set to the number of lines and columns specified.\n");

    return 0;

invalid_parameter:
    wprintf(L"Invalid parameter - %s\n", ArgStr);
    return 1;
}

int SetConsoleState(IN PCWSTR ArgStr)
{
    PCWSTR argStr = ArgStr;
    BOOL dispMode = FALSE, kbdMode = FALSE;

    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD Resolution;
    DWORD dwKbdDelay, dwKbdSpeed;
    DWORD value;

    if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
    {
        // TODO: Error message?
        return 0;
    }
    if (!SystemParametersInfoW(SPI_GETKEYBOARDDELAY, 0, &dwKbdDelay, 0))
    {
        // TODO: Error message?
        return 0;
    }
    if (!SystemParametersInfoW(SPI_GETKEYBOARDSPEED, 0, &dwKbdSpeed, 0))
    {
        // TODO: Error message?
        return 0;
    }

    Resolution = csbi.dwSize;

    while (argStr && *argStr)
    {
        while (*argStr == L' ') argStr++;
        if (!*argStr) break;

        if (!kbdMode && _wcsnicmp(argStr, L"COLS=", 5) == 0)
        {
            dispMode = TRUE;

            value = 0;
            argStr = ParseNumber(argStr+5, &value);
            if (!argStr) goto invalid_parameter;
            Resolution.X = (SHORT)value;
        }
        else if (!kbdMode && _wcsnicmp(argStr, L"LINES=", 6) == 0)
        {
            dispMode = TRUE;

            value = 0;
            argStr = ParseNumber(argStr+6, &value);
            if (!argStr) goto invalid_parameter;
            Resolution.Y = (SHORT)value;
        }
        else if (!dispMode && _wcsnicmp(argStr, L"RATE=", 5) == 0)
        {
            kbdMode = TRUE;

            argStr = ParseNumber(argStr+5, &dwKbdSpeed);
            if (!argStr) goto invalid_parameter;
        }
        else if (!dispMode && _wcsnicmp(argStr, L"DELAY=", 6) == 0)
        {
            kbdMode = TRUE;

            argStr = ParseNumber(argStr+6, &dwKbdDelay);
            if (!argStr) goto invalid_parameter;
        }
        else
        {
invalid_parameter:
            wprintf(L"Invalid parameter - %s\n", ArgStr);
            return 1;
        }
    }

    if (dispMode)
    {
        ClearScreen(hConOut, &csbi);
        if (!ResizeTextConsole(hConOut, &csbi, Resolution))
            wprintf(L"The screen cannot be set to the number of lines and columns specified.\n");
    }
    else if (kbdMode)
    {
        /*
         * Set the new keyboard settings. If those values are greater than
         * their allowed range, they are automatically corrected as follows:
         *   dwKbdSpeed = min(dwKbdSpeed, 31);
         *   dwKbdDelay = (dwKbdDelay % 4);
         */
        SystemParametersInfoW(SPI_SETKEYBOARDDELAY, dwKbdDelay, NULL, 0);
        // "Invalid keyboard delay."
        SystemParametersInfoW(SPI_SETKEYBOARDSPEED, dwKbdSpeed, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        // "Invalid keyboard rate."
    }

    return 0;
}

int SetConsoleCPState(IN PCWSTR ArgStr)
{
    PCWSTR argStr = ArgStr;
    DWORD CodePage = 0;

    if ( (_wcsnicmp(argStr, L"SELECT=", 7) == 0 && (argStr += 7)) ||
         (_wcsnicmp(argStr, L"SEL=", 4) == 0 && (argStr += 4)) )
    {
        argStr = ParseNumber(argStr, &CodePage);
        if (!argStr) goto invalid_parameter;

        /* This should be the end of the string */
        while (*argStr == L' ') argStr++;
        if (*argStr) goto invalid_parameter;

        SetConsoleCP(CodePage);
        SetConsoleOutputCP(CodePage);
        // "The code page specified is not valid."
        ShowConsoleCPStatus();
    }
    else
    {
invalid_parameter:
        wprintf(L"Invalid parameter - %s\n", ArgStr);
        return 1;
    }

    return 0;
}


/*****************************************************************************\
 **                  S E R I A L   P O R T   H E L P E R S                  **
\*****************************************************************************/

static BOOL
SerialPortQuery(INT nPortNum, LPDCB pDCB, LPCOMMTIMEOUTS pCommTimeouts, BOOL bWrite)
{
    BOOL Success;
    HANDLE hPort;
    WCHAR szPortName[MAX_PORTNAME_LEN];

    ASSERT(pDCB);
    ASSERT(pCommTimeouts);

    swprintf(szPortName, L"COM%d", nPortNum);
    hPort = CreateFileW(szPortName,
                        bWrite ? GENERIC_WRITE : GENERIC_READ,
                        0,     // exclusive
                        NULL,  // sec attr
                        OPEN_EXISTING,
                        0,     // no attributes
                        NULL); // no template

    if (hPort == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Illegal device name - %s\n", szPortName);
        wprintf(L"Last error = 0x%lx\n", GetLastError());
        return FALSE;
    }

    Success = bWrite ? SetCommState(hPort, pDCB)
                     : GetCommState(hPort, pDCB);
    if (!Success)
    {
        wprintf(L"Failed to %s the status for device COM%d:\n", bWrite ? L"set" : L"get", nPortNum);
        goto Quit;
    }

    Success = bWrite ? SetCommTimeouts(hPort, pCommTimeouts)
                     : GetCommTimeouts(hPort, pCommTimeouts);
    if (!Success)
    {
        wprintf(L"Failed to %s timeout status for device COM%d:\n", bWrite ? L"set" : L"get", nPortNum);
        goto Quit;
    }

Quit:
    CloseHandle(hPort);
    return Success;
}

int ShowSerialStatus(INT nPortNum)
{
    static const LPCWSTR parity_strings[] =
    {
        L"None",    // NOPARITY
        L"Odd",     // ODDPARITY
        L"Even",    // EVENPARITY
        L"Mark",    // MARKPARITY
        L"Space"    // SPACEPARITY
    };
    static const LPCWSTR control_strings[] = { L"OFF", L"ON", L"HANDSHAKE", L"TOGGLE" };
    static const LPCWSTR stopbit_strings[] = { L"1", L"1.5", L"2" };

    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
    WCHAR szPortName[MAX_PORTNAME_LEN];

    if (!SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE))
    {
        return 1;
    }
    if (dcb.Parity >= ARRAYSIZE(parity_strings))
    {
        wprintf(L"ERROR: Invalid value for Parity Bits %d:\n", dcb.Parity);
        dcb.Parity = 0;
    }
    if (dcb.StopBits >= ARRAYSIZE(stopbit_strings))
    {
        wprintf(L"ERROR: Invalid value for Stop Bits %d:\n", dcb.StopBits);
        dcb.StopBits = 0;
    }

    swprintf(szPortName, L"COM%d", nPortNum);

    ConPuts(StdOut, L"\n");
    UnderlinedResPrintf(StdOut, IDS_DEVICE_STATUS_HEADER, szPortName);
    ConPuts(StdOut, L"\n");

    ConResPrintf(StdOut, IDS_COM_STATUS_BAUD, dcb.BaudRate);
    ConResPrintf(StdOut, IDS_COM_STATUS_PARITY, parity_strings[dcb.Parity]);
    ConResPrintf(StdOut, IDS_COM_STATUS_DATA_BITS, dcb.ByteSize);
    ConResPrintf(StdOut, IDS_COM_STATUS_STOP_BITS, stopbit_strings[dcb.StopBits]);
    ConResPrintf(StdOut, IDS_COM_STATUS_TIMEOUT,
        control_strings[(CommTimeouts.ReadTotalTimeoutConstant  != 0) ||
                        (CommTimeouts.WriteTotalTimeoutConstant != 0) ? 1 : 0]);
    ConResPrintf(StdOut, IDS_COM_STATUS_XON_XOFF,
        control_strings[dcb.fOutX ? 1 : 0]);
    ConResPrintf(StdOut, IDS_COM_STATUS_CTS_HANDSHAKING,
        control_strings[dcb.fOutxCtsFlow ? 1 : 0]);
    ConResPrintf(StdOut, IDS_COM_STATUS_DSR_HANDSHAKING,
        control_strings[dcb.fOutxDsrFlow ? 1 : 0]);
    ConResPrintf(StdOut, IDS_COM_STATUS_DSR_SENSITIVITY,
        control_strings[dcb.fDsrSensitivity ? 1 : 0]);
    ConResPrintf(StdOut, IDS_COM_STATUS_DTR_CIRCUIT, control_strings[dcb.fDtrControl]);
    ConResPrintf(StdOut, IDS_COM_STATUS_RTS_CIRCUIT, control_strings[dcb.fRtsControl]);
    return 0;
}


/*
 * Those procedures are inspired from Wine's dll/win32/kernel32/wine/comm.c
 * Copyright 1996 Erik Bos and Marcus Meissner.
 */

static PCWSTR
ParseModes(PCWSTR argStr, PBYTE Mode)
{
    if (_wcsnicmp(argStr, L"OFF", 3) == 0)
    {
        argStr += 3;
        *Mode = 0;
    }
    else if (_wcsnicmp(argStr, L"ON", 2) == 0)
    {
        argStr += 2;
        *Mode = 1;
    }
    else if (_wcsnicmp(argStr, L"HS", 2) == 0)
    {
        argStr += 2;
        *Mode = 2;
    }
    else if (_wcsnicmp(argStr, L"TG", 2) == 0)
    {
        argStr += 2;
        *Mode = 3;
    }

    return NULL;
}

static PCWSTR
ParseBaudRate(PCWSTR argStr, PDWORD BaudRate)
{
    argStr = ParseNumber(argStr, BaudRate);
    if (!argStr) return NULL;

    /*
     * Check for Baud Rate abbreviations. This means that using
     * those values as real baud rates is impossible using MODE.
     */
    switch (*BaudRate)
    {
        /* BaudRate = 110, 150, 300, 600 */
        case 11: case 15: case 30: case 60:
            *BaudRate *= 10;
            break;

        /* BaudRate = 1200, 2400, 4800, 9600 */
        case 12: case 24: case 48: case 96:
            *BaudRate *= 100;
            break;

        case 19:
            *BaudRate = 19200;
            break;
    }

    return argStr;
}

static PCWSTR
ParseParity(PCWSTR argStr, PBYTE Parity)
{
    switch (towupper(*argStr++))
    {
        case L'N':
            *Parity = NOPARITY;
            break;

        case L'O':
            *Parity = ODDPARITY;
            break;

        case L'E':
            *Parity = EVENPARITY;
            break;

        case L'M':
            *Parity = MARKPARITY;
            break;

        case L'S':
            *Parity = SPACEPARITY;
            break;

        default:
            return NULL;
    }

    return argStr;
}

static PCWSTR
ParseByteSize(PCWSTR argStr, PBYTE ByteSize)
{
    DWORD value = 0;

    argStr = ParseNumber(argStr, &value);
    if (!argStr) return NULL;

    *ByteSize = (BYTE)value;
    if (*ByteSize < 5 || *ByteSize > 8)
        return NULL;

    return argStr;
}

static PCWSTR
ParseStopBits(PCWSTR argStr, PBYTE StopBits)
{
    if (_wcsnicmp(argStr, L"1.5", 3) == 0)
    {
        argStr += 3;
        *StopBits = ONE5STOPBITS;
    }
    else
    {
        if (*argStr == L'1')
            *StopBits = ONESTOPBIT;
        else if (*argStr == L'2')
            *StopBits = TWOSTOPBITS;
        else
            return NULL;

        argStr++;
    }

    return argStr;
}

/*
 * Build a DCB using the old style settings string eg: "96,n,8,1"
 *
 * See dll/win32/kernel32/wine/comm.c!COMM_BuildOldCommDCB()
 * for more information.
 */
static BOOL
BuildOldCommDCB(
    OUT LPDCB pDCB,
    IN PCWSTR ArgStr)
{
    PCWSTR argStr = ArgStr;
    BOOL stop = FALSE;

    /*
     * Parse the baud rate (only MANDATORY argument)
     */
    argStr = ParseBaudRate(argStr, &pDCB->BaudRate);
    if (!argStr) return FALSE;


    /*
     * Now parse the rest (OPTIONAL arguments)
     */

    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;
    if (*argStr++ != L',') return FALSE;
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;

    /* Parse the parity */
    // Default: EVENPARITY
    pDCB->Parity = EVENPARITY;
    if (*argStr != L',')
    {
        argStr = ParseParity(argStr, &pDCB->Parity);
        if (!argStr) return FALSE;
    }

    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;
    if (*argStr++ != L',') return FALSE;
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;

    /* Parse the data bits */
    // Default: 7
    pDCB->ByteSize = 7;
    if (*argStr != L',')
    {
        argStr = ParseByteSize(argStr, &pDCB->ByteSize);
        if (!argStr) return FALSE;
    }

    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;
    if (*argStr++ != L',') return FALSE;
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;

    /* Parse the stop bits */
    // Default: 1, or 2 for BAUD=110
    // pDCB->StopBits = ONESTOPBIT;
    if (*argStr != L',')
    {
        stop = TRUE;
        argStr = ParseStopBits(argStr, &pDCB->StopBits);
        if (!argStr) return FALSE;
    }

    /* The last parameter (flow control "retry") is really optional */
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;
    if (*argStr++ != L',') return FALSE;
    while (*argStr == L' ') argStr++;
    if (!*argStr) goto Quit;

Quit:
    switch (towupper(*argStr))
    {
        case L'\0':
            pDCB->fInX  = FALSE;
            pDCB->fOutX = FALSE;
            pDCB->fOutxCtsFlow = FALSE;
            pDCB->fOutxDsrFlow = FALSE;
            pDCB->fDtrControl  = DTR_CONTROL_ENABLE;
            pDCB->fRtsControl  = RTS_CONTROL_ENABLE;
            break;

        case L'X':
            pDCB->fInX  = TRUE;
            pDCB->fOutX = TRUE;
            pDCB->fOutxCtsFlow = FALSE;
            pDCB->fOutxDsrFlow = FALSE;
            pDCB->fDtrControl  = DTR_CONTROL_ENABLE;
            pDCB->fRtsControl  = RTS_CONTROL_ENABLE;
            break;

        case L'P':
            pDCB->fInX  = FALSE;
            pDCB->fOutX = FALSE;
            pDCB->fOutxCtsFlow = TRUE;
            pDCB->fOutxDsrFlow = TRUE;
            pDCB->fDtrControl  = DTR_CONTROL_HANDSHAKE;
            pDCB->fRtsControl  = RTS_CONTROL_HANDSHAKE;
            break;

        default:
            /* Unsupported */
            return FALSE;
    }
    if (*argStr) argStr++;

    /* This should be the end of the string */
    while (*argStr == L' ') argStr++;
    if (*argStr) return FALSE;

    /* If stop bits were not specified, a default is always supplied */
    if (!stop)
    {
        if (pDCB->BaudRate == 110)
            pDCB->StopBits = TWOSTOPBITS;
        else
            pDCB->StopBits = ONESTOPBIT;
    }
    return TRUE;
}

/*
 * Build a DCB using the new style settings string.
 * eg: "baud=9600 parity=n data=8 stop=1 xon=on to=on"
 *
 * See dll/win32/kernel32/wine/comm.c!COMM_BuildNewCommDCB()
 * for more information.
 */
static BOOL
BuildNewCommDCB(
    OUT LPDCB pDCB,
    OUT LPCOMMTIMEOUTS pCommTimeouts,
    IN PCWSTR ArgStr)
{
    PCWSTR argStr = ArgStr;
    BOOL baud = FALSE, stop = FALSE;
    BYTE value;

    while (argStr && *argStr)
    {
        while (*argStr == L' ') argStr++;
        if (!*argStr) break;

        if (_wcsnicmp(argStr, L"BAUD=", 5) == 0)
        {
            baud = TRUE;
            argStr = ParseBaudRate(argStr+5, &pDCB->BaudRate);
            if (!argStr) return FALSE;
        }
        else if (_wcsnicmp(argStr, L"PARITY=", 7) == 0)
        {
            // Default: EVENPARITY
            argStr = ParseParity(argStr+7, &pDCB->Parity);
            if (!argStr) return FALSE;
        }
        else if (_wcsnicmp(argStr, L"DATA=", 5) == 0)
        {
            // Default: 7
            argStr = ParseByteSize(argStr+5, &pDCB->ByteSize);
            if (!argStr) return FALSE;
        }
        else if (_wcsnicmp(argStr, L"STOP=", 5) == 0)
        {
            // Default: 1, or 2 for BAUD=110
            stop = TRUE;
            argStr = ParseStopBits(argStr+5, &pDCB->StopBits);
            if (!argStr) return FALSE;
        }
        else if (_wcsnicmp(argStr, L"TO=", 3) == 0) // TO=ON|OFF
        {
            /* Only total time-outs are get/set by Windows' MODE.COM */
            argStr = ParseModes(argStr+3, &value);
            if (!argStr) return FALSE;
            if (value == 0) // OFF
            {
                pCommTimeouts->ReadTotalTimeoutConstant  = 0;
                pCommTimeouts->WriteTotalTimeoutConstant = 0;
            }
            else if (value == 1) // ON
            {
                pCommTimeouts->ReadTotalTimeoutConstant  = 60000;
                pCommTimeouts->WriteTotalTimeoutConstant = 60000;
            }
            else
            {
                return FALSE;
            }
        }
        else if (_wcsnicmp(argStr, L"XON=", 4) == 0) // XON=ON|OFF
        {
            argStr = ParseModes(argStr+4, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1))
            {
                pDCB->fOutX = value;
                pDCB->fInX  = value;
            }
            else
            {
                return FALSE;
            }
        }
        else if (_wcsnicmp(argStr, L"ODSR=", 5) == 0) // ODSR=ON|OFF
        {
            value = 0;
            argStr = ParseModes(argStr+5, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1))
                pDCB->fOutxDsrFlow = value;
            else
                return FALSE;
        }
        else if (_wcsnicmp(argStr, L"OCTS=", 5) == 0) // OCTS=ON|OFF
        {
            value = 0;
            argStr = ParseModes(argStr+5, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1))
                pDCB->fOutxCtsFlow = value;
            else
                return FALSE;
        }
        else if (_wcsnicmp(argStr, L"DTR=", 4) == 0) // DTR=ON|OFF|HS
        {
            value = 0;
            argStr = ParseModes(argStr+4, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1) || (value == 2))
                pDCB->fDtrControl = value;
            else
                return FALSE;
        }
        else if (_wcsnicmp(argStr, L"RTS=", 4) == 0) // RTS=ON|OFF|HS|TG
        {
            value = 0;
            argStr = ParseModes(argStr+4, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1) || (value == 2) || (value == 3))
                pDCB->fRtsControl = value;
            else
                return FALSE;
        }
        else if (_wcsnicmp(argStr, L"IDSR=", 5) == 0) // IDSR=ON|OFF
        {
            value = 0;
            argStr = ParseModes(argStr+5, &value);
            if (!argStr) return FALSE;
            if ((value == 0) || (value == 1))
                pDCB->fDsrSensitivity = value;
            else
                return FALSE;
        }
        else
        {
            return FALSE;
        }
    }

    /* If stop bits were not specified, a default is always supplied */
    if (!stop)
    {
        if (baud && pDCB->BaudRate == 110)
            pDCB->StopBits = TWOSTOPBITS;
        else
            pDCB->StopBits = ONESTOPBIT;
    }
    return TRUE;
}

int SetSerialState(INT nPortNum, IN PCWSTR ArgStr)
{
    BOOL Success;
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;

    if (!SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE))
    {
        // TODO: Error message?
        return 0;
    }

    /*
     * Check whether we should use the old or the new MODE syntax:
     * in the old syntax, the separators are both spaces and commas.
     */
    if (wcschr(ArgStr, L','))
        Success = BuildOldCommDCB(&dcb, ArgStr);
    else
        Success = BuildNewCommDCB(&dcb, &CommTimeouts, ArgStr);

    if (!Success)
    {
        wprintf(L"Invalid parameter - %s\n", ArgStr);
        return 1;
    }

    SerialPortQuery(nPortNum, &dcb, &CommTimeouts, TRUE);
    ShowSerialStatus(nPortNum);

    return 0;
}


/*****************************************************************************\
 **                          E N T R Y   P O I N T                          **
\*****************************************************************************/

static PCWSTR
FindPortNum(PCWSTR argStr, PINT PortNum)
{
    *PortNum = -1;

    if (*argStr >= L'0' && *argStr <= L'9')
    {
        *PortNum = *argStr - L'0';
        argStr++;
        if (*argStr >= L'0' && *argStr <= L'9')
        {
            *PortNum *= 10;
            *PortNum += *argStr - L'0';
        }
    }
    else
    {
        return NULL;
    }

    return argStr;
}

int wmain(int argc, WCHAR* argv[])
{
    int ret = 0;
    int arg;
    SIZE_T ArgStrSize;
    PCWSTR ArgStr, argStr;

    INT nPortNum;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /*
     * MODE.COM has a very peculiar way of parsing its arguments,
     * as they can be even not separated by any space. This extreme
     * behaviour certainly is present for backwards compatibility
     * with the oldest versions of the utility present on MS-DOS.
     *
     * For example, such a command:
     *   "MODE.COM COM1baud=9600parity=ndata=8stop=1xon=onto=on"
     * will be correctly understood as:
     *   "MODE.COM COM1 baud=9600 parity=n data=8 stop=1 xon=on to=on"
     *
     * Note also that the "/STATUS" switch is actually really "/STA".
     *
     * However we will not use GetCommandLine() because we do not want
     * to deal with the prepended application path and try to find
     * where the arguments start. Our approach here will consist in
     * flattening the arguments vector.
     */
    ArgStrSize = 0;

    /* Compute space needed for the new string, and allocate it */
    for (arg = 1; arg < argc; arg++)
    {
        ArgStrSize += wcslen(argv[arg]) + 1; // 1 for space
    }
    ArgStr = HeapAlloc(GetProcessHeap(), 0, (ArgStrSize + 1) * sizeof(WCHAR));
    if (ArgStr == NULL)
    {
        wprintf(L"ERROR: Not enough memory\n");
        return 1;
    }

    /* Copy the contents and NULL-terminate the string */
    argStr = ArgStr;
    for (arg = 1; arg < argc; arg++)
    {
        wcscpy((PWSTR)argStr, argv[arg]);
        argStr += wcslen(argv[arg]);
        *(PWSTR)argStr++ = L' ';
    }
    *(PWSTR)argStr = L'\0';

    /* Parse the command line */
    argStr = ArgStr;

    while (*argStr == L' ') argStr++;
    if (!*argStr) goto show_status;

    if (wcsstr(argStr, L"/?") || wcsstr(argStr, L"-?"))
    {
        ConResPuts(StdOut, IDS_USAGE);
        goto Quit;
    }
    else if (_wcsnicmp(argStr, L"/STA", 4) == 0)
    {
        // FIXME: Check if there are other "parameters" after the status,
        // in which case this is invalid.
        goto show_status;
    }
    else if (_wcsnicmp(argStr, L"LPT", 3) == 0)
    {
        argStr = FindPortNum(argStr+3, &nPortNum);
        if (!argStr || nPortNum == -1)
            goto invalid_parameter;

        if (*argStr == L':') argStr++;
        while (*argStr == L' ') argStr++;

        ret = ShowParallelStatus(nPortNum);
        goto Quit;
    }
    else if (_wcsnicmp(argStr, L"COM", 3) == 0)
    {
        argStr = FindPortNum(argStr+3, &nPortNum);
        if (!argStr || nPortNum == -1)
            goto invalid_parameter;

        if (*argStr == L':') argStr++;
        while (*argStr == L' ') argStr++;

        if (!*argStr || _wcsnicmp(argStr, L"/STA", 4) == 0)
            ret = ShowSerialStatus(nPortNum);
        else
            ret = SetSerialState(nPortNum, argStr);
        goto Quit;
    }
    else if (_wcsnicmp(argStr, L"CON", 3) == 0)
    {
        argStr += 3;

        if (*argStr == L':') argStr++;
        while (*argStr == L' ') argStr++;

        if (!*argStr || _wcsnicmp(argStr, L"/STA", 4) == 0)
        {
            ret = ShowConsoleStatus();
        }
        else if ( (_wcsnicmp(argStr, L"CP", 2) == 0 && (argStr += 2)) ||
                  (_wcsnicmp(argStr, L"CODEPAGE", 8) == 0 && (argStr += 8)) )
        {
            while (*argStr == L' ') argStr++;

            if (!*argStr || _wcsnicmp(argStr, L"/STA", 4) == 0)
                ret = ShowConsoleCPStatus();
            else
                ret = SetConsoleCPState(argStr);
        }
        else
        {
            ret = SetConsoleState(argStr);
        }
        goto Quit;
    }
    // else if (wcschr(argStr, L','))
    else
    {
        /* Old syntax: MODE [COLS],[LINES] */
        ret = SetConsoleStateOld(argStr);
        goto Quit;
    }

show_status:
    QueryDevices();
/*
    ShowParallelStatus(1);
    for (nPortNum = 0; nPortNum < MAX_COMPORT_NUM; nPortNum++)
    {
        ShowSerialStatus(nPortNum + 1);
    }
    ShowConsoleStatus();
*/
    goto Quit;

invalid_parameter:
    wprintf(L"Invalid parameter - %s\n", ArgStr);
    goto Quit;

Quit:
    /* Free the string and quit */
    HeapFree(GetProcessHeap(), 0, (PWSTR)ArgStr);
    return ret;
}
