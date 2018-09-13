//-------------------------------------------------------------------------//
// DefSrv32.cpp : Implementation of CPTDefSrv32
//-------------------------------------------------------------------------//

#include "pch.h"
#include "PTsrv32.h"
#include "DefSrv32.h"
#include "enum.h"
#include "PTutil.h"
#include "MruProp.h"
#include "PTsniff.h"


//-------------------------------------------------------------------------//
//  class CPTDefSrv32 - primary server object
//-------------------------------------------------------------------------//

//-----------------------------//
//  ISupportErrorInfo methods
//-----------------------------//

//-------------------------------------------------------------------------//
//  ISupportErrorInfo::InterfaceSupportsErrorInfo
STDMETHODIMP CPTDefSrv32::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IAdvancedPropertyServer,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

//-------------------------------------------------------------------------//
BOOL GetBasicServerForFile( LPCWSTR pwszFile, IBasicPropertyServer** ppbps )
{
    CLSID clsid;
    USES_CONVERSION;
    *ppbps = NULL;

    if( SUCCEEDED( GetPropServerClassForFile( W2T((LPWSTR)pwszFile), FALSE, &clsid ) ) )
    {
        if( SUCCEEDED(CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IBasicPropertyServer, (void**)ppbps )) )
            return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPTDefSrv32::AcquireBasic( 
    IN const VARIANT* pvarSrc, 
    IN OUT BASICPROPITEM rgItems[], 
    IN LONG cItems )
{
    LONG        i, iStart, cSpecialized = 0;
    const DWORD grfMode =  STGM_DIRECT|STGM_READ|STGM_SHARE_EXCLUSIVE;
    HRESULT     hrSpecialized = E_FAIL, hrThis = E_FAIL;

    if( NULL == rgItems || 0 == cItems || 
        !(pvarSrc && pvarSrc->vt == VT_BSTR && pvarSrc->bstrVal && *pvarSrc->bstrVal) )
        return E_INVALIDARG;

    //  Delegate to 'specialized' property server.
    IBasicPropertyServer* pbpsSpecialized = NULL;
    CLSID clsid;
    if( GetBasicServerForFile( pvarSrc->bstrVal, &pbpsSpecialized ) )
    {
        hrSpecialized = pbpsSpecialized->AcquireBasic( pvarSrc, rgItems, cItems );
        if( SUCCEEDED( hrSpecialized ) )
        {
            for( i = 0; i < cItems; i++ )
            {
                if( VT_EMPTY != rgItems[i].val.vt )
                    cSpecialized++;
            }
        }
        pbpsSpecialized->Release();
    }

    //  If we need to supplement or replace the specialized server...
    //  supplement w/ image metadata properties
    if( cSpecialized < cItems )
        cSpecialized += AcquireBasicImageProperties( pvarSrc->bstrVal, rgItems, cItems );
        
    //  supplement w/ NSS properties
    if( cSpecialized < cItems )
    {
        IPropertySetStorage* ppss;
        hrThis = SHStgOpenStorage( pvarSrc->bstrVal, grfMode, 0, 0,
                                   IID_IPropertySetStorage, (LPVOID*)&ppss );
        if( SUCCEEDED( hrThis ) )
        {
            //  Alloc propspecs
            PROPSPEC*    rgSpec     = NULL;
            PROPVARIANT* rgVal      = NULL;
            FMTID*       rgFmtid    = NULL;
            
            if( (rgSpec  = new PROPSPEC[cItems - cSpecialized]) != NULL &&
                (rgVal   = new PROPVARIANT[cItems - cSpecialized]) != NULL && 
                (rgFmtid = new FMTID[cItems - cSpecialized]) != NULL )
            {
                int     cAcquire, cAcquired;     

                //  acquire only the properties we need to:
                for( i = cAcquire = 0; i < cItems; i++ )
                {
                    if( VT_EMPTY == rgItems[i].val.vt )
                    {
                        rgFmtid[cAcquire]       = rgItems[i].puid.fmtid;
                        rgSpec[cAcquire].ulKind = PRSPEC_PROPID;
                        rgSpec[cAcquire].propid = rgItems[i].puid.propid;
                        PropVariantInit( &rgVal[cAcquire] );
                        cAcquire++;
                    }
                }

                for( i = iStart = 0; i < cAcquire; i++ )
                {
                    BOOL bLastOfSet = (i == cAcquire - 1) || !IsEqualGUID( rgFmtid[i], rgFmtid[i+1] );

                    if( bLastOfSet )
                    {
                        HRESULT hrSet;
                        IPropertyStorage* pps;
                        UINT    uCodePage = 0;
                        if( SUCCEEDED( (hrSet = SHPropStgCreate( ppss, rgFmtid[i], NULL, PROPSETFLAG_DEFAULT, 
                                                                 grfMode, OPEN_EXISTING, &pps, &uCodePage )) ) )
                        {
                            hrSet = SHPropStgReadMultiple( pps, uCodePage, (i - iStart) + 1, rgSpec + iStart, rgVal + iStart );
                            pps->Release();
                        }
                        if( FAILED( hrSet ) && hrSet != STG_E_FILENOTFOUND /*an empty set is not an error*/  )
                            hrThis = hrSet;

                        iStart = i + 1;
                    }
                }

                //  Copy out values
                if( SUCCEEDED( hrThis ) )
                {
                    for( i = cAcquired = 0; i < cItems; i++ )
                    {
                        if( VT_EMPTY == rgItems[i].val.vt )
                        {
                            ASSERT( cAcquired < cAcquire );
                            PropVariantCopy( &rgItems[i].val, &rgVal[cAcquired] );
                            if( rgVal[cAcquired].vt != VT_EMPTY )
                                rgItems[i].bDirty = FALSE;
                            PropVariantClear( &rgVal[cAcquired] );
                            cAcquired++;
                        }
                    }
                }
            }
            else
                hrThis = E_OUTOFMEMORY;

            delete [] rgVal;
            delete [] rgSpec;
            delete [] rgFmtid;
            ppss->Release();
        }
    }
    else
        hrThis = hrSpecialized;
    
    return hrThis;
}

//-------------------------------------------------------------------------//
//  Supplements basic property values w/ read-only image metadata properties
int CPTDefSrv32::AcquireBasicImageProperties( IN LPCWSTR pwszSrc, IN OUT BASICPROPITEM rgItems[], IN LONG cItems )
{
    ULONG           cIflProps = 0,
                    cAcquired = 0;
#if defined(_X86_) 
    BOOL            bAcquired = FALSE;
    IFLPROPERTIES   iflprop;

    InitIflProperties( &iflprop );
    iflprop.mask = IPF_TEXT;

    //  Scan for empty SummaryInformation properties that we can fill in:
    for( int i=0; i< cItems; i++ )
    {
        if( VT_EMPTY == rgItems[i].val.vt &&
            IsEqualGUID( rgItems[i].puid.fmtid, FMTID_SummaryInformation ) )
        {
            //  Acquire image properties only if we have to...
            if( !bAcquired )
            {
                if( SUCCEEDED( AcquireImageProperties( pwszSrc, &iflprop, &cIflProps ) ) &&
                    (iflprop.mask & IPF_TEXT) != 0 )
                    bAcquired = TRUE;
                else
                    return 0;
            }

            LPWSTR pwszVal = NULL;
            switch( rgItems[i].puid.propid )
            {
                case PIDSI_TITLE:
                    pwszVal = (iflprop.textmask & ITPF_TITLE) ? iflprop.szTitle : NULL;
                    break;
                    
                case PIDSI_AUTHOR:
                    pwszVal = (iflprop.textmask & ITPF_AUTHOR) ? iflprop.szAuthor : NULL;
                    break;

                case PIDSI_COMMENTS:
                    pwszVal = (iflprop.textmask & ITPF_COMMENTS) ? iflprop.szComments : NULL;
                    break;

                case PIDSI_SUBJECT:
                    pwszVal = (iflprop.textmask & ITPF_DESCRIPTION) ? iflprop.szDescription : NULL;
                    break;
            }

            if( pwszVal )
            {
                //  Got one...
                rgItems[i].val.pwszVal = (LPWSTR)CoTaskMemAlloc( (lstrlenW( pwszVal ) + 1) * sizeof(WCHAR) );
                if( rgItems[i].val.pwszVal )
                {
                    lstrcpyW( rgItems[i].val.pwszVal, pwszVal );
                    rgItems[i].val.vt = VT_LPWSTR;
                    rgItems[i].dwAccess = PTIA_READONLY;   // specify read-only access
                    cAcquired++;
                }
            }
        }
    }
#endif _X86_

    return (int)cAcquired;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPTDefSrv32::PersistBasic( 
    IN const VARIANT* pvarSrc,
    IN OUT BASICPROPITEM rgItems[], 
    IN LONG cItems )
{
    LONG        i, iStart, cSpecialized = 0;
    HRESULT     hrSpecialized = E_NOINTERFACE, hrThis = E_FAIL;
    const DWORD grfMode =  STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE;

    if( NULL == rgItems || 0 == cItems || 
        !(pvarSrc && pvarSrc->vt == VT_BSTR && pvarSrc->bstrVal && *pvarSrc->bstrVal) )
        return E_INVALIDARG;
    
    //  Delegate to 'specialized' property server.
    IBasicPropertyServer* pbpsSpecialized = NULL;
    CLSID clsid;
    if( GetBasicServerForFile( pvarSrc->bstrVal, &pbpsSpecialized ) )
    {
        hrSpecialized = pbpsSpecialized->PersistBasic( pvarSrc, rgItems, cItems );
        if( SUCCEEDED( hrSpecialized ) )
        {
            for( i = 0; i < cItems; i++ )
            {
                if( rgItems[i].bDirty )
                    cSpecialized++;
            }
        }
        pbpsSpecialized->Release();
    }

    if( cSpecialized < cItems )
    {
        IPropertySetStorage* ppss;
        hrThis = SHStgOpenStorage( pvarSrc->bstrVal, grfMode, 0, 0, 
                                   IID_IPropertySetStorage, (LPVOID*)&ppss );
        if( SUCCEEDED( hrThis ) )
        {
            //  Alloc propspecs
            PROPSPEC*    rgSpec = NULL;
            PROPVARIANT* rgVal = NULL;
            FMTID*       rgFmtid = NULL;

            if( (rgVal = new PROPVARIANT[cItems - cSpecialized]) != NULL &&
                (rgSpec = new PROPSPEC[cItems - cSpecialized]) != NULL &&
                (rgFmtid = new FMTID[cItems - cSpecialized]) != NULL )
            {
                int  cPersist, cPersisted;     

                //  persist only the properties we need to:
                for( i = cPersist = 0; i < cItems; i++ )
                {
                    if( rgItems[i].bDirty )
                    {
                        rgFmtid[cPersist]       = rgItems[i].puid.fmtid;
                        rgSpec[cPersist].ulKind = PRSPEC_PROPID;
                        rgSpec[cPersist].propid = rgItems[i].puid.propid;
                        PropVariantInit( &rgVal[cPersist] );
                        PropVariantCopy( &rgVal[cPersist], &rgItems[i].val );
                        cPersist++;
                    }
                }

                for( i = iStart = 0; i< cPersist; i++ )
                {
                    BOOL bLastOfSet = (i == cPersist - 1) || !IsEqualGUID( rgFmtid[i], rgFmtid[i+1] );
            
                    if( bLastOfSet )
                    {
                        HRESULT           hrSet;
                        IPropertyStorage* pps;
                        UINT              uCodePage = 0;
                        hrSet = SHPropStgCreate( ppss, rgFmtid[i], NULL, PROPSETFLAG_DEFAULT, 
                                                 grfMode, OPEN_ALWAYS, &pps, &uCodePage );
                    
                        if( SUCCEEDED( hrSet ) )
                        {
                            hrSet = SHPropStgWriteMultiple( pps, &uCodePage, (i - iStart) + 1, rgSpec + iStart, 
                                                            rgVal + iStart, PID_FIRST_USABLE );
                            pps->Release();
                        }
                        if( FAILED( hrSet ) )
                            hrThis = hrSet;

                        iStart = i + 1;
                    }
                }

                //  cleanup copies.
                if( SUCCEEDED( hrThis ) )
                {
                    for( i = cPersisted = 0; i < cItems; i++ )
                    {
                        if( rgItems[i].bDirty )
                        {
                            ASSERT( cPersisted < cPersist )
                            rgItems[i].bDirty = FALSE;
                            PropVariantClear( &rgVal[cPersisted] );
                            cPersisted++;
                        }
                    }
                }
            }
            else
                hrThis = E_OUTOFMEMORY;

            delete [] rgVal;
            delete [] rgSpec;
            delete [] rgFmtid;
            ppss->Release();
        }
    }
    else
        hrThis = hrSpecialized;
    
    return hrThis;
}


//------------------------------//
//  IAdvancedPropertyServer methods
//------------------------------//

//-------------------------------------------------------------------------//
//  CPTDefSrv32::QueryServerInfo
STDMETHODIMP CPTDefSrv32::QueryServerInfo( PROPSERVERINFO * pInfo )
{
    // TODO: Add your implementation code here
    UNREFERENCED_PARAMETER( pInfo );

    return S_OK;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::AcquireAdvanced
STDMETHODIMP CPTDefSrv32::AcquireAdvanced( const VARIANT * pvarSrc, LPARAM * plParam )
{
    CPropertySource* pSrc;
    HRESULT              hr;

    *plParam = 0L;

    //  Create new data structure for property tree source
    if( (pSrc = new CPropertySource)==NULL )
        return E_OUTOFMEMORY;

    //  Instruct source to acquire its properties
    if( FAILED( (hr = pSrc->Acquire( pvarSrc )) ) )
    {
        delete pSrc;
        pSrc = NULL;
    }
    *plParam = (LPARAM)pSrc;
    return hr;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::MakePropertyTreeSource
HRESULT CPTDefSrv32::MakePropertyTreeSource( IN LPARAM lParam, OUT CPropertySource** ppSrc )
{
    if( !( (*ppSrc = (CPropertySource*)lParam)!=NULL &&
           (*ppSrc)->m_dwSize==sizeof(CPropertySource) ) )
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::EnumFolderItems
STDMETHODIMP CPTDefSrv32::EnumFolderItems( LPARAM lParam, IEnumPROPFOLDERITEM ** ppEnum )
{
    HRESULT hr;
    CPropertySource* pSrc;

    if( !ppEnum ) return E_POINTER;
    *ppEnum = NULL;

    //  Retrieve address of our source object
    if( FAILED( (hr = MakePropertyTreeSource( lParam, &pSrc )) ) )
        return hr;

    CEnumFolderItem* pEnum;
    if( (pEnum = new  CEnumFolderItem( pSrc ))==NULL )
        return E_OUTOFMEMORY;

    *ppEnum = pEnum;
    pEnum->AddRef();

    return S_OK;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::EnumPropertyItems
STDMETHODIMP CPTDefSrv32::EnumPropertyItems( const PROPFOLDERITEM * pItem, IEnumPROPERTYITEM ** ppEnum)
{
    HRESULT hr;
    CPropertySource* pSrc;

    if( !ppEnum ) return E_POINTER;
    *ppEnum = NULL;

    //  Retrieve address of our source object
    if( FAILED( (hr = MakePropertyTreeSource( pItem->lParam, &pSrc )) ) )
        return hr;

    CEnumPropertyItem* pEnum;
    if( (pEnum = new  CEnumPropertyItem( pSrc, pItem ))==NULL )
        return E_OUTOFMEMORY;

    *ppEnum = pEnum;
    pEnum->AddRef();

    return S_OK;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::EnumValidValues
STDMETHODIMP CPTDefSrv32::EnumValidValues( const PROPERTYITEM * pItem, IEnumPROPVARIANT_DISPLAY * * ppEnum)
{
    HRESULT hr;
    long    iDefProp = -1;
    CPropertySource* pSrc;

    if( !ppEnum ) return E_POINTER;
    *ppEnum = NULL;

    //  Retrieve address of our source object
    if( FAILED( (hr = MakePropertyTreeSource( pItem->lParam, &pSrc )) ) )
        return hr;

    //  For properties with enumerated selection lists, refer to default property definition.
    if( pItem->dwFlags & PTPIF_ENUM )
    {
        //  Find the default property item data for this property
        if( (iDefProp = FindDefPropertyItem( pItem->puid.fmtid, pItem->puid.propid, pItem->puid.vt )) >= 0 )
        {
            const DEFVAL* pVals = NULL;
            ULONG         cVals = 0;

            //  See if there is a defined value list associated with the item.
            if( SUCCEEDED( GetDefPropItemValues( iDefProp, &pVals, &cVals ) ) &&
                cVals > 0 && pVals )
            {
                CEnumHardValues* pEnum;
                if( (pEnum = new CEnumHardValues( pVals, cVals ))==NULL )
                    return E_OUTOFMEMORY;
                *ppEnum = pEnum;
                pEnum->AddRef();
            }
        }
    }
    else if( pItem->dwFlags & PTPIF_MRU )
    {
        CEnumMruValues* pEnum;

        if( (pEnum = new CEnumMruValues( pItem->puid ))==NULL )
            return E_OUTOFMEMORY;

        *ppEnum = pEnum;
        pEnum->AddRef();
    }

    return *ppEnum ? S_OK : E_FAIL;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::PersistAdvanced
STDMETHODIMP CPTDefSrv32::PersistAdvanced( PROPERTYITEM * pItem )
{
    HRESULT hr;
    CPropertySource* pSrc;

    //  Retrieve address of our source object
    if( FAILED( (hr = MakePropertyTreeSource( pItem->lParam, &pSrc )) ) )
        return hr;

    //  Forward to specialized server:
    LPARAM lParamSpecialized;
    IAdvancedPropertyServer* pSpecialized;
    CPropertyMap& map = pSrc->PropertyMap();

    //  Try specialized server
    hr = E_NOTIMPL;
    if( (pSpecialized = pSrc->GetSpecializedServer( &lParamSpecialized )) != NULL )
    {
        pItem->lParam = lParamSpecialized;
        hr = pSpecialized->PersistAdvanced( pItem );
        pItem->lParam = (LPARAM)pSrc;
    }

    //  If specialized server couldn't persist it and
    //  it's one of our default items, give it a try.
    if( S_OK != hr &&
        FindDefPropertyItem( pItem->puid.fmtid, pItem->puid.propid, pItem->puid.vt ) >= 0 )
    {
        //  Persist
        hr = pSrc->Persist( *pItem );
        ClearPropertyItem( pItem ); // release memory
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  CPTDefSrv32::ReleaseAdvanced
STDMETHODIMP CPTDefSrv32::ReleaseAdvanced( LPARAM lParamSrc, ULONG dwFlags )
{
    HRESULT hr;
    CPropertySource* pSrc;

    //  Retrieve address of our source object
    if( FAILED( (hr = MakePropertyTreeSource( lParamSrc, &pSrc )) ) )
        return hr;

    //  Forward to specialized server:
    LPARAM lParamSpecialized;
    IAdvancedPropertyServer* pSpecialized;
    if( (pSpecialized = pSrc->GetSpecializedServer( &lParamSpecialized )) != NULL )
        pSpecialized->ReleaseAdvanced( lParamSpecialized, dwFlags );

    //  Do our own processing
    if( pSrc )
    {
        if ( dwFlags & PTREL_DONE_ENUMERATING )
            pSrc->PropertyMap().Clear();   // don't need this anymore.

        if( dwFlags & PTREL_CLOSE_SOURCE )
            pSrc->Close( FALSE /* don't close permanently! */);

        if( dwFlags == PTREL_SOURCE_REMOVED )
            delete pSrc;
    }

    return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP SSAcquireMultiple (
    /*[in]*/ LONG cSrcs,
    /*[in]*/ VARIANT* aSrcs,
    /*[out,optional]*/ ULONG *aStgFmts,
    /*[out,optional]*/ HRESULT *aResults,
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[out]*/ PROPVARIANT* aVals,
    /*[out,optional]*/ ULONG* aFlags )
{
    LONG         cValidSrcs = 0;
    PROPVARIANT* pvalBuf ;
    const        ULONG stgmode = STGM_DIRECT|STGM_READ;

    //  Perform gross validation of args.
    if( cSrcs <= 0 || cProps <= 0 ||
        NULL == aSrcs || NULL == aSpecs || NULL == aVals )
        return E_INVALIDARG;

    //  If multiple sources, need a scratch buffer for merging values.
    if( cSrcs > 0 && NULL == (pvalBuf = new PROPVARIANT[cProps]) )
        return E_OUTOFMEMORY;

    //  Initialize outbounds
    for( LONG iProp = 0; iProp < cProps; iProp++ )
    {
        PropVariantInit( &aVals[iProp] );
        if( aFlags )
            aFlags[iProp] = 0L;
    }

    //  For each property source in the list...
    for( LONG iSrc = 0; iSrc < cSrcs; iSrc++ )
    {
        HRESULT                 hrSrc = S_OK;
        ULONG                   stgFmt = STGFMT_NONE;
        IPropertySetStorage*    pPSS;

        if( aSrcs[iSrc].vt != VT_BSTR ||
            aSrcs[iSrc].bstrVal == NULL ||
            aSrcs[iSrc].bstrVal[0] == (WCHAR)0 )
        {
            hrSrc = E_INVALIDARG;
        }
        else
        {
            //  If the caller wants to know storage the format, must
            //  test specific formats...
            if( aStgFmts )
            {
                //  Try opening as OLE or NSS structured storage (the file had to be created as such):
                if( SUCCEEDED( (hrSrc = SHStgOpenStorage( aSrcs[iSrc].bstrVal,
                                                            stgmode|STGM_SHARE_EXCLUSIVE, 0, 0,
                                                            IID_IPropertySetStorage, (PVOID*)&pPSS )) ) )
                    stgFmt = STGFMT_STORAGE;
                else
                //  Try opening as NFF (NTFS flat file props) (this should work for any NTFS 5+ file).
                if( SUCCEEDED( (hrSrc = SHStgOpenStorage( aSrcs[iSrc].bstrVal,
                                                          stgmode|STGM_SHARE_EXCLUSIVE,
                                                          0, 0,
                                                          IID_IPropertySetStorage, (PVOID*)&pPSS )) ) )
                    stgFmt = STGFMT_ANY;
            }
            else
            {
                if( SUCCEEDED( (hrSrc = SHStgOpenStorage( aSrcs[iSrc].bstrVal,
                                                          stgmode|STGM_SHARE_EXCLUSIVE, 0, 0,
                                                          IID_IPropertySetStorage, (PVOID*)&pPSS )) ) )
                    stgFmt = STGFMT_ANY;
            }

            if( STGFMT_NONE != stgFmt )
            {
                PROPVARIANT*        pVals = ((0 == iSrc) ? aVals : pvalBuf);
                IPropertyStorage*   pPS;

                //  got a live one.
                cValidSrcs++;

                //  initialize PROPVARIANTS.
                for( int iProp = 0; iProp < cProps; iProp++ )
                    PropVariantInit( &pVals[iProp] );

                if( SUCCEEDED( (hrSrc = pPSS->Open( fmtid, stgmode|STGM_SHARE_EXCLUSIVE, &pPS )) ) )
                {
                    hrSrc = pPS->ReadMultiple( cProps, aSpecs, pVals );
                    pPS->Release();
                }

                //  Merge values:
                if( iSrc > 0 )
                {
                    for( iProp = 0; iProp< cProps; iProp++ )
                    {
                        //  Flag property as composite value
                        if( aFlags )
                            aFlags[iProp] |= AMPF_COMPOSITE;

                        //  If the value is different than our composite, clear it; it's 'multiple values'
                        if( 0 != PropVariantCompare( aVals[iProp], pVals[iProp], STRICT_COMPARE ) )
                        {
                            PropVariantClear( &aVals[iProp] );

                            //  flag property as composite with mismatched values.
                            if( aFlags )
                                aFlags[iProp] |= AMPF_COMPOSITE_MISMATCH;
                        }
                    }

                    //  Reinit our buffer.
                    for( iProp = 0; iProp < cProps; iProp++ )
                        PropVariantClear( &pVals[iProp] );
                }

                pPSS->Release();
            }
        }

        //  Assign results.
        if( aResults )
            aResults[iSrc] = hrSrc;
        if( aStgFmts )
            aStgFmts[iSrc] = stgFmt;
    }

    if( pvalBuf )
        delete [] pvalBuf;

    return (cSrcs == cValidSrcs) ? S_OK :
           (cValidSrcs) > 0 ? S_FALSE : E_FAIL;
}

//-------------------------------------------------------------------------//
STDMETHODIMP SSPersistMultiple (
    /*[in]*/ LONG cSrcs,
    /*[in]*/ VARIANT* aSrcs,
    /*[out,optional]*/ HRESULT *aResults,
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[in]*/ PROPVARIANT* aVals,
    /*[in]*/ PROPID propidNameFirst )
{
    LONG         cValidSrcs = 0;
    const        ULONG stgmode = STGM_DIRECT|STGM_READWRITE;

    //  Perform gross validation of args.
    if( cSrcs <= 0 || cProps <= 0 ||
        NULL == aSrcs || NULL == aSpecs || NULL == aVals )
        return E_INVALIDARG;

    //  For each property source in the list...
    for( LONG iSrc = 0; iSrc < cSrcs; iSrc++ )
    {
        HRESULT                 hrSrc = S_OK;
        ULONG                   stgFmt = STGFMT_NONE;
        IPropertySetStorage*    pPSS;

        if( aSrcs[iSrc].vt != VT_BSTR ||
            aSrcs[iSrc].bstrVal == NULL ||
            aSrcs[iSrc].bstrVal[0] == (WCHAR)0 )
        {
            hrSrc = E_INVALIDARG;
        }
        else
        {
            //  Try opening as NSS flat file
            if( SUCCEEDED( (hrSrc = SHStgOpenStorage( aSrcs[iSrc].bstrVal,
                                                      stgmode|STGM_SHARE_EXCLUSIVE, 0, 0,
                                                      IID_IPropertySetStorage, (PVOID*)&pPSS )) ) )
                stgFmt = STGFMT_ANY;

            if( STGFMT_NONE != stgFmt )
            {
                IPropertyStorage*   pPS;

                if( FAILED( (hrSrc = pPSS->Open( fmtid, stgmode|STGM_SHARE_EXCLUSIVE, &pPS )) ) )
                    hrSrc = pPSS->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                          stgmode|STGM_SHARE_EXCLUSIVE, &pPS );

                if( SUCCEEDED( hrSrc ) )
                {
                    hrSrc = pPS->WriteMultiple( cProps, aSpecs, aVals, propidNameFirst );
                    pPS->Release();

                    if( SUCCEEDED( hrSrc ) )
                    {
                        cValidSrcs++;

                        //  Update MRU
                        CPropMruStor stor( PTREGROOTKEY, PTREGSUBKEY );
                        for( LONG iProp = 0; iProp < cProps; iProp++ )
                        {
                            if( PRSPEC_PROPID == aSpecs[iProp].ulKind )
                                stor.PushMruVal( fmtid, aSpecs[iProp].propid,
                                                 aVals[iProp].vt, aVals[iProp] );
                        }
                    }
                }

                pPSS->Release();
            }
        }

        //  Assign results.
        if( aResults )
            aResults[iSrc] = hrSrc;
    }

    return (cSrcs == cValidSrcs) ? S_OK :
           (cValidSrcs) > 0 ? S_FALSE : E_FAIL;
}


//---------------------------//
//  IPropertyServer methods
//---------------------------//

//-------------------------------------------------------------------------//
//  IPropertyServer::AcquireMultiple
STDMETHODIMP CPTDefSrv32::AcquireMultiple(
    /*[in]*/ LONG cSrcs,
    /*[in]*/ VARIANT* aSrcs,
    /*[out,optional]*/ ULONG *aStgFmts,
    /*[out,optional]*/ HRESULT *aResults,
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[out]*/ PROPVARIANT* aVals,
    /*[out,optional]*/ ULONG* aFlags )
{
    return SSAcquireMultiple( cSrcs, aSrcs, aStgFmts,
                              aResults, fmtid, cProps, aSpecs,
                              aVals, aFlags );
}

//-------------------------------------------------------------------------//
//  IPropertyServer::PersistMultiple
STDMETHODIMP CPTDefSrv32::PersistMultiple(
    /*[in]*/ LONG cSrcs,
    /*[in]*/ VARIANT* aSrcs,
    /*[out,optional]*/ HRESULT *aResults,
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[in]*/ PROPVARIANT* aVals,
    /*[in]*/ PROPID propidNameFirst )
{
    return SSPersistMultiple( cSrcs, aSrcs, aResults, fmtid,
                              cProps, aSpecs, aVals, propidNameFirst );
}

//-------------------------------------------------------------------------//
//  IPropertyServer::GetDisplayText
STDMETHODIMP CPTDefSrv32::GetDisplayText(
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[in]*/ PROPVARIANT* aVals,
    /*[in, optional]*/ BSTR*  abstrFmt,
    /*[in, optional]*/ ULONG* adwFlags,
    /*[out]*/ BSTR* abstrDisplay )
{
    UNREFERENCED_PARAMETER( fmtid );
    UNREFERENCED_PARAMETER( aSpecs );

    if( cProps <= 0 || NULL == aVals || NULL == abstrDisplay )
        return E_INVALIDARG;

    HRESULT hrRet = S_OK;

    for( LONG iProp =0; iProp < cProps; iProp++ )
    {
        BSTR bstrFmt    = abstrFmt ? abstrFmt[iProp]  : NULL;
        ULONG dwFlags   = adwFlags ? adwFlags[iProp] : 0L;
        CPropVariant var( aVals[iProp] );
        HRESULT hr;
        USES_CONVERSION;

        if( FAILED( (hr = var.GetDisplayText(
            abstrDisplay[iProp], W2T( bstrFmt ), dwFlags )) ) )
            hrRet = hr;
    }
    return hrRet;
}

//-------------------------------------------------------------------------//
//  IPropertyServer::AssignFromDisplayText
STDMETHODIMP CPTDefSrv32::AssignFromDisplayText(
    /*[in]*/ REFFMTID fmtid,
    /*[in]*/ LONG cProps,
    /*[in]*/ PROPSPEC* aSpecs,
    /*[in]*/ BSTR* abstrDisplay,
    /*[in, optional]*/ BSTR*  abstrFmt,
    /*[in, optional]*/ ULONG* adwFlags,
    /*[in, out]*/ PROPVARIANT* aVals )
{
    UNREFERENCED_PARAMETER( fmtid );
    UNREFERENCED_PARAMETER( aSpecs );
    UNREFERENCED_PARAMETER( adwFlags );

    if( cProps <= 0 || NULL == aVals || NULL == abstrDisplay )
        return E_INVALIDARG;

    HRESULT hrRet = S_OK;

    for( LONG iProp =0; iProp < cProps; iProp++ )
    {
        BSTR bstrFmt    = abstrFmt ? abstrFmt[iProp]  : NULL;
        CPropVariant var( aVals[iProp] );
        HRESULT hr;
        USES_CONVERSION;

        if( FAILED( (hr = var.AssignFromDisplayText(
            abstrDisplay[iProp], W2T( bstrFmt ) )) ) )
            hrRet = hr;
        else
        if( FAILED( (hr = PropVariantCopy( &aVals[iProp], &var )) ) )
            hrRet = hr;
    }
    return hrRet;
}

//-------------------------------------------------------------------------//
//  Types and helpers for class CPropertySource
//-------------------------------------------------------------------------//

//  MRU Stor registry key
const TCHAR PTREGSUBKEY[] = TEXT("Software\\Microsoft\\MSPropertyTree\\DefaultServer");

//  String resource mapping block
typedef struct tagSTRRESMAPENTRY
{
    ULONG  val;
    ULONG  nIDS;
} STRRESMAPENTRY, *PSTRRESMAPENTRY, *LPSTRRESMAPENTRY;

//-------------------------------------------------------------------------//
PVOID AllocStringValueA( LPCSTR pszSrc, VARTYPE vt )
{
    PVOID pvRet = NULL;
    int   cch = 0;
    USES_CONVERSION;

    if( pszSrc )
    {
        switch( vt )
        {
            case VT_LPSTR:
                cch = lstrlenA( pszSrc );
                if( (pvRet = CoTaskMemAlloc( (cch+1) * sizeof(CHAR) )) != NULL )
                    lstrcpyA( (LPSTR)pvRet, pszSrc );
                break;

            case VT_LPWSTR:
                cch = lstrlenA( pszSrc );
                if( (pvRet = CoTaskMemAlloc( (cch+1) * sizeof(WCHAR) )) != NULL )
                    lstrcpyW( (LPWSTR)pvRet, A2W(pszSrc) );
                break;

            case VT_BSTR:
                pvRet = SysAllocString( A2W(pszSrc) );
                break;
        }
    }
    return pvRet;
}

//-------------------------------------------------------------------------//
PVOID AllocStringValueW( LPCWSTR pwszSrc, VARTYPE vt )
{
    PVOID pvRet = NULL;
    int   cch = 0;
    USES_CONVERSION;

    if( pwszSrc )
    {
        switch( vt )
        {
            case VT_LPSTR:
                cch = lstrlenW( pwszSrc );
                if( (pvRet = CoTaskMemAlloc( (cch+1) * sizeof(CHAR) )) != NULL )
                    lstrcpyA( (LPSTR)pvRet, W2A(pwszSrc) );
                break;

            case VT_LPWSTR:
                cch = lstrlenW( pwszSrc );
                if( (pvRet = CoTaskMemAlloc( (cch+1) * sizeof(WCHAR) )) != NULL )
                    lstrcpyW( (LPWSTR)pvRet, pwszSrc );
                break;

            case VT_BSTR:
                pvRet = SysAllocString( pwszSrc );
                break;
        }
    }
    return pvRet;
}

//-------------------------------------------------------------------------//
#ifdef UNICODE
#define AllocStringValue    AllocStringValueW
#else
#define AllocStringValue    AllocStringValueA
#endif

//-------------------------------------------------------------------------//
PVOID LoadStringValue( UINT nID, VARTYPE vt )
{
    TCHAR  pszBuf[MAX_STRINGRES+1];
    int    cch;

    if( (cch = LoadString( _Module.GetResourceInstance(), nID,
                           pszBuf, sizeof(pszBuf)/sizeof(TCHAR) )) >0 )
    {
        USES_CONVERSION;
        return AllocStringValue( pszBuf, vt );
    }
    return NULL;
}

//-------------------------------------------------------------------------//
PVOID LoadStringRes( ULONG val, VARTYPE vt, STRRESMAPENTRY* map, int cMappings )
{
    for( int i=0; i<cMappings; i++ )    {
        if( map[i].val == val )
            return ::LoadStringValue( map[i].nIDS, vt );
    }
    return NULL;
}

//-------------------------------------------------------------------------//
//  String resource map definition and access macros.
#define BEGIN_STRRES_MAP( map )         static STRRESMAPENTRY map[] = {
#define STRRES_MAPENTRY( val )          { (ULONG)(val), (ULONG)(IDS_##val) },
#define END_STRRES_MAP()                };
#define LOAD_STRRES( val, vt, map )     ::LoadStringRes( val, vt, map, (sizeof((map))/sizeof(STRRESMAPENTRY)) )

//-------------------------------------------------------------------------//
//  Image property value string resource mappings
//-------------------------------------------------------------------------//

#ifdef _X86_
//  image Type values
BEGIN_STRRES_MAP( iflTypeStrRes )
    STRRES_MAPENTRY( IFLT_UNKNOWN )
    STRRES_MAPENTRY( IFLT_GIF )
    STRRES_MAPENTRY( IFLT_BMP )
    STRRES_MAPENTRY( IFLT_JPEG )
    STRRES_MAPENTRY( IFLT_TIFF )
    STRRES_MAPENTRY( IFLT_PNG )
    STRRES_MAPENTRY( IFLT_PCD )
    STRRES_MAPENTRY( IFLT_PCX )
    STRRES_MAPENTRY( IFLT_TGA )
    STRRES_MAPENTRY( IFLT_PICT )
END_STRRES_MAP()

//  image Class values
BEGIN_STRRES_MAP( iflClassStrRes )
    STRRES_MAPENTRY( IFLCL_BILEVEL )
    STRRES_MAPENTRY( IFLCL_GRAY )
    STRRES_MAPENTRY( IFLCL_GRAYA )
    STRRES_MAPENTRY( IFLCL_PALETTE )
    STRRES_MAPENTRY( IFLCL_RGB )
    STRRES_MAPENTRY( IFLCL_RGBPLANAR )
    STRRES_MAPENTRY( IFLCL_RGBA )
    STRRES_MAPENTRY( IFLCL_RGBAPLANAR )
    STRRES_MAPENTRY( IFLCL_CMYK )
    STRRES_MAPENTRY( IFLCL_YCC )
    STRRES_MAPENTRY( IFLCL_CIELAB )
    STRRES_MAPENTRY( IFLCL_NONE )
END_STRRES_MAP()

//  internal Tiling Format values
BEGIN_STRRES_MAP( iflTileFmtStrRes )
    STRRES_MAPENTRY( IFLTF_NONE )
    STRRES_MAPENTRY( IFLTF_STRIPS )
    STRRES_MAPENTRY( IFLTF_TILES )
END_STRRES_MAP()

//  internal Packing Mode values
BEGIN_STRRES_MAP( iflPackModeStrRes )
    STRRES_MAPENTRY( IFLPM_PACKED )
    STRRES_MAPENTRY( IFLPM_UNPACKED )
    STRRES_MAPENTRY( IFLPM_LEFTJUSTIFIED )
    STRRES_MAPENTRY( IFLPM_NORMALIZED )
    STRRES_MAPENTRY( IFLPM_RAW )
END_STRRES_MAP()

//  internal Line Sequence values
BEGIN_STRRES_MAP( iflLineSeqStrRes )
    STRRES_MAPENTRY( IFLSEQ_TOPDOWN )
    STRRES_MAPENTRY( IFLSEQ_BOTTOMUP )
    STRRES_MAPENTRY( IFLSEQ_GIF_INTERLACED )
    STRRES_MAPENTRY( IFLSEQ_ADAM7_INTERLACED )
END_STRRES_MAP()

//  internal Compression Format values
BEGIN_STRRES_MAP( iflCompressionStrRes )
    STRRES_MAPENTRY( IFLCOMP_NONE )
    STRRES_MAPENTRY( IFLCOMP_DEFAULT )
    STRRES_MAPENTRY( IFLCOMP_RLE )
    STRRES_MAPENTRY( IFLCOMP_CCITT1D )
    STRRES_MAPENTRY( IFLCOMP_CCITTG3 )
    STRRES_MAPENTRY( IFLCOMP_CCITTG4 )
    STRRES_MAPENTRY( IFLCOMP_LZW )
    STRRES_MAPENTRY( IFLCOMP_LZWHPRED )
    STRRES_MAPENTRY( IFLCOMP_JPEG )
END_STRRES_MAP()

//  BMP version values
BEGIN_STRRES_MAP( iflBmpVerRes )
    STRRES_MAPENTRY( IFLBV_WIN_3 )
    STRRES_MAPENTRY( IFLBV_OS2_1 )
    STRRES_MAPENTRY( IFLBV_OS2_2S )
    STRRES_MAPENTRY( IFLBV_OS2_2M )
END_STRRES_MAP()

#endif

//-------------------------------------------------------------------------//
//  class CPropertySource methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CPropertySource::CPropertySource()
    :   m_dwSize( sizeof(CPropertySource) ),
        m_dwSrcType( PST_NOSTG|PST_UNKNOWNDOC ),
        m_dwFileAttr((ULONG)-1),
        m_dwStgWriteMode(0),
        m_dwStgShareMode(0),
        m_pPSS(NULL),
        m_pSpecialized(NULL),
        m_lParamSpecialized(0L)
{
    memset( m_szStgName, 0, sizeof(m_szStgName) );

#ifdef _X86_
    m_cIflProps = 0;
    InitIflProperties( &m_IflProps );
#endif _X86_
}

//-------------------------------------------------------------------------//
//  Retrieves the appropriate server for the indicated source file.
HRESULT CPropertySource::FindServerForSource(
    LPCWSTR pwszSrc,
    IAdvancedPropertyServer** pppts )
{
    HRESULT hr = E_FAIL;
    LPCTSTR pszExt = NULL;
    USES_CONVERSION;
    ASSERT( pppts );

    *pppts = NULL;
    TCHAR   szPath[MAX_PATH];
    lstrcpyn( szPath, W2T((LPWSTR)pwszSrc), ARRAYSIZE(szPath) );
    pszExt = PathFindExtension( szPath );

    if( pszExt )
    {
        CLSID clsid;
        if( SUCCEEDED( (hr = GetPropServerClassForFile( szPath, TRUE, &clsid )) ) )
        {
            if( IsEqualGUID( clsid, CLSID_PTDefaultServer32 ) )
                return E_UNEXPECTED;

            if( FAILED( (hr = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER,
                                                IID_IAdvancedPropertyServer,
                                                (void**)pppts )) ) )
                return hr;

#ifdef _DEBUG
            WCHAR szClsid[128];
            StringFromGUID2( clsid, szClsid, ARRAYSIZE(szClsid) );
            TRACE( TEXT("Using specialized PropertyTreeServer %s for %s.\n"),
                         W2T(szClsid), PathFindExtension( szPath ) );
#endif _DEBUG

        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
CPropertySource::~CPropertySource()
{
    Close( TRUE );
    if( m_pSpecialized )
        m_pSpecialized->Release();
}

//-------------------------------------------------------------------------//
HRESULT CPropertySource::Acquire( const VARIANT* pvarSrc )
{
    ASSERT( NULL != pvarSrc );

    if( VT_BSTR == pvarSrc->vt )
    {
        if( NULL == m_pSpecialized )
        {
            //  Try finding a speciallized property tree server for this source...
            IAdvancedPropertyServer* pSpecialized = NULL;
            if( SUCCEEDED( FindServerForSource( pvarSrc->bstrVal, &pSpecialized ) ) )
            {
                LPARAM lParam = 0;
                if( SUCCEEDED( pSpecialized->AcquireAdvanced( pvarSrc, &lParam ) ) )
                {
                    //  Got one; stash it away.
                    m_pSpecialized = pSpecialized;
                    m_lParamSpecialized = lParam;
                }
                else
                    pSpecialized->Release();
            }
        }

        return Acquire( pvarSrc->bstrVal );
    }

    return E_NOTIMPL;
}

//-------------------------------------------------------------------------//
HRESULT CPropertySource::Acquire( const WCHAR* pwszSrc )
{
    HRESULT hr = E_FAIL;
    IPropertySetStorage* pPSS = NULL;
    ULONG   dwSrcType   = PST_NOSTG|PST_UNKNOWNDOC ,
            dwWriteMode = 0,
            dwShareMode = 0,
            dwFileAttr  = 0xFFFFFFFF;
    LPCWSTR pszStgName  = NULL;


    //  Attempt to acquire an IPropertySetStorage interface from the source;
    USES_CONVERSION;
    if( 0xFFFFFFFF == (dwFileAttr = PTGetFileAttributesW( pwszSrc ) ) )
        return E_FAIL;

    //  Determine the level of support for the file
    PTSRV_FILECLASS fileClass;
    PTSRV_FILETYPE  fileType;
    BOOL            bKnown = IsPTsrvKnownFileType( W2CT( pwszSrc ), &fileType, &fileClass );

    if( bKnown && PTSFCLASS_UNSUPPORTED == fileClass )
        return E_ABORT;

    //  First, test if the source is an image file
#ifdef _X86_

    InitIflProperties( &m_IflProps );
    m_IflProps.mask = (ULONG)-1;
    AcquireImageProperties( pwszSrc, &m_IflProps, &m_cIflProps );
    if( m_cIflProps > 0 && (m_IflProps.mask & IPF_TYPE) != 0 )
        SETDOCTYPE( &dwSrcType, PST_IMAGEFILE );
    else
    {
#endif _X86_

        if( bKnown )
        {
            if( PTSFCLASS_IMAGE == fileClass )
                SETDOCTYPE( &dwSrcType, PST_IMAGEFILE );
            else if ( PTSFCLASS_AUDIO == fileClass || PTSFCLASS_VIDEO == fileClass )
                SETDOCTYPE( &dwSrcType, PST_MEDIAFILE );
        }

#ifdef _X86_
    }
#endif _X86_

    //  Retrieve IPropertySetStorage interface,
    //  Do final determination of source type.
    if( SUCCEEDED( (hr = OpenPropertySetStorage( pwszSrc, &pPSS,
                                                 STGM_READ, STGM_SHARE_EXCLUSIVE,
                                                 &dwSrcType )) ) )
    {
        dwWriteMode = STGM_READ;
        dwShareMode = STGM_SHARE_EXCLUSIVE;
        pszStgName = pwszSrc;
    }

    //  Cache our IPropertySetStorage interface pointer
    if( GETSTGTYPE( dwSrcType ) != PST_NOSTG && pPSS )
    {
        //  Close down if we're already open.
        Close( FALSE );

        //  Assign IPropertSetStorage interface pointer
        m_pPSS           = pPSS;
        m_dwStgWriteMode = dwWriteMode;
        m_dwStgShareMode = dwShareMode;

        if( pszStgName )
            wcsncpy( m_szStgName, pszStgName, sizeof(m_szStgName)/sizeof(WCHAR) );
    }

    m_dwFileAttr = dwFileAttr;

    //  Assign source type and gather properties + property values.
    m_dwSrcType = dwSrcType;
    GatherProperties();

    return S_OK;
}

//-------------------------------------------------------------------------//
BOOL CPropertySource::UsesFolder( IN LONG dwFolderID )
{
#ifdef _X86_
    if( IsDefFolderPFID( dwFolderID, PFID_FaxProperties ) &&
        HasFaxProperties( &m_IflProps ) ) 
        return TRUE;
        
    if( IsDefFolderPFID( dwFolderID, PFID_ImageProperties ) &&
        HasImageProperties( &m_IflProps ) ) 
        return TRUE;
#endif _X86_

    return IsDefFolderSrcType( dwFolderID, m_dwSrcType );
}

//-------------------------------------------------------------------------//
//  Retrieves and maps properties for the current source.
int CPropertySource::GatherProperties()
{
    IEnumSTATPROPSETSTG* pEnumPropSet = NULL;
    const ULONG          nBlocks = 8;
    int                  cnt = 0;
    long                 iDefProp,
                         cDefProp = DefPropCount();
    HRESULT              hr;
    PROPERTYITEM       propitem;

    if( m_map.Count()>0 )
        m_map.Clear();

    //  Gather image properties
    if( GETDOCTYPE( m_dwSrcType ) == PST_IMAGEFILE )
        cnt += GatherImageProperties();

    //  Enumerate the source's property sets via IPropertySetStorage interface
    if( GETSTGTYPE( m_dwSrcType )!=0 && m_pPSS )
    {
        if( SUCCEEDED( m_pPSS->Enum( &pEnumPropSet ) ) )
        {
            STATPROPSETSTG  statPropSet[nBlocks];
            ULONG cSets;

            //  Grab a set enumerator
            while( SUCCEEDED( pEnumPropSet->Next( nBlocks, statPropSet, &cSets ) ) && cSets > 0 )
            {
                //  for each property set
                for( ULONG iSet=0; iSet<cSets; iSet++ )
                {
                    ULONG cSetProps = 0;   // tally of properties in set.
                    IPropertyStorage*   pPropStg = NULL;
                    UINT  uCodePage = 0;

                    //  Open the set.
                    if( FAILED( (hr = OpenPropertyStorage( m_pPSS, statPropSet[iSet].fmtid,
                                                           m_dwStgWriteMode, OPEN_EXISTING, 
                                                           &pPropStg, &uCodePage )) ) )
                        continue;

                    //  Grab a property enumerator
                    IEnumSTATPROPSTG* pEnumProp = NULL;
                    if( SUCCEEDED( (hr = pPropStg->Enum( &pEnumProp )) ) )
                    {
                        STATPROPSTG     statProp[nBlocks];
                        ULONG cProps;
                        while( SUCCEEDED( pEnumProp->Next( nBlocks, statProp, &cProps ) ) )
                        {
                            //  Retrieve default property item definition and the value for
                            //  each property in this set.
                            for( ULONG iProp = 0; iProp<cProps; iProp++ )
                            {
                                memset( &propitem, 0, sizeof(propitem) );
                                propitem.cbStruct = sizeof(propitem);
                                if( (iDefProp = FindDefPropertyItem( statPropSet[iSet].fmtid,
                                                                     statProp[iProp].propid,
                                                                     statProp[iProp].vt ))!= -1 &&
                                    SUCCEEDED( MakeDefPropertyItem( iDefProp, &propitem, (LPARAM)this )))
                                {
                                    //  Read the property's value
                                    //  BUGBUG: should figure out a way to read property values en masse.
                                    PROPSPEC propspec = { PRSPEC_PROPID, statProp[iProp].propid };
                                    SHPropStgReadMultiple( pPropStg, uCodePage, 1, &propspec, &propitem.val );
                                    
                                    if( propitem.puid.vt != propitem.val.vt )
                                    {
                                        //  Adjust vartype to agree with any type normalization done by
                                        //  SHPropStgReadMultiple.
                                        propitem.puid.vt = propitem.val.vt; 
                                    }

                                    if( propitem.val.vt==VT_FILETIME )
                                    {
                                        //  Convert to local file time before handing off
                                        FILETIME ft = propitem.val.filetime;
                                        FileTimeToLocalFileTime( &ft, &propitem.val.filetime );
                                    }

                                    PreMapPropItem( &propitem );
                                    if( m_map.Insert( propitem ) )
                                    {
        #if 0
                                        USES_CONVERSION;
                                        TRACE(TEXT("Mapped property '%s'\n"), W2T( propitem.bstrName ) );
        #endif 0
                                        cSetProps++;
                                    }
                                    else
                                    {
                                        ASSERT( FALSE );
                                    }
                                }
                            }
                            if( cProps< nBlocks ) break;
                        }
                        pEnumProp->Release();
                        cnt += cSetProps;
                    }
                    pPropStg->Release();
                }

                if( cSets < nBlocks ) break;
            }
            pEnumPropSet->Release();
        }

        //  Now, iterate our default property sets and add to our collection
        //  those properties the source is missing
        for( iDefProp=0; iDefProp < cDefProp; iDefProp++ )
        {
            memset( &propitem, 0, sizeof(propitem) );
            propitem.cbStruct = sizeof(propitem);
            const FMTID* pFmtID;
            BOOL    bInnate;

            //  If it's our type of property, retrieve it's fmtid, propid and vt.
            if( IsDefPropSrcType( iDefProp, m_dwSrcType ) &&
                (bInnate = DefPropHasFlags( iDefProp, PTPIF_INNATE ))==FALSE &&
                SUCCEEDED( GetDefPropItemID( iDefProp, &pFmtID, 
                                             &propitem.puid.propid, &propitem.puid.vt ) ) )
            {
                //  If this is a legacy ANSI property, ensure that it is 
                //  properly converted to unicode (NT raid# 332047).
                VARTYPE vtConverted = SHIsLegacyAnsiProperty( *pFmtID, propitem.puid.propid, &propitem.puid.vt ) ? 
                                      propitem.puid.vt : VT_NULL;
                
                //  Is this property already in our collection?
                propitem.puid.fmtid = *pFmtID;
                if( m_map.Lookup( CPropertyMap::iPropID, &propitem.puid, propitem ) )
                    continue;  // yes, blow it off

                //  No; add it to our collection with an empty value.
                if( SUCCEEDED( MakeDefPropertyItem( iDefProp, &propitem, (LPARAM)this ) ) )
                {
                    //  If this is a legacy ANSI property, ensure that it is 
                    //  properly converted to unicode (NT raid# 332047).
                    if( vtConverted != VT_NULL )
                    {
                        propitem.puid.vt = 
                        propitem.val.vt  = vtConverted;
                    }
                    ASSERT( propitem.val.vt == propitem.puid.vt );
                    PreMapPropItem( &propitem );
                    if( m_map.Insert( propitem ) )
                        cnt++;
                }
            }
        }
    }

    TRACE( TEXT("Total of %d items mapped, %d items returned\n"), (int)m_map.Count(), cnt );
    return cnt;
}

//-------------------------------------------------------------------------//
void CPropertySource::PreMapPropItem( PROPERTYITEM* pItem )
{
    // Make any last minute modifications before mapping...
    ASSERT( pItem->cbStruct == sizeof(*pItem) );
    ASSERT( m_dwFileAttr != 0xFFFFFFFF );

    //  If read-only file, remove write access on property.
    if( m_dwFileAttr & FILE_ATTRIBUTE_READONLY )
        pItem->dwAccess &= ~PTIA_WRITE;
}

//-------------------------------------------------------------------------//
BOOL CPropertySource::MapPropertyItem( PROPERTYITEM* pItem, int* pCount )
{
    if( m_map.Insert( *pItem ) )
    {
        if( pCount )
            (*pCount)++;
        return TRUE;
    }
    ClearPropertyItem( pItem );
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves image-file specific properties.
int CPropertySource::GatherImageProperties()
{
    int cnt = 0;

#ifdef _X86_

    PROPERTYITEM propitem;

    if( HasFaxProperties( &m_IflProps ) )
    {
        if( m_IflProps.faxmask & IFPF_FAXTIME )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_TIME, VT_FILETIME, 
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.filetime = m_IflProps.ftFaxTime;
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_RECIPIENTNAME )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_RECIPIENTNAME, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxRecipName, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_RECIPIENTNUMBER )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_RECIPIENTNUMBER, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxRecipNumber, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_SENDERNAME )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_SENDERNAME, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxSenderName, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_CALLERID )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_CALLERID, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxCallerID, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_TSID )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_TSID, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxTSID, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_CSID )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_CSID, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxCSID, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
        
        if( m_IflProps.faxmask & IFPF_ROUTING )
        {
            InitPropertyItem( &propitem );
            if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_FaxSummaryInformation, PIDFSI_ROUTING, VT_LPWSTR,
                                                  &propitem, (LPARAM)this ) ) )
            {
                propitem.val.pwszVal = (LPWSTR)AllocStringValueW( m_IflProps.szFaxRouting, VT_LPWSTR );
                MapPropertyItem( &propitem, &cnt );
            }
        }
    }

    //  Image File Type
    if( m_IflProps.mask & IPF_TYPE )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_FILETYPE, VT_LPWSTR,
                                              &propitem, (LPARAM)this ) ) )
        {
            //  Try bitmap version-specific string
            if( m_IflProps.type == IFLT_BMP && (m_IflProps.mask & IPF_BMPVER) != 0 )
            {
                if( (propitem.val.pwszVal = (LPWSTR)LOAD_STRRES( m_IflProps.bmpver, propitem.val.vt, iflBmpVerRes )) != NULL )
                    MapPropertyItem( &propitem, &cnt );
            }

            //  Default to image type string
            if( propitem.val.pwszVal == NULL )
            {
                if( (propitem.val.pwszVal = (LPWSTR)LOAD_STRRES( m_IflProps.type, propitem.val.vt, iflTypeStrRes )) != NULL )
                    MapPropertyItem( &propitem, &cnt );
            }
        }
    }

    //  Colorspace
    if( m_IflProps.mask & IPF_CLASS )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_COLORSPACE, VT_LPWSTR,
                                              &propitem, (LPARAM)this ) ) )
        {
            if( (propitem.val.pwszVal = (LPWSTR)LOAD_STRRES( m_IflProps.imageclass, propitem.val.vt, iflClassStrRes )) != NULL )
                MapPropertyItem( &propitem, &cnt );
        }
    }

    //  Width
    if( m_IflProps.mask & IPF_CX )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_CX, VT_UI4,
                                              &propitem, (LPARAM)this ) ) )
        {
            propitem.val.ulVal = m_IflProps.cx;
            MapPropertyItem( &propitem, &cnt );
        }
    }

    //  Height
    if( m_IflProps.mask & IPF_CY )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_CY, VT_UI4,
                                              &propitem, (LPARAM)this ) ) )
        {
            propitem.val.ulVal = m_IflProps.cy;
            MapPropertyItem( &propitem, &cnt );
        }
    }

    //  Bits per Pixel
    if( m_IflProps.mask & IPF_BPP )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_BITDEPTH, VT_UI4,
                                              &propitem, (LPARAM)this ) ) )
        {
            propitem.val.ulVal = m_IflProps.bpp;
            MapPropertyItem( &propitem, &cnt );
        }
    }

