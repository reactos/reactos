/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Kernel
 *  PURPOSE:          GDI Printing Support
 *  FILE:             dll/win32/gdi32/objects/printdrv.c
 *  PROGRAMER:
 *
 */

// For the wine code:
/*
 * Implementation of some printer driver bits
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 Huw Davies
 * Copyright 1998 Andreas Mohr
 * Copyright 1999 Klaas van Gend
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
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
HANDLE ghSpooler = NULL;

static ABORTPRINTER fpAbortPrinter;
CLOSEPRINTER fpClosePrinter;
static CLOSESPOOLFILEHANDLE fpCloseSpoolFileHandle;
static COMMITSPOOLDATA fpCommitSpoolData;
//static fpConnectToLd64In32Server;
static DOCUMENTEVENT fpDocumentEvent;
static DOCUMENTPROPERTIESW fpDocumentPropertiesW;
static ENDDOCPRINTER fpEndDocPrinter;
static ENDPAGEPRINTER fpEndPagePrinter;
static GETSPOOLFILEHANDLE fpGetSpoolFileHandle;
static GETPRINTERW fpGetPrinterW;
static GETPRINTERDRIVERW fpGetPrinterDriverW;
static ISVALIDDEVMODEW fpIsValidDevmodeW;
OPENPRINTERW fpOpenPrinterW;
static QUERYSPOOLMODE fpQuerySpoolMode;
static QUERYREMOTEFONTS fpQueryRemoteFonts;
static QUERYCOLORPROFILE fpQueryColorProfile;
static READPRINTER fpReadPrinter;
static RESETPRINTERW fpResetPrinterW;
static SEEKPRINTER fpSeekPrinter;
static SPLDRIVERUNLOADCOMPLETE fpSplDriverUnloadComplete;
static SPLREADPRINTER fpSplReadPrinter;
static STARTDOCDLGW fpStartDocDlgW;
static STARTDOCPRINTERW fpStartDocPrinterW;
static STARTPAGEPRINTER fpStartPagePrinter;

/* PRIVATE FUNCTIONS *********************************************************/

static
int
FASTCALL
IntEndPage(
    HDC hdc,
    BOOL Form
)
{
    PLDC pldc;
    int Ret = SP_ERROR;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    pldc = GdiGetLDC(hdc);
    if ( !pldc )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    if (pldc->Flags & LDC_ATENDPAGE) return 1;

    if (pldc->Flags & LDC_META_PRINT)
    {
        if ( Form )
        {
            // Do MF EndPageForm
        }
        else
        {
            // Do MF EndPage
        }
        return Ret;
    }

    if (pldc->Flags & LDC_KILL_DOCUMENT || pldc->Flags & LDC_INIT_PAGE)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return SP_ERROR;
    }

    if (pldc->Flags & LDC_SAPCALLBACK) GdiSAPCallback(pldc);

    pldc->Flags &= ~LDC_INIT_PAGE;

    DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_ENDPAGE, 0, NULL, 0, NULL);

    ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

    if ( NtGdiEndPage(hdc) )
    {
        BOOL Good;
//      if (pldc->pUMPDev)
        Good = EndPagePrinterEx(NULL,pldc->hPrinter);

        if (Good) pldc->Flags |= LDC_STARTPAGE;
        Ret = 1;
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);
    return Ret;
}

/* FUNCTIONS *****************************************************************/

BOOL
FASTCALL
AbortPrinterEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter
)
{
    return fpAbortPrinter(hPrinter);
}

int
FASTCALL
DocumentEventEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter,
    HDC     hdc,
    int     iEsc,
    ULONG   cbIn,
    PVOID   pvIn,
    ULONG   cbOut,
    PVOID   pvOut
)
{
    return fpDocumentEvent(hPrinter,hdc,iEsc,cbIn,pvIn,cbOut,pvOut);
}

BOOL
FASTCALL
EndDocPrinterEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter
)
{
    return fpEndDocPrinter(hPrinter);
}

BOOL
FASTCALL
EndPagePrinterEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter
)
{
    return fpEndPagePrinter(hPrinter);
}

