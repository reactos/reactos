#include "shellprv.h"
#include <runtask.h>
#include "sfviewp.h"
#include <runtask.h>

// from defview.cpp
void ChangeRefForIdle(CDefView * pdsv, BOOL bAdd);
int CALLBACK DefView_Compare(LPARAM p1, LPARAM p2, LPARAM lParam);
STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage);

#define TF_ICON TF_DEFVIEW


/////////////////////////////////////////////////////////////////////////////////////
CDVBkgrndEnumTask::CDVBkgrndEnumTask( HRESULT * pHr, 
                                      CDefView * pdsv, 
                                      IEnumIDList *peunk, 
                                      HDPA hdpaNew, 
                                      BOOL fRefresh )
   : CRunnableTask( RTF_SUPPORTKILLSUSPEND )
{
    ASSERT( pHr );
    *pHr = NOERROR;


    _pdsv = pdsv;
    if ( _pdsv )
        ChangeRefForIdle(_pdsv, TRUE);
    else
        *pHr = E_INVALIDARG;

    _peunk = peunk;
    if ( _peunk )
        _peunk->AddRef();
    else
        *pHr = E_INVALIDARG;
        
    _hdpaNew = hdpaNew;
    _fRefresh = fRefresh;

}

/////////////////////////////////////////////////////////////////////////////////////
CDVBkgrndEnumTask::~CDVBkgrndEnumTask()
{
    IUnknown_SetSite( _peunk, NULL);      // Break the site back pointer.
    
    ATOMICRELEASE( _peunk );

    if ( _hdpaNew )
    {
        // empty the DPA first of all the pidls...
        for ( int iPtr = 0; iPtr < DPA_GetPtrCount( _hdpaNew ); iPtr ++ )
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST) DPA_GetPtr( _hdpaNew, iPtr );
            ASSERT( pidl );
            ILFree( pidl );
        }
        DPA_DeleteAllPtrs( _hdpaNew );
        DPA_Destroy( _hdpaNew );
    }

    if ( _pdsv )
        ChangeRefForIdle( _pdsv, FALSE );
}

