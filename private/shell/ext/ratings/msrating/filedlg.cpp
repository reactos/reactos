/****************************************************************************\
 *
 *   FILEDLG.CPP - Code to manage the Rating Systems dialog.
 *
 *     gregj    06/27/96    Moved code here from msludlg.cpp and largely rewrote.
 *     
\****************************************************************************/

/*INCLUDES--------------------------------------------------------------------*/
#include "msrating.h"
#include "ratings.h"
#include "mslubase.h"
#include "commctrl.h"
#include "commdlg.h"
#include "buffer.h"
#include <shellapi.h>

#include <contxids.h>

#include <mluisupp.h>

typedef BOOL (APIENTRY *PFNGETOPENFILENAME)(LPOPENFILENAME);

BOOL OpenTemplateDlg(HWND hWnd,CHAR * szFilename,UINT cbFilename)
{
    OPENFILENAME ofn;
    CHAR szFilter[MAXPATHLEN];
    CHAR szOpenInfTitle[MAXPATHLEN];
    CHAR szInitialDir[MAXPATHLEN];

    GetSystemDirectory(szInitialDir, sizeof(szInitialDir));
    strcpyf(szFilename,szNULL);
    MLLoadStringA(IDS_RAT_OPENFILE, szOpenInfTitle,sizeof(szOpenInfTitle));

    // have to load the openfile filter in 2 stages, because the string
    // contains a terminating character and MLLoadString won't load the
    // whole thing in one go
    memset(szFilter,0,sizeof(szFilter));
    MLLoadStringA(IDS_RAT_FILTER_DESC,szFilter,sizeof(szFilter));
    MLLoadStringA(IDS_RAT_FILTER,szFilter+strlenf(szFilter)+1,sizeof(szFilter)-
        (strlenf(szFilter)-1));
    
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile =    szFilename;
    ofn.nMaxFile = cbFilename;
    ofn.lpstrTitle = szOpenInfTitle;
    ofn.lpstrInitialDir = szInitialDir;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST
        | OFN_SHAREAWARE | OFN_HIDEREADONLY;

    BOOL fRet = FALSE;
    HINSTANCE hCommdlg = ::LoadLibrary(::szComDlg32);
    if (hCommdlg != NULL) {
        PFNGETOPENFILENAME pfnGetOpenFileName =    (PFNGETOPENFILENAME)::GetProcAddress(hCommdlg, ::szGetOpenFileName);
        if (pfnGetOpenFileName != NULL) {
            fRet = (*pfnGetOpenFileName)(&ofn);
        }

        FreeLibrary(hCommdlg);
    }

    return fRet;
}


void SetHorizontalExtent(HWND hwndLB, LPCSTR pszString)
{
    HDC hDC = ::GetDC(hwndLB);
    HFONT hFont = (HFONT)::SendMessage(hwndLB, WM_GETFONT, 0, 0);
    HFONT hfontOld = (HFONT)::SelectObject(hDC, hFont);

    UINT cxSlop = ::GetSystemMetrics(SM_CXBORDER) * 4;    /* 2 for LB border, 2 for margin inside border */

    UINT cxNewMaxExtent = 0;
    SIZE s;
    if (pszString != NULL) {
        ::GetTextExtentPoint(hDC, pszString, ::strlenf(pszString), &s);
        UINT cxCurExtent = (UINT)::SendMessage(hwndLB, LB_GETHORIZONTALEXTENT, 0, 0);
        if ((UINT)s.cx > cxCurExtent)
            cxNewMaxExtent = s.cx + cxSlop;
    }
    else {
        UINT cItems = (UINT)::SendMessage(hwndLB, LB_GETCOUNT, 0, 0);
        for (UINT i=0; i<cItems; i++) {
            char szItem[MAXPATHLEN];    /* we know we have pathnames in the list */
            ::SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)(LPSTR)szItem);
            ::GetTextExtentPoint(hDC, szItem, ::strlenf(szItem), &s);
            if ((UINT)s.cx > cxNewMaxExtent)
                cxNewMaxExtent = s.cx;
        }
        cxNewMaxExtent += cxSlop;
    }

    if (cxNewMaxExtent > 0)
        ::SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, (WPARAM)cxNewMaxExtent, 0);

    ::SelectObject(hDC, hfontOld);
    ::ReleaseDC(hwndLB, hDC);
}


