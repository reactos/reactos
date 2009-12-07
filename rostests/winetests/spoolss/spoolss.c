/*
 * Unit test suite for the Spooler-Service helper DLL
 *
 * Copyright 2007 Detlef Riekenberg
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
#include "wine/test.h"



/* ##### */

static HMODULE hwinspool;

static HMODULE hspl;
static BOOL   (WINAPI * pBuildOtherNamesFromMachineName)(LPWSTR **, LPDWORD);
static DWORD  (WINAPI * pSplInitializeWinSpoolDrv)(LPVOID *);

#define WINSPOOL_TABLESIZE   16

static LPVOID fn_spl[WINSPOOL_TABLESIZE];
static LPVOID fn_w2k[WINSPOOL_TABLESIZE];
static LPVOID fn_xp[WINSPOOL_TABLESIZE];
static LPVOID fn_v[WINSPOOL_TABLESIZE];

/* ########################### */

static LPCSTR load_functions(void)
{
    LPCSTR  ptr;

    ptr = "spoolss.dll";
    hspl = LoadLibraryA(ptr);
    if (!hspl) return ptr;

    ptr = "BuildOtherNamesFromMachineName";
    pBuildOtherNamesFromMachineName = (void *) GetProcAddress(hspl, ptr);
    if (!pBuildOtherNamesFromMachineName) return ptr;

    ptr = "SplInitializeWinSpoolDrv";
    pSplInitializeWinSpoolDrv = (void *) GetProcAddress(hspl, ptr);
    if (!pSplInitializeWinSpoolDrv) return ptr;


    ptr = "winspool.drv";
    hwinspool = LoadLibraryA(ptr);
    if (!hwinspool) return ptr;

    memset(fn_w2k, 0xff, sizeof(fn_w2k));
    fn_w2k[0]  = (void *) GetProcAddress(hwinspool, "OpenPrinterW");
    fn_w2k[1]  = (void *) GetProcAddress(hwinspool, "ClosePrinter");
    fn_w2k[2]  = (void *) GetProcAddress(hwinspool, "SpoolerDevQueryPrintW");
    fn_w2k[3]  = (void *) GetProcAddress(hwinspool, "SpoolerPrinterEvent");
    fn_w2k[4]  = (void *) GetProcAddress(hwinspool, "DocumentPropertiesW");
    fn_w2k[5]  = (void *) GetProcAddress(hwinspool, (LPSTR) 212);  /* LoadPrinterDriver */
    fn_w2k[6]  = (void *) GetProcAddress(hwinspool, "SetDefaultPrinterW");
    fn_w2k[7]  = (void *) GetProcAddress(hwinspool, "GetDefaultPrinterW");
    fn_w2k[8]  = (void *) GetProcAddress(hwinspool, (LPSTR) 213);  /* RefCntLoadDriver */
    fn_w2k[9]  = (void *) GetProcAddress(hwinspool, (LPSTR) 214);  /* RefCntUnloadDriver */
    fn_w2k[10] = (void *) GetProcAddress(hwinspool, (LPSTR) 215);  /* ForceUnloadDriver */

    memset(fn_xp,  0xff, sizeof(fn_xp));
    fn_xp[0] = (void *) GetProcAddress(hwinspool, "OpenPrinterW");
    fn_xp[1] = (void *) GetProcAddress(hwinspool, "ClosePrinter");
    fn_xp[2] = (void *) GetProcAddress(hwinspool, "SpoolerDevQueryPrintW");
    fn_xp[3] = (void *) GetProcAddress(hwinspool, "SpoolerPrinterEvent");
    fn_xp[4] = (void *) GetProcAddress(hwinspool, "DocumentPropertiesW");
    fn_xp[5] = (void *) GetProcAddress(hwinspool, (LPSTR) 212);  /* LoadPrinterDriver */
    fn_xp[6] = (void *) GetProcAddress(hwinspool, (LPSTR) 213);  /* RefCntLoadDriver */
    fn_xp[7] = (void *) GetProcAddress(hwinspool, (LPSTR) 214);  /* RefCntUnloadDriver */
    fn_xp[8] = (void *) GetProcAddress(hwinspool, (LPSTR) 215);  /* ForceUnloadDriver */

    memset(fn_v,  0xff, sizeof(fn_v));
    fn_v[0] = (void *) GetProcAddress(hwinspool, "OpenPrinterW");
    fn_v[1] = (void *) GetProcAddress(hwinspool, "ClosePrinter");
    fn_v[2] = (void *) GetProcAddress(hwinspool, "SpoolerDevQueryPrintW");
    fn_v[3] = (void *) GetProcAddress(hwinspool, "SpoolerPrinterEvent");
    fn_v[4] = (void *) GetProcAddress(hwinspool, "DocumentPropertiesW");
    fn_v[5] = (void *) GetProcAddress(hwinspool, (LPSTR) 212);  /* LoadPrinterDriver */
    fn_v[6] = (void *) GetProcAddress(hwinspool, (LPSTR) 213);  /* RefCntLoadDriver */
    fn_v[7] = (void *) GetProcAddress(hwinspool, (LPSTR) 214);  /* RefCntUnloadDriver */
    fn_v[8] = (void *) GetProcAddress(hwinspool, (LPSTR) 215);  /* ForceUnloadDriver */
    fn_v[9] = (void *) GetProcAddress(hwinspool, (LPSTR) 251);  /* 0xfb */

    return NULL;

}

