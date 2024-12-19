#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wincon.h>
#include <commdlg.h>
#include <ddeml.h>
#include <userenv.h>

#include <shlwapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>
#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atlsimpcoll.h>
#include <atlcoll.h>
#include <powrprof.h>
#include <winnetwk.h>
#include <objsafe.h>
#include <regstr.h>

#include <comctl32_undoc.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>

#define SHLWAPI_ISHELLFOLDER_HELPERS
#include <shlwapi_undoc.h>

#include <shellapi.h>
#undef ShellExecute
#include <undocshell.h>

/*
 * For versions < Vista+, redefine ShellMessageBoxW to ShellMessageBoxWrapW
 * (this is needed to avoid a linker error). On Vista+ onwards, shell32.ShellMessageBoxW
 * redirects to shlwapi.ShellMessageBoxW so the #define should not be needed.
 *
 * However our shell32 is built with _WIN32_WINNT set to 0x600 (Vista+),
 * yet its exports (especially regarding ShellMessageBoxA/W) are Win2003
 * compatible. So the #define is still needed, and the check be disabled.
 */
// #if (_WIN32_WINNT < 0x0600)
#define ShellMessageBoxW ShellMessageBoxWrapW
// #endif

#include <browseui_undoc.h>

#include <shellutils.h>

#include "shellrecyclebin/recyclebin.h"
#include "wine/pidl.h"
#include "debughlp.h"
#include "wine/shell32_main.h"
#include "shresdef.h"
#include "wine/cpanel.h"
#include "CActiveDesktop.h"
#include "CEnumIDListBase.h"
#include "shfldr.h"
#include "CShellItem.h"
#include "CShellLink.h"
#include "CFolderItemVerbs.h"
#include "CFolderItems.h"
#include "CFolder.h"
#include "CShellDispatch.h"
#include "CDropTargetHelper.h"
#include "CFolderOptions.h"
#include "folders/CFSFolder.h"
#include "folders/CDrivesFolder.h"
#include "folders/CDesktopFolder.h"
#include "folders/CControlPanelFolder.h"
#include "folders/CMyDocsFolder.h"
#include "folders/CNetFolder.h"
#include "folders/CFontsFolder.h"
#include "folders/CPrinterFolder.h"
#include "folders/CAdminToolsFolder.h"
#include "folders/CRecycleBin.h"
#include "droptargets/CexeDropHandler.h"
#include "droptargets/CFSDropTarget.h"
#include "COpenWithMenu.h"
#include "CNewMenu.h"
#include "CSendToMenu.h"
#include "CCopyMoveToMenu.h"
#include "CCopyAsPathMenu.h"
#include "dialogs/filedefext.h"
#include "dialogs/drvdefext.h"
#include "CQueryAssociations.h"
#include "shellmenu/CMenuBand.h"
#include "shellmenu/CMenuDeskBar.h"
#include "shellmenu/CMenuSite.h"
#include "shellmenu/CMergedFolder.h"
#include "shellmenu/shellmenu.h"
#include "CUserNotification.h"
#include "dialogs/folder_options.h"
#include "shelldesktop/CChangeNotifyServer.h"
#include "utils.h"

#include <wine/debug.h>
#include <wine/unicode.h>

extern const GUID CLSID_AdminFolderShortcut;
extern const GUID CLSID_FontsFolderShortcut;
extern const GUID CLSID_StartMenu;
extern const GUID CLSID_MenuBandSite;
extern const GUID CLSID_OpenWith;
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;
extern const GUID SHELL32_AdvtShortcutProduct;
extern const GUID SHELL32_AdvtShortcutComponent;

#define VERBKEY_CCHMAX 64 // Note: 63+\0 seems to be the limit on XP

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

static inline HRESULT
AddPropSheetPage(HPROPSHEETPAGE hPage, LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    if (!hPage)
        return E_FAIL;
    if (pfnAddPage(hPage, lParam))
        return S_OK;
    DestroyPropertySheetPage(hPage);
    return E_FAIL;
}

template<class T> static UINT CALLBACK
PropSheetPageLifetimeCallback(HWND hWnd, UINT uMsg, PROPSHEETPAGEW *pPSP)
{
    if (uMsg == PSPCB_RELEASE)
        ((T*)(pPSP->lParam))->Release();
    return TRUE;
}

EXTERN_C HRESULT WINAPI
SHMultiFileProperties(IDataObject *pDataObject, DWORD dwFlags);
HRESULT
SHELL32_ShowPropertiesDialog(IDataObject *pdtobj);
HRESULT
SHELL32_ShowFilesystemItemPropertiesDialogAsync(IDataObject *pDO);
HRESULT
SHELL32_ShowShellExtensionProperties(const CLSID *pClsid, IDataObject *pDO);
HRESULT
SHELL_ShowItemIDListProperties(LPCITEMIDLIST pidl);

HRESULT
SHELL32_DefaultContextMenuCallBack(IShellFolder *psf, IDataObject *pdo, UINT msg);
UINT
MapVerbToDfmCmd(_In_ LPCSTR verba);
UINT
GetDfmCmd(_In_ IContextMenu *pCM, _In_ LPCSTR verba);
#define SHELL_ExecuteControlPanelCPL(hwnd, cpl) SHRunControlPanel((cpl), (hwnd))

#define CmicFlagsToSeeFlags(flags)  ((flags) & SEE_CMIC_COMMON_FLAGS)
static inline UINT SeeFlagsToCmicFlags(UINT flags)
{
    if (flags & SEE_MASK_CLASSNAME)
        flags &= ~(SEE_MASK_HASLINKNAME | SEE_MASK_HASTITLE);
    return flags & SEE_CMIC_COMMON_FLAGS;
}


