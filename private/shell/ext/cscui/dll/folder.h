//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       folder.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCUI_FOLDER_H
#define _INC_CSCUI_FOLDER_H

#include <shellp.h>     // IShellDetails
#include <shlguidp.h>   // IShellFolderViewCb
#include <shlwapip.h>   // QITAB, QISearch
#include <shsemip.h>    // ILFree(), etc
#include <sfview.h>
#include <comctrlp.h>
#include "util.h"

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);


#define OLID_SIG    0x4444

#pragma pack(1)
// PIDL format for CSC cache non leaf items...
typedef struct
{
    USHORT      cb;                 // Total size of IDList.
    USHORT      uSig;               // IDList signature.
    DWORD       cbFixed;            // Fixed size of IDList.
    DWORD       dwFileAttributes;   // Win32 file attributes.
    DWORD       dwStatus;           // CSC file/folder status flags.
    DWORD       dwServerStatus;     // CSC server status flags.
    DWORD       dwPinCount;         // CSC pin count.
    DWORD       dwHintFlags;        // CSC "hint" flags.
    DWORD       dwFileSizeHigh;     // Win32 file size.
    DWORD       dwFileSizeLow;
    FILETIME    ft;                 // Last write time (from CSC).
    DWORD       cchNameOfs;         // Offset of name part from szPath[0].
    TCHAR       szPath[0];          // path<nul>name<nul>  (variable length).
} OLID;
typedef UNALIGNED OLID *LPOLID;
typedef const UNALIGNED OLID *LPCOLID;
#pragma pack()

class COfflineFilesEnum;    // forward
class COfflineFilesViewCallback;
class COfflineDetails;
class COfflineItemsData;
class COfflineItems;
class CFolderCache;



//----------------------------------------------------------------------------
// CFileTypeCache
//----------------------------------------------------------------------------

class CFileTypeCache
{
    public:
        explicit CFileTypeCache(int cBuckets);
        ~CFileTypeCache(void);

        HRESULT GetTypeName(
            LPCTSTR pszPath, 
            DWORD dwFileAttributes, 
            LPTSTR pszDest, 
            int cchDest);

    private:
        class CEntry
        {
            public:
                CEntry(LPCTSTR pszExt, LPCTSTR pszTypeName);
                ~CEntry(void);

                void SetNext(CEntry *pNext)
                    { m_pNext = pNext; }

                CEntry *Next(void) const
                    { return m_pNext; }

                int CompareExt(LPCTSTR pszExt) const
                    { return lstrcmpi(m_pszExt, pszExt); }

                LPCTSTR TypeName(void) const
                    { return m_pszTypeName; }

                bool IsValid(void) const
                    { return (NULL != m_pszExt) && (NULL != m_pszTypeName); }

            private:
                LPTSTR m_pszExt;
                LPTSTR m_pszTypeName;
                CEntry *m_pNext;       // Next in hash bucket.

                LPTSTR StrDup(LPCTSTR psz);
                //
                // Prevent copy.
                //
                CEntry(const CEntry& rhs);
                CEntry& operator = (const CEntry& rhs);
        };

        int      m_cBuckets;
        CEntry **m_prgBuckets;
        CRITICAL_SECTION m_cs;

        int Hash(LPCTSTR pszExt);
        CEntry *Lookup(LPCTSTR pszExt);
        HRESULT Add(LPCTSTR pszExt, LPCTSTR pszTypeName);

        void Lock(void)
            { EnterCriticalSection(&m_cs); }

        void Unlock(void)
            { LeaveCriticalSection(&m_cs); }

        //
        // Prevent copy.
        //
        CFileTypeCache(const CFileTypeCache& rhs);
        CFileTypeCache& operator = (const CFileTypeCache& rhs);
};



STDAPI COfflineFilesFolder_CreateInstance(REFIID riid, void **ppv);




