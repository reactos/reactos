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

#define MAX_PORTNAME_LEN 20
#define MAX_COMPORT_NUM  10
#define MAX_COMPARAM_LEN 20

#define NUM_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ASSERT(a)

const char* usage_strings[] = { 
	"Device Status:     MODE [device] [/STATUS]",
	"Select code page:  MODE CON[:] CP SELECT=yyy",
	"Code page status:  MODE CON[:] CP [/STATUS]",
	"Display mode:      MODE CON[:] [COLS=c] [LINES=n]",
	"Typematic rate:    MODE CON[:] [RATE=r DELAY=d]",
	"Redirect printing: MODE LPTn[:]=COMm[:]",
	"Serial port:       MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s]\n" \
    "                            [to=on|off] [xon=on|off] [odsr=on|off]\n" \
    "                            [octs=on|off] [dtr=on|off|hs]\n" \
    "                            [rts=on|off|hs|tg] [idsr=on|off]",
};

const char* parity_strings[] = { 
    _T("None"),   // default
    _T("Odd"),    // only symbol in this set to have a 'd' in it
    _T("Even"),   // ... 'v' in it
    _T("Mark"),   // ... 'm' in it
    _T("Space")   // ... 's' and/or a 'c' in it
};

const char* control_strings[] = { "OFF", "ON", "HANDSHAKE", "TOGGLE" };

const char* stopbit_strings[] = { _T("1"), _T("1.5"), _T("2") };


int Usage()
{
	int i;

    printf("\nConfigures system devices.\n\n");
	for (i = 0; i < NUM_ELEMENTS(usage_strings); i++) {
        printf("%s\n", usage_strings[i]);
	}
    printf("\n");
	return 0;
}

int QueryDevices()
{
    char buffer[5000];
    int len;
    char* ptr = buffer;
    
    *ptr = '\0';
    if (QueryDosDevice(NULL, buffer, NUM_ELEMENTS(buffer))) {
        while (*ptr != '\0') {
            len = strlen(ptr);
            if (strstr(ptr, "COM")) {
                printf("    Found serial device - %s\n", ptr);
            } else if (strstr(ptr, "PRN")) {
                printf("    Found printer device - %s\n", ptr);
            } else if (strstr(ptr, "LPT")) {
                printf("    Found parallel device - %s\n", ptr);
            } else {
                printf("    Found other device - %s\n", ptr);
            }
            ptr += (len+1);
        }
    } else {
        printf("    ERROR: QueryDosDevice(...) failed.\n");
    }
    return 1;
}

int ShowParrallelStatus(int nPortNum)
{
    char buffer[250];
	char szPortName[MAX_PORTNAME_LEN];

	sprintf(szPortName, "LPT%d", nPortNum);
    printf("\nStatus for device LPT%d:\n", nPortNum);
    printf("-----------------------\n");
    if (QueryDosDevice(szPortName, buffer, NUM_ELEMENTS(buffer))) {
        char* ptr = strrchr(buffer, '\\');
        if (ptr != NULL) {
            if (0 == strcmp(szPortName, ++ptr)) {
                printf("    Printer output is not being rerouted.\n");
            } else {
                printf("    Printer output is being rerouted to serial port %s\n", ptr);
            }
            return 0;
        } else {
            printf("    QueryDosDevice(%s) returned unrecogised form %s.\n", szPortName, buffer);
        }
    } else {
        printf("    ERROR: QueryDosDevice(%s) failed.\n", szPortName);
    }
    return 1;
}

int ShowConsoleStatus()
{
    DWORD dwKbdDelay;
    DWORD dwKbdSpeed;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    printf("\nStatus for device CON:\n");
    printf("-----------------------\n");
    if (GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo)) {
        printf("    Lines:          %d\n", ConsoleScreenBufferInfo.dwSize.Y);
        printf("    Columns:        %d\n", ConsoleScreenBufferInfo.dwSize.X);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &dwKbdDelay, 0)) {
        printf("    Keyboard delay: %d\n", dwKbdDelay);
    }
    if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &dwKbdSpeed, 0)) {
        printf("    Keyboard rate:  %d\n", dwKbdSpeed);
    }
    printf("    Code page:      %d\n", GetConsoleOutputCP());
	return 0;
}

