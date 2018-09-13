/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSPOOL.C
 *  WOW32 printer spooler support routines
 *
 *  These routines help a Win 3.0 task to use the print spooler apis. These
 *  apis were exposed by DDK in Win 3.1.
 *
 *  History:
 *  Created 1-July-1993 by Chandan Chauhan (ChandanC)
 *
--*/


#include "precomp.h"
#pragma hdrstop
#include <winspool.h>

extern WORD gUser16hInstance;

VOID WOWSpoolerThread(WOWSPOOL *lpwowSpool);

WORD gprn16 = 0x100;  // Global spooler job # (can be anything > 0)

MODNAME(wspool.c);

LPDEVMODE GetDefaultDevMode32(LPSTR szDriver)
{
    LONG        cbDevMode;
    LPDEVMODE   lpDevMode = NULL;

    if (szDriver != NULL) {

        if (!(*spoolerapis[WOW_EXTDEVICEMODE].lpfn)) {
            if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
                goto LeaveGetDefaultDevMode32;
            }
        }

        if ((cbDevMode = (*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(NULL, NULL, NULL, szDriver, NULL, NULL, NULL, 0)) > 0) {
            if ((lpDevMode = (LPDEVMODE) malloc_w(cbDevMode)) != NULL) {
                if ((*spoolerapis[WOW_EXTDEVICEMODE].lpfn)(NULL, NULL, lpDevMode, szDriver, NULL, NULL, NULL, DM_COPY) != IDOK) {
                    free_w(lpDevMode);
                    lpDevMode = NULL;
                }
            }
        }

LeaveGetDefaultDevMode32:

        if (!lpDevMode) {
                LOGDEBUG(0,("WOW::GetDefaultDevMode32: Unable to get default DevMode\n"));
        }
    }

    return(lpDevMode);
}

ULONG FASTCALL   WG32OpenJob (PVDMFRAME pFrame)
{
    INT         len;
    PSZ         psz1      = NULL;
    PSZ         psz2      = NULL;
    PSZ         pszDriver = NULL;
    ULONG       ul=0;
    DOC_INFO_1  DocInfo1;
    HANDLE      hnd;
    register    POPENJOB16 parg16;
    PRINTER_DEFAULTS  PrinterDefault;
    PPRINTER_DEFAULTS pPrinterDefault = NULL;

    GETARGPTR(pFrame, sizeof(OPENJOB16), parg16);

    // save off the 16-bit params now since this could callback into a 16-bit
    // fax driver & cause 16-bit memory to move.
    if(parg16->f1) {
        if(psz1 = malloc_w_strcpy_vp16to32(parg16->f1, FALSE, 0)) {
            len = strlen(psz1);
            pszDriver = malloc_w(max(len, 40));
        }
    }

    if(parg16->f2) {
        psz2 = malloc_w_strcpy_vp16to32(parg16->f2, FALSE, 0);
    }

    FREEARGPTR(parg16);
    // all 16-bit pointers are now invalid!!

    // this implies that psz1 may also be bad
    if(!pszDriver) {
        goto exitpath;
    }


    if (!(*spoolerapis[WOW_OpenPrinterA].lpfn)) {
        if (!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            goto exitpath;
        }
    }

    if (GetDriverName(psz1, pszDriver)) {
        if((PrinterDefault.pDevMode = GetDefaultDevMode32(pszDriver)) != NULL) {
            PrinterDefault.pDatatype = NULL;
            PrinterDefault.DesiredAccess  = 0;
            pPrinterDefault = &PrinterDefault;

            if ((*spoolerapis[WOW_OpenPrinterA].lpfn) (pszDriver, 
                                                       &hnd, 
                                                       pPrinterDefault)) {

                DocInfo1.pDocName = psz2;
                DocInfo1.pOutputFile = psz1;
                DocInfo1.pDatatype = NULL;

                if (ul = (*spoolerapis[WOW_StartDocPrinterA].lpfn) (hnd, 1, (LPBYTE)&DocInfo1)) {
                    ul = GetPrn16(hnd);
                }
                else {
                    ul = GetLastError();
                }

            }
            else {
                ul = GetLastError();
            }
        }
    }

    LOGDEBUG(0,("WOW::WG32OpenJob: ul = %x\n", ul));

    if (pPrinterDefault) {
        free_w(PrinterDefault.pDevMode);
    }

exitpath:

    if(psz1) {
        free_w(psz1);
    }
    if(psz2) {
        free_w(psz2);
    }
    if(pszDriver) {
        free_w(pszDriver);
    }

    RETURN(ul);
}


