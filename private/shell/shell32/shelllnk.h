/*++

Revision History:

    07/20/98    arulk    Created from C source for this object.

--*/

#ifndef __SHLINK_H__
#define __SHLINK_H__

#include "tracker.h"
#include "cowsite.h"
#include <filter.h>         // COL_DATA

#if defined(__cplusplus)

class CShellLink : public IShellLinkA,
                   public IShellLinkW,
                   public IPersistStream,
                   public IPersistFile,
                   public IShellExtInit,
                   public IContextMenu3,
                   public IDropTarget,
                   public IQueryInfo,
                   public IShellLinkDataList,
                   public IExtractIconA,
                   public IExtractIconW,
                   public IPersistPropertyBag,
                   public IServiceProvider,
                   public IFilter,
                   public CObjectWithSite
{

public:
    CShellLink();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    
    // IShellLinkA methods
    STDMETHOD(GetPath)(LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD flags);
    STDMETHOD(SetPath)(LPCSTR pszFile);
    //STDMETHOD(GetIDList)(LPITEMIDLIST *ppidl);
    //STDMETHOD(SetIDList)(LPCITEMIDLIST pidl);
    STDMETHOD(GetDescription)(LPSTR pszName, int cchMaxName);
    STDMETHOD(SetDescription)(LPCSTR pszName);
    STDMETHOD(GetWorkingDirectory)(LPSTR pszDir, int cchMaxPath);
    STDMETHOD(SetWorkingDirectory)(LPCSTR pszDir);
    STDMETHOD(GetArguments)(LPSTR pszArgs, int cchMaxPath);
    STDMETHOD(SetArguments)(LPCSTR pszArgs);
    //STDMETHOD(GetHotkey)(WORD *pwHotkey);
    //STDMETHOD(SetHotkey)(WORD wHotkey);
    //STDMETHOD(GetShowCmd)(int *piShowCmd);
    //STDMETHOD(SetShowCmd)(int iShowCmd);
    STDMETHOD(GetIconLocation)(LPSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHOD(SetIconLocation)(LPCSTR pszIconPath, int iIcon);
    //STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags);
    STDMETHOD(SetRelativePath)(LPCSTR pszPathRel, DWORD dwReserved);
    
    // IShellLinkW
    STDMETHOD(GetPath)(LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags);
    STDMETHOD(GetIDList)(LPITEMIDLIST *ppidl);
    STDMETHOD(SetIDList)(LPCITEMIDLIST pidl);
    STDMETHOD(GetDescription)(LPWSTR pszName, int cchMaxName);
    STDMETHOD(SetDescription)(LPCWSTR pszName);
    STDMETHOD(GetWorkingDirectory)(LPWSTR pszDir, int cchMaxPath);
    STDMETHOD(SetWorkingDirectory)(LPCWSTR pszDir);
    STDMETHOD(GetArguments)(LPWSTR pszArgs, int cchMaxPath);
    STDMETHOD(SetArguments)(LPCWSTR pszArgs);
    STDMETHOD(GetHotkey)(WORD *pwHotKey);
    STDMETHOD(SetHotkey)(WORD wHotkey);
    STDMETHOD(GetShowCmd)(int *piShowCmd);
    STDMETHOD(SetShowCmd)(int iShowCmd);
    STDMETHOD(GetIconLocation)(LPWSTR pszIconPath, int cchIconPath, int *piIcon);
    STDMETHOD(SetIconLocation)(LPCWSTR pszIconPath, int iIcon);
    STDMETHOD(SetRelativePath)(LPCWSTR pszPathRel, DWORD dwReserved);
    STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags);
    STDMETHOD(SetPath)(LPCWSTR pszFile);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();

    // IPersistStream
    STDMETHOD(Load)(IStream *pstm);
    STDMETHOD(Save)(IStream *pstm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    // IPersistFile
    STDMETHOD(Load)(LPCOLESTR pwszFile, DWORD grfMode);
    STDMETHOD(Save)(LPCOLESTR pwszFile, BOOL fRemember);
    STDMETHOD(SaveCompleted)(LPCOLESTR pwszFile);
    STDMETHOD(GetCurFile)(LPOLESTR *lplpszFileName);

    // IPersistPropertyBag
    STDMETHOD(Save)(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    STDMETHOD(Load)(IPropertyBag* pPropBag, IErrorLog* pErrorLog);
    STDMETHOD(InitNew)(void);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    // IContextMenu3
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO piciIn);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT wFlags, UINT *pmf, LPSTR pszName, UINT cchMax);
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)();
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IQueryInfo
    STDMETHOD(GetInfoTip)(DWORD dwFlags, WCHAR **ppwszTip);
    STDMETHOD(GetInfoFlags)(LPDWORD pdwFlags);

    // IShellLinkDataList
    STDMETHOD(AddDataBlock)(void *pdb);
    STDMETHOD(CopyDataBlock)(DWORD dwSig, void **ppdb);
    STDMETHOD(RemoveDataBlock)(DWORD dwSig);
    STDMETHOD(GetFlags)(LPDWORD pdwFlags);
    STDMETHOD(SetFlags)(DWORD dwFlags);
    
    // IExtractIconA
    STDMETHOD(GetIconLocation)(UINT uFlags,LPSTR szIconFile,UINT cchMax,int *piIndex,UINT * pwFlags);
    STDMETHOD(Extract)(LPCSTR pszFile,UINT nIconIndex,HICON *phiconLarge,HICON *phiconSmall,UINT nIcons);

    // IExtractIconW
    STDMETHOD(GetIconLocation)(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
    STDMETHOD(Extract)(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppv);

    // IFilter
    STDMETHOD(Init)(ULONG grfFlags, ULONG cAttributes, const FULLPROPSPEC *aAttributes, ULONG *pFlags);
    STDMETHOD(GetChunk)(STAT_CHUNK *pStat);
    STDMETHOD(GetText)(ULONG *pcwcBuffer, WCHAR *awcBuffer);
    STDMETHOD(GetValue)(PROPVARIANT **ppPropValue);
    STDMETHOD(BindRegion)(FILTERREGION origPos, REFIID riid, void **ppunk);

    // public non interface members
    void   _AddExtraDataSection(DATABLOCK_HEADER *pdbh);
    void   _RemoveExtraDataSection(DWORD dwSig);
    void * _ReadExtraDataSection(DWORD dwSig);

    static HRESULT ResolveCallback(IShellLink *psl, HWND hwnd, DWORD fFlags, DWORD dwTracker);

protected:

    ~CShellLink();                          //This is not ordinary C++ class

    static DWORD CALLBACK _InvokeThreadProc(void *pv);
    static DWORD CALLBACK _VerifyPathThreadProc(void *pv);

    void _ResetPersistData();
    BOOL _ResolveRelative(LPTSTR pszPath);
    BOOL SetPIDLPath(LPCITEMIDLIST pidlNew, LPCTSTR pszPath, const WIN32_FIND_DATA *pfdNew);
    void UpdateWorkingDir(LPCITEMIDLIST pidlNew);
    PLINKINFO GetLinkInfo();
    BOOL SetFindData(const WIN32_FIND_DATA *pfd);
    BOOL IsEqualFindData(const WIN32_FIND_DATA *pfd);
    void GetFindData(WIN32_FIND_DATA *pfd);
    BOOL QueryAndSetFindData(const TCHAR *pszPath);
    BOOL IsEqualFileInfo(const BY_HANDLE_FILE_INFORMATION *pFileInfo);
    HRESULT ResolveLink(HWND hwnd, DWORD fFlags, DWORD dwTracker);
    HRESULT _SelfResolve(HWND hwnd, DWORD fFlags);
    HRESULT CheckLogo3Link(HWND hwnd, DWORD fFlags);
    void UpdateDirPIDL(LPTSTR pszPath);
    BOOL UpdateAndResolveLinkInfo(HWND hwnd, DWORD dwFlags);
    HRESULT CheckForLinkBlessing(LPCTSTR *ppszPathIn);
    HRESULT BlessLink(LPCTSTR *ppszPath, DWORD dwSignature);
    void _DecodeSpecialFolder();
    HRESULT _SetRelativePath(LPCTSTR pszRelSource);
    BOOL _EncodeSpecialFolder();
    HRESULT _UpdateTrackerData();
    HRESULT _LoadFromFile(LPCTSTR pszPath);
    HRESULT _LoadFromPIF(LPCTSTR szPath);
    HRESULT _SaveToFile(LPTSTR pszPathSave, BOOL fRemember);
    HRESULT _SaveAsLink(LPCTSTR szPath);
    HRESULT _SaveAsPIF(LPCTSTR pszPath, BOOL fPath);
    BOOL _GetWorkingDir(LPTSTR pszDir);
    HRESULT _GetUIObject(HWND hwnd, REFIID riid, void **ppvOut);
    HRESULT _ShortNetTimeout();

    HRESULT _CreateDarwinContextMenu(HWND hwnd,IContextMenu **pcmOut);
    HRESULT InvokeCommandAsync(LPCMINVOKECOMMANDINFO pici);
    HRESULT _InitDropTarget();
    HRESULT _GetExtractIcon(REFIID riid, void **ppvOut);
    HRESULT _InitExtractIcon();
    BOOL _GetExpPath(LPTSTR psz, DWORD cch);
    HRESULT _SetField(LPTSTR *ppszField, LPCWSTR pszValueW);
    HRESULT _SetField(LPTSTR *ppszField, LPCSTR  pszValueA);
    HRESULT _GetField(LPCTSTR pszField, LPWSTR pszValueW, int cchValue);
    HRESULT _GetField(LPCTSTR pszField, LPSTR  pszValueA, int cchValue);

    // Data Members
    LONG                _cRef;              // Ref Count
    BOOL                _bDirty;            // something has changed
    LPTSTR              _pszCurFile;        // current file from IPersistFile
    LPTSTR              _pszRelSource;      // overrides pszCurFile in relative tracking

    IContextMenu        *_pcmTarget;        // stuff for IContextMenu
    UINT                _indexMenuSave;
    UINT                _idCmdFirstSave;
    UINT                _idCmdLastSave;
    UINT                _uFlagsSave;

    // IDropTarget specific
    IDropTarget*        _pdtSrc;        // IDropTarget of link source (unresolved)
    DWORD               _grfKeyStateLast;

    IExtractIconW       *_pxi;          // for IExtractIcon support
    IExtractIconA       *_pxiA;
    UINT                _gilFlags;      // ::GetIconLocation() flags

    // persistant data

    LPITEMIDLIST        _pidl;          // may be NULL
    PLINKINFO           _pli;           // may be NULL

    LPTSTR              _pszName;       // title on short volumes
    LPTSTR              _pszRelPath;
    LPTSTR              _pszWorkingDir;
    LPTSTR              _pszArgs;
    LPTSTR              _pszIconLocation;

    LPDBLIST            _pExtraData;    // extra data to preserve for future compatibility

#ifdef WINNT
    CTracker            *_ptracker;
#endif

    WORD                _wOldHotkey;   // to broadcast hotkey changes
    WORD                _wAllign;
    SHELL_LINK_DATA     _sld;

    // IFilter stuff
    UINT _iChunkIndex;
    UINT _iValueIndex;
};
#endif  // defined(__cplusplus)

EXTERN_C void PathGetRelative(LPTSTR pszPath, LPCTSTR pszFrom, DWORD dwAttrFrom, LPCTSTR pszRel);
EXTERN_C PLINKINFO CopyLinkInfo(PCLINKINFO pcliSrc);
EXTERN_C void CheckAndFixNullCreateTime(LPCTSTR pszFile, FILETIME *pftCreationTime, const FILETIME *pftLastWriteTime);
EXTERN_C BOOL DifferentStrings(LPCTSTR psz1, LPCTSTR psz2);

#endif //__SHLINK_H__
