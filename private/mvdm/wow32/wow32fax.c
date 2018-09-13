//****************************************************************************
// WOW32 fax support.
//
// History:
//    02-jan-95   nandurir   created.
//    01-feb-95   reedb      Clean-up, support printer install and bug fixes.
//
//****************************************************************************


//****************************************************************************
// This expalins how all this works (sort of) using WinFax as example.
// Install:
//  1. App calls WriteProfileString("devices","WINFAX","WINFAX,Com1:")
//     to register a "printer" in Win.ini a-la Win3.1
//  2. Our thunks of WritexxxProfileString() look for the "devices" string
//     and pass the call to IsFaxPrinterWriteProfileString(lpszSection,lpszKey,
//     lpszString).
//  3. If lpszKey ("WINFAX" in this case) is in our supported fax drivers list
//     (See Reg\SW\MS\WinNT\CurrentVersion\WOW\WOWFax\SupportedFaxDrivers)
//     (by call to IsFaxPrinterSupportedDevice()), we call InstallWowFaxPrinter
//     to add the printer the NT way -- via AddPrinter().
//  4. To set up the call to AddPrinter, we copy WOWFAX.DLL and WOWFAXUI.DLL to
//     the print spooler driver directory (\NT\system32\spool\drivers\w32x86\2)
//  5. We next call AddPrinterDriver to register the wowfax driver.
//  6. We then call wow32!DoAddPrinterStuff which launches a new thread,
//     wow32!AddPrinterThread, which calls winspool.drv!AddPrinter() for us. The
//     PrinterInfo.pPrinterName = the 16-bit fax driver name,"WINFAX" in this 
//     case.  WinSpool.drv then does a RPC call into the spooler.
//  7. During the AddPrinter() call, the spooler calls back into the driver to
//     get driver specific info.  These callbacks are handled by our WOWFAX
//     driver in the spooler's process.  They essentially callback into WOW
//     via wow32!WOWFaxWndProc().
//  8. WOWFaxWndProc() passes the callback onto WOW32FaxHandler, which calls
//     back to wowexec!FaxWndProc().
//  9. FaxWndProc then calls the 16-bit LoadLibrary() to open the 16-bit fax 
//     driver (WinFax.drv in this case).
// 10. The messages sent to FaxWndProc tell it which exported function it needs
//     call in the 16-bit driver on behalf of the spooler.
// 11. Any info the spooler wants to pass to the 16-bit driver or get from it
//     essentially goes through the mechanism in steps 7 - 10.
// Now you know (sort of).
//****************************************************************************
//
// Notes on what allows us to support a 16-bit fax driver:
// Essentially we have to know in advance which API's an app will call in the
// driver so we can handle the thunks.  It turns out that fax drivers only
// need to export a small essential list of API's:
//    Control, Disable, Enable, BitBlt, ExtDeviceMode, DeviceCapabilities
// (see mvdm\inc\wowfax.h\_WOWFAXINFO16 struct (all the PASCAL declarations)
//  and mvdm\wow16\test\shell\wowexfax.c\FaxWndProc() )
// The list is way too big to support 16-bit printer & display drivers.
// If a 16-bit fax driver exports these API's there's a pretty good chance
// we can support it in WOW. Other issues to look into: the dlgproc's the
// driver export's, any obsolete Win 3.0 API's that the NT spooler won't know
// how to call.
//
//****************************************************************************



#include "precomp.h"
#pragma hdrstop
#define WOWFAX_INC_COMMON_CODE
#include "wowgdip.h"
#define DEFINE_DDRV_DEBUG_STRINGS
#include "wowfax.h"
#include "winddi.h"
#include "winspool.h"

MODNAME(wowfax.c);

typedef struct _WOWADDPRINTER {
    LPVOID  pPrinterStuff;
    INT     iCode;
    BOOL    bRet;
} WOWADDPRINTER, *PWOWADDPRINTER;

//****************************************************************************
// globals -
//
//****************************************************************************

DWORD DeviceCapsHandler(LPWOWFAXINFO lpfaxinfo);
DWORD ExtDevModeHandler(LPWOWFAXINFO lpfaxinfo);
BOOL ConvertDevMode(PDEVMODE16 lpdm16, LPDEVMODEW lpdmW, BOOL fTo16);
BOOL ConvertGdiInfo(LPGDIINFO16 lpginfo16, PGDIINFO lpginfo, BOOL fTo16);

extern HANDLE hmodWOW32;

LPWOWFAXINFO glpfaxinfoCur = 0;
WOWFAXINFO   gfaxinfo;

UINT  uNumSupFaxDrv;
LPSTR *SupFaxDrv;

//****************************************************************************
// SortedInsert - Alpha sort.
//****************************************************************************

VOID SortedInsert(LPSTR lpElement, LPSTR *alpList)
{
    LPSTR lpTmp, lpSwap;

    while (*alpList) {
        if (WOW32_stricmp(lpElement, *alpList) < 0) {
            break;
        }
        alpList++;
    }
    lpTmp = *alpList;
    *alpList++ = lpElement;
    while (lpTmp) {
        // SWAP(*alpList, lpTmp);
        lpSwap = *alpList; *alpList = lpTmp; lpTmp = lpSwap;
        alpList++;
    }
}

//****************************************************************************
// BuildStrList - Find the starting point of strings in a list (lpList) of
//                NULL terminated strings which is double NULL terminated.
//                If a non-NULL alpList parameter is passed, it will be
//                filled with an array of pointers to the starting point
//                of each string in the list. The number of strings in the
//                list is always returned.
//****************************************************************************

UINT BuildStrList(LPSTR lpList, LPSTR *alpList)
{
    LPSTR lp;
    TCHAR cLastChar = 1;
    UINT  uCount = 0;

    lp  = lpList;
    while ((cLastChar) || (*lp)) {
        if ((*lp == 0) && (lp != lpList)) {
            uCount++;
        }

        if ((lpList == lp) || (cLastChar == 0)) {
            if ((*lp) && (alpList)) {
                SortedInsert(lp, alpList);
            }
        }
        cLastChar = *lp++;
    }
    return uCount;
}

//****************************************************************************
// GetSupportedFaxDrivers - Read in the SupFaxDrv name list from the
//                          registry. This list is used to determine if we will
//                          install a 16-bit fax printer driver during
//                          WriteProfileString and WritePrivateProfileString.
//****************************************************************************

LPSTR *GetSupportedFaxDrivers(UINT *uCount)
{
    HKEY  hKey = 0;
    DWORD dwType;
    DWORD cbBufSize;
    LPSTR lpSupFaxDrvBuf;
    LPSTR *alpSupFaxDrvList = NULL;

    *uCount = 0;

    // Open the registry key.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\WowFax\\SupportedFaxDrivers",
                     0, KEY_READ, &hKey ) != ERROR_SUCCESS) {
        goto GSFD_error;
    }

    // Query value for size of buffer and allocate.
    if (RegQueryValueEx(hKey, "DriverNames", 0, &dwType, NULL, &cbBufSize) != ERROR_SUCCESS) {
        goto GSFD_error;
    }
    if ((dwType != REG_MULTI_SZ) ||
        ((lpSupFaxDrvBuf = (LPSTR) malloc_w(cbBufSize)) == NULL)) {
        goto GSFD_error;
    }

    if (RegQueryValueEx(hKey, "DriverNames", 0, &dwType, lpSupFaxDrvBuf, &cbBufSize) != ERROR_SUCCESS) {
        goto GSFD_error;
    }

    // Get the number of elements in the list
    if (*uCount = BuildStrList(lpSupFaxDrvBuf, NULL)) {
        // Build an array of pointers to the start of the strings in the list.
        alpSupFaxDrvList = (LPSTR *) malloc_w(*uCount * sizeof(LPSTR));
        RtlZeroMemory(alpSupFaxDrvList, *uCount * sizeof(LPSTR));
        if (alpSupFaxDrvList) {
            // Fill the array with string starting points.
            BuildStrList(lpSupFaxDrvBuf, alpSupFaxDrvList);
        }
    }
    goto GSFD_exit;

