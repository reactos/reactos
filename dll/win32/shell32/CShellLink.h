/*
 *
 *      Copyright 1997  Marcus Meissner
 *      Copyright 1998  Juergen Schmied
 *      Copyright 2005  Mike McCormack
 *      Copyright 2009  Andrew Hill
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
    public IPersistFile,
    public IPersistStream,
    public IShellLinkDataList,
    public IShellExtInit,
    public IContextMenu,
    public IDropTarget,
    public IObjectWithSite,
    public IShellPropSheetExt
{
public:
    /* link file formats */

    #include "pshpack1.h"

    struct volume_info
    {
        DWORD        type;
        DWORD        serial;
        WCHAR        label[12];  /* assume 8.3 */
    };

    #include "poppack.h"

private:
    /* data structures according to the information in the link */
    LPITEMIDLIST    pPidl;
    WORD        wHotKey;
    SYSTEMTIME    time1;
    SYSTEMTIME    time2;
    SYSTEMTIME    time3;

    DWORD         iShowCmd;
    LPWSTR        sIcoPath;
    INT           iIcoNdx;
    LPWSTR        sPath;
    LPWSTR        sArgs;
    LPWSTR        sWorkDir;
    LPWSTR        sDescription;
    LPWSTR        sPathRel;
    LPWSTR        sProduct;
    LPWSTR        sComponent;
    volume_info   volume;
    LPWSTR        sLinkPath;
    BOOL          bRunAs;
    BOOL          bDirty;
    INT           iIdOpen;  /* id of the "Open" entry in the context menu */
    CComPtr<IUnknown>        site;
    CComPtr<IDropTarget>   mDropTarget;
