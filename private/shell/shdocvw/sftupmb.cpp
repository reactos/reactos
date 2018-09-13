#include "priv.h"
#include "resource.h"

#include <mluisupp.h>

struct SUParams {
    LPSOFTDISTINFO  psdi;
    BITBOOL         bRemind : 1;
    BITBOOL         bDetails : 1;
    LONG            cyNoDetails;
    LONG            cxDlg;
    LONG            cyDlg;
};

INT_PTR CALLBACK SoftwareUpdateDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

SHDOCAPI_(DWORD) SoftwareUpdateMessageBox( HWND hWnd,
                                     LPCWSTR szDistUnit,
                                     DWORD dwFlags,
                                     LPSOFTDISTINFO psdi )
{
    HRESULT hr;
    int iRet = IDIGNORE;
    SOFTDISTINFO    sdi;
    SUParams suparams;
    DWORD dwAdStateNew = SOFTDIST_ADSTATE_NONE;

    if ( psdi == NULL )
    {
        // use a local 
        sdi.cbSize = sizeof(SOFTDISTINFO);
        sdi.dwReserved = 0;
        psdi = &sdi;
    }

    suparams.psdi = psdi;
    suparams.bRemind = TRUE;
    suparams.bDetails = FALSE;

    hr = GetSoftwareUpdateInfo( szDistUnit, psdi );
 
    // we need an HREF to work properly. The title and abstract are negotiable.
    if ( SUCCEEDED(hr) && psdi->szHREF != NULL )
    {
        // see if this is an update the user already knows about.
        // If it is, then skip the dialog.
        if (  (psdi->dwUpdateVersionMS >= psdi->dwInstalledVersionMS ||
                (psdi->dwUpdateVersionMS == psdi->dwInstalledVersionMS &&
                 psdi->dwUpdateVersionLS >= psdi->dwInstalledVersionLS))    && 
              (psdi->dwUpdateVersionMS >= psdi->dwAdvertisedVersionMS ||
                (psdi->dwUpdateVersionMS == psdi->dwAdvertisedVersionMS &&
                 psdi->dwUpdateVersionLS >= psdi->dwAdvertisedVersionLS)) )
        { 
            DWORD idDlg;

            if ( hr == S_OK ) // new version
            {
                // we have a pending update, either on the net, or downloaded
                if ( psdi->dwFlags & SOFTDIST_FLAG_USAGE_PRECACHE )
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_DOWNLOADED;
                    // Show same dialog for downloaded/available states
                    // because users get confused. See IE5 RAID entry 14488
                    idDlg = IDD_SUAVAILABLE;
                }
                else
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_AVAILABLE;
                    idDlg = IDD_SUAVAILABLE;
                }
            }
            else if ( psdi->dwUpdateVersionMS == psdi->dwInstalledVersionMS &&
                      psdi->dwUpdateVersionLS == psdi->dwInstalledVersionLS )
            {
                // if installed version matches advertised, then we autoinstalled already
                dwAdStateNew = SOFTDIST_ADSTATE_INSTALLED;
                idDlg = IDD_SUINSTALLED;
            }
            else
            {
                idDlg = 0;
            }

            // only show the dialog if we've haven't been in this ad state before for
            // this update version
            if ( dwAdStateNew > psdi->dwAdState && idDlg != 0)
            {
                // Sundown: coercion is OK since SoftwareUpdateDlgProc returns true/false
                iRet = (int) DialogBoxParam(MLGetHinst(),
                                            MAKEINTRESOURCE(idDlg),
                                            hWnd,
                                            SoftwareUpdateDlgProc,
                                            (LPARAM)&suparams);
            }
        } // if update is a newer version than advertised

        // If the user doesn't want a reminder and didn't cancel, mark the DU.

        if ( !suparams.bRemind && (iRet == IDNO || iRet == IDYES) )
        {
            SetSoftwareUpdateAdvertisementState( szDistUnit,
                                                dwAdStateNew,
                                                psdi->dwUpdateVersionMS,
                                                psdi->dwUpdateVersionLS );
        } // if we're finished with this ad state for this version
    } // if we got the update info
    else 
        iRet = IDABORT;

    if ( FAILED(hr) || psdi == &sdi )
    {
        if ( psdi->szTitle != NULL )
        {
            CoTaskMemFree( psdi->szTitle );
            psdi->szTitle = NULL;
        }
        if ( psdi->szAbstract != NULL )
        {
            CoTaskMemFree( psdi->szAbstract );
            psdi->szAbstract = NULL;
        };
        if ( psdi->szHREF != NULL )
        {
            CoTaskMemFree( psdi->szHREF );
            psdi->szHREF = NULL;
        }
    }

    return iRet;
}