struct ProviderData
{
    PicsRatingSystem *pPRS;
    PicsRatingSystem *pprsNew;
    UINT nAction;
};

const UINT PROVIDER_KEEP = 0;
const UINT PROVIDER_ADD = 1;
const UINT PROVIDER_DEL = 2;


class ProviderDlgData
{
public:
    ProviderDlgData() { }
    ~ProviderDlgData() { }    /* so array gets destructed */

    PicsRatingSystemInfo *pPRSI;
    array<ProviderData> aPD;
};


void AddProviderToList(HWND hDlg, UINT idx, LPCSTR pszFilename)
{
    UINT_PTR iItem = SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_ADDSTRING, 0,
                                        (LPARAM)pszFilename);
    if (iItem != LB_ERR)
        SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_SETITEMDATA, iItem, (LPARAM)idx);
}


BOOL InitProviderDlg(HWND hDlg, PicsRatingSystemInfo *pPRSI)
{
    ProviderDlgData *pDlgData = new ProviderDlgData;
    if (pDlgData == NULL)
        return FALSE;

    pDlgData->pPRSI = pPRSI;

    SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR) pDlgData);

    for (UINT i = 0; i < (UINT)pDlgData->pPRSI->arrpPRS.Length(); ++i)
    {
        PicsRatingSystem *pPRS = pDlgData->pPRSI->arrpPRS[i];
        ProviderData pd;
        pd.pPRS = pPRS;
        pd.pprsNew = NULL;
        pd.nAction = PROVIDER_KEEP;
        if (!pDlgData->aPD.Append(pd))
            return FALSE;

        if(pPRS->etstrName.Get())
        {
            // add provider using name
            AddProviderToList(hDlg, i, pPRS->etstrName.Get());
        }
        else if(pPRS->etstrFile.Get())
        {
            // no name - possibly missing file, use filespec instead
            AddProviderToList(hDlg, i, pPRS->etstrFile.Get());
        }
    }

    SetHorizontalExtent(::GetDlgItem(hDlg, IDC_PROVIDERLIST), NULL);
            
    EnableWindow(GetDlgItem(hDlg, IDC_CLOSEPROVIDER), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_OPENPROVIDER), TRUE);

    if (SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_SETCURSEL, 0, 0) != LB_ERR) {
        EnableWindow(GetDlgItem(hDlg, IDC_CLOSEPROVIDER), TRUE);
    }

    return TRUE;
}


void EndProviderDlg(HWND hDlg, BOOL fRet)
{
    ProviderDlgData *pDlgData = (ProviderDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);

    /* Go through our auxiliary array and delete any provider structures which
     * we added in this dialog.  Note that if the user previously hit OK, the
     * providers which were added will be marked as KEEP when they're put back
     * in the main data structure, so we won't delete them here.
     */
    UINT cProviders = pDlgData->aPD.Length();
    for (UINT i=0; i<cProviders; i++) {
        if (pDlgData->aPD[i].nAction == PROVIDER_ADD) {
            delete pDlgData->aPD[i].pPRS;
        }
        if (pDlgData->aPD[i].pprsNew != NULL) {
            delete pDlgData->aPD[i].pprsNew;
        }
    }

    delete pDlgData;

    ::EndDialog(hDlg, fRet);
}