/* ########################### */

static void test_BuildOtherNamesFromMachineName(void)
{
    LPWSTR *buffers;
    DWORD   numentries;
    DWORD   res;

    buffers = NULL;
    numentries = 0;

    SetLastError(0xdeadbeef);
    res = pBuildOtherNamesFromMachineName(&buffers, &numentries);

    /* An array with a number of stringpointers is returned (minimum of 3):
      entry_#0: "" (empty String)
      entry_#1: <hostname> (this is the same as the computername)
      1 entry per Ethernet adapter : <ip-address> (string with a IPv4 ip-address)

      As of Vista:

      IPv6 fully disabled (lan interfaces, connections, tunnel interfaces and loopback interfaces)
      entry_#0: "" (empty String)
      entry_#1: <hostname> (this is the same as the computername)
      1 entry per Ethernet adapter : <ip-address> (string with a IPv4 ip-address)
      entry_#x: "::1"

      IPv6 partly disabled (lan interfaces, connections):
      entry_#0: "" (empty String)
      entry_#1: <hostname> (this is the same as the computername)
      entry_#2: "::1"
      1 entry per Ethernet adapter : <ip-address> (string with a IPv4 ip-address)

      IPv6 fully enabled but not on all lan interfaces:
      entry_#0: "" (empty String)
      entry_#1: <hostname> (this is the same as the computername)
      1 entry per IPv6 enabled Ethernet adapter : <ip-address> (string with a Link-local IPv6 ip-address)
      1 entry per IPv4 enabled Ethernet adapter : <ip-address> (string with a IPv4 ip-address)

      IPv6 fully enabled on all lan interfaces:
      entry_#0: "" (empty String)
      entry_#1: <hostname> (this is the same as the computername)
      1 entry per IPv6 enabled Ethernet adapter : <ip-address> (string with a Link-local IPv6 ip-address)
      entry_#x: <ip-address> Tunnel adapter (string with a Link-local IPv6 ip-address)
      1 entry per IPv4 enabled Ethernet adapter : <ip-address> (string with a IPv4 ip-address)
      entry_#y: <ip-address> Tunnel adapter (string with a IPv6 ip-address)
    */

    todo_wine
    ok( res && (buffers != NULL) && (numentries >= 3) && (buffers[0] != NULL) && (buffers[0][0] == '\0'),
        "got %u with %u and %p,%u (%p:%d)\n", res, GetLastError(), buffers, numentries,
        ((numentries > 0) && buffers) ? buffers[0] : NULL,
        ((numentries > 0) && buffers && buffers[0]) ? lstrlenW(buffers[0]) : -1);

}

/* ########################### */

static void test_SplInitializeWinSpoolDrv(VOID)
{
    LPVOID *fn_ref = fn_xp;
    DWORD   res;
    LONG    id;

    memset(fn_spl, 0xff, sizeof(fn_spl));
    SetLastError(0xdeadbeef);
    res = pSplInitializeWinSpoolDrv(fn_spl);
    ok(res, "got %u with %u (expected '!= 0')\n", res, GetLastError());

    /* functions 0 to 5 are the same in "spoolss.dll" from w2k and above */
    if (fn_spl[6] == fn_w2k[6]) {
        fn_ref = fn_w2k;
    }
    if (fn_spl[9] == fn_v[9]) {
        fn_ref = fn_v;
    }

    id = 0;
    while (id < WINSPOOL_TABLESIZE) {
        ok( fn_spl[id] == fn_ref[id],
            "(#%02u) spoolss: %p (vista: %p,  xp: %p,  w2k: %p)\n",
            id, fn_spl[id], fn_v[id], fn_xp[id], fn_w2k[id]);
        id++;
    }
}

/* ########################### */

START_TEST(spoolss)
{
    LPCSTR ptr;

    /* spoolss.dll does not exist on win9x */
    ptr = load_functions();
    if (ptr) {
        skip("%s not found\n", ptr);
        return;
    }

    test_BuildOtherNamesFromMachineName();
    test_SplInitializeWinSpoolDrv();

}