INT_PTR CALLBACK SoftwareUpdateDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL        fRet = 0;
    SUParams    *psuparam = (SUParams*)GetWindowLongPtr(hDlg, DWLP_USER);;
    HRESULT     hr = S_OK;
    HWND hwndDetails;

    switch (msg)
    {
    case WM_INITDIALOG:
        int         cchDetails;
        TCHAR       *pszTitle;
        TCHAR       *pszAbstract;
        TCHAR       *pszDetails;
        TCHAR       szFmt[MAX_PATH];

        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        psuparam = (SUParams*)lParam;

        if (SHRestricted( REST_NOFORGETSOFTWAREUPDATE))
            EnableWindow(GetDlgItem(hDlg, IDC_REMIND), FALSE);

        // Prepare the details from the SOFTDISTINFO
        MLLoadString(IDS_SUDETAILSFMT, szFmt, ARRAYSIZE(szFmt) );
        cchDetails = lstrlen( szFmt );
        if ( psuparam->psdi->szTitle != NULL )
            pszTitle = psuparam->psdi->szTitle;
        if ( psuparam->psdi->szAbstract != NULL )
            pszAbstract = psuparam->psdi->szAbstract;
        pszDetails = new TCHAR[cchDetails];
        if ( pszDetails != NULL )
        {
            wnsprintf( pszDetails, cchDetails, szFmt, ((pszTitle!=NULL)?pszTitle:TEXT("")),
                                         ((pszAbstract!=NULL)?pszAbstract:TEXT("")) );
            // set the details text
            SetDlgItemText( hDlg, IDC_DETAILSTEXT, pszDetails );
            // initialize the reminder check box
            CheckDlgButton( hDlg, IDC_REMIND, ((psuparam->bRemind)?BST_CHECKED:BST_UNCHECKED) );
            // Hide or show the details
            RECT rectDlg;
            RECT rectDetails;

            GetWindowRect( hDlg, &rectDlg );
            psuparam->cyDlg = rectDlg.bottom - rectDlg.top;
            psuparam->cxDlg = rectDlg.right - rectDlg.left;
            hwndDetails = GetDlgItem( hDlg, IDC_DETAILSTEXT );
            GetWindowRect( hwndDetails, &rectDetails );
            psuparam->cyNoDetails = rectDetails.top - rectDlg.top;
            SetWindowPos( hwndDetails, NULL, 0,0,0,0, SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE );
            SetWindowPos( hDlg, NULL,
                          0,0,psuparam->cxDlg,psuparam->cyNoDetails,
                          SWP_NOMOVE | SWP_NOZORDER );
        }
        else
            EndDialog( hDlg, IDABORT );


        if ( pszDetails != NULL )
            delete pszDetails;

        fRet = TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDYES:
            EndDialog(hDlg, IDYES );
            fRet = TRUE;
            break;
        case IDNO:
            EndDialog(hDlg, IDNO );
            fRet = TRUE;
            break;
        case IDC_REMIND:
            psuparam->bRemind = IsDlgButtonChecked( hDlg, IDC_REMIND ) == BST_CHECKED;
            fRet = TRUE;
            break;
        case IDC_DETAILS:
            {
                TCHAR   szDetails[40];

                // toggle the details
                hwndDetails = GetDlgItem( hDlg, IDC_DETAILSTEXT );
                psuparam->bDetails = !psuparam->bDetails;

                if ( psuparam->bDetails )
                {
                    // show the details
                    // switch button to close text
                    MLLoadString(IDS_SUDETAILSCLOSE, szDetails, ARRAYSIZE(szDetails) );
                    SetDlgItemText( hDlg, IDC_DETAILS, szDetails );
                    SetWindowPos( hDlg, NULL,
                                  0,0,psuparam->cxDlg, psuparam->cyDlg,
                                  SWP_NOMOVE | SWP_NOZORDER );
                    SetWindowPos( hwndDetails, NULL, 0,0,0,0, SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE );
                }
                else
                {
                    MLLoadString(IDS_SUDETAILSOPEN, szDetails, ARRAYSIZE(szDetails) );
                    SetDlgItemText( hDlg, IDC_DETAILS, szDetails );
                    SetWindowPos( hwndDetails, NULL, 0,0,0,0, SWP_NOMOVE | SWP_HIDEWINDOW | SWP_NOZORDER | SWP_NOSIZE );
                    SetWindowPos( hDlg, NULL,
                                  0,0,psuparam->cxDlg,psuparam->cyNoDetails,
                                  SWP_NOMOVE | SWP_NOZORDER );
                }
            }
            fRet = TRUE;
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, IDNO);
        fRet = TRUE;
        break;

    case WM_DESTROY:
        fRet = TRUE;
        break;

    default:
        fRet = FALSE;
    }

    return fRet;
}