#if 0
    //  Bits per Channel
    if( m_IflProps.mask & IPF_BPC )
    {
    }
#endif

    //  Bits per Meters
    if( m_IflProps.mask & IPF_DPMX )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_RESOLUTIONX, VT_UI4,
                                              &propitem, (LPARAM)this ) ) )
        {
            propitem.val.ulVal = m_IflProps.dpmX;
            MapPropertyItem( &propitem, &cnt );
        }
    }

    if( m_IflProps.mask & IPF_DPMY )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_RESOLUTIONY, VT_UI4,
                                              &propitem, (LPARAM)this ) ) )
        {
            propitem.val.ulVal = m_IflProps.dpmY;
            MapPropertyItem( &propitem, &cnt );
        }
    }

#if 0
    //  Internal Image Count
    if( m_IflProps.mask & IPF_IMAGECOUNT )
    {
    }

    //  Tiling format
    if( m_IflProps.mask & IPF_TILEFMT )
    {
    }

    //  Line sequence
    if( m_IflProps.mask & IPF_LINESEQ )
    {
    }
#endif

    //  Compression
    if( m_IflProps.mask & IPF_COMPRESSION )
    {
        InitPropertyItem( &propitem );
        if( SUCCEEDED( MakeDefPropertyItemEx( FMTID_ImageSummaryInformation, PIDISI_COMPRESSION, VT_LPWSTR,
                                              &propitem, (LPARAM)this ) ) )
        {
            if( (propitem.val.pwszVal = (LPWSTR)LOAD_STRRES( m_IflProps.compression, propitem.val.vt, iflCompressionStrRes )) != NULL )
                MapPropertyItem( &propitem, &cnt );
        }
    }

