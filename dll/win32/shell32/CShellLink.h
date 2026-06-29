/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
 *      Copyright 2009  Andrew Hill
 *      Copyright 2017  Hermes Belusca-Maito
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef _SHELLLINK_H_
#define _SHELLLINK_H_

class CShellLink :
    public CComCoClass<CShellLink, &CLSID_ShellLink>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellLinkA,
    public IShellLinkW,
    public IPersistStream,
    public IPersistFile,
    public IShellExtInit,
    public IContextMenu, // Technically it should be IContextMenu3 (inherits from IContextMenu2 and IContextMenu)
    public IDropTarget,
//  public IQueryInfo,
    public IShellLinkDataList,
    public IExtractIconA,
    public IExtractIconW,
//  public IExtractImage2, // Inherits from IExtractImage
//  public IPersistPropertyBag,
//  public IServiceProvider,
//  public IFilter,
    public IObjectWithSite,
//  public ICustomizeInfoTip,
    public IShellPropSheetExt
{
public:
    /* Link file formats */

    #include "pshpack1.h"
    struct volume_info
    {
        DWORD type;
        DWORD serial;
        WCHAR label[12];  /* assume 8.3 */
    };
    #include "poppack.h"

    enum IDCMD
    {
        IDCMD_OPEN = 0,
        IDCMD_OPENFILELOCATION
    };

private:
    /* Cached link header */
    SHELL_LINK_HEADER m_Header;

    /* Cached data set according to m_Header.dwFlags (SHELL_LINK_DATA_FLAGS) */

    LPITEMIDLIST  m_pPidl;

    /* Link tracker information */
    LPWSTR        m_sPath;
    volume_info   volume;

    LPWSTR        m_sDescription;
    LPWSTR        m_sPathRel;
    LPWSTR        m_sWorkDir;
    LPWSTR        m_sArgs;
    LPWSTR        m_sIcoPath;
    BOOL          m_bRunAs;
    BOOL          m_bDirty;
    LPDBLIST      m_pDBList; /* Optional data block list (in the extra data section) */
    BOOL          m_bInInit;    // in initialization or not
    HICON         m_hIcon;

    /* Pointers to strings inside Logo3/Darwin info blocks, cached for debug info purposes only */
    LPWSTR sProduct;
    LPWSTR sComponent;

    LPWSTR        m_sLinkPath;

    CComPtr<IUnknown>    m_site;
    CComPtr<IDropTarget> m_DropTarget;

    VOID Reset();

    HRESULT GetAdvertiseInfo(LPWSTR *str, DWORD dwSig);
    HRESULT SetAdvertiseInfo(LPCWSTR str);
    HRESULT WriteAdvertiseInfo(LPCWSTR string, DWORD dwSig);
    HRESULT SetTargetFromPIDLOrPath(LPCITEMIDLIST pidl, LPCWSTR pszFile);
    HICON CreateShortcutIcon(LPCWSTR wszIconPath, INT IconIndex);

    HRESULT DoOpen(LPCMINVOKECOMMANDINFO lpici);
    HRESULT DoOpenFileLocation();

public:
    CShellLink();
    ~CShellLink();
    static INT_PTR CALLBACK SH_ShellLinkDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL OnInitDialog(HWND hwndDlg, HWND hwndFocus, LPARAM lParam);
    void OnCommand(HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(HWND hwndDlg, int idFrom, LPNMHDR pnmhdr);
    void OnDestroy(HWND hwndDlg);

    // IPersistFile
    STDMETHOD(GetClassID)(CLSID *pclsid) override;
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(LPCOLESTR pszFileName, DWORD dwMode) override;
    STDMETHOD(Save)(LPCOLESTR pszFileName, BOOL fRemember) override;
    STDMETHOD(SaveCompleted)(LPCOLESTR pszFileName) override;
    STDMETHOD(GetCurFile)(LPOLESTR *ppszFileName) override;

    // IPersistStream
    // STDMETHOD(GetClassID)(CLSID *pclsid) override;
    // STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *stm) override;
    STDMETHOD(Save)(IStream *stm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // IShellLinkA
    STDMETHOD(GetPath)(LPSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags) override;
    STDMETHOD(GetIDList)(PIDLIST_ABSOLUTE *ppidl) override;
    STDMETHOD(SetIDList)(PCIDLIST_ABSOLUTE pidl) override;
    STDMETHOD(GetDescription)(LPSTR pszName, INT cchMaxName) override;
    STDMETHOD(SetDescription)(LPCSTR pszName) override;
    STDMETHOD(GetWorkingDirectory)(LPSTR pszDir, INT cchMaxPath) override;
    STDMETHOD(SetWorkingDirectory)(LPCSTR pszDir) override;
    STDMETHOD(GetArguments)(LPSTR pszArgs, INT cchMaxPath) override;
    STDMETHOD(SetArguments)(LPCSTR pszArgs) override;
    STDMETHOD(GetHotkey)(WORD *pwHotkey) override;
    STDMETHOD(SetHotkey)(WORD wHotkey) override;
    STDMETHOD(GetShowCmd)(INT *piShowCmd) override;
    STDMETHOD(SetShowCmd)(INT iShowCmd) override;
    STDMETHOD(GetIconLocation)(LPSTR pszIconPath, INT cchIconPath, INT *piIcon) override;
    STDMETHOD(SetIconLocation)(LPCSTR pszIconPath, INT iIcon) override;
    STDMETHOD(SetRelativePath)(LPCSTR pszPathRel, DWORD dwReserved) override;
    STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags) override;
    STDMETHOD(SetPath)(LPCSTR pszFile) override;

    // IShellLinkW
    STDMETHOD(GetPath)(LPWSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags) override;
    // STDMETHOD(GetIDList)(PIDLIST_ABSOLUTE *ppidl) override;
    // STDMETHOD(SetIDList)(PCIDLIST_ABSOLUTE pidl) override;
    STDMETHOD(GetDescription)(LPWSTR pszName, INT cchMaxName) override;
    STDMETHOD(SetDescription)(LPCWSTR pszName) override;
    STDMETHOD(GetWorkingDirectory)(LPWSTR pszDir, INT cchMaxPath) override;
    STDMETHOD(SetWorkingDirectory)(LPCWSTR pszDir) override;
    STDMETHOD(GetArguments)(LPWSTR pszArgs, INT cchMaxPath) override;
    STDMETHOD(SetArguments)(LPCWSTR pszArgs) override;
    // STDMETHOD(GetHotkey)(WORD *pwHotkey) override;
    // STDMETHOD(SetHotkey)(WORD wHotkey) override;
    // STDMETHOD(GetShowCmd)(INT *piShowCmd) override;
    // STDMETHOD(SetShowCmd)(INT iShowCmd) override;
    STDMETHOD(GetIconLocation)(LPWSTR pszIconPath, INT cchIconPath, INT *piIcon) override;
    STDMETHOD(SetIconLocation)(LPCWSTR pszIconPath, INT iIcon) override;
    STDMETHOD(SetRelativePath)(LPCWSTR pszPathRel, DWORD dwReserved) override;
    // STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags) override;
    STDMETHOD(SetPath)(LPCWSTR pszFile) override;

    // IShellLinkDataList
    STDMETHOD(AddDataBlock)(void *pDataBlock) override;
    STDMETHOD(CopyDataBlock)(DWORD dwSig, void **ppDataBlock) override;
    STDMETHOD(RemoveDataBlock)(DWORD dwSig) override;
    STDMETHOD(GetFlags)(DWORD *pdwFlags) override;
    STDMETHOD(SetFlags)(DWORD dwFlags) override;

    // IExtractIconA
    STDMETHOD(Extract)(PCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) override;
    STDMETHOD(GetIconLocation)(UINT uFlags, PSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags) override;

    // IExtractIconW
    STDMETHOD(Extract)(PCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) override;
    STDMETHOD(GetIconLocation)(UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags) override;

    // IShellExtInit
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID) override;

    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici) override;
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax) override;

    // IShellPropSheetExt
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) override;
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam) override;

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *punk) override;
    STDMETHOD(GetSite)(REFIID iid, void **ppvSite) override;

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect) override;

DECLARE_REGISTRY_RESOURCEID(IDR_SHELLLINK)
DECLARE_NOT_AGGREGATABLE(CShellLink)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellLink)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkA, IShellLinkA)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkW, IShellLinkW)
    COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistFile)
    COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu) // Technically it should be IContextMenu3
    COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
//  COM_INTERFACE_ENTRY_IID(IID_IQueryInfo, IQueryInfo)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkDataList, IShellLinkDataList)
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconA, IExtractIconA)
    COM_INTERFACE_ENTRY_IID(IID_IExtractIconW, IExtractIconW)
//  COM_INTERFACE_ENTRY_IID(IID_IExtractImage2, IExtractImage2)
//  COM_INTERFACE_ENTRY_IID(IID_IPersistPropertyBag, IPersistPropertyBag)
//  COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
//  COM_INTERFACE_ENTRY_IID(IID_IFilter, IFilter)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
//  COM_INTERFACE_ENTRY_IID(IID_ICustomizeInfoTip, ICustomizeInfoTip)
    COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
END_COM_MAP()
};

#endif /* _SHELLLINK_H_ */
