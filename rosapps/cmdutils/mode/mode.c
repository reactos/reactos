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

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>

#define MAX_PORTNAME_LEN 20
#define MAX_COMPORT_NUM  10
#define MAX_COMPARAM_LEN 20

#define NUM_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ASSERT(a)

const TCHAR* const usage_strings[] = {
    _T("Device Status:     MODE [device] [/STATUS]"),
    _T("Select code page:  MODE CON[:] CP SELECT=yyy"),
    _T("Code page status:  MODE CON[:] CP [/STATUS]"),
    _T("Display mode:      MODE CON[:] [COLS=c] [LINES=n]"),
    _T("Typematic rate:    MODE CON[:] [RATE=r DELAY=d]"),
    _T("Redirect printing: MODE LPTn[:]=COMm[:]"),
    _T("Serial port:       MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s]\n") \
    _T("                            [to=on|off] [xon=on|off] [odsr=on|off]\n") \
    _T("                            [octs=on|off] [dtr=on|off|hs]\n") \
    _T("                            [rts=on|off|hs|tg] [idsr=on|off]"),
};

const TCHAR* const parity_strings[] = {
    _T("None"),   // default
    _T("Odd"),    // only symbol in this set to have a 'd' in it
    _T("Even"),   // ... 'v' in it
    _T("Mark"),   // ... 'm' in it
    _T("Space")   // ... 's' and/or a 'c' in it
};

const TCHAR* const control_strings[] = { _T("OFF"), _T("ON"), _T("HANDSHAKE"), _T("TOGGLE") };

const TCHAR* const stopbit_strings[] = { _T("1"), _T("1.5"), _T("2") };


int Usage()
{
	int i;

    _tprintf(_T("\nConfigures system devices.\n\n"));
    for (i = 0; i < NUM_ELEMENTS(usage_strings); i++) {
        _tprintf(_T("%s\n"), usage_strings[i]);
    }
    _tprintf(_T("\n"));
    return 0;
}

int QueryDevices()
{
    TCHAR buffer[10240];
    int len;
    TCHAR* ptr = buffer;

    *ptr = '\0';
    if (QueryDosDevice(NULL, buffer, NUM_ELEMENTS(buffer))) {
        while (*ptr != '\0') {
            len = _tcslen(ptr);
            if (_tcsstr(ptr, _T("COM"))) {
                _tprintf(_T("    Found serial device - %s\n"), ptr);
            } else if (_tcsstr(ptr, _T("PRN"))) {
                _tprintf(_T("    Found printer device - %s\n"), ptr);
            } else if (_tcsstr(ptr, _T("LPT"))) {
                _tprintf(_T("    Found parallel device - %s\n"), ptr);
            } else {
                _tprintf(_T("    Found other device - %s\n"), ptr);
            }
            ptr += (len+1);
        }
    } else {
        _tprintf(_T("    ERROR: QueryDosDevice(...) failed.\n"));
    }
    return 1;
}

int ShowParallelStatus(int nPortNum)
{
    TCHAR buffer[250];
    TCHAR szPortName[MAX_PORTNAME_LEN];

    _stprintf(szPortName, _T("LPT%d"), nPortNum);
    _tprintf(_T("\nStatus for device LPT%d:\n"), nPortNum);
    _tprintf(_T("-----------------------\n"));
    if (QueryDosDevice(szPortName, buffer, NUM_ELEMENTS(buffer))) {
        TCHAR* ptr = _tcsrchr(buffer, '\\');
        if (ptr != NULL) {
            if (0 == _tcscmp(szPortName, ++ptr)) {
                _tprintf(_T("    Printer output is not being rerouted.\n"));
            } else {
                _tprintf(_T("    Printer output is being rerouted to serial port %s\n"), ptr);
            }
            return 0;
        } else {
            _tprintf(_T("    QueryDosDevice(%s) returned unrecogised form %s.\n"), szPortName, buffer);
        }
    } else {
        _tprintf(_T("    ERROR: QueryDosDevice(%s) failed.\n"), szPortName);
    }
    return 1;
}

int ShowConsoleStatus()
{
    DWORD dwKbdDelay;
    DWORD dwKbdSpeed;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    _tprintf(_T("\nStatus for device CON:\n"));
    _tprintf(_T("-----------------------\n"));
    if (GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo)) {
        _tprintf(_T("    Lines:          %d\n"), ConsoleScreenBufferInfo.dwSize.Y);
        _tprintf(_T("    Columns:        %d\n"), ConsoleScreenBufferInfo.dwSize.X);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &dwKbdDelay, 0)) {
        _tprintf(_T("    Keyboard delay: %ld\n"), dwKbdDelay);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &dwKbdSpeed, 0)) {
        _tprintf(_T("    Keyboard rate:  %ld\n"), dwKbdSpeed);
    }
    _tprintf(_T("    Code page:      %d\n"), GetConsoleOutputCP());
    return 0;
}