GSFD_error:
    LOGDEBUG(0,("WOW32!GetSupportedFaxDrivers failed!\n"));

GSFD_exit:
    if (hKey) {
        RegCloseKey(hKey);
    }
    return alpSupFaxDrvList;
}


//****************************************************************************
// WowFaxWndProc - This is the 32-bit WndProc which will SubClass the 16-bit
//                 FaxWndProc in WOWEXEC.EXE. It's main function is to
//                 convert 32-bit data passed from the WOW 32-bit generic
//                 fax driver to 16-bit data to be used by the various 16-bit
//                 fax printer drivers.
//****************************************************************************

LONG WowFaxWndProc(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam)
{
    TCHAR  lpPath[MAX_PATH];
    HANDLE hMap;

    if ((uMsg >= WM_DDRV_FIRST) && (uMsg <= WM_DDRV_LAST)) {
        //
        // WM_DDRV_* message: uParam = idMap
        //                    lParam = unused.
        //
        // The corresponding data is obtained from the shared memory.
        //

        GetFaxDataMapName(uParam, lpPath);
        hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, lpPath);
        if (hMap) {
            LPWOWFAXINFO lpT;
            if (lpT = (LPWOWFAXINFO)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0)) {
                WOW32FaxHandler(lpT->msg, (LPSTR)lpT);

                // Set the status to TRUE indicating that the message
                // has been 'processed' by WOW. This doesnot indicate
                // the success or the failure of the actual processing
                // of the message.

                lpT->status = TRUE;
                UnmapViewOfFile(lpT);
                CloseHandle(hMap);
                return(TRUE);
            }
            CloseHandle(hMap);
        }
        LOGDEBUG(0,("WowFaxWndProc failed to setup shared data mapping!\n"));
        // WOW32ASSERT(FALSE);  // turn this off - Procomm tries to install
                                // this many times.
    }
    else {

        // Not a WM_DDRV_* message. Pass it on to the original proc.

        return CallWindowProc(gfaxinfo.proc16, hwnd, uMsg, uParam, lParam);
    }
    return(TRUE);
}

//**************************************************************************
// WOW32FaxHandler -
//
//      Handles various WowFax related operations.
//
//**************************************************************************

ULONG WOW32FaxHandler(UINT iFun, LPSTR lpIn)
{
    LPWOWFAXINFO lpT = (LPWOWFAXINFO)lpIn;
    LPWOWFAXINFO16 lpT16;
    HWND   hwnd = gfaxinfo.hwnd;
    LPBYTE lpData;
    VPVOID vp;

#ifdef DEBUG
    int    DebugStringIndex = iFun - (WM_USER+0x100+1);

    if ((DebugStringIndex >= WM_DDRV_FIRST) && (DebugStringIndex <= WM_DDRV_LAST) ) {
        LOGDEBUG(0,("WOW32FaxHandler, %s, 0x%lX\n", (LPSTR)szWmDdrvDebugStrings[DebugStringIndex], (LPSTR) lpIn));
    }
#endif

    switch (iFun) {
        case WM_DDRV_SUBCLASS:
            //
            // Subclass the window - This is so that we get a chance to
            // transform the 32bit data to 16bit data and vice versa. A
            // NULL HWND, passed in lpIn, indicates don't subclass.
            //

            if (gfaxinfo.hwnd = (HWND)lpIn) {
                gfaxinfo.proc16 = (WNDPROC)SetWindowLong((HWND)lpIn,
                                       GWL_WNDPROC, (DWORD)WowFaxWndProc);
                gfaxinfo.tid = GetWindowThreadProcessId((HWND)lpIn, NULL);
            }

            WOW32ASSERT(sizeof(DEVMODE16) + 4 == sizeof(DEVMODE31));

            //
            // Read in the SupFaxDrv name list from the registry.
            //

            SupFaxDrv = GetSupportedFaxDrivers(&uNumSupFaxDrv);

            break;

        case WM_DDRV_ENABLE:

            // Enable the driver:
            //    . first intialize the 16bit faxinfo datastruct
            //    . then inform the driver (dll name) to be loaded
            //
            //    format of ddrv_message:
            //            wParam = hdc (just a unique id)
            //            lparam = 16bit faxinfo struct with relevant data
            //    Must call 'callwindowproc' not 'sendmessage' because
            //    WowFaxWndProc is a subclass of the 16-bit FaxWndProc.
            //

            WOW32ASSERT(lpT->lpinfo16 == (LPSTR)NULL);
            lpT->lpinfo16 = (LPSTR)CallWindowProc( gfaxinfo.proc16,
                                       hwnd, WM_DDRV_INITFAXINFO16, lpT->hdc, (LPARAM)0);
            if (lpT->lpinfo16) {
                vp = malloc16(lpT->cData);
                GETVDMPTR(vp, lpT->cData, lpData);
                if (lpData == 0) {
                    break;
                }

                GETVDMPTR(lpT->lpinfo16, sizeof(WOWFAXINFO16), lpT16);
                if (lpT16) {
                    if (lstrlenW(lpT->szDeviceName) < sizeof(lpT16->szDeviceName)) {
                        WideCharToMultiByte(CP_ACP, 0,
                                           lpT->szDeviceName,
                                           lstrlenW(lpT->szDeviceName) + 1,
                                           lpT16->szDeviceName,
                                           sizeof(lpT16->szDeviceName),
                                           NULL, NULL);

                        lpT16->lpDriverName = lpT->lpDriverName;
                        if (lpT->lpDriverName) {
                            lpT16->lpDriverName = (LPBYTE)vp + (DWORD)lpT->lpDriverName;
                            WideCharToMultiByte(CP_ACP, 0,
                                           (PWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName),
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName)) + 1,
                                           lpData + (DWORD)lpT->lpDriverName,
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName)) + 1,
                                           NULL, NULL);
                        }

                        lpT16->lpPortName = lpT->lpPortName;
                        if (lpT->lpPortName) {
                            lpT16->lpPortName = (LPBYTE)vp + (DWORD)lpT->lpPortName;
                            WideCharToMultiByte(CP_ACP, 0,
                                           (PWSTR)((LPSTR)lpT + (DWORD)lpT->lpPortName),
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpPortName)) + 1,
                                           lpData + (DWORD)lpT->lpPortName,
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpPortName)) + 1,
                                           NULL, NULL);
                        }


                        lpT16->lpIn = lpT->lpIn;

                        if (lpT->lpIn) {
                            lpT16->lpIn = (LPBYTE)vp + (DWORD)lpT->lpIn;
                            ConvertDevMode((PDEVMODE16)(lpData + (DWORD)lpT->lpIn),
                                           (LPDEVMODEW)((LPSTR)lpT + (DWORD)lpT->lpIn), TRUE);
                        }
                        WOW32ASSERT((sizeof(GDIINFO16) + sizeof(POINT16)) <= sizeof(GDIINFO));
                        lpT16->lpOut = (LPBYTE)vp + (DWORD)lpT->lpOut;
                        FREEVDMPTR(lpData);
                        FREEVDMPTR(lpT16);
                        lpT->retvalue = CallWindowProc( gfaxinfo.proc16,
                                            hwnd, lpT->msg, lpT->hdc, (LPARAM)lpT->lpinfo16);
                        if (lpT->retvalue) {
                            GETVDMPTR(vp, lpT->cData, lpData);
                            ConvertGdiInfo((LPGDIINFO16)(lpData + (DWORD)lpT->lpOut),
                                           (PGDIINFO)((LPSTR)lpT + (DWORD)lpT->lpOut), FALSE);

                        }
                    }
                }
                free16(vp);
            }
            break;

        case WM_DDRV_ESCAPE:
            GETVDMPTR(lpT->lpinfo16, sizeof(WOWFAXINFO16), lpT16);
            if (lpT16) {
                lpT16->wCmd = lpT->wCmd;
            }
            FREEVDMPTR(lpT16);
            lpT->retvalue = CallWindowProc( gfaxinfo.proc16,
                                hwnd, lpT->msg, lpT->hdc, (LPARAM)lpT->lpinfo16);
            break;

        case WM_DDRV_PRINTPAGE:
            //
            // set the global variable. When the 16bit driver calls DMBitBlt we
            // get the bitmap info from here. Since WOW is single threaded we
            // won't receive another printpage msg before we return from here.
            //
            // All pointers in the faxinfo structure are actually
            // 'offsets from the start of the mapfile' to relevant data.
            //


            glpfaxinfoCur = lpT;
            lpT->lpbits = (LPBYTE)lpT + (DWORD)lpT->lpbits;

            // fall through;

        case WM_DDRV_STARTDOC:
            // WowFax (EasyFax Ver2.0) support...
            GETVDMPTR(lpT->lpinfo16, sizeof(WOWFAXINFO16), lpT16);
            if (lpT16) {
                WideCharToMultiByte(CP_ACP, 0,
                                    lpT->szDocName,
                                    lstrlenW(lpT->szDocName) + 1,
                                    lpT16->szDocName,
                                    sizeof(lpT16->szDocName),
                                    NULL, NULL);
            }
            lpT->retvalue = CallWindowProc( gfaxinfo.proc16,
                                hwnd, lpT->msg, lpT->hdc, (LPARAM)lpT->lpinfo16);
            break;

        case WM_DDRV_ENDDOC:
            lpT->retvalue = CallWindowProc( gfaxinfo.proc16,
                                hwnd, lpT->msg, lpT->hdc, (LPARAM)lpT->lpinfo16);
            break;

        case WM_DDRV_DISABLE:
            CallWindowProc( gfaxinfo.proc16,
                                hwnd, lpT->msg, lpT->hdc, (LPARAM)lpT->lpinfo16);
            lpT->retvalue = TRUE;
            break;


        case WM_DDRV_EXTDMODE:
        case WM_DDRV_DEVCAPS:
            WOW32ASSERT(lpT->lpinfo16 == (LPSTR)NULL);
            lpT->lpinfo16 = (LPSTR)CallWindowProc( gfaxinfo.proc16,
                                       hwnd, WM_DDRV_INITFAXINFO16, lpT->hdc, (LPARAM)0);
            if (lpT->lpinfo16) {
                vp = malloc16(lpT->cData);
                GETVDMPTR(vp, lpT->cData, lpData);
                if (lpData == 0) {
                    break;
                }
                GETVDMPTR(lpT->lpinfo16, sizeof(WOWFAXINFO16), lpT16);
                if (lpT16) {
                    if (lstrlenW(lpT->szDeviceName) < sizeof(lpT16->szDeviceName)) {
                        WideCharToMultiByte(CP_ACP, 0,
                                           lpT->szDeviceName,
                                           lstrlenW(lpT->szDeviceName) + 1,
                                           lpT16->szDeviceName,
                                           sizeof(lpT16->szDeviceName),
                                           NULL, NULL);

                        lpT16->lpDriverName = lpT->lpDriverName;
                        if (lpT->lpDriverName) {
                            lpT16->lpDriverName = (LPBYTE)vp + (DWORD)lpT->lpDriverName;
                            WideCharToMultiByte(CP_ACP, 0,
                                           (PWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName),
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName)) + 1,
                                           lpData + (DWORD)lpT->lpDriverName,
                                           lstrlenW((LPWSTR)((LPSTR)lpT + (DWORD)lpT->lpDriverName)) + 1,
                                           NULL, NULL);
                        }

                        FREEVDMPTR(lpData);
                        FREEVDMPTR(lpT16);
                        lpT->retvalue = CallWindowProc( gfaxinfo.proc16,
                                            hwnd, WM_DDRV_LOAD, lpT->hdc, (LPARAM)lpT->lpinfo16);
                        if (lpT->retvalue) {
                            lpT->retvalue = (iFun == WM_DDRV_DEVCAPS) ? DeviceCapsHandler(lpT) :
                                                                        ExtDevModeHandler(lpT) ;
                        }
                        CallWindowProc( gfaxinfo.proc16,
                                            hwnd, WM_DDRV_UNLOAD, lpT->hdc, (LPARAM)lpT->lpinfo16);
                    }
                }
                free16(vp);
            }
            break;
    }

    return TRUE;
}