class COfflineFilesFolder : public IPersistFolder2, 
                                   IShellFolder, 
                                   IShellIcon, 
                                   IShellIconOverlay
{
public:
    static HRESULT WINAPI CreateInstance(REFIID riid, void **ppv);
    static INT Open(void);
    static HRESULT CreateIDList(LPITEMIDLIST *ppidl);
    static HRESULT IdentifyIDList(LPCITEMIDLIST pidl);
    static HRESULT CreateLinkOnDesktop(HWND hwndParent);
    static HRESULT IsLinkOnDesktop(HWND hwndParent, LPTSTR pszPathOut, UINT cchPathOut);
    static HRESULT GetFolder(IShellFolder **ppsf);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellFolder
    STDMETHOD(ParseDisplayName)(HWND hwnd, LPBC pbc, LPOLESTR pDisplayName,
                                ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwnd, DWORD grfFlags, IEnumIDList **ppEnumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj);
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwnd, REFIID riid, void **ppvOut);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwnd, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppvOut);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHOD(SetNameOf)(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);

    // IPersist
    STDMETHOD(GetClassID)(LPCLSID pClassID);

    // IPersistFolder
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHOD(GetCurFolder)(LPITEMIDLIST *pidl);

    // IShellIcon
    STDMETHOD(GetIconOf)(LPCITEMIDLIST pidl, UINT gil, int *pnIcon);

    // IShellIconOverlay
    STDMETHOD(GetOverlayIndex)(LPCITEMIDLIST pidl, int * pIndex);
    STDMETHOD(GetOverlayIconIndex)(LPCITEMIDLIST pidl, int * pIconIndex);

    static bool ValidateIDList(LPCITEMIDLIST pidl);

private:

    friend COfflineFilesEnum;
    friend COfflineFilesViewCallback;
    friend COfflineDetails;
    friend COfflineItemsData;
    friend COfflineItems;
    friend CFolderCache;

    friend HRESULT COfflineFilesFolder_CreateInstance(REFIID riid, void **ppv);


    COfflineFilesFolder();
    ~COfflineFilesFolder();

    HRESULT _GetFileInfo(LPCOLID polid, SHFILEINFO *psfi, DWORD rgf);
    void _GetSyncStatusString(LPCOLID polid, LPTSTR pszStatus, UINT cchStatus);
    void _GetPinStatusString(LPCOLID polid, LPTSTR pszStatus, UINT cchStatus);
    void _GetServerStatusString(LPCOLID polid, LPTSTR pszStatus, UINT cchStatus);
    void _GetTypeString(LPCOLID polid, LPTSTR pszType, UINT cchType);
    void _GetAccessString(LPCOLID polid, LPTSTR pszAccess, UINT cchAccess);
    HRESULT GetAssociations(LPCOLID polid, void **ppvQueryAssociations);
    BOOL GetClassKey(LPCOLID polid, HKEY *phkeyProgID, HKEY *phkeyBaseID);
    static LPCOLID _Validate(LPCITEMIDLIST pidl);
    static HRESULT IsOurLink(LPCTSTR pszFile);
    static HRESULT _BindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut);
    static HRESULT ContextMenuCB(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LPCTSTR OLID_GetPath(LPCOLID polid, LPTSTR pszPath, UINT cchPath);
    static LPCTSTR OLID_GetFullPath(LPCOLID polid, LPTSTR pszPath, UINT cchPath);
    static LPCTSTR OLID_GetFileName(LPCOLID polid, LPTSTR pszName, UINT cchName);
    static HRESULT OLID_CreateFromUNCPath(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, DWORD dwStatus, DWORD dwPinCount, DWORD dwHintFlags, DWORD dwServerStatus, LPOLID *ppolid);
    static void    OLID_GetWin32FindData(LPCOLID polid, WIN32_FIND_DATA *pfd);
    static void    OLID_CombinePathAndName(LPOLID polid);
    static void    OLID_SplitPathAndName(LPOLID polid);
    static HRESULT OLID_Bind(LPCOLID polid, REFIID riid, void **ppv, LPITEMIDLIST *ppidlFull, LPCITEMIDLIST *ppidlItem);
    static HRESULT OLID_CreateSimpleIDList(LPCOLID polid, LPITEMIDLIST *ppidlOut);

    LONG _cRef;
    LPITEMIDLIST _pidl;
    IShellFolderViewCB *_psfvcb;
    CFileTypeCache m_FileTypeCache;
};


//
// This class represents a simple cache of CSC status bits for each 
// server in the CSC cache.  The reason we need this is sort of bogus
// but we have no control over it.  When enumerating shares in the CSC
// database, shares on the same server might not return the same online-offline
// status depending on if the share really has a connection or not.  The
// problem is that in the network redirector an entire server is either
// online or offline.  We display "server" status in the UI so we need to
// merge the status information from each share in the database so that
// we have status information for each server.  Clear as mud?  This cache
// implements that merging of information so that all a client (i.e. the
// enum code) has to do is call GetServerStatus() with a given UNC path and
// they'll get the status we should be reporting for that path's server.
//
class CServerStatusCache
{
    public:
        CServerStatusCache(void)
            : m_hdpa(NULL) { }

        ~CServerStatusCache(void);
        //
        // This is the only public API for this class.  When it's
        // called for the first time, the cache is populated.  Therefore,
        // you can create a cache object but you're not charged much
        // until you need to use it.
        //
        DWORD GetServerStatus(LPCTSTR pszUNC);