static
BOOL SerialPortQuery(int nPortNum, LPDCB pDCB, LPCOMMTIMEOUTS pCommTimeouts, BOOL bWrite)
{
    BOOL result;
    HANDLE hPort;
    TCHAR szPortName[MAX_PORTNAME_LEN];

    ASSERT(pDCB);
    ASSERT(pCommTimeouts);

    _stprintf(szPortName, _T("COM%d"), nPortNum);
    hPort = CreateFile(szPortName,
                       GENERIC_READ|GENERIC_WRITE,
                       0,     // exclusive
                       NULL,  // sec attr
                       OPEN_EXISTING,
                       0,     // no attributes
                       NULL); // no template

    if (hPort == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Illegal device name - %s\n"), szPortName);
        _tprintf(_T("Last error = 0x%lx\n"), GetLastError());
        return FALSE;
    }
    if (bWrite) {
        result = SetCommState(hPort, pDCB);
    } else {
        result = GetCommState(hPort, pDCB);
    }
    if (!result) {
        _tprintf(_T("Failed to %s the status for device COM%d:\n"), bWrite ? _T("set") : _T("get"), nPortNum);
        CloseHandle(hPort);
        return FALSE;
    }
    if (bWrite) {
        result = SetCommTimeouts(hPort, pCommTimeouts);
    } else {
        result = GetCommTimeouts(hPort, pCommTimeouts);
    }
    if (!result) {
        _tprintf(_T("Failed to %s Timeout status for device COM%d:\n"), bWrite ? _T("set") : _T("get"), nPortNum);
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

    if (!SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE)) {
        return 1;
    }
    if (dcb.Parity > NUM_ELEMENTS(parity_strings)) {
        _tprintf(_T("ERROR: Invalid value for Parity Bits %d:\n"), dcb.Parity);
        dcb.Parity = 0;
    }
    if (dcb.StopBits > NUM_ELEMENTS(stopbit_strings)) {
        _tprintf(_T("ERROR: Invalid value for Stop Bits %d:\n"), dcb.StopBits);
        dcb.StopBits = 0;
    }
    _tprintf(_T("\nStatus for device COM%d:\n"), nPortNum);
    _tprintf(_T("-----------------------\n"));
    _tprintf(_T("    Baud:            %ld\n"), dcb.BaudRate);
    _tprintf(_T("    Parity:          %s\n"), parity_strings[dcb.Parity]);
    _tprintf(_T("    Data Bits:       %d\n"), dcb.ByteSize);
    _tprintf(_T("    Stop Bits:       %s\n"), stopbit_strings[dcb.StopBits]);
    _tprintf(_T("    Timeout:         %s\n"), CommTimeouts.ReadIntervalTimeout ? _T("ON") : _T("OFF"));
    _tprintf(_T("    XON/XOFF:        %s\n"), dcb.fOutX ? _T("ON") : _T("OFF"));
    _tprintf(_T("    CTS handshaking: %s\n"), dcb.fOutxCtsFlow ? _T("ON") : _T("OFF"));
    _tprintf(_T("    DSR handshaking: %s\n"), dcb.fOutxDsrFlow ? _T("ON") : _T("OFF"));
    _tprintf(_T("    DSR sensitivity: %s\n"), dcb.fDsrSensitivity ? _T("ON") : _T("OFF"));
    _tprintf(_T("    DTR circuit:     %s\n"), control_strings[dcb.fDtrControl]);
    _tprintf(_T("    RTS circuit:     %s\n"), control_strings[dcb.fRtsControl]);
    return 0;
}