//**************************************************************************
// gDC_CopySize -
//
//      Indicates the size of a list item in bytes for use during
//      the DeviceCapsHandler thunk. A zero entry indicates that an
//      allocate and copy is not needed for the query.
//
//**************************************************************************

BYTE gDC_ListItemSize[DC_COPIES + 1] = {
    0,
    0,                  // DC_FIELDS           1
    sizeof(WORD),       // DC_PAPERS           2
    sizeof(POINT),      // DC_PAPERSIZE        3
    sizeof(POINT),      // DC_MINEXTENT        4
    sizeof(POINT),      // DC_MAXEXTENT        5
    sizeof(WORD),       // DC_BINS             6
    0,                  // DC_DUPLEX           7
    0,                  // DC_SIZE             8
    0,                  // DC_EXTRA            9
    0,                  // DC_VERSION          10
    0,                  // DC_DRIVER           11
    24,                 // DC_BINNAMES         12 //ANSI
    sizeof(LONG) * 2,   // DC_ENUMRESOLUTIONS  13
    64,                 // DC_FILEDEPENDENCIES 14 //ANSI
    0,                  // DC_TRUETYPE         15
    64,                 // DC_PAPERNAMES       16 //ANSI
    0,                  // DC_ORIENTATION      17
    0                   // DC_COPIES           18
};

//**************************************************************************
// DeviceCapsHandler -
//
//      Makes a single call down to the 16-bit printer driver for queries
//      which don't need to allocate and copy. For queries which do, two
//      calls to the 16-bit printer driver are made. One to get the number
//      of items, and a second to get the actual data.
//
//**************************************************************************

DWORD DeviceCapsHandler(LPWOWFAXINFO lpfaxinfo)
{
    LPWOWFAXINFO16 lpWFI16;
    LPSTR          lpSrc;
    LPBYTE         lpDest;
    INT            i;
    DWORD          cbData16;  // Size of data items.
    UINT           cbUni;

    LOGDEBUG(0,("DeviceCapsHandler, lpfaxinfo: %X, wCmd: %X\n", lpfaxinfo, lpfaxinfo->wCmd));

    GETVDMPTR(lpfaxinfo->lpinfo16, sizeof(WOWFAXINFO16), lpWFI16);

    // Get the number of data items with a call to the 16-bit printer driver.

    lpWFI16->lpDriverName = 0;
    lpWFI16->lpPortName = 0;
    lpWFI16->wCmd = lpfaxinfo->wCmd;
    lpWFI16->cData = 0;
    lpWFI16->lpOut = 0;
    lpfaxinfo->cData = 0;

    lpfaxinfo->retvalue = CallWindowProc(gfaxinfo.proc16, gfaxinfo.hwnd,
                                         lpfaxinfo->msg, lpfaxinfo->hdc,
                                         (LPARAM)lpfaxinfo->lpinfo16);

    cbData16 = gDC_ListItemSize[lpfaxinfo->wCmd];
    if (lpfaxinfo->lpOut && cbData16 && lpfaxinfo->retvalue) {

        // We need to allocate and copy for this query
        lpWFI16->cData = cbData16 * lpfaxinfo->retvalue;

        // assert the size of output buffer - and set it the actual data size
        switch (lpfaxinfo->wCmd) {
            case DC_BINNAMES:
            case DC_PAPERNAMES:
                // These fields need extra room for ANSI to UNICODE conversion.
                WOW32ASSERT((lpfaxinfo->cData - (DWORD)lpfaxinfo->lpOut) >= lpWFI16->cData * sizeof(WCHAR));
                lpfaxinfo->cData = lpWFI16->cData * sizeof(WCHAR);
                break;
            default:
                WOW32ASSERT((lpfaxinfo->cData - (DWORD)lpfaxinfo->lpOut) >= lpWFI16->cData);
                lpfaxinfo->cData = lpWFI16->cData;
                break;
        }

        if ((lpWFI16->lpOut = (LPSTR)malloc16(lpWFI16->cData)) == NULL) {
            lpfaxinfo->retvalue = 0;
            goto LeaveDeviceCapsHandler;
        }

        // Get the list data with a call to the 16-bit printer driver.
        lpfaxinfo->retvalue = CallWindowProc(gfaxinfo.proc16, gfaxinfo.hwnd,
                                             lpfaxinfo->msg, lpfaxinfo->hdc,
                                             (LPARAM)lpfaxinfo->lpinfo16);

        GETVDMPTR(lpWFI16->lpOut, 0, lpSrc);
        lpDest = (LPBYTE)lpfaxinfo + (DWORD)lpfaxinfo->lpOut;

        switch (lpfaxinfo->wCmd) {
            case DC_BINNAMES:
            case DC_PAPERNAMES:
                for (i = 0; i < (INT)lpfaxinfo->retvalue; i++) {
                     RtlMultiByteToUnicodeN((LPWSTR)lpDest,
                                            cbData16 * sizeof(WCHAR),
                                            (PULONG)&cbUni,
                                            (LPBYTE)lpSrc, cbData16);
                     lpDest += cbData16 * sizeof(WCHAR);
                     lpSrc += cbData16;
                }
                break;

            default:
#ifdef FE_SB // for buggy fax driver such as CB-FAX Pro (Bother Corp.)
                try {
                    RtlCopyMemory(lpDest, lpSrc, lpWFI16->cData);
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    // What can I do for the exception... ????
                    // Anyway, we don't want to die.....
                    #if DBG
                    LOGDEBUG(0,("Exception during copying some data\n"));
                    #endif
                }
#else // !FE_SB
                RtlCopyMemory(lpDest, lpSrc, lpWFI16->cData);
#endif // !FE_SB
                break;
        }
        free16((VPVOID)lpWFI16->lpOut);
        FREEVDMPTR(lpSrc);
    }

LeaveDeviceCapsHandler:
    FREEVDMPTR(lpWFI16);
    return lpfaxinfo->retvalue;
}

