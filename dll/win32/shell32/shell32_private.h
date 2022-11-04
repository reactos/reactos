#ifndef SHELL32_PRIVATE_H
#define SHELL32_PRIVATE_H

extern const GUID CLSID_AdminFolderShortcut;
extern const GUID CLSID_FontsFolderShortcut;
extern const GUID CLSID_StartMenu;
extern const GUID CLSID_MenuBandSite;
extern const GUID CLSID_OpenWith;
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;
extern const GUID SHELL32_AdvtShortcutProduct;
extern const GUID SHELL32_AdvtShortcutComponent;

#define MAX_PROPERTY_SHEET_PAGE 32

extern inline
BOOL
CALLBACK
AddPropSheetPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADERW *pHeader = (PROPSHEETHEADERW *)lParam;

    if (pHeader->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        pHeader->phpage[pHeader->nPages++] = hPage;
        return TRUE;
    }

    return FALSE;
}

HRESULT WINAPI
Shell_DefaultContextMenuCallBack(IShellFolder *psf, IDataObject *pdtobj);

// CStubWindow32 --- The owner window of file property sheets.
// This window hides taskbar button of property sheet.
class CStubWindow32 : public CWindowImpl<CStubWindow32>
{
public:
    DECLARE_WND_CLASS_EX(_T("StubWindow32"), 0, COLOR_WINDOWTEXT)

    BEGIN_MSG_MAP(CStubWindow32)
    END_MSG_MAP()
};


DWORD
Clipboard_GetDropEffect(IShellFolder *pShellFolder);

HRESULT
IContextMenu_HandleMenuMsg2(IContextMenu *pcm, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);


#endif /* SHELL32_PRIVATE_H */
