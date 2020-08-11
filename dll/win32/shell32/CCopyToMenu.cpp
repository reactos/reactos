#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CCopyToMenu::CCopyToMenu() :
    m_idCmdFirst(0),
    m_idCmdLast(0),
    m_idCmdCopyTo(-1)
{
}

CCopyToMenu::~CCopyToMenu()
{
}

HRESULT CCopyToMenu::DoCopyToFolder(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    HRESULT hr;

    hr = SHGetPathFromIDListW(m_pidlFolder, wszPath);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    BROWSEINFOW info = { lpici->hwnd };
    info.pidlRoot = NULL;
    info.lpszTitle = L"FIXME: This is a title";
    info.ulFlags = BIF_RETURNONLYFSDIRS;
    CComHeapPtr<ITEMIDLIST> pidl(SHBrowseForFolder(&info));
    if (pidl)
    {
        // FIXME
        return S_OK;
    }

    return E_FAIL;
}

HRESULT WINAPI
CCopyToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[200];
    UINT Pos = ::GetMenuItemCount(hMenu);

    m_idCmdFirst = m_idCmdLast = idCmdFirst;

    if (!LoadStringW(shell32_hInstance, IDS_COPYTOMENU, wszBuf, _countof(wszBuf)))
        wszBuf[0] = 0;

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STRING | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = wszBuf;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdLast;
    mii.fState = MFS_ENABLED;
    if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
    {
        m_idCmdCopyTo = m_idCmdLast++;
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, m_idCmdLast - m_idCmdFirst + 1);
}

HRESULT WINAPI
CCopyToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;

    if (HIWORD(lpici->lpVerb) == 0 &&
        m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdCopyTo)
    {
        hr = DoCopyToFolder(lpici);
    }

    TRACE("CCopyToMenu::InvokeCommand %x\n", hr);
    return hr;
}

HRESULT WINAPI
CCopyToMenu::GetCommandString(UINT_PTR idCmd,
                              UINT uType,
                              UINT *pwReserved,
                              LPSTR pszName,
                              UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax);

    return E_NOTIMPL;
}

HRESULT WINAPI
CCopyToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);
    return E_NOTIMPL;
}

HRESULT WINAPI
CCopyToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                        IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pidlFolder.Attach(ILClone(pidlFolder));
    return S_OK;
}