#if 0
    //  Internal timestamp
    if( m_IflProps.mask & IPF_DATETIME )
    {
    }
#endif

#endif _X86_

    return cnt;
}


//-------------------------------------------------------------------------//
//  Utility method; obtains an IPropertySetStorage interface pointer on
//  the specified file, and updates attribute and state information
//  for the source.
HRESULT CPropertySource::OpenPropertySetStorage(
    LPCWSTR pwszFileName,
    OUT     IPropertySetStorage** ppPSS,
    IN      ULONG  dwWriteMode,
    IN      ULONG  dwShareMode,
    IN OUT  ULONG* pdwSrcType )
{
    HRESULT     hr = E_FAIL;
    if( ppPSS ) *ppPSS = NULL;

    //  Validate arguments
    if( !( pwszFileName && *pwszFileName && pdwSrcType && ppPSS ) )
        return E_INVALIDARG;

    SETSTGTYPE( pdwSrcType, PST_NOSTG );
    if( GetFileAttributesW( pwszFileName )==0xFFFFFFFF )
        return STG_E_FILENOTFOUND;

    //  Try opening as NTFS 5.0+ flat file with property sets
    hr = SHStgOpenStorage( pwszFileName, STGM_DIRECT|dwWriteMode|dwShareMode,
                           0, 0, IID_IPropertySetStorage, (void**)ppPSS );
    if( SUCCEEDED( hr ) )
    {
        if( IsOfficeDocFile( pwszFileName ) )
        {
            SETSTGTYPE( pdwSrcType, PST_OLESS );
            SETDOCTYPE( pdwSrcType, PST_DOCFILE );
        }
        SETSTGTYPE( pdwSrcType, PST_NSS );
    }

    if( FAILED( hr ) )
        *ppPSS        = NULL;

    return hr;
}

