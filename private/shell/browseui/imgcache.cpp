#include "priv.h"

HBITMAP CreateMirroredBitmap( HBITMAP hbmOrig)
{
    HDC     hdc, hdcMem1, hdcMem2;
    HBITMAP hbm = NULL, hOld_bm1, hOld_bm2;
    BITMAP  bm;


    if (!hbmOrig)
        return NULL;

    if (!GetObject(hbmOrig, sizeof(BITMAP), &bm))
        return NULL;

    // Grab the screen DC
    hdc = GetDC(NULL);

    hdcMem1 = CreateCompatibleDC(hdc);

    if (!hdcMem1)
    {
        ReleaseDC(NULL, hdc);
        return NULL;
    }
    
    hdcMem2 = CreateCompatibleDC(hdc);
    if (!hdcMem2)
    {
        DeleteDC(hdcMem1);
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    hbm = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);

    if (!hbm)
    {
        ReleaseDC(NULL, hdc);
        DeleteDC(hdcMem1);
        DeleteDC(hdcMem2);
        return NULL;
    }

    //
    // Flip the bitmap
    //
    hOld_bm1 = (HBITMAP)SelectObject(hdcMem1, hbmOrig);
    hOld_bm2 = (HBITMAP)SelectObject(hdcMem2 , hbm );

    SET_DC_RTL_MIRRORED(hdcMem2);

    BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

    SelectObject(hdcMem1, hOld_bm1 );
    SelectObject(hdcMem1, hOld_bm2 );
    
    DeleteDC(hdcMem1);
    DeleteDC(hdcMem2);

    ReleaseDC(NULL, hdc);

    return hbm;
}

HICON CreateMirroredIcon(HICON hiconOrg)
{
    HDC      hdcScreen, hdcBitmap, hdcMask = NULL;
    HBITMAP  hbm, hbmMask, hbmOld,hbmOldMask;
    BITMAP   bm;
    ICONINFO ii;
    HICON    hicon = NULL;
#ifdef WINNT
#define      IPIXELOFFSET 0 
#else // !WINNT
#define      IPIXELOFFSET 2
#endif WINNT

        hdcBitmap = CreateCompatibleDC(NULL);
        if (hdcBitmap)
        {
            hdcMask = CreateCompatibleDC(NULL);

            if( hdcMask )
            {

                SET_DC_RTL_MIRRORED(hdcBitmap);
                SET_DC_RTL_MIRRORED(hdcMask);
            }
            else
            {
                DeleteDC( hdcBitmap );
                hdcBitmap = NULL;
            }
        }
         
    hdcScreen = GetDC(NULL);

    if (hdcBitmap && hdcScreen && hdcMask) 
    {
        if( hiconOrg )
        {
            if( GetIconInfo(hiconOrg, &ii) &&
                GetObject(ii.hbmColor, sizeof(BITMAP), &bm))
            {
                //
                // I don't want these.
                //
                DeleteObject( ii.hbmMask );
                DeleteObject( ii.hbmColor );
                ii.hbmMask = ii.hbmColor = NULL;

                hbm = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
                hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
                hbmOld = (HBITMAP)SelectObject(hdcBitmap, hbm);
                hbmOldMask = (HBITMAP)SelectObject(hdcMask, hbmMask);
      
                DrawIconEx(hdcBitmap, IPIXELOFFSET, 0, hiconOrg, bm.bmWidth, bm.bmHeight, 0,
                           NULL, DI_IMAGE);

                DrawIconEx(hdcMask, IPIXELOFFSET, 0, hiconOrg, bm.bmWidth, bm.bmHeight, 0,
                           NULL, DI_MASK);

                SelectObject(hdcBitmap, hbmOld);
                SelectObject(hdcMask, hbmOldMask);

                //
                // create the new mirrored icon, and delete bmps
                //
                ii.hbmMask  = hbmMask;
                ii.hbmColor = hbm;
                hicon = CreateIconIndirect(&ii);

                DeleteObject(hbm);
                DeleteObject(hbmMask);
            }
               
        }
    }

    ReleaseDC(NULL, hdcScreen);
    if (hdcBitmap)
        DeleteDC(hdcBitmap);

    if (hdcMask)
        DeleteDC(hdcMask);

    return hicon;
}

