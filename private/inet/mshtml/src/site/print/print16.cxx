/* dlg_prnt.c -- PRINT setup dialog */

#include "headers.hxx"

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_PAGE_H_
#define X_PAGE_H_
#include <page.h>
#endif

#ifndef X_PRINT_H_
#define X_PRINT_H_
#include <print.h>
#endif

#ifndef X_PGSTUP16_HXX_
#define X_PGSTUP16_HXX_
#include <pgstup16.hxx>
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

extern void Init_DevMode();
typedef UINT (CALLBACK *LPPRINTHOOKPROC) (HWND, UINT, WPARAM, LPARAM);
typedef UINT (CALLBACK *LPSETUPHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

BOOL DlgPrnt_RunDialog(LPPRINTDLG lppd, BOOL bSetup)
{
    BOOL bResult;

    if (!lppd)
    {
        return FALSE;
    }

    if (bSetup)
    {
        lppd->Flags |= (PD_PRINTSETUP | PD_ENABLESETUPTEMPLATE);
        lppd->lpSetupTemplateName = MAKEINTRESOURCE(RES_DLG_PAGE_PRNSETUP);
    }

    if (lppd->hDevNames)
    {
        register DEVNAMES *dn = (DEVNAMES *) GlobalLock(lppd->hDevNames);
        dn->wDefault &= ~DN_DEFAULTPRN;
        (void) GlobalUnlock(lppd->hDevNames);
    }

    bResult = PrintDlg(lppd);

    if (!bResult)
    {
        DWORD dwError = CommDlgExtendedError();

        if (dwError == 0)   /* Win32s has problem if dialog is */
        {                                        /* cancelled the first time put up. */
            return FALSE;                        /* something, so we pretend we were */
        }                                        /* never here. */

        return FALSE;
    }

    // We are using Modal Dialog box now...
    return bResult;
}