void CommitProviderDlg(HWND hDlg)
{
    ProviderDlgData *pDlgData = (ProviderDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);

    PicsRatingSystemInfo *pPRSI = pDlgData->pPRSI;

    /* We check twice to see if there are any rating systems installed.
     * Up front, we see if there's anything in the list, before we commit
     * any changes;  this lets the user change their mind, cancel the dialog,
     * and not lose any settings.
     *
     * The second check is down at the end, seeing if there are any valid
     * rating systems left after we're done committing changes.  Note that
     * the results of that check could be different than this first one if
     * any rating systems fail to load for some reason.
     *
     * If we prompt the user the first time and he says he really doesn't
     * want any rating systems (i.e., wants to disable ratings completely),
     * we don't bother prompting the second time since he's already said no.
     * Hence the fPrompted flag.
     */
    BOOL fPrompted = FALSE;

    if (SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_GETCOUNT, 0, 0) == 0) {
        MyMessageBox(hDlg, IDS_NO_PROVIDERS, IDS_GENERIC, MB_OK);
        return;
    }

    /* Go through the list and add the new ones.
     * Note that this does NOT destruct the pPRS objects themselves, it just
     * empties the array.  We have saved copies of all of them in our auxiliary
     * array.
     */

    pPRSI->arrpPRS.ClearAll();

    UINT cItems = pDlgData->aPD.Length();

    for (UINT i=0; i<cItems; i++) {
        switch (pDlgData->aPD[i].nAction) {
        case PROVIDER_DEL:
            DeleteUserSettings(pDlgData->aPD[i].pPRS);
            delete pDlgData->aPD[i].pPRS;
            pDlgData->aPD[i].pPRS = NULL;
            delete pDlgData->aPD[i].pprsNew;
            pDlgData->aPD[i].pprsNew = NULL;
            break;

        case PROVIDER_KEEP:
            if (pDlgData->aPD[i].pprsNew != NULL) {
                CheckUserSettings(pDlgData->aPD[i].pprsNew);
                pPRSI->arrpPRS.Append(pDlgData->aPD[i].pprsNew);
                delete pDlgData->aPD[i].pPRS;
                pDlgData->aPD[i].pPRS = NULL;
                pDlgData->aPD[i].pprsNew = NULL;    /* protect from cleanup code */
            }
            else if (!(pDlgData->aPD[i].pPRS->dwFlags & PRS_ISVALID)) {
                delete pDlgData->aPD[i].pPRS;
                pDlgData->aPD[i].pPRS = NULL;
            }
            else {
                CheckUserSettings(pDlgData->aPD[i].pPRS);
                pPRSI->arrpPRS.Append(pDlgData->aPD[i].pPRS);
            }
            break;

        case PROVIDER_ADD:
            if (pDlgData->aPD[i].pPRS != NULL) {
                CheckUserSettings(pDlgData->aPD[i].pPRS);
                pPRSI->arrpPRS.Append(pDlgData->aPD[i].pPRS);
                pDlgData->aPD[i].nAction = PROVIDER_KEEP;        /* keep this one now */
            }
            break;

        default:
            ASSERT(FALSE);
        }
    }

    if (pPRSI->arrpPRS.Length() == 0) {
        if (!fPrompted &&
            MyMessageBox(hDlg, IDS_NO_PROVIDERS, IDS_GENERIC, MB_YESNO) == IDYES)
        {
            return;
        }
        pPRSI->fRatingInstalled = FALSE;
    }
    else {
        pPRSI->fRatingInstalled = TRUE;
    }

    EndProviderDlg(hDlg, TRUE);
}