//**************************************************************************
// ExtDevModeHandler
//
//**************************************************************************

DWORD ExtDevModeHandler(LPWOWFAXINFO lpfaxinfo)
{
    LPWOWFAXINFO16 lpT16;
    LPSTR          lpT;
    VPVOID         vp;

    LOGDEBUG(0,("ExtDevModeHandler\n"));

    (LONG)lpfaxinfo->retvalue = -1;

    GETVDMPTR(lpfaxinfo->lpinfo16, sizeof(WOWFAXINFO16), lpT16);

    if (lpT16) {

        // assumption that 16bit data won't be larger than 32bit data.
        // this makes life easy in two ways; first we don't need to calculate
        // the exact size and secondly the 16bit pointers can be set to same
        // relative offsets as input(32 bit) pointers

        vp = malloc16(lpfaxinfo->cData);
        if (vp) {
            GETVDMPTR(vp, lpfaxinfo->cData, lpT);
            if (lpT) {
                lpT16->wCmd = lpfaxinfo->wCmd;
                lpT16->lpOut = (LPSTR)lpfaxinfo->lpOut;
                lpT16->lpIn = (LPSTR)lpfaxinfo->lpIn;
                lpT16->lpDriverName = (LPBYTE)vp + (DWORD)lpfaxinfo->lpDriverName;
                lpT16->lpPortName = (LPBYTE)vp + (DWORD)lpfaxinfo->lpPortName;
                WideCharToMultiByte(CP_ACP, 0,
                                       (PWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpDriverName),
                                       lstrlenW((LPWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpDriverName)) + 1,
                                       lpT + (DWORD)lpfaxinfo->lpDriverName,
                                       lstrlenW((LPWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpDriverName)) + 1,
                                       NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0,
                                       (PWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpPortName),
                                       lstrlenW((LPWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpPortName)) + 1,
                                       lpT + (DWORD)lpfaxinfo->lpPortName,
                                       lstrlenW((LPWSTR)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpPortName)) + 1,
                                       NULL, NULL);
                if (lpfaxinfo->lpIn) {
                    lpT16->lpIn = (LPBYTE)vp + (DWORD)lpfaxinfo->lpIn;
                    ConvertDevMode((PDEVMODE16)(lpT + (DWORD)lpfaxinfo->lpIn),
                                   (LPDEVMODEW)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpIn), TRUE);
                }

                if (lpfaxinfo->lpOut) {
                    lpT16->lpOut = (LPBYTE)vp + (DWORD)lpfaxinfo->lpOut;
                }

                lpT16->hwndui = GETHWND16(lpfaxinfo->hwndui);

                FREEVDMPTR(lpT);
                lpfaxinfo->retvalue = CallWindowProc( gfaxinfo.proc16, gfaxinfo.hwnd,
                                              lpfaxinfo->msg, lpfaxinfo->hdc, (LPARAM)lpfaxinfo->lpinfo16);

                if ((lpfaxinfo->wCmd == 0) && (lpfaxinfo->retvalue > 0)) {
                    // the 16bit driver has returned 16bit struct size. change
                    // the return value to correspond to the devmodew struct.
                    //
                    // since devmode16 (the 3.0 version) is smaller than devmode31
                    // the retvalue will take careof both win30/win31 devmode

                    WOW32ASSERT(sizeof(DEVMODE16) < sizeof(DEVMODE31));
                    lpfaxinfo->retvalue += (sizeof(DEVMODEW) - sizeof(DEVMODE16));
                }

                GETVDMPTR(vp, lpfaxinfo->cData, lpT);

                if ((lpfaxinfo->wCmd & DM_COPY) &&
                              lpfaxinfo->lpOut && (lpfaxinfo->retvalue == IDOK)) {
                    ConvertDevMode((PDEVMODE16)(lpT + (DWORD)lpfaxinfo->lpOut),
                                         (LPDEVMODEW)((LPSTR)lpfaxinfo + (DWORD)lpfaxinfo->lpOut), FALSE);
                }

            }
            free16(vp);
        }

    }

    FREEVDMPTR(lpT16);

    return lpfaxinfo->retvalue;
}

//***************************************************************************
// ConvertDevMode
//***************************************************************************

