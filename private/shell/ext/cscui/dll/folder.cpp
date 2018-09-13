//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       folder.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shguidp.h>
#include <shdguid.h>
#include <shlapip.h>
#include <shlobjp.h>
#include <shsemip.h>
#include "folder.h"
#include "resource.h"
#include "idldata.h"
#include "idlhelp.h"
#include "items.h"
#include "strings.h"
#include "msgbox.h"
#include "sharecnx.h"
#include "msg.h"
#include "security.h"

//
// This module contains several classes.  Here's a summary list.
//
// COfflineFilesFolder - Implementation for IShellFolder
//
// COfflineDetails - Implementation for IShellDetails
//
// COfflineFilesViewCallback - Implementation for IShellFolderViewCB
//
// COfflineFilesDropTarget - Implementation for IDropTarget
//
// COfflineFilesViewEnum - Implementation for IEnumSFVViews
//
// CShellObjProxy<T> - Template class that encapsulates the attainment of a 
//     shell object and item ID list for a given OLID and interface
//     type.  Also ensures proper cleanup of the interface pointer
//     and ID list.
//
// CFolderCache - A simple cache of a bound shell object pointer
//     and item ID lists for associated OLIDs.  Reduces the number
//     of binds required in the shell namespace.  A singleton instance
//     is used for all cache accesses.
//
// CFolderDeleteHandler - Centralizes folder item deletion code.
//
// CFileTypeCache - Cache of file type descriptions.  This reduces the number
//     of calls to SHGetFileInfo.
//


// 
// Columns
//
enum {
    ICOL_NAME = 0,
    ICOL_TYPE,
    ICOL_SYNCSTATUS,
    ICOL_PINSTATUS,
    ICOL_ACCESS,
    ICOL_SERVERSTATUS,
    ICOL_LOCATION,
    ICOL_SIZE,
    ICOL_DATE,
    ICOL_MAX
};

typedef struct 
{
    short int icol;       // column index
    short int ids;        // Id of string for title
    short int cchCol;     // Number of characters wide to make column
    short int iFmt;       // The format of the column;
} COL_DATA;

const COL_DATA c_cols[] = {
    {ICOL_NAME,        IDS_COL_NAME,        20, LVCFMT_LEFT},
    {ICOL_TYPE,        IDS_COL_TYPE,        20, LVCFMT_LEFT},
    {ICOL_SYNCSTATUS,  IDS_COL_SYNCSTATUS,  18, LVCFMT_LEFT},
    {ICOL_PINSTATUS,   IDS_COL_PINSTATUS,   18, LVCFMT_LEFT},
    {ICOL_ACCESS,      IDS_COL_ACCESS,      18, LVCFMT_LEFT},
    {ICOL_SERVERSTATUS,IDS_COL_SERVERSTATUS,18, LVCFMT_LEFT},
    {ICOL_LOCATION,    IDS_COL_LOCATION,    18, LVCFMT_LEFT},
    {ICOL_SIZE,        IDS_COL_SIZE,        16, LVCFMT_RIGHT},
    {ICOL_DATE,        IDS_COL_DATE,        20, LVCFMT_LEFT}
};


//
// This is a special GUID used by the folder's delete handler to obtain
// the IShellFolderViewCB pointer from the COfflineFilesFolder.
// The delete handler QI's for this "interface".  If the folder knows
// about it (only the COfflineFilesFolder will) then it returns it's
// IShellFolderViewCB pointer.  See COfflineFilesFolder::QueryInterface()
// and CFolderDeleteHandler::InvokeCommand for usage.
//
// {47862305-0417-11d3-8BED-00C04FA31A66}
static const GUID IID_OfflineFilesFolderViewCB = 
{ 0x47862305, 0x417, 0x11d3, { 0x8b, 0xed, 0x0, 0xc0, 0x4f, 0xa3, 0x1a, 0x66 } };

//
// Private message used to enable/disable redraw of the listview
// through the MessageSFVCB method of the folder view callback.
// See CFolderDeleteHandler::DeleteFiles and 
// COfflineFilesViewCallback::OnSFVMP_SetViewRedraw for usage.
//
const UINT SFVMP_SETVIEWREDRAW = 1234;
const UINT SFVMP_DELVIEWITEM   = 1235;


HRESULT StringToStrRet(LPCTSTR pString, STRRET *pstrret)
{
#ifdef UNICODE
    HRESULT hr = SHStrDup(pString, &pstrret->pOleStr);
    if (SUCCEEDED(hr))
    {
        pstrret->uType = STRRET_WSTR;
    }
    return hr;
#else
    pstrret->uType = STRRET_CSTR;
    lstrcpyn(pstrret->cStr, pString, ARRAYSIZE(pstrret->cStr));
    return S_OK;
#endif
}


//---------------------------------------------------------------------------
// Shell view details
//---------------------------------------------------------------------------
class COfflineDetails : public IShellDetails
{
public:
    COfflineDetails(COfflineFilesFolder *pFav);
    
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppv);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // IshellDetails
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);
    STDMETHOD(ColumnClick)(UINT iColumn);

protected:
    ~COfflineDetails();
    COfflineFilesFolder *_pfolder;
    LONG _cRef;
};


//---------------------------------------------------------------------------
// Folder view callback
//---------------------------------------------------------------------------
class COfflineFilesViewCallback : public IShellFolderViewCB, IObjectWithSite
{
public:
    COfflineFilesViewCallback(COfflineFilesFolder *pfolder);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellFolderViewCB
    STDMETHOD(MessageSFVCB)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *punkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppv);

private:
    LONG _cRef;
    COfflineFilesFolder *_pfolder;
    IShellFolderView    *_psfv;
    HWND m_hwnd;
    CRITICAL_SECTION m_cs;    // Serialize change notify handling.

    ~COfflineFilesViewCallback();

    DWORD GetChangeNotifyEvents(void) const
        { return (SHCNE_UPDATEITEM | SHCNE_UPDATEDIR | SHCNE_RENAMEITEM | SHCNE_DELETE); }

    HRESULT OnSFVM_WindowCreated(HWND hwnd);
    HRESULT OnSFVM_AddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA *ppagedata);
    HRESULT OnSFVM_QueryFSNotify(SHChangeNotifyEntry *pfsne);
    HRESULT OnSFVM_FSNotify(LPCITEMIDLIST *ppidl, LONG lEvent);
    HRESULT OnSFVM_GetNotify(LPITEMIDLIST *ppidl, LONG *plEvents);
    HRESULT OnSFVM_GetViews(SHELLVIEWID *pvid, IEnumSFVViews **ppev);
    HRESULT OnSFVM_AlterDropEffect(DWORD *pdwEffect, IDataObject *pdtobj);
    HRESULT OnSFVMP_SetViewRedraw(BOOL bRedraw);
    HRESULT OnSFVMP_DelViewItem(LPCTSTR pszPath);
    HRESULT UpdateDir(LPCTSTR pszDir);
    HRESULT UpdateItem(LPCTSTR pszItem);
    HRESULT UpdateItem(LPCTSTR pszPath, const WIN32_FIND_DATA& fd, DWORD dwStatus, DWORD dwPinCount, DWORD dwHintFlags);
    HRESULT RemoveItem(LPCTSTR pszPath);
    HRESULT RemoveItem(LPCOLID polid);
    HRESULT RemoveItems(LPCTSTR pszDir);
    HRESULT RenameItem(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidl);

    UINT ItemIndexFromOLID(LPCOLID polid);
    HRESULT FindOLID(LPCTSTR pszPath, LPCOLID *ppolid);

    void Lock(void)
        { EnterCriticalSection(&m_cs); }

    void Unlock(void)
        { LeaveCriticalSection(&m_cs); }
};

//---------------------------------------------------------------------------
// Drop target
//---------------------------------------------------------------------------
class COfflineFilesDropTarget : public IDropTarget
{
    public:
        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // IDropTarget
        STDMETHODIMP DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
        STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
        STDMETHODIMP DragLeave(void);
        STDMETHODIMP Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

        static HRESULT CreateInstance(HWND hwnd, REFIID riid, void **ppv);

    private:
        COfflineFilesDropTarget(HWND hwnd);
        ~COfflineFilesDropTarget();

        bool IsOurDataObject(IDataObject *pdtobj);

        LONG m_cRef;
        HWND m_hwnd;
        LPCONTEXTMENU m_pcm;
        bool m_bIsOurData;
};


//---------------------------------------------------------------------------
// View type enumerator
//---------------------------------------------------------------------------
class COfflineFilesViewEnum : public IEnumSFVViews
{
    public:
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (REFIID riid, void **ppv);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        // *** IEnumSFVViews methods ***
        STDMETHOD(Next)(ULONG celt, SFVVIEWSDATA **ppData, ULONG *pceltFetched);
        STDMETHOD(Skip)(ULONG celt);
        STDMETHOD(Reset)(void);
        STDMETHOD(Clone)(IEnumSFVViews **ppenum);

        static HRESULT CreateInstance(IEnumSFVViews **ppEnum);
        
    protected:
        COfflineFilesViewEnum(void);
        ~COfflineFilesViewEnum(void);

        LONG           m_cRef;
        int            m_iAddView;
};


//-------------------------------------------------------------------------
// Shell object proxy
//
// A simple template class to package up the attainment of a 
// shell object pointer and item PIDL from our folder cache for a given OLID.  
// The caller can then easily call the appropriate shell object function
// through operator ->().  The object automates the release of the shell 
// object interface and freeing of the IDList.  Caller must call Result()
// to verify validity of contents prior to invoking operator ->().
//
// Usage:
//
//      CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, polid);
//      if (SUCCEEDED(hr = pxy.Result()))
//      {
//          hr = pxy->GetIconOf(pxy.ItemIDList(), gil, pnIcon);
//      }
//
//-------------------------------------------------------------------------
template <class T>
class CShellObjProxy
{
    public:
        CShellObjProxy(REFIID riid, LPCOLID polid)
            : m_hr(E_INVALIDARG),
              m_pObj(NULL),
              m_pidlFull(NULL),
              m_pidlItem(NULL)
            {
                if (NULL != polid)
                {
                    m_hr = CFolderCache::Singleton().GetItem(polid,
                                                             riid,
                                                             (void **)&m_pObj,
                                                             &m_pidlFull,
                                                             &m_pidlItem);
                }
            }

        ~CShellObjProxy(void)
            { 
                if (NULL != m_pidlFull)
                    ILFree(m_pidlFull);
                if (NULL != m_pObj)
                    m_pObj->Release();
            }

        HRESULT Result(void) const
            { return m_hr; }

        T* operator -> () const
            { return m_pObj; }

        LPCITEMIDLIST ItemIDList(void) const
            { return m_pidlItem; }

    private:
        HRESULT       m_hr;
        T            *m_pObj;
        LPITEMIDLIST  m_pidlFull;
        LPCITEMIDLIST m_pidlItem;
};



//-----------------------------------------------------------------------------------
// Folder cache
//
// The OfflineFiles folder IDList format (OLID) contains fully-qualified UNC paths.
// The folder may (most likely) contain OLIDs from multiple network shares.
// Therefore, when creating IDLists to hand off to the shell's filesystem 
// implementations we create fully-qualified IDLists (an expensive operation).
// The folder cache is used to cache these IDLists to reduce the number of calls 
// to SHBindToParent.  This also speeds up filling of the listview as 
// GetAttributesOf(), GetIconIndex() etc. are called many times as the view
// is opened.
//
// The implementation is a simple circular queue to handle aging of items.
// Only three public methods are exposed.  GetItem() is used to 
// retrieve the IShellFolder ptr and IDList associated with a particular OLID.
// If the item is not in the cache, the implementation obtains the shell folder
// ptr and IDList then caches them for later use.  Clear() is used to clear the 
// contents of the cache to reduce memory footprint when the Offline Files
// folder is no longer open. 
//
// The entries in the queue utilize a handle-envelope idiom to hide memory
// management of the entries from the caching code.  This way we can assign
// handle values without copying the actual use-counted entry.  Once a use-count
// drops to 0 the actual entry is deleted.
//
// A singleton instance is enforced through a private ctor.  Use the
// Singleton() method to obtain a reference to the singleton.
//
// Note that because of the shell's icon thread this cache must be thread-safe.
// A critical section is used for this.
//-----------------------------------------------------------------------------------
class CFolderCache 
{
    public:
        ~CFolderCache(void);

        //
        // Retrieve one item from the cache.  Item is added if not in cache.
        //
        HRESULT GetItem(
            LPCOLID polid, 
            REFIID riid, 
            void **ppv, 
            LPITEMIDLIST *ppidl, 
            LPCITEMIDLIST *ppidlChild);

        //
        // Clear the cache entry data.
        //
        void Clear(void);
        //
        // Return reference to the singleton instance.
        //
        static CFolderCache& Singleton(void);

    private:
        //
        // Enforce singleton existence.
        //
        CFolderCache(void);
        //
        // Prevent copy.
        //
        CFolderCache(const CFolderCache& rhs);
        CFolderCache& operator = (const CFolderCache& rhs);

        LPOLID           m_polid; // Key olid.
        IShellFolder    *m_psf;   // Cached IShellFolder ptr.
        LPITEMIDLIST     m_pidl;  // Cached shell pidl.
        CRITICAL_SECTION m_cs;    // For synchronizing cache access.

        void Lock(void)
            { EnterCriticalSection(&m_cs); }

        void Unlock(void)
            { LeaveCriticalSection(&m_cs); }
};


//----------------------------------------------------------------------------
// COfflineDetails
//----------------------------------------------------------------------------