//-------------------------------------------------------------------------//
//  Utility method; opens an IPropertyStorage interface pointer on
//  the source's cached IPropertySetStorage interface pointer.
HRESULT CPropertySource::OpenPropertyStorage(
    IPropertySetStorage* pPSS,
    REFFMTID reffmtid,
    ULONG dwWriteMode,
    ULONG dwDisposition,
    IPropertyStorage** ppStg,
    UINT* puCodePage )
{
    ASSERT( pPSS );
    ASSERT( ppStg );

    //  Use same read/write mode as set, but property stream
    //  must be opened w/ exclusive share mode.
    HRESULT hr = SHPropStgCreate( pPSS, reffmtid, NULL, PROPSETFLAG_DEFAULT,
                                  STGM_DIRECT|dwWriteMode|STGM_SHARE_EXCLUSIVE,
                                  dwDisposition, ppStg, puCodePage );
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CPropertySource::Persist( const PROPERTYITEM& item )
{
    HRESULT hr;
    if( !m_pPSS )
    {
        ULONG dwSrcType = m_dwSrcType;
        if( FAILED( (hr = OpenPropertySetStorage( m_szStgName, &m_pPSS,
                                                  STGM_READWRITE, STGM_SHARE_EXCLUSIVE, &m_dwSrcType )) ) )
            return hr;

        m_dwStgWriteMode = STGM_READWRITE;
        m_dwStgShareMode = STGM_SHARE_EXCLUSIVE;
        m_dwSrcType      = dwSrcType;
    }

    //  First, try to open the property stream
    IPropertyStorage* pStg = NULL;
    UINT uCodePage = 0;
    if( FAILED( (hr = OpenPropertyStorage( m_pPSS, item.puid.fmtid,
                                           m_dwStgWriteMode, OPEN_ALWAYS, &pStg, &uCodePage )) ) )
        return hr;

    PROPSPEC spec;
    spec.ulKind = PRSPEC_PROPID;
    spec.propid = item.puid.propid;

    PROPVARIANT val;
    PropVariantInit( &val );
    PropVariantCopy( &val, &item.val );

    ASSERT( val.vt == item.puid.vt );

    //  Convert back to universal file time before saving
    if( val.vt==VT_FILETIME )
    {
        FILETIME ft = val.filetime;
        LocalFileTimeToFileTime( &ft, &val.filetime );
    }

    PROPVARIANT valMru;
    PropVariantInit( &valMru );
    PropVariantCopy( &valMru, &val ); 

    //  Write the value
    if( SUCCEEDED( (hr = SHPropStgWriteMultiple( pStg, &uCodePage, 1, &spec, &val, PID_FIRST_USABLE )) ) )
    {
        // Update MRU if applicable.
        if( item.dwFlags & PTPIF_MRU )
        {
            CPropMruStor stor( PTREGROOTKEY, PTREGSUBKEY );
            stor.PushMruVal( item.puid.fmtid, item.puid.propid, valMru.vt, valMru );
        }
    }

    PropVariantClear( &valMru );
    PropVariantClear( &val );
    pStg->Release();

    return hr;
}

//-------------------------------------------------------------------------//
VOID CPropertySource::Close( BOOL bPermanent )
{
    if( m_pPSS )
    {
        m_pPSS->Release();
        m_pPSS = NULL;
    }

    m_dwStgWriteMode = 0L;
    m_dwStgShareMode = 0L;
    m_map.Clear();

    if( bPermanent )
    {
        m_dwSrcType = 0;
        memset( m_szStgName, 0, sizeof(m_szStgName) );
    }
}

//-------------------------------------------------------------------------//
ULONG CPropertyMap::HashValue( UCHAR iKey, const PROPERTYITEM& val ) const
{
    //  Returns the hash value for the indicated key based on the provided value element,
    //  or (ULONG)-1 if the hash value cannot be generated.
    switch( iKey )
    {
        case iPropID:   return HashBytes( &val.puid, sizeof(val.puid) );
        case iFolderID: return HashBytes( &val.pfid, sizeof(val.pfid) );
        case iShColID:  return HashBytes( &val.puid.fmtid, sizeof(val.puid.fmtid) ) +
                               val.puid.propid;
    }
    return (ULONG)-1;
}

//-------------------------------------------------------------------------//
ULONG CPropertyMap::HashKey( UCHAR iKey, const void* pvKey ) const
{
    //  Returns the hash value for the indicated key based on the provided key value.
    //  or (ULONG)-1 if the hash value cannot be generated.
    switch( iKey )
    {
        case iPropID:   return HashBytes( (PUID*)pvKey, sizeof(PUID) );
        case iFolderID: return HashBytes( (PFID*)pvKey, sizeof(PFID) );
        case iShColID:  {
            if( pvKey )
            {
                SHCOLUMNID* pscid = (SHCOLUMNID*)pvKey;
                return HashBytes( &pscid->fmtid, sizeof(pscid->fmtid) ) +
                       pscid->pid;
            }
        }
    }
    return (ULONG)-1;
}

//-------------------------------------------------------------------------//
int CPropertyMap::Compare( UCHAR iKey, const void* pvKey, const PROPERTYITEM& val ) const
{
    //  Returns -1 if the key's value is less than that provided by the value element, 1 if
    //  the key's value is greater that provided by the provided element, or 0 if they are equal.
    switch( iKey )
    {
        case iPropID:
            return memcmp( pvKey, &val.puid, sizeof(val.puid) );

        case iFolderID:
            return memcmp( pvKey, &val.pfid, sizeof(val.pfid) );

        case iShColID:
            return pvKey ? memcmp( pvKey, &val.puid, sizeof(SHCOLUMNID) ) : -1;
    }
    return 0;
}

//-------------------------------------------------------------------------//
//  Should return the address of the indicated key's value based on the
//  provided value element, or NULL if the address cannot be determined.
PVOID CPropertyMap::GetKey( UCHAR iKey, const PROPERTYITEM& val ) const
{
    switch( iKey )
    {
        case iPropID:   return (PVOID)&val.puid;
        case iFolderID: return (PVOID)&val.pfid;
        case iShColID:  return (SHCOLUMNID*)&val.puid;
    }
    return NULL;
}

//-------------------------------------------------------------------------//
BOOLEAN CPropertyMap::AllowDuplicates( UCHAR iKey ) const
{
    return (iKey == iFolderID) ? (BOOLEAN)TRUE : (BOOLEAN)FALSE;
}

//-------------------------------------------------------------------------//
void CPropertyMap::OnDelete( PROPERTYITEM& val )
{
    //  BUGBUG!  The following line should be uncommented, but doing so
    //  now causes an AV.  Probably because IEnumPROPERTYITEM->Next() function
    //  doesn't properly copy out the item.

    //ClearPropertyItem( &val );
}