BOOL ConvertDevMode(PDEVMODE16 lpdm16, LPDEVMODEW lpdmW, BOOL fTo16)
{
    LOGDEBUG(0,("ConvertDevMode\n"));

    if (!lpdm16 || !lpdmW)
        return TRUE;

    if (fTo16) {
        RtlZeroMemory(lpdm16, sizeof(DEVMODE16));

        WideCharToMultiByte(CP_ACP, 0,
              lpdmW->dmDeviceName,
              sizeof(lpdmW->dmDeviceName) / sizeof(lpdmW->dmDeviceName[0]),
              lpdm16->dmDeviceName,
              sizeof(lpdm16->dmDeviceName) / sizeof(lpdm16->dmDeviceName[0]),
              NULL, NULL);

        lpdm16->dmSpecVersion = lpdmW->dmSpecVersion;
        lpdm16->dmDriverVersion = lpdmW->dmDriverVersion;
        lpdm16->dmSize = lpdmW->dmSize;
        lpdm16->dmDriverExtra = lpdmW->dmDriverExtra;
        lpdm16->dmFields = lpdmW->dmFields;
        lpdm16->dmOrientation = lpdmW->dmOrientation;
        lpdm16->dmPaperSize = lpdmW->dmPaperSize;
        lpdm16->dmPaperLength = lpdmW->dmPaperLength;
        lpdm16->dmPaperWidth = lpdmW->dmPaperWidth;
        lpdm16->dmScale = lpdmW->dmScale;
        lpdm16->dmCopies = lpdmW->dmCopies;
        lpdm16->dmDefaultSource = lpdmW->dmDefaultSource;
        lpdm16->dmPrintQuality = lpdmW->dmPrintQuality;
        lpdm16->dmColor = lpdmW->dmColor;
        lpdm16->dmDuplex = lpdmW->dmDuplex;

        // adjust lpdm16->dmSize (between win30 and win31 version)

        lpdm16->dmSize = (lpdm16->dmSpecVersion > 0x300) ? sizeof(DEVMODE31) :
                                                            sizeof(DEVMODE16);
        if (lpdm16->dmSize >= sizeof(DEVMODE31)) {
            ((PDEVMODE31)lpdm16)->dmYResolution = lpdmW->dmYResolution;
            ((PDEVMODE31)lpdm16)->dmTTOption = lpdmW->dmTTOption;
        }

        RtlCopyMemory((LPBYTE)lpdm16 + (DWORD)lpdm16->dmSize, (lpdmW + 1),
                                                        lpdmW->dmDriverExtra);
    }
    else {

        // LATER: should specversion be NT version rather than win30 driver version?

        MultiByteToWideChar(CP_ACP, 0,
              lpdm16->dmDeviceName,
              sizeof(lpdm16->dmDeviceName) / sizeof(lpdm16->dmDeviceName[0]),
              lpdmW->dmDeviceName,
              sizeof(lpdmW->dmDeviceName) / sizeof(lpdmW->dmDeviceName[0]));

        lpdmW->dmSpecVersion = lpdm16->dmSpecVersion;
        lpdmW->dmDriverVersion = lpdm16->dmDriverVersion;
        lpdmW->dmSize = lpdm16->dmSize;
        lpdmW->dmDriverExtra = lpdm16->dmDriverExtra;
        lpdmW->dmFields = lpdm16->dmFields;
        lpdmW->dmOrientation = lpdm16->dmOrientation;
        lpdmW->dmPaperSize = lpdm16->dmPaperSize;
        lpdmW->dmPaperLength = lpdm16->dmPaperLength;
        lpdmW->dmPaperWidth = lpdm16->dmPaperWidth;
        lpdmW->dmScale = lpdm16->dmScale;
        lpdmW->dmCopies = lpdm16->dmCopies;
        lpdmW->dmDefaultSource = lpdm16->dmDefaultSource;
        lpdmW->dmPrintQuality = lpdm16->dmPrintQuality;
        lpdmW->dmColor = lpdm16->dmColor;
        lpdmW->dmDuplex = lpdm16->dmDuplex;

        if (lpdm16->dmSize >= sizeof(DEVMODE31)) {
            lpdmW->dmYResolution = ((PDEVMODE31)lpdm16)->dmYResolution;
            lpdmW->dmTTOption = ((PDEVMODE31)lpdm16)->dmTTOption;
        }

        // 16bit world doesnot know anything about the fields like
        // formname  etc.

        RtlCopyMemory(lpdmW + 1, (LPBYTE)lpdm16 + lpdm16->dmSize, lpdm16->dmDriverExtra);

        // adjust size for 32bit world

        lpdmW->dmSize = sizeof(*lpdmW);

    }

    return TRUE;
}

//**************************************************************************
// ConvertGdiInfo
//
//**************************************************************************


BOOL ConvertGdiInfo(LPGDIINFO16 lpginfo16, PGDIINFO lpginfo, BOOL fTo16)
{
    LOGDEBUG(0,("ConvertGdiInfo\n"));

    if (!lpginfo16 || !lpginfo)
        return FALSE;

    if (!fTo16) {
        lpginfo->ulTechnology = lpginfo16->dpTechnology;
        lpginfo->ulLogPixelsX = lpginfo16->dpLogPixelsX;
        lpginfo->ulLogPixelsY = lpginfo16->dpLogPixelsY;
        lpginfo->ulDevicePelsDPI = lpginfo->ulLogPixelsX;
        lpginfo->ulHorzSize = lpginfo16->dpHorzSize;
        lpginfo->ulVertSize = lpginfo16->dpVertSize;
        lpginfo->ulHorzRes  = lpginfo16->dpHorzRes;
        lpginfo->ulVertRes  = lpginfo16->dpVertRes;
        lpginfo->cBitsPixel = lpginfo16->dpBitsPixel;
        lpginfo->cPlanes    = lpginfo16->dpPlanes;
        lpginfo->ulNumColors = lpginfo16->dpNumColors;
        lpginfo->ptlPhysOffset.x = ((PPOINT16)(lpginfo16+1))->x;
        lpginfo->ptlPhysOffset.y = ((PPOINT16)(lpginfo16+1))->y;
        lpginfo->szlPhysSize.cx = lpginfo->ulHorzRes;
        lpginfo->szlPhysSize.cy = lpginfo->ulVertRes;
        lpginfo->ulPanningHorzRes = lpginfo->ulHorzRes;
        lpginfo->ulPanningVertRes = lpginfo->ulVertRes;
        lpginfo->ulAspectX = lpginfo16->dpAspectX;
        lpginfo->ulAspectY = lpginfo16->dpAspectY;
        lpginfo->ulAspectXY = lpginfo16->dpAspectXY;

        //
        // RASDD tries to be smart as to whether the x and y DPI are equal or
        // not.  In the case of 200dpi in the x direction and 100dpi in the
        // y direction, you may want to adjust this to 2 for xStyleStep, 1 for
        // yStyleStep and dpi/50 for denStyleStep.  This basicaly determines
        // how long dashes/dots will be when drawing with styled pens.
        // Since we just hard code denStyleStep to 3, we get different lines
        // at 100dpi vs 200dpi
        //

        lpginfo->xStyleStep = 1;
        lpginfo->yStyleStep = 1;
        lpginfo->denStyleStep = 3;
    }

    return TRUE;
}


//**************************************************************************
// DMBitBlt -
//     The 16bit winfax.drv calls this , in response to a device driver
//     'bitblt' call.
//
//**************************************************************************

