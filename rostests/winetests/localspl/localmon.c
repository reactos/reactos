/* 
 * Unit test suite for localspl API functions: local print monitor
 *
 * Copyright 2006-2007 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winreg.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/test.h"


/* ##### */

static HMODULE  hdll;
static HMODULE  hlocalmon;
static LPMONITOREX (WINAPI *pInitializePrintMonitor)(LPWSTR);

static LPMONITOREX pm;
static BOOL  (WINAPI *pEnumPorts)(LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
static BOOL  (WINAPI *pOpenPort)(LPWSTR, PHANDLE);
static BOOL  (WINAPI *pOpenPortEx)(LPWSTR, LPWSTR, PHANDLE, struct _MONITOR *);
static BOOL  (WINAPI *pStartDocPort)(HANDLE, LPWSTR, DWORD, DWORD, LPBYTE);
static BOOL  (WINAPI *pWritePort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pReadPort)(HANDLE hPort, LPBYTE, DWORD, LPDWORD);
static BOOL  (WINAPI *pEndDocPort)(HANDLE);
static BOOL  (WINAPI *pClosePort)(HANDLE);
static BOOL  (WINAPI *pAddPort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pAddPortEx)(LPWSTR, DWORD, LPBYTE, LPWSTR);
static BOOL  (WINAPI *pConfigurePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pDeletePort)(LPWSTR, HWND, LPWSTR);
static BOOL  (WINAPI *pGetPrinterDataFromPort)(HANDLE, DWORD, LPWSTR, LPWSTR, DWORD, LPWSTR, DWORD, LPDWORD);
static BOOL  (WINAPI *pSetPortTimeOuts)(HANDLE, LPCOMMTIMEOUTS, DWORD);
static BOOL  (WINAPI *pXcvOpenPort)(LPCWSTR, ACCESS_MASK, PHANDLE phXcv);
static DWORD (WINAPI *pXcvDataPort)(HANDLE, LPCWSTR, PBYTE, DWORD, PBYTE, DWORD, PDWORD);
static BOOL  (WINAPI *pXcvClosePort)(HANDLE);

static HANDLE hXcv;
static HANDLE hXcv_noaccess;

/* ########################### */

static const WCHAR cmd_AddPortW[] = {'A','d','d','P','o','r','t',0};
static const WCHAR cmd_ConfigureLPTPortCommandOKW[] = {'C','o','n','f','i','g','u','r','e',
                                    'L','P','T','P','o','r','t',
                                    'C','o','m','m','a','n','d','O','K',0};
static WCHAR cmd_DeletePortW[] = {'D','e','l','e','t','e','P','o','r','t',0};
static WCHAR cmd_GetTransmissionRetryTimeoutW[] = {'G','e','t',
                                    'T','r','a','n','s','m','i','s','s','i','o','n',
                                    'R','e','t','r','y','T','i','m','e','o','u','t',0};

static WCHAR cmd_MonitorUIW[] = {'M','o','n','i','t','o','r','U','I',0};
static WCHAR cmd_MonitorUI_lcaseW[] = {'m','o','n','i','t','o','r','u','i',0};
static WCHAR cmd_PortIsValidW[] = {'P','o','r','t','I','s','V','a','l','i','d',0};
static WCHAR does_not_existW[] = {'d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};
static CHAR  emptyA[] = "";
static WCHAR emptyW[] = {0};
static WCHAR LocalPortW[] = {'L','o','c','a','l',' ','P','o','r','t',0};
static WCHAR Monitors_LocalPortW[] = {
                                'S','y','s','t','e','m','\\',
                                'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                'C','o','n','t','r','o','l','\\',
                                'P','r','i','n','t','\\',
                                'M','o','n','i','t','o','r','s','\\',
                                'L','o','c','a','l',' ','P','o','r','t',0};

static CHAR  num_0A[] = "0";
static WCHAR num_0W[] = {'0',0};
static CHAR  num_1A[] = "1";
static WCHAR num_1W[] = {'1',0};
static CHAR  num_999999A[] = "999999";
static WCHAR num_999999W[] = {'9','9','9','9','9','9',0};
static CHAR  num_1000000A[] = "1000000";
static WCHAR num_1000000W[] = {'1','0','0','0','0','0','0',0};

static WCHAR portname_comW[]  = {'C','O','M',0};
static WCHAR portname_com1W[] = {'C','O','M','1',':',0};
static WCHAR portname_com2W[] = {'C','O','M','2',':',0};
static WCHAR portname_fileW[] = {'F','I','L','E',':',0};
static WCHAR portname_lptW[]  = {'L','P','T',0};
static WCHAR portname_lpt1W[] = {'L','P','T','1',':',0};
static WCHAR portname_lpt2W[] = {'L','P','T','2',':',0};
static WCHAR server_does_not_existW[] = {'\\','\\','d','o','e','s','_','n','o','t','_','e','x','i','s','t',0};

static CHAR  TransmissionRetryTimeoutA[] = {'T','r','a','n','s','m','i','s','s','i','o','n',
                                    'R','e','t','r','y','T','i','m','e','o','u','t',0};

static CHAR  WinNT_CV_WindowsA[] = {'S','o','f','t','w','a','r','e','\\',
                                    'M','i','c','r','o','s','o','f','t','\\',
                                    'W','i','n','d','o','w','s',' ','N','T','\\',
                                    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                    'W','i','n','d','o','w','s',0};
static WCHAR wineW[] = {'W','i','n','e',0};

static WCHAR tempdirW[MAX_PATH];
static WCHAR tempfileW[MAX_PATH];

#define PORTNAME_PREFIX  3
#define PORTNAME_MINSIZE 5
#define PORTNAME_MAXSIZE 10
static WCHAR have_com[PORTNAME_MAXSIZE];
static WCHAR have_lpt[PORTNAME_MAXSIZE];
static WCHAR have_file[PORTNAME_MAXSIZE];

/* ########################### */

static DWORD delete_port(LPWSTR portname)
{
    DWORD   res;

    if (pDeletePort) {
        res = pDeletePort(NULL, 0, portname);
    }
    else
    {
        res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) portname, (lstrlenW(portname) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    }
    return res;
}

/* ########################### */

static void find_installed_ports(void)
{
    PORT_INFO_1W * pi = NULL;
    WCHAR   nameW[PORTNAME_MAXSIZE];
    DWORD   needed;
    DWORD   returned;
    DWORD   res;
    DWORD   id;

    have_com[0] = '\0';
    have_lpt[0] = '\0';
    have_file[0] = '\0';

    if (!pEnumPorts) return;

    res = pEnumPorts(NULL, 1, NULL, 0, &needed, &returned);
    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        pi = HeapAlloc(GetProcessHeap(), 0, needed);
    }
    res = pEnumPorts(NULL, 1, (LPBYTE) pi, needed, &needed, &returned);

    if (!res) {
        skip("no ports found\n");
        HeapFree(GetProcessHeap(), 0, pi);
        return;
    }

    id = 0;
    while (id < returned) {
        res = lstrlenW(pi[id].pName);
        if ((res >= PORTNAME_MINSIZE) && (res < PORTNAME_MAXSIZE) &&
            (pi[id].pName[res-1] == ':')) {
            /* copy only the prefix ("LPT" or "COM") */
            memcpy(&nameW, pi[id].pName, PORTNAME_PREFIX * sizeof(WCHAR));
            nameW[PORTNAME_PREFIX] = '\0';

            if (!have_com[0] && (lstrcmpiW(nameW, portname_comW) == 0)) {
                memcpy(&have_com, pi[id].pName, (res+1) * sizeof(WCHAR));
            }

            if (!have_lpt[0] && (lstrcmpiW(nameW, portname_lptW) == 0)) {
                memcpy(&have_lpt, pi[id].pName, (res+1) * sizeof(WCHAR));
            }

            if (!have_file[0] && (lstrcmpiW(pi[id].pName, portname_fileW) == 0)) {
                memcpy(&have_file, pi[id].pName, (res+1) * sizeof(WCHAR));
            }
        }
        id++;
    }

    HeapFree(GetProcessHeap(), 0, pi);
}

/* ########################### */

static void test_AddPort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pAddPort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pAddPort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - The Servername is ignored
        - Case of MonitorName is ignored
    */

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, emptyW);
    ok(!res, "returned %d with %u (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pAddPort(NULL, 0, does_not_existW);
    ok(!res, "returned %d with %u (expected '0')\n", res, GetLastError());

}