BOOL
FASTCALL
LoadTheSpoolerDrv(VOID)
{
    HMODULE hModWinSpoolDrv;

    if ( !ghSpooler )
    {
        RtlEnterCriticalSection(&semLocal);

        hModWinSpoolDrv = LoadLibraryW(L"WINSPOOL.DRV");

        if (hModWinSpoolDrv)
        {
            fpAbortPrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "AbortPrinter");
            fpClosePrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "ClosePrinter");
            fpCloseSpoolFileHandle = (PVOID)GetProcAddress(hModWinSpoolDrv, "CloseSpoolFileHandle");
            fpCommitSpoolData = (PVOID)GetProcAddress(hModWinSpoolDrv, "CommitSpoolData");
            // fpConnectToLd64In32Server = (PVOID)GetProcAddress(hModWinSpoolDrv, (LPCSTR)224);
            fpDocumentEvent = (PVOID)GetProcAddress(hModWinSpoolDrv,"DocumentEvent");
            fpDocumentPropertiesW = (PVOID)GetProcAddress(hModWinSpoolDrv, "DocumentPropertiesW");
            fpEndDocPrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "EndDocPrinter");
            fpEndPagePrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "EndPagePrinter");
            fpGetPrinterW = (PVOID)GetProcAddress( hModWinSpoolDrv,"GetPrinterW");
            fpGetPrinterDriverW = (PVOID)GetProcAddress(hModWinSpoolDrv,"GetPrinterDriverW");
            fpGetSpoolFileHandle = (PVOID)GetProcAddress(hModWinSpoolDrv, "GetSpoolFileHandle");
            fpIsValidDevmodeW = (PVOID)GetProcAddress(hModWinSpoolDrv, "IsValidDevmodeW");
            fpOpenPrinterW = (PVOID)GetProcAddress(hModWinSpoolDrv, "OpenPrinterW");
            fpQueryColorProfile = (PVOID)GetProcAddress(hModWinSpoolDrv,"QueryColorProfile");
            fpQueryRemoteFonts = (PVOID)GetProcAddress(hModWinSpoolDrv, "QueryRemoteFonts");
            fpQuerySpoolMode = (PVOID)GetProcAddress(hModWinSpoolDrv, "QuerySpoolMode");
            fpReadPrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "ReadPrinter");
            fpResetPrinterW = (PVOID)GetProcAddress(hModWinSpoolDrv, "ResetPrinterW");
            fpSeekPrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "SeekPrinter");
            fpSplDriverUnloadComplete = (PVOID)GetProcAddress(hModWinSpoolDrv, "SplDriverUnloadComplete");
            fpSplReadPrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, (LPCSTR)205);
            fpStartDocDlgW = (PVOID)GetProcAddress(hModWinSpoolDrv, "StartDocDlgW");
            fpStartDocPrinterW = (PVOID)GetProcAddress(hModWinSpoolDrv, "StartDocPrinterW");
            fpStartPagePrinter = (PVOID)GetProcAddress(hModWinSpoolDrv, "StartPagePrinter");

            if ( !fpAbortPrinter ||
                    !fpClosePrinter ||
                    !fpCloseSpoolFileHandle ||
                    !fpCommitSpoolData ||
                    !fpDocumentEvent ||
                    !fpDocumentPropertiesW ||
                    !fpEndDocPrinter ||
                    !fpEndPagePrinter ||
                    !fpGetPrinterW ||
                    !fpGetPrinterDriverW ||
                    !fpGetSpoolFileHandle ||
                    !fpIsValidDevmodeW ||
                    !fpOpenPrinterW ||
                    !fpQueryColorProfile ||
                    !fpQueryRemoteFonts ||
                    !fpQuerySpoolMode ||
                    !fpReadPrinter ||
                    !fpResetPrinterW ||
                    !fpSeekPrinter ||
                    !fpSplDriverUnloadComplete ||
                    !fpSplReadPrinter ||
                    !fpStartDocDlgW ||
                    !fpStartDocPrinterW ||
                    !fpStartPagePrinter )
            {
                FreeLibrary(hModWinSpoolDrv);
                hModWinSpoolDrv = NULL;
            }
            ghSpooler = hModWinSpoolDrv;
        }
        RtlLeaveCriticalSection(&semLocal);
    }
    else
        return TRUE;

    if ( !ghSpooler ) SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return (ghSpooler != NULL);
}

