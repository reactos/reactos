/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    prnsetup.c

Abstract:

    This module implements the Win32 print dialogs.

Revision History:

--*/



//
//  Include Files.
//

#undef WINVER
#define WINVER 0x0500       // Needed to get the updated DMPAPER constants.

#include "comdlg32.h"
#include <winspool.h>
#include <commctrl.h>
#include "prnsetup.h"




//
//  Extern Declarations.
//
#ifdef UNICODE
extern UINT ExtDeviceMode(HWND, HMODULE, LPDEVMODEA, LPSTR, LPSTR, LPDEVMODEA, LPSTR, UINT);
#endif

//
//  The PrintDlgEx routines.
//
extern VOID Print_UnloadLibraries();
extern BOOL Print_NewPrintDlg(PPRINTINFO pPI);





#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgA
//
//  ANSI entry point for PrintDlg when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PrintDlgA(
    LPPRINTDLGA pPDA)
{
    PRINTINFO PI;
    BOOL bResult = FALSE;
    DWORD Flags;


    ZeroMemory(&PI, sizeof(PRINTINFO));

    if (bResult = ThunkPrintDlg(&PI, pPDA))
    {
        ThunkPrintDlgA2W(&PI);

        Flags = pPDA->Flags;

        bResult = PrintDlgX(&PI);

        if ((bResult) || (Flags & (PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK)))
        {
            ThunkPrintDlgW2A(&PI);
        }
    }
    FreeThunkPrintDlg(&PI);

    return (bResult);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgW
//
//  Stub UNICODE function for PrintDlg when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PrintDlgW(
    LPPRINTDLGW pPDW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  PrintDlg
//
//  The PrintDlg function displays a Print dialog box or a Print Setup
//  dialog box.  The Print dialog box enables the user to specify the
//  properties of a particular print job.  The Print Setup dialog box
//  allows the user to select additional job properties and to configure
//  the printer.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PrintDlg(
    LPPRINTDLG pPD)
{
    PRINTINFO PI;

    ZeroMemory(&PI, sizeof(PRINTINFO));

    PI.pPD = pPD;
    PI.ApiType = COMDLG_WIDE;

    return ( PrintDlgX(&PI) );
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgX
//
//  Worker routine for the PrintDlg api.
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintDlgX(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    BOOL nResult = -1;                      // <0==error, 0==CANCEL, >0==OK
    LPDEVMODE pDM = NULL;
    LPDEVMODE pDevMode = NULL;
    LPDEVNAMES pDN = NULL;
    DWORD dwFlags;                          // old copy
    WORD nCopies, nFromPage, nToPage;       // old copy
    HGLOBAL hDevNames, hDevMode;            // old copy
    TCHAR szPrinterName[MAX_PRINTERNAME];   // old copy
    LONG cbNeeded;
#ifndef WINNT
    LPCTSTR pDN_Device = NULL;
    TCHAR szDev[2];
#endif
    DWORD dwResult = 0;


    if (!pPD)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pPD->lStructSize != sizeof(PRINTDLG))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (pPD->hwndOwner && !IsWindow(pPD->hwndOwner))
    {
        StoreExtendedError(CDERR_DIALOGFAILURE);
        return (FALSE);
    }

#ifdef WINNT
    //
    //  See if the application should get the new look.
    //
    //  Do not allow the new look if they have hooks, templates, 
    //  Invalid hwndOwner,
    //  they want the setup dialog, or they just want to get the default
    //  printer.
    //
    //  Also don't allow the new look if we are in the context of
    //  a 16 bit process.
    //
    if ( (!(pPI->Status & PI_PRINTDLGX_RECURSE)) &&
         (!pPI->pPSD) &&
         ((!(pPD->Flags & (PD_PAGESETUP |
                           PD_PRINTSETUP |
                           PD_RETURNDEFAULT |
                           PD_ENABLEPRINTHOOK |
                           PD_ENABLESETUPHOOK |
                           PD_ENABLEPRINTTEMPLATE |
                           PD_ENABLESETUPTEMPLATE |
                           PD_ENABLEPRINTTEMPLATEHANDLE |
                           PD_ENABLESETUPTEMPLATEHANDLE)))) &&
         (pPD->hwndOwner && IsWindow(pPD->hwndOwner)) &&
         (!IS16BITWOWAPP(pPD)) )
    {
        //
        //  Show the new dialog.
        //
        StoreExtendedError(0);

        return Print_NewPrintDlg(pPI);         
    }
#endif

    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (LPVOID) GetDialogLanguage(pPD->hwndOwner, NULL));

    //
    //  Get the process version of the app for later use.
    //
    pPI->ProcessVersion = GetProcessVersion(0);

#ifdef UNICODE
    //
    //  Check if we need to use ExtDeviceMode.  We use this
    //  mode only if a 16 bit app is calling us with a NULL
    //  devmode.
    //
    if ((pPD->Flags & CD_WOWAPP) && !pPD->hDevMode)
    {
        pPI->bUseExtDeviceMode = TRUE;
    }
    else
    {
        pPI->bUseExtDeviceMode = FALSE;
    }
#endif

#ifndef WINNT
    //
    //  This should NOT be in NT.  The device name cannot be longer than
    //  32 characters, so it may be truncated.  If we look for it in the
    //  registry and it's supposed to be larger than 32 characters, then
    //  we won't find it and we'll fail.
    //
    //  LATER: It probably shouldn't be in WIN95 either, but I'll leave
    //         it for now.
    //
    if (pPD->hDevMode)
    {
        if (!(pDM = GlobalLock(pPD->hDevMode)))
        {
            StoreExtendedError(CDERR_MEMLOCKFAILURE);
            goto PrintDlgX_DisplayWarning;
        }
    }

    if (pPD->hDevNames)
    {
        if (!(pDN = GlobalLock(pPD->hDevNames)))
        {
            if (pDM)
            {
                GlobalUnlock(pPD->hDevMode);
            }
            StoreExtendedError(CDERR_MEMLOCKFAILURE);
            goto PrintDlgX_DisplayWarning;
        }
        else
        {
            if (pDN->wDeviceOffset)
            {
                pDN_Device = (LPCTSTR)pDN + pDN->wDeviceOffset;
            }
        }
    }

    if (pDM && pDM->dmDeviceName[0])
    {
        pDM->dmDeviceName[CCHDEVICENAME - 1] = 0;
        GetProfileString(szTextDevices, pDM->dmDeviceName, szTextNull, szDev, 2);
        if (!szDev[0])
        {
            GlobalUnlock(pPD->hDevMode);
            if (pDN)
            {
                GlobalUnlock(pPD->hDevNames);
            }
            StoreExtendedError(PDERR_PRINTERNOTFOUND);
            goto PrintDlgX_DisplayWarning;
        }
    }

    if (pDN_Device && pDN_Device[0])
    {
        GetProfileString(szTextDevices, pDN_Device, szTextNull, szDev, 2);
        if (!szDev[0])
        {
            if (pDM)
            {
                GlobalUnlock(pPD->hDevMode);
            }
            GlobalUnlock(pPD->hDevNames);
            StoreExtendedError(PDERR_PRINTERNOTFOUND);
            goto PrintDlgX_DisplayWarning;
        }
    }

    if (pDM)
    {
        GlobalUnlock(pPD->hDevMode);
    }
    if (pDN)
    {
        GlobalUnlock(pPD->hDevNames);
    }
#endif

    pPD->hDC = 0;

    StoreExtendedError(CDERR_GENERALCODES);

    //
    //  Do minimal work when requesting a default printer.
    //
    if (pPD->Flags & PD_RETURNDEFAULT)
    {
        //
        //  Do not display a warning in this case if it fails.
        //  MFC 3.1 does not specify PD_NOWARNING, but that's what
        //  it really wants.
        //
        nResult = PrintReturnDefault(pPI);
        PrintClosePrinters(pPI);
        return (nResult);
    }

    if (!PrintLoadIcons())
    {
        //
        //  If the icons cannot be loaded, then fail.
        //
        StoreExtendedError(PDERR_SETUPFAILURE);
        goto PrintDlgX_DisplayWarning;
    }

    //
    //  Printer enumeration is delayed until the combobox is dropped down.
    //  However, if a printer is specified, we must force enumeration in
    //  order to find the printer so that the correct devmode can be created.
    //
    if ((pPD->hDevMode) &&
        (pPD->hDevNames) &&
        (pDM = GlobalLock(pPD->hDevMode)))
    {
        if (pDN = GlobalLock(pPD->hDevNames))
        {
            dwResult = lstrcmp((LPTSTR)pDM->dmDeviceName,
                               (LPTSTR)pDN + pDN->wDeviceOffset);
            GlobalUnlock(pPD->hDevNames);
        }
        GlobalUnlock(pPD->hDevMode);
    }

    //
    //  First : Try to open the printer in the DevMode.
    //
    //  Note: The printer name field in the DEVMODE struct is limited to
    //        32 chars which may cause this case to fail.
    //
    if ( (!dwResult) &&
         (!pPI->hCurPrinter) &&
         (pPD->hDevMode) &&
         (pDM = GlobalLock(pPD->hDevMode)) )
    {
        PrintOpenPrinter(pPI, pDM->dmDeviceName);
        GlobalUnlock(pPD->hDevMode);
    }

    //
    //  Second : Try to open the printer in the DevNames.
    //
    if ( (!pPI->hCurPrinter) &&
         (pPD->hDevNames) &&
         (pDN = GlobalLock(pPD->hDevNames)) )
    {
        PrintOpenPrinter(pPI, (LPTSTR)pDN + pDN->wDeviceOffset);
        GlobalUnlock(pPD->hDevNames);
    }

    //
    //  Third : Try to open the Default Printer.
    //
    PrintGetDefaultPrinterName(pPI->szDefaultPrinter, MAX_PRINTERNAME);
    if (!pPI->hCurPrinter)
    {
        if (pPI->szDefaultPrinter[0])
        {
            PrintOpenPrinter(pPI, pPI->szDefaultPrinter);
        }
    }

    //
    //  Fourth : Enumerate the Printers and try to open one of those.
    //
    if (!pPI->hCurPrinter)
    {
        if (!PrintEnumAndSelect(pPD->hwndOwner, 0, pPI, NULL, TRUE))
        {
            //
            //  There are no printers installed in the system.
            //
            goto PrintDlgX_DisplayWarning;
        }
    }

    //
    //  Save the original information passed in by the app in case the user
    //  hits cancel.
    //
    dwFlags = pPD->Flags;
    nCopies = pPD->nCopies;
    nFromPage = pPD->nFromPage;
    nToPage = pPD->nToPage;
    hDevNames = pPD->hDevNames;
    hDevMode = pPD->hDevMode;
    if ((pPI->pCurPrinter) &&
        (lstrlen(pPI->pCurPrinter->pPrinterName) < MAX_PRINTERNAME))
    {
        lstrcpy(szPrinterName, pPI->pCurPrinter->pPrinterName);
    }
    else
    {
        szPrinterName[0] = 0;
    }

    pPD->hDevNames = NULL;
    pPD->hDevMode = NULL;

    //
    //  Build a copy of the DevNames.
    //
    PrintBuildDevNames(pPI);

    //
    //  Get the *correct* DevMode.
    //
    if (hDevMode)
    {
        pDevMode = GlobalLock(hDevMode);
    }

#ifdef UNICODE
    else
    {
        //
        //  If it's WOW and the app didn't specify a devmode, get the 16-bit
        //  devmode out of the registry (ie. win.ini [Windows] device section).
        //
        if (pPI->bUseExtDeviceMode)
        {
            pDevMode = (pPI->pCurPrinter)->pDevMode;
            if (pDevMode)
            {
                cbNeeded = sizeof(DEVMODEW) + pDevMode->dmDriverExtra;
                goto GotWOWDMSize;
            }

            //
            //  If a 16-bit devmode isn't available in the registry,
            //  drop through and get the system default devmode.
            //
        }
    }
#endif

    cbNeeded = DocumentProperties( pPD->hwndOwner,
                                   pPI->hCurPrinter,
                                   (pPI->pCurPrinter)
                                       ? pPI->pCurPrinter->pPrinterName
                                       : NULL,
                                   NULL,
                                   NULL,
                                   0 );
#ifdef UNICODE
GotWOWDMSize:
#endif
    if ((cbNeeded > 0) &&
        (pPD->hDevMode = GlobalAlloc(GHND, cbNeeded)))
    {
        BOOL fSuccess = FALSE;

        if (pDM = GlobalLock(pPD->hDevMode))
        {
#ifdef UNICODE
            if (pPI->bUseExtDeviceMode && !hDevMode)
            {
                CopyMemory(pDM, pDevMode, cbNeeded);
                fSuccess = TRUE;
                goto GotNewWOWDM;
            }
#endif
            fSuccess = DocumentProperties( pPD->hwndOwner,
                                           pPI->hCurPrinter,
                                           (pPI->pCurPrinter)
                                               ? pPI->pCurPrinter->pPrinterName
                                               : NULL,
                                           pDM,            // out
                                           pDevMode,       // in
                                           DM_MODIFY | DM_COPY ) == IDOK;
#ifdef UNICODE
GotNewWOWDM:
#endif
            if (pDM->dmFields & DM_COPIES)
            {
                if ((hDevMode) || (pPD->Flags & PD_USEDEVMODECOPIES))
                {
                    pPD->nCopies = pDM->dmCopies;
                }
                else if (pPD->nCopies)
                {
                    pDM->dmCopies = pPD->nCopies;
                }
            }
            if (pDM->dmFields & DM_COLLATE)
            {
                if ((hDevMode) || (pPD->Flags & PD_USEDEVMODECOPIES))
                {
                    if (pDM->dmCollate == DMCOLLATE_FALSE)
                    {
                        pPD->Flags  &= ~PD_COLLATE;
                        pPI->Status &= ~PI_COLLATE_REQUESTED;
                    }
                    else
                    {
                        pPD->Flags  |= PD_COLLATE;
                        pPI->Status |= PI_COLLATE_REQUESTED;
                    }
                }
                else
                {
                    pDM->dmCollate = (pPD->Flags & PD_COLLATE)
                                         ? DMCOLLATE_TRUE
                                         : DMCOLLATE_FALSE;
                }
            }

            GlobalUnlock(pPD->hDevMode);
        }

        if (!fSuccess)
        {
            GlobalFree(pPD->hDevMode);
            pPD->hDevMode = NULL;
        }
    }

    if (hDevMode)
    {
        GlobalUnlock(hDevMode);
    }

    //
    //  Get the default source string.
    //
    CDLoadString(g_hinst, iszDefaultSource, szDefaultSrc, SCRATCHBUF_SIZE);

    //
    //  Call the appropriate dialog routine.
    //
    switch (pPD->Flags & (PD_PRINTSETUP | PD_PAGESETUP))
    {
        case ( 0 ) :
        {
            nResult = PrintDisplayPrintDlg(pPI);
            break;
        }
        case ( PD_PRINTSETUP ) :
        case ( PD_PAGESETUP ) :
        {
            nResult = PrintDisplaySetupDlg(pPI);
            break;
        }
        default :
        {
            StoreExtendedError(CDERR_INITIALIZATION);
            break;
        }
    }

    if (nResult > 0)
    {
        //
        //  User hit OK, so free the copies of the handles passed in
        //  by the app.
        //
        if (hDevMode && (hDevMode != pPD->hDevMode))
        {
            GlobalFree(hDevMode);
            hDevMode = NULL;
        }
        if (hDevNames && (hDevNames != pPD->hDevNames))
        {
            GlobalFree(hDevNames);
            hDevNames = NULL;
        }

        if (pPD->hDevMode)
        {
            //
            //  Make sure the device name in the devmode is null
            //  terminated.
            //
            pDevMode = GlobalLock(pPD->hDevMode);
            pDevMode->dmDeviceName[CCHDEVICENAME - 1] = 0;
            GlobalUnlock(pPD->hDevMode);
        }
    }
    else
    {
        //
        //  User hit CANCEL or there was an error, so restore original
        //  values passed in by the app.
        //
        pPD->Flags = dwFlags;
        pPD->nCopies = nCopies;
        pPD->nFromPage = nFromPage;
        pPD->nToPage = nToPage;
        if (pPD->hDevMode && (pPD->hDevMode != hDevMode))
        {
            GlobalFree(pPD->hDevMode);
        }
        if (pPD->hDevNames && (pPD->hDevNames != hDevNames))
        {
            GlobalFree(pPD->hDevNames);
        }
        pPD->hDevNames = hDevNames;
        pPD->hDevMode = hDevMode;

        //
        //  If we've been called from Page Setup, then we need to reset
        //  the current printer.
        //
        if (pPI->Status & PI_PRINTDLGX_RECURSE)
        {
            PrintCancelPrinterChanged(pPI, szPrinterName);
        }
    }

    //
    //  Make sure that we are really supposed to be leaving this function
    //  before we start closing printers and displaying error messages.
    //
    if (pPI->Status & PI_PRINTDLGX_RECURSE)
    {
        return (nResult > 0);
    }

    //
    //  Close the printers that were opened.
    //
    PrintClosePrinters(pPI);

    //
    //  Display any error messages.
    //
PrintDlgX_DisplayWarning:

    if ((nResult < 0) && (!(pPD->Flags & PD_NOWARNING)))
    {
        DWORD dwErr = GetStoredExtendedError();

        //
        //  Only do this for new apps.
        //
        if ( (pPI->ProcessVersion >= 0x40000) ||
             (dwErr == PDERR_NODEFAULTPRN) ||
             (dwErr == PDERR_PRINTERNOTFOUND) )
        {
            TCHAR szWarning[SCRATCHBUF_SIZE];
            TCHAR szTitle[SCRATCHBUF_SIZE];
            int iszWarning;

            szTitle[0] = TEXT('\0');
            if (pPD->hwndOwner)
            {
                GetWindowText(pPD->hwndOwner, szTitle, SCRATCHBUF_SIZE);
            }
            if (!szTitle[0])
            {
                CDLoadString(g_hinst, iszWarningTitle, szTitle, SCRATCHBUF_SIZE);
            }

            switch (dwErr)
            {
                case ( PDERR_NODEFAULTPRN ) :
                {
                    iszWarning = iszNoPrnsInstalled;
                    break;
                }
                case ( PDERR_PRINTERNOTFOUND ) :
                {
                    iszWarning = iszPrnNotFound;
                    break;
                }
                case ( CDERR_MEMLOCKFAILURE ) :
                case ( CDERR_MEMALLOCFAILURE ) :
                case ( PDERR_LOADDRVFAILURE ) :
                {
                    iszWarning = iszMemoryError;
                    break;
                }
                default :
                {
                    iszWarning = iszGeneralWarning;
                    break;
                }
            }

            CDLoadString(g_hinst, iszWarning, szWarning, SCRATCHBUF_SIZE);
            MessageBeep(MB_ICONEXCLAMATION);
            MessageBox( pPD->hwndOwner,
                        szWarning,
                        szTitle,
                        MB_ICONEXCLAMATION | MB_OK );
        }
    }

    return (nResult > 0);
}


#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  PageSetupDlgA
//
//  ANSI entry point for PageSetupDlg when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PageSetupDlgA(
    LPPAGESETUPDLGA pPSDA)
{
    PRINTINFO PI;
    BOOL bResult = FALSE;
    HANDLE hDevMode;
    HANDLE hDevNames;
    LPCSTR pTemplateName;


    ZeroMemory(&PI, sizeof(PRINTINFO));

    //
    //  Get the pPDA structure from the pPSDA structure.
    //
    if (bResult = ThunkPageSetupDlg(&PI, pPSDA))
    {
        //
        //  Save the original devmode and devnames.
        //
        hDevMode = pPSDA->hDevMode;
        hDevNames = pPSDA->hDevNames;
        pTemplateName = pPSDA->lpPageSetupTemplateName;

        //
        //  Convert the pPDA structure to Unicode (pPI->pPD).
        //
        if (bResult = ThunkPrintDlg(&PI, PI.pPDA))
        {
            //
            //  Fill in the pPI->pPD structure.
            //
            ThunkPrintDlgA2W(&PI);

            //
            //  Copy the Unicode information from the pPD structure to
            //  the pPSD structure for the call to PageSetupDlgX.
            //
            (PI.pPSD)->hDevMode  = (PI.pPD)->hDevMode;
            (PI.pPSD)->hDevNames = (PI.pPD)->hDevNames;

            (PI.pPSD)->lpPageSetupTemplateName = (PI.pPD)->lpSetupTemplateName;

            //
            //  Call the PageSetupDlgX function to do the work.
            //
            if (bResult = PageSetupDlgX(&PI))
            {
                //
                //  Success.  Convert the Unicode pPD structure to
                //  its ANSI equivalent.
                //
                ThunkPrintDlgW2A(&PI);

                //
                //  Save the ANSI devmode and devnames in the
                //  pPSD structure to be returned to the caller.
                //
                pPSDA->hDevMode  = (PI.pPDA)->hDevMode;
                pPSDA->hDevNames = (PI.pPDA)->hDevNames;
            }
            else
            {
                //
                //  Failure.  Restore the old devmode and devnames.
                //
                pPSDA->hDevMode = hDevMode;
                pPSDA->hDevNames = hDevNames;
            }

            //
            //  Restore the old template name (always).
            //
            pPSDA->lpPageSetupTemplateName = pTemplateName;
        }
        FreeThunkPrintDlg(&PI);
    }
    FreeThunkPageSetupDlg(&PI);

    return (bResult);
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  PageSetupDlgW
//
//  Stub UNICODE function for PageSetupDlg when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PageSetupDlgW(
    LPPAGESETUPDLGW pPSDW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (FALSE);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  PageSetupDlg
//
//  The PageSetupDlg function displays a Page Setup dialog box.  This
//  dialog box enables the user to specify the page orientation, the
//  paper size, the paper source, and the margin settings.  The
//  appearance of the printed page is shown in the dialog's page preview.
//
////////////////////////////////////////////////////////////////////////////

BOOL WINAPI PageSetupDlg(
    LPPAGESETUPDLG pPSD)
{
    PRINTINFO PI;
    BOOL bResult;

    ZeroMemory(&PI, sizeof(PRINTINFO));

    PI.pPSD = pPSD;
    PI.ApiType = COMDLG_WIDE;

    bResult = PageSetupDlgX(&PI);

    if (PI.pPD)
    {
        GlobalFree(PI.pPD);
    }

    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  PageSetupDlgX
//
//  Worker routine for the PageSetupDlg api.
//
//  NOTE:  Caller of this routine must free pPI->pPD.
//
////////////////////////////////////////////////////////////////////////////

BOOL PageSetupDlgX(
    PPRINTINFO pPI)
{
    LPPAGESETUPDLG pPSD = pPI->pPSD;
    BOOL bResult = FALSE;
    LPPRINTDLG pPD;
    RECT rtMinMargin;
    RECT rtMargin;
    POINT ptPaperSize;
    DWORD Flags;


    if (!pPSD)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pPSD->lStructSize != sizeof(PAGESETUPDLG))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if ((pPSD->Flags & PSD_RETURNDEFAULT) &&
        (pPSD->hDevNames || pPSD->hDevMode))
    {
        StoreExtendedError(PDERR_RETDEFFAILURE);
        return (FALSE);
    }

    //
    //  Make sure only the PSD_* bits are on.  Otherwise, bad things
    //  will happen.
    //
    if ((pPSD->Flags & ~(PSD_MINMARGINS |
                         PSD_MARGINS |
                         PSD_INTHOUSANDTHSOFINCHES |
                         PSD_INHUNDREDTHSOFMILLIMETERS |
                         PSD_DISABLEMARGINS |
                         PSD_DISABLEPRINTER |
                         PSD_NOWARNING |                     // must be same as PD_*
                         PSD_DISABLEORIENTATION |
                         PSD_DISABLEPAPER |
                         PSD_RETURNDEFAULT |                 // must be same as PD_*
                         PSD_SHOWHELP |                      // must be same as PD_*
                         PSD_ENABLEPAGESETUPHOOK |           // must be same as PD_*
                         PSD_ENABLEPAGESETUPTEMPLATE |       // must be same as PD_*
                         PSD_ENABLEPAGESETUPTEMPLATEHANDLE | // must be same as PD_*
                         PSD_ENABLEPAGEPAINTHOOK |
                         PSD_DISABLEPAGEPAINTING |
                         CD_WX86APP |
                         PSD_NONETWORKBUTTON))  ||           // must be same as PD_*
        ((pPSD->Flags & (PSD_INTHOUSANDTHSOFINCHES |
                         PSD_INHUNDREDTHSOFMILLIMETERS)) ==
         (PSD_INTHOUSANDTHSOFINCHES | PSD_INHUNDREDTHSOFMILLIMETERS)))
    {
        StoreExtendedError(PDERR_INITFAILURE);
        return (FALSE);
    }

    if ((pPSD->Flags & PSD_MINMARGINS) && (pPSD->Flags & PSD_MARGINS))
    {
        if ( (pPSD->rtMargin.left   < pPSD->rtMinMargin.left)  ||
             (pPSD->rtMargin.top    < pPSD->rtMinMargin.top)   ||
             (pPSD->rtMargin.right  < pPSD->rtMinMargin.right) ||
             (pPSD->rtMargin.bottom < pPSD->rtMinMargin.bottom) )
        {
            StoreExtendedError(PDERR_INITFAILURE);
            return (FALSE);
        }
    }

    if (pPSD->Flags & PSD_ENABLEPAGESETUPHOOK)
    {
        if (!pPSD->lpfnPageSetupHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pPSD->lpfnPageSetupHook = NULL;
    }

    if (pPSD->Flags & PSD_ENABLEPAGEPAINTHOOK)
    {
        if (!pPSD->lpfnPagePaintHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pPSD->lpfnPagePaintHook = NULL;
    }

    if ((pPI->pPD) || (pPI->pPD = GlobalAlloc(GPTR, sizeof(PRINTDLG))))
    {
        pPD = pPI->pPD;

        pPD->lStructSize         = sizeof(PRINTDLG);
        pPD->hwndOwner           = pPSD->hwndOwner;
        pPD->Flags               = PD_PAGESETUP |
                                     (pPSD->Flags &
                                       (PSD_NOWARNING |
                                        PSD_SHOWHELP |
                                        PSD_ENABLEPAGESETUPHOOK |
                                        PSD_ENABLEPAGESETUPTEMPLATE |
                                        PSD_ENABLEPAGESETUPTEMPLATEHANDLE |
                                        CD_WX86APP |
                                        PSD_NONETWORKBUTTON));
        pPD->hInstance           = pPSD->hInstance;
        pPD->lCustData           = pPSD->lCustData;
        pPD->lpfnSetupHook       = pPSD->lpfnPageSetupHook;
        pPD->lpSetupTemplateName = pPSD->lpPageSetupTemplateName;
        pPD->hSetupTemplate      = pPSD->hPageSetupTemplate;

        //
        //  Save original settings in case the user hits cancel.
        //
        rtMinMargin = pPSD->rtMinMargin;
        rtMargin    = pPSD->rtMargin;
        ptPaperSize = pPSD->ptPaperSize;
        Flags       = pPSD->Flags;

        //
        //  Make sure the measure choice is set.
        //
        if ((pPSD->Flags & (PSD_INTHOUSANDTHSOFINCHES |
                            PSD_INHUNDREDTHSOFMILLIMETERS)) == 0)
        {
            TCHAR szIMeasure[2];

            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szIMeasure, 2);
            if (szIMeasure[0] == TEXT('1'))
            {
                pPSD->Flags |= PSD_INTHOUSANDTHSOFINCHES;
            }
            else
            {
                pPSD->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
            }
        }

        //
        //  Set minimum margins to 0 if not passed in.
        //
        if (!(pPSD->Flags & PSD_MINMARGINS))
        {
            pPSD->rtMinMargin.left   = 0;
            pPSD->rtMinMargin.top    = 0;
            pPSD->rtMinMargin.right  = 0;
            pPSD->rtMinMargin.bottom = 0;
        }

        //
        //  Set margins to defaults if not passed in.
        //
        if (!(pPSD->Flags & PSD_MARGINS))
        {
            LONG MarginDefault = (pPSD->Flags & PSD_INTHOUSANDTHSOFINCHES)
                                     ? INCHES_DEFAULT
                                     : MMS_DEFAULT;

            pPSD->rtMargin.left   = MarginDefault;
            pPSD->rtMargin.top    = MarginDefault;
            pPSD->rtMargin.right  = MarginDefault;
            pPSD->rtMargin.bottom = MarginDefault;
        }

        TransferPSD2PD(pPI);

        bResult = PrintDlgX(pPI);

        TransferPD2PSD(pPI);

        if (!bResult)
        {
            //
            //  Restore original settings when the user hits cancel.
            //
            pPSD->rtMinMargin = rtMinMargin;
            pPSD->rtMargin    = rtMargin;
            pPSD->ptPaperSize = ptPaperSize;
            pPSD->Flags       = Flags;
        }
    }
    else
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
    }

    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintLoadIcons
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintLoadIcons()
{
    //
    //  See if we need to load the icons.
    //
    if (bAllIconsLoaded == FALSE)
    {
        //
        //  Load the orientation icons.
        //
        hIconPortrait = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_PORTRAIT));
        hIconLandscape = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_LANDSCAPE));

        //
        //  Load the duplex icons.
        //
        hIconPDuplexNone = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_P_NONE));
        hIconLDuplexNone = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_L_NONE));
        hIconPDuplexTumble = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_P_HORIZ));
        hIconLDuplexTumble = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_L_VERT));
        hIconPDuplexNoTumble = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_P_VERT));
        hIconLDuplexNoTumble = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_L_HORIZ));

        //
        //  Load the page setup icons.
        //
        hIconPSStampP = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_P_PSSTAMP));
        hIconPSStampL = LoadIcon(g_hinst, MAKEINTRESOURCE(ICO_L_PSSTAMP));

        //
        //  Load the collation images.
        //
        hIconCollate = LoadImage( g_hinst,
                                  MAKEINTRESOURCE(ICO_COLLATE),
                                  IMAGE_ICON,
                                  0,
                                  0,
                                  LR_SHARED );
        hIconNoCollate = LoadImage( g_hinst,
                                    MAKEINTRESOURCE(ICO_NO_COLLATE),
                                    IMAGE_ICON,
                                    0,
                                    0,
                                    LR_SHARED );

        bAllIconsLoaded = ( hIconPortrait &&
                            hIconLandscape &&
                            hIconPDuplexNone &&
                            hIconLDuplexNone &&
                            hIconPDuplexTumble &&
                            hIconLDuplexTumble &&
                            hIconPDuplexNoTumble &&
                            hIconLDuplexNoTumble &&
                            hIconPSStampP &&
                            hIconPSStampL &&
                            hIconCollate &&
                            hIconNoCollate );
    }

    //
    //  Return TRUE only if all icons/images were loaded properly.
    //
    return (bAllIconsLoaded);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintDisplayPrintDlg
//
////////////////////////////////////////////////////////////////////////////