HBITMAP AddImage_PrepareBitmap(LPCIMAGECACHEINFO pInfo, HBITMAP hbmp)
{
    if (pInfo->dwMask & ICIFLAG_MIRROR)
    {
        return CreateMirroredBitmap(hbmp);
    }
    else 
    {
        return hbmp;
    }    
 }

HICON AddImage_PrepareIcon(LPCIMAGECACHEINFO pInfo, HICON hicon)
{
    if (pInfo->dwMask & ICIFLAG_MIRROR)
    {
        return CreateMirroredIcon(hicon);
    }
    else 
    {
        return hicon;
    }    
}

void AddImage_CleanupBitmap(LPCIMAGECACHEINFO pInfo, HBITMAP hbmp)
{
    if (pInfo->dwMask & ICIFLAG_MIRROR)
    {
        if (hbmp)
        {
            DeleteObject(hbmp);
        }    
    }
}

void AddImage_CleanupIcon(LPCIMAGECACHEINFO pInfo, HICON hicon)
{
    if (pInfo->dwMask & ICIFLAG_MIRROR)
    {
        if (hicon)
        {
            DestroyIcon(hicon);
        }    
    }
}

///////////////////////////////////////////////////////////////////////////////////
typedef struct 
{
    DWORD       dwFlags;    // key: flags
    int         iIndex;     // data: icon index
    UINT        iUsage;     // usage count....
    FILETIME    ftDateStamp;
    WCHAR   szName[1];  // the filename of the item....
} ICONCACHE_DATA, *PICONCACHE_DATA;

class CImageListCache : public IImageCache2
{
public:
    CImageListCache( void );
    ~CImageListCache( void );

    STDMETHOD ( QueryInterface )( REFIID riid, LPVOID * ppvObj );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );
    
    STDMETHOD ( AddImage ) ( THIS_ LPCIMAGECACHEINFO pInfo, UINT * puIndex );
    STDMETHOD ( FindImage ) ( THIS_ LPCIMAGECACHEINFO pInfo, UINT * puIndex );
    STDMETHOD ( FreeImage ) ( THIS_ UINT iImageIndex );
    STDMETHOD ( Flush ) ( THIS_ BOOL fRelease );
    STDMETHOD ( ChangeImageInfo ) ( THIS_ UINT IImageIndex, LPCIMAGECACHEINFO pInfo );
    STDMETHOD ( GetCacheSize ) ( THIS_ UINT * puSize );
    STDMETHOD ( GetUsage ) ( THIS_ UINT uIndex, UINT * puUsage );

    STDMETHOD( GetImageList ) ( THIS_ LPIMAGECACHEINITINFO pInfo );

    STDMETHOD ( DeleteImage ) ( UINT iImageIndex );
    STDMETHOD ( GetImageInfo ) ( UINT iImageIndex, LPIMAGECACHEINFO pInfo );
    
protected:  //internal methods.
    UINT CountFreeSlots( void );
    int FindEmptySlot( void );
    ICONCACHE_DATA * CreateDataNode( LPCIMAGECACHEINFO pInfo ) const;

    HDPA            m_hListData;
    HIMAGELIST      m_himlLarge;
    HIMAGELIST      m_himlSmall;
    CRITICAL_SECTION m_csLock;
    DWORD           m_dwFlags;
    long            m_cRef;
};