/*
  Note from msdn:

   The sequence for a print job is as follows:

   1. To begin a print job, call StartDocPrinter.
   2. To begin each page, call StartPagePrinter.
   3. To write data to a page, call WritePrinter.
   4. To end each page, call EndPagePrinter.
   5. Repeat 2, 3, and 4 for as many pages as necessary.
   6. To end the print job, call EndDocPrinter.

 */
DWORD
FASTCALL
StartDocPrinterWEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    return fpStartDocPrinterW(hPrinter,Level,pDocInfo);
}

BOOL
FASTCALL
StartPagePrinterEx(
    PVOID   pvUMPDev,
    HANDLE  hPrinter
)
{
    return fpStartPagePrinter(hPrinter);
}

/* SYSCALLS ******************************************************************/

/*
 * @unimplemented
 */
int
WINAPI
AbortDoc(
    HDC	hdc
)
{
    PLDC pldc;
    int Ret = SP_ERROR;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    pldc = GdiGetLDC(hdc);
    if ( !pldc )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    if ( !(pldc->Flags & LDC_INIT_DOCUMENT) ) return 1;

    DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_ABORTDOC, 0, NULL, 0, NULL);

    ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

    if ( pldc->Flags & LDC_META_PRINT)
    {
        UNIMPLEMENTED;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return Ret;
    }

    if (NtGdiAbortDoc(hdc))
    {
        if (fpAbortPrinter(pldc->hPrinter)) Ret = 1;
    }
    else
        Ret = SP_ERROR;

    pldc->Flags &= ~(LDC_ATENDPAGE|LDC_META_PRINT|LDC_STARTPAGE|LDC_INIT_PAGE|LDC_INIT_DOCUMENT|LDC_SAPCALLBACK);

    return Ret;
}

/*
 * @unimplemented
 */
int
WINAPI
EndDoc(
    HDC	hdc
)
{
    PLDC pldc;
    int Ret = SP_ERROR;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    pldc = GdiGetLDC(hdc);
    if ( !pldc )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    if (pldc->Flags & LDC_META_PRINT)
    {
        UNIMPLEMENTED;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return Ret;
    }

    if (pldc->Flags & LDC_INIT_DOCUMENT)
    {
        BOOL Good;
        if (pldc->Flags & LDC_INIT_PAGE) EndPage(hdc);

        DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_ENDDOC, 0, NULL, 0, NULL);

        ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

        Good = NtGdiEndDoc(hdc);

//      if (pldc->pUMPDev)
        Good = EndDocPrinterEx(NULL,pldc->hPrinter);

        if (Good)
        {
            DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_ENDDOCPOST, 0, NULL, 0, NULL);
            Ret = 1;
        }
        pldc->Flags &= ~(LDC_ATENDPAGE|LDC_STARTPAGE|LDC_INIT_DOCUMENT|LDC_SAPCALLBACK);
    }
    return Ret;
}

/*
 * @implemented
 */
int
WINAPI
EndFormPage(HDC hdc)
{
    return IntEndPage(hdc,TRUE);
}

/*
 * @implemented
 */