int PrintDisplayPrintDlg(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    int fGotInput = -1;
    HANDLE hDlgTemplate = NULL;
    HANDLE hInstance;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif


    //
    //  NOTE:  The print hook check must be done here rather than in
    //         PrintDlgX.  Old apps that set this flag without the
    //         PrintHook when calling Print Setup will fail - they
    //         used to succeed.
    //
    if (pPD->Flags & PD_ENABLEPRINTHOOK)
    {
        if (!pPD->lpfnPrintHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pPD->lpfnPrintHook = NULL;
    }

    if (pPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE)
    {
        if (pPD->hPrintTemplate)
        {
            hDlgTemplate = pPD->hPrintTemplate;
            hInstance = g_hinst;
        }
        else
        {
            StoreExtendedError(CDERR_NOTEMPLATE);
        }
    }
    else
    {
        LPTSTR pTemplateName = NULL;

        if (pPD->Flags & PD_ENABLEPRINTTEMPLATE)
        {
            if (pPD->lpPrintTemplateName)
            {
                if (pPD->hInstance)
                {
                    pTemplateName = (LPTSTR)pPD->lpPrintTemplateName;
                    hInstance = pPD->hInstance;

                }
                else
                {
                    StoreExtendedError(CDERR_NOHINSTANCE);
                }
            }
            else
            {
                StoreExtendedError(CDERR_NOTEMPLATE);
            }
        }
        else
        {
            hInstance = g_hinst;
            pTemplateName = (LPTSTR)(DWORD)PRINTDLGORD;
        }

        if (pTemplateName)
        {
            hDlgTemplate = PrintLoadResource( hInstance,
                                              pTemplateName,
                                              RT_DIALOG);
        }
    }

    if (!hDlgTemplate)
    {
        return (FALSE);
    }

    if (LockResource(hDlgTemplate))
    {
        glpfnPrintHook = GETPRINTHOOKFN(pPD);

#ifdef UNICODE
        if (IS16BITWOWAPP(pPD))
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        fGotInput = (BOOL)DialogBoxIndirectParamAorW( hInstance,
                                                (LPDLGTEMPLATE)hDlgTemplate,
                                                pPD->hwndOwner,
                                                PrintDlgProc,
                                                (LPARAM)pPI,
                                                uiWOWFlag );
#else
        fGotInput = (BOOL)DialogBoxIndirectParam( hInstance,
                                            (LPDLGTEMPLATE)hDlgTemplate,
                                            pPD->hwndOwner,
                                            PrintDlgProc,
                                            (LPARAM)pPI );
#endif

        glpfnPrintHook = NULL;
        if (fGotInput == -1)
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
    }

    return (fGotInput);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintDisplaySetupDlg
//
////////////////////////////////////////////////////////////////////////////

int PrintDisplaySetupDlg(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    int fGotInput = -1;
    HANDLE hDlgTemplate = NULL;
    HANDLE hInstance;
#ifdef UNICODE
    UINT uiWOWFlag = 0;
#endif


    //
    //  NOTE:  The setup hook check must be done here rather than in
    //         PrintDlgX.  Old apps that set this flag without the
    //         SetupHook when calling Print will fail - they
    //         used to succeed.
    //
    if (pPD->Flags & PD_ENABLESETUPHOOK)
    {
        if (!pPD->lpfnSetupHook)
        {
            StoreExtendedError(CDERR_NOHOOK);
            return (FALSE);
        }
    }
    else
    {
        pPD->lpfnSetupHook = NULL;
    }

    if (pPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE)
    {
        if (pPD->hSetupTemplate)
        {
            hDlgTemplate = pPD->hSetupTemplate;
            hInstance = g_hinst;
        }
        else
        {
            StoreExtendedError(CDERR_NOTEMPLATE);
        }
    }
    else
    {
        LPTSTR pTemplateName = NULL;

        if (pPD->Flags & PD_ENABLESETUPTEMPLATE)
        {
            if (pPD->lpSetupTemplateName)
            {
                if (pPD->hInstance)
                {
                    pTemplateName = (LPTSTR)pPD->lpSetupTemplateName;
                    hInstance = pPD->hInstance;
                }
                else
                {
                    StoreExtendedError(CDERR_NOHINSTANCE);
                }
            }
            else
            {
                StoreExtendedError(CDERR_NOTEMPLATE);
            }
        }
        else
        {
            hInstance = g_hinst;
            pTemplateName = (LPTSTR)(DWORD)( (pPD->Flags & PD_PRINTSETUP)
                                                 ? PRNSETUPDLGORD
                                                 : PAGESETUPDLGORD );
        }

        if (pTemplateName)
        {
            hDlgTemplate = PrintLoadResource( hInstance,
                                              pTemplateName,
                                              RT_DIALOG);
        }
    }

    if (!hDlgTemplate)
    {
        return (FALSE);
    }

    if (LockResource(hDlgTemplate))
    {
        glpfnSetupHook = GETSETUPHOOKFN(pPD);

#ifdef UNICODE
        if (IS16BITWOWAPP(pPD))
        {
            uiWOWFlag = SCDLG_16BIT;
        }

        fGotInput = (BOOL)DialogBoxIndirectParamAorW( hInstance,
                                                (LPDLGTEMPLATE)hDlgTemplate,
                                                pPD->hwndOwner,
                                                PrintSetupDlgProc,
                                                (LPARAM)pPI,
                                                uiWOWFlag );
#else
        fGotInput = (BOOL)DialogBoxIndirectParam( hInstance,
                                            (LPDLGTEMPLATE)hDlgTemplate,
                                            pPD->hwndOwner,
                                            PrintSetupDlgProc,
                                            (LPARAM)pPI );
#endif

        glpfnSetupHook = NULL;
        if (fGotInput == -1)
        {
            StoreExtendedError(CDERR_DIALOGFAILURE);
        }
    }
    else
    {
        StoreExtendedError(CDERR_LOCKRESFAILURE);
    }

    return (fGotInput);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgProc
//
//  Print Dialog procedure.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK PrintDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPRINTINFO pPI;
    LPPRINTDLG pPD;
    HWND hCtl;
    BOOL bTest;
    BOOL_PTR bResult;
    LPDEVMODE pDM;
    LPDEVNAMES pDN;


    if (pPI = (PPRINTINFO)GetProp(hDlg, PRNPROP))
    {
        if ((pPD = pPI->pPD) && (pPD->lpfnPrintHook))
        {
            LPPRINTHOOKPROC lpfnPrintHook = GETPRINTHOOKFN(pPD);

#ifdef UNICODE
            if (pPI->ApiType == COMDLG_ANSI)
            {
                ThunkPrintDlgW2A(pPI);
            }
#endif
            if ((bResult = (*lpfnPrintHook)(hDlg, wMsg, wParam, lParam)))
            {
#ifdef UNICODE
                if (pPI->ApiType == COMDLG_ANSI)
                {
                    ThunkPrintDlgA2W(pPI);
                }
#endif
                return (bResult);
            }
        }
    }
    else if (glpfnPrintHook &&
             (wMsg != WM_INITDIALOG) &&
             (bResult = (*glpfnPrintHook)(hDlg, wMsg, wParam, lParam)))
    {

        return (bResult);
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            DWORD dwResult = 0;

            HourGlass(TRUE);
#ifndef WINNT
            msgHELPA = RegisterWindowMessage(szCommdlgHelp);
#endif
            SetProp(hDlg, PRNPROP, (HANDLE)lParam);
            glpfnPrintHook = NULL;

            pPI = (PPRINTINFO)lParam;
            pPD = pPI->pPD;
            if (pPI->pPSD)
            {
                TCHAR szTitle[32];
                RECT aRtDlg;
                RECT aRtGrp;
                RECT aRtYep;
                RECT aRtCan;
                HWND hBtnYep = GetDlgItem(hDlg, IDOK);
                HWND hBtnCan = GetDlgItem(hDlg, IDCANCEL);
                RECT aRtWhere;
                RECT aRtCmmnt;
                LONG GapHeight, DlgTop;

                //
                //  Save the client coordinate for the top of the dialog.
                //  Also, save the height of the gap between the bottom of
                //  the original OK button and the bottom of the original
                //  dialog.
                //
                GetWindowRect(hDlg, &aRtDlg);
                GetWindowRect(hBtnYep, &aRtYep);
                ScreenToClient(hDlg, (LPPOINT)&aRtDlg.left);
                ScreenToClient(hDlg, (LPPOINT)&aRtDlg.right);
                ScreenToClient(hDlg, (LPPOINT)&aRtYep.right);
                DlgTop = aRtDlg.top;
                GapHeight = (aRtDlg.bottom - aRtYep.bottom > 0)
                               ? aRtDlg.bottom - aRtYep.bottom
                               : 15;

                //
                //  Display the title of the dialog box.
                //
                GetWindowText(GetParent(hDlg), szTitle, 32);
                SetWindowText(hDlg, szTitle);

                //
                //  Get the screen and client coordinates for the dialog,
                //  the Printer group box, the OK button, and the Cancel
                //  button.  These will be used to reposition the OK,
                //  Cancel, and Help buttons.
                //
                GetWindowRect(hDlg, &aRtDlg);
                GetWindowRect(GetDlgItem(hDlg, ID_PRINT_G_PRINTER), &aRtGrp);
                GetWindowRect(hBtnYep, &aRtYep);
                GetWindowRect(hBtnCan, &aRtCan);

                //
                //If we are in a mirrored Dlg the use the left side.
                //
                if (IS_WINDOW_RTL_MIRRORED(hDlg)) {
                    aRtDlg.right = aRtDlg.left;
                }
                ScreenToClient(hDlg   , (LPPOINT)&aRtDlg.right);
                ScreenToClient(hDlg   , (LPPOINT)&aRtGrp.right);
                MapWindowPoints(NULL, hDlg, (LPPOINT)&aRtYep, 2);
                aRtYep.right -= aRtYep.left;
                aRtYep.bottom -= aRtYep.top;

                MapWindowPoints(NULL, hDlg, (LPPOINT)&aRtCan, 2);
                aRtCan.right -= aRtCan.left;
                aRtCan.bottom -= aRtCan.top;
#ifdef WINNT
                if (pPD->Flags & PD_SHOWHELP)
                {
                    HWND hBtnHlp = GetDlgItem(hDlg, ID_BOTH_P_HELP);
                    RECT aRtHlp;

                    //
                    //  Move the Help button up underneath the
                    //  Printer group box.
                    //
                    if (hBtnHlp)
                    {
                        GetWindowRect(hBtnHlp, &aRtHlp);
                        MapWindowPoints(NULL, hDlg, (LPPOINT)&aRtHlp, 2);
                        aRtHlp.right -= aRtHlp.left;
                        aRtHlp.bottom -= aRtHlp.top;

                        MoveWindow( hBtnHlp,
                                    aRtHlp.left,
                                    aRtGrp.bottom + 2 * aRtHlp.bottom / 3,
                                    aRtHlp.right,
                                    aRtHlp.bottom,
                                    FALSE );
                    }
                }
#endif
                //
                //  Move the OK and Cancel buttons up underneath the
                //  Printer group box.
                //
                MoveWindow( hBtnYep,
                            aRtYep.left,
                            aRtGrp.bottom + 2 * aRtYep.bottom / 3,
                            aRtYep.right,
                            aRtYep.bottom,
                            FALSE );
                MoveWindow( hBtnCan,
                            aRtCan.left,
                            aRtGrp.bottom + 2 * aRtCan.bottom / 3,
                            aRtCan.right,
                            aRtCan.bottom,
                            FALSE );

                //
                //  Resize the dialog.
                //
                GetWindowRect(hBtnYep, &aRtYep);
                MapWindowPoints(NULL, hDlg, (LPPOINT)&aRtYep, 2);
                MoveWindow( hDlg,
                            aRtDlg.left,
                            aRtDlg.top,
                            aRtDlg.right,
                            (aRtYep.bottom - DlgTop) + GapHeight,
                            FALSE );

                //
                //  Hide all other print dlg items.
                //
                //  NOTE: Need to do a SetWindowPos to actually remove
                //        the window so that the AddNetButton call does
                //        not think it's there.
                //
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_X_TOFILE),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_X_COLLATE),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_E_FROM),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_E_TO),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_E_COPIES),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_G_RANGE),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_G_COPIES),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_I_COLLATE),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_R_ALL),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_R_SELECTION),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_R_PAGES),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_S_FROM),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_S_TO),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );
                SetWindowPos( GetDlgItem(hDlg, ID_PRINT_S_COPIES),
                              NULL,
                              0, 0, 0, 0,
                              SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );

                //
                //  Enlarge the comment edit control, since the
                //  "print to file" check box is hidden.
                //
                GetWindowRect(GetDlgItem(hDlg, ID_BOTH_S_WHERE), &aRtWhere);
                GetWindowRect( hCtl = GetDlgItem(hDlg, ID_BOTH_S_COMMENT),
                               &aRtCmmnt );
                MapWindowPoints(NULL, hDlg, (LPPOINT)&aRtCmmnt, 2);
                MoveWindow( hCtl,
                            aRtCmmnt.left,
                            aRtCmmnt.top,
                            aRtWhere.right  - aRtWhere.left,
                            aRtWhere.bottom - aRtWhere.top,
                            FALSE );
#ifdef WINNT
                //
                //  Add or hide net button, if necessary.
                //
                if ((pPD->Flags & PD_NONETWORKBUTTON))
                {
                    if (hCtl = GetDlgItem(hDlg, ID_BOTH_P_NETWORK))
                    {
                        EnableWindow(hCtl, FALSE);
                        ShowWindow(hCtl, SW_HIDE);
                    }
                }
                else
                {
                    AddNetButton( hDlg,
                                  g_hinst,
                                  FILE_BOTTOM_MARGIN,
                                  TRUE,
                                  FALSE,
                                  TRUE);

                    //
                    //  The button can be added in two ways -
                    //      statically (they have it predefined in their template) and
                    //      dynamically (successful call to AddNetButton).
                    //
                    if (!IsNetworkInstalled())
                    {
                        hCtl = GetDlgItem(hDlg, ID_BOTH_P_NETWORK);

                        EnableWindow(hCtl, FALSE);
                        ShowWindow(hCtl, SW_HIDE);
                    }
                }
