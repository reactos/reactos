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
#include <stdio.h>

#define MAX_PORTNAME_LEN 20
#define MAX_COMPORT_NUM  10

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
    _T("None"),
    _T("Odd"),
    _T("Even"),
    _T("Mark"),
    _T("Space")
};

const char* stopbit_strings[] = { _T("1"), _T("1.5"), _T("2") };


int Usage()
{
	int i;

    printf("\nConfigures system devices.\n\n");
	for (i = 0; i < sizeof(usage_strings)/sizeof(usage_strings[0]); i++) {
        printf("%s\n", usage_strings[i]);
	}
    printf("\n");
	return 0;
}

int ShowParrallelStatus(int nPortNum)
{
    TCHAR buffer[250];

    printf("\nStatus for device LPT%d:\n", nPortNum);
    printf("-----------------------\n");

    if (QueryDosDevice("LPT1", buffer, sizeof(buffer)/sizeof(TCHAR))) {
        printf("    %s.\n", buffer);
    } else {
        //printf("    Printer output is not being rerouted.\n");
    }
    printf("    Printer output is not being rerouted.\n");
/*
DWORD QueryDosDevice(
  LPCTSTR lpDeviceName, // MS-DOS device name string
  LPTSTR lpTargetPath,  // query results buffer
  DWORD ucchMax         // maximum size of buffer
);
 */
	return 0;
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
BOOL GetComData(int nPortNum, LPDCB pDCB, LPCOMMTIMEOUTS pCommTimeouts)
{
    HANDLE hPort;
	TCHAR szPortName[MAX_PORTNAME_LEN];

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
    if (!GetCommState(hPort, pDCB)) {
        printf("Failed to get the status for device COM%d:\n", nPortNum);
        CloseHandle(hPort);
        return FALSE;
    }
    if (!GetCommTimeouts(hPort, pCommTimeouts)) {
        printf("Failed to get Timeout status for device COM%d:\n", nPortNum);
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
	TCHAR szPortName[MAX_PORTNAME_LEN];

	sprintf(szPortName, _T("COM%d"), nPortNum);
    hPort = CreateFile(szPortName,
                       GENERIC_READ|GENERIC_WRITE,
                       0,     // exclusive
                       NULL,  // sec attr
                       OPEN_EXISTING,
                       0,     // no attributes
                       NULL); // no template

    if (hPort == (HANDLE)-1) {
        //printf("Illegal device name - %s\n", szPortName);
        return 1;
    }
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(hPort, &dcb)) {
        printf("Failed to get the status for device COM%d:\n", nPortNum);
        CloseHandle(hPort);
        return 1;
    }
    if (!GetCommTimeouts(hPort, &CommTimeouts)) {
        printf("Failed to get Timeout status for device COM%d:\n", nPortNum);
        CloseHandle(hPort);
        return 1;
    }
    CloseHandle(hPort);

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
    printf("    DTR circuit:     %s\n", dcb.fDtrControl ? "ON" : "OFF");
    printf("    RTS circuit:     %s\n", dcb.fRtsControl ? "ON" : "OFF");
	return 0;
}

int SetParrallelState(int nPortNum)
{
	return 0;
}

int SetConsoleState()
{
	return 0;
}

static 
int GetComParameterData(const char* param)
{
    if (strstr(param, "off")) {
        return 0;
    } else if (strstr(param, "on")) {
        return 1;
    } else if (strstr(param, "hs")) {
        return 2;
    } else if (strstr(param, "tg")) {
        return 3;
    }
    return -1;
}

int SetSerialState(int nPortNum, int args, char *argv[])
{
    int Baud =             1200;
//    int Parity =           None;
    int DataBits =         7;
    int StopBits =         1;
    BOOL bTimeout =        FALSE;
    BOOL bXonXoff =        FALSE;
    BOOL bCTShandshaking = FALSE;
    BOOL bDSRhandshaking = FALSE;
    BOOL bDSRsensitivity = FALSE;
    BOOL bDTRcircuit =     TRUE;
    BOOL bRTScircuit =     TRUE;

    int arg;

    DCB dcb;
    COMMTIMEOUTS CommTimeouts;
    int value;
    char* ptr;

    if (GetComData(nPortNum, &dcb, &CommTimeouts)) {
        for (arg = 0; arg < args; arg++) {
            ptr = argv[arg];
            printf("Parsing arg %d - %s\n", arg, ptr);

            if (strstr(ptr, "BAUD=")) {
            } else if (strstr(ptr, "PARITY=")) {
            } else if (strstr(ptr, "DATA=")) {
            } else if (strstr(ptr, "STOP=")) {
            } else if (strstr(ptr, "to=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "xon=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "odsr=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "octs=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "dtr=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "rts=")) {
                value = GetComParameterData(ptr);
            } else if (strstr(ptr, "idsr=")) {
                value = GetComParameterData(ptr);
            } else {
            }
        }
    }
/*
	"Serial port:       MODE COMm[:] [BAUD=b] [PARITY=p] [DATA=d] [STOP=s]\n" \
    "                            [to=on|off] [xon=on|off] [odsr=on|off]\n" \
    "                            [octs=on|off] [dtr=on|off|hs]\n" \
    "                            [rts=on|off|hs|tg] [idsr=on|off]",

 */
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

    if (argc > 1) {
		if (strstr(argv[1], "/?")) {
            return Usage();
		} else if (strstr(argv[1], "LPT")) {
			nPortNum = find_portnum(argv[1]);
			if (nPortNum != -1) 
				return ShowParrallelStatus(nPortNum);
		} else if (strstr(argv[1], "CON")) {
            return ShowConsoleStatus();
		} else if (strstr(argv[1], "COM")) {
			nPortNum = find_portnum(argv[1]);
			if (nPortNum != -1) 
                return ShowSerialStatus(nPortNum);
		}
        printf("Invalid parameter - %s\n", argv[1]);
		return 1;
    } else {
	    ShowParrallelStatus(1);
        for (nPortNum = 0; nPortNum < MAX_COMPORT_NUM; nPortNum++) {
    	   ShowSerialStatus(nPortNum + 1);
        }
	    ShowConsoleStatus();
	}
    return 0;
}
