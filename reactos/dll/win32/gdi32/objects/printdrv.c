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

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
HANDLE ghSpooler = NULL;

static ABORTPRINTER fpAbortPrinter;
static CLOSEPRINTER fpClosePrinter;
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
static OPENPRINTERW fpOpenPrinterW;
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

   return Ret;
}

/* FUNCTIONS *****************************************************************/

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
        fpAbortPrinter = GetProcAddress(hModWinSpoolDrv, "AbortPrinter");
        fpClosePrinter = GetProcAddress(hModWinSpoolDrv, "ClosePrinter");
        fpCloseSpoolFileHandle = GetProcAddress(hModWinSpoolDrv, "CloseSpoolFileHandle");
        fpCommitSpoolData = (PVOID)GetProcAddress(hModWinSpoolDrv, "CommitSpoolData");
       // fpConnectToLd64In32Server = GetProcAddress(hModWinSpoolDrv, (LPCSTR)224);
        fpDocumentEvent = (PVOID)GetProcAddress(hModWinSpoolDrv,"DocumentEvent");
        fpDocumentPropertiesW = (PVOID)GetProcAddress(hModWinSpoolDrv, "DocumentPropertiesW");
        fpEndDocPrinter = GetProcAddress(hModWinSpoolDrv, "EndDocPrinter");
        fpEndPagePrinter = GetProcAddress(hModWinSpoolDrv, "EndPagePrinter");
        fpGetPrinterW = GetProcAddress( hModWinSpoolDrv,"GetPrinterW");
        fpGetPrinterDriverW = GetProcAddress(hModWinSpoolDrv,"GetPrinterDriverW");
        fpGetSpoolFileHandle = (PVOID)GetProcAddress(hModWinSpoolDrv, "GetSpoolFileHandle");
        fpIsValidDevmodeW = GetProcAddress(hModWinSpoolDrv, "IsValidDevmodeW");
        fpOpenPrinterW = GetProcAddress(hModWinSpoolDrv, "OpenPrinterW");
        fpQueryColorProfile = GetProcAddress(hModWinSpoolDrv,"QueryColorProfile");
        fpQueryRemoteFonts = (PVOID)GetProcAddress(hModWinSpoolDrv, "QueryRemoteFonts");
        fpQuerySpoolMode = (PVOID)GetProcAddress(hModWinSpoolDrv, "QuerySpoolMode");
        fpReadPrinter = GetProcAddress(hModWinSpoolDrv, "ReadPrinter");
        fpResetPrinterW = GetProcAddress(hModWinSpoolDrv, "ResetPrinterW");
        fpSeekPrinter = GetProcAddress(hModWinSpoolDrv, "SeekPrinter");
        fpSplDriverUnloadComplete = GetProcAddress(hModWinSpoolDrv, "SplDriverUnloadComplete");
        fpSplReadPrinter = GetProcAddress(hModWinSpoolDrv, (LPCSTR)205);
        fpStartDocDlgW = GetProcAddress(hModWinSpoolDrv, "StartDocDlgW");
        fpStartDocPrinterW = (PVOID)GetProcAddress(hModWinSpoolDrv, "StartDocPrinterW");
        fpStartPagePrinter = GetProcAddress(hModWinSpoolDrv, "StartPagePrinter");

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
      if (pldc->Flags & LDC_ENDPAGE_MFDC) EndPage(hdc);

      DocumentEventEx(NULL, pldc->hPrinter, hdc, DOCUMENTEVENT_ENDDOC, 0, NULL, 0, NULL);
   
      ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;

      Good = NtGdiEndDoc(hdc);

      if (Good)
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
EndPage(
	HDC	hdc
	)
{
   return IntEndPage(hdc,FALSE);
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

   return NtGdiStartDoc ( hdc, (DOCINFOW *)lpdi, NULL, 0);
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

   pldc->Flags &= ~(LDC_STARTPAGE);

   if (pldc->Flags & LDC_ENDPAGE_MFDC) return 1;

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