int
WINAPI
EndPage(HDC hdc	)
{
    return IntEndPage(hdc,FALSE);
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiGetSpoolFileHandle(LPWSTR pwszPrinterName,
                      LPDEVMODEW pDevmode,
                      LPWSTR pwszDocName)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiDeleteSpoolFileHandle(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GdiGetPageCount(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
int
WINAPI
StartDocW(
    HDC		hdc,
    CONST DOCINFOW	*lpdi
)
{
    PLDC pldc;
    DOCINFOW diW;
    DOC_INFO_1W di1W;
    LPWSTR lpwstrRet = NULL;
    BOOL Banding;
    int PrnJobNo, Ret = SP_ERROR;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
        return SP_ERROR;

    pldc = GdiGetLDC(hdc);
    if ( !pldc || pldc->Flags & LDC_ATENDPAGE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    if (!pldc->hPrinter) return SP_ERROR;

    pldc->Flags &= ~LDC_KILL_DOCUMENT;

    if (lpdi)
        RtlCopyMemory(&diW, lpdi, sizeof(DOCINFOW));
    else
    {
        diW.cbSize = sizeof(DOCINFOW);
        diW.lpszDocName  = NULL;
        diW.lpszOutput   = NULL;
        diW.lpszDatatype = NULL;
        diW.fwType = 0;
    }

    if (!diW.lpszOutput)
        if (pldc->pwszPort) diW.lpszOutput = pldc->pwszPort;

    lpwstrRet = fpStartDocDlgW(pldc->hPrinter, &diW);
    if (lpwstrRet == (LPWSTR)SP_APPABORT)
    {
        pldc->Flags |= LDC_KILL_DOCUMENT;
        return SP_ERROR;
    }
    if (lpwstrRet == (LPWSTR)SP_ERROR) return SP_ERROR;

    if (lpwstrRet != 0) diW.lpszOutput = lpwstrRet;

    Ret = DocumentEventEx( NULL,
                           pldc->hPrinter,
                           hdc,
                           DOCUMENTEVENT_STARTDOC,
                           sizeof(ULONG),
                           &diW,
                           0,
                           NULL);

    if (Ret == SP_APPABORT)
    {
        pldc->Flags |= LDC_KILL_DOCUMENT;
        Ret = SP_ERROR;
    }
    if (Ret == SP_ERROR)
    {
        if (lpwstrRet) LocalFree(lpwstrRet);
        return Ret;
    }

    di1W.pDocName    = (LPWSTR)diW.lpszDocName;
    di1W.pOutputFile = (LPWSTR)diW.lpszOutput;
    di1W.pDatatype   = (LPWSTR)diW.lpszDatatype;

    Ret = SP_ERROR;

    PrnJobNo = StartDocPrinterWEx(NULL, pldc->hPrinter, 1, (LPBYTE)&di1W);
    if (PrnJobNo <= 0)
    {
        Ret = NtGdiStartDoc( hdc, &diW, &Banding, PrnJobNo);
        if (Ret)
        {
            if (pldc->pAbortProc)
            {
                GdiSAPCallback(pldc);
                pldc->Flags |= LDC_SAPCALLBACK;
                pldc->CallBackTick = GetTickCount();
            }
            pldc->Flags |= LDC_INIT_DOCUMENT;
            if (!Banding) pldc->Flags |= LDC_STARTPAGE;
        }
    }
    if (Ret == SP_ERROR)
    {
        //if ( pldc->pUMPDev  )
        AbortPrinterEx(NULL, pldc->hPrinter);
        DPRINT1("StartDoc Died!!!\n");
    }
    else
    {
        if ( DocumentEventEx( NULL,
                              pldc->hPrinter,
                              hdc,
                              DOCUMENTEVENT_STARTDOCPOST,
                              sizeof(ULONG),
                              &Ret,
                              0,
                              NULL) == SP_ERROR)
        {
            AbortDoc(hdc);
            Ret = SP_ERROR;
        }
    }
    if (lpwstrRet) LocalFree(lpwstrRet);
    return Ret;
}

/*
 * @implemented
 */
int
WINAPI
StartDocA(
    HDC		hdc,
    CONST DOCINFOA	*lpdi
)
{
    LPWSTR szDocName = NULL, szOutput = NULL, szDatatype = NULL;
    DOCINFOW docW;
    INT ret, len;

    docW.cbSize = lpdi->cbSize;
    if (lpdi->lpszDocName)
    {
        len = MultiByteToWideChar(CP_ACP,0,lpdi->lpszDocName,-1,NULL,0);
        szDocName = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpdi->lpszDocName,-1,szDocName,len);
    }
    if (lpdi->lpszOutput)
    {
        len = MultiByteToWideChar(CP_ACP,0,lpdi->lpszOutput,-1,NULL,0);
        szOutput = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpdi->lpszOutput,-1,szOutput,len);
    }
    if (lpdi->lpszDatatype)
    {
        len = MultiByteToWideChar(CP_ACP,0,lpdi->lpszDatatype,-1,NULL,0);
        szDatatype = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpdi->lpszDatatype,-1,szDatatype,len);
    }

    docW.lpszDocName = szDocName;
    docW.lpszOutput = szOutput;
    docW.lpszDatatype = szDatatype;
    docW.fwType = lpdi->fwType;

    ret = StartDocW(hdc, &docW);

    HeapFree( GetProcessHeap(), 0, szDocName );
    HeapFree( GetProcessHeap(), 0, szOutput );
    HeapFree( GetProcessHeap(), 0, szDatatype );

    return ret;
}

/*
 * @unimplemented
 */
int
WINAPI
StartPage(
    HDC	hdc
)
{
    PLDC pldc;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    pldc = GdiGetLDC(hdc);
    if ( !pldc )
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return SP_ERROR;
    }

    if (pldc->Flags & LDC_META_PRINT)
    {
        UNIMPLEMENTED;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return SP_ERROR;
    }

    pldc->Flags &= ~(LDC_ATENDPAGE|LDC_STARTPAGE);

    if (pldc->Flags & LDC_INIT_PAGE) return 1;

    if (DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_STARTPAGE, 0, NULL, 0, NULL) != SP_ERROR)
    {
        pldc->Flags |= LDC_INIT_PAGE;

        ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

        if (StartPagePrinterEx(NULL, pldc->hPrinter))
        {
            if (NtGdiStartPage(hdc)) return 1;
        }

        pldc->Flags &= ~(LDC_INIT_PAGE);
        EndDoc(hdc);
        SetLastError(ERROR_INVALID_HANDLE);
    }
    return SP_ERROR;
}

