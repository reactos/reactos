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

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>
#include <stdio.h>

#define MAX_PORTNAME_LEN 20
#define MAX_COMPORT_NUM  10
#define MAX_COMPARAM_LEN 20

#define ASSERT(a)

const WCHAR* const usage_strings[] =
{
    L"Device Status:     MODE [device] [/STATUS]",
    L"Select code page:  MODE CON[:] CP SELECT=yyy",
    L"Code page status:  MODE CON[:] CP [/STATUS]",
    L"Display mode:      MODE CON[:] [COLS=c] [LINES=n]",
    L"Typematic rate:    MODE CON[:] [RATE=r DELAY=d]",
    L"Redirect printing: MODE LPTn[:]=COMm[:]",
    L"Serial port:       MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s]\n" \
    L"                            [to=on|off] [xon=on|off] [odsr=on|off]\n"    \
    L"                            [octs=on|off] [dtr=on|off|hs]\n"             \
    L"                            [rts=on|off|hs|tg] [idsr=on|off]",
};

const WCHAR* const parity_strings[] =
{
    L"None",    // default
    L"Odd",     // only symbol in this set to have a 'd' in it
    L"Even",    // ... 'v' in it
    L"Mark",    // ... 'm' in it
    L"Space"    // ... 's' and/or a 'c' in it
};

const WCHAR* const control_strings[] = { L"OFF", L"ON", L"HANDSHAKE", L"TOGGLE" };

const WCHAR* const stopbit_strings[] = { L"1", L"1.5", L"2" };


int Usage()
{
    int i;

    wprintf(L"\nConfigures system devices.\n\n");
    for (i = 0; i < ARRAYSIZE(usage_strings); i++)
    {
        wprintf(L"%s\n", usage_strings[i]);
    }
    wprintf(L"\n");
    return 0;
}

int QueryDevices()
{
    WCHAR buffer[20240];
    int len;
    WCHAR* ptr = buffer;

    *ptr = L'\0';
    if (QueryDosDeviceW(NULL, buffer, ARRAYSIZE(buffer)))
    {
        while (*ptr != L'\0')
        {
            len = wcslen(ptr);
            if (wcsstr(ptr, L"COM"))
            {
                wprintf(L"    Found serial device - %s\n", ptr);
            }
            else if (wcsstr(ptr, L"PRN"))
            {
                wprintf(L"    Found printer device - %s\n", ptr);
            }
            else if (wcsstr(ptr, L"LPT"))
            {
                wprintf(L"    Found parallel device - %s\n", ptr);
            }
            else
            {
                // wprintf(L"    Found other device - %s\n", ptr);
            }
            ptr += (len+1);
        }
    }
    else
    {
        wprintf(L"    ERROR: QueryDosDeviceW(...) failed: 0x%lx\n", GetLastError());
    }
    return 1;
}

int ShowParallelStatus(int nPortNum)
{
    WCHAR buffer[250];
    WCHAR szPortName[MAX_PORTNAME_LEN];

    swprintf(szPortName, L"LPT%d", nPortNum);
    wprintf(L"\nStatus for device LPT%d:\n", nPortNum);
    wprintf(L"-----------------------\n");
    if (QueryDosDeviceW(szPortName, buffer, ARRAYSIZE(buffer)))
    {
        WCHAR* ptr = wcsrchr(buffer, L'\\');
        if (ptr != NULL)
        {
            if (0 == wcscmp(szPortName, ++ptr))
            {
                wprintf(L"    Printer output is not being rerouted.\n");
            }
            else
            {
                wprintf(L"    Printer output is being rerouted to serial port %s\n", ptr);
            }
            return 0;
        }
        else
        {
            wprintf(L"    QueryDosDeviceW(%s) returned unrecognised form %s.\n", szPortName, buffer);
        }
    }
    else
    {
        wprintf(L"    ERROR: QueryDosDeviceW(%s) failed: 0x%lx\n", szPortName, GetLastError());
    }
    return 1;
}

int ShowConsoleStatus()
{
    DWORD dwKbdDelay;
    DWORD dwKbdSpeed;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    wprintf(L"\nStatus for device CON:\n");
    wprintf(L"-----------------------\n");
    if (GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo))
    {
        wprintf(L"    Lines:          %d\n", ConsoleScreenBufferInfo.dwSize.Y);
        wprintf(L"    Columns:        %d\n", ConsoleScreenBufferInfo.dwSize.X);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &dwKbdDelay, 0))
    {
        wprintf(L"    Keyboard delay: %ld\n", dwKbdDelay);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &dwKbdSpeed, 0))
    {
        wprintf(L"    Keyboard rate:  %ld\n", dwKbdSpeed);
    }
    wprintf(L"    Code page:      %d\n", GetConsoleOutputCP());
    return 0;
}