/////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVBkgrndEnumTask::RunInitRT( )
{
    // nothing needed to init ...
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVBkgrndEnumTask::InternalResumeRT( )
{
    LPITEMIDLIST pidl = NULL;   // just in case
    ULONG celt;

    while ( _peunk->Next( 1, &pidl, &celt) == S_OK)
    {
        // do we need to quit, 
        if ( DPA_AppendPtr(_hdpaNew, pidl) == -1 )
        {
            SHFree(pidl);
        }

        // we were told to either suspend or quit...
        if ( WaitForSingleObject( _hDone, 0 ) == WAIT_OBJECT_0 )
        {
            // return a different error code if we are beign killed...
            if ( _lState != IRTIR_TASK_SUSPENDED )
            {
                _pdsv->_bBkFilling = FALSE;
            }
            return ( _lState == IRTIR_TASK_SUSPENDED ) ? E_PENDING : E_FAIL;
        }

        pidl = NULL;
    }

    _pdsv->_bBkFilling = FALSE;

    HWND hwndView;

    // Sort on this thread so we do not hang the main thread for as long
    DPA_Sort( _hdpaNew, _pdsv->_GetCompareFunction(), (LPARAM)_pdsv );

    // Tell the main thread to merge the items
    hwndView = _pdsv->_hwndView;
    ASSERT( IsWindow( hwndView ));
    if (PostMessage(hwndView, WM_DSV_DESTROYSTATIC,
                                _fRefresh, (LPARAM)_hdpaNew))
    {
        // If hwndView is destroyed before receiving this message, we could
        // have a memory leak.  Oh well.
        _hdpaNew = NULL;
    }

    // notify DefView that we're done
    // (NOTE: We can not call the callback directly from here. This causes
    // a gp fault in RecalcIdealSize when we restart after setup (if we hit
    // a small timing window when pdvoi becomes NULL from underneath us.
    // So, we post a message here and while processing it, we will do the
    // actual callback.
    PostMessage( _pdsv->_hwndView, WM_DSV_BACKGROUNDENUMDONE, 0, 0);

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////
HRESULT CDVBkgrndEnumTask_CreateInstance( CDefView * pdsv, IEnumIDList *peunk,
            HDPA hdpaNew, BOOL fRefresh, LPRUNNABLETASK *ppTask )
{
    HRESULT hr = NOERROR;

    *ppTask = NULL;
    
    CDVBkgrndEnumTask * pNewTask = new CDVBkgrndEnumTask( &hr, pdsv, peunk, hdpaNew, fRefresh );
    if ( pNewTask )
    {
        if ( SUCCEEDED( hr ))
        {
            *ppTask = SAFECAST( pNewTask, IRunnableTask *);
        }

        if ( FAILED( hr ))
        {
            delete pNewTask;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CDVGetIconTask::CDVGetIconTask( HRESULT * pHr, 
                                LPCITEMIDLIST pidl,
                                CDefView * pdsv )
    : CRunnableTask( RTF_DEFAULT )

{
    // assume zero init
    ASSERT( !_pdsv && !_pidl);
    ASSERT( pHr );
    if ( pdsv && pidl)
    {
        _pidl = ILClone( pidl );
        if ( !_pidl )
        {
            *pHr = E_OUTOFMEMORY;
            return;
        }
        
        _pdsv = pdsv;
        ChangeRefForIdle(_pdsv, TRUE);
    }
    else
    {
        *pHr = E_INVALIDARG;
    }
}

/////////////////////////////////////////////////////////////////////////////////////
CDVGetIconTask::~CDVGetIconTask()
{
    if ( _pdsv )
    {
        ChangeRefForIdle( _pdsv, FALSE );
    }
    if ( _pidl )
    {
        ILFree( _pidl );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVGetIconTask::RunInitRT( )
{
    BOOL fPosted = FALSE;

    ASSERT(_pidl);
    ASSERT( IsWindow( _pdsv->_hwndView ));

    _iIcon = -1;

    //
    // get the icon for this item.
    //
    SHGetIconFromPIDL(_pdsv->_pshf, _pdsv->_psi, _pidl, 0, &_iIcon);

    if (_iIcon != -1)
    {
        //
        //  now post the result back to the main thread
        //
        
        // bump the ref count, it will get released from the reciever...
        this->AddRef();
            
        if (PostMessage(_pdsv->_hwndView, WM_DSV_UPDATEICON, 0, (LPARAM) this))
        {
            if (_pdsv->_AsyncIconEvent)
                SetEvent(_pdsv->_AsyncIconEvent);

            fPosted = TRUE;
        }
        else
        {
            this->Release();
        }
    }

    //
    // Clean up everything ourselves if we didn't post anything.
    //
    if (!fPosted )
    {
        InterlockedDecrement(&_pdsv->_AsyncIconCount);
        
        DebugMsg(TF_ICON, TEXT("async icon CANCELED: pidl=%08X count=%d"), _pidl, _pdsv->_AsyncIconCount);
    }
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////
HRESULT CDVGetIconTask_CreateInstance( CDefView * pdsv, LPCITEMIDLIST pidl, LPRUNNABLETASK *ppTask, CDVGetIconTask ** ppObject )
{
    HRESULT hr = NOERROR;

    *ppObject = NULL;
    
    CDVGetIconTask * pNewTask = new CDVGetIconTask( &hr, pidl, pdsv);
    if(pNewTask)
    {
        if(SUCCEEDED(hr))
        {
            *ppTask = SAFECAST( pNewTask, IRunnableTask *);
            if ( ppObject )
                *ppObject = pNewTask;
        }
        else
            delete pNewTask;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

CDVExtendedColumnTask::CDVExtendedColumnTask(CDefView * pdsv, LPCITEMIDLIST pidl, int fmt, UINT uiColumn)
    : CRunnableTask(RTF_DEFAULT),
    _pdsv(pdsv),
    _pidl(ILClone(pidl)),
    _fmt(fmt),
    _uiCol(uiColumn)
{
    _pdsv->AddRef();
}

CDVExtendedColumnTask::~CDVExtendedColumnTask()
{
    ILFree(const_cast<LPITEMIDLIST>(_pidl));
    ATOMICRELEASE(_pdsv);
}

STDMETHODIMP CDVExtendedColumnTask::RunInitRT(void)
{
    DETAILSINFO di;
    
    di.pidl = _pidl;
    di.fmt = _fmt;

    HRESULT hr = _pdsv->_GetDetailsHelper(_uiCol, &di);
    if (SUCCEEDED(hr))
    {
        CBackgroundColInfo  *pbgci;

        pbgci = new CBackgroundColInfo(_pidl, _uiCol, di.str);
        if (pbgci == NULL)
            return E_OUTOFMEMORY;

        _pidl = NULL;        // ILFree checks for null

        //TraceMsg(TF_DEFVIEW, "DVECTask::RunInitRT- got text %s for %d", pbgci->szText, _iCol);

        if (!PostMessage(_pdsv->_hwndView, WM_DSV_UPDATECOLDATA, 0, (LPARAM)pbgci))
            delete pbgci;
    }
    return hr;

}

HRESULT CDVExtendedColumnTask_CreateInstance(CDefView * pdsv, LPCITEMIDLIST pidl, int fmt, UINT uiColumn, IRunnableTask **ppTask)
{
    CDVExtendedColumnTask *pDVECTask = new CDVExtendedColumnTask(pdsv, pidl, fmt, uiColumn);
    if (!pDVECTask)
        return E_OUTOFMEMORY;

    if (ppTask)
        *ppTask = SAFECAST(pDVECTask, IRunnableTask*);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CDVIconOverlayTask::CDVIconOverlayTask( HRESULT * pHr, 
                                LPCITEMIDLIST pidl,
                                int iList, 
                                CDefView * pdsv )
    : CRunnableTask( RTF_DEFAULT ), _iList(iList)

{
    // assume zero init
    ASSERT( !_pdsv && !_pidl);
    ASSERT( pHr );
    if ( pdsv && pidl)
    {
        _pidl = ILClone( pidl );
        if ( !_pidl )
        {
            *pHr = E_OUTOFMEMORY;
            return;
        }
        
        _pdsv = pdsv;
        ChangeRefForIdle(_pdsv, TRUE);
    }
    else
    {
        *pHr = E_INVALIDARG;
    }
}

/////////////////////////////////////////////////////////////////////////////////////
CDVIconOverlayTask::~CDVIconOverlayTask()
{
    if ( _pdsv )
    {
        ChangeRefForIdle( _pdsv, FALSE );
    }
    if ( _pidl )
    {
        ILFree( _pidl );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVIconOverlayTask::RunInitRT( )
{
    ASSERT(_pidl);
    ASSERT( IsWindow( _pdsv->_hwndView ));
    ASSERT(IS_VALID_CODE_PTR(_pdsv->_psio, IShellIconOverlay));
    
    int iOverlay = 0;
    
    //
    // get the overlay index for this item.
    //
    _pdsv->_psio->GetOverlayIndex(_pidl, &iOverlay);

    if (iOverlay > 0)
    {
        //
        //  now post the result back to the main thread
        //
        PostMessage(_pdsv->_hwndView, WM_DSV_UPDATEOVERLAY, (WPARAM)_iList, (LPARAM)iOverlay);
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////
HRESULT CDVIconOverlayTask_CreateInstance( CDefView * pdsv, LPCITEMIDLIST pidl, int iList, LPRUNNABLETASK *ppTask)
{
    HRESULT hr = NOERROR;

    *ppTask = NULL;
    
    CDVIconOverlayTask * pNewTask = new CDVIconOverlayTask( &hr, pidl, iList, pdsv);
    if(pNewTask)
    {
        if(SUCCEEDED(hr))
            *ppTask = SAFECAST( pNewTask, IRunnableTask *);
        else
        {
            delete pNewTask;
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}