/* ########################### */

static void test_AddPortEx(void)
{
    PORT_INFO_2W pi;
    DWORD   res;

    if (!pAddPortEx) {
        skip("AddPortEx\n");
        return;
    }
    if ((!pDeletePort) &&  (!hXcv)) {
        skip("No API to delete a Port\n");
        return;
    }

    /* start test with clean ports */
    delete_port(tempfileW);

    pi.pPortName = tempfileW;
    if (0) {
        /* tests crash with native localspl.dll in w2k,
           but works with native localspl.dll in wine */
        SetLastError(0xdeadbeef);
        res = pAddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
        trace("returned %u with %u\n", res, GetLastError() );
        ok( res, "got %u with %u (expected '!= 0')\n", res, GetLastError());

        /* port already exists: */
        SetLastError(0xdeadbeef);
        res = pAddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
        trace("returned %u with %u\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %u with %u (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        delete_port(tempfileW);


        /*  NULL for pMonitorName is documented for Printmonitors, but
            localspl.dll fails always with ERROR_INVALID_PARAMETER  */
        SetLastError(0xdeadbeef);
        res = pAddPortEx(NULL, 1, (LPBYTE) &pi, NULL);
        trace("returned %u with %u\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %u with %u (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);


        SetLastError(0xdeadbeef);
        res = pAddPortEx(NULL, 1, (LPBYTE) &pi, emptyW);
        trace("returned %u with %u\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %u with %u (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);


        SetLastError(0xdeadbeef);
        res = pAddPortEx(NULL, 1, (LPBYTE) &pi, does_not_existW);
        trace("returned %u with %u\n", res, GetLastError() );
        ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
            "got %u with %u (expected '0' with ERROR_INVALID_PARAMETER)\n",
            res, GetLastError());
        if (res) delete_port(tempfileW);
    }

    pi.pPortName = NULL;
    SetLastError(0xdeadbeef);
    res = pAddPortEx(NULL, 1, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %u with %u (expected '0' with ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());

    /*  level 2 is documented as supported for Printmonitors,
        but localspl.dll fails always with ERROR_INVALID_LEVEL */

    pi.pPortName = tempfileW;
    pi.pMonitorName = LocalPortW;
    pi.pDescription = wineW;
    pi.fPortType = PORT_TYPE_WRITE;

    SetLastError(0xdeadbeef);
    res = pAddPortEx(NULL, 2, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %u with %u (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);


    /* invalid levels */
    SetLastError(0xdeadbeef);
    res = pAddPortEx(NULL, 0, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %u with %u (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);


    SetLastError(0xdeadbeef);
    res = pAddPortEx(NULL, 3, (LPBYTE) &pi, LocalPortW);
    ok( !res && (GetLastError() == ERROR_INVALID_LEVEL),
        "got %u with %u (expected '0' with ERROR_INVALID_LEVEL)\n",
        res, GetLastError());
    if (res) delete_port(tempfileW);

    /* cleanup */
    delete_port(tempfileW);
}

/* ########################### */

static void test_ClosePort(void)
{
    HANDLE  hPort;
    HANDLE  hPort2;
    LPWSTR  nameW = NULL;
    DWORD   res;
    DWORD   res2;


    if (!pOpenPort || !pClosePort) return;

    if (have_com[0]) {
        nameW = have_com;

        hPort = (HANDLE) 0xdeadbeef;
        res = pOpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = pOpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %u with %u (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %u with %u (expected '!= 0')\n", res, GetLastError());
        }
    }


    if (have_lpt[0]) {
        nameW = have_lpt;

        hPort = (HANDLE) 0xdeadbeef;
        res = pOpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = pOpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %u with %u (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %u with %u (expected '!= 0')\n", res, GetLastError());
        }
    }


    if (have_file[0]) {
        nameW = have_file;

        hPort = (HANDLE) 0xdeadbeef;
        res = pOpenPort(nameW, &hPort);
        hPort2 = (HANDLE) 0xdeadbeef;
        res2 = pOpenPort(nameW, &hPort2);

        if (res2 && (hPort2 != hPort)) {
            SetLastError(0xdeadbeef);
            res2 = pClosePort(hPort2);
            ok(res2, "got %u with %u (expected '!= 0')\n", res2, GetLastError());
        }

        if (res) {
            SetLastError(0xdeadbeef);
            res = pClosePort(hPort);
            ok(res, "got %u with %u (expected '!= 0')\n", res, GetLastError());
        }

    }

    if (0) {
        /* an invalid HANDLE crash native localspl.dll */

        SetLastError(0xdeadbeef);
        res = pClosePort(NULL);
        trace("got %u with %u\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pClosePort( (HANDLE) 0xdeadbeef);
        trace("got %u with %u\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pClosePort(INVALID_HANDLE_VALUE);
        trace("got %u with %u\n", res, GetLastError());
    }

}

/* ########################### */
                                       
static void test_ConfigurePort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pConfigurePort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pConfigurePort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - Case of Portname is ignored
        - "COM1:" and "COM01:" are the same (Compared by value)
        - Portname without ":" => Dialog "Nothing to configure" comes up; Success
        - "LPT1:", "LPT0:" and "LPT:" are the same (Numbers in "LPT:" are ignored)
        - Empty Servername (LPT1:) => Dialog comes up (Servername is ignored)
        - "FILE:" => Dialog "Nothing to configure" comes up; Success
        - Empty Portname =>  => Dialog "Nothing to configure" comes up; Success
        - Port "does_not_exist" => Dialog "Nothing to configure" comes up; Success
    */
    if (winetest_interactive > 0) {

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_com1W);
        trace("returned %d with %u\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_lpt1W);
        trace("returned %d with %u\n", res, GetLastError());

        SetLastError(0xdeadbeef);
        res = pConfigurePort(NULL, 0, portname_fileW);
        trace("returned %d with %u\n", res, GetLastError());
    }
}

/* ########################### */

static void test_DeletePort(void)
{
    DWORD   res;

    /* moved to localui.dll since w2k */
    if (!pDeletePort) return;

    if (0)
    {
    /* NT4 crash on this test */
    res = pDeletePort(NULL, 0, NULL);
    }

    /*  Testing-Results (localmon.dll from NT4.0):
        - Case of Portname is ignored (returned '1' on Success)
        - "COM1:" and "COM01:" are different (Compared as string)
        - server_does_not_exist (LPT1:) => Port deleted, Success (Servername is ignored)
        - Empty Portname =>  => FALSE (LastError not changed)
        - Port "does_not_exist" => FALSE (LastError not changed)
    */

    SetLastError(0xdeadbeef);
    res = pDeletePort(NULL, 0, emptyW);
    ok(!res, "returned %d with %u (expected '0')\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pDeletePort(NULL, 0, does_not_existW);
    ok(!res, "returned %d with %u (expected '0')\n", res, GetLastError());

}

/* ########################### */

static void test_EnumPorts(void)
{
    DWORD   res;
    DWORD   level;
    LPBYTE  buffer;
    DWORD   cbBuf;
    DWORD   pcbNeeded;
    DWORD   pcReturned;

    if (!pEnumPorts) return;

    /* valid levels are 1 and 2 */
    for(level = 0; level < 4; level++) {

        cbBuf = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, NULL, 0, &cbBuf, &pcReturned);

        /* use only a short test, when we test with an invalid level */
        if(!level || (level > 2)) {
            /* NT4 fails with ERROR_INVALID_LEVEL (as expected)
               XP succeeds with ERROR_SUCCESS () */
            ok( (cbBuf == 0) && (pcReturned == 0),
                "(%d) returned %d with %u and %d, %d (expected 0, 0)\n",
                level, res, GetLastError(), cbBuf, pcReturned);
            continue;
        }        

        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%d) returned %d with %u and %d, %d (expected '0' with "
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), cbBuf, pcReturned);

        buffer = HeapAlloc(GetProcessHeap(), 0, cbBuf * 2);
        if (buffer == NULL) continue;

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %u and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);
        /* We can compare the returned Data with the Registry / "win.ini",[Ports] here */

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %u and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(NULL, level, buffer, cbBuf-1, &pcbNeeded, &pcReturned);
        ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER),
            "(%d) returned %d with %u and %d, %d (expected '0' with "
            "ERROR_INSUFFICIENT_BUFFER)\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        if (0)
        {
        /* The following tests crash this app with native localmon/localspl */
        res = pEnumPorts(NULL, level, NULL, cbBuf, &pcbNeeded, &pcReturned);
        res = pEnumPorts(NULL, level, buffer, cbBuf, NULL, &pcReturned);
        res = pEnumPorts(NULL, level, buffer, cbBuf, &pcbNeeded, NULL);
        }

        /* The Servername is ignored */
        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(emptyW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %u and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        pcbNeeded = 0xdeadbeef;
        pcReturned = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pEnumPorts(server_does_not_existW, level, buffer, cbBuf+1, &pcbNeeded, &pcReturned);
        ok( res, "(%d) returned %d with %u and %d, %d (expected '!= 0')\n",
            level, res, GetLastError(), pcbNeeded, pcReturned);

        HeapFree(GetProcessHeap(), 0, buffer);
    }
}

/* ########################### */


static void test_InitializePrintMonitor(void)
{
    LPMONITOREX res;

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(NULL);
    /* The Parameter was unchecked before w2k */
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %u\n (expected '!= NULL' or: NULL with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(emptyW);
    ok( res || (GetLastError() == ERROR_INVALID_PARAMETER),
        "returned %p with %u\n (expected '!= NULL' or: NULL with "
        "ERROR_INVALID_PARAMETER)\n", res, GetLastError());


    /* Every call with a non-empty string returns the same Pointer */
    SetLastError(0xdeadbeef);
    res = pInitializePrintMonitor(Monitors_LocalPortW);
    ok( res == pm,
        "returned %p with %u (expected %p)\n", res, GetLastError(), pm);
}


/* ########################### */

static void test_OpenPort(void)
{
    HANDLE  hPort;
    HANDLE  hPort2;
    LPWSTR  nameW = NULL;
    DWORD   res;
    DWORD   res2;

    if (!pOpenPort || !pClosePort) return;

    if (have_com[0]) {
        nameW = have_com;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pOpenPort(nameW, &hPort);
        ok( res, "got %u with %u and %p (expected '!= 0')\n",
            res, GetLastError(), hPort);

        /* the same HANDLE is returned for a second OpenPort in native localspl */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = pOpenPort(nameW, &hPort2);
        ok( res2, "got %u with %u and %p (expected '!= 0')\n",
            res2, GetLastError(), hPort2);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (have_lpt[0]) {
        nameW = have_lpt;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pOpenPort(nameW, &hPort);
        ok( res || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %u with %u and %p (expected '!= 0' or '0' with ERROR_ACCESS_DENIED)\n",
            res, GetLastError(), hPort);

        /* the same HANDLE is returned for a second OpenPort in native localspl */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = pOpenPort(nameW, &hPort2);
        ok( res2 || (GetLastError() == ERROR_ACCESS_DENIED),
            "got %u with %u and %p (expected '!= 0' or '0' with ERROR_ACCESS_DENIED)\n",
            res2, GetLastError(), hPort2);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (have_file[0]) {
        nameW = have_file;

        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pOpenPort(nameW, &hPort);
        ok( res, "got %u with %u and %p (expected '!= 0')\n",
            res, GetLastError(), hPort);

        /* a different HANDLE is returned for a second OpenPort */
        hPort2 = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res2 = pOpenPort(nameW, &hPort2);
        ok( res2 && (hPort2 != hPort),
            "got %u with %u and %p (expected '!= 0' and '!= %p')\n",
            res2, GetLastError(), hPort2, hPort);

        if (res) pClosePort(hPort);
        if (res2 && (hPort2 != hPort)) pClosePort(hPort2);
    }

    if (0) {
        /* this test crash native localspl (w2k+xp) */
        if (nameW) {
            hPort = (HANDLE) 0xdeadbeef;
            SetLastError(0xdeadbeef);
            res = pOpenPort(nameW, NULL);
            trace("got %u with %u and %p\n", res, GetLastError(), hPort);
        }
    }

    hPort = (HANDLE) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pOpenPort(does_not_existW, &hPort);
    ok (!res && (hPort == (HANDLE) 0xdeadbeef),
        "got %u with 0x%x and %p (expected '0' and 0xdeadbeef)\n", res, GetLastError(), hPort);
    if (res) pClosePort(hPort);

    hPort = (HANDLE) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pOpenPort(emptyW, &hPort);
    ok (!res && (hPort == (HANDLE) 0xdeadbeef),
        "got %u with 0x%x and %p (expected '0' and 0xdeadbeef)\n", res, GetLastError(), hPort);
    if (res) pClosePort(hPort);


    /* NULL as name crash native localspl (w2k+xp) */
    if (0) {
        hPort = (HANDLE) 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pOpenPort(NULL, &hPort);
        trace("got %u with %u and %p\n", res, GetLastError(), hPort);
    }

}

/* ########################### */

static void test_XcvClosePort(void)
{
    DWORD   res;
    HANDLE hXcv2;


    if (0)
    {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvClosePort(NULL);
    res = pXcvClosePort(INVALID_HANDLE_VALUE);
    }


    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv2);
    ok(res, "returned %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);

    if (res) {
        SetLastError(0xdeadbeef);
        res = pXcvClosePort(hXcv2);
        ok( res, "returned %d with %u (expected '!= 0')\n", res, GetLastError());

        if (0)
        {
        /* test for "Double Free": crash with native localspl.dll (w2k+xp) */
        res = pXcvClosePort(hXcv2);
        }
    }
}

/* ########################### */

static void test_XcvDataPort_AddPort(void)
{
    DWORD   res;


    /*
     * The following tests crash with native localspl.dll on w2k and xp,
     * but it works, when the native dll (w2k and xp) is used in wine.
     * also tested (same crash): replacing emptyW with portname_lpt1W
     * and replacing "NULL, 0, NULL" with "buffer, MAX_PATH, &needed"
     *
     * We need to use a different API (AddPortEx) instead
     */
    if (0)
    {
    /* create a Port for a normal, writable file */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);

    /* add our testport again */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);

    /* create a well-known Port  */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) portname_lpt1W, (lstrlenW(portname_lpt1W) + 1) * sizeof(WCHAR), NULL, 0, NULL);

    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) portname_lpt1W, (lstrlenW(portname_lpt1W) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    /* native localspl.dll on wine: ERROR_ALREADY_EXISTS */

    /* ERROR_ALREADY_EXISTS is also returned from native localspl.dll on wine,
       when "RPT1:" was already installed for redmonnt.dll:
       res = pXcvDataPort(hXcv, cmd_AddPortW, (PBYTE) portname_rpt1W, ...
    */

    /* cleanup */
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, NULL);
    }

}

/* ########################### */

static void test_XcvDataPort_ConfigureLPTPortCommandOK(void)
{
    CHAR    org_value[16];
    CHAR    buffer[16];
    HKEY    hroot = NULL;
    DWORD   res;
    DWORD   needed;


    /* Read the original value from the registry */
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsA, 0, KEY_ALL_ACCESS, &hroot);
    if (res == ERROR_ACCESS_DENIED) {
        skip("ACCESS_DENIED\n");
        return;
    }

    if (res != ERROR_SUCCESS) {
        /* unable to open the registry: skip the test */
        skip("got %d\n", res);
        return;
    }
    org_value[0] = '\0';
    needed = sizeof(org_value)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) org_value, &needed);
    ok( (res == ERROR_SUCCESS) || (res == ERROR_FILE_NOT_FOUND),
        "returned %u and %u for \"%s\" (expected ERROR_SUCCESS or "
        "ERROR_FILE_NOT_FOUND)\n", res, needed, org_value);

    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);

    /* set to "0" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_0W, sizeof(num_0W), NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'ConfigureLPTPortCommandOK' not supported\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_0A) == 0),
        "returned %d and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_0A);


    /* set to "1" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_1W, sizeof(num_1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_1A) == 0),
        "returned %d and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_1A);

    /* set to "999999" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_999999W, sizeof(num_999999W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_999999A) == 0),
        "returned %d and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_999999A);

    /* set to "1000000" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_ConfigureLPTPortCommandOKW, (PBYTE) num_1000000W, sizeof(num_1000000W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());
    needed = sizeof(buffer)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) buffer, &needed);
    ok( (res == ERROR_SUCCESS) && (lstrcmpA(buffer, num_1000000A) == 0),
        "returned %d and '%s' (expected ERROR_SUCCESS and '%s')\n",
        res, buffer, num_1000000A);

    /*  using cmd_ConfigureLPTPortCommandOKW with does_not_existW:
        the string "does_not_exist" is written to the registry */


    /* restore the original value */
    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    if (org_value[0]) {
        res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)org_value, lstrlenA(org_value)+1);
        ok(res == ERROR_SUCCESS, "unable to restore original value (got %u): %s\n", res, org_value);
    }

    RegCloseKey(hroot);

}

