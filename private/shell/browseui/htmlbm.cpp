
#include "priv.h"
//#include "local.h"
#include "sccls.h"
#include "htmlbm.h"
//#include "deskstat.h"
//#include "dutil.h"
#include "isfband.h"
#include "runtask.h"
#include "dbgmem.h"

#ifndef POSTPOSTSPLIT

static const GUID TOID_Thumbnail = { 0xadec3450, 0xe907, 0x11d0, {0xa5, 0x7b, 0x00, 0xc0, 0x4f, 0xc2, 0xf7, 0x6a} };

HRESULT CDiskCacheTask_Create( CLogoBase * pView,
                               LPSHELLIMAGESTORE pImageStore,
                               LPCWSTR pszItem,
                               LPCWSTR pszGLocation,
                               DWORD dwItem,
                               LPRUNNABLETASK * ppTask );
                               
class CDiskCacheTask : public CRunnableTask
{
    public:
        CDiskCacheTask();
        ~CDiskCacheTask();

        STDMETHODIMP RunInitRT( void );

        friend HRESULT CDiskCacheTask_Create( CLogoBase * pView,
                               LPSHELLIMAGESTORE pImageStore,
                               LPCWSTR pszItem,
                               LPCWSTR pszGLocation,
                               DWORD dwItem,
                               const SIZE * prgSize,
                               LPRUNNABLETASK * ppTask );

    protected:
        HRESULT PrepImage( HBITMAP * phBmp );
        
        LPSHELLIMAGESTORE _pImageStore;
        WCHAR _szItem[MAX_PATH];
        WCHAR _szGLocation[MAX_PATH];
        CLogoBase * _pView;
        DWORD _dwItem;
        SIZE m_rgSize;
};