ULONG FASTCALL WG32DMBitBlt( PVDMFRAME pFrame)
{
    register PDMBITBLT16 parg16;
#ifdef DBCS /* wowfax support */
    register PDEV_BITMAP16   pbm16;
#else // !DBCS
    register PBITMAP16   pbm16;
#endif /* !DBCS */
    LPBYTE  lpDest, lpSrc;
    UINT    cBytes;
    LPBYTE  lpbits, lpbitsEnd;

    LOGDEBUG(0,("WG32DMBitBlt\n"));

    GETARGPTR(pFrame, sizeof(DMBITBLT16), parg16);
#ifdef DBCS /* wowfax support */
    GETVDMPTR(parg16->pbitmapdest, sizeof(DEV_BITMAP16), pbm16);
#else // !DBCS
    GETVDMPTR(parg16->pbitmapdest, sizeof(BITMAP16), pbm16);
#endif /* !DBCS */
    GETVDMPTR(pbm16->bmBits, 0, lpDest);

    WOW32ASSERT(glpfaxinfoCur != NULL);
    lpbits = glpfaxinfoCur->lpbits;
    lpbitsEnd = (LPBYTE)lpbits + glpfaxinfoCur->bmHeight *
                                           glpfaxinfoCur->bmWidthBytes;

#ifdef DBCS /* wowfax support */
    lpSrc  = (LPBYTE)lpbits + (parg16->srcx / glpfaxinfoCur->bmPixPerByte) +
                              (parg16->srcy * glpfaxinfoCur->bmWidthBytes);

    if (lpSrc >= lpbits) {

        WORD    extx,exty,srcx,srcy,desty,destx;

        extx  = FETCHWORD(parg16->extx);
        exty  = FETCHWORD(parg16->exty);
        srcx  = FETCHWORD(parg16->srcx);
        srcy  = FETCHWORD(parg16->srcy);
        destx = FETCHWORD(parg16->destx);
        desty = FETCHWORD(parg16->desty);

        #if DBG
        LOGDEBUG(10,("\n"));
        LOGDEBUG(10,("bmType         = %d\n",pbm16->bmType));
        LOGDEBUG(10,("bmWidth        = %d\n",pbm16->bmWidth));
        LOGDEBUG(10,("bmHeight       = %d\n",pbm16->bmHeight));
        LOGDEBUG(10,("bmWidthBytes   = %d\n",pbm16->bmWidthBytes));
        LOGDEBUG(10,("bmPlanes       = %d\n",pbm16->bmPlanes));
        LOGDEBUG(10,("bmBitsPixel    = %d\n",pbm16->bmBitsPixel));
        LOGDEBUG(10,("bmBits         = %x\n",pbm16->bmBits));
        LOGDEBUG(10,("bmWidthPlances = %d\n",pbm16->bmWidthPlanes));
        LOGDEBUG(10,("bmlpPDevice    = %x\n",pbm16->bmlpPDevice));
        LOGDEBUG(10,("bmSegmentIndex = %d\n",pbm16->bmSegmentIndex));
        LOGDEBUG(10,("bmScanSegment  = %d\n",pbm16->bmScanSegment));
        LOGDEBUG(10,("bmFillBytes    = %d\n",pbm16->bmFillBytes));
        LOGDEBUG(10,("\n"));
        LOGDEBUG(10,("bmWidthBytesSrc= %d\n",glpfaxinfoCur->bmWidthBytes));
        LOGDEBUG(10,("\n"));
        LOGDEBUG(10,("extx           = %d\n",extx));
        LOGDEBUG(10,("exty           = %d\n",exty));
        LOGDEBUG(10,("srcx           = %d\n",srcx));
        LOGDEBUG(10,("srcy           = %d\n",srcy));
        LOGDEBUG(10,("destx          = %d\n",destx));
        LOGDEBUG(10,("desty          = %d\n",desty));
        LOGDEBUG(10,("\n"));
        #endif

        if (pbm16->bmSegmentIndex) {

            SHORT  WriteSegment;
            SHORT  WriteOffset;
            SHORT  Segment=0,SegmentMax=0;
            LPBYTE DstScan0,SrcScan0;
            UINT   cBytesInLastSegment;
            INT    RestLine = (INT) exty;
 
            WriteSegment = desty / pbm16->bmScanSegment;
            WriteOffset  = desty % pbm16->bmScanSegment;

            if (WriteOffset) {
                WriteSegment += 1;
            }

            #if DBG
            LOGDEBUG(10,("WriteSegment      = %d\n",WriteSegment));
            LOGDEBUG(10,("WriteOffset       = %d\n",WriteOffset));
            LOGDEBUG(10,("\n"));
            LOGDEBUG(10,("lpDest            = %x\n",lpDest));
            LOGDEBUG(10,("\n"));
            #endif

            SegmentMax = exty / pbm16->bmScanSegment;
            if ( exty % pbm16->bmScanSegment) {
                SegmentMax += 1;
            }

            cBytes = glpfaxinfoCur->bmWidthBytes * pbm16->bmScanSegment;
            lpDest = lpDest + destx + (WriteSegment * 0x10000L) +
                                      (WriteOffset  * pbm16->bmWidthBytes);

            #if DBG
            LOGDEBUG(10,("SourceBitmap      = %x\n",lpSrc));
            LOGDEBUG(10,("DestinationBitmap = %x\n",lpDest));
            LOGDEBUG(10,("SegmentMax        = %d\n",SegmentMax));
            LOGDEBUG(10,("\n"));
            LOGDEBUG(10,("cBytes            = %d\n",cBytes));
            LOGDEBUG(10,("\n"));
            #endif

            if ((DWORD)glpfaxinfoCur->bmWidthBytes == (DWORD)pbm16->bmWidthBytes) {

                try {
                    for( Segment = 1,DstScan0 = lpDest,SrcScan0 = lpSrc;
                         Segment < SegmentMax;
                         Segment++,DstScan0 += 0x10000L,
                         SrcScan0 += cBytes,RestLine -= pbm16->bmScanSegment ) {

                        #if DBG
                        LOGDEBUG(10,("%d ",Segment-1));
                        #endif

                        RtlCopyMemory(DstScan0,SrcScan0,cBytes);
                        RtlZeroMemory(DstScan0+cBytes,pbm16->bmFillBytes);
                    }

                    #if DBG
                    LOGDEBUG(10,("%d\n",Segment-1));
                    #endif

                    if( RestLine > 0 ) {
                       cBytesInLastSegment = RestLine * pbm16->bmWidthBytes;

                       #if DBG
                       LOGDEBUG(10,("RestLine            = %d\n",RestLine));
                       LOGDEBUG(10,("cBytesInLastSegment = %d\n",cBytes));
                       #endif

                       // do for last segment..
                       RtlCopyMemory(DstScan0,SrcScan0,cBytesInLastSegment);
                    }
                    
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    #if DBG
                    LOGDEBUG(10,("Exception during copying image\n"));
                    #endif
                }

            } else if ((DWORD)glpfaxinfoCur->bmWidthBytes > (DWORD)pbm16->bmWidthBytes) {

                SHORT Line;
                UINT  cSrcAdvance = glpfaxinfoCur->bmWidthBytes;
                UINT  cDstAdvance = pbm16->bmWidthBytes;

                try {
                    for( Segment = 1,DstScan0 = lpDest,SrcScan0 = lpSrc;
                         Segment < SegmentMax;
                         Segment++,DstScan0 += 0x10000L,
                         SrcScan0 += cBytes,RestLine -= pbm16->bmScanSegment ) {

                        LPBYTE DstScanl = DstScan0;
                        LPBYTE SrcScanl = SrcScan0;

                        #if DBG
                        LOGDEBUG(10,("%d ",Segment-1));
                        #endif

                        for( Line = 0;
                             Line < pbm16->bmScanSegment;
                             Line++,DstScanl += cDstAdvance,SrcScanl += cSrcAdvance ) {

                            RtlCopyMemory(DstScanl,SrcScanl,cDstAdvance);
                        }
                    }

                    #if DBG
                    LOGDEBUG(10,("%d\n",Segment-1));
                    #endif

                    if( RestLine > 0 ) {

                        LPBYTE DstScanl = DstScan0;
                        LPBYTE SrcScanl = SrcScan0;

                        for( Line = 0;
                             Line < RestLine;
                             Line++,DstScanl += cDstAdvance,SrcScanl += cSrcAdvance ) {

                            RtlCopyMemory(DstScanl,SrcScanl,cDstAdvance);
                        }
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    #if DBG
                    LOGDEBUG(10,("Exception during copying image\n"));
                    #endif
                }
            } else {
                WOW32ASSERT(FALSE);
            }

        } else {

            lpDest = lpDest + destx + desty * pbm16->bmWidthBytes;

            if ((DWORD)glpfaxinfoCur->bmWidthBytes  == (DWORD)pbm16->bmWidthBytes) {
                cBytes =  parg16->exty * glpfaxinfoCur->bmWidthBytes;
                if (cBytes > (UINT)(pbm16->bmHeight * pbm16->bmWidthBytes)) {
                    cBytes = pbm16->bmHeight * pbm16->bmWidthBytes;
                    WOW32ASSERT(FALSE);
                }
                if ((lpSrc + cBytes) <= lpbitsEnd) {
                    RtlCopyMemory(lpDest, lpSrc, cBytes);
                }
            } else {
                int i;

                // we need to transfer bits one partial scanline at a time
                WOW32ASSERT((DWORD)pbm16->bmHeight <= (DWORD)glpfaxinfoCur->bmHeight);
                WOW32ASSERT((DWORD)parg16->exty <= (DWORD)pbm16->bmHeight);

                cBytes = ((DWORD)pbm16->bmWidthBytes < (DWORD)glpfaxinfoCur->bmWidthBytes) ?
                                 pbm16->bmWidthBytes :        glpfaxinfoCur->bmWidthBytes;

                for (i = 0; i < parg16->exty; i++) {
                     if ((lpSrc + cBytes) <= lpbitsEnd) {
                         RtlCopyMemory(lpDest, lpSrc, cBytes);
                     }
                     lpDest += pbm16->bmWidthBytes;
                     lpSrc  += glpfaxinfoCur->bmWidthBytes;
                }
            }
        }
    }
#else // !DBCS
    lpDest = lpDest + parg16->destx + parg16->desty * pbm16->bmWidthBytes;
    lpSrc = (LPBYTE)lpbits + (parg16->srcx / glpfaxinfoCur->bmPixPerByte) +
                                 parg16->srcy * glpfaxinfoCur->bmWidthBytes;
    if (lpSrc >= lpbits) {
        if ((DWORD)glpfaxinfoCur->bmWidthBytes  == (DWORD)pbm16->bmWidthBytes) {
            cBytes =  parg16->exty * glpfaxinfoCur->bmWidthBytes;
            if (cBytes > (UINT)(pbm16->bmHeight * pbm16->bmWidthBytes)) {
                cBytes = pbm16->bmHeight * pbm16->bmWidthBytes;
                WOW32ASSERT(FALSE);
            }
            if ((lpSrc + cBytes) <= lpbitsEnd) {
                RtlCopyMemory(lpDest, lpSrc, cBytes);
            }
        }
        else if ((DWORD)glpfaxinfoCur->bmWidthBytes > (DWORD)pbm16->bmWidthBytes) {
            int i;

            // we need to transfer bits one partial scanline at a time
            WOW32ASSERT((DWORD)pbm16->bmHeight <= (DWORD)glpfaxinfoCur->bmHeight);
            WOW32ASSERT((DWORD)parg16->exty <= (DWORD)pbm16->bmHeight);

            for (i = 0; i < parg16->exty; i++) {
                 if ((lpSrc + pbm16->bmWidthBytes) <= lpbitsEnd) {
                     RtlCopyMemory(lpDest, lpSrc, pbm16->bmWidthBytes);
                 }
                 lpDest += pbm16->bmWidthBytes;
                 lpSrc  += glpfaxinfoCur->bmWidthBytes;
            }

        }
        else {
            WOW32ASSERT(FALSE);
        }


    }
#endif /* !DBCS */
    return (ULONG)TRUE;
}

PSZ StrDup(PSZ szStr)
{
    PSZ  pszTmp;

    pszTmp = malloc_w(strlen(szStr)+1);
    return(strcpy(pszTmp, szStr));
}

PSZ BuildPath(PSZ szPath, PSZ szFileName)
{
    char szTmp[MAX_PATH];

    strcpy(szTmp, szPath);
    strcat(szTmp, "\\");
    strcat(szTmp, szFileName);
    return(StrDup(szTmp));
}

//**************************************************************************
// AddPrinterThread -
//
//  Worker thread to make the AddPrinter call into the spooler.
//
//**************************************************************************

VOID AddPrinterThread(PWOWADDPRINTER pWowAddPrinter)
{
    
    if ((*spoolerapis[pWowAddPrinter->iCode].lpfn)(NULL, 2,
                                           pWowAddPrinter->pPrinterStuff)) {
        pWowAddPrinter->bRet = TRUE;
    }
    else {
        if (GetLastError() == ERROR_PRINTER_ALREADY_EXISTS) {
            pWowAddPrinter->bRet = TRUE;
        }
        else {
#ifdef DBG
            LOGDEBUG(0,("AddPrinterThread, AddPrinterxxx call failed: 0x%X\n", GetLastError()));
#endif
            pWowAddPrinter->bRet = FALSE;
        }
    }
}

//**************************************************************************
// DoAddPrinterStuff -
//
// Spin a worker thread to make the AddPrinterxxx calls into
// spooler. This is needed to prevent a deadlock when spooler
// RPC's to spoolss.
//
// This thread added for bug #107426.
//**************************************************************************

BOOL DoAddPrinterStuff(LPVOID pPrinterStuff, INT iCode)
{
    WOWADDPRINTER   WowAddPrinter;
    HANDLE          hWaitObjects;
    DWORD           dwEvent, dwUnused;
    MSG             msg;

    // Spin the worker thread.
    WowAddPrinter.pPrinterStuff = pPrinterStuff;
    WowAddPrinter.iCode = iCode;
    WowAddPrinter.bRet  = FALSE;
    if (hWaitObjects = CreateThread(NULL, 0,
                                    (LPTHREAD_START_ROUTINE)AddPrinterThread,
                                    &WowAddPrinter, 0, &dwUnused)) {

        // Pump messages while we wait for AddPrinterThread to finish.
        for (;;) {
            dwEvent = MsgWaitForMultipleObjects(1,
                                                &hWaitObjects,
                                                FALSE,
                                                INFINITE,
                                                QS_ALLEVENTS | QS_SENDMESSAGE);

            if (dwEvent == WAIT_OBJECT_0 + 0) {

                // Worker thread done.
                break;

            }
            else {
                // pump messages so the callback into wowexec!FaxWndProc doesn't
                // get hung
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        CloseHandle(hWaitObjects);
    }
    else {
        LOGDEBUG(0,
              ("DoAddPrinterStuff, CreateThread on AddPrinterThread failed\n"));
    }

    return WowAddPrinter.bRet;
}

//**************************************************************************
// InstallWowFaxPrinter -
//
//  Installs the WowFax 32-bit print driver when a 16-bit fax printer
//  installation is detected.
//
//**************************************************************************

BOOL InstallWowFaxPrinter(PSZ szSection, PSZ szKey, PSZ szString)
{
    CHAR  szTmp[MAX_PATH];
    PSZ   szSrcPath;
    DWORD dwNeeded;
    DRIVER_INFO_2 DriverInfo;
    PRINTER_INFO_2 PrinterInfo;
    PORT_INFO_1 PortInfo;
    HKEY hKey = 0, hSubKey = 0;
    BOOL bRetVal;

    LOGDEBUG(0,("InstallWowFaxPrinter, Section = %s, Key = %s, String = %s\n", szSection, szKey, szString));

    // Write the entry to the registry. We'll keep shadow entries
    // in the registry for the WOW fax applications and drivers to
    // read, since the entries that the spooler writes pertain
    // to winspool, not the 16-bit fax driver.

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\WowFax",
                      0, KEY_WRITE, &hKey ) == ERROR_SUCCESS) {
        if (RegCreateKey(hKey, szSection, &hSubKey) == ERROR_SUCCESS) {
            RegSetValueEx(hSubKey, szKey, 0, REG_SZ, szString, strlen(szString)+1);
            RegCloseKey(hKey);
            RegCloseKey(hSubKey);

            // Dynamically link to spooler API's
            if (!(*spoolerapis[WOW_GetPrinterDriverDirectory].lpfn)) {
                if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
                    LOGDEBUG(0,("InstallWowFaxPrinter, Unable to load WINSPOOL API's\n"));
                    return(FALSE);
                }
            }

            // Copy the printer driver files.
            RtlZeroMemory(&DriverInfo, sizeof(DRIVER_INFO_2));
            RtlZeroMemory(&PrinterInfo, sizeof(PRINTER_INFO_2));
            if (!(*spoolerapis[WOW_GetPrinterDriverDirectory].lpfn)(NULL, NULL, 1, szTmp, MAX_PATH, &dwNeeded)) {
                LOGDEBUG(0,("InstallWowFaxPrinter, GetPrinterDriverDirectory failed: 0x%X\n", GetLastError()));
                return(FALSE);
            }
 
            // This is a dummy. We've no data file, but spooler won't take NULL.
            DriverInfo.pDataFile = BuildPath(szTmp, WOWFAX_DLL_NAME_A);

            DriverInfo.pDriverPath = BuildPath(szTmp, WOWFAX_DLL_NAME_A);
            LOGDEBUG(0,("InstallWowFaxPrinter, pDriverPath = %s\n", DriverInfo.pDataFile));
            szSrcPath = BuildPath(pszSystemDirectory, WOWFAX_DLL_NAME_A);
            CopyFile(szSrcPath, DriverInfo.pDriverPath, FALSE);
            free_w(szSrcPath);

            DriverInfo.pConfigFile = BuildPath(szTmp, WOWFAXUI_DLL_NAME_A);
            szSrcPath = BuildPath(pszSystemDirectory, WOWFAXUI_DLL_NAME_A);
            CopyFile(szSrcPath, DriverInfo.pConfigFile, FALSE);
            free_w(szSrcPath);

            // Install the printer driver.
            DriverInfo.cVersion = 1;
            DriverInfo.pName = "Windows 3.1 Compatible Fax Driver";
            if ((bRetVal = DoAddPrinterStuff((LPVOID)&DriverInfo, 
                                             WOW_AddPrinterDriver)) == FALSE) {

                // if the driver is already installed, it won't hurt to install
                // it a second time.  This might be necessary if the user is
                // upgrading from WinFax Lite to WinFax Pro.
                bRetVal = (GetLastError() == ERROR_PRINTER_DRIVER_ALREADY_INSTALLED);
            }

            if (bRetVal) {
                // Parse out the printer name.
                RtlZeroMemory(&PrinterInfo, sizeof(PRINTER_INFO_2));
                PrinterInfo.pPrinterName = szKey;

                LOGDEBUG(0,("InstallWowFaxPrinter, pPrinterName = %s\n", PrinterInfo.pPrinterName));
 
                // Use private API to add a NULL port. Printer guys need to fix
                // redirection to NULL bug.
                RtlZeroMemory(&PortInfo, sizeof(PORT_INFO_1));
                PrinterInfo.pPortName = "NULL";
                PortInfo.pName = PrinterInfo.pPortName;

                // Get "Local Port" string.
                LoadString(hmodWOW32, iszWowFaxLocalPort, szTmp, sizeof szTmp);

                (*spoolerapis[WOW_AddPortEx].lpfn)(NULL, 1, &PortInfo, szTmp);

                // Set the other defaults and install the printer.
                PrinterInfo.pDriverName     = "Windows 3.1 Compatible Fax Driver";
                PrinterInfo.pPrintProcessor = "WINPRINT";
                PrinterInfo.pDatatype       = "RAW";
                bRetVal = DoAddPrinterStuff((LPVOID)&PrinterInfo, 
                                            WOW_AddPrinter);
#ifdef DBG
                if (!bRetVal) {
                    LOGDEBUG(0,("InstallWowFaxPrinter, AddPrinter failed: 0x%X\n", GetLastError()));
                }
#endif
            }
            else {
                LOGDEBUG(0,("InstallWowFaxPrinter, AddPrinterDriver failed: 0x%X\n", GetLastError()));
            }
            free_w(DriverInfo.pDataFile);
            free_w(DriverInfo.pDriverPath);
            free_w(DriverInfo.pConfigFile);

            return(bRetVal);
        }
        else {
           LOGDEBUG(0,("InstallWowFaxPrinter, Unable to create Key: %s\n", szSection));
        }
    }
    else {
        LOGDEBUG(0,("InstallWowFaxPrinter, Unable to open key: HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\WowFax\n"));
    }

    if (hKey) {
        RegCloseKey(hKey);
        if (hSubKey) {
            RegCloseKey(hSubKey);
        }
    }
    return(FALSE);
}