/* ########################### */

static void test_XcvDataPort_DeletePort(void)
{
    DWORD   res;
    DWORD   needed;


    /* cleanup: just to make sure */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( !res  || (res == ERROR_FILE_NOT_FOUND),
        "returned %d with %u (expected ERROR_SUCCESS or ERROR_FILE_NOT_FOUND)\n",
        res, GetLastError());


    /* ToDo: cmd_AddPortW for tempfileW, then cmd_DeletePortW for the existing Port */


    /* try to delete a nonexistent Port */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_DeletePortW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( res == ERROR_FILE_NOT_FOUND,
        "returned %d with %u (expected ERROR_FILE_NOT_FOUND)\n", res, GetLastError());

    /* emptyW as Portname: ERROR_FILE_NOT_FOUND is returned */
    /* NULL as Portname: Native localspl.dll crashed */

}

/* ########################### */

static void test_XcvDataPort_GetTransmissionRetryTimeout(void)
{
    CHAR    org_value[16];
    HKEY    hroot = NULL;
    DWORD   buffer[2];
    DWORD   res;
    DWORD   needed;
    DWORD   len;


    /* ask for needed size */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'GetTransmissionRetryTimeout' not supported\n");
        return;
    }
    len = sizeof(DWORD);
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (needed == len),
        "returned %d with %u and %u (expected ERROR_INSUFFICIENT_BUFFER "
        "and '%u')\n", res, GetLastError(), needed, len);
    len = needed;

    /* Read the original value from the registry */
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, WinNT_CV_WindowsA, 0, KEY_ALL_ACCESS, &hroot);
    if (res == ERROR_ACCESS_DENIED) {
        skip("ACCESS_DENIED\n");
        return;
    }

    if (res != ERROR_SUCCESS) {
        /* unable to open the registry: skip the test */
        skip("got %d\n", res);
        return;
    }

    org_value[0] = '\0';
    needed = sizeof(org_value)-1 ;
    res = RegQueryValueExA(hroot, TransmissionRetryTimeoutA, NULL, NULL, (PBYTE) org_value, &needed);
    ok( (res == ERROR_SUCCESS) || (res == ERROR_FILE_NOT_FOUND),
        "returned %u and %u for \"%s\" (expected ERROR_SUCCESS or "
        "ERROR_FILE_NOT_FOUND)\n", res, needed, org_value);

    /* Get default value (documented as 90 in the w2k reskit, but that is wrong) */
    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 45),
        "returned %d with %u and %u for %d\n (expected ERROR_SUCCESS "
        "for '45')\n", res, GetLastError(), needed, buffer[0]);

    /* the default timeout is returned, when the value is empty */
    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)emptyA, 1);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 45),
        "returned %d with %u and %u for %d\n (expected ERROR_SUCCESS "
        "for '45')\n", res, GetLastError(), needed, buffer[0]);

    /* the dialog is limited (1 - 999999), but that is done somewhere else */
    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_0A, lstrlenA(num_0A)+1);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 0),
        "returned %d with %u and %u for %d\n (expected ERROR_SUCCESS "
        "for '0')\n", res, GetLastError(), needed, buffer[0]);


    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_1A, lstrlenA(num_1A)+1);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 1),
        "returned %d with %u and %u for %d\n (expected 'ERROR_SUCCESS' "
        "for '1')\n", res, GetLastError(), needed, buffer[0]);

    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_999999A, lstrlenA(num_999999A)+1);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 999999),
        "returned %d with %u and %u for %d\n (expected ERROR_SUCCESS "
        "for '999999')\n", res, GetLastError(), needed, buffer[0]);


    res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)num_1000000A, lstrlenA(num_1000000A)+1);
    needed = (DWORD) 0xdeadbeef;
    buffer[0] = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_GetTransmissionRetryTimeoutW, NULL, 0, (PBYTE) buffer, len, &needed);
    ok( (res == ERROR_SUCCESS) && (buffer[0] == 1000000),
        "returned %d with %u and %u for %d\n (expected ERROR_SUCCESS "
        "for '1000000')\n", res, GetLastError(), needed, buffer[0]);

    /* restore the original value */
    RegDeleteValueA(hroot, TransmissionRetryTimeoutA);
    if (org_value[0]) {
        res = RegSetValueExA(hroot, TransmissionRetryTimeoutA, 0, REG_SZ, (PBYTE)org_value, lstrlenA(org_value)+1);
        ok(res == ERROR_SUCCESS, "unable to restore original value (got %u): %s\n", res, org_value);
    }

    RegCloseKey(hroot);
}