// CreateInstance
HRESULT CThumbnail_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CThumbnail *thumbnailInstance = new CThumbnail();
    if (thumbnailInstance != NULL)
    {
        *ppunk = SAFECAST(thumbnailInstance, IThumbnail*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

// Constructor / Destructor
CThumbnail::CThumbnail(void):
    m_cRef(1)
{
    DllAddRef();
}

CThumbnail::~CThumbnail(void)
{
    if (_pTaskScheduler)
    {
        _pTaskScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);
        _pTaskScheduler->Release();
        _pTaskScheduler = NULL;
    }

    DllRelease();
}

// *** IUnknown ***
HRESULT CThumbnail::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IThumbnail))
    {
        *ppvObj = SAFECAST(this, IThumbnail*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CThumbnail::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}

ULONG CThumbnail::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;

    if (m_cRef > 0)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

// *** IThumbnail ***
HRESULT CThumbnail::Init(HWND hwnd, UINT uMsg)
{
    _hwnd = hwnd;
    _uMsg = uMsg;
    _pTaskScheduler = NULL;

    return S_OK;
}   // Init

HRESULT CThumbnail::GetBitmap(LPCWSTR wszFile, DWORD dwItem, LONG lWidth, LONG lHeight)
{
    HRESULT hr = E_FAIL;

    //
    // Make sure we have a task scheduler.
    //
    if (!_pTaskScheduler)
    {
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_IShellTaskScheduler, (void **)&_pTaskScheduler)))
        {
            // make sure RemoveTasks() actually kills old tasks even if they're not done yet
            _pTaskScheduler->Status(ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE);
        }
    }

    if (_pTaskScheduler)
    {
        //
        // Kill any old tasks in the scheduler.
        //
        _pTaskScheduler->RemoveTasks(TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, FALSE);

        if (wszFile)
        {
            LPITEMIDLIST pidl = ILCreateFromPathW(wszFile);
            if (pidl)
            {
                LPCITEMIDLIST pidlFile;
                IShellFolder *psf;

                if (SUCCEEDED(SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlFile)))
                {
                    IExtractImage *pei;

                    if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlFile, IID_IExtractImage, NULL, (void **)&pei)))
                    {
                        DWORD dwPriority;
                        DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_ASPECT | IEIFLAG_OFFLINE;
                        SIZEL rgSize;

                        rgSize.cx = lWidth;
                        rgSize.cy = lHeight;

                        WCHAR szBufferW[MAX_PATH];
                        HRESULT hr2 = pei->GetLocation(szBufferW, ARRAYSIZE(szBufferW), &dwPriority, &rgSize, SHGetCurColorRes(), &dwFlags);
                        if ( hr2 == E_PENDING)
                        {
                            // now get the date stamp and check the disk cache....
                            FILETIME ftImageTimeStamp;
                            BOOL fNoDateStamp = TRUE;
                            LPEXTRACTIMAGE2 pei2;
                            // try it in the background...
                            LPRUNNABLETASK prt;

                            // od they support date stamps....
                            if ( SUCCEEDED( pei->QueryInterface( IID_IExtractImage2, (void **) &pei2)))
                            {
                                if ( SUCCEEDED( pei2->GetDateStamp( & ftImageTimeStamp )))
                                {
                                    // Houston, we have a date stamp..
                                    fNoDateStamp = FALSE;
                                }
        
                                pei2->Release();
                            }

                            // if it is in the cache, and it is an uptodate image, then fetch from disk....
                            // if the timestamps are wrong, then the extract code further down will then try

                            // we only test the cache on NT5 because the templates are old on other platforms and 
                            // thus the image will be the wrong size...
                            if ( IsOS( OS_NT5 ) && IsItInCache( wszFile, szBufferW, ( fNoDateStamp ? NULL : &ftImageTimeStamp )))
                            {
                                hr = CDiskCacheTask_Create( this, _pImageStore, wszFile, szBufferW,  
                                                            dwItem, &rgSize, & prt );

                                if ( SUCCEEDED( hr ))
                                {
                                    // let go of the image store, so the task has the only ref and the lock..
                                    ATOMICRELEASE( _pImageStore );
                                }
                            }
                            else
                            {
                                // Cannot hold the prt which is returned in a member variable since that
                                // would be a circular reference
                                hr = CExtractImageTask_Create(this, 
                                                pei, L"", dwItem,
                                                -1, EITF_SAVEBITMAP | EITF_ALWAYSCALL, &prt);
                            }
                            
                            //
                            // Create our own task to perform the extraction.
                            //
                            if ( SUCCEEDED( hr ))
                            {
                                //
                                // Add the task to the scheduler.
                                //
                                if (SUCCEEDED(_pTaskScheduler->AddTask(prt, TOID_Thumbnail, ITSAT_DEFAULT_LPARAM, dwPriority)))
                                {
                                    // now belongs to the scheduler thread
                                    remove_from_memlist( prt );
                                    hr = S_OK;
                                }
                                else
                                {
                                    TraceMsg(TF_WARNING, "hb: Could not add task!");
                                }

                                prt->Release();
                            }
                            else
                            {
                                TraceMsg(TF_WARNING, "hb: Could not create task!");
                            }
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "hb: Could not get location on IExtractImage");
                        }

                        pei->Release();
                    }

                    psf->Release();
                }

                ILFree(pidl);
            }
        }
    }

    // make sure we are not still holding it...
    ReleaseImageStore();
    
    return hr;
}   // GetBitmap

// private stuff
HRESULT CThumbnail::UpdateLogoCallback(DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache)
{
    if (!PostMessage(_hwnd, _uMsg, dwItem, (LPARAM)hImage))
    {
        DeleteObject(hImage);
    }

    return S_OK;
}

REFTASKOWNERID CThumbnail::GetTOID()    
{ 
    return TOID_Thumbnail;
}

BOOL CThumbnail::IsItInCache( LPCWSTR pszItemPath, LPCWSTR pszGLocation, const FILETIME * pftDateStamp )
{
    LPPERSISTFILE pFile;
    WCHAR szName[MAX_PATH];
    BOOL fRes = FALSE;
    DWORD dwStoreLock;
    
    // use pszItemPath to find the cache.....
    StrCpyNW( szName, pszItemPath, MAX_PATH );
    PathRemoveFileSpecW( szName );

    HRESULT hr = CoCreateInstance( CLSID_ShellThumbnailDiskCache, NULL, CLSCTX_INPROC, IID_IPersistFile, (void **) & pFile);
    if (FAILED( hr ))
    {
        return FALSE;
    }

    hr = pFile->Load( szName, STGM_READ );
    if ( SUCCEEDED( hr ))
    {
        hr = pFile->QueryInterface( IID_IShellImageStore, (void **) & _pImageStore );
    }

    if ( SUCCEEDED( hr ))
    {
        hr = _pImageStore->Open( STGM_READ, & dwStoreLock );
    }
    
    pFile->Release();
    
    if ( FAILED( hr ))
    {
        return FALSE;
    }
    
    FILETIME ftCacheDateStamp;
    hr = _pImageStore->IsEntryInStore( pszGLocation, &ftCacheDateStamp );
    if (( hr == S_OK ) && (!pftDateStamp || 
        (pftDateStamp->dwLowDateTime == ftCacheDateStamp.dwLowDateTime && 
         pftDateStamp->dwHighDateTime == ftCacheDateStamp.dwHighDateTime)))
    {
        fRes = TRUE;
    }
    _pImageStore->Close( &dwStoreLock );
    
    return fRes;
}

