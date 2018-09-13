#include "precomp.h"
#pragma hdrstop


static PMONPARAM pMonParams;


BOOL AdvMonDoNotify(HWND hDlg, int idControl, NMHDR *lpnmh, UINT iNoteCode ) {
    UINT iFreq;

    switch (iNoteCode) {


    case PSN_APPLY:
        iFreq = SendDlgItemMessage(hDlg, ID_DSP_FREQ, CB_GETCURSEL, 0, 0);
        if (iFreq != CB_ERR ) {
            pMonParams->iCurRate = iFreq;
        }

        SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);

        break;


    default:
        return FALSE;
    }

    return TRUE;
}



/***************************************************************************\
*  AdvMonitorPageProc
*
*   Dialog Proc callable from PropertyPage code.
*
* History:
* 14-Oct-1996 JonPa Created it
\***************************************************************************/
BOOL CALLBACK AdvMonitorPageProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    UINT i;

    switch (msg) {
    case WM_INITDIALOG:
        pMonParams = (PMONPARAM)(((LPPROPSHEETPAGE)lParam)->lParam);

        SendDlgItemMessage(hwnd, ID_DSP_FREQ, CB_RESETCONTENT, 0, 0);

        // Populate the combo box
        for( i = 0; i < pMonParams->cRates; i++ ) {
            LPTSTR lpszFreq;

            //
            // convert bit count to number of colors and make it a string.
            //

            if ((pMonParams->aRates[i] == 0) ||
                (pMonParams->aRates[i] == 1) ) {

                lpszFreq = FmtSprint(ID_DSP_TXT_DEFFREQ);

            } else if (pMonParams->aRates[i] < 50) {

                lpszFreq = FmtSprint(ID_DSP_TXT_INTERLACED, pMonParams->aRates[i]);


            } else {

                lpszFreq = FmtSprint(ID_DSP_TXT_FREQ, pMonParams->aRates[i]);
            }

            SendDlgItemMessage(hwnd, ID_DSP_FREQ, CB_INSERTSTRING, i, (LPARAM)lpszFreq);

            LocalFree(lpszFreq);
        }

        SendDlgItemMessage(hwnd, ID_DSP_FREQ, CB_SETCURSEL, pMonParams->iCurRate, 0);
        break;

#if 0
    case WM_COMMMAND:
        return AdvMonDoCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam) );
#endif

    case WM_NOTIFY:
        return AdvMonDoNotify(hwnd, (int)wParam, (NMHDR *)lParam, ((NMHDR *)lParam)->code );

    default:
        return FALSE;
    }

    return TRUE;
}


BOOL GetAdvMonitorPropPageParam(LPVOID lpv, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lparam, LPARAM lparamPage) {
    // Create a property page and call lpfnAdd to add it in.

    HPROPSHEETPAGE page = NULL;
    PROPSHEETPAGE psp;
    BOOL fRet = FALSE;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;

    psp.pfnDlgProc = AdvMonitorPageProc;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_ADVDSP_MON);
    psp.lParam = lparamPage;

    if ((page = CreatePropertySheetPage(&psp)) != NULL) {

        fRet = TRUE;

        if (!lpfnAdd(page, lparam)) {
            DestroyPropertySheetPage(page);
            page = NULL;
            fRet = FALSE;
        }
    }

    return fRet;

}