/* ########################### */

static void test_XcvDataPort_MonitorUI(void)
{
    DWORD   res;
    BYTE    buffer[MAX_PATH + 2];
    DWORD   needed;
    DWORD   len;


    /* ask for needed size */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'MonitorUI' nor supported\n");
        return;
    }
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (needed <= MAX_PATH),
        "returned %d with %u and 0x%x (expected 'ERROR_INSUFFICIENT_BUFFER' "
        " and '<= MAX_PATH')\n", res, GetLastError(), needed);

    if (needed > MAX_PATH) {
        skip("buffer overflow (%u)\n", needed);
        return;
    }
    len = needed;

    /* the command is required */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, emptyW, NULL, 0, NULL, 0, &needed);
    ok( res == ERROR_INVALID_PARAMETER, "returned %d with %u and 0x%x "
        "(expected 'ERROR_INVALID_PARAMETER')\n", res, GetLastError(), needed);

    if (0) {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvDataPort(hXcv, NULL, NULL, 0, buffer, MAX_PATH, &needed);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, NULL, len, &needed);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len, NULL);
    }


    /* hXcv is ignored for the command "MonitorUI" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(NULL, cmd_MonitorUIW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u and 0x%x "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* pszDataName is case-sensitive */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUI_lcaseW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_INVALID_PARAMETER, "returned %d with %u and 0x%x "
        "(expected 'ERROR_INVALID_PARAMETER')\n", res, GetLastError(), needed);

    /* off by one: larger  */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len+1, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u and 0x%x "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* off by one: smaller */
    /* the buffer is not modified for NT4, w2k, XP */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len-1, &needed);
    ok( res == ERROR_INSUFFICIENT_BUFFER, "returned %d with %u and 0x%x "
        "(expected 'ERROR_INSUFFICIENT_BUFFER')\n", res, GetLastError(), needed);

    /* Normal use. The DLL-Name without a Path is returned */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_MonitorUIW, NULL, 0, buffer, len, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u and 0x%x "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);


    /* small check without access-rights: */
    if (!hXcv_noaccess) return;

    /* The ACCESS_MASK is ignored for "MonitorUI" */
    memset(buffer, 0, len);
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv_noaccess, cmd_MonitorUIW, NULL, 0, buffer, sizeof(buffer), &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u and 0x%x "
        "(expected 'ERROR_SUCCESS')\n", res, GetLastError(), needed);
}

