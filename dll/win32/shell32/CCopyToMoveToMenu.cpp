/*
 * PROJECT:     shell32
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     MoveTo implementation
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

HRESULT
CCopyToMoveToMenu::DoGetFileTitle(CStringW& strTitle, IDataObject *pDataObject)
{
    CDataObjectHIDA pCIDA(pDataObject);
    if (FAILED_UNEXPECTEDLY(pCIDA.hr()))
        return E_FAIL;

    PCUIDLIST_ABSOLUTE pidlParent = HIDA_GetPIDLFolder(pCIDA);
    if (!pidlParent)
    {
        ERR("HIDA_GetPIDLFolder failed\n");
        return E_FAIL;
    }

    WCHAR szPath[MAX_PATH];
    PCUIDLIST_RELATIVE pidlRelative = HIDA_GetPIDLItem(pCIDA, 0);
    if (!pidlRelative)
    {
        ERR("HIDA_GetPIDLItem failed\n");
        return E_FAIL;
    }

    CComHeapPtr<ITEMIDLIST> pidlCombine(ILCombine(pidlParent, pidlRelative));

    if (!SHGetPathFromIDListW(pidlCombine, szPath))
    {
        ERR("Cannot get path\n");
        return E_FAIL;
    }

    strTitle = PathFindFileNameW(szPath);
    if (strTitle.IsEmpty())
        return E_FAIL;

    if (pCIDA->cidl > 1)
        strTitle += L" ...";

    return S_OK;
}

CCopyToMoveToMenu::CCopyToMoveToMenu() :
    m_idCmdFirst(0),
    m_idCmdLast(0),
    m_idCmdDoTo(-1),
    m_fnOldWndProc(NULL),
    m_bIgnoreTextBoxChange(FALSE)
{
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szPath[MAX_PATH];
    CCopyToMoveToMenu *this_ =
        reinterpret_cast<CCopyToMoveToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        if (!this_->m_bIgnoreTextBoxChange)
                        {
                            // get the text
                            GetDlgItemTextW(hwnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szPath, _countof(szPath));
                            StrTrimW(szPath, L" \t");

                            // update OK button
                            BOOL bValid = !PathIsRelative(szPath) && PathIsDirectoryW(szPath);
                            SendMessageW(hwnd, BFFM_ENABLEOK, 0, bValid);

                            return 0;
                        }

                        // reset flag
                        this_->m_bIgnoreTextBoxChange = FALSE;
                    }
                    break;
                }
            }
            break;
        }
    }
    return CallWindowProcW(this_->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
}

INT CALLBACK
CCopyToMoveToMenu::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    CCopyToMoveToMenu *this_ =
        reinterpret_cast<CCopyToMoveToMenu *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lpData);
            this_ = reinterpret_cast<CCopyToMoveToMenu *>(lpData);

            // Select initial directory
            SendMessageW(hwnd, BFFM_SETSELECTION, FALSE,
                reinterpret_cast<LPARAM>(static_cast<LPCITEMIDLIST>(this_->m_pidlFolder)));

            // Set caption
            CString strCaption(MAKEINTRESOURCEW(this_->GetDoItemsStringID()));
            SetWindowTextW(hwnd, strCaption);

            // Set OK button text
            CString strCopyOrMove(MAKEINTRESOURCEW(this_->GetDoButtonStringID()));
            SetDlgItemText(hwnd, IDOK, strCopyOrMove);

            // Subclassing
            this_->m_fnOldWndProc =
                reinterpret_cast<WNDPROC>(
                    SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));

            // Disable OK
            PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);
            break;
        }
        case BFFM_SELCHANGED:
        {
            if (!this_)
                break;

            WCHAR szPath[MAX_PATH];
            LPCITEMIDLIST pidl = reinterpret_cast<LPCITEMIDLIST>(lParam);

            szPath[0] = 0;
            SHGetPathFromIDListW(pidl, szPath);

            if (ILIsEqual(pidl, this_->m_pidlFolder))
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);
            else if (PathFileExistsW(szPath) || _ILIsDesktop(pidl))
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, TRUE);
            else
                PostMessageW(hwnd, BFFM_ENABLEOK, 0, FALSE);

            // the text box will be updated later soon, ignore it
            this_->m_bIgnoreTextBoxChange = TRUE;
            break;
        }
    }

    return FALSE;
}

STDMETHODIMP
CCopyToMoveToMenu::GetCommandString(
    UINT_PTR idCmd,
    UINT uType,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax);

    return E_NOTIMPL;
}

STDMETHODIMP
CCopyToMoveToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);
    return E_NOTIMPL;
}

STDMETHODIMP
CCopyToMoveToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pidlFolder.Attach(ILClone(pidlFolder));
    m_pDataObject = pdtobj;
    return S_OK;
}

STDMETHODIMP
CCopyToMoveToMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

STDMETHODIMP
CCopyToMoveToMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_pSite)
        return E_FAIL;

    return m_pSite->QueryInterface(riid, ppvSite);
}