STDMETHODIMP 
COfflineDetails::GetDetailsOf(
    LPCITEMIDLIST pidl, 
    UINT iColumn, 
    LPSHELLDETAILS pDetails
    )
{
    TCHAR szTemp[MAX_PATH];
    HRESULT hres;

    if (!pidl)
    {
        if (iColumn < ICOL_MAX)
        {
            pDetails->fmt    = c_cols[iColumn].iFmt;
            pDetails->cxChar = c_cols[iColumn].cchCol;

            LoadString(g_hInstance, c_cols[iColumn].ids, szTemp, ARRAYSIZE(szTemp));
            hres = StringToStrRet(szTemp, &pDetails->str);
        }
        else
        {
            pDetails->str.uType = STRRET_CSTR;
            pDetails->str.cStr[0] = 0;
            hres = E_NOTIMPL;
        }
    }
    else
    {
        LPCOLID polid = _pfolder->_Validate(pidl);
        if (polid)
        {
            hres = S_OK;

            // Need to fill in the details
            switch (iColumn)
            {
            case ICOL_TYPE:
                _pfolder->_GetTypeString(polid, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_SYNCSTATUS:
                _pfolder->_GetSyncStatusString(polid, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_PINSTATUS:
                _pfolder->_GetPinStatusString(polid, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_ACCESS:
                _pfolder->_GetAccessString(polid, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_SERVERSTATUS:
                _pfolder->_GetServerStatusString(polid, szTemp, ARRAYSIZE(szTemp));
                break;

            case ICOL_LOCATION:
                lstrcpyn(szTemp, polid->szPath, ARRAYSIZE(szTemp));
                break;

            case ICOL_SIZE:
            {
                ULARGE_INTEGER ullSize = {polid->dwFileSizeLow, polid->dwFileSizeHigh};
                StrFormatKBSize(ullSize.QuadPart, szTemp, ARRAYSIZE(szTemp));
                break;
            }

            case ICOL_DATE:
                SHFormatDateTime(&polid->ft, NULL, szTemp, ARRAYSIZE(szTemp));
                break;

            default:
                hres = E_FAIL;
            }

            if (SUCCEEDED(hres))
                hres = StringToStrRet(szTemp, &pDetails->str);
        }
        else
            hres = E_INVALIDARG;
    }
    return hres;
}



STDMETHODIMP 
COfflineDetails::ColumnClick(
    UINT iColumn
    )
{
    return S_FALSE;     // bounce this to the IShellFolderViewCB handler
}


//----------------------------------------------------------------------------
// CFolderCache
//----------------------------------------------------------------------------
//
// This is a very simple cache of one entry.
// Originally I implemented a multi-item cache.  This of course had more
// overhead than a single-item cache.  The problem is that the access patterns
// in the folder are such that cache hits occur consecutively for the
// same item as the view is filling.  Rarely (or never) was there a hit
// for an item other than the most-recently-added item.  Therefore, items
// 1 through n-1 were just taking up space.  This is why I went back to using
// a single-item cache. [brianau - 5/27/99]
//

//
// Returns a reference to the global shell folder cache.
// Since the folder cache object is a function static it will not be created
// until this function is first called.  That also means it will not be
// destroyed until the module is unloaded.  That's why we have the Clear() method.
// The FolderViewCallback dtor clears the cache so that we don't have cached
// info laying around in memory while the Offline Files folder isn't open.
// The cache skeleton is very cheap so that's not a problem to leave in memory.
//
CFolderCache& 
CFolderCache::Singleton(
    void
    )
{
    static CFolderCache TheFolderCache;
    return TheFolderCache;
}


CFolderCache::CFolderCache(
    void
    ) : m_polid(NULL),
        m_pidl(NULL),
        m_psf(NULL)
{
    InitializeCriticalSection(&m_cs); 
}



CFolderCache::~CFolderCache(
    void
    )
{ 
    Clear();
    DeleteCriticalSection(&m_cs); 
}

//
// Clear the cache by deleting the queue array and
// resetting the head/tail indexes.  A subsequent call to
// GetItem() will re-initialize the queue.
//
void 
CFolderCache::Clear(
    void
    )
{
    Lock();

    if (m_polid)
    {
        ILFree((LPITEMIDLIST)m_polid);
        m_polid = NULL;
    }
    if (m_pidl)
    {
        ILFree(m_pidl);
        m_pidl = NULL;
    }
    if (m_psf)
    {
        m_psf->Release();
        m_psf = NULL;
    }

    Unlock();
}


//
// Retrieve an item from the cache. If not found, bind to
// and cache a new one.
//
HRESULT
CFolderCache::GetItem(
    LPCOLID polid, 
    REFIID riid,
    void **ppv,
    LPITEMIDLIST *ppidlParent, 
    LPCITEMIDLIST *ppidlChild
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != ppv);
    TraceAssert(NULL != ppidlParent);
    TraceAssert(NULL != ppidlChild);

    HRESULT hr = NOERROR;

    *ppidlParent = NULL;
    *ppidlChild = NULL;
    *ppv = NULL;

    Lock();

    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    LPITEMIDLIST pidl;
    if (NULL == m_polid || !ILIsEqual((LPCITEMIDLIST)m_polid, (LPCITEMIDLIST)polid))
    {
        //
        // Cache miss.
        //
        Clear();
        hr = COfflineFilesFolder::OLID_Bind(polid, 
                                            IID_IShellFolder, 
                                            (void **)&psf, 
                                            (LPITEMIDLIST *)&pidl, 
                                            &pidlChild);
        if (SUCCEEDED(hr))
        {
            //
            // Cache the new item.
            //
            m_polid = (LPOLID)ILClone((LPCITEMIDLIST)polid);
            if (NULL != m_polid)
            {
                m_pidl  = pidl;      // Take ownership of pidl from Bind
                m_psf   = psf;       // Use ref count from Bind.
            }
            else
            {
                ILFree(pidl);
                m_psf->Release();
                hr = E_OUTOFMEMORY;
            }
        }
    }
        
    if (SUCCEEDED(hr))
    {
        //
        // Cache hit or we just bound and cached a new item.
        //
        *ppidlParent = ILClone(m_pidl);
        if (NULL != *ppidlParent)
        {
            *ppidlChild  = ILFindLastID(*ppidlParent);
            hr = m_psf->QueryInterface(riid, ppv);
        }
    }
    Unlock();
    return hr;
}


//----------------------------------------------------------------------------
// CFolderDeleteHandler
//----------------------------------------------------------------------------

CFolderDeleteHandler::CFolderDeleteHandler(
    HWND hwndParent,
    IDataObject *pdtobj,
    IShellFolderViewCB *psfvcb
    ) : m_hwndParent(hwndParent),
        m_pdtobj(pdtobj),
        m_psfvcb(psfvcb)
{
    TraceAssert(NULL != pdtobj);

    if (NULL != m_pdtobj)
        m_pdtobj->AddRef();

    if (NULL != m_psfvcb)
        m_psfvcb->AddRef();
}


CFolderDeleteHandler::~CFolderDeleteHandler(
    void
    )
{
    if (NULL != m_pdtobj)
        m_pdtobj->Release();

    if (NULL != m_psfvcb)
        m_psfvcb->Release();
}


//
// This function deletes files from the cache while also displaying
// standard shell progress UI.
//
HRESULT
CFolderDeleteHandler::DeleteFiles(
    void
    )
{
    HRESULT hr = E_FAIL;

    if (!ConfirmDeleteFiles(m_hwndParent))
        return S_FALSE;

    if (NULL != m_pdtobj)
    {
        //
        // Retrieve the selection as an HDROP.
        //
        FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM medium;

        hr = m_pdtobj->GetData(&fe, &medium);
        if (SUCCEEDED(hr))
        {
            LPDROPFILES pDropFiles = (LPDROPFILES)GlobalLock(medium.hGlobal);
            if (NULL != pDropFiles)
            {
                //
                // Create the progress dialog.
                //
                IProgressDialog *ppd;
                if (SUCCEEDED(CoCreateInstance(CLSID_ProgressDialog, 
                                               NULL, 
                                               CLSCTX_INPROC_SERVER, 
                                               IID_IProgressDialog, 
                                               (void **)&ppd)))
                {
                    //
                    // Init and start the progress dialog.
                    //
                    TCHAR szCaption[80];
                    TCHAR szLine1[80];
                    LPTSTR pszFileList = (LPTSTR)((LPBYTE)pDropFiles + pDropFiles->pFiles);
                    LPTSTR pszFile     = pszFileList;
                    int cFiles = 0;
                    int iFile  = 0;
                    bool bCancelled = false;
                    bool bNoToAll   = false;

                    //
                    // Count the number of files in the list.
                    //
                    while(TEXT('\0') != *pszFile && !bCancelled)
                    {
                        //
                        // Need to guard against deleting files that have offline
                        // changes but haven't been synchronized.  User may want
                        // to delete these but we give them lot's of warning.
                        //
                        if (FileModifiedOffline(pszFile) &&
                           (bNoToAll || !ConfirmDeleteModifiedFile(m_hwndParent, 
                                                                   pszFile, 
                                                                   &bNoToAll, 
                                                                   &bCancelled)))
                        {
                            //
                            // "Remove" this file from the list by replacing the
                            // first char with a '*'.  We'll use this as an indicator
                            // when scanning through the file list during the deletion
                            // phase below.
                            //
                            *pszFile = TEXT('*');
                            cFiles--;
                        }
                        while(*pszFile)
                            pszFile++;
                        pszFile++;
                        cFiles++;
                    }

                    if (!bCancelled)
                    {
                        LoadString(g_hInstance, IDS_APPLICATION, szCaption, ARRAYSIZE(szCaption));
                        LoadString(g_hInstance, IDS_DELFILEPROG_LINE1, szLine1, ARRAYSIZE(szLine1));
                        ppd->SetTitle(szCaption);
                        ppd->SetLine(1, szLine1, FALSE, NULL);
                        ppd->SetAnimation(g_hInstance, IDA_FILEDEL);
                        ppd->StartProgressDialog(m_hwndParent, 
                                                 NULL, 
                                                 PROGDLG_AUTOTIME | PROGDLG_MODAL, 
                                                 NULL);
                    }

                    //
                    // Process the files in the list.
                    //
                    CShareCnxStatusCache CnxStatus;

                    BOOL bUserIsAdmin = IsCurrentUserAnAdminMember();
                    //
                    // Disable redraw on the view to avoid flicker.
                    //
                    m_psfvcb->MessageSFVCB(SFVMP_SETVIEWREDRAW, 0, 0);
                    
                    pszFile = pszFileList;

                    while(TEXT('\0') != *pszFile && !bCancelled)
                    {
                        //
                        // If the file wasn't excluded from deletion above
                        // by replacing the first character with '*', delete it.
                        //
                        if (TEXT('*') != *pszFile)
                        {
                            DWORD dwErr = ERROR_ACCESS_DENIED;

                            ppd->SetLine(2, pszFile, FALSE, NULL);
                            if (bUserIsAdmin || !OthersHaveAccess(pszFile))
                            {
                                dwErr = CscDelete(pszFile);
                                if (ERROR_ACCESS_DENIED == dwErr)
                                {
                                    //
                                    // This is a little weird.  CscDelete
                                    // returns ERROR_ACCESS_DENIED if there's
                                    // a handle open on the file. Set the
                                    // code to ERROR_BUSY so we know to handle 
                                    // this as a special case below.
                                    //
                                    dwErr = ERROR_BUSY;
                                }
                            }
                            if (ERROR_SUCCESS == dwErr)
                            {
                                //
                                // File was deleted.
                                //
                                if (S_OK == CnxStatus.IsOpenConnectionPathUNC(pszFile))
                                {
                                    //
                                    // Post a shell chg "update" notify if there's
                                    // an open connection to the path.  Deleting
                                    // something from the cache will remove the 
                                    // "pinned" icon overlay in shell filesystem folders.
                                    //
                                    ShellChangeNotify(pszFile, NULL, iFile == cFiles, SHCNE_UPDATEITEM);
                                }
                                m_psfvcb->MessageSFVCB(SFVMP_DELVIEWITEM, 0, (LPARAM)pszFile);
                            }
                            else
                            {
                                //
                                // Error deleting file.
                                //
                                HWND hwndProgress = NULL;
                                IOleWindow *pow;
                                INT iUserResponse = IDOK;
                                //
                                // Get the progress dialog's window handle.  We'll use
                                // it as a parent window for error UI.
                                //
                                hr = ppd->QueryInterface(IID_IOleWindow, (void **)&pow);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pow->GetWindow(&hwndProgress);
                                    pow->Release();
                                }
                                if (ERROR_BUSY == dwErr)
                                {
                                    //
                                    // Special handling for ERROR_BUSY.
                                    //
                                    iUserResponse = CscMessageBox(hwndProgress ? hwndProgress : m_hwndParent,
                                                                  MB_OKCANCEL | MB_ICONERROR,
                                                                  g_hInstance,
                                                                  IDS_FMT_ERR_DELFROMCACHE_BUSY, 
                                                                  pszFile);
                                }
                                else
                                {
                                    //
                                    // Hit an error deleting file.  Display message and 
                                    // give user a chance to cancel operation.
                                    //
                                    iUserResponse = CscMessageBox(hwndProgress ? hwndProgress : m_hwndParent,
                                                                  MB_OKCANCEL | MB_ICONERROR,
                                                                  Win32Error(dwErr),
                                                                  g_hInstance,
                                                                  IDS_FMT_DELFILES_ERROR,
                                                                  pszFile);
                                }                                                              
                                bCancelled = bCancelled || IDCANCEL == iUserResponse;
                            }
                            ppd->SetProgress(iFile++, cFiles);
                            bCancelled = bCancelled || ppd->HasUserCancelled();
                        }

                        while(*pszFile)
                            pszFile++;
                        pszFile++;
                    }
                    //
                    // Clean up the progress dialog.
                    //
                    ppd->StopProgressDialog();
                    ppd->Release();
                    m_psfvcb->MessageSFVCB(SFVMP_SETVIEWREDRAW, 0, 1);
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}



//
// Inform user they're deleting only the offline copy of the
// selected file(s) and that the file(s) will no longer be
// available offline once they're deleted.  The dialog also
// provides a "don't bug me again" checkbox.  This setting
// is saved per-user in the registry.
//
// Returns:
//
//      true  = User pressed [OK] or had checked "don't show me
//              this again" at some time in the past.
//      false = User cancelled operation.
//
bool
CFolderDeleteHandler::ConfirmDeleteFiles(
    HWND hwndParent
    )
{
    //
    // See if user has already seen this dialog and checked 
    // the "don't bug me again" checkbox".
    //
    DWORD dwType  = REG_DWORD;
    DWORD cbData  = sizeof(DWORD);
    DWORD bNoShow = 0;
    SHGetValue(HKEY_CURRENT_USER,
               c_szCSCKey,
               c_szConfirmDelShown,
               &dwType,
               &bNoShow,
               &cbData);

    return bNoShow || IDOK == DialogBox(g_hInstance,
                                        MAKEINTRESOURCE(IDD_CONFIRM_DELETE),
                                        hwndParent,
                                        ConfirmDeleteFilesDlgProc);
}


INT_PTR
CFolderDeleteHandler::ConfirmDeleteFilesDlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
        {
            UINT idCmd = LOWORD(wParam);
            switch(LOWORD(idCmd))
            {
                case IDOK:
                {
                    //
                    // Save the "Don't bug me" value if the checkbox is 
                    // checked.  If it's not checked, no need to take up
                    // reg space with a 0 value.
                    //
                    if (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CBX_CONFIRMDEL_NOSHOW))
                    {
                        DWORD dwNoShow = 1;
                        SHSetValue(HKEY_CURRENT_USER,
                                   c_szCSCKey,
                                   c_szConfirmDelShown,
                                   REG_DWORD,
                                   &dwNoShow,
                                   sizeof(dwNoShow));
                    }
                    EndDialog(hwnd, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;

                default:
                    break;
            }
        }
        break;
    }
    return FALSE;
}


//
// Inform user that the file they're about to delete has been 
// modified offline and the changes haven't been synchronized.
// Ask if they still want to delete it.
// The choices are Yes, No, NoToAll, Cancel.
//
//
//  Arguments:
//
//      hwndParent - Dialog parent.
//
//      pszFile   - Address of filename string to embed in 
//                  dialog text.  Passed to dialog proc in the
//                  LPARAM of DialogBoxParam.
//
//      pbNoToAll - On return, indicates if the user pressed
//                  the "No To All" button.
//
//      pbCancel  - On return, indicates if the user pressed
//                  the "Cancel" button.
//  Returns:
//
//      true   = Delete it.
//      false  = Don't delete it.
//
bool
CFolderDeleteHandler::ConfirmDeleteModifiedFile(
    HWND hwndParent,
    LPCTSTR pszFile,
    bool *pbNoToAll,
    bool *pbCancel
    )
{
    TraceAssert(NULL != pszFile);
    TraceAssert(NULL != pbNoToAll);
    TraceAssert(NULL != pbCancel);

    INT_PTR iResult = DialogBoxParam(g_hInstance,
                                     MAKEINTRESOURCE(IDD_CONFIRM_DELETEMOD),
                                     hwndParent,
                                     ConfirmDeleteModifiedFileDlgProc,
                                     (LPARAM)pszFile);
    bool bResult = false;
    *pbNoToAll   = false;
    *pbCancel    = false;
    switch(iResult)
    {
        case IDYES:
            bResult = true;
            break;

        case IDCANCEL:
            *pbCancel = true;
            break;

        case IDIGNORE:
            *pbNoToAll = true;
            break;

        case IDNO:
        default:
            break;
    }
    return bResult;
}


INT_PTR
CFolderDeleteHandler::ConfirmDeleteModifiedFileDlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            //
            // lParam is the address of the filename string to be
            // embedded in the dialog text.  If the path is too long
            // to fit in the text control, shorten it with an embedded
            // ellipsis.
            //
            LPTSTR pszPath = NULL;
            if (LocalAllocString(&pszPath, (LPCTSTR)lParam))
            {
                LPTSTR pszText = NULL;
                RECT rc;

                GetWindowRect(GetDlgItem(hwnd, IDC_TXT_CONFIRM_DELETEMOD), &rc);
                PathCompactPath(NULL, pszPath, rc.right - rc.left);

                FormatStringID(&pszText,
                               g_hInstance,
                               IDS_CONFIRM_DELETEMOD,
                               pszPath);

                if (NULL != pszText)
                {
                    SetWindowText(GetDlgItem(hwnd, IDC_TXT_CONFIRM_DELETEMOD), pszText);
                    LocalFree(pszText);
                }
                LocalFreeString(&pszPath);
            }
            return TRUE;
        }

        case WM_COMMAND:
            EndDialog(hwnd, LOWORD(wParam));
            break;

        default:
            break;
    }
    return FALSE;
}


//
// Determine if a particular file has been modified offline.
//
bool
CFolderDeleteHandler::FileModifiedOffline(
    LPCTSTR pszFile
    )
{
    TraceAssert(NULL != pszFile);

    DWORD dwStatus = 0;
    CSCQueryFileStatus(pszFile, &dwStatus, NULL, NULL);
    return 0 != (FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY & dwStatus);
}


//
// Determine if a particular file can be access by another
// user other than guest.
//
bool
CFolderDeleteHandler::OthersHaveAccess(
    LPCTSTR pszFile
    )
{
    TraceAssert(NULL != pszFile);

    DWORD dwStatus = 0;
    CSCQueryFileStatus(pszFile, &dwStatus, NULL, NULL);

    return CscAccessOther(dwStatus);
}


//----------------------------------------------------------------------------
// COfflineFilesFolder
//----------------------------------------------------------------------------

COfflineFilesFolder::COfflineFilesFolder(
    void
    ) : _cRef(1),
        _psfvcb(NULL),        // Non ref-counted interface ptr.
        m_FileTypeCache(101)  // Bucket count should be prime.
{ 
    DllAddRef(); 
    _pidl = NULL;
}

COfflineFilesFolder::~COfflineFilesFolder(
    void
    )
{ 
    if (_pidl)
        ILFree(_pidl);
    DllRelease(); 
}

// class factory constructor

STDAPI 
COfflineFilesFolder_CreateInstance(
    REFIID riid, 
    void **ppv
    )
{
    HRESULT hr;

    COfflineFilesFolder* polff = new COfflineFilesFolder();
    if (polff)
    {
        hr = polff->QueryInterface(riid, ppv);
        polff->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}



LPCOLID 
COfflineFilesFolder::_Validate(
    LPCITEMIDLIST pidl
    )
{
    LPCOLID polid = (LPCOLID)pidl;
    if (polid && (polid->cbFixed == sizeof(*polid)) && (polid->uSig == OLID_SIG))
        return polid;
    return NULL;
}


//
// External version of _Validate but returns only T/F.
//
bool 
COfflineFilesFolder::ValidateIDList(
    LPCITEMIDLIST pidl
    )
{
    return NULL != _Validate(pidl);
}


STDMETHODIMP 
COfflineFilesFolder::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineFilesFolder, IShellFolder),
        QITABENT(COfflineFilesFolder, IPersistFolder2),
        QITABENTMULTI(COfflineFilesFolder, IPersistFolder, IPersistFolder2),
        QITABENTMULTI(COfflineFilesFolder, IPersist, IPersistFolder2),
        QITABENT(COfflineFilesFolder, IShellIcon),
        QITABENT(COfflineFilesFolder, IShellIconOverlay),
        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr))
    {
        //
        // OK, this is a little slimy.  The "delete handler" needs to
        // get at the folder's IShellFolderViewCB interface so it can
        // update the view following a deletion (remember, we're only deleting
        // from the cache so no meaningful FS notification will occur).  
        // We define this secret IID that only our folder knows about.  This 
        // way the delete handler can safely QI any IShellFolder interface
        // and only our folder will respond with a view CB pointer.
        // [brianau - 5/5/99]
        //
        if (riid == IID_OfflineFilesFolderViewCB && NULL != _psfvcb)
        {
            _psfvcb->AddRef();
            *ppv = (void **)_psfvcb;
            hr = NOERROR;
        }
    }
    return hr;
}


