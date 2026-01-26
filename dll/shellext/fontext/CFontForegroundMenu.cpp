/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Fonts folder foreground menu implementation
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <undocshell.h>

WINE_DEFAULT_DEBUG_CHANNEL(fontext);

const char* DFM_TO_STR(UINT uMsg)
{
    switch(uMsg)
    {
    case DFM_MERGECONTEXTMENU: return "DFM_MERGECONTEXTMENU";
    case DFM_INVOKECOMMAND: return "DFM_INVOKECOMMAND";
    case DFM_MODIFYQCMFLAGS: return "DFM_MODIFYQCMFLAGS";
    case DFM_MERGECONTEXTMENU_TOP: return "DFM_MERGECONTEXTMENU_TOP";
    case DFM_MERGECONTEXTMENU_BOTTOM: return "DFM_MERGECONTEXTMENU_BOTTOM";
    case DFM_GETHELPTEXTW: return "DFM_GETHELPTEXTW";
    case DFM_GETVERBW: return "DFM_GETVERBW";
    case DFM_GETVERBA: return "DFM_GETVERBA";
    case DFM_WM_INITMENUPOPUP: return "DFM_WM_INITMENUPOPUP";
    case DFM_INVOKECOMMANDEX: return "DFM_INVOKECOMMANDEX";
    case DFM_GETDEFSTATICID:  return "DFM_GETDEFSTATICID";
    case 3: return "MENU_BEGIN";
    case 4: return "MENU_END";
    default: return "";
    }
}

static void RunFontViewer(HWND hwnd, const FontPidlEntry* fontEntry)
{
    CStringW Path = g_FontCache->GetFontFilePath(fontEntry->FileName());
    if (Path.IsEmpty())
        return;

    // '/d' disables the install button
    WCHAR FontPathArg[MAX_PATH + 3];
    StringCchPrintfW(FontPathArg, _countof(FontPathArg), L"/d %s", Path.GetString());
    PathQuoteSpacesW(FontPathArg + 3);

    SHELLEXECUTEINFOW si = { sizeof(si) };
    si.fMask = SEE_MASK_DOENVSUBST;
    si.hwnd = hwnd;
    si.lpFile = L"%SystemRoot%\\System32\\fontview.exe";
    si.lpParameters = FontPathArg;
    si.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&si);
}

class CFontForegroundMenu
    : public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IContextMenu3
{
    HWND m_hwnd = nullptr;
    CComPtr<IShellFolder> m_psf;
    CComPtr<IContextMenuCB> m_pmcb;
    LPFNDFMCALLBACK m_pfnmcb = nullptr;
    UINT m_cidl = 0;
    PCUITEMID_CHILD_ARRAY m_apidl = nullptr;
    CComPtr<IDataObject> m_pDataObj;

    HRESULT DoDelete();

public:
    CFontForegroundMenu();
    virtual ~CFontForegroundMenu();
    HRESULT WINAPI Initialize(const DEFCONTEXTMENU *pdcm);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;
    STDMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen) override;

    // IContextMenu2
    STDMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // IContextMenu3
    STDMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult) override;

    BEGIN_COM_MAP(CFontForegroundMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
    END_COM_MAP()
};

CFontForegroundMenu::CFontForegroundMenu()
{
}

CFontForegroundMenu::~CFontForegroundMenu()
{
}

HRESULT WINAPI CFontForegroundMenu::Initialize(const DEFCONTEXTMENU *pdcm)
{
    m_psf = pdcm->psf;
    m_pmcb = pdcm->pcmcb;
    m_hwnd = pdcm->hwnd;
    m_cidl = pdcm->cidl;
    m_apidl = pdcm->apidl;

    m_psf->GetUIObjectOf(pdcm->hwnd, m_cidl, m_apidl, IID_NULL_PPV_ARG(IDataObject, &m_pDataObj));
    return S_OK;
}