int SetParallelState(int nPortNum)
{
    TCHAR szPortName[MAX_PORTNAME_LEN];
    TCHAR szTargetPath[MAX_PORTNAME_LEN];

    _stprintf(szPortName, _T("LPT%d"), nPortNum);
    _stprintf(szTargetPath, _T("COM%d"), nPortNum);
    if (!DefineDosDevice(DDD_REMOVE_DEFINITION, szPortName, szTargetPath)) {
        DWORD error = GetLastError();

        _tprintf(_T("SetParallelState(%d) - DefineDosDevice(%s) failed: 0x%lx\n"), nPortNum, szPortName, error);
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
int ExtractModeSerialParams(const TCHAR* param)
{
    if (       _tcsstr(param, _T("OFF"))) {
        return 0;
    } else if (_tcsstr(param, _T("ON"))) {
        return 1;
    } else if (_tcsstr(param, _T("HS"))) {
        return 2;
    } else if (_tcsstr(param, _T("TG"))) {
        return 3;
    }
    return -1;
}

int SetSerialState(int nPortNum, int args, TCHAR *argv[])
{
    int arg;
    int value;
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
    TCHAR buf[MAX_COMPARAM_LEN+1];

    if (SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE)) {
        for (arg = 2; arg < args; arg++) {
            if (_tcslen(argv[arg]) > MAX_COMPARAM_LEN) {
                _tprintf(_T("Invalid parameter (too long) - %s\n"), argv[arg]);
                return 1;
            }
            _tcscpy(buf, argv[arg]);
            _tcslwr(buf);
            if (_tcsstr(buf, _T("baud="))) {
                _tscanf(buf+5, "%lu", &dcb.BaudRate);
            } else if (_tcsstr(buf, _T("parity="))) {
                if (_tcschr(buf, 'D')) {
                    dcb.Parity = 1;
                } else if (_tcschr(buf, 'V')) {
                    dcb.Parity = 2;
                } else if (_tcschr(buf, 'M')) {
                    dcb.Parity = 3;
                } else if (_tcschr(buf, 'S')) {
                    dcb.Parity = 4;
                } else {
                    dcb.Parity = 0;
                }
            } else if (_tcsstr(buf, _T("data="))) {
                _tscanf(buf+5, "%lu", &dcb.ByteSize);
            } else if (_tcsstr(buf, _T("stop="))) {
                if (_tcschr(buf, '5')) {
                    dcb.StopBits = 1;
                } else if (_tcschr(buf, '2')) {
                    dcb.StopBits = 2;
                } else {
                    dcb.StopBits = 0;
                }
            } else if (_tcsstr(buf, _T("to="))) { // to=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("xon="))) { // xon=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutX = value;
                    dcb.fInX = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("odsr="))) { // odsr=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutxDsrFlow = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("octs="))) { // octs=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutxCtsFlow = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("dtr="))) { // dtr=on|off|hs
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fDtrControl = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("rts="))) { // rts=on|off|hs|tg
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fRtsControl = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (_tcsstr(buf, _T("idsr="))) { // idsr=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fDsrSensitivity = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else {
invalid_serial_parameter:;
                _tprintf(_T("Invalid parameter - %s\n"), buf);
                return 1;
            }
        }
        SerialPortQuery(nPortNum, &dcb, &CommTimeouts, TRUE);
    }
    return 0;
}

int find_portnum(const TCHAR* cmdverb)
{
    int portnum = -1;

    if (cmdverb[3] >= '0' && cmdverb[3] <= '9') {
        portnum = cmdverb[3] - '0';
        if (cmdverb[4] >= '0' && cmdverb[4] <= '9') {
            portnum *= 10;
            portnum += cmdverb[4] - '0';
        }
    }
    return portnum;
}

int main(int argc, TCHAR *argv[])
{
    int nPortNum;
    TCHAR param1[MAX_COMPARAM_LEN+1];
    TCHAR param2[MAX_COMPARAM_LEN+1];

    if (argc > 1) {
        if (_tcslen(argv[1]) > MAX_COMPARAM_LEN) {
            _tprintf(_T("Invalid parameter (too long) - %s\n"), argv[1]);
            return 1;
        }
        _tcscpy(param1, argv[1]);
        _tcslwr(param1);
        if (argc > 2) {
            if (_tcslen(argv[2]) > MAX_COMPARAM_LEN) {
                _tprintf(_T("Invalid parameter (too long) - %s\n"), argv[2]);
                return 1;
            }
            _tcscpy(param2, argv[2]);
            _tcslwr(param2);
        } else {
            param2[0] = '\0';
        }
        if (_tcsstr(param1, _T("/?")) || _tcsstr(param1, _T("-?"))) {
            return Usage();
        } else if (_tcsstr(param1, _T("/status"))) {
            goto show_status;
        } else if (_tcsstr(param1, _T("lpt"))) {
            nPortNum = find_portnum(param1);
            if (nPortNum != -1)
                return ShowParallelStatus(nPortNum);
        } else if (_tcsstr(param1, _T("con"))) {
            return ShowConsoleStatus();
        } else if (_tcsstr(param1, _T("com"))) {
            nPortNum = find_portnum(param1);
            if (nPortNum != -1) {
                if (param2[0] == '\0' || _tcsstr(param2, _T("/status"))) {
                    return ShowSerialStatus(nPortNum);
                } else {
                    return SetSerialState(nPortNum, argc, argv);
                }
            }
        }
        _tprintf(_T("Invalid parameter - %s\n"), param1);
        return 1;
    } else {
show_status:;

        QueryDevices();
/*
        ShowParallelStatus(1);
        for (nPortNum = 0; nPortNum < MAX_COMPORT_NUM; nPortNum++) {
    	   ShowSerialStatus(nPortNum + 1);
        }
	    ShowConsoleStatus();
 */
    }
    return 0;
}