void CThumbnail::ReleaseImageStore()
{
    ATOMICRELEASE( _pImageStore );
}


////////////////////////////////////////////////////////////////////////////////////
HRESULT CDiskCacheTask_Create( CLogoBase * pView,
                               LPSHELLIMAGESTORE pImageStore,
                               LPCWSTR pszItem,
                               LPCWSTR pszGLocation,
                               DWORD dwItem,
                               const SIZE * prgSize,
                               LPRUNNABLETASK * ppTask )
{
    if ( !ppTask || !pView || !pszItem || !pszGLocation || !pImageStore || !prgSize )
    {
        return E_INVALIDARG;
    }

    CDiskCacheTask *pTask = new CDiskCacheTask;
    if ( pTask == NULL )
    {
        return E_OUTOFMEMORY;
    }

    StrCpyW( pTask->_szItem, pszItem );
    StrCpyW( pTask->_szGLocation, pszGLocation );
    
    pTask->_pView = pView;
    pTask->_pImageStore = pImageStore;
    pImageStore->AddRef();
    pTask->_dwItem = dwItem;

    pTask->m_rgSize = * prgSize;
    
    *ppTask = SAFECAST( pTask, IRunnableTask *);
    return NOERROR;
}

STDMETHODIMP CDiskCacheTask::RunInitRT( )
{
    // otherwise, run the task ....
    HBITMAP hBmp = NULL;
    DWORD dwLock;

    HRESULT hr = _pImageStore->Open (STGM_READ, &dwLock );
    if ( SUCCEEDED( hr ))
    {
        // at this point, we assume that it IS in the cache, and we already have a read lock on the cache...
        hr = _pImageStore->GetEntry( _szGLocation, STGM_READ, &hBmp );
    
        // release the lock, we don't need it...
        _pImageStore->Close( &dwLock );
    }
    ATOMICRELEASE( _pImageStore );

    PrepImage( &hBmp );
    
    _pView->UpdateLogoCallback( _dwItem, 0, hBmp, _szItem, TRUE );

    // ensure we don't return the  "we've suspended" value...
    if ( hr == E_PENDING )
        hr = E_FAIL;
        
    return hr;
}

CDiskCacheTask::CDiskCacheTask()
    : CRunnableTask( RTF_DEFAULT )
{
}

CDiskCacheTask::~CDiskCacheTask()
{
    ATOMICRELEASE( _pImageStore );
}

HRESULT CDiskCacheTask::PrepImage( HBITMAP * phBmp )
{
    ASSERT( phBmp && *phBmp );

    DIBSECTION rgDIB;

    if ( !GetObject( *phBmp, sizeof( rgDIB ), &rgDIB ))
    {
        return E_FAIL;
    }

    // the disk cache only supports 32 Bpp DIBS now, so we can ignore the palette issue...
    ASSERT( rgDIB.dsBm.bmBitsPixel == 32 );
    
    HBITMAP hBmpNew = NULL;
    HPALETTE hPal = NULL;
    if ( SHGetCurColorRes() == 8 )
    {
        hPal = SHCreateShellPalette( NULL );
    }
    
    IScaleAndSharpenImage2 * pScale;
    HRESULT hr = CoCreateInstance( CLSID_ThumbnailScaler,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IScaleAndSharpenImage2,
                           (void **) &pScale );
    if ( SUCCEEDED( hr ))
    {
        hr = pScale->ScaleSharpen2((BITMAPINFO *) &rgDIB.dsBmih,
                                    rgDIB.dsBm.bmBits,
                                    &hBmpNew,
                                    &m_rgSize,
                                    SHGetCurColorRes(),
                                    hPal,
                                    0, FALSE );
        pScale->Release();
    }

    DeletePalette( hPal );
    
    if ( SUCCEEDED (hr ) && hBmpNew )
    {
        DeleteObject( *phBmp );
        *phBmp = hBmpNew;
    }

    return NOERROR;
}

#endif // !POSTPOSTSPLIT