// Come here if szSection=="devices" or if gbWinFaxHack==TRUE
BOOL IsFaxPrinterWriteProfileString(PSZ szSection, PSZ szKey, PSZ szString)
{
    BOOL  Result = FALSE;

    // Don't install if trying to clear an entry.
    if (*szString == '\0') {
        goto Done;
    }

    // Trying to install a fax printer?
    LOGDEBUG(0,("IsFaxPrinterWriteProfileString, Section = devices, Key = %s\n", szKey));

    // Is the WinFax Lite hack enabled?
    if(gbWinFaxHack) {

        // if ("WINFAX", "modem", "xxx") we know the WinFax install program
        // has had a chance to copy "WinFax.drv" to the hard drive.  So
        // now we can call AddPrinter which can callback into WinFax.drv to
        // its hearts content.
        if(!WOW32_strcmp(szSection, szWINFAX) && !WOW32_stricmp(szKey, szModem)) {

            // Our hack has run its course.  We set this before making the call 
            // to AddPrinter because it calls back into WinFax.drv which calls
            // WriteProfileString()!
            gbWinFaxHack = FALSE;

            // Call into the spooler to add our driver to the registry.
            if (!InstallWowFaxPrinter(szDevices, szWINFAX, szWINFAXCOMx)) {
                WOW32ASSERTMSG(FALSE, 
                               "Install of generic fax printer failed.\n");
            }
        }
        Result = TRUE;
        goto Done;
    }

    // Is it one of the fax drivers we recognize?
    if (IsFaxPrinterSupportedDevice(szKey)) {

        // Time to enable the WinFax Lite hack?
        // if("devices", "WINFAX", "WINFAX,COMx:") we need to avoid the call to
        // InstallWOWFaxPrinter() at this time -- the install program hasn't
        // copied the driver to the hard drive yet!!  This causes loadLibrary
        // of WinFax.drv to fail when the spooler tries to callback into it.
        // We also don't want this particular call to WriteProfileString to
        // really be written to the registry -- we let the later call to
        // AddPrinter take care of all the registration stuff.
        if(!WOW32_strcmp(szKey, szWINFAX)        && 
           !WOW32_strncmp(szString, szWINFAX, 6) &&
           (szString[6] == ',')) {

            VPVOID vpPathName;
            PSZ    pszPathName;
            char   szFileName[32];

            // get the install program file name
            // be sure allocation size matches stackfree16() size below
            if(vpPathName = stackalloc16(MAX_PATH)) {
                GetModuleFileName16(CURRENTPTD()->hMod16, vpPathName, MAX_PATH);
                GETVDMPTR(vpPathName, MAX_PATH, pszPathName);
                _splitpath(pszPathName,NULL,NULL,szFileName,NULL);

                // WinFax Lite is "INSTALL", WinFax Pro 4.0 is "SETUP"
                if(!WOW32_stricmp(szINSTALL, szFileName)) {

                    strcpy(szWINFAXCOMx, szString); // save the port string
                    gbWinFaxHack = TRUE;            // enable the hack
                    Result = TRUE;  
                    stackfree16(vpPathName, MAX_PATH);
                    goto Done;     // skip the call to InstallWowFaxPrinter 
                }
                // No hack needed for WinFax Pro 4.0, the driver is copied
                // to the hard disk long before they update win.ini
                else {
                    stackfree16(vpPathName, MAX_PATH);
                }
            }
        }

        if (!InstallWowFaxPrinter(szSection, szKey, szString)) {
            WOW32ASSERTMSG(FALSE, "Install of generic fax printer failed.\n");
        }
        Result = TRUE;
    }

Done:
    return Result;
}