STDMETHODIMP_ (ULONG) 
COfflineFilesFolder::AddRef(
    void
    )
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_ (ULONG) 
    COfflineFilesFolder::Release(
    void
    )
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IPersist methods
STDMETHODIMP 
COfflineFilesFolder::GetClassID(
    CLSID *pclsid
    )
{
    *pclsid = CLSID_OfflineFilesFolder;
    return S_OK;
}


HRESULT 
COfflineFilesFolder::Initialize(
    LPCITEMIDLIST pidl
    )
{
    if (_pidl)
        ILFree(_pidl);

    _pidl = ILClone(pidl);

    return _pidl ? S_OK : E_OUTOFMEMORY;
}


HRESULT 
COfflineFilesFolder::GetCurFolder(
    LPITEMIDLIST *ppidl
    )
{
    if (_pidl)
    {
        *ppidl = ILClone(_pidl);
        return *ppidl ? NOERROR : E_OUTOFMEMORY;
    }

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}


STDMETHODIMP 
COfflineFilesFolder::ParseDisplayName(
    HWND hwnd, 
    LPBC pbc,
    LPOLESTR pDisplayName, 
    ULONG* pchEaten,
    LPITEMIDLIST* ppidl, 
    ULONG *pdwAttributes
    )
{
    return E_NOTIMPL;
}