void RemoveProvider(HWND hDlg)
{
    ProviderDlgData *pDlgData = (ProviderDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);

    UINT_PTR i = SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_GETCURSEL,0,0);

    if (i != LB_ERR)
    {
        UINT idx = (UINT)SendDlgItemMessage(hDlg, IDC_PROVIDERLIST,
                                            LB_GETITEMDATA, i, 0);
        if (idx < (UINT)pDlgData->aPD.Length()) {
            /* If the user added the provider in this dialog session, just
             * delete it from the array.  The null pPRS pointer will be
             * detected later, so it's OK to leave the array element itself.
             * (Yes, if the user adds and removes an item over and over, we
             * consume 12 bytes of memory each time. Oh well.)
             *
             * If the item was there before the user launched the dialog,
             * then just mark it for deletion on OK.
             */
            if (pDlgData->aPD[idx].nAction == PROVIDER_ADD) {
                delete pDlgData->aPD[idx].pPRS;
                pDlgData->aPD[idx].pPRS = NULL;
            }
            else
                pDlgData->aPD[idx].nAction = PROVIDER_DEL;
        }

        SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_DELETESTRING, i, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_CLOSEPROVIDER), FALSE);
        SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_SETCURSEL,0,0);
        SetFocus(GetDlgItem(hDlg, IDOK));
        SetHorizontalExtent(::GetDlgItem(hDlg, IDC_PROVIDERLIST), NULL);
    }
}


/* Returns zero if the two PicsRatingSystems have the same RAT-filename,
 * non-zero otherwise.  Handles the '*' marker on the end for failed
 * loads.  It is assumed that only pprsOld may have that marker.
 */
int CompareProviderNames(PicsRatingSystem *pprsOld, PicsRatingSystem *pprsNew)
{
    if (!pprsOld->etstrFile.fIsInit())
        return 1;

    UINT cbNewName = ::strlenf(pprsNew->etstrFile.Get());

    LPSTR pszOld = pprsOld->etstrFile.Get();
    int nCmp = ::strnicmpf(pprsNew->etstrFile.Get(), pszOld, cbNewName);
    if (nCmp != 0)
        return nCmp;

    pszOld += cbNewName;
    if (*pszOld == '\0' || (*pszOld == '*' && *(pszOld+1) == '\0'))
        return 0;

    return 1;
}


void AddProvider(HWND hDlg,PSTR szAddFileName=NULL)
{
    BOOL fAdd=FALSE;
    char szFileName[MAXPATHLEN+1];

    ProviderDlgData *pDlgData = (ProviderDlgData*) GetWindowLongPtr(hDlg, DWLP_USER);
    PicsRatingSystemInfo *pPRSI = pDlgData->pPRSI;

    if (szAddFileName!=NULL)
    {
        lstrcpy(szFileName,szAddFileName);
        fAdd=TRUE;
    }
    else
    {
        fAdd=OpenTemplateDlg(hDlg,szFileName, sizeof(szFileName));
    }
    
    if (fAdd==TRUE)
    {
        PicsRatingSystem *pPRS;
        HRESULT hres = LoadRatingSystem(szFileName, &pPRS);

        if (FAILED(hres)) {
            if (pPRS != NULL) {
                pPRS->ReportError(hres);
                delete pPRS;
            }
        }
        else {
            /* Check to see if this guy is already in the list.  If he is,
             * the user might have said to delete him;  in that case, put
             * him back.  Otherwise, the system is already installed, so
             * tell the user he doesn't have to install it again.
             */
            for (UINT i=0; i<(UINT)pDlgData->aPD.Length(); i++) {
                ProviderData *ppd = &pDlgData->aPD[i];
                if (ppd->pPRS==NULL) {
                    //This system was added and then removed during
                    //this dialog session.  It will be detected later,
                    //so just skip it and keep appending entries.
                    continue;
                }
                if (!CompareProviderNames(ppd->pPRS, pPRS)) {

                    if (!(ppd->pPRS->dwFlags & PRS_ISVALID) &&
                        (ppd->pprsNew == NULL))
                        ppd->pprsNew = pPRS;
                    else
                        delete pPRS;    /* don't need copy */

                    if (ppd->nAction == PROVIDER_DEL) {
                        ppd->nAction = PROVIDER_KEEP;
                        AddProviderToList(hDlg, i, ppd->pPRS->etstrName.Get());
                    }
                    else {
                        MyMessageBox(hDlg, IDS_ALREADY_INSTALLED, IDS_GENERIC, MB_OK);
                    }
                    return;
                }
            }

            /* This guy isn't already in the list.  Add him to the listbox
             * and to the array.
             */
            ProviderData pd;
            pd.nAction = PROVIDER_ADD;
            pd.pPRS = pPRS;
            pd.pprsNew = NULL;

            if (!pDlgData->aPD.Append(pd)) {
                MyMessageBox(hDlg, IDS_LOADRAT_MEMORY, IDS_GENERIC, MB_OK | MB_ICONWARNING);
                delete pPRS;
                return;
            }
            AddProviderToList(hDlg, pDlgData->aPD.Length() - 1, pPRS->etstrName.Get());

            SetFocus(GetDlgItem(hDlg, IDOK));
            SetHorizontalExtent(::GetDlgItem(hDlg, IDC_PROVIDERLIST), szFileName);
        }
    }
}