static 
BOOL SerialPortQuery(int nPortNum, LPDCB pDCB, LPCOMMTIMEOUTS pCommTimeouts, BOOL bWrite)
{
    BOOL result;
    HANDLE hPort;
	char szPortName[MAX_PORTNAME_LEN];

    ASSERT(pDCB);
    ASSERT(pCommTimeouts);

	sprintf(szPortName, _T("COM%d"), nPortNum);
    hPort = CreateFile(szPortName,
                       GENERIC_READ|GENERIC_WRITE,
                       0,     // exclusive
                       NULL,  // sec attr
                       OPEN_EXISTING,
                       0,     // no attributes
                       NULL); // no template

    if (hPort == (HANDLE)-1) {
        printf("Illegal device name - %s\n", szPortName);
        return FALSE;
    }
    if (bWrite) {
        result = SetCommState(hPort, pDCB);
    } else {
        result = GetCommState(hPort, pDCB);
    }
    if (!result) {
        printf("Failed to %s the status for device COM%d:\n", bWrite ? "set" : "get", nPortNum);
        CloseHandle(hPort);
        return FALSE;
    }
    if (bWrite) {
        result = SetCommTimeouts(hPort, pCommTimeouts);
    } else {
        result = GetCommTimeouts(hPort, pCommTimeouts);
    }
    if (!result) {
        printf("Failed to %s Timeout status for device COM%d:\n", bWrite ? "set" : "get", nPortNum);
        CloseHandle(hPort);
        return FALSE;

    }
    CloseHandle(hPort);
    return TRUE;
}

int ShowSerialStatus(int nPortNum)
{
    HANDLE hPort;
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
	char szPortName[MAX_PORTNAME_LEN];

    if (!SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE)) {
        return 1;
    }
    if (dcb.Parity > NUM_ELEMENTS(parity_strings)) {
        printf("ERROR: Invalid value for Parity Bits %d:\n", dcb.Parity);
        dcb.Parity = 0;
    }
    if (dcb.StopBits > NUM_ELEMENTS(stopbit_strings)) {
        printf("ERROR: Invalid value for Stop Bits %d:\n", dcb.StopBits);
        dcb.StopBits = 0;
    }
    printf("\nStatus for device COM%d:\n", nPortNum);
    printf("-----------------------\n");
    printf("    Baud:            %d\n", dcb.BaudRate);
    printf("    Parity:          %s\n", parity_strings[dcb.Parity]);
    printf("    Data Bits:       %d\n", dcb.ByteSize);
    printf("    Stop Bits:       %s\n", stopbit_strings[dcb.StopBits]);
    printf("    Timeout:         %s\n", CommTimeouts.ReadIntervalTimeout ? "ON" : "OFF");
    printf("    XON/XOFF:        %s\n", dcb.fOutX ? "ON" : "OFF");
    printf("    CTS handshaking: %s\n", dcb.fOutxCtsFlow ? "ON" : "OFF");
    printf("    DSR handshaking: %s\n", dcb.fOutxDsrFlow ? "ON" : "OFF");
    printf("    DSR sensitivity: %s\n", dcb.fDsrSensitivity ? "ON" : "OFF");
    printf("    DTR circuit:     %s\n", control_strings[dcb.fDtrControl]);
    printf("    RTS circuit:     %s\n", control_strings[dcb.fRtsControl]);
	return 0;
}