STDMETHODIMP 
COfflineFilesFolder::EnumObjects(
    HWND hwnd, 
    DWORD grfFlags, 
    IEnumIDList **ppenum
    )
{
    *ppenum = NULL;

    HRESULT hr = E_FAIL;
    COfflineFilesEnum *penum = new COfflineFilesEnum(grfFlags, this);
    if (penum)
    {
        if (penum->IsValid())
            hr = penum->QueryInterface(IID_IEnumIDList, (void **)ppenum);
        penum->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP 
COfflineFilesFolder::BindToObject(
    LPCITEMIDLIST pidl, 
    LPBC pbc, 
    REFIID riid, 
    void **ppv
    )
{
    return E_NOTIMPL;
}


STDMETHODIMP 
COfflineFilesFolder::BindToStorage(
    LPCITEMIDLIST pidl, 
    LPBC pbc, 
    REFIID riid, 
    void **ppv
    )
{
    return E_NOTIMPL;
}


void 
COfflineFilesFolder::_GetSyncStatusString(
    LPCOLID polid, 
    LPTSTR pszStatus, 
    UINT cchStatus
    )
{
    //
    // Translate a file status into a stale state code.
    // Note that the stale state codes are the same values as their
    // corresponding string resource IDs.  Order of this array is
    // important.  The first match is the message that is displayed.
    // In the case of multiple bits being set, we want to display a
    // message for the most "serious" reason.
    //
    static const struct
    {
        DWORD dwStatusMask;
        UINT idMsg;

    } rgStaleInfo[] = {
        { FLAG_CSC_COPY_STATUS_SUSPECT,                 IDS_STALEREASON_SUSPECT         },
        { FLAG_CSC_COPY_STATUS_ORPHAN,                  IDS_STALEREASON_ORPHAN          },
        { FLAG_CSC_COPY_STATUS_STALE,                   IDS_STALEREASON_STALE           },
        { FLAG_CSC_COPY_STATUS_LOCALLY_CREATED,         IDS_STALEREASON_LOCALLY_CREATED },
        { FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED,   IDS_STALEREASON_LOCALLY_MODDATA },
        { FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED,   IDS_STALEREASON_LOCALLY_MODTIME },
        { FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED, IDS_STALEREASON_LOCALLY_MODATTR },
        { FLAG_CSC_COPY_STATUS_SPARSE,                  IDS_STALEREASON_SPARSE          }
                      };

    int idStatusText = IDS_STALEREASON_NOTSTALE; // Default is "not stale".

    for (int i = 0; i < ARRAYSIZE(rgStaleInfo); i++)
    {
        if (0 != (rgStaleInfo[i].dwStatusMask & polid->dwStatus))
        {
            idStatusText = rgStaleInfo[i].idMsg;
            break;
        }
    }
    LoadString(g_hInstance, idStatusText, pszStatus, cchStatus);
}


void 
COfflineFilesFolder::_GetPinStatusString(
    LPCOLID polid, 
    LPTSTR pszStatus, 
    UINT cchStatus
    )
{
    LoadString(g_hInstance, 
               (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_ADMIN) & polid->dwHintFlags ? IDS_FILE_PINNED : IDS_FILE_NOTPINNED,
               pszStatus, 
               cchStatus);
}


void 
COfflineFilesFolder::_GetServerStatusString(
    LPCOLID polid, 
    LPTSTR pszStatus, 
    UINT cchStatus
    )
{
    //
    // Only two possible status strings: "Online" and "Offline".
    //
    UINT idText = IDS_SHARE_STATUS_ONLINE;
    if (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & polid->dwServerStatus)
        idText = IDS_SHARE_STATUS_OFFLINE;

    LoadString(g_hInstance, idText, pszStatus, cchStatus);
}


void 
COfflineFilesFolder::_GetTypeString(
    LPCOLID polid, 
    LPTSTR pszType, 
    UINT cchType
    )
{
    //
    // We utilize a local cache of type name information to reduce
    // the number of calls to SHGetFileInfo.  This speeds things up
    // tremendously.  The shell does something similar in DefView.
    // Note that the filetype cache is a member of COfflineFilesFolder
    // so that it lives only while the folder is active.  The alternative
    // would be to make a local static object here in this function.
    // The problem with that is that once created the cache would remain
    // in memory until our DLL is unloaded; which in explorer.exe is NEVER.
    //
    m_FileTypeCache.GetTypeName(polid->szPath + polid->cchNameOfs,
                                polid->dwFileAttributes,
                                pszType,
                                cchType);
}


void 
COfflineFilesFolder::_GetAccessString(
    LPCOLID polid, 
    LPTSTR pszAccess, 
    UINT cchAccess
    )
{
    //
    // Three strings containing the replacement text for the rgFmts[i] template.
    // Note that the index value corresponds directly with the access values
    // obtained from the OLID's dwStatus member.  This makes the translation from
    // OLID access info to text string very fast.  These are small enough to 
    // cache.  Caching saves us three LoadStrings each time.
    //
    //  Index    String resource        (english)
    //  -------- ---------------------- ---------
    //    0      IDS_ACCESS_READ        "R"
    //    1      IDS_ACCESS_WRITE       "W"
    //    2      IDS_ACCESS_READWRITE   "R/W"
    //
    static TCHAR rgszAccess[3][4] = {0};
    //
    // This table lists the "mask" and "shift count" used to retrieve the access
    // information from the OLID dwStatus value.
    //
    static const struct
    {
        DWORD dwMask;
        DWORD dwShift;

    } rgAccess[] = {{ FLAG_CSC_USER_ACCESS_MASK,  FLAG_CSC_USER_ACCESS_SHIFT_COUNT  },
                    { FLAG_CSC_GUEST_ACCESS_MASK, FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT },
                    { FLAG_CSC_OTHER_ACCESS_MASK, FLAG_CSC_OTHER_ACCESS_SHIFT_COUNT }};

    //
    // These IDs specify which format string to use for a given
    // item access value in the OLID's dwStatus member.
    // The index into this array is calculated below from the access bits
    // set for this OLID.  Note that these are "message" formats defined
    // in msg.mc and not resource strings.  This way we eliminate the 
    // need for a LoadString and do everything with FormatMessage.
    //
                                                                    // iFmt  (see below)
    static const UINT rgFmts[] = { 0,                               // 0x0000
                                   MSG_FMT_ACCESS_USER,             // 0x0001
                                   MSG_FMT_ACCESS_GUEST,            // 0x0002
                                   MSG_FMT_ACCESS_USERGUEST,        // 0x0003
                                   MSG_FMT_ACCESS_OTHER,            // 0x0004
                                   MSG_FMT_ACCESS_USEROTHER,        // 0x0005
                                   MSG_FMT_ACCESS_GUESTOTHER,       // 0x0006
                                   MSG_FMT_ACCESS_USERGUESTOTHER }; // 0x0007
    
    const DWORD dwAccess = polid->dwStatus & FLAG_CSC_ACCESS_MASK;
    int i;

    if (TEXT('\0') == rgszAccess[0][0])
    {
        //
        // First-time init for strings used in access text.
        // This stuff happens only once.
        //
        const UINT rgidStr[] = { IDS_ACCESS_READ,
                                 IDS_ACCESS_WRITE,
                                 IDS_ACCESS_READWRITE };
        //
        // Load up the "R", "W", "R/W" strings.
        //
        for (i = 0; i < ARRAYSIZE(rgidStr); i++)
        {
            TraceAssert(i < ARRAYSIZE(rgszAccess));
            LoadString(g_hInstance, rgidStr[i], rgszAccess[i], ARRAYSIZE(rgszAccess[i]));
        }
    }
    //
    // Build an index into rgFmts[] based on the access bits set on the olid.
    //
    int iFmt = 0;
    if (FLAG_CSC_USER_ACCESS_MASK & dwAccess)
        iFmt |= 0x0001;
    if (FLAG_CSC_GUEST_ACCESS_MASK & dwAccess)
        iFmt |= 0x0002;
    if (FLAG_CSC_OTHER_ACCESS_MASK & dwAccess)
        iFmt |= 0x0004;

    *pszAccess = TEXT('\0');
    if (0 != iFmt)
    {
        //
        // Fill in the argument array passed to FormatMessage.
        // Each of the elements will contain the address of one element in the 
        // rgszAccess[] string array.  
        //
        LPCTSTR rgpszArgs[ARRAYSIZE(rgszAccess)] = {0};
        int iArg = 0;
        for (i = 0; i < ARRAYSIZE(rgpszArgs); i++)
        {
            int a = dwAccess & rgAccess[i].dwMask;
            if (0 != a)
            {
                rgpszArgs[iArg++] = &rgszAccess[(a >> rgAccess[i].dwShift) - 1][0];
            }
        }
        //
        // Finally, format the message text.
        //
        FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      g_hInstance,
                      rgFmts[iFmt],
                      0,
                      pszAccess,
                      cchAccess,
                      (va_list *)rgpszArgs);
    }
}



STDMETHODIMP 
COfflineFilesFolder::CompareIDs(
    LPARAM lParam, 
    LPCITEMIDLIST pidl1, 
    LPCITEMIDLIST pidl2
    )
{
    HRESULT hres;
    LPCOLID polid1 = _Validate(pidl1);
    LPCOLID polid2 = _Validate(pidl2);
    if (polid1 && polid2)
    {
        TCHAR szStr1[MAX_PATH], szStr2[MAX_PATH];

        switch (lParam & SHCIDS_COLUMNMASK)
        {
        case ICOL_NAME:
            hres = ResultFromShort(lstrcmpi(polid1->szPath + polid1->cchNameOfs, 
                                            polid2->szPath + polid2->cchNameOfs));
            if (0 == hres)
            {
                //
                // Since we present a "flat" view of the CSC cache,
                // we can't compare only by name.  We have to include
                // path for items with the same name.  This is because the
                // shell uses column 0 as the unique identifying column for
                // an ID.
                //
                hres = ResultFromShort(lstrcmpi(polid1->szPath, polid2->szPath));
            }
            break;

        case ICOL_TYPE:
            _GetTypeString(polid1, szStr1, ARRAYSIZE(szStr1));
            _GetTypeString(polid2, szStr2, ARRAYSIZE(szStr2));
            hres = ResultFromShort(lstrcmpi(szStr1, szStr2));
            break;

        case ICOL_SYNCSTATUS:
            _GetSyncStatusString(polid1, szStr1, ARRAYSIZE(szStr1));
            _GetSyncStatusString(polid2, szStr2, ARRAYSIZE(szStr2));
            hres = ResultFromShort(lstrcmpi(szStr1, szStr2));
            break;

        case ICOL_PINSTATUS:
            _GetPinStatusString(polid1, szStr1, ARRAYSIZE(szStr1));
            _GetPinStatusString(polid2, szStr2, ARRAYSIZE(szStr2));
            hres = ResultFromShort(lstrcmpi(szStr1, szStr2));
            break;

        case ICOL_ACCESS:
            _GetAccessString(polid1, szStr1, ARRAYSIZE(szStr1));
            _GetAccessString(polid2, szStr2, ARRAYSIZE(szStr2));
            hres = ResultFromShort(lstrcmpi(szStr1, szStr2));
            break;

        case ICOL_SERVERSTATUS:
            _GetServerStatusString(polid1, szStr1, ARRAYSIZE(szStr1));
            _GetServerStatusString(polid2, szStr2, ARRAYSIZE(szStr2));
            hres = ResultFromShort(lstrcmpi(szStr1, szStr2));
            break;

        case ICOL_LOCATION:
            hres = ResultFromShort(lstrcmpi(polid1->szPath, polid2->szPath));
            break;

        case ICOL_SIZE:
            if (polid1->dwFileSizeLow > polid2->dwFileSizeLow)
                hres = ResultFromShort(1);
            else if (polid1->dwFileSizeLow < polid2->dwFileSizeLow)
                hres = ResultFromShort(-1);
            else
                hres = ResultFromShort(0);
            break;

        case ICOL_DATE:
            hres = ResultFromShort(CompareFileTime(&polid1->ft, &polid2->ft));
            break;
        }

        if (hres == S_OK && (lParam & SHCIDS_ALLFIELDS)) 
        {
            hres = CompareIDs(ICOL_PINSTATUS, pidl1, pidl2);
            if (hres == S_OK)
            {
                hres = CompareIDs(ICOL_SYNCSTATUS, pidl1, pidl2);
                if (hres == S_OK)
                {
                    hres = CompareIDs(ICOL_SIZE, pidl1, pidl2);
                    if (hres == S_OK)
                    {
                        hres = CompareIDs(ICOL_DATE, pidl1, pidl2);
                    }
                }
            }
        }
    }
    else
        hres = E_INVALIDARG;
    return hres;
}




STDMETHODIMP 
COfflineFilesFolder::CreateViewObject(
    HWND hwnd, 
    REFIID riid, 
    void **ppv
    )
{
    HRESULT hres;

    if (IsEqualIID(riid, IID_IShellView))
    {
        COfflineFilesViewCallback *pViewCB = new COfflineFilesViewCallback(this);
        if (pViewCB)
        {
            SFV_CREATE sSFV;
            sSFV.cbSize   = sizeof(sSFV);
            sSFV.psvOuter = NULL;
            sSFV.pshf     = this;
            sSFV.psfvcb   = pViewCB;
            hres = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);
            pViewCB->Release();

            if (SUCCEEDED(hres))
            {
                //
                // Save the view callback pointer so we can use it in our context menu
                // handler for view notifications.  Note we don't take a ref count as that
                // would create a ref count cycle.  The view will live as long as the
                // folder does.
                //
                _psfvcb = pViewCB; 
            }
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else if (IsEqualIID(riid, IID_IShellDetails))
    {
        COfflineDetails *pDetails = new COfflineDetails(this);
        if (pDetails)
        {
            *ppv = (IShellDetails *)pDetails;
            hres = S_OK;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hres = COfflineFilesDropTarget::CreateInstance(hwnd, riid, ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        IContextMenu *pcmCSCUI;
        hres = CreateOfflineFilesContextMenu(NULL, riid, (void **)ppv);
    }
    else 
    {
        *ppv = NULL;
        hres = E_NOINTERFACE;
    }
    return hres;
}



STDMETHODIMP 
COfflineFilesFolder::GetAttributesOf(
    UINT cidl, 
    LPCITEMIDLIST* apidl, 
    ULONG *rgfInOut
    )
{

    HRESULT hr             = NOERROR;
    IShellFolder *psf      = NULL;
    ULONG ulAttrRequested  = *rgfInOut;

    *rgfInOut = (ULONG)-1;

    for (UINT i = 0; i < cidl && SUCCEEDED(hr); i++)
    {
        CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, (LPCOLID)*apidl++);
        if (SUCCEEDED(hr = pxy.Result()))
        {
            ULONG ulThis           = ulAttrRequested;
            LPCITEMIDLIST pidlItem = pxy.ItemIDList();
            hr = pxy->GetAttributesOf(1, &pidlItem, &ulThis);
            if (SUCCEEDED(hr))
            {
                //
                // Build up the intersection of attributes for all items
                // in the IDList.  Note that we don't allow move.
                //
                *rgfInOut &= (ulThis & ~SFGAO_CANMOVE);
            }
        }
    }
    return hr;
}



HRESULT 
COfflineFilesFolder::GetAssociations(
    LPCOLID polid, 
    void **ppvQueryAssociations
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != ppvQueryAssociations);

    HRESULT hr = NOERROR;
    *ppvQueryAssociations = NULL;

    CCoInit coinit;
    if (SUCCEEDED(hr = coinit.Result()))
    {
        CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, polid);
        if (SUCCEEDED(hr = pxy.Result()))
        {
            LPCITEMIDLIST pidlItem = pxy.ItemIDList();
            hr = pxy->GetUIObjectOf(NULL, 1, &pidlItem, IID_IQueryAssociations, NULL, ppvQueryAssociations);

            if (FAILED(hr))
            {
                //  this means that the folder doesnt support
                //  the IQueryAssociations.  so we will
                //  just check to see if this is a folder
                ULONG rgfAttrs = SFGAO_FOLDER | SFGAO_BROWSABLE;
                IQueryAssociations *pqa;
                if (SUCCEEDED(pxy->GetAttributesOf(1, &pidlItem, &rgfAttrs))
                    && (rgfAttrs & SFGAO_FOLDER | SFGAO_BROWSABLE)
                    && (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (void **)&pqa))))
                {
                    hr = pqa->Init(0, L"Folder", NULL, NULL);

                    if (SUCCEEDED(hr))
                        *ppvQueryAssociations = (void *)pqa;
                    else
                        pqa->Release();
                }
            }
        }
    }
    return hr;
}


BOOL 
COfflineFilesFolder::GetClassKey(
    LPCOLID polid, 
    HKEY *phkeyProgID, 
    HKEY *phkeyBaseID
    )
{
    TraceAssert(NULL != polid);

    BOOL bRet = FALSE;
    IQueryAssociations *pqa;

    if (phkeyProgID)
        *phkeyProgID = NULL;

    if (phkeyBaseID)
        *phkeyBaseID = NULL;

    if (SUCCEEDED(GetAssociations(polid, (void **)&pqa)))
    {
        if (phkeyProgID)
            pqa->GetKey(ASSOCF_IGNOREBASECLASS, ASSOCKEY_CLASS, NULL, phkeyProgID);
        if (phkeyBaseID)
            pqa->GetKey(0, ASSOCKEY_BASECLASS, NULL, phkeyBaseID);
        pqa->Release();
        bRet = TRUE;
    }
    return bRet;
}



HRESULT
COfflineFilesFolder::ContextMenuCB(
    IShellFolder *psf, 
    HWND hwndOwner,
    IDataObject *pdtobj, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HRESULT hr = NOERROR;

    switch(uMsg)
    {
        case DFM_MERGECONTEXTMENU:
            //
            // Return NOERROR.
            // This causes the shell to add the default verbs
            // (i.e. Open, Print etc) to the menu.
            //
            break;
        
        case DFM_INVOKECOMMAND:
            switch(wParam)
            {
                case DFM_CMD_DELETE:
                {
                    IShellFolderViewCB *psfvcb = NULL;
                    if (SUCCEEDED(psf->QueryInterface(IID_OfflineFilesFolderViewCB, (void **)&psfvcb)))
                    {
                        CFolderDeleteHandler handler(hwndOwner, pdtobj, psfvcb);
                        handler.DeleteFiles();
                        psfvcb->Release();
                    }
                    break;
                }

                case DFM_CMD_COPY:
                    SetPreferredDropEffect(pdtobj, DROPEFFECT_COPY);
                    hr = S_FALSE;
                    break;

                case DFM_CMD_PROPERTIES:
                    SHMultiFileProperties(pdtobj, 0);
                    break;

                default:
                    hr = S_FALSE;  // Execute default code.
                    break;
            }
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }

    return hr;
}