#endif
            }
            else
            {
                if (pPD->Flags & PD_COLLATE)
                {
                    pPI->Status |= PI_COLLATE_REQUESTED;
                }
            }

            if (!PrintInitGeneral(hDlg, ID_PRINT_C_NAME, pPI) ||
                ((dwResult = PrintInitPrintDlg( hDlg,
                                                wParam,
                                                pPI )) == 0xFFFFFFFF))
            {
                RemoveProp(hDlg, PRNPROP);
                EndDialog(hDlg, -2);
            }

            HourGlass(FALSE);
            bResult = (dwResult == 1);
            return (bResult);
        }
        case ( WM_COMMAND ) :
        {
            if (!pPI)
            {
                return (FALSE);
            }

            bResult = FALSE;

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( ID_PRINT_C_NAME ) :       // Printer Name combobox
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PrintPrinterChanged(hDlg, ID_PRINT_C_NAME, pPI);
                    }
                    else if ( (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_DROPDOWN) &&
                              !(pPI->Status & PI_PRINTERS_ENUMERATED) )
                    {
                        //
                        //  Enumerate printers if this hasn't been done yet.
                        //
                        PrintEnumAndSelect( hDlg,
                                            ID_PRINT_C_NAME,
                                            pPI,
                                            (pPI->pCurPrinter)
                                              ? pPI->pCurPrinter->pPrinterName
                                              : NULL,
                                            TRUE );
                    }

                    break;
                }
                case ( ID_BOTH_P_PROPERTIES ) :  // Properties... button
                {
                    PrintChangeProperties(hDlg, ID_PRINT_C_NAME, pPI);

                    break;
                }
                case ( ID_PRINT_P_SETUP ) :      // Setup... button
                {
                    DWORD dwFlags = pPD->Flags;
                    HWND hwndOwner = pPD->hwndOwner;

                    pPD->Flags |= PD_PRINTSETUP;
                    pPD->Flags &= ~(PD_RETURNDC | PD_RETURNIC);
                    pPI->Status |= PI_PRINTDLGX_RECURSE;
                    pPD->hwndOwner = hDlg;

                    if (PrintDlgX(pPI))
                    {
                        if (!PrintInitBannerAndQuality(hDlg, pPI, pPD))
                        {
                            StoreExtendedError(CDERR_GENERALCODES);
                        }
                    }

                    pPI->Status &= ~PI_PRINTDLGX_RECURSE;
                    pPD->Flags = dwFlags;
                    pPD->hwndOwner = hwndOwner;

                    break;
                }
                case ( ID_PRINT_R_ALL ) :        // Print Range - All
                case ( ID_PRINT_R_SELECTION ) :  // Print Range - Selection
                case ( ID_PRINT_R_PAGES ) :      // Print Range - Pages (From, To)
                {
                    CheckRadioButton( hDlg,
                                      ID_PRINT_R_ALL,
                                      ID_PRINT_R_PAGES,
                                      GET_WM_COMMAND_ID(wParam, lParam) );

                    //
                    //  Only move the the focus to the "From" control when
                    //  the up/down arrow is NOT used.
                    //
                    if ( !IS_KEY_PRESSED(VK_UP) &&
                         !IS_KEY_PRESSED(VK_DOWN) &&
                         ((BOOL)(GET_WM_COMMAND_ID(wParam, lParam) == ID_PRINT_R_PAGES)) )
                    {
                        SendMessage( hDlg,
                                     WM_NEXTDLGCTL,
                                     (WPARAM)GetDlgItem(hDlg, ID_PRINT_E_FROM),
                                     1L );
                    }

                    break;
                }
                case ( ID_PRINT_E_FROM ) :       // From  (Print Range - Pages)
                {
                    //
                    //  Only enable the "To" control if the "From" control
                    //  contains a value.
                    //
                    GetDlgItemInt(hDlg, ID_PRINT_E_FROM, &bTest, FALSE);
                    EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_TO), bTest);
                    EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_TO), bTest);

                    //  FALL THRU...
                }
                case ( ID_PRINT_E_TO ) :         // To  (Print Range - Pages)
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        CheckRadioButton( hDlg,
                                          ID_PRINT_R_ALL,
                                          ID_PRINT_R_PAGES,
                                          ID_PRINT_R_PAGES );
                    }

                    break;
                }


                case (ID_PRINT_E_COPIES ) :
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)                        
                    {
                        BOOL bTest;
                        //
                        //  Save the number of copies.
                        //
                        DWORD nCopies = GetDlgItemInt(hDlg, ID_PRINT_E_COPIES, &bTest, FALSE);

                        //
                        //  If the copy count is > 1, enable collate.  Otherwise,
                        //  disable it.
                        //
                        if (hCtl = GetDlgItem(hDlg, ID_PRINT_X_COLLATE))
                        {
                            EnableWindow(hCtl, (nCopies > 1));
                        }

                    }

                    break;
                }

                case ( ID_PRINT_X_COLLATE ) :    // Collate check box
                {
                    if (hCtl = GetDlgItem(hDlg, ID_PRINT_I_COLLATE))
                    {
                        ShowWindow(hCtl, SW_HIDE);
                        SendMessage( hCtl,
                                     STM_SETICON,
                                     IsDlgButtonChecked(hDlg, ID_PRINT_X_COLLATE)
                                         ? (LONG_PTR)hIconCollate
                                         : (LONG_PTR)hIconNoCollate,
                                     0L );
                        ShowWindow(hCtl, SW_SHOW);

                        if (IsDlgButtonChecked(hDlg, ID_PRINT_X_COLLATE))
                        {
                            pPI->Status |= PI_COLLATE_REQUESTED;
                        }
                        else
                        {
                            pPI->Status &= ~PI_COLLATE_REQUESTED;
                        }
                    }

                    break;
                }
                case ( ID_BOTH_P_NETWORK ) :     // Network... button
                {
#ifdef WINNT
                    HANDLE hPrinter;
                    DWORD cbPrinter = 0;
                    PPRINTER_INFO_2 pPrinter = NULL;

                    hPrinter = (HANDLE)ConnectToPrinterDlg(hDlg, 0);
                    if (hPrinter)
                    {
                        if (!GetPrinter( hPrinter,
                                         2,
                                         (LPBYTE)pPrinter,
                                         cbPrinter,
                                         &cbPrinter ))
                        {
                            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                            {
                                if (pPrinter = LocalAlloc(LPTR, cbPrinter))
                                {
                                    if (!GetPrinter( hPrinter,
                                                     2,
                                                     (LPBYTE)pPrinter,
                                                     cbPrinter,
                                                     &cbPrinter ))
                                    {
                                        StoreExtendedError(PDERR_PRINTERNOTFOUND);
                                    }
                                    else
                                    {
                                        SendDlgItemMessage( hDlg,
                                                            ID_PRINT_C_NAME,
                                                            CB_RESETCONTENT,
                                                            0,
                                                            0 );
                                        PrintEnumAndSelect( hDlg,
                                                            ID_PRINT_C_NAME,
                                                            pPI,
                                                            pPrinter->pPrinterName,
                                                            TRUE );
                                    }
                                }
                                else
                                {
                                    StoreExtendedError(CDERR_MEMALLOCFAILURE);
                                }
                            }
                            else
                            {
                                StoreExtendedError(PDERR_SETUPFAILURE);
                            }
                        }

                        if (!GetStoredExtendedError())
                        {
                            SendDlgItemMessage( hDlg,
                                                ID_PRINT_C_NAME,
                                                CB_SETCURSEL,
                                                (WPARAM)SendDlgItemMessage(
                                                      hDlg,
                                                      ID_PRINT_C_NAME,
                                                      CB_FINDSTRING,
                                                      0,
                                                      (LPARAM)pPrinter->pPrinterName ),
                                                (LPARAM)0 );

                            PrintPrinterChanged(hDlg, ID_PRINT_C_NAME, pPI);
                        }

                        LocalFree(pPrinter);
                        ClosePrinter(hPrinter);
                    }
#else
                    WNetConnectionDialog(hDlg, RESOURCETYPE_PRINT);
#endif
                    break;
                }
                case ( ID_BOTH_P_HELP ) :        // Help button
                {
#ifdef UNICODE
                    if (pPI->ApiType == COMDLG_ANSI)
                    {
                        if (msgHELPA && pPD->hwndOwner)
                        {
                            SendMessage( pPD->hwndOwner,
                                         msgHELPA,
                                         (WPARAM)hDlg,
                                         (DWORD_PTR)pPI->pPDA );
                        }
                    }
                    else
#endif
                    {
                        if (msgHELPW && pPD->hwndOwner)
                        {
                            SendMessage( pPD->hwndOwner,
                                         msgHELPW,
                                         (WPARAM)hDlg,
                                         (DWORD_PTR)pPD );
                        }
                    }

                    break;
                }
                case ( IDOK ) :                  // OK button
                {
                    bResult = TRUE;
                    if (!(pPI->pPSD))
                    {
                        pPD->Flags &= ~((DWORD)( PD_PRINTTOFILE |
                                                 PD_PAGENUMS    |
                                                 PD_SELECTION   |
                                                 PD_COLLATE ));

                        pPD->nCopies = (WORD)GetDlgItemInt( hDlg,
                                                            ID_PRINT_E_COPIES,
                                                            &bTest,
                                                            FALSE );
                        if ((!bTest) || (!pPD->nCopies))
                        {
                            PrintEditError( hDlg,
                                            ID_PRINT_E_COPIES,
                                            iszCopiesZero );
                            return (TRUE);
                        }

                        if (IsDlgButtonChecked(hDlg, ID_PRINT_R_SELECTION))
                        {
                            pPD->Flags |= PD_SELECTION;
                        }
                        else if (IsDlgButtonChecked(hDlg, ID_PRINT_R_PAGES))
                        {
                            //
                            //  Check the "From" and "To" values.
                            //
                            pPD->Flags |= PD_PAGENUMS;
                            pPD->nFromPage = (WORD)GetDlgItemInt( hDlg,
                                                                  ID_PRINT_E_FROM,
                                                                  &bTest,
                                                                  FALSE );
                            if (!bTest)
                            {
                                PrintEditError( hDlg,
                                                ID_PRINT_E_FROM,
                                                iszPageFromError );
                                return (TRUE);
                            }

                            pPD->nToPage = (WORD)GetDlgItemInt( hDlg,
                                                                ID_PRINT_E_TO,
                                                                &bTest,
                                                                FALSE );
                            if (!bTest)
                            {
                                TCHAR szBuf[PAGE_EDIT_SIZE + 1];

                                if (GetDlgItemText( hDlg,
                                                    ID_PRINT_E_TO,
                                                    szBuf,
                                                    PAGE_EDIT_SIZE + 1 ))
                                {
                                    PrintEditError( hDlg,
                                                    ID_PRINT_E_TO,
                                                    iszPageToError );
                                    return (TRUE);
                                }
                                else
                                {
                                    pPD->nToPage = pPD->nFromPage;
                                }
                            }

                            if ( (pPD->nFromPage < pPD->nMinPage) ||
                                 (pPD->nFromPage > pPD->nMaxPage) )
                            {
                                PrintEditError( hDlg,
                                                ID_PRINT_E_FROM,
                                                iszPageRangeError,
                                                pPD->nMinPage,
                                                pPD->nMaxPage );
                                return (TRUE);
                            }
                            if ( (pPD->nToPage < pPD->nMinPage) ||
                                 (pPD->nToPage > pPD->nMaxPage) )
                            {
                                PrintEditError( hDlg,
                                                ID_PRINT_E_TO,
                                                iszPageRangeError,
                                                pPD->nMinPage,
                                                pPD->nMaxPage );
                                return (TRUE);
                            }
                            if (pPD->nFromPage > pPD->nToPage)
                            {
                                PrintEditError( hDlg,
                                                ID_PRINT_E_FROM,
                                                iszFromToError );
                                return (TRUE);
                            }
                        }
                    }

                    HourGlass(TRUE);

                    if (IsDlgButtonChecked(hDlg, ID_PRINT_X_TOFILE))
                    {
                        pPD->Flags |= PD_PRINTTOFILE;
                    }

                    if ( (hCtl = GetDlgItem(hDlg, ID_PRINT_X_COLLATE)) &&
                         IsWindowEnabled(hCtl) &&
                         IsDlgButtonChecked(hDlg, ID_PRINT_X_COLLATE) )
                    {
                        pPD->Flags |= PD_COLLATE;
                    }

                    if (!PrintSetCopies(hDlg, pPI, ID_PRINT_C_NAME))
                    {
                        HourGlass(FALSE);
                        return (TRUE);
                    }

                    pDM = NULL;
                    pDN = NULL;
                    if (pPD->hDevMode)
                    {
                        pDM = GlobalLock(pPD->hDevMode);
                    }
                    if (pPD->hDevNames)
                    {
                        pDN = GlobalLock(pPD->hDevNames);
                    }
                    if (pDM && pDN)
                    {
                        DWORD nNum;

                        if ( GetDlgItem(hDlg, ID_PRINT_C_QUALITY) &&
                             (nNum = (DWORD) SendDlgItemMessage( hDlg,
                                                         ID_PRINT_C_QUALITY,
                                                         CB_GETCURSEL,
                                                         0,
                                                         0L )) != CB_ERR )
                        {
                            pDM->dmPrintQuality =
                                (WORD)SendDlgItemMessage( hDlg,
                                                          ID_PRINT_C_QUALITY,
                                                          CB_GETITEMDATA,
                                                          (WPARAM)nNum,
                                                          0L );
                        }

                        PrintReturnICDC(pPD, pDN, pDM);
                    }
                    if (pDM)
                    {
                        GlobalUnlock(pPD->hDevMode);
                    }
                    if (pDN)
                    {
                        GlobalUnlock(pPD->hDevNames);
                    }

#ifdef UNICODE
                    if (pPD->Flags & CD_WOWAPP)
                    {
                        UpdateSpoolerInfo(pPI);
                    }
#endif

                    //  FALL THRU...
                }
                case ( IDCANCEL ) :              // Cancel button
                case ( IDABORT ) :
                {
                    HourGlass(TRUE);

                    glpfnPrintHook = GETPRINTHOOKFN(pPD);

                    RemoveProp(hDlg, PRNPROP);
                    EndDialog(hDlg, bResult);

                    HourGlass(FALSE);

                    break;
                }
                default :
                {
                    return (FALSE);
                    break;
                }
            }

            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            PrintMeasureItem(hDlg, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)aPrintHelpIDs );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)aPrintHelpIDs );
            }
            break;
        }
        case ( WM_CTLCOLOREDIT ) :
        {
            if (GetWindowLong((HWND)lParam, GWL_STYLE) & ES_READONLY)
            {
                return ( (BOOL_PTR) SendMessage(hDlg, WM_CTLCOLORDLG, wParam, lParam) );
            }

            //  FALL THRU...
        }
        default :
        {
            return (FALSE);
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetupDlgProc
//
//  Print Setup Dialog proc.
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK PrintSetupDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PPRINTINFO pPI;
    LPPRINTDLG pPD;
    BOOL_PTR bResult;
    UINT uCmdId;
    LPDEVMODE pDM;


    if (pPI = (PPRINTINFO)GetProp(hDlg, PRNPROP))
    {
        if ((pPD = pPI->pPD) && (pPD->lpfnSetupHook))
        {
            LPSETUPHOOKPROC lpfnSetupHook = GETSETUPHOOKFN(pPD);

#ifdef UNICODE
            if (pPI->ApiType == COMDLG_ANSI)
            {
                ThunkPrintDlgW2A(pPI);
                TransferPDA2PSD(pPI);

                pPI->NestCtr++;
                bResult = (*lpfnSetupHook)(hDlg, wMsg, wParam, lParam);
                pPI->NestCtr--;

                if (bResult)
                {
                    TransferPSD2PDA(pPI);
                    ThunkPrintDlgA2W(pPI);
                    if (pPI->NestCtr == 0)
                    {
                        TransferPD2PSD(pPI);
                    }
                    return (bResult);
                }
            }
            else
#endif
            {
                TransferPD2PSD(pPI);

                bResult = (*lpfnSetupHook)(hDlg, wMsg, wParam, lParam);

                if (bResult)
                {
                    TransferPSD2PD(pPI);
                    return (bResult);
                }
            }
        }
    }
    else if (glpfnSetupHook &&
             (wMsg != WM_INITDIALOG) &&
             (bResult = (*glpfnSetupHook)(hDlg, wMsg, wParam, lParam)))
    {
        return (bResult);
    }

    switch (wMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            DWORD dwResult = 0;

            //
            // Disable RTL mirroring on the WHITE-SAMPLE
            //
            SetWindowLong(GetDlgItem(hDlg, ID_SETUP_W_SAMPLE), 
                          GWL_EXSTYLE,
                          GetWindowLong(GetDlgItem(hDlg, ID_SETUP_W_SAMPLE), GWL_EXSTYLE) & ~RTL_MIRRORED_WINDOW);
            HourGlass(TRUE);
#ifndef WINNT
            msgHELPA = RegisterWindowMessage(szCommdlgHelp);
#endif
            SetProp(hDlg, PRNPROP, (HANDLE)lParam);
            pPI = (PPRINTINFO)lParam;
            pPI->bKillFocus = FALSE;
            glpfnSetupHook = NULL;

            if (!PrintInitGeneral(hDlg, ID_SETUP_C_NAME, pPI) ||
                ((dwResult = PrintInitSetupDlg( hDlg,
                                                wParam,
                                                pPI )) == 0xFFFFFFFF))
            {
                RemoveProp(hDlg, PRNPROP);
                EndDialog(hDlg, FALSE);
            }
            else if (pPI->pPSD && (pPI->pPSD->Flags & PSD_RETURNDEFAULT))
            {
                //
                //  PSD_RETURNDEFAULT goes through the entire initialization
                //  in order to set rtMinMargin, rtMargin, and ptPaperSize.
                //  Win95 Notepad relies on this behavior.
                //
                SendMessage(hDlg, WM_COMMAND, IDOK, 0);
            }

            HourGlass(FALSE);
            bResult = (dwResult == 1);
            return (bResult);
        }
        case ( WM_COMMAND ) :
        {
            if (!pPI)
            {
                return (FALSE);
            }

            bResult = FALSE;

            switch (uCmdId = GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( ID_SETUP_C_NAME ) :       // Printer Name combobox
                {
                    if ( (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_DROPDOWN) &&
                         !(pPI->Status & PI_PRINTERS_ENUMERATED) )
                    {
                        //
                        //  Enumerate printers if this hasn't been done yet.
                        //
                        PrintEnumAndSelect( hDlg,
                                            ID_SETUP_C_NAME,
                                            pPI,
                                            (pPI->pCurPrinter)
                                              ? pPI->pCurPrinter->pPrinterName
                                              : NULL,
                                            TRUE );
                    }
                    if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                    {
                        break;
                    }
                    if ( !GetDlgItem(hDlg, ID_SETUP_R_SPECIFIC) ||
                         IsDlgButtonChecked(hDlg, ID_SETUP_R_SPECIFIC) )
                    {
                        PrintPrinterChanged(hDlg, ID_SETUP_C_NAME, pPI);
                        break;
                    }

                    uCmdId = ID_SETUP_R_SPECIFIC;

                    // FALL THRU...
                }
                case ( ID_SETUP_R_DEFAULT ) :    // Default printer
                case ( ID_SETUP_R_SPECIFIC ) :   // Specific printer
                {
                    //
                    //  Sanity check for Publisher bug where user tries to
                    //  set focus to ID_SETUP_R_DEFAULT on exit if the
                    //  dialog has no default printer.
                    //
                    if (pPI->hCurPrinter)
                    {
                        HWND hCmb;
                        DWORD dwStyle;

                        hCmb = GetDlgItem(hDlg, ID_SETUP_C_NAME);
                        if (hCmb && (uCmdId == ID_SETUP_R_DEFAULT))
                        {
                            if (!(pPI->Status & PI_PRINTERS_ENUMERATED))
                            {
                                //
                                //  Enumerate printers if this hasn't been
                                //  done yet.  Otherwise, the default printer
                                //  may not be found in the list box when
                                //  switching from Specific to Default.
                                //
                                PrintEnumAndSelect( hDlg,
                                                    ID_SETUP_C_NAME,
                                                    pPI,
                                                    NULL,
                                                    TRUE );
                            }

                            SendMessage( hCmb,
                                         CB_SETCURSEL,
                                         (WPARAM)SendMessage(
                                             hCmb,
                                             CB_FINDSTRINGEXACT,
                                             (WPARAM)-1,
                                             (LPARAM)(pPI->szDefaultPrinter) ),
                                         (LPARAM)0 );
                        }

                        PrintPrinterChanged(hDlg, ID_SETUP_C_NAME, pPI);

                        CheckRadioButton( hDlg,
                                          ID_SETUP_R_DEFAULT,
                                          ID_SETUP_R_SPECIFIC,
                                          uCmdId);

                        dwStyle = GetWindowLong(hCmb, GWL_STYLE);
                        if (uCmdId == ID_SETUP_R_DEFAULT)
                        {
                            dwStyle &= ~WS_TABSTOP;
                        }
                        else
                        {
                            dwStyle |= WS_TABSTOP;
                            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hCmb, 1L);
                        }
                        SetWindowLong(hCmb, GWL_STYLE, dwStyle);
                    }

                    break;
                }
                case ( ID_BOTH_P_PROPERTIES ) :  // Properties... button
                {
                    PrintChangeProperties(hDlg, ID_SETUP_C_NAME, pPI);

                    break;
                }
                case ( ID_SETUP_P_MORE ) :      // More... button
                {
                    pDM = GlobalLock(pPD->hDevMode);

                    AdvancedDocumentProperties( hDlg,
                                                pPI->hCurPrinter,
                                                (pPI->pCurPrinter)
                                                  ? pPI->pCurPrinter->pPrinterName
                                                  : NULL,
                                                pDM,
                                                pDM );

                    GlobalUnlock(pPD->hDevMode);
                    SendMessage( hDlg,
                                 WM_NEXTDLGCTL,
                                 (WPARAM)GetDlgItem(hDlg, IDOK),
                                 1L );

                    break;
                }
                case ( ID_SETUP_R_PORTRAIT ) :   // Portrait
                case ( ID_SETUP_R_LANDSCAPE ) :  // Landscape
                {
                    if ((pPD->hDevMode) && (pDM = GlobalLock(pPD->hDevMode)))
                    {
                        PrintSetOrientation( hDlg,
                                             pPI,
                                             pDM,
                                             pPI->uiOrientationID,
                                             uCmdId );
                        GlobalUnlock(pPD->hDevMode);
                    }

                    //  FALL THRU ...
                }
                case ( ID_SETUP_R_NONE ) :       // None       (2-Sided)
                case ( ID_SETUP_R_LONG ) :       // Long Side  (2-Sided)
                case ( ID_SETUP_R_SHORT ) :      // Short Side (2-Sided)
                {
                    if ((pPD->hDevMode) && (pDM = GlobalLock(pPD->hDevMode)))
                    {
                        PrintSetDuplex(hDlg, pDM, uCmdId);
                        GlobalUnlock(pPD->hDevMode);
                    }

                    break;
                }
                case ( ID_SETUP_C_SIZE ) :       // Size combobox
                {
                    UINT Orientation;

                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        if ((pPD->hDevMode) && (pDM = GlobalLock(pPD->hDevMode)))
                        {
                        //  pDM->dmFields |= DM_PAPERSIZE;
                            pDM->dmPaperSize =
                                (SHORT)SendMessage( (HWND)lParam,
                                                    CB_GETITEMDATA,
                                                    SendMessage( (HWND)lParam,
                                                                 CB_GETCURSEL,
                                                                 0,
                                                                 0L ),
                                                    0L );

                            Orientation =
                                IsDlgButtonChecked(hDlg, ID_SETUP_R_PORTRAIT)
                                              ? ID_SETUP_R_PORTRAIT
                                              : ID_SETUP_R_LANDSCAPE;
                            PrintSetOrientation( hDlg,
                                                 pPI,
                                                 pDM,
                                                 Orientation,
                                                 Orientation );
                            GlobalUnlock(pPD->hDevMode);
                        }
                    }

                    break;
                }
                case ( ID_SETUP_C_SOURCE ) :       // Source combobox
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        if ((pPD->hDevMode) && (pDM = GlobalLock(pPD->hDevMode)))
                        {
                        //  pDM->dmFields |= DM_DEFAULTSOURCE;
                            pDM->dmDefaultSource =
                                (SHORT)SendMessage( (HWND)lParam,
                                                    CB_GETITEMDATA,
                                                    SendMessage( (HWND)lParam,
                                                                 CB_GETCURSEL,
                                                                 0,
                                                                 0L ),
                                                    0L );

                            GlobalUnlock(pPD->hDevMode);
                        }
                    }

                    break;
                }
                case ( ID_SETUP_E_LEFT ) :       // Left    (Margins)
                case ( ID_SETUP_E_TOP ) :        // Top     (Margins)
                case ( ID_SETUP_E_RIGHT ) :      // Right   (Margins)
                case ( ID_SETUP_E_BOTTOM ) :     // Bottom  (Margins)
                {
                    if (pPI->bKillFocus)
                    {
                        break;
                    }

                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case ( EN_KILLFOCUS ) :
                        {
                            pPI->bKillFocus = TRUE;
                            PrintSetMargin( hDlg,
                                            pPI,
                                            uCmdId,
                                            *((LONG*)&pPI->pPSD->rtMargin +
                                              uCmdId - ID_SETUP_E_LEFT) );
                            pPI->bKillFocus = FALSE;

                            break;
                        }
                        case ( EN_CHANGE ) :
                        {
                            HWND hSample;

                            PrintGetMargin( GET_WM_COMMAND_HWND(wParam, lParam),
                                            pPI,
                                            *((LONG*)&pPI->pPSD->rtMinMargin +
                                              uCmdId - ID_SETUP_E_LEFT),
                                            (LONG*)&pPI->pPSD->rtMargin +
                                              uCmdId - ID_SETUP_E_LEFT,
                                            (LONG*)&pPI->RtMarginMMs +
                                              uCmdId - ID_SETUP_E_LEFT );

                            if (hSample = GetDlgItem(hDlg, ID_SETUP_W_SAMPLE))
                            {
                                RECT rect;

                                GetClientRect(hSample, &rect);
                                InflateRect(&rect, -1, -1);
                                InvalidateRect(hSample, &rect, TRUE);
                            }

                            break;
                        }
                    }

                    break;
                }
                case ( ID_SETUP_P_PRINTER ) :    // Printer... button
                {
                    //
                    //  Save a copy of the original values.
                    //
                    HWND hwndOwner = pPD->hwndOwner;
                    DWORD dwFlags = pPD->Flags;
                    HINSTANCE hInstance = pPD->hInstance;
                    LPCTSTR lpPrintTemplateName = pPD->lpPrintTemplateName;

                    //
                    //  Set up pPI so that PrintDlgX can do all the work.
                    //
                    pPD->hwndOwner = hDlg;
                    pPD->Flags &= ~( PD_ENABLEPRINTTEMPLATEHANDLE |
                                     PD_RETURNIC |
                                     PD_RETURNDC |
                                     PD_PAGESETUP );
                    pPD->Flags |= PD_ENABLEPRINTTEMPLATE;
                    pPD->hInstance = g_hinst;
                    pPD->lpPrintTemplateName = MAKEINTRESOURCE(PRINTDLGORD);
                    pPI->Status |= PI_PRINTDLGX_RECURSE;

                    if (PrintDlgX(pPI))
                    {
                        PrintUpdateSetupDlg( hDlg,
                                             pPI,
                                             GlobalLock(pPD->hDevMode),
                                             TRUE );
                        GlobalUnlock(pPD->hDevMode);
                    }

                    //
                    //  Restore the original values.
                    //
                    pPD->hwndOwner = hwndOwner;
                    pPD->Flags = dwFlags;
                    pPD->hInstance = hInstance;
                    pPD->lpPrintTemplateName = lpPrintTemplateName;
                    pPI->Status &= ~PI_PRINTDLGX_RECURSE;

                    //
                    //  Set the keyboard focus to the OK button.
                    //
                    SendMessage( hDlg,
                                 WM_NEXTDLGCTL,
                                 (WPARAM)GetDlgItem(hDlg, IDOK),
                                 1L );

                    HourGlass(FALSE);

                    break;
                }
                case ( ID_BOTH_P_NETWORK ) :     // Network... button
                {
#ifdef WINNT
                    HANDLE hPrinter;
                    DWORD cbPrinter = 0;
                    PPRINTER_INFO_2 pPrinter = NULL;

                    hPrinter = (HANDLE)ConnectToPrinterDlg(hDlg, 0);
                    if (hPrinter)
                    {
                        if (!GetPrinter( hPrinter,
                                         2,
                                         (LPBYTE)pPrinter,
                                         cbPrinter,
                                         &cbPrinter ))
                        {
                            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                            {
                                if (pPrinter = LocalAlloc(LPTR, cbPrinter))
                                {
                                    if (!GetPrinter( hPrinter,
                                                     2,
                                                     (LPBYTE)pPrinter,
                                                     cbPrinter,
                                                     &cbPrinter ))
                                    {
                                        StoreExtendedError(PDERR_PRINTERNOTFOUND);
                                    }
                                    else
                                    {
                                        SendDlgItemMessage( hDlg,
                                                            ID_SETUP_C_NAME,
                                                            CB_RESETCONTENT,
                                                            0,
                                                            0 );
                                        PrintEnumAndSelect( hDlg,
                                                            ID_SETUP_C_NAME,
                                                            pPI,
                                                            pPrinter->pPrinterName,
                                                            TRUE );
                                    }
                                }
                                else
                                {
                                    StoreExtendedError(CDERR_MEMALLOCFAILURE);
                                }
                            }
                            else
                            {
                                StoreExtendedError(PDERR_SETUPFAILURE);
                            }
                        }

                        if (!GetStoredExtendedError())
                        {
                            SendDlgItemMessage( hDlg,
                                                ID_SETUP_C_NAME,
                                                CB_SETCURSEL,
                                                (WPARAM)SendDlgItemMessage(
                                                      hDlg,
                                                      ID_SETUP_C_NAME,
                                                      CB_FINDSTRING,
                                                      0,
                                                      (LPARAM)pPrinter->pPrinterName ),
                                                (LPARAM)0 );

                            PrintPrinterChanged(hDlg, ID_SETUP_C_NAME, pPI);
                        }

                        LocalFree(pPrinter);
                        ClosePrinter(hPrinter);
                    }
#else
                    WNetConnectionDialog(hDlg, RESOURCETYPE_PRINT);
#endif
                    break;
                }
                case ( ID_BOTH_P_HELP ) :        // Help button
                {
#ifdef UNICODE
                    if (pPI->ApiType == COMDLG_ANSI)
                    {
                        if (msgHELPA && pPD->hwndOwner)
                        {
                            SendMessage( pPD->hwndOwner,
                                         msgHELPA,
                                         (WPARAM)hDlg,
                                         (LPARAM)pPI->pPDA );
                        }
                    }
                    else
#endif
                    {
                        if (msgHELPW && pPD->hwndOwner)
                        {
                            SendMessage( pPD->hwndOwner,
                                         msgHELPW,
                                         (WPARAM)hDlg,
                                         (LPARAM)pPD );
                        }
                    }

                    break;
                }
                case ( IDOK ) :                  // OK button
                {
                    LPPAGESETUPDLG pPSD = pPI->pPSD;
                    int i;

                    if (pPSD)
                    {
                        if ((pPSD->rtMinMargin.left + pPSD->rtMinMargin.right >
                                pPSD->ptPaperSize.x) ||
                            (pPSD->rtMinMargin.top + pPSD->rtMinMargin.bottom >
                                pPSD->ptPaperSize.y))
                        {
                            //
                            //  This is an unprintable case that can happen.
                            //  Let's assume that the driver is at fault
                            //  and accept whatever the user entered.
                            //
                        }
                        else if (pPSD->rtMargin.left + pPSD->rtMargin.right >
                                     pPSD->ptPaperSize.x)
                        {
                            i = (pPSD->rtMargin.left >= pPSD->rtMargin.right)
                                    ? ID_SETUP_E_LEFT
                                    : ID_SETUP_E_RIGHT;
                            PrintEditError(hDlg, i, iszBadMarginError);
                            return (TRUE);
                        }
                        else if (pPSD->rtMargin.top + pPSD->rtMargin.bottom >
                                     pPSD->ptPaperSize.y)
                        {
                            i = (pPSD->rtMargin.top >= pPSD->rtMargin.bottom)
                                    ? ID_SETUP_E_TOP
                                    : ID_SETUP_E_BOTTOM;
                            PrintEditError(hDlg, i, iszBadMarginError);
                            return (TRUE);
                        }
                    }
                    else
                    {
                        HourGlass(TRUE);
                        if (!PrintSetCopies(hDlg, pPI, ID_SETUP_C_NAME))
                        {
                            HourGlass(FALSE);
                            return (TRUE);
                        }
                    }

                    bResult = TRUE;
                    SetFocus( GetDlgItem(hDlg, IDOK) );

                    //  FALL THRU...
                }
                case ( IDCANCEL ) :              // Cancel button
                case ( IDABORT ) :
                {
                    HourGlass(TRUE);

                    if (bResult)
                    {
                        PrintGetSetupInfo(hDlg, pPD);
#ifdef UNICODE
                        if (pPD->Flags & CD_WOWAPP)
                        {
                            UpdateSpoolerInfo(pPI);
                        }
#endif
                    }
                    else
                    {
                        SetFocus( GetDlgItem(hDlg, IDCANCEL) );
                    }
                    pPI->bKillFocus = TRUE;

                    glpfnSetupHook = GETSETUPHOOKFN(pPD);

                    RemoveProp(hDlg, PRNPROP);
                    EndDialog(hDlg, bResult);

                    HourGlass(FALSE);

                    break;
                }
                default :
                {
                    return (FALSE);
                    break;
                }
            }

            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            PrintMeasureItem(hDlg, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_HELP ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)((pPD->Flags & PD_PRINTSETUP)
                                            ? aPrintSetupHelpIDs
                                            : aPageSetupHelpIDs) );
            }
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            if (IsWindowEnabled(hDlg))
            {
                WinHelp( (HWND)wParam,
                         NULL,
                         HELP_CONTEXTMENU,
                         (ULONG_PTR)(LPVOID)((pPD->Flags & PD_PRINTSETUP)
                                            ? aPrintSetupHelpIDs
                                            : aPageSetupHelpIDs) );
            }
            break;
        }
        case ( WM_CTLCOLOREDIT ) :
        {
            if (GetWindowLong((HWND)lParam, GWL_STYLE) & ES_READONLY)
            {
                return ( (BOOL_PTR) SendMessage(hDlg, WM_CTLCOLORDLG, wParam, lParam) );
            }

            //  FALL THRU...
        }
        default :
        {
            return (FALSE);
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintEditMarginProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT PrintEditMarginProc(
    HWND hWnd,
    UINT msg,
    WPARAM wP,
    LPARAM lP)
{
    if ( (msg == WM_CHAR) &&
         (wP != BACKSPACE) &&
         (wP != CTRL_X_CUT) &&
         (wP != CTRL_C_COPY) &&
         (wP != CTRL_V_PASTE) &&
         (wP != (WPARAM)cIntlDecimal) &&
         ((wP < TEXT('0')) || (wP > TEXT('9'))) )
    {
        MessageBeep(0);
        return (FALSE);
    }

    return ( CallWindowProc(lpEditMarginProc, hWnd, msg, wP, lP) );
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintPageSetupPaintProc
//
////////////////////////////////////////////////////////////////////////////

LRESULT PrintPageSetupPaintProc(
    HWND hWnd,
    UINT msg,
    WPARAM wP,
    LPARAM lP)
{
    LRESULT lResult;
    PPRINTINFO pPI;
    LPPAGESETUPDLG pPSD;
    HDC hDC;
    RECT aRt, aRtPage, aRtUser;
    PAINTSTRUCT aPs;
    HGDIOBJ hPen, hBr, hFont, hFontGreek;
    HRGN hRgn;
    TCHAR szGreekText[] = TEXT("Dheevaeilnorpoefdi lfaocr, \nMoiccsriocsnoafrtf \tbnya\nSFlr acnn IF iynnnaepgmaonc\n F&i nyneelglaanm 'Ox' Mnaalgleenyn i&f QCnoamgpeannnyi FI nxca.r\nFSoaynb  Ftrfaonscoirscciom,  \rCoafl idfeopronlieav\ne\n");
    LPTSTR psGreekText;
    int i;


    if (msg != WM_PAINT)
    {
        return ( CallWindowProc(lpStaticProc, hWnd, msg, wP, lP) );
    }

    hDC = BeginPaint(hWnd, &aPs);
    GetClientRect(hWnd, &aRt);
    FillRect(hDC, &aRt, (HBRUSH)GetStockObject(WHITE_BRUSH));
    EndPaint(hWnd, &aPs);
    lResult = 0;

    if ( (!(hDC = GetDC(hWnd))) ||
         (!(pPI = (PPRINTINFO)GetProp(GetParent(hWnd), PRNPROP))) )
    {
        return (0);
    }
    pPSD = pPI->pPSD;

    TransferPD2PSD(pPI);
    aRtPage = aRt;
    hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    hPen = SelectObject(hDC, hPen);

    // Rectangle() does not work here
    MoveToEx( hDC, 0            , 0             , NULL );
    LineTo(   hDC, aRt.right - 1, 0              );
    MoveToEx( hDC, 0            , 1             , NULL );
    LineTo(   hDC, 0            , aRt.bottom - 1 );
    DeleteObject(SelectObject(hDC, hPen));

    // Rectangle() does not work here
    MoveToEx( hDC, aRt.right - 1, 0             , NULL );
    LineTo(   hDC, aRt.right - 1, aRt.bottom - 1 );
    MoveToEx( hDC, 0            , aRt.bottom - 1, NULL );
    LineTo(   hDC, aRt.right    , aRt.bottom - 1 );

    SetBkMode(hDC, TRANSPARENT);
    hPen = (HGDIOBJ)CreatePen(PS_DOT, 1, RGB(128, 128, 128));
    hPen = SelectObject(hDC, hPen);
    hBr  = (HGDIOBJ)GetStockObject(NULL_BRUSH);
    hBr  = SelectObject(hDC, hBr);

    hFont = hFontGreek = CreateFont( pPI->PtMargins.y,
                                     pPI->PtMargins.x,
                                     0,
                                     0,
                                     FW_DONTCARE,
                                     0,
                                     0,
                                     0,
                                     ANSI_CHARSET,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                     VARIABLE_PITCH | FF_SWISS,
                                     NULL );
    hFont = SelectObject(hDC, hFont);

    InflateRect(&aRt, -1, -1);
    aRtUser = aRt;
    hRgn = CreateRectRgnIndirect(&aRtUser);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);

    if (pPSD->lpfnPagePaintHook)
    {
        WORD wFlags;
        LPPAGEPAINTHOOK lpfnPagePaintHook = GETPAGEPAINTHOOKFN(pPSD);

        switch (pPI->dwRotation)
        {
            default :
            {
                //
                //  Portrait mode only.
                //
                wFlags = 0x0000;
                break;
            }
            case ( ROTATE_LEFT ) :
            {
                //
                //  Dot-Matrix (270)
                //
                wFlags = 0x0001;
                break;
            }
            case ( ROTATE_RIGHT ) :
            {
                //
                //  HP PCL (90)
                //
                wFlags = 0x0003;
                break;
            }
        }
        if ( !wFlags ||
             IsDlgButtonChecked(GetParent(hWnd), ID_SETUP_R_PORTRAIT) )
        {
            //
            //  Paper in portrait.
            //
            wFlags |= 0x0004;
        }
        if (pPI->pPD->Flags & PI_WPAPER_ENVELOPE)
        {
            wFlags |= 0x0008;
            if (aRt.right < aRt.bottom)
            {
                //
                //  Envelope in portrait.
                //
                wFlags |= 0x0010;
            }
        }
        if ((*lpfnPagePaintHook)( hWnd,
                                  WM_PSD_PAGESETUPDLG,
                                  MAKELONG(pPI->wPaper, wFlags),
                                  (LPARAM)pPSD ) ||
            (*lpfnPagePaintHook)( hWnd,
                                  WM_PSD_FULLPAGERECT,
                                  (WPARAM)hDC,
                                  (LPARAM)(LPRECT)&aRtUser ))
        {
            goto NoMorePainting;
        }

        aRtUser = aRt;
        aRtUser.left   += aRtUser.right  * pPI->RtMinMarginMMs.left   / pPI->PtPaperSizeMMs.x;
        aRtUser.top    += aRtUser.bottom * pPI->RtMinMarginMMs.top    / pPI->PtPaperSizeMMs.y;
        aRtUser.right  -= aRtUser.right  * pPI->RtMinMarginMMs.right  / pPI->PtPaperSizeMMs.x;
        aRtUser.bottom -= aRtUser.bottom * pPI->RtMinMarginMMs.bottom / pPI->PtPaperSizeMMs.y;

        if ((aRtUser.left   < aRtUser.right)  &&
            (aRtUser.top    < aRtUser.bottom) &&
            (aRtUser.left   > aRtPage.left)   &&
            (aRtUser.top    > aRtPage.top)    &&
            (aRtUser.right  < aRtPage.right)  &&
            (aRtUser.bottom < aRtPage.bottom))
        {
            hRgn = CreateRectRgnIndirect(&aRtUser);
            SelectClipRgn(hDC, hRgn);
            DeleteObject(hRgn);
            if ((*lpfnPagePaintHook)( hWnd,
                                      WM_PSD_MINMARGINRECT,
                                      (WPARAM)hDC,
                                      (LPARAM)(LPRECT)&aRtUser ))
            {
                goto NoMorePainting;
            }
        }
    }

    aRt.left   += aRt.right  * pPI->RtMarginMMs.left   / pPI->PtPaperSizeMMs.x;
    aRt.top    += aRt.bottom * pPI->RtMarginMMs.top    / pPI->PtPaperSizeMMs.y;
    aRt.right  -= aRt.right  * pPI->RtMarginMMs.right  / pPI->PtPaperSizeMMs.x;
    aRt.bottom -= aRt.bottom * pPI->RtMarginMMs.bottom / pPI->PtPaperSizeMMs.y;

    if ( (aRt.left > aRtPage.left) && (aRt.left < aRtPage.right) &&
         (aRt.right < aRtPage.right) && (aRt.right > aRtPage.left) &&
         (aRt.top > aRtPage.top) && (aRt.top < aRtPage.bottom) &&
         (aRt.bottom < aRtPage.bottom) && (aRt.bottom > aRtPage.top) &&
         (aRt.left < aRt.right) &&
         (aRt.top < aRt.bottom) )
    {
        if (pPSD->lpfnPagePaintHook)
        {
            LPPAGEPAINTHOOK lpfnPagePaintHook = GETPAGEPAINTHOOKFN(pPSD);

            aRtUser = aRt;
            hRgn = CreateRectRgnIndirect(&aRtUser);
            SelectClipRgn(hDC, hRgn);
            DeleteObject(hRgn);
            if ((*lpfnPagePaintHook)( hWnd,
                                      WM_PSD_MARGINRECT,
                                      (WPARAM)hDC,
                                      (LPARAM)(LPRECT)&aRtUser ))
            {
                goto SkipMarginRectangle;
            }
        }
        if (!(pPSD->Flags & PSD_DISABLEPAGEPAINTING))
        {
            Rectangle(hDC, aRt.left, aRt.top, aRt.right, aRt.bottom);
        }

SkipMarginRectangle:

        InflateRect(&aRt, -1, -1);
        if (pPSD->lpfnPagePaintHook)
        {
            LPPAGEPAINTHOOK lpfnPagePaintHook = GETPAGEPAINTHOOKFN(pPSD);

            aRtUser = aRt;
            hRgn = CreateRectRgnIndirect(&aRtUser);
            SelectClipRgn(hDC, hRgn);
            DeleteObject(hRgn);
            if ((*lpfnPagePaintHook)( hWnd,
                                      WM_PSD_GREEKTEXTRECT,
                                      (WPARAM)hDC,
                                      (LPARAM)(LPRECT)&aRtUser ))
            {
                goto SkipGreekText;
            }
        }
        if (!(pPSD->Flags & PSD_DISABLEPAGEPAINTING))
        {
            psGreekText = LocalAlloc( LPTR,
                                      10 * (sizeof(szGreekText) + sizeof(TCHAR)) );
            for (i = 0; i < 10; i++)
            {
                CopyMemory( &(psGreekText[i * (sizeof(szGreekText) / sizeof(TCHAR))]),
                            szGreekText,
                            sizeof(szGreekText) );
            }
            aRt.left++;
            aRt.right--;
            aRt.bottom -= (aRt.bottom - aRt.top) % pPI->PtMargins.y;
            hFontGreek = SelectObject(hDC, hFontGreek);
            DrawText( hDC,
                      psGreekText,
                      10 * (sizeof(szGreekText) / sizeof(TCHAR)),
                      &aRt,
                      DT_NOPREFIX | DT_WORDBREAK );
            SelectObject(hDC, hFontGreek);
            LocalFree(psGreekText);
        }
    }

SkipGreekText:

    InflateRect(&aRtPage, -1, -1);
    if (pPI->pPD->Flags & PI_WPAPER_ENVELOPE)
    {
        int iOrientation;

        aRt = aRtPage;
        if (aRt.right < aRt.bottom)     // portrait
        //  switch (pPI->dwRotation)
            {
        //      default :               // no landscape
        //      case ( ROTATE_LEFT ) :  // dot-matrix
        //      {
        //          aRt.left = aRt.right  - 16;
        //          aRt.top  = aRt.bottom - 32;
        //          iOrientation = 2;
        //          break;
        //      }
        //      case ( ROTATE_RIGHT ) : // HP PCL
        //      {
                    aRt.right  = aRt.left + 16;
                    aRt.bottom = aRt.top  + 32;
                    iOrientation = 1;
        //          break;
        //      }
            }
        else                            // landscape
        {
            aRt.left   = aRt.right - 32;
            aRt.bottom = aRt.top   + 16;
            iOrientation = 3;
        }
        hRgn = CreateRectRgnIndirect(&aRt);
        SelectClipRgn(hDC, hRgn);
        DeleteObject(hRgn);
        if (pPSD->lpfnPagePaintHook)
        {
            LPPAGEPAINTHOOK lpfnPagePaintHook = GETPAGEPAINTHOOKFN(pPSD);

            aRtUser = aRt;
            if ((*lpfnPagePaintHook)( hWnd,
                                      WM_PSD_ENVSTAMPRECT,
                                      (WPARAM)hDC,
                                      (LPARAM)(LPRECT)&aRtUser ))
            {
                goto SkipEnvelopeStamp;
            }
        }
        if (!(pPSD->Flags & PSD_DISABLEPAGEPAINTING))
        {
            switch (iOrientation)
            {
                default :          // HP PCL
            //  case ( 1 ) :
                {
                    DrawIcon(hDC, aRt.left, aRt.top, hIconPSStampP);
                    break;
                }
            //  case ( 2 ) :       // dot-matrix
            //  {
            //      DrawIcon(hDC, aRt.left - 16, aRt.top, hIconPSStampP);
            //      break;
            //  }
                case ( 3 ) :       // landscape
                {
                    DrawIcon(hDC, aRt.left, aRt.top, hIconPSStampL);
                    break;
                }
            }
        }
    }

SkipEnvelopeStamp:;

    aRtUser = aRtPage;
    hRgn = CreateRectRgnIndirect(&aRtUser);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);
    if (pPSD->lpfnPagePaintHook)
    {
        LPPAGEPAINTHOOK lpfnPagePaintHook = GETPAGEPAINTHOOKFN(pPSD);

        if ((*lpfnPagePaintHook)( hWnd,
                                  WM_PSD_YAFULLPAGERECT,
                                  (WPARAM)hDC,
                                  (LPARAM)(LPRECT)&aRtUser ))
        {
            goto NoMorePainting;
        }
    }

    //
    //  Draw the envelope lines.
    //
    if ( (!(pPSD->Flags & PSD_DISABLEPAGEPAINTING)) &&
         (pPI->pPD->Flags & PI_WPAPER_ENVELOPE) )
    {
        int iRotation;
        HGDIOBJ hPenBlack;

        aRt = aRtPage;
        if (aRt.right < aRt.bottom)                     // portrait
        {
        //  if (pPI->dwRotation == ROTATE_LEFT )        // dot-matrix
        //      iRotation = 3;
        //  else            // ROTATE_RIGHT             // HP PCL
                iRotation = 2;
        }
        else                                            // landscape
        {
            iRotation = 1;                              // normal
        }

        switch (iRotation)
        {
            default :
        //  case ( 1 ) :      // normal
            {
                aRt.right  = aRt.left + 32;
                aRt.bottom = aRt.top  + 13;
                break;
            }
            case ( 2 ) :      // left
            {
                aRt.right = aRt.left   + 13;
                aRt.top   = aRt.bottom - 32;
                break;
            }
        //  case ( 3 ) :      // right
        //  {
        //      aRt.left   = aRt.right - 13;
        //      aRt.bottom = aRt.top   + 32;
        //      break;
        //  }
        }

        InflateRect(&aRt, -3, -3);
        hPenBlack = SelectObject(hDC, GetStockObject(BLACK_PEN));
        switch (iRotation)
        {
            case ( 1 ) :       // normal
            {
                MoveToEx(hDC, aRt.left , aRt.top    , NULL);
                LineTo(  hDC, aRt.right, aRt.top);
                MoveToEx(hDC, aRt.left , aRt.top + 3, NULL);
                LineTo(  hDC, aRt.right, aRt.top + 3);
                MoveToEx(hDC, aRt.left , aRt.top + 6, NULL);
                LineTo(  hDC, aRt.right, aRt.top + 6);

                break;
            }
        //  case ( 2 ) :       // left
        //  case ( 3 ) :       // right
            default :
            {
                MoveToEx( hDC, aRt.left      , aRt.top       , NULL );
                LineTo(   hDC, aRt.left      , aRt.bottom     );
                MoveToEx( hDC, aRt.left   + 3, aRt.top       , NULL );
                LineTo(   hDC, aRt.left   + 3, aRt.bottom     );
                MoveToEx( hDC, aRt.left   + 6, aRt.top       , NULL );
                LineTo(   hDC, aRt.left   + 6, aRt.bottom     );

                break;
            }
        }
        SelectObject(hDC, hPenBlack);
    }

NoMorePainting:

    DeleteObject(SelectObject(hDC, hPen));
    SelectObject(hDC, hBr);
    DeleteObject(SelectObject(hDC, hFont));
    TransferPSD2PD(pPI);
    ReleaseDC(hWnd, hDC);

    return (lResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintLoadResource
//
//  This routine loads the resource with the given name and type.
//
////////////////////////////////////////////////////////////////////////////

HANDLE PrintLoadResource(
    HANDLE hInst,
    LPTSTR pResName,
    LPTSTR pType)
{
    HANDLE hResInfo, hRes;
    LANGID LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    //
    // If we are loading a resource from ComDlg32 then use the correct LangID. 
    //
    if (hInst == g_hinst) {
        LangID = (LANGID) TlsGetValue(g_tlsLangID);
    }

    if (!(hResInfo = FindResourceEx(hInst, pType, pResName, LangID)))
    {
        StoreExtendedError(CDERR_FINDRESFAILURE);
        return (NULL);
    }

    if (!(hRes = LoadResource(hInst, hResInfo)))
    {
        StoreExtendedError(CDERR_LOADRESFAILURE);
        return (NULL);
    }

    return (hRes);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetDefaultPrinterName
//
//  This routine gets the name of the default printer and stores it
//  in the given buffer.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintGetDefaultPrinterName(
    LPTSTR pDefaultPrinter,
    UINT cchSize)
{
    DWORD dwSize;
    LPTSTR lpsz;


    if (pDefaultPrinter[0] != CHAR_NULL)
    {
        return;
    }

    //
    //  First, try to get the default printername from the win.ini file.
    //
    if (GetProfileString( szTextWindows,
                          szTextDevice,
                          szTextNull,
                          pDefaultPrinter,
                          cchSize ))
    {
        lpsz = pDefaultPrinter;

        while (*lpsz != CHAR_COMMA)
        {
            if (!*lpsz++)
            {
                pDefaultPrinter[0] = CHAR_NULL;
                goto GetDefaultFromRegistry;
            }
        }

        *lpsz = CHAR_NULL;
    }
    else
    {

GetDefaultFromRegistry:

        //
        //  Second, try to get it from the registry.
        //
        dwSize = cchSize * sizeof(TCHAR);

        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          szRegistryPrinter,
                          0,
                          KEY_READ,
                          &hPrinterKey ) == ERROR_SUCCESS)
        {
            RegQueryValueEx( hPrinterKey,
                             szRegistryDefaultValueName,
                             NULL,
                             NULL,
                             (LPBYTE)(pDefaultPrinter),
                             &dwSize );

            RegCloseKey(hPrinterKey);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintReturnDefault
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintReturnDefault(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    LPDEVNAMES pDN;
    LPDEVMODE pDM;


    StoreExtendedError(CDERR_GENERALCODES);

    if (pPD->hDevNames || pPD->hDevMode)
    {
        StoreExtendedError(PDERR_RETDEFFAILURE);
        return (FALSE);
    }

    PrintBuildDevNames(pPI);

    if ((pPD->hDevNames) && (pDN = GlobalLock(pPD->hDevNames)))
    {
#ifdef WINNT
        //
        //  This is not needed in Win95.  An optimization was
        //  added to DocumentProperties that allows the caller to
        //  simply pass in the printer name without the printer
        //  handle.
        //
        LPTSTR pPrinterName;

        pPrinterName = (LPTSTR)pDN + pDN->wDeviceOffset;

        if (pPrinterName[0])
        {
            PrintOpenPrinter(pPI, pPrinterName);
        }

        pPD->hDevMode = PrintGetDevMode( 0,
                                         pPI->hCurPrinter,
                                         pPrinterName,
                                         NULL);
#else
        pPD->hDevMode = PrintGetDevMode( 0,
                                         NULL,
                                         (LPTSTR)pDN + pDN->wDeviceOffset,
                                         NULL );
#endif

        if ((pPD->hDevMode) && (pDM = GlobalLock(pPD->hDevMode)))
        {
            PrintReturnICDC(pPD, pDN, pDM);

            GlobalUnlock(pPD->hDevMode);
            GlobalUnlock(pPD->hDevNames);

            return (TRUE);
        }
        GlobalUnlock(pPD->hDevNames);
        GlobalFree(pPD->hDevNames);
        pPD->hDevNames = NULL;
    }

    StoreExtendedError(PDERR_NODEFAULTPRN);
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitGeneral
//
//  Initialize (enable/disable) dialog elements general to both PrintDlg
//  and SetupDlg.
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintInitGeneral(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    HWND hCtl;


    SetWindowLong( hDlg,
                   GWL_STYLE,
                   GetWindowLong(hDlg, GWL_STYLE) | DS_CONTEXTHELP );

#ifndef WINNT
    if (!lpEditMarginProc)
    {
        WNDCLASSEX wc;

        wc.cbSize = sizeof(wc);

        GetClassInfoEx(NULL, TEXT("edit"), &wc);
        lpEditMarginProc = wc.lpfnWndProc;

        GetClassInfoEx(NULL, TEXT("static"), &wc);
        lpStaticProc = wc.lpfnWndProc;
    }
#endif

    //
    //  LATER: If we don't enumerate here, there will only be ONE item
    //         in the list box.  As a result, we won't catch the
    //         keyboard strokes within the list box (eg. arrow keys,
    //         pgup, pgdown, etc).  Need to subclass the combo boxes
    //         to catch these key strokes so that the printers can be
    //         enumerated.
    //
    if (!PrintEnumAndSelect( hDlg,
                             Id,
                             pPI,
                             (pPI->pCurPrinter)
                               ? pPI->pCurPrinter->pPrinterName
                               : NULL,
                             (!(pPI->Status & PI_PRINTERS_ENUMERATED)) ))
    {
        goto InitGeneral_ConstructFailure;
    }

    PrintUpdateStatus(hDlg, pPI);

    //
    //  See if the Help button should be hidden.
    //
    if (!(pPD->Flags & PD_SHOWHELP))
    {
        if (hCtl = GetDlgItem(hDlg, ID_BOTH_P_HELP))
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);
#ifdef WINNT
            //
            //  Move the window out of this spot so that no overlap
            //  will be detected when adding the network button.
            //
            MoveWindow(hCtl, -8000, -8000, 20, 20, FALSE);
#endif
        }
    }

    return (TRUE);

InitGeneral_ConstructFailure:

    if (!GetStoredExtendedError())
    {
        StoreExtendedError(PDERR_INITFAILURE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitPrintDlg
//
//  Initialize PRINT DLG-specific dialog stuff.
//
//  Returns 0xFFFFFFFF if the dialog should be ended.
//  Otherwise, returns 1/0 (TRUE/FALSE) depending on focus.
//
////////////////////////////////////////////////////////////////////////////

DWORD PrintInitPrintDlg(
    HWND hDlg,
    WPARAM wParam,
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    WORD wCheckID;
    HWND hCtl;


    //
    //  Set the number of copies.
    //
    pPD->nCopies = max(pPD->nCopies, 1);
    pPD->nCopies = min(pPD->nCopies, 9999);
    SetDlgItemInt(hDlg, ID_PRINT_E_COPIES, pPD->nCopies, FALSE);

    if ( !(pPI->pPSD) &&
         (hCtl = GetDlgItem(hDlg, ID_PRINT_E_COPIES)) &&
         (GetWindowLong(hCtl, GWL_STYLE) & WS_VISIBLE) )
    {
        //
        //  "9999" is the maximum value.
        //
        Edit_LimitText(hCtl, COPIES_EDIT_SIZE);

        CreateUpDownControl( WS_CHILD | WS_BORDER | WS_VISIBLE |
                                 UDS_ALIGNRIGHT | UDS_SETBUDDYINT |
                                 UDS_NOTHOUSANDS | UDS_ARROWKEYS,
                             0,
                             0,
                             0,
                             0,
                             hDlg,
                             9999,
                             g_hinst,
                             hCtl,
                             9999,
                             1,
                             pPD->nCopies );

        //
        // Adjust the width of the copies edit control using the current
        // font and the scroll bar width.  This is necessary to handle the 
        // the up down control from encroching on the space in the edit
        // control when we are in High Contrast (extra large) mode.
        //
        SetCopiesEditWidth(hDlg, hCtl);
    }

    if (!PrintInitBannerAndQuality(hDlg, pPI, pPD))
    {
        if (!GetStoredExtendedError())
        {
            StoreExtendedError(PDERR_INITFAILURE);
        }
        return (0xFFFFFFFF);
    }

#ifdef WINNT
    if (!(pPD->Flags & PD_SHOWHELP))
    {
        if (hCtl = GetDlgItem(hDlg, ID_BOTH_P_HELP))
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);

            //
            //  Move the window out of this spot so that no overlap
            //  will be detected when adding the network button.
            //
            MoveWindow(hCtl, -8000, -8000, 20, 20, FALSE);
        }
    }
#endif

    if (hCtl = GetDlgItem(hDlg, ID_PRINT_X_TOFILE))
    {
        if (pPD->Flags & PD_PRINTTOFILE)
        {
            CheckDlgButton(hDlg, ID_PRINT_X_TOFILE, TRUE);
        }

        if (pPD->Flags & PD_HIDEPRINTTOFILE)
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);
        }
        else if (pPD->Flags & PD_DISABLEPRINTTOFILE)
        {
            EnableWindow(hCtl, FALSE);
        }
    }

    if (pPD->Flags & PD_NOPAGENUMS)
    {
        EnableWindow(GetDlgItem(hDlg, ID_PRINT_R_PAGES), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_FROM), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_FROM), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_TO), FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_TO), FALSE);

        //
        //  Don't allow disabled button checked.
        //
        pPD->Flags &= ~((DWORD)PD_PAGENUMS);
    }
    else
    {
        //
        //  Some apps (marked 3.1) do not pass in valid ranges.
        //      (e.g. Corel Ventura)
        //
        if ((pPI->ProcessVersion < 0x40000) || (!(pPD->Flags & PD_PAGENUMS)))
        {
            if (pPD->nFromPage != 0xFFFF)
            {
                if (pPD->nFromPage < pPD->nMinPage)
                {
                    pPD->nFromPage = pPD->nMinPage;
                }
                else if (pPD->nFromPage > pPD->nMaxPage)
                {
                    pPD->nFromPage = pPD->nMaxPage;
                }
            }
            if (pPD->nToPage != 0xFFFF)
            {
                if (pPD->nToPage < pPD->nMinPage)
                {
                    pPD->nToPage = pPD->nMinPage;
                }
                else if (pPD->nToPage > pPD->nMaxPage)
                {
                    pPD->nToPage = pPD->nMaxPage;
                }
            }
        }

        if ( pPD->nMinPage > pPD->nMaxPage ||
             ( pPD->nFromPage != 0xFFFF &&
               ( pPD->nFromPage < pPD->nMinPage ||
                 pPD->nFromPage > pPD->nMaxPage ) ) ||
             ( pPD->nToPage != 0xFFFF &&
               ( pPD->nToPage < pPD->nMinPage ||
                 pPD->nToPage > pPD->nMaxPage ) ) )
        {
            StoreExtendedError(PDERR_INITFAILURE);
            return (0xFFFFFFFF);
        }

        if (pPD->nFromPage != 0xFFFF)
        {
            SetDlgItemInt(hDlg, ID_PRINT_E_FROM, pPD->nFromPage, FALSE);
            if (pPD->nToPage != 0xFFFF)
            {
                SetDlgItemInt(hDlg, ID_PRINT_E_TO, pPD->nToPage, FALSE);
            }
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_TO), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_TO), FALSE);
        }

        if (pPD->nMinPage == pPD->nMaxPage)
        {
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_R_PAGES), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_FROM), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_FROM), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_S_TO), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_E_TO), FALSE);

            //
            //  Don't allow disabled button checked.
            //
            pPD->Flags &= ~((DWORD)(PD_PAGENUMS | PD_COLLATE));
            pPI->Status &= ~PI_COLLATE_REQUESTED;
            EnableWindow(GetDlgItem(hDlg, ID_PRINT_X_COLLATE), FALSE);
            ShowWindow(GetDlgItem(hDlg, ID_PRINT_X_COLLATE), SW_HIDE);
        }