/*
 * @implemented
 */
int
WINAPI
StartFormPage(HDC hdc)
{
    return StartPage(hdc);
}


/*
 * @implemented
 */
int
WINAPI
SetAbortProc(
    HDC hdc,
    ABORTPROC lpAbortProc)
{
    PLDC pldc;
    ULONG hType = GDI_HANDLE_GET_TYPE(hdc);

    if (hType == GDILoObjType_LO_DC_TYPE || hType == GDILoObjType_LO_METADC16_TYPE)
        return SP_ERROR;

    pldc = GdiGetLDC(hdc);
    if ( pldc )
    {
        if ( lpAbortProc )
        {
            if ( pldc->Flags & LDC_INIT_DOCUMENT )
            {
                pldc->Flags |= LDC_SAPCALLBACK;
                pldc->CallBackTick = GetTickCount();
            }
        }
        else
        {
            pldc->Flags &= ~LDC_SAPCALLBACK;
        }
        pldc->pAbortProc = lpAbortProc;
        return 1;
    }
    else
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }
    return SP_ERROR;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
gdiPlaySpoolStream(
    DWORD	a0,
    DWORD	a1,
    DWORD	a2,
    DWORD	a3,
    DWORD	a4,
    DWORD	a5
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HDC
WINAPI
GdiGetDC(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiGetPageHandle(HANDLE SpoolFileHandle,
                 DWORD Page,
                 LPDWORD pdwPageType)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiStartDocEMF(HANDLE SpoolFileHandle,
               DOCINFOW *pDocInfo)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiStartPageEMF(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayPageEMF(HANDLE SpoolFileHandle,
               HANDLE hemf,
               RECT *prectDocument,
               RECT *prectBorder,
               RECT *prectClip)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiEndPageEMF(HANDLE SpoolFileHandle,
              DWORD dwOptimization)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiEndDocEMF(HANDLE SpoolFileHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiGetDevmodeForPage(HANDLE SpoolFileHandle,
                     DWORD dwPageNumber,
                     PDEVMODEW *pCurrDM,
                     PDEVMODEW *pLastDM)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiResetDCEMF(HANDLE SpoolFileHandle,
              PDEVMODEW pCurrDM)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayEMF(LPWSTR pwszPrinterName,
           LPDEVMODEW pDevmode,
           LPWSTR pwszDocName,
           EMFPLAYPROC pfnEMFPlayFn,
           HANDLE hPageQuery
          )
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPlayPrivatePageEMF(HANDLE SpoolFileHandle,
                      DWORD unknown,
                      RECT *prectDocument)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiPrinterThunk(
    IN HUMPD humpd,
    DWORD *status,
    DWORD unuse)
{
    /* FIXME figout the protypes, the HUMPD are a STRUCT or COM object */
    /* status contain some form of return value that being save, what it is I do not known */
    /* unsue seam have zero effect, what it is for I do not known */

    // ? return NtGdiSetPUMPDOBJ(humpd->0x10,TRUE, humpd, ?) <- blackbox, OpenRCE info, and api hooks for anylaysing;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiArtificialDecrementDriver(LPWSTR pDriverName,BOOL unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