/*
// Used for dumping out interface requests.  Uncomment if you want to use it.
//
//
LPCTSTR IIDToStr(REFIID riid, LPTSTR pszDest, UINT cchDest)
{
    struct
    {
        const IID *piid;
        LPCTSTR s;

    } rgMap[] = { { &IID_IDataObject,   TEXT("IID_IDataObject")       },
                  { &IID_IUnknown,      TEXT("IID_IUnknown")          },
                  { &IID_IContextMenu,  TEXT("IID_IContextMenu")      },
                  { &IID_IExtractIconA, TEXT("IID_IExtractIconA")     },
                  { &IID_IExtractIconW, TEXT("IID_IExtractIconW")     },
                  { &IID_IExtractImage, TEXT("IID_IExtractImage")     },
                  { &IID_IPersistFolder2, TEXT("IID_IPersistFolder2") },
                  { &IID_IQueryInfo,    TEXT("IID_IQueryInfo")        },
                  { &IID_IDropTarget,   TEXT("IID_IDropTarget")       },
                  { &IID_IQueryAssociations, TEXT("IID_IQueryAssociations") }
                };

    StringFromGUID2(riid, pszDest, cchDest);

    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        if (riid == *(rgMap[i].piid))
        {
            lstrcpyn(pszDest, rgMap[i].s, cchDest);
            break;
        }
    }
    return pszDest;
}
*/


STDMETHODIMP 
COfflineFilesFolder::GetUIObjectOf(
    HWND hwnd, 
    UINT cidl, 
    LPCITEMIDLIST *ppidl, 
    REFIID riid, 
    UINT* prgfReserved, 
    void **ppv
    )
{
    HRESULT hr;

    if (IID_IDataObject == riid)
    {
        LPITEMIDLIST pidlOfflineFiles;
        hr = COfflineFilesFolder::CreateIDList(&pidlOfflineFiles);
        if (SUCCEEDED(hr))
        {
            hr = COfflineItemsData::CreateInstance((IDataObject **)ppv, 
                                                    pidlOfflineFiles, 
                                                    cidl, 
                                                    ppidl,
                                                    hwnd);
            if (SUCCEEDED(hr))
            {
                SetPreferredDropEffect((IDataObject *)*ppv, DROPEFFECT_COPY);
            }                
            ILFree(pidlOfflineFiles);
        }
    }
    else if (riid == IID_IContextMenu)
    {
        HKEY hkeyBaseProgID = NULL;
        HKEY hkeyProgID     = NULL;
        HKEY hkeyAllFileSys = NULL;
        //
        // Get the hkeyProgID and hkeyBaseProgID from the first item.
        //
        GetClassKey((LPCOLID)*ppidl, &hkeyProgID, &hkeyBaseProgID);

        //
        // Pick up "Send To..."
        //
        RegOpenKeyEx(HKEY_CLASSES_ROOT,
                     TEXT("AllFilesystemObjects"),
                     0,
                     KEY_READ,
                     &hkeyAllFileSys);

        LPITEMIDLIST pidlOfflineFilesFolder;
        hr = COfflineFilesFolder::CreateIDList(&pidlOfflineFilesFolder);
        if (SUCCEEDED(hr))
        {
            HKEY rgClassKeys[] = { hkeyProgID, hkeyBaseProgID, hkeyAllFileSys };

            hr = CDefFolderMenu_Create2(pidlOfflineFilesFolder, 
                                        hwnd,
                                        cidl, 
                                        ppidl,
                                        this,
                                        COfflineFilesFolder::ContextMenuCB,
                                        ARRAYSIZE(rgClassKeys),
                                        rgClassKeys,
                                        (IContextMenu **)ppv);

            ILFree(pidlOfflineFilesFolder);
        }

        if (NULL != hkeyBaseProgID)
            RegCloseKey(hkeyBaseProgID);    
        if (NULL != hkeyProgID)
            RegCloseKey(hkeyProgID);
        if (NULL != hkeyAllFileSys)
            RegCloseKey(hkeyAllFileSys);
    }
    else if (1 == cidl)
    {
        CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, (LPCOLID)*ppidl);
        if (SUCCEEDED(hr = pxy.Result()))
        {
            //
            // Forward single-item selection to the filesystem implementation.
            //
            LPCITEMIDLIST pidlItem = pxy.ItemIDList();
            hr = pxy->GetUIObjectOf(hwnd, 1, &pidlItem, riid, prgfReserved, ppv);
        }
    }
    else if (0 == cidl)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppv = NULL;
        hr = E_FAIL;
    }
    return hr;
}



HRESULT 
COfflineFilesFolder::_GetFileInfo(
    LPCOLID polid, 
    SHFILEINFO *psfi, 
    DWORD rgf
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != psfi);

    return SHGetFileInfo(polid->szPath + polid->cchNameOfs, 
                         polid->dwFileAttributes, 
                         psfi, 
                         sizeof(*psfi), 
                         rgf | SHGFI_USEFILEATTRIBUTES) ? S_OK : E_FAIL;
}



STDMETHODIMP 
COfflineFilesFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl, 
    DWORD uFlags, 
    STRRET *pName
    )
{
    TraceAssert(NULL != pidl);

    HRESULT hres = E_INVALIDARG;
    LPCOLID polid = _Validate(pidl);
    if (polid)
    {
        if (uFlags & SHGDN_FORPARSING)
        {
            TCHAR szPath[MAX_PATH];
            OLID_GetFullPath(polid, szPath, ARRAYSIZE(szPath));
            hres = StringToStrRet(szPath, pName);
        }
        else
        {
            CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, polid);
            if (SUCCEEDED(hres = pxy.Result()))
            {
                hres = pxy->GetDisplayNameOf(pxy.ItemIDList(), uFlags, pName);
            }
        }
    }
    return hres;
}



STDMETHODIMP 
COfflineFilesFolder::SetNameOf(
    HWND hwnd, 
    LPCITEMIDLIST pidl, 
    LPCOLESTR pName, 
    DWORD uFlags, 
    LPITEMIDLIST *ppidlOut
    )
{
    HRESULT hr;
    CShellObjProxy<IShellFolder> pxy(IID_IShellFolder, _Validate(pidl));
    if (SUCCEEDED(hr = pxy.Result()))
    {
        hr = pxy->SetNameOf(hwnd, pxy.ItemIDList(), pName, uFlags, ppidlOut);
    }
    return hr;
}



//
// Forward IShellIcon methods to parent filesystem folder.
//
HRESULT 
COfflineFilesFolder::GetIconOf(
    LPCITEMIDLIST pidl, 
    UINT gil, 
    int *pnIcon
    )
{
    TraceAssert(NULL != pidl);

    HRESULT hr;
    CShellObjProxy<IShellIcon> pxy(IID_IShellIcon, _Validate(pidl));
    if (SUCCEEDED(hr = pxy.Result()))
    {
        hr = pxy->GetIconOf(pxy.ItemIDList(), gil, pnIcon);
    }
    return hr;
}



//
// Defer IShellIconOverlay methods to parent filesystem folder.
//
HRESULT 
COfflineFilesFolder::GetOverlayIndex(
    LPCITEMIDLIST pidl, 
    int *pIndex
    )
{
    TraceAssert(NULL != pidl);

    HRESULT hr;
    CShellObjProxy<IShellIconOverlay> pxy(IID_IShellIconOverlay, _Validate(pidl));
    if (SUCCEEDED(hr = pxy.Result()))
    {
        hr = pxy->GetOverlayIndex(pxy.ItemIDList(), pIndex);
    }
    return hr;
}


//
// Defer IShellIconOverlay methods to parent filesystem folder.
//
HRESULT
COfflineFilesFolder::GetOverlayIconIndex(
    LPCITEMIDLIST pidl, 
    int * pIconIndex
    )
{
    TraceAssert(NULL != pidl);

    HRESULT hr;
    CShellObjProxy<IShellIconOverlay> pxy(IID_IShellIconOverlay, _Validate(pidl));
    if (SUCCEEDED(hr = pxy.Result()))
    {
        hr = pxy->GetOverlayIconIndex(pxy.ItemIDList(), pIconIndex);
    }
    return hr;
}


//
// Static member function for creating and opening the offline files folder.
//
INT 
COfflineFilesFolder::Open(  // [static]
    void
    )
{
    INT iReturn = 0;
    if (CConfig::GetSingleton().NoCacheViewer())
    {
        CscMessageBox(NULL,
                      MB_OK | MB_ICONINFORMATION,
                      g_hInstance,
                      IDS_ERR_POLICY_NOVIEWCACHE);

        iReturn = -1;
    }
    else
    {
        SHELLEXECUTEINFO shei = { 0 };

        shei.cbSize     = sizeof(shei);
        shei.fMask      = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST;
        shei.nShow      = SW_SHOWNORMAL;

        if (SUCCEEDED(COfflineFilesFolder::CreateIDList((LPITEMIDLIST *)(&shei.lpIDList))))
        {
            ShellExecuteEx(&shei);
            ILFree((LPITEMIDLIST)(shei.lpIDList));
        }
    }
    return iReturn;
}



//
// Static member function for creating the folder's IDList.
//
HRESULT 
COfflineFilesFolder::CreateIDList(  // [static]
    LPITEMIDLIST *ppidl
    )
{
    TraceAssert(NULL != ppidl);
    
    IShellFolder *psf;
    HRESULT hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hr))
        {
            BIND_OPTS bo;
            memset(&bo, 0, sizeof(bo));
            bo.cbStruct = sizeof(bo);
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;
            bo.grfMode  = STGM_CREATE;
            pbc->SetBindOptions(&bo);
            
            WCHAR wszPath[80] = L"::";
            StringFromGUID2(CLSID_OfflineFilesFolder, 
                            &wszPath[2], 
                            sizeof(wszPath) - (2 * sizeof(WCHAR)));

            hr = psf->ParseDisplayName(NULL, pbc, wszPath, NULL, ppidl, NULL);
            pbc->Release();
        } 
        psf->Release();
    }
    return hr;
}



//
// Static function for creating a link to the folder on the desktop.
//
HRESULT
COfflineFilesFolder::CreateLinkOnDesktop(  // [static]
    HWND hwndParent
    )
{
    IShellLink* psl;  
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_ShellLink, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IShellLink, 
                              (void **)&psl); 

        if (SUCCEEDED(hr)) 
        {
            LPITEMIDLIST pidl = NULL;
            hr = COfflineFilesFolder::CreateIDList(&pidl);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetIDList(pidl);
                if (SUCCEEDED(hr))
                {
                    TCHAR szLinkTitle[80] = { 0 };
                    LoadString(g_hInstance, IDS_FOLDER_LINK_NAME, szLinkTitle, ARRAYSIZE(szLinkTitle));
                    psl->SetDescription(szLinkTitle);  

                    IPersistFile* ppf;  
                    hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);          
                    if (SUCCEEDED(hr)) 
                    { 
                        TCHAR szLinkPath[MAX_PATH];
                        hr = SHGetSpecialFolderPath(hwndParent, szLinkPath, CSIDL_DESKTOPDIRECTORY, FALSE);
                        if (SUCCEEDED(hr))
                        {
                            TCHAR szLinkFileName[80];
                            LoadStringW(g_hInstance, IDS_FOLDER_LINK_NAME, szLinkFileName, ARRAYSIZE(szLinkFileName));
                            lstrcat(szLinkFileName, TEXT(".LNK"));
                            PathAppend(szLinkPath, szLinkFileName);
                            hr = ppf->Save(szLinkPath, TRUE); 
                        }
                        ppf->Release();         
                    } 
                }
                ILFree(pidl);
            }
            psl->Release();     
        }
    }
    return hr; 
} 


//
// Static function for determining if there's a link to the offline files
// folder sitting on the user's desktop.
//
HRESULT
COfflineFilesFolder::IsLinkOnDesktop(  // [static]
    HWND hwndParent,
    LPTSTR pszPathOut,
    UINT cchPathOut
    )
{
    TCHAR szDesktop[MAX_PATH];
    HRESULT hr = SHGetSpecialFolderPath(hwndParent, szDesktop, CSIDL_DESKTOPDIRECTORY, FALSE);
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;  // Assume not found.
        TCHAR szPath[MAX_PATH];
        PathCombine(szPath, szDesktop, TEXT("*.LNK"));
        WIN32_FIND_DATA fd;
        HANDLE hFind = FindFirstFile(szPath, &fd);
        if (INVALID_HANDLE_VALUE != hFind)
        {
            do
            {
                PathRemoveFileSpec(szPath);
                PathAppend(szPath, fd.cFileName);
                hr = IsOurLink(szPath);
                if (S_OK == hr)
                {
                    if (NULL != pszPathOut)
                    {
                        lstrcpyn(pszPathOut, szPath, cchPathOut);
                    }
                    break;
                }
            }
            while(FindNextFile(hFind, &fd));
            FindClose(hFind);
        }
    }
    return hr;
}


