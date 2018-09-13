#include "shellprv.h"
#pragma  hdrstop

#include "datautil.h"

// shlexec.c
STDAPI_(BOOL) DoesAppWantUrl(LPCTSTR pszFullPathToApp);

//
// IDropTarget::Drop
//
STDMETHODIMP CExeIDLDropTarget_Drop(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    STGMEDIUM medium;
    DWORD dwEffectPerformed = 0;
    TCHAR szExePath[MAX_PATH];

    if (!(this->grfKeyStateLast & MK_LBUTTON))
    {
        HMENU hmenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_DROPONEXE);
        if (hmenu)
        {
            UINT idCmd = SHTrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, this->hwndOwner, NULL);
            DestroyMenu(hmenu);
            if (idCmd != DDIDM_OPENWITH)
            {
                *pdwEffect = 0; // canceled
            }
        }
    }

    CIDLDropTarget_GetPath(this, szExePath);

    // required call in DropTarget::Drop
    DAD_DragLeave();

    if (*pdwEffect)
    {
        SHELLEXECUTEINFO ei = {
            SIZEOF(SHELLEXECUTEINFO),
            0, NULL, NULL, szExePath, NULL, NULL, SW_SHOWNORMAL, NULL 
        };

        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        HRESULT hres = pDataObj->lpVtbl->GetData(pDataObj, &fmte, &medium);
        if (SUCCEEDED(hres))
        {
            TCHAR szPath[MAX_PATH];
            UINT i;
            int cchParam = 0;

            for (i = 0; DragQueryFile(medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
            {
                PathQuoteSpaces(szPath);
                cchParam += lstrlen(szPath) + 2;    // space and NULL
            }

            if (cchParam)
            {
                LPTSTR pszParam = LocalAlloc(LPTR, cchParam * SIZEOF(TCHAR));
                if (pszParam)
                {
                    for (i = 0; DragQueryFile(medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
                    {
                        PathQuoteSpaces(szPath);
                        if (*pszParam)
                            lstrcat(pszParam, c_szSpace);
                        lstrcat(pszParam, szPath);
                    }

                    ei.lpParameters = pszParam;

                    ShellExecuteEx(&ei);

                    LocalFree((HLOCAL)pszParam);

                    dwEffectPerformed = DROPEFFECT_COPY;  // what we did
                }
            }
            ReleaseStgMedium(&medium);
        }
        else
        {
            LPSTR pszURL;

            if (SUCCEEDED(DataObj_GetShellURL(pDataObj, &medium, &pszURL)))
            {
                if (DoesAppWantUrl(szExePath))
                {
                    TCHAR szURL[INTERNET_MAX_URL_LENGTH];
                    SHAnsiToTChar(pszURL, szURL, ARRAYSIZE(szURL));

                    ei.lpParameters = szURL;

                    ShellExecuteEx(&ei);

                    dwEffectPerformed = DROPEFFECT_LINK;  // what we did
                }
                ReleaseStgMediumHGLOBAL(pszURL, &medium);
            }
        }
        *pdwEffect = dwEffectPerformed; 
    }

    CIDLDropTarget_DragLeave(pdropt);
    return S_OK;
}


//
// IDropTarget::DragEnter
//
STDMETHODIMP CExeIDLDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);

    // let the base-class process it, first.
    CIDLDropTarget_DragEnter(pdropt, pDataObj, grfKeyState, pt, pdwEffect);

    if ((this->dwData & DTID_HDROP) || (DataObj_GetShellURL(pDataObj, NULL, NULL) == S_OK))
        *pdwEffect &= (DROPEFFECT_COPY | DROPEFFECT_LINK);
    else
        *pdwEffect = 0;

    this->dwEffectLastReturned = *pdwEffect;

    return S_OK;        // Notes: we should NOT return hres as it.
}


const IDropTargetVtbl c_CExeDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CExeIDLDropTarget_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CExeIDLDropTarget_Drop,
};


STDAPI CExeDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppvOut)
{
    return CIDLDropTarget_Create(hwnd, &c_CExeDropTargetVtbl, pidl, ppvOut);
}

