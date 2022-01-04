/*
 * PROJECT:     ReactOS Font Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CFontMenu implementation
 * COPYRIGHT:   Copyright 2019-2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "precomp.h"

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
    WCHAR FontViewerPath[MAX_PATH] = L"%SystemRoot%\\System32\\fontview.exe";
    WCHAR FontPathArg[MAX_PATH + 3];

    CStringW Path = g_FontCache->Filename(g_FontCache->Find(fontEntry), true);
    if (!Path.IsEmpty())
    {
        // '/d' disables the install button
        StringCchPrintfW(FontPathArg, _countof(FontPathArg), L"/d %s", Path.GetString());
        PathQuoteSpacesW(FontPathArg + 3);

        SHELLEXECUTEINFOW si = { sizeof(si) };
        si.fMask = SEE_MASK_DOENVSUBST;
        si.hwnd = hwnd;
        si.lpFile = FontViewerPath;
        si.lpParameters = FontPathArg;
        si.nShow = SW_SHOWNORMAL;
        ShellExecuteExW(&si);
    }
}

static HRESULT CALLBACK FontFolderMenuCallback(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj,
                                               UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("FontFolderMenuCallback(%u {%s})\n", uMsg, DFM_TO_STR(uMsg));
    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
    {
        QCMINFO *pqcminfo = (QCMINFO *)lParam;

        CStringW menuText(MAKEINTRESOURCEW(IDS_FONT_PREVIEW));
        MENUITEMINFOW cmi = { sizeof(cmi) };
        cmi.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
        cmi.fType = MFT_STRING;
        cmi.fState = MFS_DEFAULT;
        cmi.wID = pqcminfo->idCmdFirst++;
        cmi.dwTypeData = (LPWSTR)menuText.GetString();
        InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu, TRUE, &cmi);

        return S_OK;
    }
    case DFM_INVOKECOMMAND:
        // Preview is the only item we handle
        if (wParam == 0)
        {
            CDataObjectHIDA cida(pdtobj);

            if (FAILED_UNEXPECTEDLY(cida.hr()))
                return cida.hr();

            for (UINT n = 0; n < cida->cidl; ++n)
            {
                const FontPidlEntry* fontEntry = _FontFromIL(HIDA_GetPIDLItem(cida, n));
                RunFontViewer(hwnd, fontEntry);
            }
            return S_OK;
        }
        else if (wParam == DFM_CMD_PROPERTIES)
        {
            ERR("Default properties handling!\n");
            return S_FALSE;
        }
        else
        {
            ERR("Unhandled DFM_INVOKECOMMAND(wParam=0x%x)\n", wParam);
        }
        return S_FALSE;

    case DFM_INVOKECOMMANDEX:
        return E_NOTIMPL;
    case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
        return S_FALSE;
    }
    return E_NOTIMPL;
}


HRESULT _CFontMenu_CreateInstance(HWND hwnd, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
                                  IShellFolder *psf, REFIID riid, LPVOID* ppvOut)
{
    if (cidl > 0)
    {
        HKEY keys[1] = {0};
        int nkeys = 0;
        CComPtr<IContextMenu> spMenu;

        // Use the default context menu handler, but augment it from the callbacks
        HRESULT hr = CDefFolderMenu_Create2(NULL, hwnd, cidl, apidl, psf, FontFolderMenuCallback, nkeys, keys, &spMenu);

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        // See if the requested interface (e.g. IContextMenu3) is also available
        return spMenu->QueryInterface(riid, ppvOut);
    }

    // We can't create a background menu
    return E_FAIL;
}