//
// Given a link file path, determine if it's a link to the
// offline files folder.
//
HRESULT
COfflineFilesFolder::IsOurLink(  // [static]
    LPCTSTR pszFile
    )
{
    IShellLink *psl;
    CCoInit coinit;
    HRESULT hr = coinit.Result();
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_ShellLink, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IShellLink, 
                              (void **)&psl); 

        if (SUCCEEDED(hr)) 
        {
            IPersistFile *ppf;
            hr = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
            if (SUCCEEDED(hr))
            {
                hr = ppf->Load(pszFile, STGM_DIRECT);
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidlLink;
                    hr = psl->GetIDList(&pidlLink);
                    if (SUCCEEDED(hr))
                    {
                        hr = COfflineFilesFolder::IdentifyIDList(pidlLink);
                        ILFree(pidlLink);
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
    }
    return hr;
}


//
// Determines if a given IDList is the IDList of the
// offline files folder.
//
// Returns:
//
//      S_OK    = It's our IDList.
//      S_FALSE = It's not our IDList.
//
HRESULT
COfflineFilesFolder::IdentifyIDList(  // [static]
    LPCITEMIDLIST pidl
    )
{
    IShellFolder *psf;
    HRESULT hr = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hr))
        {
            STRRET strret;
            BIND_OPTS bo;
            memset(&bo, 0, sizeof(bo));
            bo.cbStruct = sizeof(bo);
            bo.grfFlags = BIND_JUSTTESTEXISTENCE;
            bo.grfMode  = STGM_CREATE;
            pbc->SetBindOptions(&bo);
            hr = psf->GetDisplayNameOf(pidl,
                                       SHGDN_FORPARSING,
                                       &strret);
            if (SUCCEEDED(hr))
            {
                TCHAR szIDList[80];
                TCHAR szPath[80] = TEXT("::");
                StrRetToBuf(&strret, pidl, szIDList, ARRAYSIZE(szIDList));
                StringFromGUID2(CLSID_OfflineFilesFolder, 
                                &szPath[2], 
                                sizeof(szPath) - (2 * sizeof(TCHAR)));

                if (0 == lstrcmpi(szIDList, szPath))
                    hr = S_OK;
                else
                    hr = S_FALSE;
            }
            pbc->Release();
        } 
        psf->Release();
    }
    return hr;
}



HRESULT 
COfflineFilesFolder::GetFolder(   // [static]
    IShellFolder **ppsf
    )
{
    TraceAssert(NULL != ppsf);

    *ppsf = NULL;

    IShellFolder *psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
       LPITEMIDLIST pidlOfflineFiles;
       hr = COfflineFilesFolder::CreateIDList(&pidlOfflineFiles);
       if (SUCCEEDED(hr))
       {
            hr = psfDesktop->BindToObject(pidlOfflineFiles, NULL, IID_IShellFolder, (void **)ppsf);
            ILFree(pidlOfflineFiles);
       }
       psfDesktop->Release();
    }
    return hr;
}


//
// Generate a new OLID from a UNC path.
//
HRESULT
COfflineFilesFolder::OLID_CreateFromUNCPath(   // [static]
    LPCTSTR pszPath,
    const WIN32_FIND_DATA *pfd,
    DWORD dwStatus,
    DWORD dwPinCount,
    DWORD dwHintFlags,
    DWORD dwServerStatus,
    LPOLID *ppolid
    )
{
    HRESULT hr  = E_OUTOFMEMORY;
    int cchPath = lstrlen(pszPath) + 1;
    int cbIDL   = sizeof(OLID) + (cchPath * sizeof(TCHAR)) + sizeof(WORD);  // NULL terminator WORD
    WIN32_FIND_DATA fd;

    if (NULL == pfd)
    {
        //
        // Caller didn't provide a finddata block.  Use a default one
        // with all zeros.
        //
        ZeroMemory(&fd, sizeof(fd));
        pfd = &fd;
    }

    *ppolid = NULL;

    LPITEMIDLIST pidl = (LPITEMIDLIST)SHAlloc(cbIDL);
    if (NULL != pidl)
    {
        OLID *polid = (OLID *)pidl;
        memset(pidl, 0, cbIDL);
        polid->cb               = (USHORT)(cbIDL - sizeof(WORD));
        polid->uSig             = OLID_SIG;
        polid->cbFixed          = sizeof(OLID);
        polid->cchNameOfs       = (DWORD)(PathFindFileName(pszPath) - pszPath);
        polid->dwStatus         = dwStatus;
        polid->dwPinCount       = dwPinCount;
        polid->dwHintFlags      = dwHintFlags;
        polid->dwServerStatus   = dwServerStatus;
        polid->dwFileAttributes = pfd->dwFileAttributes;
        polid->dwFileSizeLow    = pfd->nFileSizeLow;
        polid->dwFileSizeHigh   = pfd->nFileSizeHigh;
        polid->ft               = pfd->ftLastWriteTime;
        lstrcpy(polid->szPath, pszPath);
        OLID_SplitPathAndName(polid);
        *ppolid = polid;
        hr = NOERROR;
    }
    return hr;
}

void
COfflineFilesFolder::OLID_GetWin32FindData(   // [static]
    LPCOLID polid,
    WIN32_FIND_DATA *pfd
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != pfd);

    ZeroMemory(pfd, sizeof(*pfd));
    pfd->dwFileAttributes = polid->dwFileAttributes;
    pfd->nFileSizeLow     = polid->dwFileSizeLow;
    pfd->nFileSizeHigh    = polid->dwFileSizeHigh;
    pfd->ftLastWriteTime  = polid->ft;
    OLID_GetFileName(polid, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
}


//
// Retrieve the full path (including filename) from an OLID.
//
LPCTSTR 
COfflineFilesFolder::OLID_GetFullPath(   // [static]
    LPCOLID polid, 
    LPTSTR pszPath,
    UINT cchPath
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != pszPath);

    PathCombine(pszPath, polid->szPath, polid->szPath + polid->cchNameOfs);
    return pszPath;
}

//
// Retrieve only the path portion of the OLID.
//
LPCTSTR 
COfflineFilesFolder::OLID_GetPath(   // [static]
    LPCOLID polid, 
    LPTSTR pszPath,
    UINT cchPath
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != pszPath);

    StrCpyN(pszPath, polid->szPath, cchPath);
    return pszPath;
}

//
// Retrieve only the filename portion of the OLID.
//
LPCTSTR 
COfflineFilesFolder::OLID_GetFileName(   // [static]
    LPCOLID polid, 
    LPTSTR pszName,
    UINT cchName
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != pszName);

    StrCpyN(pszName, polid->szPath + polid->cchNameOfs, cchName);
    return pszName;
}


//
// Restore the backslash separating the path from the filename 
// in the OLID.
//
void
COfflineFilesFolder::OLID_CombinePathAndName(   // [static]
    LPOLID polid
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(COfflineFilesFolder::ValidateIDList((LPCITEMIDLIST)polid));

    if (0 < polid->cchNameOfs)
        polid->szPath[polid->cchNameOfs - 1] = TEXT('\\');
}


//
// Remove the backslash separating the path from the filename
// in the OLID.
//
void
COfflineFilesFolder::OLID_SplitPathAndName(   // [static]
    LPOLID polid
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(COfflineFilesFolder::ValidateIDList((LPCITEMIDLIST)polid));

    if (0 < polid->cchNameOfs)
        polid->szPath[polid->cchNameOfs - 1] = TEXT('\0');
}


//
// Given an OLID this function creates a fully-qualified simple
// IDList for use by the shell.  The returned IDList is relative to the
// desktop folder.
//
HRESULT
COfflineFilesFolder::OLID_CreateSimpleIDList(   // [static]
    LPCOLID polid,
    LPITEMIDLIST *ppidlOut
    )
{
    TraceAssert(NULL != polid);
    TraceAssert(NULL != ppidlOut);
    TraceAssert(COfflineFilesFolder::ValidateIDList((LPCITEMIDLIST)polid));

    WIN32_FIND_DATA fd;
    TCHAR szFullPath[MAX_PATH];

    OLID_GetWin32FindData(polid, &fd);
    OLID_GetFullPath(polid, szFullPath, ARRAYSIZE(szFullPath));
    return SHSimpleIDListFromFindData(szFullPath, &fd, ppidlOut);
}



HRESULT
COfflineFilesFolder::OLID_Bind(   // [static]
    LPCOLID polid,
    REFIID riid,
    void **ppv,
    LPITEMIDLIST *ppidlFull,
    LPCITEMIDLIST *ppidlItem
    )
{
    *ppidlFull = NULL;
    *ppidlItem = NULL;
    HRESULT hr = OLID_CreateSimpleIDList(polid, ppidlFull);
    if (SUCCEEDED(hr))
    {
        hr = ::BindToIDListParent((LPCITEMIDLIST)*ppidlFull, riid, ppv, ppidlItem);
    }
    return hr;
}


//-----------------------------------------------------------------------------
// COfflineFilesDropTarget
//-----------------------------------------------------------------------------

COfflineFilesDropTarget::COfflineFilesDropTarget(
    HWND hwnd
    ) : m_cRef(1),
        m_hwnd(hwnd),
        m_pcm(NULL),
        m_bIsOurData(false)
{

}



COfflineFilesDropTarget::~COfflineFilesDropTarget(
    void
    )
{
    DoRelease(m_pcm);
}


HRESULT
COfflineFilesDropTarget::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineFilesDropTarget, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


ULONG
COfflineFilesDropTarget::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}


ULONG
COfflineFilesDropTarget::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;
    delete this;
    return 0;
}



HRESULT
COfflineFilesDropTarget::DragEnter(
    IDataObject *pDataObject, 
    DWORD grfKeyState, 
    POINTL pt, 
    DWORD *pdwEffect
    )
{
    HRESULT hr;

    *pdwEffect = DROPEFFECT_NONE;

    // The context menu handler has logic to check whether
    // the selected files are cacheable, etc.  It only adds
    // verbs to the context menu when it makes sense to do so.
    // We can make use of this by calling QueryContextMenu
    // here and seeing whether anything is added to the menu.

    DoRelease(m_pcm);

    if (!(m_bIsOurData = IsOurDataObject(pDataObject)))
    {
        hr = CreateOfflineFilesContextMenu(pDataObject, IID_IContextMenu, (void **)&m_pcm);
        if (SUCCEEDED(hr))
        {
            HMENU hmenu = CreateMenu();
            if (hmenu)
            {
                hr = m_pcm->QueryContextMenu(hmenu, 0, 0, 100, 0);
                DestroyMenu(hmenu);
            }
            else
                hr = E_OUTOFMEMORY;

            // Did the context menu add anything?
            if (FAILED(hr) || ResultFromShort(0) == hr)
            {
                // No, release m_pcm and set it to NULL
                DoRelease(m_pcm);
            }
            else
            {
                // Yes
                *pdwEffect |= DROPEFFECT_COPY;
            }
        }
    }
    return NOERROR;
}


HRESULT
COfflineFilesDropTarget::DragOver(
    DWORD grfKeyState, 
    POINTL pt, 
    DWORD *pdwEffect
    )
{
    *pdwEffect = DROPEFFECT_NONE;
    if (m_pcm && !m_bIsOurData)
        *pdwEffect = DROPEFFECT_COPY;
    return NOERROR;
}


HRESULT
COfflineFilesDropTarget::DragLeave(
    void
    )
{
    DoRelease(m_pcm);
    return NOERROR;
}


HRESULT
COfflineFilesDropTarget::Drop(
    IDataObject *pDataObject, 
    DWORD grfKeyState,
    POINTL pt, 
    DWORD *pdwEffect
    )
{
    HRESULT hr = E_FAIL;
    *pdwEffect = DROPEFFECT_NONE;
    if (m_pcm && !m_bIsOurData)
    {
        CMINVOKECOMMANDINFO cmi;
        ZeroMemory(&cmi, sizeof(cmi));
        cmi.cbSize = sizeof(cmi);
        cmi.hwnd   = m_hwnd;
        cmi.lpVerb = STR_PIN_VERB;
        cmi.nShow  = SW_SHOWNORMAL;
        hr = m_pcm->InvokeCommand(&cmi);

        if (SUCCEEDED(hr))
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
    }
    DoRelease(m_pcm);
    return hr;
}