/* ########################### */

static void test_XcvDataPort_PortIsValid(void)
{
    DWORD   res;
    DWORD   needed;

    /* normal use: "LPT1:" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, &needed);
    if (res == ERROR_INVALID_PARAMETER) {
        skip("'PostIsValid' not supported\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());


    if (0) {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, NULL, 0, NULL, 0, &needed);
    }


    /* hXcv is ignored for the command "PortIsValid" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(NULL, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, NULL);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());

    /* needed is ignored */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, NULL);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());


    /* cbInputData is ignored */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, 0, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, 1, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W) -1, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);

    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W) -2, NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* an empty name is not allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) emptyW, sizeof(emptyW), NULL, 0, &needed);
    ok( res == ERROR_PATH_NOT_FOUND,
        "returned %d with %u and 0x%x (expected ERROR_PATH_NOT_FOUND)\n",
        res, GetLastError(), needed);


    /* a directory is not allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) tempdirW, (lstrlenW(tempdirW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    /* XP(admin): ERROR_INVALID_NAME, XP(user): ERROR_PATH_NOT_FOUND, w2k ERROR_ACCESS_DENIED */
    ok( (res == ERROR_INVALID_NAME) || (res == ERROR_PATH_NOT_FOUND) ||
        (res == ERROR_ACCESS_DENIED), "returned %d with %u and 0x%x "
        "(expected ERROR_INVALID_NAME, ERROR_PATH_NOT_FOUND or ERROR_ACCESS_DENIED)\n",
        res, GetLastError(), needed);


    /* test more valid well known Ports: */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_lpt2W, sizeof(portname_lpt2W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_com1W, sizeof(portname_com1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_com2W, sizeof(portname_com2W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) portname_fileW, sizeof(portname_fileW), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* a normal, writable file is allowed */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv, cmd_PortIsValidW, (PBYTE) tempfileW, (lstrlenW(tempfileW) + 1) * sizeof(WCHAR), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS,
        "returned %d with %u and 0x%x (expected ERROR_SUCCESS)\n",
        res, GetLastError(), needed);


    /* small check without access-rights: */
    if (!hXcv_noaccess) return;

    /* The ACCESS_MASK from XcvOpenPort is ignored in "PortIsValid" */
    needed = (DWORD) 0xdeadbeef;
    SetLastError(0xdeadbeef);
    res = pXcvDataPort(hXcv_noaccess, cmd_PortIsValidW, (PBYTE) portname_lpt1W, sizeof(portname_lpt1W), NULL, 0, &needed);
    ok( res == ERROR_SUCCESS, "returned %d with %u (expected ERROR_SUCCESS)\n", res, GetLastError());

}

