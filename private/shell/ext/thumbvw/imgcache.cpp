#include "precomp.h"

WCHAR const c_szDefault[] = L"Default";

////////////////////////////////////////////////////////////////////////////////////
HRESULT CImgCacheTidyup_Create( IImageCache * pCache, 
                                BOOL fMultiple,
                                HWND hWndListView,
                                int * piItemPos,
                               LPRUNNABLETASK * ppTask )
{
    if ( !ppTask || !pCache )
    {
        return E_INVALIDARG;
    }

    CImgCacheTidyup *pTask = new CComObject<CImgCacheTidyup>;
    if ( pTask == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // not ref-counted as it is not a COM object...
    pTask->m_pCache = pCache;
    pTask->m_fMultiple = fMultiple;
    pTask->m_piItemPos = piItemPos;
    pTask->m_hWndListView = hWndListView;
    pCache->AddRef();
    pTask->AddRef();

    *ppTask = (LPRUNNABLETASK) pTask;
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImgCacheTidyup::Run( )
{
    if ( m_lState == IRTIR_TASK_FINISHED || m_lState == IRTIR_TASK_RUNNING )
    {
        return S_FALSE;
    }
    if ( m_lState == IRTIR_TASK_PENDING )
    {
        return E_FAIL;
    }

    LONG lRes = InterlockedExchange( & m_lState, IRTIR_TASK_RUNNING );
    if ( lRes == IRTIR_TASK_PENDING )
    {
        m_lState = IRTIR_TASK_FINISHED;
        return NOERROR;
    }

    BOOL fReset = FALSE;
    int iItem = *m_piItemPos;
    do
    {
        iItem = ListView_GetNextItem( m_hWndListView, iItem, LVNI_ALL);
        if ( iItem == -1 )
        {
            if ( fReset == FALSE && *m_piItemPos != -1)
            {
                fReset = TRUE;
                continue;
            }
            else
            {
                break;
            }
        }

        LV_ITEMW rgItem;
        ZeroMemory( &rgItem, sizeof(rgItem));
        rgItem.iItem = iItem;
        rgItem.mask = LVIF_IMAGE | LVIF_NORECOMPUTE;

        ListView_GetItemWrapW( m_hWndListView, &rgItem);
        
        UINT uUsage;
        if ( SUCCEEDED( m_pCache->GetUsage( rgItem.iImage, &uUsage )))
        {
            // single usage....
            if ( uUsage == 1 )
            {
                // check to see if it is visible...
                if (!ImageIsInView( iItem ))
                {
                    UINT uImage = rgItem.iImage;
                    
                    // free it then ....
                    rgItem.mask = LVIF_IMAGE;
                    rgItem.iImage = I_IMAGECALLBACK;
                    rgItem.iItem = iItem;

                    ListView_SetItemWrapW( m_hWndListView, &rgItem);

                    // free it in the cache...
                    m_pCache->FreeImage( uImage );

                    *m_piItemPos = iItem;
                    
                    if ( !m_fMultiple )
                    {
                        break;
                    }
                }
            }
        }
    } while ( TRUE );
    
    m_lState = IRTIR_TASK_FINISHED;
    
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImgCacheTidyup::Kill( BOOL fWait )
{
    if ( fWait == TRUE )
    {
        return E_NOTIMPL;
    }

    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        LONG lRes = InterlockedExchange( & m_lState, IRTIR_TASK_PENDING);
        if ( lRes == IRTIR_TASK_FINISHED )
        {
            m_lState = lRes;
            return NOERROR;
        }
        return E_PENDING;
    }
    else
    {
        return S_FALSE;
    }

    return E_PENDING;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImgCacheTidyup::Suspend( void )
{
    // not supported....
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImgCacheTidyup::Resume( void )
{
    // not supported....
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImgCacheTidyup::IsRunning()
{
    return (ULONG) m_lState;
}

////////////////////////////////////////////////////////////////////////////////////
CImgCacheTidyup::CImgCacheTidyup()
{
    m_lState = IRTIR_TASK_NOT_RUNNING;
    m_pCache = NULL;
}

////////////////////////////////////////////////////////////////////////////////////
CImgCacheTidyup::~CImgCacheTidyup()
{
    if ( m_pCache )
    {
        m_pCache->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////
BOOL CImgCacheTidyup::ImageIsInView( int iIndex )
{
    RECT rectListView, rectItem;
    GetClientRect( m_hWndListView, &rectListView );

    // get the view and item dimensions.
    ListView_GetItemRect( m_hWndListView, iIndex, &rectItem, LVIR_ICON );
    
    // check if they overlap.
    return !(( rectItem.left >= rectListView.right ) || 
             ( rectItem.right <= rectListView.left ) ||
             ( rectItem.top >= rectListView.bottom ) ||
             ( rectItem.bottom <= rectListView.top ));
  }