HRESULT 
COfflineFilesDropTarget::CreateInstance(
    HWND hwnd,
    REFIID riid,
    void **ppv
    )
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    COfflineFilesDropTarget* pdt = new COfflineFilesDropTarget(hwnd);
    if (NULL != pdt)
    {
        hr = pdt->QueryInterface(riid, ppv);
        pdt->Release();
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


//
// If the source of the data is the Offline Files folder the data object
// will support the "Data Source Clsid" clipboard format and the CLSID
// will be CLSID_OfflineFilesFolder.
// Checking for this is how we keep from dropping our own data on ourselves.
//
bool
COfflineFilesDropTarget::IsOurDataObject(
    IDataObject *pdtobj
    )
{
    TraceAssert(NULL != pdtobj);

    bool bIsOurData = false;
    CLIPFORMAT cfSrcClsid = (CLIPFORMAT)RegisterClipboardFormat(c_szCFDataSrcClsid);
    FORMATETC fe = { cfSrcClsid, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    HRESULT hr = pdtobj->GetData(&fe, &medium);
    if (SUCCEEDED(hr))
    {
        const CLSID *pclsid = (const CLSID *)GlobalLock(medium.hGlobal);
        if (pclsid)
        {
            bIsOurData = boolify(IsEqualCLSID(CLSID_OfflineFilesFolder, *pclsid));
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
    }
    return bIsOurData;
}



//-----------------------------------------------------------------------------
// COfflineFilesViewCallback
//-----------------------------------------------------------------------------

COfflineFilesViewCallback::COfflineFilesViewCallback(
    COfflineFilesFolder *pfolder
    ) : _cRef(1)
{
    m_hwnd = NULL;
    _psfv = NULL;
    _pfolder = pfolder;
    _pfolder->AddRef();
    InitializeCriticalSection(&m_cs);
}


COfflineFilesViewCallback::~COfflineFilesViewCallback(
    void
    )
{
    _pfolder->Release();

    if (_psfv)
        _psfv->Release();

    //
    // Since the folder cache is global we don't want it taking up space while the
    // Offline Folders view isn't active.  Clear it when the view callback is 
    // destroyed.
    //
    CFolderCache::Singleton().Clear();
    DeleteCriticalSection(&m_cs);

}


STDMETHODIMP 
COfflineFilesViewCallback::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineFilesViewCallback, IShellFolderViewCB),    // IID_IShellFolderViewCB
        QITABENT(COfflineFilesViewCallback, IObjectWithSite),       // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_ (ULONG) 
COfflineFilesViewCallback::AddRef(
    void
    )
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_ (ULONG) 
COfflineFilesViewCallback::Release(
    void
    )
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


HRESULT 
COfflineFilesViewCallback::SetSite(
    IUnknown *punkSite
    )
{
    if (_psfv)
    {
        _psfv->Release();
        _psfv = NULL;
    }

    if (punkSite)
        punkSite->QueryInterface(IID_IShellFolderView, (void **)&_psfv);

    return S_OK;
}


HRESULT 
COfflineFilesViewCallback::GetSite(
    REFIID riid, 
    void **ppv
    )
{
    if (_psfv)
        return _psfv->QueryInterface(riid, ppv);

    *ppv = NULL;
    return E_FAIL;
}


STDMETHODIMP 
COfflineFilesViewCallback::MessageSFVCB(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HRESULT hres = S_OK;

    switch (uMsg)
    {
        case SFVM_COLUMNCLICK:
            if (_psfv)
                return _psfv->Rearrange((int)wParam);
            break;

        case SFVM_WINDOWCREATED:
            OnSFVM_WindowCreated((HWND)wParam);
            break;

        case SFVM_ADDPROPERTYPAGES:
            OnSFVM_AddPropertyPages((DWORD)wParam, (SFVM_PROPPAGE_DATA *)lParam);
            break;

        case SFVM_GETHELPTOPIC:
            StrCpyW(((SFVM_HELPTOPIC_DATA *)lParam)->wszHelpFile, L"offlinefolders.chm > windefault");
            break;

        case SFVM_QUERYFSNOTIFY:
            hres = OnSFVM_QueryFSNotify((SHChangeNotifyEntry *)lParam);
            break;

        case SFVM_GETNOTIFY:
            hres = OnSFVM_GetNotify((LPITEMIDLIST *)wParam, (LONG *)lParam);
            break;
            
        case SFVM_FSNOTIFY:
            hres = OnSFVM_FSNotify((LPCITEMIDLIST *)wParam, (LONG)lParam);
            break;

        case SFVM_GETVIEWS:
            hres = OnSFVM_GetViews((SHELLVIEWID *)wParam, (IEnumSFVViews **)lParam);
            break;

        case SFVM_ALTERDROPEFFECT:
            hres = OnSFVM_AlterDropEffect((DWORD *)wParam, (IDataObject *)lParam);
            break;
 
        case SFVMP_SETVIEWREDRAW:
            hres = OnSFVMP_SetViewRedraw(lParam != FALSE);
            break;

        case SFVMP_DELVIEWITEM:
            hres = OnSFVMP_DelViewItem((LPCTSTR)lParam);
            break;

        default:
            hres = E_NOTIMPL;
    }
    return hres;
}


HRESULT 
COfflineFilesViewCallback::OnSFVM_WindowCreated(
    HWND hwnd
    )
{
    m_hwnd = hwnd;
    return NOERROR;
}


HRESULT 
COfflineFilesViewCallback::OnSFVM_AddPropertyPages(
    DWORD pv, 
    SFVM_PROPPAGE_DATA *ppagedata
    )
{
    const CLSID *c_rgFilePages[] = {
        &CLSID_FileTypes,
        &CLSID_OfflineFilesOptions
    };
   
    IShellPropSheetExt * pspse;
    HRESULT hr;

    for (int i = 0; i < ARRAYSIZE(c_rgFilePages); i++)
    {
        hr = SHCoCreateInstance(NULL, 
                                c_rgFilePages[i], 
                                NULL, 
                                IID_IShellPropSheetExt, 
                                (void **)&pspse);
        if (SUCCEEDED(hr))
        {
            pspse->AddPages(ppagedata->pfn, ppagedata->lParam);
            pspse->Release();
        }
    }
    return S_OK;
}


HRESULT 
COfflineFilesViewCallback::OnSFVM_GetViews(
    SHELLVIEWID *pvid,
    IEnumSFVViews **ppev
    )
{
    //
    // Offline files folder prefers details view.
    //
    *pvid = VID_Details;
    return COfflineFilesViewEnum::CreateInstance(ppev);
}


HRESULT
COfflineFilesViewCallback::OnSFVM_GetNotify(
    LPITEMIDLIST *ppidl,
    LONG *plEvents
    )
{
    *ppidl    = NULL;
    *plEvents = GetChangeNotifyEvents();
    return NOERROR;
}


HRESULT 
COfflineFilesViewCallback::OnSFVM_QueryFSNotify(
    SHChangeNotifyEntry *pfsne
    )
{
    //
    // Register to receive global events
    //
    pfsne->pidl       = NULL;
    pfsne->fRecursive = TRUE;

    return NOERROR;
}


HRESULT
COfflineFilesViewCallback::OnSFVMP_SetViewRedraw(
    BOOL bRedraw
    )
{
    if (_psfv)
        _psfv->SetRedraw(bRedraw);
    return NOERROR;
}


HRESULT
COfflineFilesViewCallback::OnSFVMP_DelViewItem(
    LPCTSTR pszPath
    )
{
    Lock();
    HRESULT hr = RemoveItem(pszPath);
    Unlock();
    return hr;
}

//
// This is called immediately before the shell calls DoDragDrop().
// It let's us turn off "move" after all of the other drop effect
// modifications have taken place.
//
HRESULT
COfflineFilesViewCallback::OnSFVM_AlterDropEffect(
    DWORD *pdwEffect,
    IDataObject *pdtobj // unused.
    )
{
    *pdwEffect &= ~DROPEFFECT_MOVE;  // Disable move.
    return NOERROR;
}



//
// Handler for shell change notifications.
//
// We handle SHCNE_UPDATEITEM, SHCNE_UPDATEDIR, SHCNE_DELETE
// and SHCNE_RENAMEITEM
//
HRESULT 
COfflineFilesViewCallback::OnSFVM_FSNotify(
    LPCITEMIDLIST *ppidl, 
    LONG lEvent
    )
{
    HRESULT hr = NOERROR;
    if (GetChangeNotifyEvents() & lEvent)
    {
        Lock();
        if (SHCNE_RENAMEITEM & lEvent)
        {
            hr = RenameItem(*ppidl, *(ppidl + 1));
        }
        else
        {
            //
            // Convert the full pidl to a UNC path.
            //
            TCHAR szPath[MAX_PATH];
            if (SHGetPathFromIDList(*ppidl, szPath))
            {
                if (SHCNE_UPDATEDIR & lEvent)
                    hr = UpdateDir(szPath);
                else if (SHCNE_UPDATEITEM & lEvent)
                    hr = UpdateItem(szPath);
                else if (SHCNE_DELETE & lEvent)
                    hr = RemoveItem(szPath);
            }
        }
        Unlock();
    }
    return hr;
}


//
// Handler for SHCNE_RENAMEITEM notifications.
//
HRESULT
COfflineFilesViewCallback::RenameItem(
    LPCITEMIDLIST pidlOld,
    LPCITEMIDLIST pidl
    )
{
    TraceAssert(NULL != pidlOld);
    TraceAssert(NULL != pidl);

    //
    // Get the full path for the original pidl.
    //
    TCHAR szPath[MAX_PATH];
    HRESULT hr = NOERROR;
    if (SHGetPathFromIDList(pidlOld, szPath))
    {
        //
        // Find the original OLID in the listview.
        //
        LPCOLID polid = NULL;
        hr = FindOLID(szPath, &polid);
        if (SUCCEEDED(hr))
        {
            //
            // Get the full path for the new renamed pidl.
            //
            if (SHGetPathFromIDList(pidl, szPath))
            {
                //
                // Create a new OLID for the newly renamed pidl.
                //
                LPOLID polidNew;
                WIN32_FIND_DATA fd;

                ZeroMemory(&fd, sizeof(fd));
                fd.nFileSizeHigh    = polid->dwFileSizeHigh;
                fd.nFileSizeLow     = polid->dwFileSizeLow;
                fd.ftLastWriteTime  = polid->ft;
                fd.dwFileAttributes = polid->dwFileAttributes;

                hr = COfflineFilesFolder::OLID_CreateFromUNCPath(szPath,
                                                                 &fd,
                                                                 polid->dwStatus,
                                                                 polid->dwPinCount,
                                                                 polid->dwHintFlags,
                                                                 polid->dwServerStatus,
                                                                 &polidNew);
                if (SUCCEEDED(hr))
                {
                    UINT iItem;
                    //
                    // Replace the old olid in the view with the new olid.
                    // DefView will free the old one if successful.
                    //
                    hr = _psfv->UpdateObject((LPITEMIDLIST)polid, 
                                             (LPITEMIDLIST)polidNew, 
                                             &iItem);
                    if (FAILED(hr))
                    {
                        //
                        // View wouldn't accept the new OLID so free it.
                        //
                        ILFree((LPITEMIDLIST)polidNew);
                    }
                }
            }
        }
    }

    return hr;
}


//
// Locates an OLID in the view and returns the address of the
// OLID.  The returned pointer is to a const object so the caller
// should not call ILFree on it.
//
HRESULT
COfflineFilesViewCallback::FindOLID(
    LPCTSTR pszPath,
    LPCOLID *ppolid
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(NULL != ppolid);

    //
    // Create one of our OLIDs from the UNC path to use as a search key.
    //
    LPOLID polid = NULL;
    HRESULT hr = COfflineFilesFolder::OLID_CreateFromUNCPath(pszPath, NULL, 0, 0, 0, 0, &polid);
    if (SUCCEEDED(hr))
    {
        //
        // Lock so that index returned by IndexItemFromOLID() is
        // still valid in call to GetObject().
        //
        Lock();
        //
        // Get our item's index in the listview.
        //
        UINT iItem = ItemIndexFromOLID(polid);
        if ((UINT)-1 != iItem)
            hr = _psfv->GetObject((LPITEMIDLIST *)ppolid, iItem);
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        Unlock();
        ILFree((LPITEMIDLIST)polid);
        
    }
    return hr;
}



//
// Handler for SHCNE_UPDATEDIR notifications.
//
// Enumerates each immediate child of the directory and performs
// an update.
//
HRESULT
COfflineFilesViewCallback::UpdateDir(
    LPCTSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);

    HRESULT hr = NOERROR;
    //
    // First remove all items from the listview that are immediate children 
    // of this directory.  This in effect causes a refresh.
    //
    RemoveItems(pszPath);
    //
    // Now scan the CSC cache for all items in this directory and update/add
    // to the listview as appropriate.
    //
    WIN32_FIND_DATA fd;
    FILETIME ft;
    DWORD dwHintFlags;
    DWORD dwPinCount;
    DWORD dwStatus;

    CCscFindHandle hFind = CacheFindFirst(pszPath, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft);
    if (hFind.IsValid())
    {
        TCHAR szPath[MAX_PATH];
        do
        {
            if (0 == (FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes))
            {
                if (NULL != PathCombine(szPath, pszPath, fd.cFileName))
                    UpdateItem(szPath, fd, dwStatus, dwPinCount, dwHintFlags);
            }
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft));
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}


//
// Given a directory path, remove all immediate children from the listview.
//
HRESULT
COfflineFilesViewCallback::RemoveItems(
    LPCTSTR pszDir
    )
{
    TraceAssert(NULL != pszDir);

    UINT cItems;
    if (SUCCEEDED(_psfv->GetObjectCount(&cItems)))
    {
        LPCOLID polid;
        for (UINT i = 0; i < cItems; i++)
        {
            //
            // Ensure our loop control manipulation below is correct.
            //
            TraceAssert(0 <= i);
            TraceAssert(0 <= cItems);

            if (SUCCEEDED(_psfv->GetObject((LPITEMIDLIST *)&polid, i)))
            {
                if (0 == lstrcmpi(pszDir, polid->szPath))
                {
                    //
                    // This item is from the "pszDir" directory.
                    // Remove it from the listview.
                    //
                    RemoveItem(polid);
                    //
                    // Adjust item count and loop variable for deleted
                    // item.
                    //
                    cItems--;
                    i--;
                }
            }
        }
    }
    return NOERROR;
}


//
// Given an OLID, remove an item from the view.
//
HRESULT
COfflineFilesViewCallback::RemoveItem(
    LPCOLID polid
    )
{
    TraceAssert(NULL != polid);

    HRESULT hr = E_FAIL;
    UINT iItem = ItemIndexFromOLID(polid);
    if ((UINT)-1 != iItem)
    {
        //
        // File is in the listview.  Remove it.
        //
        hr = _psfv->RemoveObject((LPITEMIDLIST)polid, &iItem);
    }
    return hr;
}



//
// Give a UNC path, remove an item from the view.
//
HRESULT
COfflineFilesViewCallback::RemoveItem(
    LPCTSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);

    LPOLID polid = NULL;
    HRESULT hr   = COfflineFilesFolder::OLID_CreateFromUNCPath(pszPath, NULL, 0, 0, 0, 0, &polid);
    if (SUCCEEDED(hr))
    {
        hr = RemoveItem(polid);
        ILFree((LPITEMIDLIST)polid);
    }
    return hr;
}


//
// Handler for SHCNE_UPDATEITEM notifications.
//
// Updates a single item in the viewer.  If the item no longer
// exists in the cache, it is removed from the view.
//
HRESULT
COfflineFilesViewCallback::UpdateItem(
    LPCTSTR pszPath
    )
{
    TraceAssert(NULL != pszPath);

    HRESULT hr = NOERROR;

    DWORD dwAttr = ::GetFileAttributes(pszPath);
    if (DWORD(-1) != dwAttr)
    {
        if (0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttr))
        {
            DWORD dwHintFlags = 0;
            DWORD dwPinCount = 0;
            DWORD dwStatus = 0;
            WIN32_FIND_DATA fd;
            FILETIME ft;

            CCscFindHandle hFind = CacheFindFirst(pszPath, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft);
            if (hFind.IsValid())
            {
                hr = UpdateItem(pszPath, fd, dwStatus, dwPinCount, dwHintFlags);
            }
            else
            {
                hr = RemoveItem(pszPath);
            }
        }
    }
    return hr;
}