/* ########################### */

static void test_XcvOpenPort(void)
{
    DWORD   res;
    HANDLE  hXcv2;


    if (0)
    {
    /* crash with native localspl.dll (w2k+xp) */
    res = pXcvOpenPort(NULL, SERVER_ACCESS_ADMINISTER, &hXcv2);
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, NULL);
    }


    /* The returned handle is the result from a previous "spoolss.dll,DllAllocSplMem" */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv2);
    ok(res, "returned %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);


    /* The ACCESS_MASK is not checked in XcvOpenPort */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(emptyW, 0, &hXcv2);
    ok(res, "returned %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);


    /* A copy of pszObject is saved in the Memory-Block */
    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(portname_lpt1W, SERVER_ALL_ACCESS, &hXcv2);
    ok(res, "returned %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);

    SetLastError(0xdeadbeef);
    hXcv2 = (HANDLE) 0xdeadbeef;
    res = pXcvOpenPort(portname_fileW, SERVER_ALL_ACCESS, &hXcv2);
    ok(res, "returned %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv2);
    if (res) pXcvClosePort(hXcv2);

}

/* ########################### */

#define GET_MONITOR_FUNC(name) \
            if(numentries > 0) { \
                numentries--; \
                p##name = pm->Monitor.pfn##name ;  \
            }


START_TEST(localmon)
{
    DWORD   numentries;
    DWORD   res;

    LoadLibraryA("winspool.drv");
    /* This DLL does not exist on Win9x */
    hdll = LoadLibraryA("localspl.dll");
    if (!hdll) {
        skip("localspl.dll cannot be loaded, most likely running on Win9x\n");
        return;
    }

    tempdirW[0] = '\0';
    tempfileW[0] = '\0';
    res = GetTempPathW(MAX_PATH, tempdirW);
    ok(res != 0, "with %u\n", GetLastError());
    res = GetTempFileNameW(tempdirW, wineW, 0, tempfileW);
    ok(res != 0, "with %u\n", GetLastError());

    pInitializePrintMonitor = (void *) GetProcAddress(hdll, "InitializePrintMonitor");

    if (!pInitializePrintMonitor) {
        /* The Monitor for "Local Ports" was in a separate dll before w2k */
        hlocalmon = LoadLibraryA("localmon.dll");
        if (hlocalmon) {
            pInitializePrintMonitor = (void *) GetProcAddress(hlocalmon, "InitializePrintMonitor");
        }
    }
    if (!pInitializePrintMonitor) return;

    /* Native localmon.dll / localspl.dll need a valid Port-Entry in:
       a) since xp: HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports 
       b) up to w2k: Section "Ports" in win.ini
       or InitializePrintMonitor fails. */
    pm = pInitializePrintMonitor(Monitors_LocalPortW);
    if (pm) {
        numentries = (pm->dwMonitorSize ) / sizeof(VOID *);
        /* NT4: 14, since w2k: 17 */
        ok( numentries == 14 || numentries == 17, 
            "dwMonitorSize (%d) => %d Functions\n", pm->dwMonitorSize, numentries);

        GET_MONITOR_FUNC(EnumPorts);
        GET_MONITOR_FUNC(OpenPort);
        GET_MONITOR_FUNC(OpenPortEx);
        GET_MONITOR_FUNC(StartDocPort);
        GET_MONITOR_FUNC(WritePort);
        GET_MONITOR_FUNC(ReadPort);
        GET_MONITOR_FUNC(EndDocPort);
        GET_MONITOR_FUNC(ClosePort);
        GET_MONITOR_FUNC(AddPort);
        GET_MONITOR_FUNC(AddPortEx);
        GET_MONITOR_FUNC(ConfigurePort);
        GET_MONITOR_FUNC(DeletePort);
        GET_MONITOR_FUNC(GetPrinterDataFromPort);
        GET_MONITOR_FUNC(SetPortTimeOuts);
        GET_MONITOR_FUNC(XcvOpenPort);
        GET_MONITOR_FUNC(XcvDataPort);
        GET_MONITOR_FUNC(XcvClosePort);

        if ((pXcvOpenPort) && (pXcvDataPort) && (pXcvClosePort)) {
            SetLastError(0xdeadbeef);
            res = pXcvOpenPort(emptyW, SERVER_ACCESS_ADMINISTER, &hXcv);
            ok(res, "hXcv: %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv);

            SetLastError(0xdeadbeef);
            res = pXcvOpenPort(emptyW, 0, &hXcv_noaccess);
            ok(res, "hXcv_noaccess: %d with %u and %p (expected '!= 0')\n", res, GetLastError(), hXcv_noaccess);
        }
    }

    test_InitializePrintMonitor();

    find_installed_ports();

    test_AddPort();
    test_AddPortEx();
    test_ClosePort();
    test_ConfigurePort();
    test_DeletePort();
    test_EnumPorts();
    test_OpenPort();

    if ( !hXcv ) {
        skip("Xcv not supported\n");
    }
    else
    {
        test_XcvClosePort();
        test_XcvDataPort_AddPort();
        test_XcvDataPort_ConfigureLPTPortCommandOK();
        test_XcvDataPort_DeletePort();
        test_XcvDataPort_GetTransmissionRetryTimeout();
        test_XcvDataPort_MonitorUI();
        test_XcvDataPort_PortIsValid();
        test_XcvOpenPort();

        pXcvClosePort(hXcv);
    }
    if (hXcv_noaccess) pXcvClosePort(hXcv_noaccess);

    /* Cleanup our temporary file */
    DeleteFileW(tempfileW);
}