ULONG FASTCALL   WG32StartSpoolPage (PVDMFRAME pFrame)
{
    ULONG       ul=0;
    register    PSTARTSPOOLPAGE16 parg16;

    GETARGPTR(pFrame, sizeof(STARTSPOOLPAGE16), parg16);

    if (!(ul = (*spoolerapis[WOW_StartPagePrinter].lpfn) (Prn32(parg16->f1)))) {
        ul = GetLastError();
    }

    LOGDEBUG(0,("WOW::WG32StartSpoolPage: ul = %x\n", ul));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL   WG32EndSpoolPage (PVDMFRAME pFrame)
{
    ULONG       ul=0;
    register    PENDSPOOLPAGE16 parg16;

    GETARGPTR(pFrame, sizeof(ENDSPOOLPAGE16), parg16);

    if (!(ul = (*spoolerapis[WOW_EndPagePrinter].lpfn) (Prn32(parg16->f1)))) {
        ul = GetLastError();
    }

    LOGDEBUG(0,("WOW::WG32EndSpoolPage: ul = %x\n", ul));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL   WG32CloseJob (PVDMFRAME pFrame)
{
    ULONG       ul=0;
    register    PCLOSEJOB16 parg16;

    GETARGPTR(pFrame, sizeof(CLOSEJOB16), parg16);

    if (!(ul = (*spoolerapis[WOW_EndDocPrinter].lpfn) (Prn32(parg16->f1)))) {

        ul = GetLastError();
    }

    if (!(ul = (*spoolerapis[WOW_ClosePrinter].lpfn) (Prn32(parg16->f1)))) {
        ul = GetLastError();
    }

    if (ul) {
        FreePrn(parg16->f1);
    }

    LOGDEBUG(0,("WOW::WG32CloseJob: ul = %x\n", ul));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL   WG32WriteSpool (PVDMFRAME pFrame)
{
    DWORD       dwWritten;
    ULONG       ul=0;
    register    PWRITESPOOL16 parg16;
    LPVOID      pBuf;

    GETARGPTR(pFrame, sizeof(WRITESPOOL16), parg16);
    GETMISCPTR (parg16->f2, pBuf);

    if (ul = (*spoolerapis[WOW_WritePrinter].lpfn) (Prn32(parg16->f1), pBuf,
                             FETCHWORD(parg16->f3), &dwWritten)) {
        ul = FETCHWORD(parg16->f3);
    }
    else {
        ul = GetLastError();
    }

    LOGDEBUG(0,("WOW::WG32WriteSpool: ul = %x\n", ul));

    FREEMISCPTR(pBuf);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL   WG32DeleteJob (PVDMFRAME pFrame)
{
    ULONG       ul = 0;
    register    PDELETEJOB16 parg16;

    GETARGPTR(pFrame, sizeof(DELETEJOB16), parg16);

    if (!(ul = (*spoolerapis[WOW_DeletePrinter].lpfn) (Prn32(parg16->f1)))) {
        ul = GetLastError();
    }

    LOGDEBUG(0,("WOW::WG32DeleteJob: ul = %x\n", ul));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WG32SpoolFile (PVDMFRAME pFrame)
{
    INT         len;
    PSZ         psz2      = NULL;
    PSZ         psz3      = NULL;
    PSZ         psz4      = NULL;
    PSZ         pszDriver = NULL;
    LONG        ul        = -1;   // SP_ERROR
    HANDLE      hFile     = NULL;
    HANDLE      hPrinter  = NULL;
    HANDLE      hThread   = NULL;
    WOWSPOOL    *lpwowSpool = NULL;
    DOC_INFO_1  DocInfo1;
    DWORD       dwUnused;
    register    PSPOOLFILE16 parg16;


    GETARGPTR(pFrame, sizeof(SPOOLFILE16), parg16);

    // save off the 16-bit params now since this could callback into a 16-bit
    // fax driver & cause 16-bit memory to move.

    // ignore psz1 (printer name)

    // get the port name and the associated driver name
    if(parg16->f2) {
        if(!(psz2 = malloc_w_strcpy_vp16to32(parg16->f2, FALSE, 0))) {
            goto exitpath;
        }
        len = strlen(psz2);
        if(!(pszDriver = malloc_w(max(len, 40)))) {
            goto exitpath;
        }
        if(!GetDriverName(psz2, pszDriver)) {
            goto exitpath;
        }
    }

    // get the Job Title
    if(parg16->f3) {
        if(!(psz3 = malloc_w_strcpy_vp16to32(parg16->f3, FALSE, 0))) {
            goto exitpath;
        }
    }

    // get the file name
    if(parg16->f4) {
        if(!(psz4 = malloc_w_strcpy_vp16to32(parg16->f4, FALSE, 0))) {
            goto exitpath;
        }
    }

    FREEARGPTR(parg16);
    // all 16-bit pointers are now invalid!!

    // all fields of this struct are initially zero
    if(!(lpwowSpool = (WOWSPOOL *)malloc_w_zero(sizeof(WOWSPOOL)))) {
        goto exitpath;
    }

    if(!(*spoolerapis[WOW_OpenPrinterA].lpfn)) {
        if(!LoadLibraryAndGetProcAddresses("WINSPOOL.DRV", spoolerapis, WOW_SPOOLERAPI_COUNT)) {
            goto exitpath;
        }
    }

    // open the specified file
    if((hFile = CreateFile(psz4, 
                           GENERIC_READ, 
                           0, 
                           NULL, 
                           OPEN_EXISTING,
                           FILE_FLAG_SEQUENTIAL_SCAN,
                           NULL)) == INVALID_HANDLE_VALUE) {

        goto exitpath;
    }

    // create the WOWSpoolerThread to handle the "spooling"
    if(!(hThread = CreateThread(NULL,
                                16384,
                                (LPTHREAD_START_ROUTINE)WOWSpoolerThread,
                                lpwowSpool,
                                CREATE_SUSPENDED,
                                (LPDWORD)&dwUnused))) {
        goto exitpath;
    }
    
    // open the printer
    if((*spoolerapis[WOW_OpenPrinterA].lpfn)(pszDriver, &hPrinter, NULL)) {

        DocInfo1.pDocName    = psz3;
        DocInfo1.pOutputFile = NULL;
        DocInfo1.pDatatype   = "RAW";

        // start a doc
        if(!(*spoolerapis[WOW_StartDocPrinterA].lpfn)(hPrinter, 
                                                      1, 
                                                      (LPBYTE)&DocInfo1)) {
            goto ClosePrinter;
        }

        // start a page
        if((*spoolerapis[WOW_StartPagePrinter].lpfn)(hPrinter)) {

            // tell the WOWSpoolerThread that it's OK to do its thing
            lpwowSpool->fOK      = TRUE;
            lpwowSpool->hFile    = hFile;
            lpwowSpool->hPrinter = hPrinter;
            lpwowSpool->prn16    = gprn16;

            // tell the app that everything is hunky dory
            ul = (LONG)gprn16++;

            // make sure this doesn't go negative (-> an error ret to the app)
            if(gprn16 & 0x8000) {
                gprn16 = 0x100;
            }
        } 

        // error path
        else {

            (*spoolerapis[WOW_EndDocPrinter].lpfn)  (hPrinter);
ClosePrinter:
            // note: hPrinter is freed by WOW_ClosePrinter
            (*spoolerapis[WOW_ClosePrinter].lpfn)   (hPrinter);
        }
    }

exitpath:

    LOGDEBUG(2,("WOW::WG32SpoolFile: ul = %x\n", ul));

    if(psz2) {
        free_w(psz2);
    }
    if(psz3) {
        free_w(psz3);
    }
    if(psz4) {
        free_w(psz4);
    }
    if(pszDriver) {
        free_w(pszDriver);
    }

    // give the spooler thread a kick start then close the thread handle
    // (note: the thread will still be active)
    if(hThread) {
        ResumeThread(hThread);
        CloseHandle(hThread);
    }

    // clean up if there was an error -- otherwise the thread will clean up
    if(ul == -1) {
        if(hFile) {
            CloseHandle(hFile);
        }
        if(lpwowSpool) {
            free_w(lpwowSpool);
        }
        // note: hPrinter is freed by WOW_ClosePrinter
    }

    return((ULONG)ul);
}





#define  SPOOL_BUFF_SIZE   4096

VOID WOWSpoolerThread(WOWSPOOL *lpwowSpool)
{
    DWORD  dwBytes; 
    DWORD  dwWritten;
    LPBYTE buf[SPOOL_BUFF_SIZE];


    // this thread will only do something if fOK is TRUE
    if(lpwowSpool->fOK) {
      do {

        // this is a sequential read
        if(ReadFile(lpwowSpool->hFile, buf, SPOOL_BUFF_SIZE, &dwBytes, NULL)) { 

            // if dwBytes==0 --> EOF
            if(dwBytes) {

                // 
                if(!(*spoolerapis[WOW_WritePrinter].lpfn)(lpwowSpool->hPrinter, 
                                                          buf, 
                                                          dwBytes, 
                                                          &dwWritten)) {
                    LOGDEBUG(0,("WOW::WOWSpoolerThread:WritePrinter ERROR!\n"));
                    break;
                }
                else if(dwBytes != dwWritten) {
                    LOGDEBUG(0,("WOW::WOWSpoolerThread:WritePrinter error!\n"));
                    break;
                }
                
            }
        }

      } while (dwBytes == SPOOL_BUFF_SIZE);

      // shut down the print job
      (*spoolerapis[WOW_EndPagePrinter].lpfn) (lpwowSpool->hPrinter);
      (*spoolerapis[WOW_EndDocPrinter].lpfn)  (lpwowSpool->hPrinter);
      // note: hPrinter is freed by WOW_ClosePrinter
      (*spoolerapis[WOW_ClosePrinter].lpfn)   (lpwowSpool->hPrinter);

      // clean up
      if(lpwowSpool->hFile) {
          CloseHandle(lpwowSpool->hFile);
      }
      if(lpwowSpool) {
          free_w(lpwowSpool);
      }

    } // end if

    ExitThread(0);
}





WORD GetPrn16(HANDLE h32)
{
    HANDLE  hnd;
    VPVOID  vp;
    LPBYTE  lpMem16;

    hnd = LocalAlloc16(LMEM_MOVEABLE, sizeof(HANDLE), (HANDLE) gUser16hInstance);

    vp = LocalLock16(hnd);

    if (vp) {
        GETMISCPTR (vp, lpMem16);
        if (lpMem16) {
            *((PDWORD16)lpMem16) = (DWORD) h32;
            FREEMISCPTR(lpMem16);
            LocalUnlock16(hnd);
        }
    }
    else {
        LOGDEBUG (0, ("WOW::GETPRN16: Can't allocate a 16 bit handle\n"));
    }

    return (LOWORD(hnd));
}


HANDLE Prn32(WORD h16)
{
    VPVOID  vp;
    HANDLE  h32 = NULL;
    LPBYTE  lpMem16;

    vp = LocalLock16 ((HANDLE) MAKELONG(h16, gUser16hInstance));
    if (vp) {
        GETMISCPTR (vp, lpMem16);

        if (lpMem16) {
            h32 = (HANDLE) *((PDWORD16)lpMem16);
            FREEMISCPTR(lpMem16);
        }
        LocalUnlock16 ((HANDLE) MAKELONG(h16, gUser16hInstance));
    }

    return (h32);
}


VOID FreePrn (WORD h16)
{
    LocalFree16 ((HANDLE) MAKELONG(h16, gUser16hInstance));
}


BOOL GetDriverName (char *psz, char *pszDriver)
{
    CHAR szAllDevices[1024];
    CHAR *szNextDevice;
    CHAR szPrinter[64];
    CHAR *szOutput;
    UINT len;

    if(!psz || (*psz == '\0')) {
        return FALSE;
    }
  
    len = strlen(psz);

    GetProfileString ("devices", NULL, "", szAllDevices, sizeof(szAllDevices));
    szNextDevice = szAllDevices;

    LOGDEBUG(6,("WOW::GetDriverName: szAllDevices = %s\n", szAllDevices));

    // strings from win.ini will be of the form "PS Printer=PSCRIPT,LPT1:"
    while (*szNextDevice) {
        GetProfileString ("devices", szNextDevice, "", szPrinter, sizeof(szPrinter));
        if (*szPrinter) {
            if (szOutput = WOW32_strchr (szPrinter, ',')) {
                szOutput++;
                while (*szOutput == ' ') {
                    szOutput++;
                }

                if (!WOW32_stricmp(psz, szOutput)) {
                    break;  // found it!
                }

                // some apps pass "LPT1" without the ':' -- account for that
                // if the app passed "LPT1" and ...
                if (psz[len-1] != ':') {

                    // ...strlen(szOutput) == 5 && szOutput[4] == ':' ...
                    if((strlen(szOutput) == len+1) && (szOutput[len] == ':')) {

                        // ...clobber the ':' char ...
                        szOutput[len] = '\0';

                        // ...and see if the strings match now
                        if (!WOW32_stricmp(psz, szOutput)) {
                            break;  // found it!
                        }
                    }
                }
            }
        }

        if (szNextDevice = WOW32_strchr (szNextDevice, '\0')) {
            szNextDevice++;
        }
        else {
            szNextDevice = "";
            break;
        }
    }

    if (*szNextDevice) {
        LOGDEBUG(0,("WOW::GetDriverName: szNextDevice = %s\n", szNextDevice));

        if (lstrcpy (pszDriver, szNextDevice)) {
            return TRUE;
        }
    }

    // else they may have specified a network printer eg. "\\msprint44\corpk"
    // in which case we'll assume it's all right (since it will fail once the
    // WOW functions that call into this will fail when they call into the 
    // driver with a bogus driver name)
    if(psz[0] == '\\' && psz[1] == '\\') {
        strcpy(pszDriver, psz);
        return TRUE;
    }

    return FALSE;
}