//
// Update a single item in the cache.  This instance of UpdateItem()
// is called once we have information on the item from the CSC cache.
// If an item doesn't already exist in the viewer, it is added.
// If an item does exist, it is updated with the new CSC info.
//
// This function assumes the item is NOT a directory.
//
HRESULT
COfflineFilesViewCallback::UpdateItem(
    LPCTSTR pszPath,
    const WIN32_FIND_DATA& fd,
    DWORD dwStatus,
    DWORD dwPinCount,
    DWORD dwHintFlags
    )
{
    TraceAssert(NULL != pszPath);
    TraceAssert(0 == (FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes));

    HRESULT hr = NOERROR;
    UINT iItem = (UINT)-1;

    //
    // Now create one of our OLIDs from the UNC path.
    //
    LPOLID polid = NULL;
    hr = COfflineFilesFolder::OLID_CreateFromUNCPath(pszPath, NULL, 0, 0, 0, 0, &polid);
    if (SUCCEEDED(hr))
    {
        //
        // Get our item's index in the listview.
        //
        LPCITEMIDLIST pidlOld = NULL;
        //
        // Lock so that index returned by ItemIndexFromOLID() is
        // still valid in call to GetObject().
        //
        Lock();
        
        iItem = ItemIndexFromOLID(polid);
        if ((UINT)-1 != iItem)
        {
            //
            // Won't be using this olid.  We'll be cloning the one from the 
            // listview.
            //
            ILFree((LPITEMIDLIST)polid);
            polid = NULL; 
            //
            // Item is in the view.  Get the existing OLID and clone it.
            // IMPORTANT:  We DON'T call ILFree on pidlOld.  Despite the
            //             argument to GetObject being non-const, it's
            //             really returning a pointer to a const object.
            //             In actuality, it's the address of the listview
            //             item's LPARAM.
            //
            hr = _psfv->GetObject((LPITEMIDLIST *)&pidlOld, iItem);
            if (SUCCEEDED(hr))
            {
                polid = (LPOLID)ILClone(pidlOld);
                if (NULL == polid)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        Unlock();
        
        if (NULL != polid)
        {
            //
            // polid either points to the new partial OLID we created 
            // with OLID_CreateFromUNCPath() or a clone of the existing 
            // OLID in the listview.  Fill/update the file and
            // CSC information.
            //
            polid->dwFileSizeHigh   = fd.nFileSizeHigh;
            polid->dwFileSizeLow    = fd.nFileSizeLow;
            polid->ft               = fd.ftLastWriteTime;
            polid->dwFileAttributes = fd.dwFileAttributes;
            polid->dwStatus         = dwStatus;
            polid->dwHintFlags      = dwHintFlags;
            polid->dwPinCount       = dwPinCount;

            if ((UINT)-1 != iItem)
            {
                //
                // Replace the old olid in the view with the new olid.
                // DefView will free the old one if successful.
                //
                hr = _psfv->UpdateObject((LPITEMIDLIST)pidlOld, 
                                         (LPITEMIDLIST)polid, 
                                         &iItem);
            }
            else
            {
                //
                // Add the new olid to the view.
                //
                hr = _psfv->AddObject((LPITEMIDLIST)polid, &iItem);
            }
            if (SUCCEEDED(hr))
            {
                //
                // Added new OLID to the listview.  Null out the local
                // ptr so we don't free the IDList later.
                //
                polid = NULL;
            }
        }
        if (NULL != polid)
            ILFree((LPITEMIDLIST)polid);
    }

    return hr;
}



//
// Retrieve the listview index for a give OLID.
// Returns:  Index of item or -1 if not found.
//
UINT
COfflineFilesViewCallback::ItemIndexFromOLID(
    LPCOLID polid
    )
{
    TraceAssert(NULL != polid);

    UINT iItem = (UINT)-1;
    UINT cItems;
    //
    // Lock so that list remains consistent while we locate the item.
    //
    Lock();
    if (SUCCEEDED(_psfv->GetObjectCount(&cItems)))
    {
        for (UINT i = 0; i < cItems; i++)
        {
            LPCITEMIDLIST pidl;
            if (SUCCEEDED(_psfv->GetObject((LPITEMIDLIST *)&pidl, i)))
            {
                //
                // Do name comparison first since it is least likely to find a match.
                //
                if (S_OK == _pfolder->CompareIDs(ICOL_NAME, pidl, (LPCITEMIDLIST)polid) &&
                    S_OK == _pfolder->CompareIDs(ICOL_LOCATION, pidl, (LPCITEMIDLIST)polid))
                {
                    iItem = i;
                    break;
                }
            }
        }
    }
        
    Unlock();        
    return (UINT)iItem;
}



//-----------------------------------------------------------------------------
// COfflineFilesViewEnum
//-----------------------------------------------------------------------------
COfflineFilesViewEnum::COfflineFilesViewEnum(
    void
    ) 
    : m_cRef(1),
      m_iAddView(0)
{

}

COfflineFilesViewEnum::~COfflineFilesViewEnum(
    void
    )
{

}


HRESULT
COfflineFilesViewEnum::CreateInstance(
    IEnumSFVViews **ppenum
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    COfflineFilesViewEnum *pEnum = new COfflineFilesViewEnum;
    if (NULL != pEnum)
    {
        hr = pEnum->QueryInterface(IID_IEnumSFVViews, (void **)ppenum);
    }    
    return hr;
}



STDMETHODIMP 
COfflineFilesViewEnum::QueryInterface (
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineFilesViewEnum, IEnumSFVViews),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) 
COfflineFilesViewEnum::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) 
COfflineFilesViewEnum::Release(
    void
    )
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP 
COfflineFilesViewEnum::Next(
    ULONG celt, 
    SFVVIEWSDATA **ppData, 
    ULONG *pceltFetched
    )
{
    HRESULT hr = S_FALSE;
    ULONG celtFetched = 0;
    
    if (!celt || !ppData || (celt > 1 && !pceltFetched))
    {
        return E_INVALIDARG;
    }

    if (0 == m_iAddView)
    {
        //
        // All we add is Thumbnail view.
        //
        ppData[0] = (SFVVIEWSDATA *) SHAlloc(sizeof(SFVVIEWSDATA));
        if (ppData[0])
        {
            ppData[0]->idView         = CLSID_ThumbnailViewExt;
            ppData[0]->idExtShellView = CLSID_ThumbnailViewExt;
            ppData[0]->dwFlags        = SFVF_TREATASNORMAL | SFVF_NOWEBVIEWFOLDERCONTENTS;
            ppData[0]->lParam         = 0x00000011;
            ppData[0]->wszMoniker[0]  = 0;

            celtFetched++;
            m_iAddView++;
            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    if ( pceltFetched )
    {
        *pceltFetched = celtFetched;
    }
    
    return hr;
}

STDMETHODIMP 
COfflineFilesViewEnum::Skip(
    ULONG celt
    )
{
    if (celt && !m_iAddView)
    {
        m_iAddView++;
        celt--;
    }

    return (celt ? S_FALSE : S_OK );
}

STDMETHODIMP COfflineFilesViewEnum::Reset(
    void
    )
{
    m_iAddView = 0;
    return NOERROR;
}


STDMETHODIMP 
COfflineFilesViewEnum::Clone(
    IEnumSFVViews **ppenum
    )
{
    return CreateInstance(ppenum);
}


//-----------------------------------------------------------------------------
// COfflineDetails
//-----------------------------------------------------------------------------

COfflineDetails::COfflineDetails(
    COfflineFilesFolder *pfolder
    ) : _cRef (1)
{
    _pfolder = pfolder;
    _pfolder->AddRef();
}


COfflineDetails::~COfflineDetails()
{
    if (_pfolder)
        _pfolder->Release();

}


STDMETHODIMP 
COfflineDetails::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    static const QITAB qit[] = {
        QITABENT(COfflineDetails, IShellDetails),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


STDMETHODIMP_(ULONG) 
COfflineDetails::AddRef(
    void
    )
{
    return InterlockedIncrement(&_cRef);
}


STDMETHODIMP_(ULONG) 
COfflineDetails::Release(
    void
    )
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}



//-----------------------------------------------------------------------------
// CFileTypeCache
//
// Implements a simple hash table for storing file type strings keyed on
// file extension.
//
//-----------------------------------------------------------------------------
CFileTypeCache::CFileTypeCache(
    int cBuckets
    ) : m_cBuckets(cBuckets),
        m_prgBuckets(NULL)
{
    InitializeCriticalSection(&m_cs);
}


CFileTypeCache::~CFileTypeCache(
    void
    )
{
    Lock();
    if (NULL != m_prgBuckets)
    {
        for (int i = 0; i < m_cBuckets; i++)
        {
            while(NULL != m_prgBuckets[i])
            {
                CEntry *pDelThis = m_prgBuckets[i];
                m_prgBuckets[i]  = m_prgBuckets[i]->Next();
                delete pDelThis;
            }
        }
        delete[] m_prgBuckets;
        m_prgBuckets = NULL;
    }
    Unlock();
    DeleteCriticalSection(&m_cs);
}


CFileTypeCache::CEntry *
CFileTypeCache::Lookup(
    LPCTSTR pszExt
    )
{
    if (NULL != m_prgBuckets)
    {
        for (CEntry *pEntry = m_prgBuckets[Hash(pszExt)]; pEntry; pEntry = pEntry->Next())
        {
            if (0 == pEntry->CompareExt(pszExt))
                return pEntry;
        }
    }
    return NULL;
}



HRESULT
CFileTypeCache::Add(
    LPCTSTR pszExt,
    LPCTSTR pszTypeName
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    if (NULL != m_prgBuckets)
    {
        CEntry *pNewEntry = new CEntry(pszExt, pszTypeName);
        if (NULL != pNewEntry && pNewEntry->IsValid())
        {
            //
            // Link new entry at the head of the bucket's linked list.
            //
            int iHash = Hash(pszExt);
            pNewEntry->SetNext(m_prgBuckets[iHash]);
            m_prgBuckets[iHash] = pNewEntry;
            hr = NOERROR;
        }
        else
        {
            delete pNewEntry;
        }
    }
    return hr;
}



HRESULT
CFileTypeCache::GetTypeName(
    LPCTSTR pszPath,          // Can be full path or only "filename.ext".
    DWORD dwFileAttributes,
    LPTSTR pszDest,
    int cchDest
    )
{
    Lock();
    if (NULL == m_prgBuckets)
    {
        //
        // Create hash bucket array on-demand.  This way it's not
        // created until someone asks for something from the cache.
        // Simple "creation" of the cache object is therefore cheap.
        //
        m_prgBuckets = new CEntry* [m_cBuckets];
        if (NULL != m_prgBuckets)
        {
            ZeroMemory(m_prgBuckets, sizeof(m_prgBuckets[0]) * m_cBuckets);
        }
    }

    SHFILEINFO sfi;
    LPCTSTR pszTypeName = NULL;
    LPCTSTR pszExt      = ::PathFindExtension(pszPath);

    //
    // Note that Lookup will gracefully fail if the hash bucket array
    // creation failed.  In that case we'll get the info from 
    // SHGetFileInfo and return it directly to the caller.  This means
    // that failure to create the cache is not fatal.  It just means we
    // don't cache any data.
    //
    CEntry *pEntry = Lookup(pszExt);
    if (NULL != pEntry)
    {
        // Cache hit.
        pszTypeName = pEntry->TypeName();
    }
    if (NULL == pszTypeName)
    {
        // Cache miss.
        if (SHGetFileInfo(::PathFindFileName(pszPath), 
                          dwFileAttributes, 
                          &sfi, 
                          sizeof(sfi), 
                          SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
        {
            //
            // Add new entry to cache.  We're not concerned if the 
            // addition fails.  It just means we'll get a cache miss
            // on this item next time and repeat the SHGetFileInfo call.
            //
            pszTypeName = sfi.szTypeName;
            Add(pszExt, sfi.szTypeName);
        }
    }
    if (NULL != pszTypeName)
    {
        lstrcpyn(pszDest, pszTypeName, cchDest);
    }
    Unlock();
    return pszTypeName ? NOERROR : E_FAIL;
}



int
CFileTypeCache::Hash(
    LPCTSTR pszExt
    )
{
    int iSum = 0;
    while(*pszExt)
        iSum += int(*pszExt++);

    return iSum % m_cBuckets;
}



CFileTypeCache::CEntry::CEntry(
    LPCTSTR pszExt,
    LPCTSTR pszTypeName
    ) : m_pNext(NULL)
{
    m_pszExt      = StrDup(pszExt);
    m_pszTypeName = StrDup(pszTypeName);
}


CFileTypeCache::CEntry::~CEntry(
    void
    )
{
    delete[] m_pszExt;
    delete[] m_pszTypeName;
}


LPTSTR
CFileTypeCache::CEntry::StrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
        lstrcpy(pszNew, psz);
    return pszNew;
}





//
// This function creates our standard offline-files context menu.
// This is the one used by the shell that inserts the 
// "Make Available Offline" and "Synchronize" items.
//
HRESULT
CreateOfflineFilesContextMenu(
    IDataObject *pdtobj,
    REFIID riid,
    void **ppv
    )
{
    TraceAssert(NULL != ppv);

    HRESULT hr = E_OUTOFMEMORY;

    *ppv = NULL;

    CCscShellExt *pse = new CCscShellExt;
    if (NULL != pse)
    {
        IShellExtInit *psei;
        hr = pse->QueryInterface(IID_IShellExtInit, (void **)&psei);
        pse->Release();
        if (SUCCEEDED(hr))
        {
            if (NULL != pdtobj)
                hr = psei->Initialize(NULL, pdtobj, NULL);

            if (SUCCEEDED(hr))
            {
                hr = psei->QueryInterface(riid, ppv);
            }
            psei->Release();
        }
    }
    return hr;
}