public:
    CShellLink();
    ~CShellLink();
    LPWSTR ShellLink_GetAdvertisedArg(LPCWSTR str);
    HRESULT ShellLink_SetAdvertiseInfo(LPCWSTR str);
    static INT_PTR CALLBACK SH_ShellLinkDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IPersistFile
    virtual HRESULT WINAPI GetClassID(CLSID *pclsid);
    virtual HRESULT WINAPI IsDirty();
    virtual HRESULT WINAPI Load(LPCOLESTR pszFileName, DWORD dwMode);
    virtual HRESULT WINAPI Save(LPCOLESTR pszFileName, BOOL fRemember);
    virtual HRESULT WINAPI SaveCompleted(LPCOLESTR pszFileName);
    virtual HRESULT WINAPI GetCurFile(LPOLESTR *ppszFileName);

    // IPersistStream
    // virtual WINAPI HRESULT GetClassID(CLSID *pclsid);
    // virtual HRESULT WINAPI IsDirty();
    virtual HRESULT WINAPI Load(IStream *stm);
    virtual HRESULT WINAPI Save(IStream *stm, BOOL fClearDirty);
    virtual HRESULT WINAPI GetSizeMax(ULARGE_INTEGER *pcbSize);

    // IShellLinkA
    virtual HRESULT WINAPI GetPath(LPSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags);
    virtual HRESULT WINAPI GetIDList(LPITEMIDLIST * ppidl);
    virtual HRESULT WINAPI SetIDList(LPCITEMIDLIST pidl);
    virtual HRESULT WINAPI GetDescription(LPSTR pszName,INT cchMaxName);
    virtual HRESULT WINAPI SetDescription(LPCSTR pszName);
    virtual HRESULT WINAPI GetWorkingDirectory(LPSTR pszDir,INT cchMaxPath);
    virtual HRESULT WINAPI SetWorkingDirectory(LPCSTR pszDir);
    virtual HRESULT WINAPI GetArguments(LPSTR pszArgs,INT cchMaxPath);
    virtual HRESULT WINAPI SetArguments(LPCSTR pszArgs);
    virtual HRESULT WINAPI GetHotkey(WORD *pwHotkey);
    virtual HRESULT WINAPI SetHotkey(WORD wHotkey);
    virtual HRESULT WINAPI GetShowCmd(INT *piShowCmd);
    virtual HRESULT WINAPI SetShowCmd(INT iShowCmd);
    virtual HRESULT WINAPI GetIconLocation(LPSTR pszIconPath,INT cchIconPath,INT *piIcon);
    virtual HRESULT WINAPI SetIconLocation(LPCSTR pszIconPath,INT iIcon);
    virtual HRESULT WINAPI SetRelativePath(LPCSTR pszPathRel, DWORD dwReserved);
    virtual HRESULT WINAPI Resolve(HWND hwnd, DWORD fFlags);
    virtual HRESULT WINAPI SetPath(LPCSTR pszFile);

    // IShellLinkW
    virtual HRESULT WINAPI GetPath(LPWSTR pszFile, INT cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);
    // virtual HRESULT WINAPI GetIDList(LPITEMIDLIST *ppidl);
    // virtual HRESULT WINAPI SetIDList(LPCITEMIDLIST pidl);
    virtual HRESULT WINAPI GetDescription(LPWSTR pszName, INT cchMaxName);
    virtual HRESULT WINAPI SetDescription(LPCWSTR pszName);
    virtual HRESULT WINAPI GetWorkingDirectory(LPWSTR pszDir, INT cchMaxPath);
    virtual HRESULT WINAPI SetWorkingDirectory(LPCWSTR pszDir);
    virtual HRESULT WINAPI GetArguments(LPWSTR pszArgs,INT cchMaxPath);
    virtual HRESULT WINAPI SetArguments(LPCWSTR pszArgs);
    // virtual HRESULT WINAPI GetHotkey(WORD *pwHotkey);
    // virtual HRESULT WINAPI SetHotkey(WORD wHotkey);
    // virtual HRESULT WINAPI GetShowCmd(INT *piShowCmd);
    // virtual HRESULT WINAPI SetShowCmd(INT iShowCmd);
    virtual HRESULT WINAPI GetIconLocation(LPWSTR pszIconPath,INT cchIconPath,INT *piIcon);
    virtual HRESULT WINAPI SetIconLocation(LPCWSTR pszIconPath,INT iIcon);
    virtual HRESULT WINAPI SetRelativePath(LPCWSTR pszPathRel, DWORD dwReserved);
    // virtual HRESULT WINAPI Resolve(HWND hwnd, DWORD fFlags);
    virtual HRESULT WINAPI SetPath(LPCWSTR pszFile);

    // IShellLinkDataList
    virtual HRESULT WINAPI AddDataBlock(void *pDataBlock);
    virtual HRESULT WINAPI CopyDataBlock(DWORD dwSig, void **ppDataBlock);
    virtual HRESULT WINAPI RemoveDataBlock(DWORD dwSig);
    virtual HRESULT WINAPI GetFlags(DWORD *pdwFlags);
    virtual HRESULT WINAPI SetFlags(DWORD dwFlags);

    // IShellExtInit
    virtual HRESULT WINAPI Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IContextMenu
    virtual HRESULT WINAPI QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    virtual HRESULT WINAPI GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // IShellPropSheetExt
    virtual HRESULT WINAPI AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    virtual HRESULT WINAPI ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam);

    // IObjectWithSite
    virtual HRESULT WINAPI SetSite(IUnknown *punk);
    virtual HRESULT WINAPI GetSite(REFIID iid, void **ppvSite);

    // IDropTarget
    virtual HRESULT WINAPI DragEnter(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT WINAPI DragOver(DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT WINAPI DragLeave();
    virtual HRESULT WINAPI Drop(IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);

DECLARE_REGISTRY_RESOURCEID(IDR_SHELLLINK)
DECLARE_NOT_AGGREGATABLE(CShellLink)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CShellLink)
    COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistFile)
    COM_INTERFACE_ENTRY_IID(IID_IPersistFile, IPersistFile)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkA, IShellLinkA)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkW, IShellLinkW)
    COM_INTERFACE_ENTRY_IID(IID_IShellLinkDataList, IShellLinkDataList)
    COM_INTERFACE_ENTRY_IID(IID_IShellExtInit, IShellExtInit)
    COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
    COM_INTERFACE_ENTRY_IID(IID_IShellPropSheetExt, IShellPropSheetExt)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
END_COM_MAP()
};

#endif /* _SHELLLINK_H_ */