static
BOOL SerialPortQuery(int nPortNum, LPDCB pDCB, LPCOMMTIMEOUTS pCommTimeouts, BOOL bWrite)
{
    BOOL result;
    HANDLE hPort;
    WCHAR szPortName[MAX_PORTNAME_LEN];

    ASSERT(pDCB);
    ASSERT(pCommTimeouts);

    swprintf(szPortName, L"COM%d", nPortNum);
    hPort = CreateFileW(szPortName,
                        GENERIC_READ | GENERIC_WRITE,
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

    result = bWrite ? SetCommState(hPort, pDCB)
                    : GetCommState(hPort, pDCB);
    if (!result)
    {
        wprintf(L"Failed to %s the status for device COM%d:\n", bWrite ? L"set" : L"get", nPortNum);
        CloseHandle(hPort);
        return FALSE;
    }

    result = bWrite ? SetCommTimeouts(hPort, pCommTimeouts)
                    : GetCommTimeouts(hPort, pCommTimeouts);
    if (!result)
    {
        wprintf(L"Failed to %s Timeout status for device COM%d:\n", bWrite ? L"set" : L"get", nPortNum);
        CloseHandle(hPort);
        return FALSE;
    }
    CloseHandle(hPort);
    return TRUE;
}

int ShowSerialStatus(int nPortNum)
{
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;

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
    wprintf(L"\nStatus for device COM%d:\n", nPortNum);
    wprintf(L"-----------------------\n");
    wprintf(L"    Baud:            %ld\n", dcb.BaudRate);
    wprintf(L"    Parity:          %s\n", parity_strings[dcb.Parity]);
    wprintf(L"    Data Bits:       %d\n", dcb.ByteSize);
    wprintf(L"    Stop Bits:       %s\n", stopbit_strings[dcb.StopBits]);
    wprintf(L"    Timeout:         %s\n", CommTimeouts.ReadIntervalTimeout ? L"ON" : L"OFF");
    wprintf(L"    XON/XOFF:        %s\n", dcb.fOutX           ? L"ON" : L"OFF");
    wprintf(L"    CTS handshaking: %s\n", dcb.fOutxCtsFlow    ? L"ON" : L"OFF");
    wprintf(L"    DSR handshaking: %s\n", dcb.fOutxDsrFlow    ? L"ON" : L"OFF");
    wprintf(L"    DSR sensitivity: %s\n", dcb.fDsrSensitivity ? L"ON" : L"OFF");
    wprintf(L"    DTR circuit:     %s\n", control_strings[dcb.fDtrControl]);
    wprintf(L"    RTS circuit:     %s\n", control_strings[dcb.fRtsControl]);
    return 0;
}

int SetParallelState(int nPortNum)
{
    WCHAR szPortName[MAX_PORTNAME_LEN];
    WCHAR szTargetPath[MAX_PORTNAME_LEN];

    swprintf(szPortName, L"LPT%d", nPortNum);
    swprintf(szTargetPath, L"COM%d", nPortNum);
    if (!DefineDosDeviceW(DDD_REMOVE_DEFINITION, szPortName, szTargetPath))
    {
        wprintf(L"SetParallelState(%d) - DefineDosDevice(%s) failed: 0x%lx\n", nPortNum, szPortName, GetLastError());
    }
    return 0;
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

int SetConsoleState()
{
/*
    "Select code page:  MODE CON[:] CP SELECT=yyy",
    "Code page status:  MODE CON[:] CP [/STATUS]",
    "Display mode:      MODE CON[:] [COLS=c] [LINES=n]",
    "Typematic rate:    MODE CON[:] [RATE=r DELAY=d]",
 */
    return 0;
}

static
int ExtractModeSerialParams(const WCHAR* param)
{
    if (wcsstr(param, L"OFF"))
        return 0;
    else if (wcsstr(param, L"ON"))
        return 1;
    else if (wcsstr(param, L"HS"))
        return 2;
    else if (wcsstr(param, L"TG"))
        return 3;

    return -1;
}

int SetSerialState(int nPortNum, int args, WCHAR *argv[])
{
    int arg;
    int value;
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
    WCHAR buf[MAX_COMPARAM_LEN+1];

    if (SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE))
    {
        for (arg = 2; arg < args; arg++)
        {
            if (wcslen(argv[arg]) > MAX_COMPARAM_LEN)
            {
                wprintf(L"Invalid parameter (too long) - %s\n", argv[arg]);
                return 1;
            }
            wcscpy(buf, argv[arg]);
            _wcslwr(buf);
            if (wcsstr(buf, L"baud="))
            {
                wscanf(buf+5, L"%lu", &dcb.BaudRate);
            }
            else if (wcsstr(buf, L"parity="))
            {
                if (wcschr(buf, L'D'))
                    dcb.Parity = 1;
                else if (wcschr(buf, L'V'))
                    dcb.Parity = 2;
                else if (wcschr(buf, L'M'))
                    dcb.Parity = 3;
                else if (wcschr(buf, L'S'))
                    dcb.Parity = 4;
                else
                    dcb.Parity = 0;
            }
            else if (wcsstr(buf, L"data="))
            {
                wscanf(buf+5, L"%lu", &dcb.ByteSize);
            }
            else if (wcsstr(buf, L"stop="))
            {
                if (wcschr(buf, L'5'))
                    dcb.StopBits = 1;
                else if (wcschr(buf, L'2'))
                    dcb.StopBits = 2;
                else
                    dcb.StopBits = 0;
            }
            else if (wcsstr(buf, L"to=")) // to=on|off
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"xon=")) // xon=on|off
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fOutX = value;
                    dcb.fInX = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"odsr=")) // odsr=on|off
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fOutxDsrFlow = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"octs=")) // octs=on|off
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fOutxCtsFlow = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"dtr=")) // dtr=on|off|hs
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fDtrControl = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"rts=")) // rts=on|off|hs|tg
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fRtsControl = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else if (wcsstr(buf, L"idsr=")) // idsr=on|off
            {
                value = ExtractModeSerialParams(buf);
                if (value != -1)
                {
                    dcb.fDsrSensitivity = value;
                }
                else
                {
                    goto invalid_serial_parameter;
                }
            }
            else
            {
invalid_serial_parameter:;
                wprintf(L"Invalid parameter - %s\n", buf);
                return 1;
            }
        }
        SerialPortQuery(nPortNum, &dcb, &CommTimeouts, TRUE);
    }
    return 0;
}