STDAPI  CImageListCache_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CImageListCache * pCache = new CImageListCache();
    if (pCache != NULL)
    {
        *ppunk = SAFECAST(pCache, IImageCache *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    if ( ppvObj == NULL )
    {
        return E_INVALIDARG;
    }

    if ( riid == IID_IUnknown || riid == IID_IImageCache || riid == IID_IImageCache2 )
    {
        *ppvObj = (LPVOID) ( IImageCache2 *) this;
    }
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
    
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImageListCache::AddRef()
{
    InterlockedIncrement( &m_cRef );
    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImageListCache::Release()
{
    if (InterlockedDecrement( &m_cRef ))
        return m_cRef;

    delete this;
    return 0;
}

WCHAR const c_szDefault[] = L"Default";

int CALLBACK DestroyEnum( LPVOID p, LPVOID pData )
{
    ASSERT( p );
    LocalFree((ICONCACHE_DATA *) p);

    return TRUE;
}

int CALLBACK UsageEnum( LPVOID p, LPVOID pData )
{
    ASSERT( p);
    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) p;

    pNode->iUsage = PtrToUlong(pData);
    
    return TRUE;
}

//--- Implementation of CImageListCache public methods.
////////////////////////////////////////////////////////////////////////////////////
CImageListCache::CImageListCache( )
{
    // imagelist info.
    InitializeCriticalSection( &m_csLock );
    m_cRef = 1;

    DllAddRef();
}

////////////////////////////////////////////////////////////////////////////////////
CImageListCache::~CImageListCache( )
{
    // don't bother entering the critical section, if we shouldn't be accessed
    // by multiple threads if we have reached the destructor...
    
    if ( m_himlLarge ) 
    {
        ImageList_Destroy( m_himlLarge );
    }
    if ( m_himlSmall ) 
    {
        ImageList_Destroy( m_himlSmall );
    }

    if ( m_hListData )
    {
        DPA_DestroyCallback( m_hListData, DestroyEnum, NULL );
    }
    
    DeleteCriticalSection( &m_csLock );

    DllRelease();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
ICONCACHE_DATA * CImageListCache::CreateDataNode( LPCIMAGECACHEINFO pInfo ) const
{
    UINT cbSize = sizeof( ICONCACHE_DATA );
    if ( pInfo->dwMask & ICIFLAG_NAME )
    {
        ASSERT( pInfo->pszName );
        cbSize += lstrlenW( pInfo->pszName ) * sizeof( WCHAR );
    }

    // zero init mem alloc
    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) LocalAlloc( LPTR, cbSize );
    if ( !pNode )
    {
        return NULL;
    }
    
    // fill in the data...
    if ( pInfo->dwMask & ICIFLAG_NAME )
    {
        StrCpyW( pNode->szName, pInfo->pszName );
    }
    pNode->iIndex = pInfo->iIndex;
    pNode->dwFlags = pInfo->dwFlags;
    pNode->iUsage = 1;
    pNode->ftDateStamp = pInfo->ftDateStamp;

    return pNode;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: AddImage ( THIS_ LPCIMAGECACHEINFO pInfo, UINT * puIndex )
{
    if ( !pInfo || !puIndex || !(pInfo->dwMask & (ICIFLAG_LARGE | ICIFLAG_SMALL)) ||
        !(pInfo->dwMask & (ICIFLAG_BITMAP | ICIFLAG_ICON )))
    {
        return E_INVALIDARG;
    }

    ICONCACHE_DATA * pNode = CreateDataNode( pInfo );
    if ( !pNode )
    {
        return E_OUTOFMEMORY;
    }

    if ( pInfo->dwMask & ICIFLAG_NOUSAGE )
    {
        pNode->iUsage = (UINT)-1;
    }
    
    EnterCriticalSection( &m_csLock );
    int iIndex = FindEmptySlot();
    if ( iIndex != -1 )
    {
        // swap for the old one...
        ICONCACHE_DATA * pOld = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, iIndex );
        DPA_SetPtr( m_hListData, iIndex, pNode );
        
        LocalFree((LPVOID) pOld );

        ASSERT(!(m_dwFlags & ICIIFLAG_LARGE) == !(pInfo->dwMask & ICIFLAG_LARGE)
                && !(m_dwFlags & ICIIFLAG_SMALL) == !(pInfo->dwMask & ICIFLAG_SMALL));
        
        if ( pInfo->dwMask & ICIFLAG_LARGE )
        {
            ASSERT( m_dwFlags & ICIIFLAG_LARGE );
            if ( pInfo->dwMask & ICIFLAG_BITMAP )
            {
                ASSERT( pInfo->hBitmapLarge );

                HBITMAP hBitmapLarge = AddImage_PrepareBitmap(pInfo, pInfo->hBitmapLarge);
                HBITMAP hMaskLarge = AddImage_PrepareBitmap(pInfo, pInfo->hMaskLarge);
                ImageList_Replace( m_himlLarge, iIndex, hBitmapLarge, hMaskLarge );
                AddImage_CleanupBitmap(pInfo, hBitmapLarge);
                AddImage_CleanupBitmap(pInfo, hMaskLarge);
            }
            else
            {
                ASSERT( pInfo->hIconLarge && pInfo->dwMask & ICIFLAG_ICON );

                HICON hIconLarge = AddImage_PrepareIcon(pInfo, pInfo->hIconLarge);
                ImageList_ReplaceIcon( m_himlLarge, iIndex, hIconLarge );
                AddImage_CleanupIcon(pInfo, hIconLarge);
             }
        }
        if ( pInfo->dwMask & ICIFLAG_SMALL )
        {
            ASSERT( m_dwFlags & ICIIFLAG_SMALL );
            if ( pInfo->dwMask & ICIFLAG_BITMAP )
            {
                ASSERT( pInfo->hBitmapSmall );

                HBITMAP hBitmapSmall = AddImage_PrepareBitmap(pInfo, pInfo->hBitmapSmall);
                HBITMAP hMaskSmall = AddImage_PrepareBitmap(pInfo, pInfo->hMaskSmall);
                ImageList_Replace( m_himlSmall, iIndex, hBitmapSmall, hMaskSmall );
                AddImage_CleanupBitmap(pInfo, hBitmapSmall);
                AddImage_CleanupBitmap(pInfo, hMaskSmall);
            }
            else
            {
                ASSERT( pInfo->hIconSmall && pInfo->dwMask & ICIFLAG_ICON );

                HICON hIconSmall = AddImage_PrepareIcon(pInfo, pInfo->hIconSmall);
                ImageList_ReplaceIcon( m_himlSmall, iIndex, hIconSmall );
                AddImage_CleanupIcon(pInfo, hIconSmall);
            }
        }
    }
    else
    {
        UINT iListIndex = (UINT) -1;
        iIndex = DPA_AppendPtr( m_hListData, pNode );
        if ( iIndex >= 0 )
        {
            if ( pInfo->dwMask & ICIFLAG_BITMAP )
            {
                if ( pInfo->dwMask & ICIFLAG_LARGE )
                {
                    ASSERT( m_dwFlags & ICIIFLAG_LARGE );
                    ASSERT( pInfo->hBitmapLarge );

                    HBITMAP hBitmapLarge = AddImage_PrepareBitmap(pInfo, pInfo->hBitmapLarge);
                    HBITMAP hMaskLarge = AddImage_PrepareBitmap(pInfo, pInfo->hMaskLarge);
                    iListIndex  = ImageList_Add( m_himlLarge, hBitmapLarge, hMaskLarge);
                    AddImage_CleanupBitmap(pInfo, hBitmapLarge);
                    AddImage_CleanupBitmap(pInfo, hMaskLarge);
                }
                if ( pInfo->dwMask & ICIFLAG_SMALL )
                {
                    ASSERT( m_dwFlags & ICIIFLAG_SMALL );
                    ASSERT( pInfo->hBitmapSmall );

                    HBITMAP hBitmapSmall = AddImage_PrepareBitmap(pInfo, pInfo->hBitmapSmall);
                    HBITMAP hMaskSmall = AddImage_PrepareBitmap(pInfo, pInfo->hMaskSmall);
                    iListIndex  = ImageList_Add( m_himlSmall, hBitmapSmall, hMaskSmall);
                    AddImage_CleanupBitmap(pInfo, hBitmapSmall);
                    AddImage_CleanupBitmap(pInfo, hMaskSmall);
                }
            }
            else
            {
                ASSERT( pInfo->dwMask & ICIFLAG_ICON );
                if ( pInfo->dwMask & ICIFLAG_LARGE )
                {
                    ASSERT( m_dwFlags & ICIIFLAG_LARGE );
                    ASSERT( pInfo->hIconLarge );

                    HICON hIconLarge = AddImage_PrepareIcon(pInfo, pInfo->hIconLarge);
                    iListIndex = ImageList_AddIcon( m_himlLarge, hIconLarge );
                    AddImage_CleanupIcon(pInfo, hIconLarge);
                }
                if ( pInfo->dwMask & ICIFLAG_SMALL )
                {
                    ASSERT( m_dwFlags & ICIIFLAG_SMALL );
                    ASSERT( pInfo->hIconSmall );

                    HICON hIconSmall = AddImage_PrepareIcon(pInfo, pInfo->hIconSmall);
                    iListIndex = ImageList_AddIcon( m_himlSmall, hIconSmall );
                    AddImage_CleanupIcon(pInfo, hIconSmall);
                }
            }
            ASSERT( iListIndex == (UINT) iIndex );
        }
        else
        {
            // failed to add to the list...
            LocalFree( pNode );
        }
    }

    LeaveCriticalSection( &m_csLock );
    *puIndex = (UINT) iIndex;
    
    return (iIndex >= 0) ? NOERROR : E_OUTOFMEMORY;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache::FindImage ( THIS_ LPCIMAGECACHEINFO pInfo, UINT *puIndex )
{
    if ( !pInfo || !puIndex )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_FALSE;
    ASSERT( m_hListData );

    DWORD dwMatch = pInfo->dwMask & (ICIFLAG_FLAGS | ICIFLAG_NAME | ICIFLAG_INDEX | ICIFLAG_DATESTAMP );
    DWORD dwMask;
    int iIndex = 0;
    ICONCACHE_DATA * pNode = NULL;
    EnterCriticalSection( &m_csLock );
    do
    {
        dwMask = 0;
        pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, iIndex );
        if ( !pNode )
        {
            break;
        }

        if ( pNode->iUsage == (UINT) -2 )
        {
            iIndex ++;
            continue;
        }
        
        if (( dwMatch & ICIFLAG_NAME ) && StrCmpW( pInfo->pszName, pNode->szName ) == 0 )
        {
            // found it
            dwMask |= ICIFLAG_NAME;
        }
        if (( dwMatch& ICIFLAG_INDEX ) && pInfo->iIndex == pNode->iIndex )
        {
            dwMask |= ICIFLAG_INDEX;
        }
        if (( dwMatch & ICIFLAG_FLAGS ) && pInfo->dwFlags == pNode->dwFlags )
        {
            dwMask |= ICIFLAG_FLAGS;
        }

        if (( dwMatch & ICIFLAG_DATESTAMP ) && ( pInfo->dwFlags & ICIFLAG_DATESTAMP ) &&
            ( pInfo->ftDateStamp.dwLowDateTime == pNode->ftDateStamp.dwLowDateTime &&
              pInfo->ftDateStamp.dwHighDateTime == pNode->ftDateStamp.dwHighDateTime ))
        {
            dwMask |= ICIFLAG_DATESTAMP;
        }

        iIndex ++;
    }
    while ( dwMask != dwMatch);

    // found it, save the index... (as long as it was freed not deleted...
    if ( dwMask == dwMatch )
    {
        if ( !(pInfo->dwMask & ICIFLAG_NOUSAGE ) && ( pNode->iUsage != (UINT) -1 ))
        {
            // bump the usage count...
            pNode->iUsage ++;
        }
        *puIndex = (UINT) (iIndex - 1);
        
        hr = NOERROR;
    }
    
    LeaveCriticalSection( &m_csLock );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache::Flush( BOOL fRelease)
{
    ASSERT( m_hListData );

    EnterCriticalSection( &m_csLock );
    if ( fRelease )
    {
        // simply empty the data list. The ImageList never shrinks...
        DPA_EnumCallback( m_hListData, DestroyEnum, NULL );
        DPA_DeleteAllPtrs( m_hListData );

        if ( m_himlLarge )
        {
            ImageList_RemoveAll( m_himlLarge );
        }

        if ( m_himlSmall )
        {
            ImageList_RemoveAll( m_himlSmall );
        }
    }
    else
    {
        DPA_EnumCallback( m_hListData, UsageEnum, 0 );
    }
    LeaveCriticalSection( &m_csLock );

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: FreeImage ( THIS_ UINT uImageIndex )
{
    ASSERT ( m_hListData );

    EnterCriticalSection( &m_csLock );
    
    // make sure we are in range...
    if ( uImageIndex >= (UINT) DPA_GetPtrCount( m_hListData ))
    {
        LeaveCriticalSection( &m_csLock );
        return E_INVALIDARG;
    }

    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, uImageIndex );
    ASSERT( pNode );

    HRESULT hr = ( pNode->iUsage > 0 ) ? NOERROR : S_FALSE;
    if ( hr == NOERROR && (pNode->iUsage != (UINT) -1 ) && (pNode->iUsage != (UINT) -2 ))
    {
        pNode->iUsage --;
    }
    LeaveCriticalSection( &m_csLock );
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: DeleteImage( THIS_ UINT uImageIndex )
{
    ASSERT ( m_hListData );

    EnterCriticalSection( &m_csLock );
    
    // make sure we are in range...
    if ( uImageIndex >= (UINT) DPA_GetPtrCount( m_hListData ))
    {
        LeaveCriticalSection( &m_csLock );
        return E_INVALIDARG;
    }

    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, uImageIndex );
    ASSERT( pNode );

    HRESULT hr = ( pNode->iUsage > 0 ) ? NOERROR : S_FALSE;
    if ( hr == NOERROR && (pNode->iUsage != (UINT) -2 ))
    {
        pNode->iUsage = (UINT) -2;
    }
    LeaveCriticalSection( &m_csLock );
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: ChangeImageInfo ( THIS_ UINT uImageIndex, LPCIMAGECACHEINFO pInfo )
{
    if ( !pInfo )
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_csLock );
    if ( uImageIndex >= (UINT) DPA_GetPtrCount( m_hListData ))
    {
        LeaveCriticalSection( &m_csLock );
        return E_INVALIDARG;
    }
    
    ICONCACHE_DATA * pNode = CreateDataNode( pInfo );
    if ( !pInfo )
    {
        LeaveCriticalSection( &m_csLock );
        return E_OUTOFMEMORY;
    }

    ASSERT( m_hListData );
    ICONCACHE_DATA * pOld = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, uImageIndex );
    ASSERT( pOld );
    DPA_SetPtr( m_hListData, uImageIndex, pNode );
    pNode->iUsage = pOld->iUsage;
    LocalFree( pOld );

    if ( pInfo->dwMask & ( ICIFLAG_BITMAP | ICIFLAG_ICON ))
    {
        // update the picture....
        if ( pInfo->dwMask & ICIFLAG_LARGE )
        {
            if ( pInfo->dwMask & ICIFLAG_BITMAP )
            {
                ASSERT( pInfo->hBitmapLarge );
                ImageList_Replace( m_himlLarge, uImageIndex, pInfo->hBitmapLarge, pInfo->hMaskLarge );
            }
            else
            {
                ASSERT( pInfo->hIconLarge && pInfo->dwMask & ICIFLAG_ICON );
                ImageList_ReplaceIcon( m_himlLarge, uImageIndex, pInfo->hIconLarge );
            }
        }
        if ( pInfo->dwMask & ICIFLAG_SMALL )
        {
            if ( pInfo->dwMask & ICIFLAG_BITMAP )
            {
                ASSERT( pInfo->hBitmapSmall );
                ImageList_Replace( m_himlSmall, uImageIndex, pInfo->hBitmapSmall, pInfo->hMaskSmall );
            }
            else
            {
                ASSERT( pInfo->hIconSmall && pInfo->dwMask & ICIFLAG_ICON );
                ImageList_ReplaceIcon( m_himlLarge, uImageIndex, pInfo->hIconSmall );
            }
        }
    }
    LeaveCriticalSection(&m_csLock );
    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CImageListCache::CountFreeSlots( )
{
    // walk the list looking for free slots (those with ref 0)
    int iSlot = 0;
    UINT uFree = 0;

    ASSERT( m_hListData );
    
    do
    {
        ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, iSlot ++ );
        if ( !pNode )
        {
            break;
        }

        if ( pNode->iUsage == 0 || pNode->iUsage == (UINT) -2 )
        {
            uFree ++;
        }
    }
    while ( TRUE );

    return uFree;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: GetCacheSize ( THIS_ UINT * puSize )
{
    if ( !puSize )
    {
        return E_INVALIDARG;
    }

    ASSERT( m_hListData );

    EnterCriticalSection( &m_csLock );
    *puSize = DPA_GetPtrCount( m_hListData ) - CountFreeSlots();
    LeaveCriticalSection( &m_csLock );
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache:: GetImageList ( THIS_ LPIMAGECACHEINITINFO pInfo )
{
    if ( !pInfo )
    {
        return E_INVALIDARG;
    }

    ASSERT( pInfo->cbSize == sizeof( IMAGECACHEINITINFO ));

    if ( !(pInfo->dwMask & (ICIIFLAG_LARGE | ICIIFLAG_SMALL)))
    {
        // must specify one or both of large or small
        return E_INVALIDARG;
    }

    if ( m_hListData )
    {
        // we have already been created, just pass back the info if they match.....
        if ((( pInfo->dwMask & ICIIFLAG_SMALL ) && !m_himlSmall ) ||
            (( pInfo->dwMask & ICIIFLAG_LARGE ) && !m_himlLarge ) ||
            ( m_dwFlags != pInfo->dwMask ))
        {
            return E_INVALIDARG;
        }

        if ( pInfo->dwMask & ICIIFLAG_SMALL )
        {
            pInfo->himlSmall = m_himlSmall;
        }
        if ( pInfo->dwMask & ICIIFLAG_LARGE )
        {
            pInfo->himlLarge = m_himlLarge;
        }

        return S_FALSE;
    }
    
    m_hListData = DPA_Create( 30 );
    if ( !m_hListData )
    {
        return E_OUTOFMEMORY;
    }
    
    if ( pInfo->dwMask & ICIIFLAG_LARGE )
    {
        m_himlLarge = ImageList_Create( pInfo->rgSizeLarge.cx, pInfo->rgSizeLarge.cy, pInfo->dwFlags,
            pInfo->iStart, pInfo->iGrow );
        if ( !m_himlLarge )
        {
            return E_OUTOFMEMORY;
        }
        pInfo->himlLarge = m_himlLarge;
    }
    if ( pInfo->dwMask & ICIIFLAG_SMALL )
    {
        m_himlSmall = ImageList_Create( pInfo->rgSizeSmall.cx, pInfo->rgSizeSmall.cy, pInfo->dwFlags,
            pInfo->iStart, pInfo->iGrow );
        if ( !m_himlSmall )
        {
            return E_OUTOFMEMORY;
        }
        pInfo->himlSmall = m_himlSmall;
    }

    m_dwFlags = pInfo->dwMask;
    
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////
int CImageListCache::FindEmptySlot()
{
    // search for an element with a zero usage count...
    ASSERT( m_hListData );
    
    int iIndex = 0;
    do
    {
        ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, iIndex );
        if ( !pNode )
        {
            break;
        }

        if ( pNode->iUsage == 0 || pNode->iUsage == -2 )
        {
            return iIndex;
        }
        iIndex ++;
    } while (TRUE);
    
    return  -1;
}

////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache::GetUsage ( UINT uIndex, UINT * puUsage )
{
    ASSERT( m_hListData );
    if ( !puUsage || uIndex >= (UINT) DPA_GetPtrCount( m_hListData ))
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_csLock );
    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, uIndex );
    *puUsage = (UINT) pNode-> iUsage;
    LeaveCriticalSection( &m_csLock );
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImageListCache::GetImageInfo( UINT uImageIndex, LPIMAGECACHEINFO pInfo )
{
    if ( !pInfo )
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_csLock );
    if ( uImageIndex >= (UINT) DPA_GetPtrCount( m_hListData ))
    {
        LeaveCriticalSection( &m_csLock );
        return E_INVALIDARG;
    }
    
    ASSERT( m_hListData );
    ICONCACHE_DATA * pNode = (ICONCACHE_DATA *) DPA_GetPtr( m_hListData, uImageIndex );
    ASSERT( pNode );

    HRESULT hRes = E_NOTIMPL;
    
    if ( pInfo->dwMask & ICIFLAG_DATESTAMP )
    {
        if ( pNode->dwFlags & ICIFLAG_DATESTAMP )
        {
            pInfo->ftDateStamp = pNode->ftDateStamp;
            hRes = NOERROR;
        }
        else
        {
            hRes = E_FAIL;
        }
    }

    if ( pInfo->dwMask & ICIFLAG_NOUSAGE )
    {
        hRes = NOERROR;
    }
    pInfo->dwMask = pNode->dwFlags & pInfo->dwMask;
    
    LeaveCriticalSection(&m_csLock );
    return hRes;
}