HRESULT CFontForegroundMenu::DoDelete()
{
    return DoDeleteFontFiles(m_hwnd, m_cidl, m_apidl);
}

// IContextMenu
STDMETHODIMP CFontForegroundMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    TRACE("%d\n", idCmdFirst);
    CString strPreview(MAKEINTRESOURCEW(IDS_FONT_PREVIEW));
    CString strCopy(MAKEINTRESOURCEW(IDS_COPY));
    CString strDelete(MAKEINTRESOURCEW(IDS_DELETE));
    CString strProp(MAKEINTRESOURCEW(IDS_PROPERTIES));
    INT idCmd = idCmdFirst;
    AppendMenuW(hMenu, MF_STRING, idCmd++, strPreview); // 0
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, idCmd++, strCopy); // 1
    AppendMenuW(hMenu, MF_STRING, idCmd++, strDelete); // 2
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, idCmd++, strProp); // 3
    SetMenuDefaultItem(hMenu, 0, TRUE);
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmd - idCmdFirst);
}

STDMETHODIMP CFontForegroundMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    INT idCmd = IS_INTRESOURCE(lpcmi->lpVerb) ? LOWORD(lpcmi->lpVerb) : -1;
    TRACE("%d\n", idCmd);

    if (idCmd == 0 || idCmd == FCIDM_SHVIEW_OPEN || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "open")))
    {
        for (UINT n = 0; n < m_cidl; ++n)
        {
            const FontPidlEntry* fontEntry = _FontFromIL(m_apidl[n]);
            RunFontViewer(m_hwnd, fontEntry);
        }
        return S_OK;
    }

    if (idCmd == 1 || idCmd == FCIDM_SHVIEW_COPY || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "copy")))
    {
        OleSetClipboard(m_pDataObj);
        return S_OK;
    }

    if (idCmd == 2 || idCmd == FCIDM_SHVIEW_DELETE || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "delete")))
    {
        return DoDelete();
    }

    if (idCmd == 3 || idCmd == FCIDM_SHVIEW_PROPERTIES || (idCmd == -1 && !lstrcmpiA(lpcmi->lpVerb, "properties")))
    {
        return SHMultiFileProperties(m_pDataObj, 0);
    }

    return S_FALSE;
}

STDMETHODIMP CFontForegroundMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    TRACE("%d\n", idCommand);
    if (idCommand == 0 || idCommand == FCIDM_SHVIEW_OPEN)
    {
        lstrcpynA(lpszName, "open", uMaxNameLen);
        return S_OK;
    }
    if (idCommand == 1 || idCommand == FCIDM_SHVIEW_COPY)
    {
        lstrcpynA(lpszName, "copy", uMaxNameLen);
        return S_OK;
    }
    if (idCommand == 2 || idCommand == FCIDM_SHVIEW_DELETE)
    {
        lstrcpynA(lpszName, "delete", uMaxNameLen);
        return S_OK;
    }
    if (idCommand == 3 || idCommand == FCIDM_SHVIEW_PROPERTIES)
    {
        lstrcpynA(lpszName, "properties", uMaxNameLen);
        return S_OK;
    }
    return E_FAIL;
}

// IContextMenu2
STDMETHODIMP CFontForegroundMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

// IContextMenu3
STDMETHODIMP CFontForegroundMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    return E_NOTIMPL;
}

HRESULT _CFontMenu_CreateInstance(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                  IShellFolder *psf, REFIID riid, LPVOID* ppvOut)
{
    DEFCONTEXTMENU dcm;
    ZeroMemory(&dcm, sizeof(dcm));
    dcm.hwnd = hwnd;
    dcm.psf = psf;
    dcm.cidl = cidl;
    dcm.apidl = apidl;
    return ShellObjectCreatorInit<CFontForegroundMenu>(&dcm, IID_PPV_ARG(IContextMenu, (IContextMenu **)ppvOut));
}