// CStubWindow32 --- The owner window of file property sheets.
// This window hides taskbar button of property sheet.
#define CSTUBWINDOW32_CLASSNAME _T("StubWindow32")
class CStubWindow32 : public CWindowImpl<CStubWindow32>
{
    static HWND FindStubWindow(UINT Type, LPCWSTR Path);
public:
    DECLARE_WND_CLASS_EX(CSTUBWINDOW32_CLASSNAME, 0, COLOR_WINDOWTEXT)
    enum {
        TYPE_FORMATDRIVE = 1,
        TYPE_PROPERTYSHEET,
    };
    static LPCWSTR GetTypePropName() { return L"StubType"; }
    HRESULT CreateStub(UINT Type, LPCWSTR Path, const POINT *pPt);

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (HICON hIco = (HICON)SendMessage(WM_SETICON, ICON_BIG, NULL))
            DestroyIcon(hIco);
        ::RemovePropW(m_hWnd, GetTypePropName());
        return 0;
    }

    BEGIN_MSG_MAP(CStubWindow32)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()
};

void PostCabinetMessage(UINT Msg, WPARAM wParam, LPARAM lParam);

HRESULT
Shell_TranslateIDListAlias(
    _In_ LPCITEMIDLIST pidl,
    _In_ HANDLE hToken,
    _Out_ LPITEMIDLIST *ppidlAlias,
    _In_ DWORD dwFlags);

BOOL BindCtx_ContainsObject(_In_ IBindCtx *pBindCtx, _In_ LPCWSTR pszName);
DWORD BindCtx_GetMode(_In_ IBindCtx *pbc, _In_ DWORD dwDefault);
BOOL SHSkipJunctionBinding(_In_ IBindCtx *pbc, _In_ CLSID *pclsid);
HRESULT SHIsFileSysBindCtx(_In_ IBindCtx *pBindCtx, _Out_opt_ WIN32_FIND_DATAW *pFindData);
BOOL Shell_FailForceReturn(_In_ HRESULT hr);

EXTERN_C INT
Shell_ParseSpecialFolder(_In_ LPCWSTR pszStart, _Out_ LPWSTR *ppch, _Out_ INT *pcch);

HRESULT
Shell_DisplayNameOf(
    _In_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_ LPWSTR pszBuf,
    _In_ UINT cchBuf);

EXTERN_C
HRESULT SHBindToObject(
    _In_opt_ IShellFolder *psf,
    _In_ LPCITEMIDLIST pidl,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

HRESULT
SHBindToObjectEx(
    _In_opt_ IShellFolder *pShellFolder,
    _In_ LPCITEMIDLIST pidl,
    _In_opt_ IBindCtx *pBindCtx,
    _In_ REFIID riid,
    _Out_ void **ppvObj);

EXTERN_C HRESULT
SHELL_GetUIObjectOfAbsoluteItem(
    _In_opt_ HWND hWnd,
    _In_ PCIDLIST_ABSOLUTE pidl,
    _In_ REFIID riid, _Out_ void **ppvObj);

DWORD
SHGetAttributes(_In_ IShellFolder *psf, _In_ LPCITEMIDLIST pidl, _In_ DWORD dwAttributes);
HRESULT SHELL_GetIDListTarget(_In_ LPCITEMIDLIST pidl, _Out_ PIDLIST_ABSOLUTE *ppidl);
HRESULT SHCoInitializeAnyApartment(VOID);

HRESULT
SHGetNameAndFlagsW(
    _In_ LPCITEMIDLIST pidl,
    _In_ DWORD dwFlags,
    _Out_opt_ LPWSTR pszText,
    _In_ UINT cchBuf,
    _Inout_opt_ DWORD *pdwAttributes);

EXTERN_C HWND BindCtx_GetUIWindow(_In_ IBindCtx *pBindCtx);

EXTERN_C HRESULT
BindCtx_RegisterObjectParam(
    _In_ IBindCtx *pBindCtx,
    _In_ LPOLESTR pszKey,
    _In_opt_ IUnknown *punk,
    _Out_ LPBC *ppbc);

BOOL PathIsDotOrDotDotW(_In_ LPCWSTR pszPath);
BOOL PathIsValidElement(_In_ LPCWSTR pszPath);
BOOL PathIsDosDevice(_In_ LPCWSTR pszName);
HRESULT SHILAppend(_Inout_ LPITEMIDLIST pidl, _Inout_ LPITEMIDLIST *ppidl);

PIDLIST_ABSOLUTE SHELL_CIDA_ILCloneFull(_In_ const CIDA *pCIDA, _In_ UINT Index);
PIDLIST_ABSOLUTE SHELL_DataObject_ILCloneFullItem(_In_ IDataObject *pDO, _In_ UINT Index);

EXTERN_C HRESULT
IUnknown_InitializeCommand(
    _In_ IUnknown *pUnk,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB);
EXTERN_C HRESULT
InvokeIExecuteCommand(
    _In_ IExecuteCommand *pEC,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB,
    _In_opt_ IShellItemArray *pSIA,
    _In_opt_ LPCMINVOKECOMMANDINFOEX pICI,
    _In_opt_ IUnknown *pSite);
EXTERN_C HRESULT
InvokeIExecuteCommandWithDataObject(
    _In_ IExecuteCommand *pEC,
    _In_ PCWSTR pszCommandName,
    _In_opt_ IPropertyBag *pPB,
    _In_ IDataObject *pDO,
    _In_opt_ LPCMINVOKECOMMANDINFOEX pICI,
    _In_opt_ IUnknown *pSite);

#endif /* _PRECOMP_H__ */
