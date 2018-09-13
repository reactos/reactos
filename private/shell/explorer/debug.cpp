#include "cabinet.h"
#include "intshcut.h"

#include "rcids.h"

#ifdef FULL_DEBUG

typedef struct tagUDINFO
{
    DWORD cbSize;
} UDINFO;


void UD_SplatMe(HWND hdlg, BOOL bSet)
{
    HRESULT hres;
    IUniformResourceLocator * purl;
    HWND hwndEdit = GetDlgItem(hdlg, IDC_URL);

    hres = SHCoCreateInstance(NULL, &CLSID_InternetShortcut, NULL, 
                              IID_IUniformResourceLocator, (LPVOID *)&purl);
    if (SUCCEEDED(hres))
    {
        ShStr shstr;
        int cch = Edit_GetTextLength(hwndEdit) + 1;

        shstr.SetSize(cch);
        Edit_GetText(hwndEdit, shstr, cch);

        hres = purl->SetURL(shstr, IURL_SETURL_FL_GUESS_PROTOCOL | IURL_SETURL_FL_USE_DEFAULT_PROTOCOL);
        if (SUCCEEDED(hres))
        {
            // update the edit box to reflect any canonicalization done
            LPTSTR psz;

            if (SUCCEEDED(purl->GetURL(&psz)))
            {
                Edit_SetText(hwndEdit, psz);
                Edit_SetSel(hwndEdit, 0, -1);
                CoTaskMemFree(psz);
            }

            IPropertySetStorage * ppropsetstg;

            hres = purl->QueryInterface(IID_IPropertySetStorage, (LPVOID *)&ppropsetstg);
            if (SUCCEEDED(hres))
            {
                IPropertyStorage * ppropstg;

                hres = ppropsetstg->Open(FMTID_InternetSite, STGM_WRITE, &ppropstg);
                if (SUCCEEDED(hres))
                {
                    const PROPSPEC propspec = 
                        { PRSPEC_PROPID, PID_INTSITE_FLAGS };
                    PROPVARIANT propvar = { 0 };

                    // Get the previous values
                    hres = ppropstg->ReadMultiple(1, &propspec, &propvar);

                    if (SUCCEEDED(hres))
                    {
                        PROPVARIANT * ppropvar;

                        // Set or clear the "recently changed" flag 
                        ppropvar = &propvar;
                        if (VT_UI4 == ppropvar->vt)
                        {
                            if (bSet)
                                SetFlag(ppropvar->ulVal, PIDISF_RECENTLYCHANGED);
                            else
                                ClearFlag(ppropvar->ulVal, PIDISF_RECENTLYCHANGED);
                        }
                        else
                        {
                            ppropvar->vt = VT_UI4;
                            ppropvar->ulVal = 0;
                        }

                        // Write the changes out and commit
                        ppropstg->WriteMultiple(1, &propspec, &propvar, 0);
                        ppropstg->Commit(STGC_DEFAULT);

                        FreePropVariantArray(1, &propvar);
                    }
                    ppropstg->Release();
                }
                ppropsetstg->Release();
            }
        }
        purl->Release();
    }
}    


void UD_OnCommand(HWND hdlg, int id, HWND hwndCtl, UINT nCodeNotify)
{
    switch (id)
    {
    case IDC_SPLATME:
    case IDC_CLEARSPLAT:
        UD_SplatMe(hdlg, IDC_SPLATME == id);
        
        SetFocus(GetDlgItem(hdlg, IDC_URL));
        break;

    case IDOK:
    case IDCANCEL:
        EndDialog(hdlg, id);
        break;
    }
}    


BOOL_PTR CALLBACK UDDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet;

    switch (uMsg)
    {
    HANDLE_MSG(hdlg, WM_COMMAND, UD_OnCommand);

    default:
        bRet = FALSE;
        break;
    }
    return bRet;
}   


extern "C" void InvokeURLDebugDlg(HWND hwnd)
{
    UDINFO udi;

    DialogBoxParam(hinstCabinet, MAKEINTRESOURCE(IDD_DEBUGURLDB), hwnd, UDDlgProc, (LPARAM)&udi);
}   

 
#endif // FULL_DEBUG



// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "explorer"
#define SZ_MODULE           "EXPLORER"
#define DECLARE_DEBUG
#include <debug.h>