int find_portnum(const WCHAR* cmdverb)
{
    int portnum = -1;

    if (cmdverb[3] >= L'0' && cmdverb[3] <= L'9')
    {
        portnum = cmdverb[3] - L'0';
        if (cmdverb[4] >= L'0' && cmdverb[4] <= L'9')
        {
            portnum *= 10;
            portnum += cmdverb[4] - L'0';
        }
    }
    return portnum;
}

int wmain(int argc, WCHAR* argv[])
{
    int nPortNum;
    WCHAR param1[MAX_COMPARAM_LEN+1];
    WCHAR param2[MAX_COMPARAM_LEN+1];

    if (argc > 1)
    {
        if (wcslen(argv[1]) > MAX_COMPARAM_LEN)
        {
            wprintf(L"Invalid parameter (too long) - %s\n", argv[1]);
            return 1;
        }
        wcscpy(param1, argv[1]);
        _wcslwr(param1);
        if (argc > 2)
        {
            if (wcslen(argv[2]) > MAX_COMPARAM_LEN)
            {
                wprintf(L"Invalid parameter (too long) - %s\n", argv[2]);
                return 1;
            }
            wcscpy(param2, argv[2]);
            _wcslwr(param2);
        }
        else
        {
            param2[0] = L'\0';
        }
        if (wcsstr(param1, L"/?") || wcsstr(param1, L"-?"))
        {
            return Usage();
        }
        else if (wcsstr(param1, L"/status"))
        {
            goto show_status;
        }
        else if (wcsstr(param1, L"lpt"))
        {
            nPortNum = find_portnum(param1);
            if (nPortNum != -1)
                return ShowParallelStatus(nPortNum);
        }
        else if (wcsstr(param1, L"con"))
        {
            return ShowConsoleStatus();
        }
        else if (wcsstr(param1, L"com"))
        {
            nPortNum = find_portnum(param1);
            if (nPortNum != -1)
            {
                if (param2[0] == L'\0' || wcsstr(param2, L"/status"))
                {
                    return ShowSerialStatus(nPortNum);
                }
                else
                {
                    return SetSerialState(nPortNum, argc, argv);
                }
            }
        }
        wprintf(L"Invalid parameter - %s\n", param1);
        return 1;
    }
    else
    {
show_status:;

        QueryDevices();
/*
        ShowParallelStatus(1);
        for (nPortNum = 0; nPortNum < MAX_COMPORT_NUM; nPortNum++)
        {
            ShowSerialStatus(nPortNum + 1);
        }
        ShowConsoleStatus();
 */
    }
    return 0;
}