int SetParrallelState(int nPortNum)
{
	char szPortName[MAX_PORTNAME_LEN];
    char szTargetPath[MAX_PORTNAME_LEN];

	sprintf(szPortName, _T("LPT%d"), nPortNum);
	sprintf(szTargetPath, _T("COM%d"), nPortNum);
    if (!DefineDosDevice(DDD_REMOVE_DEFINITION, szPortName, szTargetPath)) {
        DWORD error = GetLastError();

        printf("SetParrallelState(%d) - DefineDosDevice(%s) failed: %x\n", nPortNum, error);
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
int ExtractModeSerialParams(const char* param)
{
    if (       strstr(param, "OFF")) {
        return 0;
    } else if (strstr(param, "ON")) {
        return 1;
    } else if (strstr(param, "HS")) {
        return 2;
    } else if (strstr(param, "TG")) {
        return 3;
    }
    return -1;
}

int SetSerialState(int nPortNum, int args, char *argv[])
{
    int arg;
    int value;
    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
    char buf[MAX_COMPARAM_LEN+1];

    if (SerialPortQuery(nPortNum, &dcb, &CommTimeouts, FALSE)) {
        for (arg = 2; arg < args; arg++) {
            if (strlen(argv[arg]) > MAX_COMPARAM_LEN) {
                printf("Invalid parameter (too long) - %s\n", argv[arg]);
                return 1;
            }
            strcpy(buf, argv[arg]);
            strupr(buf);
            if (strstr(buf, "BAUD=")) {
                dcb.BaudRate = atol(buf+5);
            } else if (strstr(buf, "PARITY=")) {
                if (strchr(buf, 'D')) {
                    dcb.Parity = 1;
                } else if (strchr(buf, 'V')) {
                    dcb.Parity = 2;
                } else if (strchr(buf, 'M')) {
                    dcb.Parity = 3;
                } else if (strchr(buf, 'S')) {
                    dcb.Parity = 4;
                } else {
                    dcb.Parity = 0;
                }
            } else if (strstr(buf, "DATA=")) {
                dcb.ByteSize = atol(buf+5);
            } else if (strstr(buf, "STOP=")) {
                if (strchr(buf, '5')) {
                    dcb.StopBits = 1;
                } else if (strchr(buf, '2')) {
                    dcb.StopBits = 2;
                } else {
                    dcb.StopBits = 0;
                }
            } else if (strstr(buf, "TO=")) { // to=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "XON=")) { // xon=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutX = value;
                    dcb.fInX = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "ODSR=")) { // odsr=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutxDsrFlow = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "OCTS=")) { // octs=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fOutxCtsFlow = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "DTR=")) { // dtr=on|off|hs
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fDtrControl = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "RTS=")) { // rts=on|off|hs|tg
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fRtsControl = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else if (strstr(buf, "IDSR=")) { // idsr=on|off
                value = ExtractModeSerialParams(buf);
                if (value != -1) {
                    dcb.fDsrSensitivity = value;
                } else {
                    goto invalid_serial_parameter;
                }
            } else {
invalid_serial_parameter:;
                printf("Invalid parameter - %s\n", buf);
                return 1;
            }
        }
        SerialPortQuery(nPortNum, &dcb, &CommTimeouts, TRUE);
    }
	return 0;
}

int find_portnum(const char* cmdverb)
{
	int portnum = -1;

	if ((char)*(cmdverb + 3) >= '0' && (char)*(cmdverb + 3) <= '9') {
		portnum = ((char)*(cmdverb + 3)) - '0';
        if ((char)*(cmdverb + 4) >= '0' && (char)*(cmdverb + 4) <= '9') {
    		portnum *= 10;
		    portnum += ((char)*(cmdverb + 4)) - '0';
		}
	}
	return portnum;
}

int main(int argc, char *argv[])
{
	int nPortNum;
    char param1[MAX_COMPARAM_LEN+1];
    char param2[MAX_COMPARAM_LEN+1];

    if (argc > 1) {
        if (strlen(argv[1]) > MAX_COMPARAM_LEN) {
            printf("Invalid parameter (too long) - %s\n", argv[1]);
            return 1;
        }
        strcpy(param1, argv[1]);
        strupr(param1);
        if (argc > 2) {
            if (strlen(argv[2]) > MAX_COMPARAM_LEN) {
                printf("Invalid parameter (too long) - %s\n", argv[2]);
                return 1;
            }
            strcpy(param2, argv[2]);
            strupr(param2);
        } else {
            param2[0] = '\0';
        }
		if (strstr(param1, "/?") || strstr(param1, "-?")) {
            return Usage();
        } else if (strstr(param1, "/STA")) {
            goto show_status;
		} else if (strstr(param1, "LPT")) {
			nPortNum = find_portnum(param1);
			if (nPortNum != -1) 
				return ShowParrallelStatus(nPortNum);
		} else if (strstr(param1, "CON")) {
            return ShowConsoleStatus();
		} else if (strstr(param1, "COM")) {
			nPortNum = find_portnum(param1);
            if (nPortNum != -1) {
                if (param2[0] == '\0' || strstr(param2, "/STA")) {
                    return ShowSerialStatus(nPortNum);
                } else {
                    return SetSerialState(nPortNum, argc, argv);
                }
            }
		}
        printf("Invalid parameter - %s\n", param1);
		return 1;
    } else {
show_status:;

        QueryDevices();
/*
        ShowParrallelStatus(1);
        for (nPortNum = 0; nPortNum < MAX_COMPORT_NUM; nPortNum++) {
    	   ShowSerialStatus(nPortNum + 1);
        }
	    ShowConsoleStatus();
 */
	}
    return 0;
}