    private:
        //
        // A single entry in the cache.
        //
        class CEntry
        {
            public:
                CEntry(LPCTSTR pszServer, DWORD dwStatus);
                ~CEntry(void);

                void AddStatus(DWORD dwStatus)
                    { m_dwStatus |= dwStatus; }

                DWORD GetStatus(void) const
                    { return m_dwStatus; }

                LPCTSTR GetServer(void) const
                    { return m_pszServer; }

                bool IsValid(void) const
                    { return NULL != m_pszServer; }

            private:
                LPTSTR m_pszServer;
                DWORD  m_dwStatus;

                LPTSTR StrDup(LPCTSTR psz);
                //
                // Prevent copy.
                //
                CEntry(const CEntry& rhs);
                CEntry& operator = (const CEntry& rhs);
        };

        HDPA m_hdpa;  // The DPA for holding entries.

        bool AddShareStatus(LPCTSTR pszShare, DWORD dwShareStatus);
        CEntry *FindEntry(LPCTSTR pszShare);
        LPTSTR ServerFromUNC(LPCTSTR pszShare, LPTSTR pszServer, UINT cchServer);

        //
        // Prevent copy.
        //
        CServerStatusCache(const CServerStatusCache& rhs);
        CServerStatusCache& operator = (const CServerStatusCache& rhs);
};



class COfflineFilesEnum : public IEnumIDList
{
public:
    COfflineFilesEnum(DWORD grfFlags, COfflineFilesFolder *pfolder);
    bool IsValid(void) const;
    
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods 
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumIDList **ppenum);

protected:
    //
    // Element of the folder path stack. (_hdsaFolderPaths).
    // Includes length to reduce length calculations.
    //
    struct FolderPathInfo
    {
        DWORD cchPath;   // Chars in path including nul term.
        LPTSTR pszPath;  // Folder path string.
    };

    ~COfflineFilesEnum();

    LONG                _cRef;          // ref count
    COfflineFilesFolder *_pfolder;      // this is what we enumerate    
    UINT                _grfFlags;      // enumeration flags 
    CCscFindHandle      _hEnumShares;
    CCscFindHandle      _hEnum;
    HDSA                _hdsaFolderPathInfo; // A stack of FolderPathInfo.
    LPTSTR              _pszPath;            // Dynamic scratch buffer for paths.
    INT                 _cchPathBuf;         // Current length of _pszPath buffer. 
    DWORD               _dwServerStatus;     // dwStatus flags for current server.
    CServerStatusCache  _ServerStatusCache;
    bool                _bShowSuperHiddenFiles;
    bool                _bShowHiddenFiles;
    bool                _bUserIsAdmin;

private:
    bool PopFolderPathInfo(FolderPathInfo *pfpi);

    bool PushFolderPathInfo(const FolderPathInfo& fpi)
        { return (-1 != DSA_AppendItem(_hdsaFolderPathInfo, (LPVOID)&fpi)); }

    bool SaveFolderPath(LPCTSTR pszRoot, LPCTSTR pszFolder);

    bool Exclude(const CscFindData& cscfd);

    bool OkToEnumFolder(const CscFindData& cscfd);

    bool UserHasAccess(const CscFindData& cscfd);

    HRESULT GrowPathBuffer(INT cchRequired, INT cchExtra);

};


//----------------------------------------------------------------------------
// Delete handler
//
// This class packages up the operation of deleting a selection of files
// from the folder view.  These methods could easily be made members of
// the COfflineFilesFolder class.  I think the separation is reasonable.
//----------------------------------------------------------------------------
class CFolderDeleteHandler
{
    public:
        CFolderDeleteHandler(HWND hwndParent, IDataObject *pdtobj, IShellFolderViewCB *psfvcb);
        ~CFolderDeleteHandler(void);

        HRESULT DeleteFiles(void);

    private:
        HWND                  m_hwndParent;// Parent for any UI.
        IDataObject          *m_pdtobj;    // Data object containing IDArray.
        IShellFolderViewCB   *m_psfvcb;    // View callback for view notifications.

        static INT_PTR ConfirmDeleteFilesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static INT_PTR ConfirmDeleteModifiedFileDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        bool ConfirmDeleteFiles(HWND hwndParent);
        bool ConfirmDeleteModifiedFile(HWND hwndParent, LPCTSTR pszFile, bool *pbNoToAll, bool *pbCancel);
        bool FileModifiedOffline(LPCTSTR pszFile);
        bool OthersHaveAccess(LPCTSTR pszFile);
};



HRESULT
CreateOfflineFilesContextMenu(
    IDataObject *pdtobj,
    REFIID riid,
    void **ppv);



#endif // _INC_CSCUI_FOLDER_H