BOOL IsFaxPrinterSupportedDevice(PSZ pszDevice)
{
    UINT  i, iNotFound;

    // Trying to read from a fax printer entry?
    LOGDEBUG(0,("IsFaxPrinterSupportedDevice, Device = %s\n", pszDevice));

    // Is it one of the fax drivers we recognize?
    for (i = 0; i < uNumSupFaxDrv; i++) {
        iNotFound =  WOW32_stricmp(pszDevice, SupFaxDrv[i]);
        if (iNotFound > 0) continue;
        if (iNotFound == 0) {
            LOGDEBUG(0,("IsFaxPrinterSupportedDevice returns TRUE\n"));
            return(TRUE);
        }
        else {
            break;
        }
    }
    return(FALSE);
}

DWORD GetFaxPrinterProfileString(PSZ szSection, PSZ szKey, PSZ szDefault, PSZ szRetBuf, DWORD cbBufSize)
{
    char  szTmp[MAX_PATH];
    HKEY  hKey = 0;
    DWORD dwType;

    // Read the entry from the shadow entries in registry.
    strcpy(szTmp, "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\WowFax\\");
    strcat(szTmp, szSection);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmp, 0, KEY_READ, &hKey ) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, szKey, 0, &dwType, szRetBuf, &cbBufSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return(cbBufSize);
        }
    }

    if (hKey) {
        RegCloseKey(hKey);
    }
    WOW32WARNMSG(FALSE, ("GetFaxPrinterProfileString Failed. Section = %s, Key = %s\n", szSection, szKey));
    strcpy(szRetBuf, szDefault);
    return(strlen(szDefault));
}

