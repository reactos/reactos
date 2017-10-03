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

    /* Pointers to strings inside Logo3/Darwin info blocks, cached for debug info purposes only */
    LPWSTR sProduct;
    LPWSTR sComponent;

    LPWSTR        m_sLinkPath;
    INT           m_iIdOpen;     /* ID of the "Open" entry in the context menu */

    CComPtr<IUnknown>    m_site;
    CComPtr<IDropTarget> m_DropTarget;

    VOID Reset();

    HRESULT GetAdvertiseInfo(LPWSTR *str, DWORD dwSig);
    HRESULT SetAdvertiseInfo(LPCWSTR str);
    HRESULT WriteAdvertiseInfo(LPCWSTR string, DWORD dwSig);
    HRESULT SetTargetFromPIDLOrPath(LPCITEMIDLIST pidl, LPCWSTR pszFile);

public:
    CShellLink();
    ~CShellLink();
    static INT_PTR CALLBACK SH_ShellLinkDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IPersistFile
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pclsid);
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE Save(LPCOLESTR pszFileName, BOOL fRemember);
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR pszFileName);
    virtual HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *ppszFileName);

    // IPersistStream
    // virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pclsid);
    // virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *stm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *stm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // IShellLinkA
    virtual HRESULT STDMETHODCALLTYPE GetPath(LPSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags);
    virtual HRESULT STDMETHODCALLTYPE GetIDList(LPITEMIDLIST *ppidl);
    virtual HRESULT STDMETHODCALLTYPE SetIDList(LPCITEMIDLIST pidl);
    virtual HRESULT STDMETHODCALLTYPE GetDescription(LPSTR pszName, INT cchMaxName);
    virtual HRESULT STDMETHODCALLTYPE SetDescription(LPCSTR pszName);
    virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory(LPSTR pszDir, INT cchMaxPath);
    virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory(LPCSTR pszDir);
    virtual HRESULT STDMETHODCALLTYPE GetArguments(LPSTR pszArgs, INT cchMaxPath);
    virtual HRESULT STDMETHODCALLTYPE SetArguments(LPCSTR pszArgs);
    virtual HRESULT STDMETHODCALLTYPE GetHotkey(WORD *pwHotkey);
    virtual HRESULT STDMETHODCALLTYPE SetHotkey(WORD wHotkey);
    virtual HRESULT STDMETHODCALLTYPE GetShowCmd(INT *piShowCmd);
    virtual HRESULT STDMETHODCALLTYPE SetShowCmd(INT iShowCmd);
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(LPSTR pszIconPath, INT cchIconPath, INT *piIcon);
    virtual HRESULT STDMETHODCALLTYPE SetIconLocation(LPCSTR pszIconPath, INT iIcon);
    virtual HRESULT STDMETHODCALLTYPE SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE Resolve(HWND hwnd, DWORD fFlags);
    virtual HRESULT STDMETHODCALLTYPE SetPath(LPCSTR pszFile);

    // IShellLinkW
    virtual HRESULT STDMETHODCALLTYPE GetPath(LPWSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);
    // virtual HRESULT STDMETHODCALLTYPE GetIDList(LPITEMIDLIST *ppidl);
    // virtual HRESULT STDMETHODCALLTYPE SetIDList(LPCITEMIDLIST pidl);
    virtual HRESULT STDMETHODCALLTYPE GetDescription(LPWSTR pszName, INT cchMaxName);
    virtual HRESULT STDMETHODCALLTYPE SetDescription(LPCWSTR pszName);
    virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory(LPWSTR pszDir, INT cchMaxPath);
    virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory(LPCWSTR pszDir);
    virtual HRESULT STDMETHODCALLTYPE GetArguments(LPWSTR pszArgs, INT cchMaxPath);
    virtual HRESULT STDMETHODCALLTYPE SetArguments(LPCWSTR pszArgs);
    // virtual HRESULT STDMETHODCALLTYPE GetHotkey(WORD *pwHotkey);
    // virtual HRESULT STDMETHODCALLTYPE SetHotkey(WORD wHotkey);
    // virtual HRESULT STDMETHODCALLTYPE GetShowCmd(INT *piShowCmd);
    // virtual HRESULT STDMETHODCALLTYPE SetShowCmd(INT iShowCmd);
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(LPWSTR pszIconPath, INT cchIconPath, INT *piIcon);
    virtual HRESULT STDMETHODCALLTYPE SetIconLocation(LPCWSTR pszIconPath, INT iIcon);
    virtual HRESULT STDMETHODCALLTYPE SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved);
    // virtual HRESULT STDMETHODCALLTYPE Resolve(HWND hwnd, DWORD fFlags);
    virtual HRESULT STDMETHODCALLTYPE SetPath(LPCWSTR pszFile);

    // IShellLinkDataList
    virtual HRESULT STDMETHODCALLTYPE AddDataBlock(void *pDataBlock);
    virtual HRESULT STDMETHODCALLTYPE CopyDataBlock(DWORD dwSig, void **ppDataBlock);
    virtual HRESULT STDMETHODCALLTYPE RemoveDataBlock(DWORD dwSig);
    virtual HRESULT STDMETHODCALLTYPE GetFlags(DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetFlags(DWORD dwFlags);

    // IExtractIconA
    virtual HRESULT STDMETHODCALLTYPE Extract(PCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, PSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);

    // IExtractIconW
    virtual HRESULT STDMETHODCALLTYPE Extract(PCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
    virtual HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, PWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);

    // IShellExtInit
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IContextMenu
    virtual HRESULT STDMETHODCALLTYPE QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT STDMETHODCALLTYPE InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    virtual HRESULT STDMETHODCALLTYPE GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IShellPropSheetExt
    virtual HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam);

    // IObjectWithSite
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID iid, void **ppvSite);

    // IDropTarget
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);

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