#ifndef WINNT
        //
        //  This is NOT desirable.  Most apps use a very high number for
        //  their max page, so it looks very strange.
        //
        else
        {
            TCHAR szAll[32];
            TCHAR szBuf[64];

            if (pPD->nMinPage != pPD->nMaxPage)
            {
                if (pPD->nMaxPage != 0xFFFF)
                {
                    if (CDLoadString(g_hinst, iszPrintRangeAll, szAll, 32)) 
                    {
                        wsprintf( szBuf,
                                  szAll,
                                  (DWORD)pPD->nMaxPage - (DWORD)pPD->nMinPage + 1 );
                        SetDlgItemText(hDlg, ID_PRINT_R_ALL, szBuf);
                    }
                    else
                    {
                        StoreExtendedError(CDERR_LOADSTRFAILURE);
                        return (0xFFFFFFFF);
                    }
                }
            }
        }
#endif
    }

    if (pPD->Flags & PD_NOSELECTION)
    {
        HWND hRad = GetDlgItem(hDlg, ID_PRINT_R_SELECTION);

        if (hRad)
        {
            EnableWindow(hRad, FALSE);
        }

        //
        //  Don't allow disabled button checked.
        //
        pPD->Flags &= ~((DWORD)PD_SELECTION);
    }

    if (pPD->Flags & PD_PAGENUMS)
    {
        wCheckID = ID_PRINT_R_PAGES;
    }
    else if (pPD->Flags & PD_SELECTION)
    {
        wCheckID = ID_PRINT_R_SELECTION;
    }
    else
    {
        // PD_ALL
        wCheckID = ID_PRINT_R_ALL;
    }

    CheckRadioButton(hDlg, ID_PRINT_R_ALL, ID_PRINT_R_PAGES, (int)wCheckID);

    //
    //  Subclass the integer only edit controls.
    //
    if (!(pPI->pPSD))
    {
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_E_FROM))
        {
            //
            //  "99999" is the maximum value.
            //
            Edit_LimitText(hCtl, PAGE_EDIT_SIZE);
        }
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_E_TO))
        {
            //
            //  "99999" is the maximum value.
            //
            Edit_LimitText(hCtl, PAGE_EDIT_SIZE);
        }
    }

    if (pPD->lpfnPrintHook)
    {
        LPPRINTHOOKPROC lpfnPrintHook = GETPRINTHOOKFN(pPD);

#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            DWORD dwHookRet;

            ThunkPrintDlgW2A(pPI);
            dwHookRet = (*lpfnPrintHook)( hDlg,
                                          WM_INITDIALOG,
                                          wParam,
                                          (LONG_PTR)pPI->pPDA ) != 0;
            if (dwHookRet)
            {
                ThunkPrintDlgA2W(pPI);
            }

            return (dwHookRet);
        }
        else
#endif
        {
            return ( (*lpfnPrintHook)( hDlg,
                                       WM_INITDIALOG,
                                       wParam,
                                       (LONG_PTR)pPD ) ) != 0;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitSetupDlg
//
//  Initialize SETUP-specific dialog stuff.
//
//  Returns 0xFFFFFFFF if the dialog should be ended.
//  Otherwise, returns 1/0 (TRUE/FALSE) depending on focus.
//
////////////////////////////////////////////////////////////////////////////

DWORD PrintInitSetupDlg(
    HWND hDlg,
    WPARAM wParam,
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    LPDEVMODE pDM = NULL;
    HWND hCtl;
    LPPAGESETUPDLG pPSD = pPI->pPSD;
    UINT Orientation;


    if (!pPD->hDevMode ||
        !(pDM = GlobalLock(pPD->hDevMode)))
    {
        StoreExtendedError(CDERR_MEMLOCKFAILURE);
        goto InitSetupDlg_ConstructFailure;
    }

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SIZE))
    {
        PrintInitPaperCombo( pPI,
                             hCtl,
                             GetDlgItem(hDlg, ID_SETUP_S_SIZE),
                             pPI->pCurPrinter,
                             pDM,
                             DC_PAPERNAMES,
                             CCHPAPERNAME,
                             DC_PAPERS );
    }

    //
    //  Provide backward compatibility for old-style-template sources
    //  ID_SETUP_C_SOURCE.
    //
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SOURCE))
    {
        PrintInitPaperCombo( pPI,
                             hCtl,
                             GetDlgItem(hDlg, ID_SETUP_S_SOURCE),
                             pPI->pCurPrinter,
                             pDM,
                             DC_BINNAMES,
                             CCHBINNAME,
                             DC_BINS );
    }

    //
    //  Set the edit field lengths and other setup stuff for margins.
    //  This must be called before PrintSetMargin, which is called in
    //  PrintSetOrientation.
    //
    PrintSetupMargins(hDlg, pPI);

    PrintInitOrientation(hDlg, pPI, pDM);
    Orientation = pDM->dmOrientation + ID_SETUP_R_PORTRAIT - DMORIENT_PORTRAIT;
    PrintSetOrientation( hDlg,
                         pPI,
                         pDM,
                         Orientation,
                         Orientation );

    PrintInitDuplex(hDlg, pDM);
    PrintSetDuplex( hDlg,
                    pDM,
                    pDM->dmDuplex + ID_SETUP_R_NONE - DMDUP_SIMPLEX );

    GlobalUnlock(pPD->hDevMode);

    if (pPSD)
    {
        if (pPSD->Flags & PSD_DISABLEORIENTATION)
        {
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_R_PORTRAIT), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_R_LANDSCAPE), FALSE );
        }
        if (pPSD->Flags & PSD_DISABLEPAPER)
        {
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_SIZE), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_C_SIZE), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_SOURCE), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_C_SOURCE), FALSE );
        }
        if (pPSD->Flags & PSD_DISABLEMARGINS)
        {
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_LEFT), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_E_LEFT),  FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_RIGHT), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_E_RIGHT),  FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_TOP), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_E_TOP),  FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_S_BOTTOM), FALSE );
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_E_BOTTOM),  FALSE );
        }
        if (pPSD->Flags & PSD_DISABLEPRINTER)
        {
            EnableWindow(GetDlgItem(hDlg, ID_SETUP_P_PRINTER), FALSE );
        }
    }

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_W_SAMPLE))
    {
        lpStaticProc = (WNDPROC)SetWindowLongPtr( hCtl,
                                               GWLP_WNDPROC,
                                               (LONG_PTR)PrintPageSetupPaintProc );
    }

    if ((pPD->Flags & PD_NONETWORKBUTTON))
    {
        if (hCtl = GetDlgItem(hDlg, ID_BOTH_P_NETWORK))
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);
        }
    }
    else if (!(pPI->pPSD))
    {
#ifdef WINNT
        AddNetButton( hDlg,
                      ((pPD->Flags & PD_ENABLESETUPTEMPLATE)
                          ? pPD->hInstance : g_hinst),
                      FILE_BOTTOM_MARGIN,
                      (pPD->Flags & (PD_ENABLESETUPTEMPLATE |
                                     PD_ENABLESETUPTEMPLATEHANDLE))
                          ? FALSE : TRUE,
                      FALSE,
                      TRUE );
#endif
        //
        //  The button can be added in two ways -
        //      statically (they have it predefined in their template) and
        //      dynamically (successful call to AddNetButton).
        //
#ifdef WINNT
        if (!IsNetworkInstalled())
#else
        if (!GetSystemMetrics(SM_NETWORK))
#endif
        {
            hCtl = GetDlgItem(hDlg, ID_BOTH_P_NETWORK);

            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);
        }
    }

#ifdef WINNT
    if (!(pPD->Flags & PD_SHOWHELP))
    {
        if (hCtl = GetDlgItem(hDlg, ID_BOTH_P_HELP))
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);

            //
            //  Move the window out of this spot so that no overlap
            //  will be detected when adding the network button.
            //
            MoveWindow(hCtl, -8000, -8000, 20, 20, FALSE);
        }
    }
#endif

    //
    //  Provide backward compatibility for old-style-template radio buttons.
    //
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_DEFAULT))
    {
        TCHAR szBuf[MAX_DEV_SECT];
        TCHAR szDefFormat[MAX_DEV_SECT];

        if (pPI->szDefaultPrinter[0])
        {
            if (!CDLoadString( g_hinst,
                             iszDefCurOn,
                             szDefFormat,
                             MAX_DEV_SECT ))
            {
                StoreExtendedError(CDERR_LOADSTRFAILURE);
                goto InitSetupDlg_ConstructFailure;
            }

            wsprintf(szBuf, szDefFormat, pPI->szDefaultPrinter);
        }
        else
        {
            szBuf[0] = CHAR_NULL;
            EnableWindow(hCtl, FALSE);
        }
        SetDlgItemText(hDlg, ID_SETUP_S_DEFAULT, szBuf);

        if ( pPI->pCurPrinter &&
             pPI->pCurPrinter->pPrinterName &&
             !lstrcmp(pPI->pCurPrinter->pPrinterName, pPI->szDefaultPrinter) )
        {
            CheckRadioButton( hDlg,
                              ID_SETUP_R_DEFAULT,
                              ID_SETUP_R_SPECIFIC,
                              ID_SETUP_R_DEFAULT );
        }
        else
        {
            CheckRadioButton( hDlg,
                              ID_SETUP_R_DEFAULT,
                              ID_SETUP_R_SPECIFIC,
                              ID_SETUP_R_SPECIFIC );
        }
    }

    if (pPD->lpfnSetupHook)
    {
        DWORD dwHookRet;
        LPSETUPHOOKPROC lpfnSetupHook = GETSETUPHOOKFN(pPD);

#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgW2A(pPI);
            TransferPDA2PSD(pPI);

            pPI->NestCtr++;
            dwHookRet = (*lpfnSetupHook)( hDlg,
                                          WM_INITDIALOG,
                                          wParam,
                                          (pPI->pPSD)
                                              ? (LONG_PTR)pPI->pPSD
                                              : (LONG_PTR)pPI->pPDA ) != 0;
            pPI->NestCtr--;

            if (dwHookRet)
            {
                TransferPSD2PDA(pPI);
                ThunkPrintDlgA2W(pPI);
                if (pPI->NestCtr == 0)
                {
                    TransferPD2PSD(pPI);
                }
            }
        }
        else
#endif
        {
            TransferPD2PSD(pPI);
            dwHookRet = (*lpfnSetupHook)( hDlg,
                                          WM_INITDIALOG,
                                          wParam,
                                          (pPI->pPSD)
                                              ? (LONG_PTR)pPI->pPSD
                                              : (LONG_PTR)pPD ) != 0;
            TransferPSD2PD(pPI);
        }


        return (dwHookRet);
    }

    return (TRUE);

InitSetupDlg_ConstructFailure:

    if (!GetStoredExtendedError())
    {
        StoreExtendedError(PDERR_INITFAILURE);
    }

    return (0xFFFFFFFF);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintUpdateSetupDlg
//
//  Update the print setup and page setup dialogs with the new settings.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintUpdateSetupDlg(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    BOOL fResetContent)
{
    HWND hCtl;
    UINT Count;
    UINT Orientation = 0;


    //
    //  Update the Size combo box.
    //
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SIZE))
    {
        if (fResetContent)
        {
            PrintInitPaperCombo( pPI,
                                 hCtl,
                                 GetDlgItem(hDlg, ID_SETUP_S_SIZE),
                                 pPI->pCurPrinter,
                                 pDM,
                                 DC_PAPERNAMES,
                                 CCHPAPERNAME,
                                 DC_PAPERS );
            //
            //  PrintInitPaperCombo will turn off the hour glass cursor, so
            //  turn it back on.
            //
            HourGlass(TRUE);
        }
        else
        {
            Count = (UINT) SendMessage(hCtl, CB_GETCOUNT, 0, 0);
            while (Count != 0)
            {
                Count--;
                if (pDM->dmPaperSize == (SHORT)SendMessage( hCtl,
                                                            CB_GETITEMDATA,
                                                            Count,
                                                            0 ) )
                {
                    break;
                }
            }

            SendMessage( hCtl,
                         CB_SETCURSEL,
                         Count,
                         0 );
        }
    }

    //
    //  Update the Source combo box.
    //
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SOURCE))
    {
        if (fResetContent)
        {
            PrintInitPaperCombo( pPI,
                                 hCtl,
                                 GetDlgItem(hDlg, ID_SETUP_S_SOURCE),
                                 pPI->pCurPrinter,
                                 pDM,
                                 DC_BINNAMES,
                                 CCHBINNAME,
                                 DC_BINS );
            //
            //  PrintInitPaperCombo will turn off the hour glass cursor, so
            //  turn it back on.
            //
            HourGlass(TRUE);
        }
        else
        {
            Count = (UINT) SendMessage(hCtl, CB_GETCOUNT, 0, 0);
            while (Count != 0)
            {
                Count--;
                if (pDM->dmDefaultSource == (SHORT)SendMessage( hCtl,
                                                                CB_GETITEMDATA,
                                                                Count,
                                                                0 ) )
                {
                    break;
                }
            }

            SendMessage( hCtl,
                         CB_SETCURSEL,
                         Count,
                         0 );
        }
    }

    //
    //  Update the Orientation radio buttons.
    //
    if (GetDlgItem(hDlg, ID_SETUP_R_PORTRAIT))
    {
        Orientation = pDM->dmOrientation + ID_SETUP_R_PORTRAIT - DMORIENT_PORTRAIT;
        PrintSetOrientation( hDlg,
                             pPI,
                             pDM,
                             IsDlgButtonChecked(hDlg, ID_SETUP_R_PORTRAIT)
                                 ? ID_SETUP_R_PORTRAIT
                                 : ID_SETUP_R_LANDSCAPE,
                             Orientation );
    }

    //
    //  Update the Duplex radio buttons.
    //
    if (GetDlgItem(hDlg, ID_SETUP_R_NONE))
    {
        PrintSetDuplex( hDlg,
                        pDM,
                        pDM->dmDuplex + ID_SETUP_R_NONE - DMDUP_SIMPLEX );
    }

    //
    //  Update the page setup sample picture.
    //
    if ((Orientation == 0) && (hCtl = GetDlgItem(hDlg, ID_SETUP_W_SAMPLE)))
    {
        Orientation = pDM->dmOrientation + ID_SETUP_R_PORTRAIT - DMORIENT_PORTRAIT;
        PrintUpdatePageSetup( hDlg,
                              pPI,
                              pDM,
                              0,
                              Orientation );
    }

    //
    //  Update the Default/Specific Printer radio buttons.
    //
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_DEFAULT))
    {
        if ( pPI->pCurPrinter &&
             pPI->pCurPrinter->pPrinterName &&
             !lstrcmp(pPI->pCurPrinter->pPrinterName, pPI->szDefaultPrinter) )
        {
            CheckRadioButton( hDlg,
                              ID_SETUP_R_DEFAULT,
                              ID_SETUP_R_SPECIFIC,
                              ID_SETUP_R_DEFAULT );
        }
        else
        {
            CheckRadioButton( hDlg,
                              ID_SETUP_R_DEFAULT,
                              ID_SETUP_R_SPECIFIC,
                              ID_SETUP_R_SPECIFIC );
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetCopies
//
//  Sets the appropriate number of copies in the PrintDlg structure and
//  in the DevMode structure.
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintSetCopies(
    HWND hDlg,
    PPRINTINFO pPI,
    UINT Id)
{
    LPPRINTDLG pPD = pPI->pPD;
    LPDEVMODE pDM;
    WORD sMaxCopies;


    if ( (pPD->hDevMode) &&
         (pDM = GlobalLock(pPD->hDevMode)) )
    {
#ifdef UNICODE
        //
        //  If we're coming from a WOW app, we need to only set
        //  the copies in the devmode if the PD_USEDEVMODECOPIES
        //  flag is set.
        //
        if (IS16BITWOWAPP(pPD))
        {
            if (pPD->Flags & PD_USEDEVMODECOPIES)
            {
                pDM->dmCopies = pPD->nCopies;
                pPD->nCopies = 1;
            }
            else
            {
                pDM->dmCopies = 1;
            }

            return (TRUE);
        }
#endif
        if ( (!(pDM->dmFields & DM_COPIES)) ||
             ((!(pPI->pPSD)) &&
              (pPI->ProcessVersion < 0x40000) &&
              (!(pPD->Flags & PD_USEDEVMODECOPIES))) )
        {
LeaveInfoInPD:
            //
            //  The driver cannot do copies, so leave the
            //  copy/collate info in the pPD.
            //
            pDM->dmCopies = 1;
            SetField(pDM, dmCollate, DMCOLLATE_FALSE);
        }
        else if ( (pDM->dmSpecVersion < 0x0400) ||
                  (!(pDM->dmFields & DM_COLLATE)) )
        {
            //
            //  The driver can do copies, but not collate.
            //  Where the info goes depends on the PD_COLLATE flag.
            //
            if (pPD->Flags & PD_COLLATE)
            {
                goto LeaveInfoInPD;
            }
            else
            {
                goto PutInfoInDevMode;
            }
        }
        else
        {
PutInfoInDevMode:
            //
            //  Make sure we have a current printer.
            //
            if (!pPI->pCurPrinter)
            {
                goto LeaveInfoInPD;
            }

            //
            //  Make sure the driver can support the number
            //  of copies requested.
            //
            sMaxCopies = (WORD)DeviceCapabilities(
                                     pPI->pCurPrinter->pPrinterName,
                                     pPI->pCurPrinter->pPortName,
                                     DC_COPIES,
                                     NULL,
                                     NULL );
            if ((sMaxCopies < 1) || (sMaxCopies == (WORD)(-1)))
            {
                sMaxCopies = 1;
            }
            if (sMaxCopies < pPD->nCopies)
            {
                if (pPD->Flags & PD_USEDEVMODECOPIES)
                {
                    PrintEditError( hDlg,
                                    (Id == ID_PRINT_C_NAME)
                                        ? ID_PRINT_E_COPIES
                                        : ID_BOTH_P_PROPERTIES,
                                    iszTooManyCopies,
                                    sMaxCopies );

                    GlobalUnlock(pPD->hDevMode);
                    return (FALSE);
                }

                goto LeaveInfoInPD;
            }

            //
            //  The driver can do both copies and collate,
            //  so move the info to the devmode.
            //
            pDM->dmCopies = pPD->nCopies;
            SetField( pDM,
                      dmCollate,
                      (pPD->Flags & PD_COLLATE)
                          ? DMCOLLATE_TRUE
                          : DMCOLLATE_FALSE );
            pPD->nCopies = 1;
            pPD->Flags &= ~PD_COLLATE;
        }

        GlobalUnlock(pPD->hDevMode);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetMinMargins
//
////////////////////////////////////////////////////////////////////////////

VOID PrintSetMinMargins(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM)
{
    LPPAGESETUPDLG pPSD = pPI->pPSD;
    HDC hDC;
    RECT rtMinMargin;


    if (!pPSD)
    {
        return;
    }

    if (pPSD->Flags & PSD_MINMARGINS)
    {
        //
        //  Convert passed in margins to 10th of MMs.
        //
        if (pPSD->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
        {
            pPI->RtMinMarginMMs.left   = pPSD->rtMinMargin.left / 10;
            pPI->RtMinMarginMMs.top    = pPSD->rtMinMargin.top / 10;
            pPI->RtMinMarginMMs.right  = pPSD->rtMinMargin.right / 10;
            pPI->RtMinMarginMMs.bottom = pPSD->rtMinMargin.bottom / 10;
        }
        else           // PSD_INTHOUSANDTHSOFINCHES
        {
            pPI->RtMinMarginMMs.left   = pPSD->rtMinMargin.left * MMS_PER_INCH / 100;
            pPI->RtMinMarginMMs.top    = pPSD->rtMinMargin.top * MMS_PER_INCH / 100;
            pPI->RtMinMarginMMs.right  = pPSD->rtMinMargin.right * MMS_PER_INCH / 100;
            pPI->RtMinMarginMMs.bottom = pPSD->rtMinMargin.bottom * MMS_PER_INCH / 100;
        }
    }
    else
    {
        //
        //  Default to no minimum if we can't get the info.
        //
        pPI->RtMinMarginMMs.left   = 0;
        pPI->RtMinMarginMMs.top    = 0;
        pPI->RtMinMarginMMs.right  = 0;
        pPI->RtMinMarginMMs.bottom = 0;
        pPSD->rtMinMargin.left   = 0;
        pPSD->rtMinMargin.top    = 0;
        pPSD->rtMinMargin.right  = 0;
        pPSD->rtMinMargin.bottom = 0;

        //
        //  Calculate new min margins from driver.
        //
        if (hDC = CreateIC(NULL, pDM->dmDeviceName, NULL, pDM))
        {
            //
            //  These are in PIXELS.
            //
            int nPageWidth = GetDeviceCaps(hDC, PHYSICALWIDTH);
            int nPageHeight = GetDeviceCaps(hDC, PHYSICALHEIGHT);
            int nPrintWidth = GetDeviceCaps(hDC, HORZRES);
            int nPrintHeight = GetDeviceCaps(hDC, VERTRES);
            int nOffsetWidth = GetDeviceCaps(hDC, PHYSICALOFFSETX);
            int nOffsetHeight = GetDeviceCaps(hDC, PHYSICALOFFSETY);
            int nPerInchWidth = GetDeviceCaps(hDC, LOGPIXELSX);
            int nPerInchHeight = GetDeviceCaps(hDC, LOGPIXELSY);

            //
            //  Calculate min margins in PIXELS.
            //
            rtMinMargin.left   = nOffsetWidth;
            rtMinMargin.top    = nOffsetHeight;
            rtMinMargin.right  = nPageWidth - nPrintWidth - nOffsetWidth;
            rtMinMargin.bottom = nPageHeight - nPrintHeight - nOffsetHeight;

            //
            //  Convert to 10ths of MMs.
            //
            if (nPerInchWidth && nPerInchHeight)
            {
                pPI->RtMinMarginMMs.left   = rtMinMargin.left * MMS_PER_INCH / nPerInchWidth / 10;
                pPI->RtMinMarginMMs.top    = rtMinMargin.top * MMS_PER_INCH / nPerInchHeight / 10;
                pPI->RtMinMarginMMs.right  = rtMinMargin.right * MMS_PER_INCH / nPerInchHeight / 10;
                pPI->RtMinMarginMMs.bottom = rtMinMargin.bottom * MMS_PER_INCH / nPerInchHeight / 10;
            }

            if (pPSD->Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
            {
                //
                //  Convert to 100ths of MMs.
                //
                pPSD->rtMinMargin.left   = pPI->RtMinMarginMMs.left / 10;
                pPSD->rtMinMargin.top    = pPI->RtMinMarginMMs.top / 10;
                pPSD->rtMinMargin.right  = pPI->RtMinMarginMMs.right / 10;
                pPSD->rtMinMargin.bottom = pPI->RtMinMarginMMs.bottom / 10;
            }
            else           // PSD_INTHOUSANDTHSOFINCHES
            {
                //
                //  Convert to 1000ths of inches.
                //
                if (nPerInchWidth && nPerInchHeight)
                {
                    pPSD->rtMinMargin.left   = rtMinMargin.left * 1000 / nPerInchWidth;
                    pPSD->rtMinMargin.top    = rtMinMargin.top * 1000 / nPerInchHeight;
                    pPSD->rtMinMargin.right  = rtMinMargin.right * 1000 / nPerInchHeight;
                    pPSD->rtMinMargin.bottom = rtMinMargin.bottom * 1000 / nPerInchHeight;
                }
            }

            DeleteDC(hDC);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetupMargins
//
////////////////////////////////////////////////////////////////////////////

VOID PrintSetupMargins(
    HWND hDlg,
    PPRINTINFO pPI)
{
    TCHAR szMars[32];
    TCHAR szText[16];
    int ids[4] = { ID_SETUP_E_LEFT,
                   ID_SETUP_E_TOP,
                   ID_SETUP_E_RIGHT,
                   ID_SETUP_E_BOTTOM };
    int i;
    HWND hEdt;


    //
    //  Margins are only available from the PageSetupDlg.
    //
    if (!(pPI->pPSD))
    {
        return;
    }

    for (i = 0; i < 4; i++)
    {
        if (hEdt = GetDlgItem(hDlg, ids[i]))
        {
            //
            //  "999999" is the maximum value.
            //
            SendMessage(hEdt, EM_LIMITTEXT, MARGIN_EDIT_SIZE, 0);

            lpEditMarginProc =
                (WNDPROC)SetWindowLongPtr( hEdt,
                                        GWLP_WNDPROC,
                                        (LONG_PTR)PrintEditMarginProc );

        }
    }

    if (!GetLocaleInfo( LOCALE_USER_DEFAULT,
                        LOCALE_SDECIMAL,
                        szText,
                        16 ))
    {
        cIntlDecimal = CHAR_DOT;
    }
    else
    {
        cIntlDecimal = szText[0];
    }

    switch (pPI->pPSD->Flags & (PSD_INTHOUSANDTHSOFINCHES |
                                PSD_INHUNDREDTHSOFMILLIMETERS))
    {
        case ( PSD_INHUNDREDTHSOFMILLIMETERS ) :
        {
            CDLoadString(g_hinst, iszMarginsMillimeters, szMars, 32);
            CDLoadString(g_hinst, iszMillimeters, cIntlMeasure, 5);

            break;
        }
        case ( PSD_INTHOUSANDTHSOFINCHES ) :
        {
            CDLoadString(g_hinst, iszMarginsInches, szMars, 32);
            CDLoadString(g_hinst, iszInches, cIntlMeasure, 5);

            break;
        }
    }

    cchIntlMeasure = lstrlen(cIntlMeasure);

    SetWindowText(GetDlgItem(hDlg, ID_SETUP_G_MARGINS), szMars);
    pPI->PtMargins.x = 2 * (IS_KEY_PRESSED(pPI->PtMargins.x / 4) &&
                            IS_KEY_PRESSED(pPI->PtMargins.y / 4)
                                ? sizeof(WCHAR)
                                : sizeof(CHAR));
    pPI->PtMargins.y = 2 * pPI->PtMargins.x;
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetMargin
//
////////////////////////////////////////////////////////////////////////////

VOID PrintSetMargin(
    HWND hDlg,
    PPRINTINFO pPI,
    UINT Id,
    LONG lValue)
{
    HWND hEdt;
    TCHAR szText[32];
    TCHAR szILZero[2];
    LONG lFract;


    if (hEdt = GetDlgItem(hDlg, Id))
    {
        switch (pPI->pPSD->Flags & (PSD_INTHOUSANDTHSOFINCHES |
                                    PSD_INHUNDREDTHSOFMILLIMETERS))
        {
            case ( PSD_INHUNDREDTHSOFMILLIMETERS ) :
            {
                lFract = lValue % 100;
                wsprintf( szText,
                          lFract ? TEXT("%lu%c%02lu") : TEXT("%lu"),
                          lValue / 100,
                          cIntlDecimal,
                          lFract );
                break;
            }
            case ( PSD_INTHOUSANDTHSOFINCHES ) :
            {
                lFract = lValue % 1000;
                wsprintf( szText,
                          lFract ? TEXT("%lu%c%03lu") : TEXT("%lu"),
                          lValue / 1000,
                          cIntlDecimal,
                          lFract );
                break;
            }
        }

        //
        //  Remove trailing zeros off of fraction.
        //
        if (lFract)
        {
            LPTSTR pStr = szText + lstrlen(szText) - 1;

            while (*pStr == TEXT('0'))
            {
                *pStr-- = TEXT('\0');
            }
        }

        //
        //  Determine if a leading zero is to be used and write the
        //  text to the edit window.
        //
        if (!GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szILZero, 2))
        {
            szILZero[0] = TEXT('0');
        }
        SetWindowText( hEdt,
                       szText + (szText[0] == TEXT('0') &&
                                 szText[1] == cIntlDecimal &&
                                 szILZero[0] == TEXT('0')) );
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetMargin
//
////////////////////////////////////////////////////////////////////////////

VOID PrintGetMargin(
    HWND hEdt,
    PPRINTINFO pPI,
    LONG lMin,
    LONG *plMargin,
    LONG *plSample)
{
    TCHAR szText[16];
    TCHAR *pText;
    TCHAR *pFrac;

    GetWindowText(hEdt, szText, 16);
    *plMargin = ConvertStringToInteger(szText);

    for (pText = szText; *pText;)
    {
        if (*pText++ == cIntlDecimal)
        {
            break;
        }
    }

    for (pFrac = pText; *pFrac; pFrac++)
    {
        if (*pFrac == cIntlMeasure[0])
        {
            *pFrac = CHAR_NULL;
            break;
        }

        if (*pFrac == cIntlDecimal)
        {
            *pFrac = CHAR_NULL;
            break;
        }
    }
    lstrcat(pText, TEXT("000"));

    switch (pPI->pPSD->Flags & (PSD_INTHOUSANDTHSOFINCHES |
                                PSD_INHUNDREDTHSOFMILLIMETERS))
    {
        case ( PSD_INTHOUSANDTHSOFINCHES ) :
        {
            //
            //  In 1000ths of inches.
            //
            *plMargin *= 1000;
            pText[3] = CHAR_NULL;
            *plMargin += ConvertStringToInteger(pText);
            *plMargin = max(lMin, *plMargin);

            //
            //  In 10ths of MMs.
            //
            *plSample = *plMargin * MMS_PER_INCH / 1000;

            break;
        }
        case ( PSD_INHUNDREDTHSOFMILLIMETERS ) :
        {
            //
            //  In 100ths of MMs.
            //
            *plMargin *= 100 ;
            pText[2] = CHAR_NULL;
            *plMargin += ConvertStringToInteger(pText);
            *plMargin = max(lMin, *plMargin);

            //
            //  In 10ths of MMs.
            //
            *plSample = *plMargin / 10;

            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitBannerAndQuality
//
//  Reset PRINT DLG items dependent upon which printer was selected.
//  Assumes that pPD->hDevNames is non-NULL.  pPD->hDevMode non-NULL.
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintInitBannerAndQuality(
    HWND hDlg,
    PPRINTINFO pPI,
    LPPRINTDLG pPD)
{
    HWND hCtl;
    BOOL bResult = TRUE;
    LPDEVMODE pDM = NULL;
    LPDEVNAMES pDN = NULL;
    TCHAR szText[MAX_DEV_SECT];


    //
    //  ID_PRINT_S_DEFAULT is from one of the old templates.
    //
    if (GetDlgItem(hDlg, ID_PRINT_S_DEFAULT))
    {
        if (!pPD->hDevNames ||
            !(pDN = GlobalLock(pPD->hDevNames)))
        {
            StoreExtendedError(CDERR_MEMLOCKFAILURE);
            return (FALSE);
        }

        if (PrintCreateBanner(hDlg, pDN, szText, MAX_DEV_SECT))
        {
            SetDlgItemText(hDlg, ID_PRINT_S_DEFAULT, szText);
        }
        else
        {
            //
            //  PrintCreateBanner sets the extended error.
            //
            bResult = FALSE;
        }

        GlobalUnlock(pPD->hDevNames);
    }

    //
    //  If the driver says it can do copies, pay attention to what the
    //  app requested.  If it cannot do copies, check & disable the
    //  checkbox.
    //
    if (pPD->hDevMode)
    {
        if (!(pDM = GlobalLock(pPD->hDevMode)))
        {
            StoreExtendedError(CDERR_MEMLOCKFAILURE);
            return (FALSE);
        }

        //
        //  Enable print quality, if it exists.
        //
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_S_QUALITY))
        {
            EnableWindow(hCtl, TRUE);
        }
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_C_QUALITY))
        {
            EnableWindow(hCtl, TRUE);

            PrintInitQuality( hCtl,
                              pDM->dmSpecVersion <= 0x0300 ? 0L : pPD,
                              pDM->dmPrintQuality );
        }

        //
        //  If PD_USEDEVMODECOPIES(COLLATE), disable collate if the driver
        //  cannot collate.
        //
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_X_COLLATE))
        {
            if ( pDM->dmFields & DM_COLLATE ||
                 !(pPD->Flags & PD_USEDEVMODECOPIES) )
            {
                EnableWindow(hCtl, TRUE);
                CheckDlgButton( hDlg,
                                ID_PRINT_X_COLLATE,
                                (pPI->Status & PI_COLLATE_REQUESTED)
                                    ? TRUE : FALSE );
            }
            else
            {
                EnableWindow(hCtl, FALSE);
                CheckDlgButton(hDlg, ID_PRINT_X_COLLATE, FALSE);
            }
        }

        //
        //  If PD_USEDEVMODECOPIES(COLLATE), disable copies if the driver
        //  cannot copy.
        //
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_E_COPIES))
        {
            if ( pDM->dmFields & DM_COPIES ||
                 !(pPD->Flags & PD_USEDEVMODECOPIES) )
            {
                SetDlgItemInt(hDlg, ID_PRINT_E_COPIES, pPD->nCopies, FALSE);
                EnableWindow(hCtl, TRUE);
            }
            else
            {
                SetDlgItemInt(hDlg, ID_PRINT_E_COPIES, 1, FALSE);
                EnableWindow(hCtl, FALSE);
            }
        }

        //
        //  Display the appropriate collate icon.
        //
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_I_COLLATE))
        {
            SetWindowLong( hCtl,
                           GWL_STYLE,
                           GetWindowLong(hCtl, GWL_STYLE) | SS_CENTERIMAGE );
            ShowWindow(hCtl, SW_HIDE);
            SendMessage( hCtl,
                         STM_SETICON,
                         IsDlgButtonChecked(hDlg, ID_PRINT_X_COLLATE)
                             ? (LONG_PTR)hIconCollate
                             : (LONG_PTR)hIconNoCollate,
                         0L );
            ShowWindow(hCtl, SW_SHOW);
        }

        GlobalUnlock(pPD->hDevMode);
    }
    else
    {
        //
        //  Disable the print quality, collate, and copies.
        //
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_S_QUALITY))
        {
            EnableWindow(hCtl, FALSE);
        }
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_C_QUALITY))
        {
            EnableWindow(hCtl, FALSE);
        }
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_X_COLLATE))
        {
            EnableWindow(hCtl, FALSE);
        }
        if (hCtl = GetDlgItem(hDlg, ID_PRINT_E_COPIES))
        {
            EnableWindow(hCtl, FALSE);
        }
    }

    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintCreateBanner
//
//  Create "Printer: Prn on Port" or "Printer:  System Printer (Prn)".
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintCreateBanner(
    HWND hDlg,
    LPDEVNAMES pDN,
    LPTSTR psBanner,
    UINT cchBanner)
{
    if (GetDlgItem(hDlg, ID_BOTH_S_PRINTER))
    {
        psBanner[0] = CHAR_NULL;
    }
    else if (!CDLoadString( g_hinst,
                          iszPrinter,
                          psBanner,
                          cchBanner ))
    {
        goto LoadStrFailure;
    }

    if (pDN->wDefault & DN_DEFAULTPRN)
    {
        TCHAR szSysPrn[MAX_DEV_SECT];

        if (!CDLoadString(g_hinst, iszSysPrn, szSysPrn, MAX_DEV_SECT))
        {
            goto LoadStrFailure;
        }
        lstrcat(psBanner, (LPTSTR)szSysPrn);
        lstrcat(psBanner, (LPTSTR)pDN + pDN->wDeviceOffset);
        lstrcat(psBanner, (LPTSTR)TEXT(")"));
    }
    else
    {
        TCHAR szPrnOnPort[64];

        if (!CDLoadString(g_hinst, iszPrnOnPort, szPrnOnPort, 64))
        {
            goto LoadStrFailure;
        }
        lstrcat(psBanner, (LPTSTR)pDN + pDN->wDeviceOffset);
        lstrcat(psBanner, (LPTSTR)szPrnOnPort);
        lstrcat(psBanner, (LPTSTR)pDN + pDN->wOutputOffset);
    }

    return (TRUE);

LoadStrFailure:

    StoreExtendedError(CDERR_LOADSTRFAILURE);
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitQuality
//
//  Initializes the Printer Quality Combobox.
//
//  Assumes pPD structure filled by caller.  If non-NULL, it's a 3.1 or
//  later driver.  If NULL, fill with default for 3.0.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintInitQuality(
    HANDLE hCmb,
    LPPRINTDLG pPD,
    SHORT nQuality)
{
    SHORT nStringID;
    SHORT i;
    TCHAR szBuf[64];
    LPDEVMODE  pDM = NULL;
    LPDEVNAMES pDN = NULL;


    SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);

    //
    //  Enum print qualities.
    //
    if (pPD && pPD->hDevMode && pPD->hDevNames)
    {
        HANDLE hPrnQ;                  // Memory handle for print qualities
        DWORD dw;                      // return from DC_ENUMRESOLUTIONS
        LPLONG pLong;                  // Pointer to pairs of longs
        LPTSTR psDevice;
        LPTSTR psPort;

        pDM = GlobalLock(pPD->hDevMode);
        pDN = GlobalLock(pPD->hDevNames);

        if (pDM->dmSpecVersion < 0x030A)
        {
            goto EnumResNotSupported;
        }

        psDevice = (LPTSTR)pDN + pDN->wDeviceOffset;
        psPort   = (LPTSTR)pDN + pDN->wOutputOffset;

        dw = DeviceCapabilities( psDevice,
                                 psPort,
                                 DC_ENUMRESOLUTIONS,
                                 NULL,
                                 NULL );
        if (!dw || (dw == (DWORD)(-1)))
        {
            goto EnumResNotSupported;
        }

        hPrnQ = GlobalAlloc(GHND, dw * 2 * sizeof(LONG));
        if (!hPrnQ)
        {
            goto EnumResNotSupported;
        }

        if (pLong = GlobalLock(hPrnQ))
        {
            dw = DeviceCapabilities( psDevice,
                                     psPort,
                                     DC_ENUMRESOLUTIONS,
                                     (LPTSTR)pLong,
                                     0 );

            for (nStringID = 0, i = (SHORT)(LOWORD(dw) - 1); i >= 0; i--)
            {
                DWORD xRes, yRes;

                if ((xRes = pLong[i * 2]) != (yRes = pLong[i * 2 + 1]) )
                {
                    wsprintf(szBuf, TEXT("%ld dpi x %ld dpi"), xRes, yRes);
                }
                else
                {
                    wsprintf(szBuf, TEXT("%ld dpi"), yRes);
                }

                SendMessage(hCmb, CB_INSERTSTRING, 0, (LONG_PTR)(LPTSTR)szBuf);
                SendMessage(hCmb, CB_SETITEMDATA, 0, xRes);

                if ( ((SHORT)xRes == nQuality) &&
                     ( (wWinVer < 0x030A) ||
                       !pDM->dmYResolution ||
                       (pDM->dmYResolution == (SHORT)yRes) ) )
                {
                    nStringID = i;
                }
            }
            GlobalUnlock(hPrnQ);
        }
        GlobalFree(hPrnQ);

        SendMessage(hCmb, CB_SETCURSEL, (WPARAM)nStringID, 0L);
    }
    else
    {
EnumResNotSupported:

        for ( i = -1, nStringID = iszDraftPrnQ;
              nStringID >= iszHighPrnQ;
              i--, nStringID-- )
        {
            if (!CDLoadString(g_hinst, nStringID, szBuf, 64))
            {
                return;
            }
            SendMessage(hCmb, CB_INSERTSTRING, 0, (LONG_PTR)(LPTSTR)szBuf);
            SendMessage(hCmb, CB_SETITEMDATA, 0, MAKELONG(i, 0));
        }

        if ((nQuality >= 0) || (nQuality < -4))
        {
            //
            //  Set to HIGH.
            //
            nQuality = -4;
        }
        SendMessage(hCmb, CB_SETCURSEL, (WPARAM)(nQuality + 4), 0L);
    }

    if (pDM)
    {
        GlobalUnlock(pPD->hDevMode);
    }
    if (pDN)
    {
        GlobalUnlock(pPD->hDevNames);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintChangeProperties
//
//  Puts up the dialog to modify the properties.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintChangeProperties(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    LPDEVMODE pDM;
    LONG cbNeeded;
    HANDLE hDevMode;
    WORD nCopies, nCollate;
    BOOL bTest;
    HWND hCtl;


    //
    //  There must be a devmode already.
    //
    if (!pPD->hDevMode)
    {
        return;
    }

    //
    //  Get the number of bytes needed for the devmode.
    //
    cbNeeded = DocumentProperties( hDlg,
                                   pPI->hCurPrinter,
                                   (pPI->pCurPrinter)
                                       ? pPI->pCurPrinter->pPrinterName
                                       : NULL,
                                   NULL,
                                   NULL,
                                   0 );

    //
    //  Reallocate the devmode to be sure there is enough room in it, and
    //  then put up the document properties dialog box.
    //
    if ( (cbNeeded > 0) &&
         (hDevMode = GlobalReAlloc(pPD->hDevMode, cbNeeded, GHND)) &&
         (pDM = GlobalLock(hDevMode)) )
    {
        //
        //  This is done here to make sure that the ReAlloc succeeded
        //  before trashing the old hDevMode.
        //
        pPD->hDevMode = hDevMode;

        //
        //  Set the number of copies and collation in the devmode before
        //  calling DocumentProperties, if appropriate.
        //
        nCopies = pDM->dmCopies;
        nCollate = pDM->dmCollate;
        if (Id == ID_PRINT_C_NAME)
        {
            //
            //  Get the number of copies from the edit control.
            //
            pDM->dmCopies = (WORD)GetDlgItemInt( hDlg,
                                                 ID_PRINT_E_COPIES,
                                                 &bTest,
                                                 FALSE );
            if ((!bTest) || (!pDM->dmCopies))
            {
                pDM->dmCopies = nCopies;
            }

            //
            //  Get the collation from the check box.
            //
            if ( (hCtl = GetDlgItem(hDlg, ID_PRINT_X_COLLATE)) &&
                 IsWindowEnabled(hCtl) )
            {
                SetField( pDM,
                          dmCollate,
                          (IsDlgButtonChecked(hDlg, ID_PRINT_X_COLLATE))
                              ? DMCOLLATE_TRUE
                              : DMCOLLATE_FALSE );
            }
        }
        else   // ID_SETUP_C_NAME
        {
            if ( (pDM->dmFields & DM_COPIES) &&
                 (pPI->ProcessVersion < 0x40000) &&
                 (!(pPD->Flags & PD_USEDEVMODECOPIES)) &&
                 (pPD->nCopies) )
            {
                pDM->dmCopies = pPD->nCopies;

                if (pDM->dmFields & DM_COLLATE)
                {
                    //
                    //  DM_COLLATE was specified, so dmCollate exists.
                    //
                    pDM->dmCollate = (pPD->Flags & PD_COLLATE)
                                         ? DMCOLLATE_TRUE
                                         : DMCOLLATE_FALSE;
                }
            }
        }

        //
        //  Put up the Document Properties dialog box.
        //
        if (DocumentProperties( hDlg,
                                pPI->hCurPrinter,
                                (pPI->pCurPrinter)
                                    ? pPI->pCurPrinter->pPrinterName
                                    : NULL,
                                pDM,
                                pDM,
                                DM_PROMPT | DM_MODIFY | DM_COPY ) == IDOK)
        {
            //
            //  Save the new number of copies and collation, if appropriate.
            //
            if (pDM->dmFields & DM_COPIES)
            {
                pPD->nCopies = pDM->dmCopies;
            }
            if (pDM->dmFields & DM_COLLATE)
            {
                if (pDM->dmCollate == DMCOLLATE_FALSE)
                {
                    pPD->Flags  &= ~PD_COLLATE;
                    pPI->Status &= ~PI_COLLATE_REQUESTED;
                }
                else
                {
                    pPD->Flags  |= PD_COLLATE;
                    pPI->Status |= PI_COLLATE_REQUESTED;
                }
            }

            //
            //  Update the dialog.
            //
            if (Id == ID_PRINT_C_NAME)
            {
                //
                //  Update the print dialog with the new info.
                //
                PrintInitBannerAndQuality(hDlg, pPI, pPD);
            }
            else   // ID_SETUP_C_NAME
            {
                //
                //  Update the print setup dialog with the new info.
                //
                PrintUpdateSetupDlg(hDlg, pPI, pDM, FALSE);
            }
        }
        else
        {
            //
            //  Operation cancelled.  Restore the number of copies
            //  and the collation in the devmode.
            //
            pDM->dmCopies = nCopies;
            SetField(pDM, dmCollate, nCollate);
        }

        GlobalUnlock(pPD->hDevMode);

        SendMessage( hDlg,
                     WM_NEXTDLGCTL,
                     (WPARAM)GetDlgItem(hDlg, IDOK),
                     1L );
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  PrintPrinterChanged
//
////////////////////////////////////////////////////////////////////////////

VOID PrintPrinterChanged(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    HANDLE hDM = NULL;
    LPDEVMODE pDM = NULL;
    LPDEVMODE pDMOld = NULL;
    HWND hCtl;
    UINT Orientation;
    LONG cbSize;
    DWORD dmSize;


    HourGlass(TRUE);

    //
    //  Close the old printer, if necessary.
    //
    if (pPI->hCurPrinter)
    {
        ClosePrinter(pPI->hCurPrinter);
        pPI->hCurPrinter = 0;
    }

    //
    //  Get the current printer from the combo box.
    //
    if (Id && (hCtl = GetDlgItem(hDlg, Id)))
    {
        TCHAR szPrinter[MAX_PRINTERNAME];
        DWORD ctr;

        SendMessage( hCtl,
                     CB_GETLBTEXT,
                     (WPARAM)SendMessage(hCtl, CB_GETCURSEL, 0, 0),
                     (LPARAM)(LPTSTR)szPrinter );

        pPI->pCurPrinter = NULL;
        for (ctr = 0; ctr < pPI->cPrinters; ctr++)
        {
            if (!lstrcmp(pPI->pPrinters[ctr].pPrinterName, szPrinter))
            {
                pPI->pCurPrinter = &pPI->pPrinters[ctr];
                break;
            }
        }
        if (!pPI->pCurPrinter)
        {
            HourGlass(FALSE);
            return;
        }
    }

    //
    //  Open the current printer.
    //
    OpenPrinter(pPI->pCurPrinter->pPrinterName, &pPI->hCurPrinter, NULL);

    //
    //  Build the device names.
    //
    PrintBuildDevNames(pPI);

    //
    //  Get the devmode information.
    //
    cbSize = DocumentProperties( hDlg,
                                 pPI->hCurPrinter,
                                 pPI->pCurPrinter->pPrinterName,
                                 NULL,
                                 NULL,
                                 0 );
    if (cbSize > 0)
    {
        hDM = GlobalAlloc(GHND, cbSize);

        //
        //  Get the default DevMode for the new printer.
        //
        if (hDM && (pDM = GlobalLock(hDM)) &&
            (DocumentProperties( hDlg,
                                 pPI->hCurPrinter,
                                 pPI->pCurPrinter->pPrinterName,
                                 pDM,
                                 NULL,
                                 DM_COPY ) == IDOK))
        {
            //
            //  See if we need to merge in old DevMode settings.
            //
            if (pPD->hDevMode && (pDMOld = GlobalLock(pPD->hDevMode)))
            {
                //
                //  Reset the PaperSource back to the Document Default.
                //
                if (pDM->dmFields & DM_DEFAULTSOURCE)
                {
                    pDMOld->dmFields |= DM_DEFAULTSOURCE;
                    pDMOld->dmDefaultSource = pDM->dmDefaultSource;
                }
                else
                {
                    pDMOld->dmFields &= ~DM_DEFAULTSOURCE;
                }

                //
                //  Copy relevant info from the old devmode to the new
                //  devmode.
                //
                dmSize = min(pDM->dmSize, pDMOld->dmSize);
                if (dmSize > FIELD_OFFSET(DEVMODE, dmFields))
                {
                    CopyMemory( &(pDM->dmFields),
                                &(pDMOld->dmFields),
                                dmSize - FIELD_OFFSET(DEVMODE, dmFields) );
                }

                //
                //  Free the old devmode.
                //
                GlobalUnlock(pPD->hDevMode);
                GlobalFree(pPD->hDevMode);
            }

            //
            //  Save the new DevMode in the pPD structure.
            //
            pPD->hDevMode = hDM;

            //
            //  Get the newly merged DevMode.
            //
            pDM->dmFields = pDM->dmFields & (DM_ORIENTATION | DM_PAPERSIZE  |
                                             DM_PAPERLENGTH | DM_PAPERWIDTH |
                                             DM_SCALE       | DM_COPIES     |
                                             DM_COLLATE     | DM_FORMNAME   |
                                             DM_DEFAULTSOURCE);
            DocumentProperties( hDlg,
                                pPI->hCurPrinter,
                                pPI->pCurPrinter->pPrinterName,
                                pDM,
                                pDM,
                                DM_MODIFY | DM_COPY );
            GlobalUnlock(hDM);
        }
        else if (hDM)
        {
            if (pDM)
            {
                GlobalUnlock(hDM);
            }
            GlobalFree(hDM);
        }
    }

    //
    //  Fill in the appropriate information for the rest of the
    //  Print or Print Setup dialog box.
    //
    if (Id == ID_PRINT_C_NAME)
    {
        PrintInitBannerAndQuality(hDlg, pPI, pPD);
    }
    else   // ID_SETUP_C_NAME
    {
        if (pPD->hDevMode && (pDM = GlobalLock(pPD->hDevMode)))
        {
            if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SIZE))
            {
                PrintInitPaperCombo( pPI,
                                     hCtl,
                                     GetDlgItem(hDlg, ID_SETUP_S_SIZE),
                                     pPI->pCurPrinter,
                                     pDM,
                                     DC_PAPERNAMES,
                                     CCHPAPERNAME,
                                     DC_PAPERS );
            }

            if (hCtl = GetDlgItem(hDlg, ID_SETUP_C_SOURCE))
            {
                PrintInitPaperCombo( pPI,
                                     hCtl,
                                     GetDlgItem(hDlg, ID_SETUP_S_SOURCE),
                                     pPI->pCurPrinter,
                                     pDM,
                                     DC_BINNAMES,
                                     CCHBINNAME,
                                     DC_BINS );
            }

            PrintInitOrientation(hDlg, pPI, pDM);
            Orientation = (pDM->dmOrientation == DMORIENT_PORTRAIT)
                              ? ID_SETUP_R_PORTRAIT
                              : ID_SETUP_R_LANDSCAPE;
            PrintSetOrientation(hDlg, pPI, pDM, Orientation, Orientation);

            PrintInitDuplex(hDlg, pDM);
            PrintSetDuplex( hDlg,
                            pDM,
                            pDM->dmDuplex + ID_SETUP_R_NONE - DMDUP_SIMPLEX );

            GlobalUnlock(pPD->hDevMode);
        }
    }

    //
    //  Update the status information.
    //
    PrintUpdateStatus(hDlg, pPI);

    HourGlass(FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintCancelPrinterChanged
//
//  Opens the old printer since the user hit cancel.  The devmode and
//  devnames structures have already been set back to the old ones.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintCancelPrinterChanged(
    PPRINTINFO pPI,
    LPTSTR pPrinterName)
{
    LPPRINTDLG pPD = pPI->pPD;
    PPRINTER_INFO_2 pCurPrinter;


    //
    //  Make sure we have a previous printer and a devmode.
    //
    if ((pPrinterName[0] == 0) || (!pPD->hDevMode))
    {
        return;
    }

    //
    //  Turn on the hour glass.
    //
    HourGlass(TRUE);

    //
    //  Find the current printer in the list.
    //
    pCurPrinter = PrintSearchForPrinter(pPI, pPrinterName);
    if (!pCurPrinter)
    {
        HourGlass(FALSE);
        return;
    }

    //
    //  Close the old printer, if necessary.
    //
    if (pPI->hCurPrinter)
    {
        ClosePrinter(pPI->hCurPrinter);
        pPI->hCurPrinter = 0;
    }

    //
    //  Save the current printer.
    //
    pPI->pCurPrinter = pCurPrinter;

    //
    //  Open the current printer.
    //
    OpenPrinter(pPI->pCurPrinter->pPrinterName, &pPI->hCurPrinter, NULL);

    //
    //  Turn off the hour glass.
    //
    HourGlass(FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintUpdateStatus
//
////////////////////////////////////////////////////////////////////////////

VOID PrintUpdateStatus(
    HWND hDlg,
    PPRINTINFO pPI)
{
    TCHAR szSeparator[] = TEXT("; ");
    TCHAR szText[256];
    TCHAR szJobs[64];
    LPDEVMODE pDM;
    UINT Length;
    DWORD dwStatus;
    int ctr;
    TCHAR *ps;
    BOOL bFound;


    //
    //  Update the printer status information in the dialog.
    //
    if (!GetDlgItem(hDlg, ID_BOTH_S_STATUS) || (!pPI->pCurPrinter))
    {
        return;
    }

    //
    //  ----------------------  Update Status  ----------------------
    //
    szText[0] = CHAR_NULL;

    if (pPI->pCurPrinter->Attributes & PRINTER_ATTRIBUTE_DEFAULT)
    {
        CDLoadString(g_hinst, iszStatusDefaultPrinter, szText, 32);
    }

    Length = lstrlen(szText);
    dwStatus = pPI->pCurPrinter->Status;
    for (ctr = 0; ctr++ < 32; dwStatus = dwStatus >> 1)
    {
        if (dwStatus & 1)
        {
            CDLoadString( g_hinst,
                        iszStatusReady + ctr,
                        szText + lstrlen(szText),
                        32);
        }
    }

    if (szText[Length])
    {
        if (CDLoadString(g_hinst, iszStatusDocumentsWaiting, szJobs, 64))
        {
            wsprintf( szText + lstrlen(szText),
                      szJobs,
                      pPI->pCurPrinter->cJobs );
        }
    }
    else
    {
        CDLoadString(g_hinst, iszStatusReady, szText + Length, 32);
    }

    SetDlgItemText(hDlg, ID_BOTH_S_STATUS, szText);
    UpdateWindow(GetDlgItem(hDlg, ID_BOTH_S_STATUS));

    //
    //  ----------------------  Update Type  ----------------------
    //
    if (pPI->pCurPrinter->pDriverName)
    {
        lstrcpy(szText, pPI->pCurPrinter->pDriverName);
    }
    else
    {
        szText[0] = CHAR_NULL;
    }

    if (pPI->pPD->hDevMode && (pDM = GlobalLock(pPI->pPD->hDevMode)))
    {
        if (pDM->dmSpecVersion < 0x0400)
        {
            lstrcat(szText, TEXT(" (3.x)"));  // old driver designation
        }
        GlobalUnlock(pPI->pPD->hDevMode);
    }

    SetDlgItemText(hDlg, ID_BOTH_S_TYPE, szText);
    UpdateWindow(GetDlgItem(hDlg, ID_BOTH_S_TYPE));

    //
    //  ----------------------  Update Location  ----------------------
    //
    if (pPI->pCurPrinter->pLocation && pPI->pCurPrinter->pLocation[0])
    {
        bFound = FALSE;
        lstrcpy(szText, pPI->pCurPrinter->pLocation);
        for (ps = szText; *ps; ps++)
        {
            if (ps[0] == TEXT('\r') && ps[1] == TEXT('\n'))
            {
                *ps++ = CHAR_SEMICOLON;
                *ps   = CHAR_SPACE;
            }
            else
            {
                bFound = TRUE;
            }
        }
        if (!bFound)
        {
            goto ShowPortName;
        }
    }
    else
    {
ShowPortName:
        if (pPI->pCurPrinter->pPortName)
        {
            lstrcpy(szText, pPI->pCurPrinter->pPortName);
        }
        else
        {
            szText[0] = CHAR_NULL;
        }
    }

    EnableWindow(GetDlgItem(hDlg, ID_BOTH_S_WHERE), szText[0]);
    SetDlgItemText(hDlg, ID_BOTH_S_WHERE, szText);
    UpdateWindow(GetDlgItem(hDlg, ID_BOTH_S_WHERE));

    //
    //  ----------------------  Update Comment  ----------------------
    //
    if (pPI->pCurPrinter->pComment && pPI->pCurPrinter->pComment[0])
    {
        bFound = FALSE;
        lstrcpy(szText, pPI->pCurPrinter->pComment);
        for (ps = szText; *ps; ps++)
        {
            if (ps[0] == TEXT('\r') && ps[1] == TEXT('\n'))
            {
                *ps++ = CHAR_SEMICOLON;
                *ps   = CHAR_SPACE;
            }
            else
            {
                bFound = TRUE;
            }
        }
        if (!bFound)
        {
            //
            //  This is needed in case the comment field only has a
            //  carriage return in it.  Without this check, it will
            //  show a ";" in the comment field.  In this case, it
            //  should show "" in the comment field.
            //
            szText[0] = CHAR_NULL;
        }
    }
    else
    {
        szText[0] = CHAR_NULL;
    }

    EnableWindow(GetDlgItem(hDlg, ID_BOTH_S_COMMENT), szText[0]);
    SetDlgItemText(hDlg, ID_BOTH_S_COMMENT, szText);
    UpdateWindow(GetDlgItem(hDlg, ID_BOTH_S_COMMENT));
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetSetupInfo
//
//  Purpose:  Retrieve info from Print Setup dialog elements
//  Assumes:  hDevMode handle to valid DEVMODE structure
//  Returns:  TRUE if hDevMode valid, FALSE otherwise
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintGetSetupInfo(
    HWND hDlg,
    LPPRINTDLG pPD)
{
    LPDEVMODE pDM = NULL;
    LPDEVNAMES pDN = NULL;
    HWND hCmb;
    int nInd;


    if ( !pPD->hDevMode ||
         !(pDM = GlobalLock(pPD->hDevMode)) )
    {
        return (FALSE);
    }

    // Don't need to do this - this is kept up to date.
    // pDM->dmFields |= DM_ORIENTATION;

    if (hCmb = GetDlgItem(hDlg, ID_SETUP_C_SIZE))
    {
        nInd = (int) SendMessage(hCmb, CB_GETCURSEL, 0, 0L);
        if (nInd != CB_ERR)
        {
        //  pDM->dmFields |= DM_PAPERSIZE;
            pDM->dmPaperSize = (SHORT)SendMessage( hCmb,
                                                   CB_GETITEMDATA,
                                                   nInd,
                                                   0 );
#ifndef WINNT
            if (pDM->dmSpecVersion >= 0x0400)
#endif
            {
            //  pDM->dmFields |= DM_FORMNAME;
                SendMessage( hCmb,
                             CB_GETLBTEXT,
                             nInd,
                             (LPARAM)pDM->dmFormName );
            }
        }
    }

    if (hCmb = GetDlgItem(hDlg, ID_SETUP_C_SOURCE))
    {
        nInd = (int) SendMessage(hCmb, CB_GETCURSEL, 0 , 0L);
        if (nInd != CB_ERR)
        {
        //  pDM->dmFields |= DM_DEFAULTSOURCE;
            pDM->dmDefaultSource = (SHORT)SendMessage( hCmb,
                                                       CB_GETITEMDATA,
                                                       nInd,
                                                       0 );
        }
    }

    if ( (pPD->hDevNames) &&
         (pDN = GlobalLock(pPD->hDevNames)) )
    {
        PrintReturnICDC(pPD, pDN, pDM);
        GlobalUnlock(pPD->hDevNames);
    }

    GlobalUnlock(pPD->hDevMode);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSearchForPrinter
//
//  Returns the pointer to the PRINTER_INFO_2 structure for the printer
//  with the name pPrinterName.
//
////////////////////////////////////////////////////////////////////////////

PPRINTER_INFO_2 PrintSearchForPrinter(
    PPRINTINFO pPI,
    LPCTSTR lpsPrinterName)
{
    DWORD ctr;

    //
    //  Search for the printer.
    //
    for (ctr = 0; ctr < pPI->cPrinters; ctr++)
    {
        if (!lstrcmp(pPI->pPrinters[ctr].pPrinterName, lpsPrinterName))
        {
            //
            //  Found it.
            //
            return (&pPI->pPrinters[ctr]);
        }
    }

    //
    //  Did not find the printer.
    //
    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetExtDeviceMode
//
////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

VOID PrintGetExtDeviceMode(
    HWND hDlg,
    PPRINTINFO pPI)
{
    DWORD ctr;
    LPDEVMODEA pDMA;
    LPDEVMODEW pDMW;
    int iResult;
    CHAR szPrinterNameA[MAX_PRINTERNAME];


    if (!pPI->bUseExtDeviceMode)
    {
        return;
    }

    //
    //  Allocate the array to hold whether or not a new devmode has been
    //  allocated for each of the printers.
    //
    //  This is necessary because if the call to ExtDeviceMode fails, then
    //  nothing was allocated.  The one that is currently in the pPrinters
    //  array is actually part of the big pPrinters array (from the call
    //  to GetPrinter - it wants one giant buffer).
    //
    if (pPI->cPrinters)
    {
        if (pPI->pAllocInfo)
        {
            GlobalFree(pPI->pAllocInfo);
        }
        pPI->pAllocInfo = (LPBOOL)GlobalAlloc( GPTR,
                                               pPI->cPrinters * sizeof(BOOL) );
    }

    //
    //  If we were called from a WOW app with a NULL devmode,
    //  then call ExtDeviceMode to get a default devmode.
    //
    for (ctr = 0; ctr < pPI->cPrinters; ctr++)
    {
        //
        //  Convert the printer name from Unicode to ANSI.
        //
        SHUnicodeToAnsi(pPI->pPrinters[ctr].pPrinterName, szPrinterNameA, MAX_PRINTERNAME);

        //
        //  Call ExtDeviceMode with 0 flags to find out the
        //  size of the devmode structure we need.
        //
        iResult = ExtDeviceMode( hDlg,
                                 NULL,
                                 NULL,
                                 szPrinterNameA,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0 );
        if (iResult < 0)
        {
            continue;
        }

        //
        //  Allocate the space.
        //
        pDMA = GlobalAlloc(GPTR, iResult);
        if (!pDMA)
        {
            continue;
        }

        //
        //  Call ExtDeviceMode to get the dummy devmode structure.
        //
        iResult = ExtDeviceMode( hDlg,
                                 NULL,
                                 pDMA,
                                 szPrinterNameA,
                                 NULL,
                                 NULL,
                                 NULL,
                                 DM_COPY );
        if (iResult < 0)
        {
            GlobalFree(pDMA);
            continue;
        }

        //
        //  Call AllocateUnicodeDevMode to allocate and copy the unicode
        //  version of this ANSI dev mode.
        //
        pDMW = AllocateUnicodeDevMode(pDMA);
        if (!pDMW)
        {
            GlobalFree(pDMA);
            continue;
        }

        //
        //  Store the pointer to the new devmode in the old pointer
        //  position.  We don't have to worry about freeing the
        //  current contents of pPrinter[ctr].pDevMode before sticking
        //  in the new pointer because in reality the pPrinter memory
        //  buffer is just one long allocation (the memory pDevmode
        //  points to is part of the pPrinters buffer).  So, when the
        //  buffer is freed at the end, the old devmode will be freed
        //  with it.
        //
        pPI->pPrinters[ctr].pDevMode = pDMW;
        pPI->pAllocInfo[ctr] = TRUE;

        //
        //  Free the ANSI dev mode.
        //
        GlobalFree(pDMA);
    }
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  PrintEnumAndSelect
//
//  This routine enumerates the LOCAL and CONNECTED printers.
//  It is called at initialization and when a new printer is
//  added via the NETWORK... button.
//
//  If the second parameter is set, the first parameter is overridden.
//  When the second parameter is NULL, the first parameter is used.
//  In this case, if the first parameter is greater than the total
//  number of printers enumerated, then the last one in the list is
//  selected.
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintEnumAndSelect(
    HWND hDlg,
    UINT Id,
    PPRINTINFO pPI,
    LPTSTR lpsPrinterToSelect,
    BOOL bEnumPrinters)
{
    HWND hCtl = ((hDlg && Id) ? GetDlgItem(hDlg, Id) : 0);
    LPPRINTDLG pPD = pPI->pPD;
    TCHAR szPrinter[MAX_PRINTERNAME];
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD ctr;
    PPRINTER_INFO_2 pPrinters = NULL;


    //
    //  Enumerate the printers, if necessary.
    //
    if (bEnumPrinters)
    {
Print_Enumerate:
        //
        //  Save lpsPrinterToSelect in a local before it gets freed.
        //
        if (lpsPrinterToSelect)
        {
            lstrcpy(szPrinter, lpsPrinterToSelect);
            lpsPrinterToSelect = szPrinter;
        }

        //
        //  Close and free any open printers.
        //
        PrintClosePrinters(pPI);

        //
        //  Clear out the error code.
        //
        StoreExtendedError(CDERR_GENERALCODES);

        //
        //  Enumerate the printers.
        //
        if (!EnumPrinters( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                           NULL,
                           2,
                           NULL,
                           0,
                           &cbNeeded,
                           &cReturned ))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (pPrinters = GlobalAlloc(GPTR, cbNeeded))
                {
                    if (EnumPrinters( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                                      NULL,
                                      2,
                                      (LPBYTE)pPrinters,
                                      cbNeeded,
                                      &cbNeeded,
                                      &cReturned ))
                    {
                        pPI->cPrinters = cReturned;
                        pPI->pPrinters =  pPrinters;
                        pPI->Status |= PI_PRINTERS_ENUMERATED;
                    }
                    else
                    {
                        StoreExtendedError(PDERR_NODEFAULTPRN);
                    }
                }
                else
                {
                    StoreExtendedError(CDERR_MEMALLOCFAILURE);
                }
            }
            else
            {
                StoreExtendedError(PDERR_NODEFAULTPRN);
            }
        }
        else
        {
            StoreExtendedError(PDERR_NODEFAULTPRN);
        }

        if (GetStoredExtendedError())
        {
            if (pPrinters)
            {
                GlobalFree(pPrinters);
            }
            return (FALSE);
        }

        //
        //  Make modifications for a WOW app.
        //
#ifdef UNICODE
        if (pPI->bUseExtDeviceMode)
        {
            PrintGetExtDeviceMode(hDlg, pPI);
        }
#endif

        //
        //  Try the selected printer.
        //
        if (lpsPrinterToSelect)
        {
            pPI->pCurPrinter = PrintSearchForPrinter(pPI, lpsPrinterToSelect);
        }

        //
        //  Open the current printer.
        //
        if (pPI->pCurPrinter)
        {
            //
            //  Open the current printer.
            //
            OpenPrinter(pPI->pCurPrinter->pPrinterName, &pPI->hCurPrinter, NULL);
        }
        else
        {
            //
            //  If there isn't a current printer, try the printers in
            //  the list until either one is found that can be opened or
            //  until there are no more printers in the list.
            //
            for (ctr = 0; ctr < pPI->cPrinters; ctr++)
            {
                pPI->pCurPrinter = &pPI->pPrinters[ctr];

                //
                //  Try to open the printer.
                //
                if (OpenPrinter( pPI->pCurPrinter->pPrinterName,
                                 &pPI->hCurPrinter,
                                 NULL ))
                {
                    break;
                }
            }
        }
    }
    else
    {
        //
        //  If there isn't a current printer, then try to enumerate.
        //  This means something isn't setup properly.
        //
        if ((!pPI->pCurPrinter) || (!pPI->pPrinters))
        {
            goto Print_Enumerate;
        }
    }

    if (hCtl)
    {
        //
        //  Reset the contents of the list box.
        //
        SendMessage(hCtl, CB_RESETCONTENT, 0, 0);

        //
        //  Add all of the printer name strings to the list box.
        //
        for (ctr = 0; ctr < pPI->cPrinters; ctr++)
        {
            SendMessage( hCtl,
                         CB_ADDSTRING,
                         0,
                         (LPARAM)pPI->pPrinters[ctr].pPrinterName );
        }

        //
        //  Set the current selection in the list box.
        //
        SendMessage( hCtl,
                     CB_SETCURSEL,
                     SendMessage( hCtl,
                                  CB_FINDSTRINGEXACT,
                                  (WPARAM)-1,
                                  (LPARAM)pPI->pCurPrinter->pPrinterName ),
                     0L );
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintBuildDevNames
//
////////////////////////////////////////////////////////////////////////////

VOID PrintBuildDevNames(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    LPTSTR pPrinterName = NULL;
    LPTSTR pPortName = NULL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szPort[MAX_PATH];
    LPTSTR pStr;
    LPDEVNAMES pDN;
    DWORD cbDevNames;
    HANDLE hPrinter ;
    PPRINTER_INFO_2 pPrinter = NULL;


    //
    //  If this is called from PrintReturnDefault, there is no
    //  PrinterInfo (pPI->pCurPrinter) because the printers were not
    //  enumerated.  So, build the DEVNAME from win.ini.
    //
    pStr = szBuffer;
    if (!pPI->pCurPrinter)
    {
        //
        //  Get the default printer from the "Windows" section of win.ini.
        //      (eg. device=\\server\local,winspool,Ne00:)
        //
        if ( (pPD->Flags & PD_RETURNDEFAULT) &&
             GetProfileString( szTextWindows,
                               szTextDevice,
                               szTextNull,
                               szBuffer,
                               MAX_PATH ) )
        {
            //  Examples of szBuffer:
            //    "My Local Printer,winspool,LPT1:"   or
            //    "\\server\local,winspool,Ne00:"

            //
            //  Skip leading space (if any).
            //
            while (*pStr == CHAR_SPACE)
            {
                pStr++;
            }

            //
            //  First token is the printer name.
            //
            pPrinterName = pStr;

            while (*pStr && *pStr != CHAR_COMMA)
            {
                pStr++;
            }

            //
            //  NULL terminate the printer name.
            //
            *pStr++ = CHAR_NULL;

            // For Newer Apps  return the port name from the PRINT_INFO_2 structure.
            // For older apps  return the short port name give in the win.ini
            if (pPI->ProcessVersion >= 0x40000)
            {
                //Newer App
                if (OpenPrinter(pPrinterName, &hPrinter, NULL))
                {
                    if (pPrinter = PrintGetPrinterInfo2(hPrinter))
                    {
                        lstrcpy(szPort, pPrinter->pPortName);
                        pPortName = szPort;
                        GlobalFree(pPrinter);
                    }
                    ClosePrinter(hPrinter);

                 }
                 else
                 {
                     //Unable to Open Printer so return
                     return ;
                 }
            }
            else
            {

                //Old App

                //
                //  Skip the driver name (second token).
                //
                while (*pStr && *pStr++ != CHAR_COMMA)
                {
                    ;
                }

                //
                //  Skip leading space (if any).
                //
                while (*pStr == CHAR_SPACE)
                {
                    pStr++;
                }

                //
                //  Third (and last) token is the port name.
                //
                pPortName = pStr;
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        //
        //  Get the printer name from the PrinterInfo2 structure
        //  for the current printer.
        //
        pPrinterName = pPI->pCurPrinter->pPrinterName;

        //
        //  Newer Apps:
        //    Get the port name from the PrinterInfo2 structure for the
        //    current printer.  Want to use the PrinterInfo2 structure
        //    for newer apps so that we can support multiple ports for
        //    one printer.
        //
        //  Older Apps:
        //    First try to get the port name from the "devices" section
        //    of win.ini.  If that fails, then use the PrinterInfo2
        //    structure for the current printer.
        //
        //    This needs to use the "devices" section first due to a bug
        //    in AutoCAD.  AutoCAD only allows 13 characters for the port
        //    name and it does not check the length when it tries to copy
        //    it to its own buffer.
        //
#ifdef WINNT
        if ( (pPI->ProcessVersion >= 0x40000) ||
             (!GetProfileString( szTextDevices,
                                 pPrinterName,
                                 szTextNull,
                                 szBuffer,
                                 MAX_PATH )) ||
             (!(pPortName = StrChr(szBuffer, CHAR_COMMA))) ||
             (!((++pPortName)[0])) )
#endif
        {
            //
            //  Get the port name from the PrinterInfo2 structure
            //  for the current printer.
            //
            pPortName = pPI->pCurPrinter->pPortName;
        }
    }

    //
    //  Compute the size of the DevNames structure.
    //
    cbDevNames = lstrlen(szDriver) + 1 +
                 lstrlen(pPortName) + 1 +
                 lstrlen(pPrinterName) + 1 +
                 DN_PADDINGCHARS;

    cbDevNames *= sizeof(TCHAR);
    cbDevNames += sizeof(DEVNAMES);

    //
    //  Allocate the new DevNames structure.
    //
    pDN = NULL;
    if (pPD->hDevNames)
    {
        HANDLE handle;

        handle = GlobalReAlloc(pPD->hDevNames, cbDevNames, GHND);

        //Make sure the Realloc succeeded.
        if (handle)
        {
            pPD->hDevNames = handle;
        }
        else
        {
            //Realloc didn't succeed.  Free the old the memory
            GlobalFree(pPD->hDevNames);
        }
    }
    else
    {
        pPD->hDevNames = GlobalAlloc(GHND, cbDevNames);
    }

    //
    //  Fill in the DevNames structure with the appropriate information.
    //
    if ( (pPD->hDevNames) &&
         (pDN = GlobalLock(pPD->hDevNames)) )
    {
        pDN->wDriverOffset = sizeof(DEVNAMES) / sizeof(TCHAR);
        lstrcpy((LPTSTR)pDN + pDN->wDriverOffset, szDriver);

        pDN->wDeviceOffset = pDN->wDriverOffset + lstrlen(szDriver) + 1;
        lstrcpy((LPTSTR)pDN + pDN->wDeviceOffset, pPrinterName);

        pDN->wOutputOffset = pDN->wDeviceOffset + lstrlen(pPrinterName) + 1;
        lstrcpy((LPTSTR)pDN + pDN->wOutputOffset, pPortName);

        if ( (pPD->Flags & PD_RETURNDEFAULT) ||
             !lstrcmp(pPrinterName, pPI->szDefaultPrinter) )
        {
            pDN->wDefault = DN_DEFAULTPRN;
        }
        else
        {
            pDN->wDefault = 0;
        }

        GlobalUnlock(pPD->hDevNames);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetDevMode
//
//  Create and/or fill DEVMODE structure.
//
////////////////////////////////////////////////////////////////////////////

HANDLE PrintGetDevMode(
    HWND hDlg,
    HANDLE hPrinter,
    LPTSTR lpsDeviceName,
    HANDLE hDevMode)
{
    LONG cbNeeded;
    LPDEVMODE pDM;


    cbNeeded = DocumentProperties( hDlg,
                                   hPrinter,
                                   lpsDeviceName,
                                   (PDEVMODE)NULL,
                                   (PDEVMODE)NULL,
                                   0 );

    if (cbNeeded > 0)
    {
        if (hDevMode)
        {
            HANDLE handle;
            
            handle = GlobalReAlloc(hDevMode, cbNeeded, GHND);

            //Make sure realloc succeeded.
            if (handle)
            {
                hDevMode  = handle;
            }
            else
            {
                //Realloc didn't succeed. Free the memory occupied
                GlobalFree(hDevMode);
            }

        }
        else
        {
            hDevMode = GlobalAlloc(GHND, cbNeeded);
        }

        if (hDevMode && (pDM = GlobalLock(hDevMode)))
        {
            if (DocumentProperties( hDlg,
                                    hPrinter,
                                    lpsDeviceName,
                                    pDM,
                                    NULL,
                                    DM_COPY ) != IDOK)
            {
                StoreExtendedError(PDERR_NODEFAULTPRN);
                GlobalUnlock(hDevMode);
                GlobalFree(hDevMode);
                return (NULL);
            }

            GlobalUnlock(hDevMode);
        }
        else
        {
            if (hDevMode)
            {
                StoreExtendedError(CDERR_MEMLOCKFAILURE);
                GlobalFree(hDevMode);
            }
            else
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
            }
            return (NULL);
        }
    }
    else
    {
        DWORD dwErrCode;

        hDevMode = NULL;
        dwErrCode = GetLastError();

        if ( (dwErrCode == ERROR_UNKNOWN_PRINTER_DRIVER) ||
             (dwErrCode == ERROR_MOD_NOT_FOUND) )
        {
            if (hDlg)
            {
                PrintEditError(hDlg, 0, iszUnknownDriver, lpsDeviceName);
            }
        }
    }

    return (hDevMode);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintReturnICDC
//
//  Retrieve either the hDC or the hIC if either flag is set.
//  Assumes the PD_PRINTOFILE flag is appropriately set.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintReturnICDC(
    LPPRINTDLG pPD,
    LPDEVNAMES pDN,
    LPDEVMODE pDM)
{
    if (pPD->Flags & PD_PRINTTOFILE)
    {
        lstrcpy((LPTSTR)pDN + pDN->wOutputOffset, szFilePort);
    }

#ifdef UNICODE
    //
    //  The dmCollate field wasn't part of the Win3.1 DevMode struct.  The way
    //  16-bit apps achieved collation was by checking the PD_COLLATE flag in
    //  the PrintDlg struct.  The app would then figure out the page printing
    //  order to achieve collation.  So what we're doing here is making sure
    //  that PD_COLLATE is the only collation mechanism for 16-bit apps.  If we
    //  let DM_COLLATE get into the DC we'd end up with the driver trying to
    //  collate a job that the app is already trying to collate!
    //
    if ((pPD->Flags & CD_WOWAPP) && pDM)
    {
        if (pDM->dmFields & DM_COLLATE)
        {
            pPD->Flags |= PD_COLLATE;
        }

        // these should always be off for WOW apps
        pDM->dmCollate = DMCOLLATE_FALSE;
        pDM->dmFields &= ~DM_COLLATE;
    }
#endif

    switch (pPD->Flags & (PD_RETURNDC | PD_RETURNIC))
    {
        case ( PD_RETURNIC ) :
        {
            pPD->hDC = CreateIC( (LPTSTR)pDN + pDN->wDriverOffset,
                                 (LPTSTR)pDN + pDN->wDeviceOffset,
                                 (LPTSTR)pDN + pDN->wOutputOffset,
                                 pDM);
            if (pPD->hDC)
            {
                break;
            }

            // else fall thru...
        }
        case ( PD_RETURNDC ) :
        case ( PD_RETURNDC | PD_RETURNIC ) :
        {
            //
            //  PD_RETURNDC has priority if they are both set.
            //
            pPD->hDC = CreateDC( (LPTSTR)pDN + pDN->wDriverOffset,
                                 (LPTSTR)pDN + pDN->wDeviceOffset,
                                 (LPTSTR)pDN + pDN->wOutputOffset,
                                 pDM );
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintMeasureItem
//
////////////////////////////////////////////////////////////////////////////

VOID PrintMeasureItem(
    HANDLE hDlg,
    LPMEASUREITEMSTRUCT mis)
{
    HDC hDC;
    TEXTMETRIC TM;
    HANDLE hFont;


    if (hDC = GetDC(hDlg))
    {
        hFont = (HANDLE)SendMessage(hDlg, WM_GETFONT, 0, 0L);
        if (!hFont)
        {
            hFont = GetStockObject(SYSTEM_FONT);
        }
        hFont = SelectObject(hDC, hFont);
        GetTextMetrics(hDC, &TM);
        mis->itemHeight = (WORD)TM.tmHeight;
        SelectObject(hDC, hFont);
        ReleaseDC(hDlg, hDC);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitOrientation
//
//  Enable/Disable Paper Orientation controls
//
//  NOTE: If the driver doesn't support orientation AND is smart
//  enough to tell us about it, disable the appropriate dialog items.
//  "Smart enough" means the driver must support DC_ORIENTATION in its
//  DeviceCapabilities routine.  This was introduced for 3.1, hence the
//  version test.  NotBadDriver() may need to be incorporated if a
//  problem driver is found in testing.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintInitOrientation(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM)
{
    BOOL bEnable = TRUE;
    HWND hCtl;
    HDC hDC;
    int iHeight;
    PPRINTER_INFO_2 pPrinter = pPI->pCurPrinter;


    if (!pPrinter)
    {
        return;
    }

    if (pDM->dmSpecVersion >= 0x030A)
    {
        pPI->dwRotation = DeviceCapabilities( pPrinter->pPrinterName,
                                              pPrinter->pPortName,
                                              DC_ORIENTATION,
                                              NULL,
                                              pDM );
        switch (pPI->dwRotation)
        {
            case ( ROTATE_LEFT ) :
            case ( ROTATE_RIGHT ) :
            {
                bEnable = TRUE;
                break;
            }
            default :
            {
                pPI->dwRotation = 0;
                bEnable = FALSE;
                pDM->dmOrientation = DMORIENT_PORTRAIT;
                CheckRadioButton( hDlg,
                                  ID_SETUP_R_PORTRAIT,
                                  ID_SETUP_R_LANDSCAPE,
                                  ID_SETUP_R_PORTRAIT );
                break;
            }
        }
    }

    if ( (pDM->dmOrientation != DMORIENT_PORTRAIT) &&
         (pDM->dmOrientation != DMORIENT_LANDSCAPE) )
    {
        pDM->dmOrientation  = DMORIENT_PORTRAIT;
    }

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_LANDSCAPE))
    {
        //
        //  Landscape
        //
        if ( !( (pPI->pPSD) &&
                (pPI->pPSD->Flags & PSD_DISABLEORIENTATION) ) )
        {
            EnableWindow(hCtl, bEnable);
        }
    }
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_I_ORIENTATION))
    {
        //
        //  Orientation of icon.
        //
        SetWindowLong( hCtl,
                       GWL_STYLE,
                       GetWindowLong(hCtl, GWL_STYLE) | SS_CENTERIMAGE );
    }

    if ( (!pPI->RtSampleXYWH.left) &&
         (hCtl = GetDlgItem(hDlg, ID_SETUP_W_SAMPLE)) )
    {
        GetWindowRect(hCtl, (LPRECT)&pPI->RtSampleXYWH);
        ScreenToClient(hDlg, (LPPOINT)&pPI->RtSampleXYWH.left);
        ScreenToClient(hDlg, (LPPOINT)&pPI->RtSampleXYWH.right);

        iHeight = pPI->RtSampleXYWH.bottom - pPI->RtSampleXYWH.top;
        pPI->RtSampleXYWH.bottom = iHeight;

        if (hDC = GetDC(0))
        {
            iHeight = iHeight * GetDeviceCaps(hDC, LOGPIXELSX) /
                                GetDeviceCaps(hDC, LOGPIXELSY);
            ReleaseDC(0, hDC);
        }

        pPI->RtSampleXYWH.left =
            (pPI->RtSampleXYWH.left + pPI->RtSampleXYWH.right - iHeight) / 2;
        pPI->RtSampleXYWH.right = iHeight;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetOrientation
//
//  Switch icon, check button, for Portrait or LandScape printing mode.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintSetOrientation(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    UINT uiOldId,
    UINT uiNewId)
{
    BOOL bPortrait;
    HWND hIcn;


    bPortrait = (uiNewId == ID_SETUP_R_PORTRAIT);

    pDM->dmOrientation = ( bPortrait
                               ? DMORIENT_PORTRAIT
                               : DMORIENT_LANDSCAPE );

    CheckRadioButton(hDlg, ID_SETUP_R_PORTRAIT, ID_SETUP_R_LANDSCAPE, uiNewId);

    if (hIcn = GetDlgItem(hDlg, ID_SETUP_I_ORIENTATION))
    {
        ShowWindow(hIcn, SW_HIDE);
        SendMessage( hIcn,
                     STM_SETICON,
                     bPortrait ? (LONG_PTR)hIconPortrait : (LONG_PTR)hIconLandscape,
                     0L );
        ShowWindow(hIcn, SW_SHOW);
    }

    //
    //  Update the page setup dialog, if necessary.
    //
    if (pPI->pPSD)
    {
        PrintUpdatePageSetup(hDlg, pPI, pDM, uiOldId, uiNewId);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintUpdatePageSetup
//
//  Update the page setup information.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintUpdatePageSetup(
    HWND hDlg,
    PPRINTINFO pPI,
    LPDEVMODE pDM,
    UINT uiOldId,
    UINT uiNewId)
{
    BOOL bPortrait = (uiNewId == ID_SETUP_R_PORTRAIT);
    LPPAGESETUPDLG pPSD = pPI->pPSD;
    LPPRINTDLG pPD = pPI->pPD;
    HWND hWndSample;
    HWND hWndShadowRight;
    HWND hWndShadowBottom;
    HWND hWndSize;
    LONG lTemp;


    if (!pPSD)
    {
        return;
    }

    if (uiOldId != uiNewId)
    {
        RECT aRtMinMargin = pPSD->rtMinMargin;
        RECT aRtMargin    = pPSD->rtMargin;
        HWND hWndLeft     = GetDlgItem(hDlg, ID_SETUP_E_LEFT);
        HWND hWndTop      = GetDlgItem(hDlg, ID_SETUP_E_TOP);
        HWND hWndRight    = GetDlgItem(hDlg, ID_SETUP_E_RIGHT);
        HWND hWndBottom   = GetDlgItem(hDlg, ID_SETUP_E_BOTTOM);
        TCHAR szLeft  [8];
        TCHAR szTop   [8];
        TCHAR szRight [8];
        TCHAR szBottom[8];

        GetWindowText(hWndLeft, szLeft, 8);
        GetWindowText(hWndTop, szTop, 8);
        GetWindowText(hWndRight, szRight, 8);
        GetWindowText(hWndBottom, szBottom, 8);

        switch (uiNewId + pPI->dwRotation)
        {
            case ( ID_SETUP_R_PORTRAIT  + ROTATE_RIGHT ) :  // HP PCL
            case ( ID_SETUP_R_LANDSCAPE + ROTATE_LEFT ) :   // dot-matrix
            {
                pPSD->rtMinMargin.left   = aRtMinMargin.top;
                pPSD->rtMinMargin.top    = aRtMinMargin.right;
                pPSD->rtMinMargin.right  = aRtMinMargin.bottom;
                pPSD->rtMinMargin.bottom = aRtMinMargin.left;

                pPSD->rtMargin.left   = aRtMargin.top;
                pPSD->rtMargin.top    = aRtMargin.right;
                pPSD->rtMargin.right  = aRtMargin.bottom;
                pPSD->rtMargin.bottom = aRtMargin.left;

                SetWindowText(hWndLeft, szTop);
                SetWindowText(hWndRight, szBottom);
                SetWindowText(hWndTop, szRight);
                SetWindowText(hWndBottom, szLeft);

                break;
            }
            case ( ID_SETUP_R_PORTRAIT  + ROTATE_LEFT ) :   // dot-matrix
            case ( ID_SETUP_R_LANDSCAPE + ROTATE_RIGHT ) :  // HP PCL
            {
                pPSD->rtMinMargin.left   = aRtMinMargin.bottom;
                pPSD->rtMinMargin.top    = aRtMinMargin.left;
                pPSD->rtMinMargin.right  = aRtMinMargin.top;
                pPSD->rtMinMargin.bottom = aRtMinMargin.right;

                pPSD->rtMargin.left   = aRtMargin.bottom;
                pPSD->rtMargin.top    = aRtMargin.left;
                pPSD->rtMargin.right  = aRtMargin.top;
                pPSD->rtMargin.bottom = aRtMargin.right;

                SetWindowText(hWndLeft, szBottom);
                SetWindowText(hWndRight, szTop);
                SetWindowText(hWndTop, szLeft);
                SetWindowText(hWndBottom, szRight);

                break;
            }
        }
    }
    pPI->uiOrientationID = uiNewId;

    //
    //  Update ptPaperSize.
    //
    pPI->PtPaperSizeMMs.x = 0;
    pPI->PtPaperSizeMMs.y = 0;
    pPD->Flags &= ~PI_WPAPER_ENVELOPE;

    if ((hWndSize = GetDlgItem(hDlg, ID_SETUP_C_SIZE)) && (pPI->pCurPrinter))
    {
        PPRINTER_INFO_2 pPrinter = pPI->pCurPrinter;
        DWORD dwNumber;
        LPWORD lpPapers;
        LPPOINT lpPaperSize;
        int nInd;
        DWORD i;

        dwNumber = DeviceCapabilities( pPrinter->pPrinterName,
                                       pPrinter->pPortName,
                                       DC_PAPERS,
                                       NULL,
                                       pDM );
        if ( dwNumber &&
             (dwNumber != (DWORD)-1) &&
             (lpPapers = LocalAlloc( LPTR,
                                     dwNumber *
                                         (sizeof(WORD) + sizeof(POINT)) * 2 )) )
        {
            lpPaperSize = (LPPOINT)(lpPapers + dwNumber * 2);

            DeviceCapabilities( pPrinter->pPrinterName,
                                pPrinter->pPortName,
                                DC_PAPERS,
                                (LPTSTR)lpPapers,
                                pDM );
            DeviceCapabilities( pPrinter->pPrinterName,
                                pPrinter->pPortName,
                                DC_PAPERSIZE,
                                (LPTSTR)lpPaperSize,
                                pDM );

            if ((nInd = (int) SendMessage(hWndSize, CB_GETCURSEL, 0, 0)) != CB_ERR)
            {
                pPI->wPaper = (WORD)SendMessage( hWndSize,
                                                 CB_GETITEMDATA,
                                                 nInd,
                                                 0 );
                pDM->dmPaperSize = pPI->wPaper;
            }
            else
            {
                pPI->wPaper = pDM->dmPaperSize;
            }

#ifndef WINNT
            if (pDM->dmSpecVersion >= 0x0400)
#endif
            {
                SendMessage( hWndSize,
                             CB_GETLBTEXT,
                             nInd,
                             (LPARAM)pDM->dmFormName );
            }

            switch (pPI->wPaper)
            {
                case ( DMPAPER_ENV_9 ) :
                case ( DMPAPER_ENV_10 ) :
                case ( DMPAPER_ENV_11 ) :
                case ( DMPAPER_ENV_12 ) :
                case ( DMPAPER_ENV_14 ) :
                case ( DMPAPER_ENV_DL ) :
                case ( DMPAPER_ENV_C5 ) :
                case ( DMPAPER_ENV_C3 ) :
                case ( DMPAPER_ENV_C4 ) :
                case ( DMPAPER_ENV_C6 ) :
                case ( DMPAPER_ENV_C65 ) :
                case ( DMPAPER_ENV_B4 ) :
                case ( DMPAPER_ENV_B5 ) :
                case ( DMPAPER_ENV_B6 ) :
                case ( DMPAPER_ENV_ITALY ) :
                case ( DMPAPER_ENV_MONARCH ) :
                case ( DMPAPER_ENV_PERSONAL ) :
                case ( DMPAPER_ENV_INVITE ) :
                case ( DMPAPER_JENV_KAKU2 ) :
                case ( DMPAPER_JENV_KAKU3 ) :
                case ( DMPAPER_JENV_CHOU3 ) :
                case ( DMPAPER_JENV_CHOU4 ) :
                case ( DMPAPER_JENV_KAKU2_ROTATED ) :
                case ( DMPAPER_JENV_KAKU3_ROTATED ) :
                case ( DMPAPER_JENV_CHOU3_ROTATED ) :
                case ( DMPAPER_JENV_CHOU4_ROTATED ) :
                case ( DMPAPER_JENV_YOU4 ) :
                case ( DMPAPER_JENV_YOU4_ROTATED ) :
                case ( DMPAPER_PENV_1 ) :
                case ( DMPAPER_PENV_2 ) :
                case ( DMPAPER_PENV_3 ) :
                case ( DMPAPER_PENV_4 ) :
                case ( DMPAPER_PENV_5 ) :
                case ( DMPAPER_PENV_6 ) :
                case ( DMPAPER_PENV_7 ) :
                case ( DMPAPER_PENV_8 ) :
                case ( DMPAPER_PENV_9 ) :
                case ( DMPAPER_PENV_10 ) :
                case ( DMPAPER_PENV_1_ROTATED ) :
                case ( DMPAPER_PENV_2_ROTATED ) :
                case ( DMPAPER_PENV_3_ROTATED ) :
                case ( DMPAPER_PENV_4_ROTATED ) :
                case ( DMPAPER_PENV_5_ROTATED ) :
                case ( DMPAPER_PENV_6_ROTATED ) :
                case ( DMPAPER_PENV_7_ROTATED ) :
                case ( DMPAPER_PENV_8_ROTATED ) :
                case ( DMPAPER_PENV_9_ROTATED ) :
                case ( DMPAPER_PENV_10_ROTATED ) :
                {
                    pPD->Flags |= PI_WPAPER_ENVELOPE;
                    break;
                }
            }

            for (i = 0; i < dwNumber; i++)
            {
                if (lpPapers[i] == pPI->wPaper)
                {
                    //
                    //  In tenths of MMs.
                    //
                    *(LPPOINT)&pPI->PtPaperSizeMMs = lpPaperSize[i];
                    break;
                }
            }

            LocalFree(lpPapers);
        }
    }

    //
    //  If the paper size could not be found, use something reasonable
    //  (eg. letter).
    //
    if (!pPI->PtPaperSizeMMs.x)
    {
        pPI->PtPaperSizeMMs.x = 85 * MMS_PER_INCH / 10;
    }
    if (!pPI->PtPaperSizeMMs.y)
    {
        pPI->PtPaperSizeMMs.y = 11 * MMS_PER_INCH;
    }

    //
    //  Rotate envelopes as needed.
    //
    if ( (pPD->Flags & PI_WPAPER_ENVELOPE) &&
         (!pPI->dwRotation) &&
         (pPI->PtPaperSizeMMs.x < pPI->PtPaperSizeMMs.y) )
    {
        lTemp = pPI->PtPaperSizeMMs.x;
        pPI->PtPaperSizeMMs.x = pPI->PtPaperSizeMMs.y;
        pPI->PtPaperSizeMMs.y = lTemp;
    }

    //
    //  Maintain everything in accordance with the orientation
    //  so that apps have to do as little work as possible.
    //
    if (!bPortrait)
    {
        lTemp = pPI->PtPaperSizeMMs.x;
        pPI->PtPaperSizeMMs.x = pPI->PtPaperSizeMMs.y;
        pPI->PtPaperSizeMMs.y = lTemp;
    }

    //
    //  Set up return ptPaperSize value.
    //
    if (pPSD->Flags & PSD_INTHOUSANDTHSOFINCHES)
    {
        pPSD->ptPaperSize.x = pPI->PtPaperSizeMMs.x * 1000 / MMS_PER_INCH;
        pPSD->ptPaperSize.y = pPI->PtPaperSizeMMs.y * 1000 / MMS_PER_INCH;
    }
    else           // PSD_INHUNDREDTHSOFMILLIMETERS
    {
        pPSD->ptPaperSize.x = pPI->PtPaperSizeMMs.x * 10;
        pPSD->ptPaperSize.y = pPI->PtPaperSizeMMs.y * 10;
    }

    //
    //  Update RtMinMarginMMs and rtMinMargin for new papersize/orientation.
    //
    PrintSetMinMargins(hDlg, pPI, pDM);

    //
    //  Don't let margins overlap (page might have shrunk).
    //
    if (pPSD->rtMargin.left + pPSD->rtMargin.right > pPSD->ptPaperSize.x)
    {
        lTemp = (pPD->Flags & PSD_INTHOUSANDTHSOFINCHES) ? 1000 : MMS_PER_INCH;
        pPSD->rtMargin.left  = (pPSD->ptPaperSize.x - lTemp) / 2;
        pPSD->rtMargin.right = (pPSD->ptPaperSize.x - lTemp) / 2;
    }
    if (pPSD->rtMargin.top + pPSD->rtMargin.bottom > pPSD->ptPaperSize.y)
    {
        lTemp = (pPD->Flags & PSD_INTHOUSANDTHSOFINCHES) ? 1000 : MMS_PER_INCH;
        pPSD->rtMargin.top    = (pPSD->ptPaperSize.y - lTemp) / 2;
        pPSD->rtMargin.bottom = (pPSD->ptPaperSize.y - lTemp) / 2;
    }

    //
    //  There are new minimal margins, so adjust rtMargin
    //  (min margins might have grown).
    //
    if (pPSD->rtMargin.left < pPSD->rtMinMargin.left)
        pPSD->rtMargin.left = pPSD->rtMinMargin.left;
    if (pPSD->rtMargin.top < pPSD->rtMinMargin.top)
        pPSD->rtMargin.top = pPSD->rtMinMargin.top;
    if (pPSD->rtMargin.right < pPSD->rtMinMargin.right)
        pPSD->rtMargin.right = pPSD->rtMinMargin.right;
    if (pPSD->rtMargin.bottom < pPSD->rtMinMargin.bottom)
        pPSD->rtMargin.bottom = pPSD->rtMinMargin.bottom;

    //
    //  The margins were adjusted, so update the ui.
    //
    PrintSetMargin(hDlg, pPI, ID_SETUP_E_LEFT, pPSD->rtMargin.left);
    PrintSetMargin(hDlg, pPI, ID_SETUP_E_TOP, pPSD->rtMargin.top);
    PrintSetMargin(hDlg, pPI, ID_SETUP_E_RIGHT, pPSD->rtMargin.right);
    PrintSetMargin(hDlg, pPI, ID_SETUP_E_BOTTOM, pPSD->rtMargin.bottom);

    //
    //  Update the sample window size & shadow.
    //
    if ( (hWndSample = GetDlgItem(hDlg, ID_SETUP_W_SAMPLE)) &&
         (hWndShadowRight = GetDlgItem(hDlg, ID_SETUP_W_SHADOWRIGHT)) &&
         (hWndShadowBottom = GetDlgItem(hDlg, ID_SETUP_W_SHADOWBOTTOM)) )
    {
        int iWidth = pPI->PtPaperSizeMMs.x;
        int iLength = pPI->PtPaperSizeMMs.y;
        int iExtent;
        RECT aRtSampleXYWH = pPI->RtSampleXYWH;
        int iX = aRtSampleXYWH.right  / 16;
        int iY = aRtSampleXYWH.bottom / 16;

        if (iWidth > iLength)
        {
            iExtent = aRtSampleXYWH.bottom * iLength / iWidth;
            aRtSampleXYWH.top += (aRtSampleXYWH.bottom - iExtent) / 2;
            aRtSampleXYWH.bottom = iExtent;
        }
        else
        {
            iExtent = aRtSampleXYWH.right * iWidth / iLength;
            aRtSampleXYWH.left += (aRtSampleXYWH.right - iExtent) / 2;
            aRtSampleXYWH.right = iExtent;
        }

        SetWindowPos( hWndSample,
                      0,
                      aRtSampleXYWH.left,
                      aRtSampleXYWH.top,
                      aRtSampleXYWH.right,
                      aRtSampleXYWH.bottom,
                      SWP_NOZORDER );

        SetWindowPos( hWndShadowRight,
                      0,
                      aRtSampleXYWH.left + aRtSampleXYWH.right,
                      aRtSampleXYWH.top + iY,
                      iX,
                      aRtSampleXYWH.bottom,
                      SWP_NOZORDER );

        SetWindowPos( hWndShadowBottom,
                      0,
                      aRtSampleXYWH.left + iX,
                      aRtSampleXYWH.top + aRtSampleXYWH.bottom,
                      aRtSampleXYWH.right,
                      iY,
                      SWP_NOZORDER );

        InvalidateRect(hWndSample, NULL, TRUE);
        UpdateWindow(hDlg);
        UpdateWindow(hWndSample);
        UpdateWindow(hWndShadowRight);
        UpdateWindow(hWndShadowBottom);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitDuplex
//
//  Enable/Disable Paper Duplexing controls.
//
//  Returns TRUE iff buttons used to be disabled, now enabled.
//  Returns FALSE otherwise.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintInitDuplex(
    HWND hDlg,
    LPDEVMODE pDM)
{
    BOOL bEnable;
    HWND hCtl;


    bEnable = (pDM->dmFields & DM_DUPLEX);

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_G_DUPLEX))
    {
        EnableWindow(hCtl, bEnable);
    }
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_NONE))
    {
        EnableWindow(hCtl, bEnable);
    }
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_LONG))
    {
        EnableWindow(hCtl, bEnable);
    }
    if (hCtl = GetDlgItem(hDlg, ID_SETUP_R_SHORT))
    {
        EnableWindow(hCtl, bEnable);
    }

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_I_DUPLEX))
    {
        SetWindowLong( hCtl,
                       GWL_STYLE,
                       GetWindowLong(hCtl, GWL_STYLE) | SS_CENTERIMAGE );
        if (!bEnable)
        {
            ShowWindow(hCtl, SW_HIDE);
            SendMessage(hCtl, STM_SETICON, (LONG_PTR)hIconPDuplexNone, 0L);
            ShowWindow(hCtl, SW_SHOW);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintSetDuplex
//
//  This routine will operate on pDocDetails->pDMInput PSDEVMODE structure,
//  making sure that is a structure we know about and can handle.
//
//  If the pd doesn't have DM_DUPLEX caps then just display the appropriate
//  paper icon for DMDUP_SIMPLEX (case where nRad = ID_SETUP_R_NONE).
//
//  If nRad = 0, update icon but don't change radio button.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintSetDuplex(
    HWND hDlg,
    LPDEVMODE pDM,
    UINT nRad)
{
    BOOL bPortrait;
    HANDLE hDuplexIcon;
    HWND hCtl;


    bPortrait = (pDM->dmOrientation == DMORIENT_PORTRAIT);

    if (!(pDM->dmFields & DM_DUPLEX))
    {
        nRad = ID_SETUP_R_NONE;
    }

    //
    //  Boundary checking - default to ID_SETUP_R_NONE.
    //
    if (GetDlgItem(hDlg, ID_SETUP_R_NONE))
    {
        if ((nRad < ID_SETUP_R_NONE) || (nRad > ID_SETUP_R_SHORT))
        {
            if (IsDlgButtonChecked(hDlg, ID_SETUP_R_SHORT))
            {
                nRad = ID_SETUP_R_SHORT;
            }
            else if (IsDlgButtonChecked(hDlg, ID_SETUP_R_LONG))
            {
                nRad = ID_SETUP_R_LONG;
            }
            else
            {
                nRad = ID_SETUP_R_NONE;
            }
        }
        else
        {
            CheckRadioButton(hDlg, ID_SETUP_R_NONE, ID_SETUP_R_SHORT, nRad);
        }
    }

    if (hCtl = GetDlgItem(hDlg, ID_SETUP_I_DUPLEX))
    {
        switch (nRad)
        {
            case ( ID_SETUP_R_LONG ) :      // Long Side - 2 sided printing
            {
                pDM->dmDuplex = DMDUP_VERTICAL;
                hDuplexIcon = bPortrait ? hIconPDuplexNoTumble : hIconLDuplexTumble;

                break;
            }
            case ( ID_SETUP_R_SHORT ) :     // Short Side - 2 sided printing
            {
                pDM->dmDuplex = DMDUP_HORIZONTAL;
                hDuplexIcon = bPortrait ? hIconPDuplexTumble : hIconLDuplexNoTumble;

                break;
            }
            default :                       // None - 2 sided printing
            {
                pDM->dmDuplex = DMDUP_SIMPLEX;
                hDuplexIcon = bPortrait ? hIconPDuplexNone : hIconLDuplexNone;

                break;
            }
        }

        //
        //  Set the appropriate icon.
        //
        ShowWindow(hCtl, SW_HIDE);
        SendMessage(hCtl, STM_SETICON, (LONG_PTR)hDuplexIcon, 0L);
        ShowWindow(hCtl, SW_SHOW);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintInitPaperCombo
//
////////////////////////////////////////////////////////////////////////////

VOID PrintInitPaperCombo(
    PPRINTINFO pPI,
    HWND hCmb,
    HWND hStc,
    PPRINTER_INFO_2 pPrinter,
    LPDEVMODE pDM,
    WORD fwCap1,
    WORD cchSize1,
    WORD fwCap2)
{
    DWORD cStr1, cStr2, cRet1, cRet2, i;
    LPTSTR lpsOut1;
    LPWORD lpwOut2;
    BOOL fFill;


    HourGlass(TRUE);

    SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);

    cStr1 = DeviceCapabilities( pPrinter->pPrinterName,
                                pPrinter->pPortName,
                                fwCap1,
                                NULL,
                                pDM );

    cStr2 = DeviceCapabilities( pPrinter->pPrinterName,
                                pPrinter->pPortName,
                                fwCap2,
                                NULL,
                                pDM );

    //
    //  Check for error from DeviceCapabilities calls.  If either
    //  call failed, simply set cStr1 to be 0 so that the windows will be
    //  disabled and nothing will be initialized.
    //
    if ((cStr1 == (DWORD)(-1)) || (cStr2 == (DWORD)(-1)))
    {
        cStr1 = 0;
    }

    fFill = (cStr1 > 0) && (cStr1 == cStr2);

    if (!((pPI->pPSD) && (pPI->pPSD->Flags & PSD_DISABLEPAPER)))
    {
        //
        //  If no entries, disable hCmb and hStc.
        //
        EnableWindow(hCmb, fFill);
        EnableWindow(hStc, fFill);
    }

    if (fFill)
    {
        lpsOut1 = LocalAlloc(LPTR, cStr1 * cchSize1 * sizeof(TCHAR));

        lpwOut2 = LocalAlloc(LPTR, cStr2 * sizeof(WORD));

        if (lpsOut1 && lpwOut2)
        {
            cRet1 = DeviceCapabilities( pPrinter->pPrinterName,
                                        pPrinter->pPortName,
                                        fwCap1,
                                        (LPTSTR)lpsOut1,
                                        pDM );

            cRet2 = DeviceCapabilities( pPrinter->pPrinterName,
                                        pPrinter->pPortName,
                                        fwCap2,
                                        (LPTSTR)lpwOut2,
                                        pDM );

            if ((pPI->dwRotation =
                    DeviceCapabilities( pPrinter->pPrinterName,
                                        pPrinter->pPortName,
                                        DC_ORIENTATION,
                                        NULL,
                                        pDM )) == (DWORD)(-1))
            {
                pPI->dwRotation = 0;
            }

            if ((cRet1 == cStr1) && (cRet2 == cStr2))
            {
                LPTSTR lpsT1 = lpsOut1;
                LPWORD lpwT2 = lpwOut2;
                int nInd;
                LPTSTR lpFound = NULL;
                LPTSTR lpFirst = NULL;

                for (i = 0; i < cRet1; i++, lpsT1 += cchSize1, lpwT2++)
                {
#ifndef WINNT
                    LPTSTR lpsT3;
                    LPWORD lpwT4;

                    //
                    //  Various checks for HP LaserJet and PostScript
                    //  driver bugs.
                    //
                    //
                    //  Look for duplicate names.
                    //
                    for (lpsT3 = lpsOut1; lpsT3 <= lpsT1; lpsT3 += cchSize1)
                    {
                        if (!lstrcmp(lpsT3, lpsT1))
                        {
                            break;
                        }
                    }
                    if (lpsT3 != lpsT1)
                    {
                        //
                        //  Duplicate found, so ignore.
                        //
                        continue;
                    }

                    //
                    //  Look for duplicate values.
                    //
                    for (lpwT4 = lpwOut2; lpwT4 <= lpwT2; lpwT4++)
                    {
                        if (*lpwT4 == *lpwT2)
                        {
                            break;
                        }
                    }
                    if (lpwT4 != lpwT2)
                    {
                        //
                        //  Duplicate found, so ignore.
                        //
                        continue;
                    }
#endif
                    //
                    //  Look for a blank name entry.
                    //
                    if (!*lpsT1)
                    {
                        //
                        //  Blank entry, so ignore.
                        //
                        continue;
                    }

                    //
                    //  Add the string to the list box.
                    //
                    nInd = (int) SendMessage( hCmb,
                                              CB_ADDSTRING,
                                              0,
                                              (LPARAM)lpsT1 );
                    if (nInd != CB_ERR)
                    {
                        //
                        //  Set the data associated with the string that
                        //  was just added to the list box.
                        //
                        SendMessage( hCmb,
                                     CB_SETITEMDATA,
                                     nInd,
                                     (LPARAM)*lpwT2 );

                        //
                        //  See if this item should be selected.
                        //
                        if (!lpFound)
                        {
                            if (!lpFirst)
                            {
                                lpFirst = lpsT1;
                            }

                            if ( (fwCap1 == DC_PAPERNAMES) &&
                                 (pDM->dmFields & DM_PAPERSIZE) &&
                                 (pDM->dmPaperSize == (SHORT)*lpwT2) )
                            {
                                lpFound = lpsT1;
                            }
                            else if ( (fwCap1 == DC_BINNAMES) &&
                                      (pDM->dmFields & DM_DEFAULTSOURCE) &&
                                      (pDM->dmDefaultSource == (SHORT)*lpwT2) )
                            {
                               lpFound = lpsT1;
                            }
                        }
                    }
                }

                //
                //  Set the appropriate selection.
                //
                if (lpFound)
                {
                    SendMessage( hCmb,
                                 CB_SETCURSEL,
                                 SendMessage( hCmb,
                                              CB_FINDSTRINGEXACT,
                                              (WPARAM)-1,
                                              (LPARAM)lpFound ),
                                 0 );
                }
                else
                {
                    if (fwCap1 == DC_PAPERNAMES)
                    {
                        //
                        //  Check for a default FORM name.
                        //
                        if (!( (pDM->dmFields & DM_FORMNAME) &&
                               ((nInd = (int) 
                                   SendMessage( hCmb,
                                                CB_SELECTSTRING,
                                                (WPARAM)-1,
                                                (LPARAM)pDM->dmFormName )) != CB_ERR) ))
                        {
                            //
                            //  Always select the first *enumerated* entry
                            //  if no other selection was found.
                            //
                            SendMessage( hCmb,
                                         CB_SETCURSEL,
                                         (lpFirst)
                                           ? SendMessage( hCmb,
                                                          CB_FINDSTRINGEXACT,
                                                          (WPARAM)-1,
                                                          (LPARAM)lpFirst )
                                           : 0,
                                         0 );
                        }
                        else
                        {
                            //
                            //  Save the paper size since the form name exists
                            //  in the list box.
                            //
                        //  pDM->dmFields |= DM_PAPERSIZE;
                            pDM->dmPaperSize =
                                (SHORT)SendMessage( hCmb,
                                                    CB_GETITEMDATA,
                                                    nInd,
                                                    0 );
                        }
                    }
                    else
                    {
                        //
                        //  Set the SOURCE to the Default if it exists.
                        //
                        nInd = (int) SendMessage( hCmb,
                                                  CB_SELECTSTRING,
                                                  (WPARAM)-1,
                                                  (LPARAM)szDefaultSrc );
                        if (nInd != CB_ERR)
                        {
                        //  pDM->dmFields |= DM_DEFAULTSOURCE;
                            pDM->dmDefaultSource =
                                (SHORT)SendMessage( hCmb,
                                                    CB_GETITEMDATA,
                                                    nInd,
                                                    0 );
                        }
                        else
                        {
                            //
                            //  Always select the first *enumerated* entry
                            //  if no other selection was found.
                            //
                            SendMessage( hCmb,
                                         CB_SETCURSEL,
                                         (lpFirst)
                                           ? SendMessage( hCmb,
                                                          CB_FINDSTRINGEXACT,
                                                          (WPARAM)-1,
                                                          (LPARAM)lpFirst )
                                           : 0,
                                         0 );
                        }
                    }
                }
            }
        }
        if (lpsOut1)
        {
            LocalFree((HLOCAL)lpsOut1);
        }

        if (lpwOut2)
        {
            LocalFree((HLOCAL)lpwOut2);
        }
    }

    HourGlass(FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintEditError
//
//  Set focus to an edit control and select the entire contents.
//  This is generally used when an improper value was found at OK time.
//
//  Assumes edit control not disabled.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintEditError(
    HWND hDlg,
    int Id,
    UINT MessageId,
    ...)
{
    HWND hEdit;
    TCHAR pszTitle[MAX_PATH];
    TCHAR pszFormat[MAX_PATH];
    TCHAR pszMessage[MAX_PATH];


    //
    //  Put up the error message box.
    //
    if ( GetWindowText(hDlg, pszTitle, MAX_PATH) &&
         CDLoadString(g_hinst, MessageId, pszFormat, MAX_PATH) )
    {
        va_list ArgList;

        va_start(ArgList, MessageId);
        wvsprintf(pszMessage, pszFormat, ArgList);
        va_end(ArgList);
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBox(hDlg, pszMessage, pszTitle, MB_ICONEXCLAMATION | MB_OK);
    }

    //
    //  Highlight the invalid value.
    //
    if (hEdit = ((Id == 0) ? NULL : GetDlgItem(hDlg, Id)))
    {
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hEdit, 1L);
        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintOpenPrinter
//
//  If the OpenPrinter call is successful, this sets hPrinter, pPrinter,
//  cPrinters, and pCurPrinter.
//
////////////////////////////////////////////////////////////////////////////

VOID PrintOpenPrinter(
    PPRINTINFO pPI,
    LPTSTR pPrinterName)
{
    if (OpenPrinter(pPrinterName, &pPI->hCurPrinter, NULL))
    {
        if (pPI->pPrinters = PrintGetPrinterInfo2(pPI->hCurPrinter))
        {
            pPI->cPrinters = 1;

#ifdef UNICODE
            if (pPI->bUseExtDeviceMode)
            {
                PrintGetExtDeviceMode(NULL, pPI);
            }
#endif
        }
        pPI->pCurPrinter = pPI->pPrinters;
    }
    else
    {
        //
        //  Cannot trust the OpenPrinter call.
        //
        pPI->hCurPrinter = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  PrintClosePrinters
//
////////////////////////////////////////////////////////////////////////////

BOOL PrintClosePrinters(
    PPRINTINFO pPI)
{
    if (pPI->hCurPrinter)
    {
        ClosePrinter(pPI->hCurPrinter);
        pPI->hCurPrinter = 0;
    }
    pPI->pCurPrinter = NULL;

    FreePrinterArray(pPI);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateSpoolerInfo
//
////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE

VOID UpdateSpoolerInfo(
    PPRINTINFO pPI)
{
    LPDEVMODEA pDMA;
    CHAR szPrinterNameA[33];
    LPDEVMODEW pDMW;


    //
    //  Get a pointer to the devmode structure.
    //
    pDMW = GlobalLock(pPI->pPD->hDevMode);
    if ((!pDMW) || (!pPI->pCurPrinter))
    {
        return;
    }

    //
    //  Convert the printer name from Unicode to ANSI.
    //
    SHUnicodeToAnsi(pPI->pCurPrinter->pPrinterName, szPrinterNameA,32);

    //
    //  Allocate and convert the Unicode devmode to ANSI.
    //
    pDMA = AllocateAnsiDevMode(pDMW);
    if (!pDMA)
    {
        GlobalUnlock(pPI->pPD->hDevMode);
        return;
    }

    //
    //  Update the spooler's information.
    //
    ExtDeviceMode( NULL,
                   NULL,
                   NULL,
                   szPrinterNameA,
                   NULL,
                   pDMA,
                   NULL,
                   DM_UPDATE | DM_MODIFY );

    //
    //  Free the buffer.
    //
    GlobalFree(pDMA);
    GlobalUnlock(pPI->pPD->hDevMode);
}
#endif


////////////////////////////////////////////////////////////////////////////
//
//  PrintGetPrinterInfo2
//
////////////////////////////////////////////////////////////////////////////

PPRINTER_INFO_2 PrintGetPrinterInfo2(
    HANDLE hPrinter)
{
    PPRINTER_INFO_2 pPrinter = NULL;
    DWORD cbPrinter = 0;


    StoreExtendedError(CDERR_GENERALCODES);

    if (!GetPrinter(hPrinter, 2, NULL, 0, &cbPrinter) &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        if (pPrinter = GlobalAlloc(GPTR, cbPrinter))
        {
            if (!GetPrinter( hPrinter,
                             2,
                             (LPBYTE)pPrinter,
                             cbPrinter,
                             &cbPrinter ))
            {
                GlobalFree(pPrinter);
                pPrinter = NULL;
                StoreExtendedError(PDERR_PRINTERNOTFOUND);
            }
        }
        else
        {
            StoreExtendedError(CDERR_MEMALLOCFAILURE);
        }
    }
    else
    {
        StoreExtendedError(PDERR_SETUPFAILURE);
    }

    return (pPrinter);
}


////////////////////////////////////////////////////////////////////////////
//
//  ConvertStringToInteger
//
//  Converts a string to an integer.  Stops at the first non digit.
//
////////////////////////////////////////////////////////////////////////////

int ConvertStringToInteger(
    LPCTSTR pSrc)
{
    int Number = 0;
    BOOL bNeg = FALSE;


    if (*pSrc == TEXT('-'))
    {
        bNeg = TRUE;
        pSrc++;
    }

    while (ISDIGIT(*pSrc))
    {
        Number *= 10;
        Number += *pSrc - TEXT('0');
        pSrc++;
    }

    return ( bNeg ? -Number : Number );
}


////////////////////////////////////////////////////////////////////////////
//
//  FreePrinterArray
//
//  Purpose:    Frees the buffer allocated to store printers.
//
//  Parameters: PPRINTINFO pPI
//
//  Return:     void
//
////////////////////////////////////////////////////////////////////////////

VOID FreePrinterArray(
    PPRINTINFO pPI)
{
    PPRINTER_INFO_2 pPrinters = pPI->pPrinters;
#ifdef UNICODE
    DWORD dwCount;
#endif
    //
    //  If NULL, we can return now.
    //
    if (!pPrinters)
    {
        return;
    }

#ifdef UNICODE
    //
    //  If we made calls to ExtDeviceMode, then we need to
    //  free the buffers allocated for each devmode.
    //
    if (pPI->bUseExtDeviceMode)
    {
        if (pPI->pAllocInfo)
        {
            //
            //  Loop through each of the printers.
            //
            for (dwCount = 0; dwCount < pPI->cPrinters; dwCount++)
            {
                //
                //  If pDevMode exists, free it.
                //
                if ((pPrinters[dwCount].pDevMode) &&
                    (pPI->pAllocInfo[dwCount]))
                {
                    GlobalFree(pPrinters[dwCount].pDevMode);
                    pPrinters[dwCount].pDevMode = NULL;
                }
            }
            GlobalFree(pPI->pAllocInfo);
            pPI->pAllocInfo = NULL;
        }
    }
#endif

    //
    //  Free the entire block.
    //
    GlobalFree(pPI->pPrinters);
    pPI->pPrinters = NULL;
    pPI->cPrinters = 0;
}


////////////////////////////////////////////////////////////////////////////
//
//  TermPrint
//
////////////////////////////////////////////////////////////////////////////

VOID TermPrint(void)
{
#ifdef WINNT
    Print_UnloadLibraries();           // printnew.cpp
#endif
}





/*========================================================================*/
/*                   Page Setup <-> Print Dialog                          */
/*========================================================================*/


////////////////////////////////////////////////////////////////////////////
//
//  TransferPSD2PD
//
////////////////////////////////////////////////////////////////////////////

VOID TransferPSD2PD(
    PPRINTINFO pPI)
{
    if (pPI->pPSD && pPI->pPD)
    {
        pPI->pPD->hDevMode  = pPI->pPSD->hDevMode;
        pPI->pPD->hDevNames = pPI->pPSD->hDevNames;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  TransferPD2PSD
//
////////////////////////////////////////////////////////////////////////////

VOID TransferPD2PSD(
    PPRINTINFO pPI)
{
    if (pPI->pPSD && pPI->pPD)
    {
        pPI->pPSD->hDevMode  = pPI->pPD->hDevMode;
        pPI->pPSD->hDevNames = pPI->pPD->hDevNames;
    }
}


#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  TransferPSD2PDA
//
////////////////////////////////////////////////////////////////////////////

VOID TransferPSD2PDA(
    PPRINTINFO pPI)
{
    if (pPI->pPSD && pPI->pPDA)
    {
        pPI->pPDA->hDevMode  = pPI->pPSD->hDevMode;
        pPI->pPDA->hDevNames = pPI->pPSD->hDevNames;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  TransferPDA2PSD
//
////////////////////////////////////////////////////////////////////////////

VOID TransferPDA2PSD(
    PPRINTINFO pPI)
{
    if (pPI->pPSD && pPI->pPDA)
    {
        pPI->pPSD->hDevMode  = pPI->pPDA->hDevMode;
        pPI->pPSD->hDevNames = pPI->pPDA->hDevNames;
    }
}

#endif





/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkPageSetupDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkPageSetupDlg(
    PPRINTINFO pPI,
    LPPAGESETUPDLGA pPSDA)
{
    LPPRINTDLGA pPDA;


    if (!pPSDA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pPSDA->lStructSize != sizeof(PAGESETUPDLGA))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if ((pPSDA->Flags & PSD_RETURNDEFAULT) &&
        (pPSDA->hDevNames || pPSDA->hDevMode))
    {
        StoreExtendedError(PDERR_RETDEFFAILURE);
        return (FALSE);
    }

    //
    //  Reset the size of the pPSD structure to the UNICODE size and
    //  save it in the pPI structure.
    //
    //  NOTE:  This must be reset back to the ANSI size before
    //         returning to the caller.
    //
    pPSDA->lStructSize = sizeof(PAGESETUPDLGW);
    pPI->pPSD = (LPPAGESETUPDLG)pPSDA;
    pPI->ApiType = COMDLG_ANSI;

    //
    //  Create the ANSI version of the print dialog structure.
    //
    if (pPDA = GlobalAlloc(GPTR, sizeof(PRINTDLGA)))
    {
        pPI->pPDA = pPDA;

        pPDA->lStructSize         = sizeof(PRINTDLGA);
        pPDA->hwndOwner           = pPSDA->hwndOwner;
        pPDA->Flags               = PD_PAGESETUP |
                                      (pPSDA->Flags &
                                        (PSD_NOWARNING |
                                         PSD_SHOWHELP |
                                         PSD_ENABLEPAGESETUPHOOK |
                                         PSD_ENABLEPAGESETUPTEMPLATE |
                                         PSD_ENABLEPAGESETUPTEMPLATEHANDLE |
                                         CD_WX86APP |
                                         PSD_NONETWORKBUTTON));
        pPDA->hInstance           = pPSDA->hInstance;
        pPDA->lCustData           = pPSDA->lCustData;
        pPDA->lpfnSetupHook       = pPSDA->lpfnPageSetupHook;
        pPDA->lpSetupTemplateName = pPSDA->lpPageSetupTemplateName;
        pPDA->hSetupTemplate      = pPSDA->hPageSetupTemplate;

        pPDA->hDevMode            = pPSDA->hDevMode;
        pPDA->hDevNames           = pPSDA->hDevNames;
    }
    else
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeThunkPageSetupDlg
//
////////////////////////////////////////////////////////////////////////////

VOID FreeThunkPageSetupDlg(
    PPRINTINFO pPI)
{
    //
    //  Reset the size of the pPSD structure to the correct value.
    //
    if (pPI->pPSD)
    {
        pPI->pPSD->lStructSize = sizeof(PAGESETUPDLGA);
    }

    //
    //  Free the ANSI print dialog structure.
    //
    if (pPI->pPDA)
    {
        GlobalFree(pPI->pPDA);
        pPI->pPDA = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlg
//
////////////////////////////////////////////////////////////////////////////

BOOL ThunkPrintDlg(
    PPRINTINFO pPI,
    LPPRINTDLGA pPDA)
{
    LPPRINTDLGW pPDW;
    LPDEVNAMES pDNA;
    LPDEVMODEA pDMA;
    DWORD cbLen;


    if (!pPDA)
    {
        StoreExtendedError(CDERR_INITIALIZATION);
        return (FALSE);
    }

    if (pPDA->lStructSize != sizeof(PRINTDLGA))
    {
        StoreExtendedError(CDERR_STRUCTSIZE);
        return (FALSE);
    }

    if (!(pPDW = GlobalAlloc(GPTR, sizeof(PRINTDLGW))))
    {
        StoreExtendedError(CDERR_MEMALLOCFAILURE);
        return (FALSE);
    }

    //
    //  IN-only constant stuff.
    //
    pPDW->lStructSize    = sizeof(PRINTDLGW);
    pPDW->hwndOwner      = pPDA->hwndOwner;
    pPDW->hInstance      = pPDA->hInstance;
    pPDW->lpfnPrintHook  = pPDA->lpfnPrintHook;
    pPDW->lpfnSetupHook  = pPDA->lpfnSetupHook;
    pPDW->hPrintTemplate = pPDA->hPrintTemplate;
    pPDW->hSetupTemplate = pPDA->hSetupTemplate;

    //
    //  IN-OUT Variable Structs.
    //
    if ((pPDA->hDevMode) && (pDMA = GlobalLock(pPDA->hDevMode)))
    {
        //
        //  Make sure the device name in the devmode is not too long such that
        //  it has overwritten the other devmode fields.
        //
        if ((pDMA->dmSize < MIN_DEVMODE_SIZEA) ||
            (lstrlenA(pDMA->dmDeviceName) > CCHDEVICENAME))
        {
            pPDW->hDevMode = NULL;
        }
        else
        {
            pPDW->hDevMode = GlobalAlloc( GHND,
                                          sizeof(DEVMODEW) + pDMA->dmDriverExtra );
        }
        GlobalUnlock(pPDA->hDevMode);
    }
    else
    {
        pPDW->hDevMode = NULL;
    }

    if ((pPDA->hDevNames) && (pDNA = GlobalLock(pPDA->hDevNames)))
    {
        cbLen = lstrlenA((LPSTR)pDNA + pDNA->wOutputOffset) + 1 +
                lstrlenA((LPSTR)pDNA + pDNA->wDriverOffset) + 1 +
                lstrlenA((LPSTR)pDNA + pDNA->wDeviceOffset) + 1 +
                DN_PADDINGCHARS;

        cbLen *= sizeof(WCHAR);
        cbLen += sizeof(DEVNAMES);
        pPDW->hDevNames = GlobalAlloc(GHND, cbLen);
        GlobalUnlock(pPDA->hDevNames);
    }
    else
    {
        pPDW->hDevNames = NULL;
    }

    //
    //  IN-only constant strings.
    //
    //  Init Print TemplateName constant.
    //
    if ((pPDA->Flags & PD_ENABLEPRINTTEMPLATE) && (pPDA->lpPrintTemplateName))
    {
        //
        //  See if it's a string or an integer.
        //
        if (!IS_INTRESOURCE(pPDA->lpPrintTemplateName))
        {
            //
            //  String.
            //
            cbLen = lstrlenA(pPDA->lpPrintTemplateName) + 1;
            if (!(pPDW->lpPrintTemplateName =
                     GlobalAlloc( GPTR,
                                  (cbLen * sizeof(WCHAR)) )))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                return (FALSE);
            }
            else
            {
                pPI->fPrintTemplateAlloc = TRUE;
                SHAnsiToUnicode(pPDA->lpPrintTemplateName,(LPWSTR)pPDW->lpPrintTemplateName, cbLen);
            }
        }
        else
        {
            //
            //  Integer.
            //
            (DWORD_PTR)pPDW->lpPrintTemplateName = (DWORD_PTR)pPDA->lpPrintTemplateName;
        }
    }
    else
    {
        pPDW->lpPrintTemplateName = NULL;
    }

    //
    //  Init Print Setup TemplateName constant.
    //
    if ((pPDA->Flags & PD_ENABLESETUPTEMPLATE) && (pPDA->lpSetupTemplateName))
    {
        //
        //  See if it's a string or an integer.
        //
        if (!IS_INTRESOURCE(pPDA->lpSetupTemplateName))
        {
            //
            //  String.
            //
            cbLen = lstrlenA(pPDA->lpSetupTemplateName) + 1;
            if (!(pPDW->lpSetupTemplateName =
                      GlobalAlloc( GPTR,
                                   (cbLen * sizeof(WCHAR)) )))
            {
                StoreExtendedError(CDERR_MEMALLOCFAILURE);
                return (FALSE);
            }
            else
            {
                pPI->fSetupTemplateAlloc = TRUE;
                SHAnsiToUnicode(pPDA->lpSetupTemplateName,(LPWSTR)pPDW->lpSetupTemplateName,cbLen);
            }
        }
        else
        {
            //
            //  Integer.
            //
            (DWORD_PTR)pPDW->lpSetupTemplateName = (DWORD_PTR)pPDA->lpSetupTemplateName;
        }
    }
    else
    {
        pPDW->lpSetupTemplateName = NULL;
    }

    pPI->pPD = pPDW;
    pPI->pPDA = pPDA;
    pPI->ApiType = COMDLG_ANSI;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeThunkPrintDlg
//
////////////////////////////////////////////////////////////////////////////

VOID FreeThunkPrintDlg(
    PPRINTINFO pPI)
{
    LPPRINTDLGW pPDW = pPI->pPD;


    if (!pPDW)
    {
        return;
    }

    if (pPDW->hDevNames)
    {
        GlobalFree(pPDW->hDevNames);
    }

    if (pPDW->hDevMode)
    {
        GlobalFree(pPDW->hDevMode);
    }

    if (pPI->fPrintTemplateAlloc)
    {
        GlobalFree((LPWSTR)(pPDW->lpPrintTemplateName));
    }

    if (pPI->fSetupTemplateAlloc)
    {
        GlobalFree((LPWSTR)(pPDW->lpSetupTemplateName));
    }

    GlobalFree(pPDW);
    pPI->pPD = NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlgA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkPrintDlgA2W(
    PPRINTINFO pPI)
{
    LPPRINTDLGW pPDW = pPI->pPD;
    LPPRINTDLGA pPDA = pPI->pPDA;


    //
    //  Copy info A => W
    //
    pPDW->hDC           = pPDA->hDC;
    pPDW->Flags         = pPDA->Flags;
    pPDW->nFromPage     = pPDA->nFromPage;
    pPDW->nToPage       = pPDA->nToPage;
    pPDW->nMinPage      = pPDA->nMinPage;
    pPDW->nMaxPage      = pPDA->nMaxPage;
    pPDW->nCopies       = pPDA->nCopies;
    pPDW->lCustData     = pPDA->lCustData;
    pPDW->lpfnPrintHook = pPDA->lpfnPrintHook;
    pPDW->lpfnSetupHook = pPDA->lpfnSetupHook;

    //
    //  Thunk Device Names A => W
    //
    if (pPDA->hDevNames && pPDW->hDevNames)
    {
        LPDEVNAMES pDNA = GlobalLock(pPDA->hDevNames);
        LPDEVNAMES pDNW = GlobalLock(pPDW->hDevNames);

        ThunkDevNamesA2W(pDNA, pDNW);

        GlobalUnlock(pPDW->hDevNames);
        GlobalUnlock(pPDA->hDevNames);
    }

    //
    //  Thunk Device Mode A => W
    //
    if (pPDA->hDevMode && pPDW->hDevMode)
    {
        LPDEVMODEW pDMW = GlobalLock(pPDW->hDevMode);
        LPDEVMODEA pDMA = GlobalLock(pPDA->hDevMode);

        ThunkDevModeA2W(pDMA, pDMW);

        GlobalUnlock(pPDW->hDevMode);
        GlobalUnlock(pPDA->hDevMode);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlgW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkPrintDlgW2A(
    PPRINTINFO pPI)
{
    LPPRINTDLGA pPDA = pPI->pPDA;
    LPPRINTDLGW pPDW = pPI->pPD;
    DWORD cbLen;


    //
    //  Copy info W => A
    //
    pPDA->hDC           = pPDW->hDC;
    pPDA->Flags         = pPDW->Flags;
    pPDA->nFromPage     = pPDW->nFromPage;
    pPDA->nToPage       = pPDW->nToPage;
    pPDA->nMinPage      = pPDW->nMinPage;
    pPDA->nMaxPage      = pPDW->nMaxPage;
    pPDA->nCopies       = pPDW->nCopies;
    pPDA->lCustData     = pPDW->lCustData;
    pPDA->lpfnPrintHook = pPDW->lpfnPrintHook;
    pPDA->lpfnSetupHook = pPDW->lpfnSetupHook;

    //
    //  Thunk Device Names W => A
    //
    if (pPDW->hDevNames)
    {
        LPDEVNAMES pDNW = GlobalLock(pPDW->hDevNames);
        LPDEVNAMES pDNA;

        cbLen = lstrlenW((LPWSTR)pDNW + pDNW->wOutputOffset) + 1 +
                lstrlenW((LPWSTR)pDNW + pDNW->wDriverOffset) + 1 +
                lstrlenW((LPWSTR)pDNW + pDNW->wDeviceOffset) + 1 +
                DN_PADDINGCHARS;
        cbLen += sizeof(DEVNAMES);
        if (pPDA->hDevNames)
        {
            HANDLE handle;
            
            handle = GlobalReAlloc(pPDA->hDevNames, cbLen, GHND);
            //Make sure realloc succeeded.
            if (handle)
            {
                pPDA->hDevNames = handle;
            }
            else
            {
                //Realloc didn't succeed. Free the memory occupied
                GlobalFree(pPDA->hDevNames);
            }
        }
        else
        {
            pPDA->hDevNames = GlobalAlloc(GHND, cbLen);
        }
        if (pPDA->hDevNames)
        {
            pDNA = GlobalLock(pPDA->hDevNames);
            ThunkDevNamesW2A(pDNW, pDNA);
            GlobalUnlock(pPDA->hDevNames);
        }
        GlobalUnlock(pPDW->hDevNames);
    }

    //
    //  Thunk Device Mode W => A
    //
    if (pPDW->hDevMode)
    {
        LPDEVMODEW pDMW = GlobalLock(pPDW->hDevMode);
        LPDEVMODEA pDMA;

        if (pPDA->hDevMode)
        {
            HANDLE  handle;
            handle = GlobalReAlloc( pPDA->hDevMode,
                                            sizeof(DEVMODEA) + pDMW->dmDriverExtra,
                                            GHND );
            //Make sure realloc succeeded.
            if (handle)
            {
                pPDA->hDevMode = handle;
            }
            else
            {
                //Realloc didn't succeed. Free the memory occupied
                GlobalFree(pPDA->hDevMode);
            }

        }
        else
        {
            pPDA->hDevMode = GlobalAlloc( GHND,
                                          sizeof(DEVMODEA) + pDMW->dmDriverExtra );
        }
        if (pPDA->hDevMode)
        {
            pDMA = GlobalLock(pPDA->hDevMode);
            ThunkDevModeW2A(pDMW, pDMA);
            GlobalUnlock(pPDA->hDevMode);
        }
        GlobalUnlock(pPDW->hDevMode);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkDevNamesA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkDevNamesA2W(
    LPDEVNAMES pDNA,
    LPDEVNAMES pDNW)
{
    LPSTR lpTempA;
    LPWSTR lpTempW;

    if (!pDNA || !pDNW)
    {
        return;
    }

    //
    //  Note: Offsets are in CHARS not bytes.
    //
    pDNW->wDriverOffset = sizeof(DEVNAMES) / sizeof(WCHAR);
    lpTempW = (LPWSTR)pDNW + pDNW->wDriverOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wDriverOffset;
    SHAnsiToUnicode(lpTempA, lpTempW, lstrlenA(lpTempA) + 1);

    pDNW->wDeviceOffset = pDNW->wDriverOffset + lstrlenW(lpTempW) + 1;
    lpTempW = (LPWSTR)pDNW + pDNW->wDeviceOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wDeviceOffset;
    SHAnsiToUnicode(lpTempA, lpTempW, lstrlenA(lpTempA) + 1);


    pDNW->wOutputOffset = pDNW->wDeviceOffset + lstrlenW(lpTempW) + 1;
    lpTempW = (LPWSTR)pDNW + pDNW->wOutputOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wOutputOffset;
    SHAnsiToUnicode(lpTempA, lpTempW, lstrlenA(lpTempA) + 1);

    pDNW->wDefault = pDNA->wDefault;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkDevNamesW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkDevNamesW2A(
    LPDEVNAMES pDNW,
    LPDEVNAMES pDNA)
{
    LPSTR lpTempA;
    LPWSTR lpTempW;

    if (!pDNW || !pDNA)
    {
        return;
    }

    pDNA->wDriverOffset = sizeof(DEVNAMES);
    lpTempW = (LPWSTR)pDNW + pDNW->wDriverOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wDriverOffset;
    SHUnicodeToAnsi(lpTempW,lpTempA,(lstrlenW(lpTempW) + 1) * 2);


    pDNA->wDeviceOffset = pDNA->wDriverOffset + lstrlenA(lpTempA) + 1;
    lpTempW = (LPWSTR)pDNW + pDNW->wDeviceOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wDeviceOffset;
    SHUnicodeToAnsi(lpTempW,lpTempA,(lstrlenW(lpTempW) + 1) * 2);

    pDNA->wOutputOffset = pDNA->wDeviceOffset + lstrlenA(lpTempA) + 1;
    lpTempW = (LPWSTR)pDNW + pDNW->wOutputOffset;
    lpTempA = (LPSTR)pDNA + pDNA->wOutputOffset;
    SHUnicodeToAnsi(lpTempW,lpTempA,(lstrlenW(lpTempW) + 1) * 2);

    pDNA->wDefault = pDNW->wDefault;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkDevModeA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkDevModeA2W(
    LPDEVMODEA pDMA,
    LPDEVMODEW pDMW)
{
    LPDEVMODEA pDevModeA;

    if (!pDMA || !pDMW)
    {
        return;
    }

    //
    //  Make sure the device name in the devmode is not too long such that
    //  it has overwritten the other devmode fields.
    //
    if ((pDMA->dmSize < MIN_DEVMODE_SIZEA) ||
        (lstrlenA(pDMA->dmDeviceName) > CCHDEVICENAME))
    {
        return;
    }

    //
    //  We need to create a temporary ANSI that is a known size.
    //  The problem is if we are being called from WOW, the WOW
    //  app could be either a Windows 3.1 or 3.0 app.  The size
    //  of the devmode structure was different for both of these
    //  versions compared to the DEVMODE structure in NT.
    //  By copying the ANSI devmode to one we allocate, then we
    //  can access all of the fields (26 currently) without causing
    //  an access violation.
    //
    pDevModeA = GlobalAlloc(GPTR, sizeof(DEVMODEA) + pDMA->dmDriverExtra);
    if (!pDevModeA)
    {
        return;
    }

    CopyMemory( (LPBYTE)pDevModeA,
                (LPBYTE)pDMA,
                min(sizeof(DEVMODEA), pDMA->dmSize) );

    CopyMemory( (LPBYTE)pDevModeA + sizeof(DEVMODEA),
                (LPBYTE)pDMA + pDMA->dmSize,
                pDMA->dmDriverExtra );

    //
    //  Now we can thunk the contents of the ANSI structure to the
    //  Unicode structure.
    //    
    SHAnsiToUnicode((LPSTR)pDevModeA->dmDeviceName,(LPWSTR)pDMW->dmDeviceName,CCHDEVICENAME );

    pDMW->dmSpecVersion   = pDevModeA->dmSpecVersion;
    pDMW->dmDriverVersion = pDevModeA->dmDriverVersion;
    pDMW->dmSize          = sizeof(DEVMODEW);
    pDMW->dmDriverExtra   = pDevModeA->dmDriverExtra;
    pDMW->dmFields        = pDevModeA->dmFields;
    pDMW->dmOrientation   = pDevModeA->dmOrientation;
    pDMW->dmPaperSize     = pDevModeA->dmPaperSize;
    pDMW->dmPaperLength   = pDevModeA->dmPaperLength;
    pDMW->dmPaperWidth    = pDevModeA->dmPaperWidth;
    pDMW->dmScale         = pDevModeA->dmScale;
    pDMW->dmCopies        = pDevModeA->dmCopies;
    pDMW->dmDefaultSource = pDevModeA->dmDefaultSource;
    pDMW->dmPrintQuality  = pDevModeA->dmPrintQuality;
    pDMW->dmColor         = pDevModeA->dmColor;
    pDMW->dmDuplex        = pDevModeA->dmDuplex;
    pDMW->dmYResolution   = pDevModeA->dmYResolution;
    pDMW->dmTTOption      = pDevModeA->dmTTOption;
    pDMW->dmCollate       = pDevModeA->dmCollate;

    SHAnsiToUnicode((LPSTR)pDevModeA->dmFormName,(LPWSTR)pDMW->dmFormName,CCHFORMNAME );

    pDMW->dmLogPixels        = pDevModeA->dmLogPixels;
    pDMW->dmBitsPerPel       = pDevModeA->dmBitsPerPel;
    pDMW->dmPelsWidth        = pDevModeA->dmPelsWidth;
    pDMW->dmPelsHeight       = pDevModeA->dmPelsHeight;
    pDMW->dmDisplayFlags     = pDevModeA->dmDisplayFlags;
    pDMW->dmDisplayFrequency = pDevModeA->dmDisplayFrequency;

    pDMW->dmICMMethod        = pDevModeA->dmICMMethod;
    pDMW->dmICMIntent        = pDevModeA->dmICMIntent;
    pDMW->dmMediaType        = pDevModeA->dmMediaType;
    pDMW->dmDitherType       = pDevModeA->dmDitherType;

    pDMW->dmReserved1        = pDevModeA->dmReserved1;
    pDMW->dmReserved2        = pDevModeA->dmReserved2;

    pDMW->dmPanningWidth     = pDevModeA->dmPanningWidth;
    pDMW->dmPanningHeight    = pDevModeA->dmPanningHeight;

    CopyMemory( (pDMW + 1),
                (pDevModeA + 1),
                pDevModeA->dmDriverExtra );

    //
    //  Free the memory we allocated.
    //
    GlobalFree(pDevModeA);
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkDevModeW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkDevModeW2A(
    LPDEVMODEW pDMW,
    LPDEVMODEA pDMA)
{
    if (!pDMW || !pDMA)
    {
        return;
    }


    SHUnicodeToAnsi((LPWSTR)pDMW->dmDeviceName,(LPSTR)pDMA->dmDeviceName,CCHDEVICENAME);

    pDMA->dmSpecVersion   = pDMW->dmSpecVersion;
    pDMA->dmDriverVersion = pDMW->dmDriverVersion;
    pDMA->dmSize          = sizeof(DEVMODEA);
    pDMA->dmDriverExtra   = pDMW->dmDriverExtra;
    pDMA->dmFields        = pDMW->dmFields;
    pDMA->dmOrientation   = pDMW->dmOrientation;
    pDMA->dmPaperSize     = pDMW->dmPaperSize;
    pDMA->dmPaperLength   = pDMW->dmPaperLength;
    pDMA->dmPaperWidth    = pDMW->dmPaperWidth;
    pDMA->dmScale         = pDMW->dmScale;
    pDMA->dmCopies        = pDMW->dmCopies;
    pDMA->dmDefaultSource = pDMW->dmDefaultSource;
    pDMA->dmPrintQuality  = pDMW->dmPrintQuality;
    pDMA->dmColor         = pDMW->dmColor;
    pDMA->dmDuplex        = pDMW->dmDuplex;
    pDMA->dmYResolution   = pDMW->dmYResolution;
    pDMA->dmTTOption      = pDMW->dmTTOption;
    pDMA->dmCollate       = pDMW->dmCollate;

    SHUnicodeToAnsi((LPWSTR)pDMW->dmFormName,(LPSTR)pDMA->dmFormName,CCHFORMNAME);

    pDMA->dmLogPixels        = pDMW->dmLogPixels;
    pDMA->dmBitsPerPel       = pDMW->dmBitsPerPel;
    pDMA->dmPelsWidth        = pDMW->dmPelsWidth;
    pDMA->dmPelsHeight       = pDMW->dmPelsHeight;
    pDMA->dmDisplayFlags     = pDMW->dmDisplayFlags;
    pDMA->dmDisplayFrequency = pDMW->dmDisplayFrequency;

    pDMA->dmICMMethod        = pDMW->dmICMMethod;
    pDMA->dmICMIntent        = pDMW->dmICMIntent;
    pDMA->dmMediaType        = pDMW->dmMediaType;
    pDMA->dmDitherType       = pDMW->dmDitherType;

    pDMA->dmReserved1        = pDMW->dmReserved1;
    pDMA->dmReserved2        = pDMW->dmReserved2;

    pDMA->dmPanningWidth     = pDMW->dmPanningWidth;
    pDMA->dmPanningHeight    = pDMW->dmPanningHeight;

    CopyMemory( (pDMA + 1),
                (pDMW + 1),
                pDMA->dmDriverExtra );
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateUnicodeDevMode
//
//  Purpose:    Allocates a Unicode devmode structure, and calls
//              the thunk function to fill it in.
//
//  Parameters: LPDEVMODEA pANSIDevMode
//
//  Return:     LPDEVMODEW - pointer to new devmode if successful
//                           NULL if not.
//
////////////////////////////////////////////////////////////////////////////

LPDEVMODEW AllocateUnicodeDevMode(
    LPDEVMODEA pANSIDevMode)
{
    int iSize;
    LPDEVMODEW pUnicodeDevMode;

    //
    //  Check for NULL pointer.
    //
    if (!pANSIDevMode)
    {
        return (NULL);
    }

    //
    //  Determine output structure size.  This has two components:  the
    //  DEVMODEW structure size,  plus any private data area.  The latter
    //  is only meaningful when a structure is passed in.
    //
    iSize = sizeof(DEVMODEW);

    iSize += pANSIDevMode->dmDriverExtra;

    pUnicodeDevMode = GlobalAlloc(GPTR, iSize);

    if (!pUnicodeDevMode)
    {
        return (NULL);
    }

    //
    //  Now call the thunk routine to copy the ANSI devmode to the
    //  Unicode devmode.
    //
    ThunkDevModeA2W(pANSIDevMode, pUnicodeDevMode);

    //
    //  Return the pointer.
    //
    return (pUnicodeDevMode);
}


////////////////////////////////////////////////////////////////////////////
//
//  AllocateAnsiDevMode
//
//  Purpose:    Allocates a Ansi devmode structure, and calls
//              the thunk function to fill it in.
//
//  Parameters: LPDEVMODEW pUnicodeDevMode
//
//  Return:     LPDEVMODEA - pointer to new devmode if successful
//                           NULL if not.
//
////////////////////////////////////////////////////////////////////////////

LPDEVMODEA AllocateAnsiDevMode(
    LPDEVMODEW pUnicodeDevMode)
{
    int iSize;
    LPDEVMODEA pANSIDevMode;

    //
    //  Check for NULL pointer.
    //
    if (!pUnicodeDevMode)
    {
        return (NULL);
    }

    //
    //  Determine output structure size.  This has two components:  the
    //  DEVMODEW structure size,  plus any private data area.  The latter
    //  is only meaningful when a structure is passed in.
    //
    iSize = sizeof(DEVMODEA);

    iSize += pUnicodeDevMode->dmDriverExtra;

    pANSIDevMode = GlobalAlloc(GPTR, iSize);

    if (!pANSIDevMode)
    {
        return (NULL);
    }

    //
    //  Now call the thunk routine to copy the Unicode devmode to the
    //  ANSI devmode.
    //
    ThunkDevModeW2A(pUnicodeDevMode, pANSIDevMode);

    //
    //  Return the pointer.
    //
    return (pANSIDevMode);
}


#ifdef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  Ssync_ANSI_UNICODE_PD_For_WOW
//
//  Function to allow NT WOW to keep the ANSI & UNICODE versions of
//  the CHOOSEFONT structure in ssync as required by many 16-bit apps.
//  See notes for Ssync_ANSI_UNICODE_Struct_For_WOW() in dlgs.c.
//
////////////////////////////////////////////////////////////////////////////

VOID Ssync_ANSI_UNICODE_PD_For_WOW(
    HWND hDlg,
    BOOL f_ANSI_to_UNICODE)
{
    PPRINTINFO pPI;

    if (pPI = (PPRINTINFO)GetProp(hDlg, PRNPROP))
    {
        if (pPI->pPD && pPI->pPDA)
        {
            if (f_ANSI_to_UNICODE)
            {
                ThunkPrintDlgA2W(pPI);
            }
            else
            {
                ThunkPrintDlgW2A(pPI);
            }
        }
    }
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////
//
//  SetCopiesEditWidth
//
// Adjust the width of the copies edit control using the current
// font and the scroll bar width.  This is necessary to handle the 
// the up down control from encroching on the space in the edit
// control when we are in High Contrast (extra large) mode.
//
////////////////////////////////////////////////////////////////////////////

VOID SetCopiesEditWidth(
    HWND hDlg,
    HWND hControl)
{
    HDC hDC                 = NULL;
    LONG MaxDigitExtant     = 0;
    LONG EditControlWidth   = 0;
    LONG CurrentWidth       = 0;
    UINT i                  = 0;
    INT aDigitWidth[10];
    WINDOWPLACEMENT WndPl;

    //
    // Acquire the edit controls device context.
    //
    hDC = GetDC( hControl );

    if (hDC)
    {
        //
        // Determine max width of the digit group.
        //
        if (GetCharWidth32( hDC, TEXT('0'), TEXT('9'), aDigitWidth))
        {
            for (i = 0; i < ARRAYSIZE(aDigitWidth); i++)
            {
                if (aDigitWidth[i] > MaxDigitExtant)
                {
                    MaxDigitExtant = aDigitWidth[i];
                }
            }

            //
            // Get the edit control placement.
            //
            WndPl.length = sizeof( WndPl );

            if (GetWindowPlacement( hControl, &WndPl ))
            {
                //
                // Calculate the edit control current width.
                //
                EditControlWidth = MaxDigitExtant * COPIES_EDIT_SIZE;

                //
                // Calculate the current width of the edit control.
                //
                CurrentWidth = WndPl.rcNormalPosition.right - WndPl.rcNormalPosition.left;
                
                //
                // Set the new position of the edit control.
                //                
                WndPl.rcNormalPosition.left = WndPl.rcNormalPosition.left - (EditControlWidth - CurrentWidth);

                //
                // Place the control.
                //
                SetWindowPlacement( hControl, &WndPl );
            }
        }

        //
        // Release the device context.
        //
        ReleaseDC( hControl, hDC );
    }
}