INT_PTR ProviderDlgProc(HWND hDlg, UINT uMsg, 
                        WPARAM wParam, LPARAM lParam)
{
    static DWORD aIds[] = {
        IDC_STATIC1,            IDH_RATINGS_SYSTEM_RATSYS_LIST,
        IDC_PROVIDERLIST,        IDH_RATINGS_SYSTEM_RATSYS_LIST,
        IDC_OPENPROVIDER,        IDH_RATINGS_SYSTEM_RATSYS_ADD,
        IDC_CLOSEPROVIDER,        IDH_RATINGS_SYSTEM_RATSYS_REMOVE,
        0,0
    };

    switch (uMsg) {
        case WM_INITDIALOG:
            if (!InitProviderDlg(hDlg, (PicsRatingSystemInfo *)lParam)) {
                MyMessageBox(hDlg, IDS_LOADRAT_MEMORY, IDS_GENERIC, MB_OK | MB_ICONSTOP);
                EndProviderDlg(hDlg, FALSE);
            }

            if(((PicsRatingSystemInfo *)lParam)->lpszFileName!=NULL)
            {
                AddProvider(hDlg,((PicsRatingSystemInfo *)lParam)->lpszFileName);
            }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                
                case IDC_PROVIDERLIST:
                    if (LBN_SELCHANGE == HIWORD(wParam))
                    {
                        if (SendDlgItemMessage(hDlg, IDC_PROVIDERLIST, LB_GETCURSEL, 0,0) >= 0)
                            EnableWindow(GetDlgItem(hDlg, IDC_CLOSEPROVIDER), TRUE);
                    }
                    break;


                case IDOK:
                    CommitProviderDlg(hDlg);
                    break;

                case IDCANCEL:
                    EndProviderDlg(hDlg,FALSE);
                    break;

                case IDC_CLOSEPROVIDER:
                    RemoveProvider(hDlg);                    
                    break;

                case IDC_OPENPROVIDER:
                    AddProvider(hDlg);
                    break;
            }

            break;

        case WM_HELP:
            SHWinHelpOnDemandWrap((HWND)((LPHELPINFO)lParam)->hItemHandle, ::szHelpFile,
                    HELP_WM_HELP, (DWORD_PTR)(LPSTR)aIds);
            break;

        case WM_CONTEXTMENU:
            SHWinHelpOnDemandWrap((HWND)wParam, ::szHelpFile, HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPVOID)aIds);
            break;
            
    }

    return FALSE;
}


INT_PTR DoProviderDialog(HWND hDlg, PicsRatingSystemInfo *pPRSI)
{
    return DialogBoxParam(MLGetHinst(),
                          MAKEINTRESOURCE(IDD_PROVIDERS),
                          hDlg,
                          (DLGPROC)ProviderDlgProc,
                          (LPARAM)pPRSI);
}

