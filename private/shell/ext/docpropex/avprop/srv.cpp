//-------------------------------------------------------------------------//
// srv.cpp : Implementation of CAVFilePropServer
//-------------------------------------------------------------------------//
#include "pch.h"
#include "srv.h"
#include "propvar.h"
#include "ptsniff.h"

//-------------------------------------------------------------------------//
LPCTSTR szWAVFILE_EXT   = TEXT(".wav"),
        szAVIFILE_EXT   = TEXT(".avi"),
        szMIDIFILE_EXT  = TEXT(".mid") ;

//-------------------------------------------------------------------------//
//  raw property definition block
//
typedef struct tagFOLDER_DEF
{
    LPTSTR  pszName;
    UINT    nIdsDisplay;
    UINT    nIdsQtip;
    ULONG   dwAccess;
    ULONG   iOrder;
    LONG    lParam;
    const PFID* ppfid ;
    ULONG   dwFlags;

} FOLDER_DEF, *PFOLDER_DEF;

//-------------------------------------------------------------------------//
//  Avprop folder definitions.
//  The ordering of members MUST be kept in synch with that of 
//  AV_MEDIA_TYPE enum members.
static const FOLDER_DEF folders[] = 
{
    {   NULL, 0,0,0,0,0,&PFID_NULL,0 },
    
    {   TEXT("waveaudio"), IDS_WAVEAUDIO_FOLDER, IDS_WAVEAUDIO_FOLDER_TIP,
        PTIA_READONLY, 0, 0, &PFID_AudioProperties, 0 },

    {   TEXT("wideo"), IDS_VIDEO_FOLDER, IDS_VIDEO_FOLDER_TIP,
        PTIA_READONLY, 0, 0, &PFID_VideoProperties, 0 },
        
    {   TEXT("midi"), IDS_MIDI_FOLDER, IDS_MIDI_FOLDER_TIP,
        PTIA_READONLY, 0, 0, &PFID_MidiProperties, 0 },
} ;

//-------------------------------------------------------------------------//
//  raw property definition block
//
typedef struct tagPROPERTY_DEF
{
    LPCTSTR      pszName ;
    UINT         nIdsDisplay ;
    UINT         nIdsTip ;
    const FMTID* pfmtid ;
    PROPID       pid ;
    BOOL         bShellColumn ;

} PROPERTY_DEF, *PPROPERTY_DEF ;

//-------------------------------------------------------------------------//
//  Avprop property definitions.

static const PROPERTY_DEF audiopropdefs[] = 
{
    { TEXT("Format"), IDS_PIDASI_FORMAT, IDS_PIDASI_FORMAT_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_FORMAT, TRUE },

    { TEXT("TimeLength"), IDS_PIDASI_TIMELENGTH, IDS_PIDASI_TIMELENGTH_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_TIMELENGTH, FALSE },

    { TEXT("AvgDataRate"), IDS_PIDASI_AVG_DATA_RATE, IDS_PIDASI_AVG_DATA_RATE_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_AVG_DATA_RATE, FALSE },

    { TEXT("SampleRate"), IDS_PIDASI_SAMPLE_RATE, IDS_PIDASI_SAMPLE_RATE_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_SAMPLE_RATE, TRUE },

    { TEXT("SampleSize"), IDS_PIDASI_SAMPLE_SIZE, IDS_PIDASI_SAMPLE_SIZE_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_SAMPLE_SIZE, TRUE },

    { TEXT("ChannelCount"), IDS_PIDASI_CHANNEL_COUNT, IDS_PIDASI_CHANNEL_COUNT_TIP,
      &FMTID_AudioSummaryInformation, PIDASI_CHANNEL_COUNT, TRUE },
} ;

static const PROPERTY_DEF videopropdefs[] = 
{
    { TEXT("StreamName"), IDS_PIDVSI_STREAM_NAME, IDS_PIDVSI_STREAM_NAME_TIP, 
      &FMTID_VideoSummaryInformation, PIDVSI_STREAM_NAME },

    { TEXT("FrameWidth"), IDS_PIDVSI_FRAME_WIDTH, IDS_PIDVSI_FRAME_WIDTH_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_FRAME_WIDTH, FALSE },

    { TEXT("FrameHeight"), IDS_PIDVSI_FRAME_HEIGHT, IDS_PIDVSI_FRAME_HEIGHT_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_FRAME_HEIGHT, FALSE },

    { TEXT("TimeLength"), IDS_PIDVSI_TIMELENGTH, IDS_PIDVSI_TIMELENGTH_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_TIMELENGTH, TRUE },

    { TEXT("FrameCount"), IDS_PIDVSI_FRAME_COUNT, IDS_PIDVSI_FRAME_COUNT_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_FRAME_COUNT, TRUE },

    { TEXT("FrameRate"), IDS_PIDVSI_FRAME_RATE, IDS_PIDVSI_FRAME_RATE_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_FRAME_RATE, TRUE },

    { TEXT("DataRate"), IDS_PIDVSI_DATA_RATE, IDS_PIDVSI_DATA_RATE_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_DATA_RATE, FALSE },

    { TEXT("SampleSize"), IDS_PIDVSI_SAMPLE_SIZE, IDS_PIDVSI_SAMPLE_SIZE_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_SAMPLE_SIZE, TRUE },

    { TEXT("Compression"), IDS_PIDVSI_COMPRESSION, IDS_PIDVSI_COMPRESSION_TIP,
      &FMTID_VideoSummaryInformation, PIDVSI_COMPRESSION, TRUE },
} ;

//-------------------------------------------------------------------------//
//  fn forwards...
static HRESULT PTPropertyItemFromPropDef( const PROPERTY_DEF* ppd, IN OUT PPROPERTYITEM pItem ) ;
static void    CopyPTPropertyItem( OUT PPROPERTYITEM pDest, IN const PPROPERTYITEM pSrc ) ;
static void    ClearPTPropertyItem( struct tagPROPERTYITEM* pItem ) ;

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::RegisterFileAssocs()
{
    HRESULT hr[3], hrRet ; 
     
    hr[0] = RegisterPropServerClassForExtension( szWAVFILE_EXT, CLSID_AVFilePropServer ) ;
    hr[1] = RegisterPropServerClassForExtension( szAVIFILE_EXT, CLSID_AVFilePropServer ) ;
    hr[2] = RegisterPropServerClassForExtension( szMIDIFILE_EXT, CLSID_AVFilePropServer ) ;

    for( int i = 0; i < ARRAYSIZE(hr); i++ )
    {
        if( FAILED( hr[i] ) )
            return hr[i] ;
    }
    return S_OK ;
}

//-------------------------------------------------------------------------//
//  class CAVFilePropServer : IAdvancedPropertyServer.
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::QueryServerInfo( 
    IN OUT PROPSERVERINFO *pInfo )
{
    return E_NOTIMPL ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::AcquireAdvanced(
    IN const VARIANT *pvarSrc,
    OUT LPARAM *plParamSrc )
{
    HRESULT hr ;
    CAVPropSrc* pSrc ;
    
    //  Instantiate a data object to attach to the property source (AV file)
    if( FAILED( (hr = CAVPropSrc::CreateInstance( pvarSrc, &pSrc )) ) )
        return hr ;

    //  Accumulate property values in the object.
    if( FAILED( (hr = pSrc->Acquire()) ) )
    {
        delete pSrc ;
        return hr ;
    }

    *plParamSrc = (LPARAM)pSrc ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::EnumFolderItems( 
    IN LPARAM lParamSrc,
    OUT IEnumPROPFOLDERITEM **ppEnum )
{
    //  Pass the source's data object to the new enumerator
    CAVPropSrc* pSrc = (CAVPropSrc*)lParamSrc ;
    
    if( ((*ppEnum) = new CEnumAVFolderItem( pSrc ))==NULL )
        return E_OUTOFMEMORY ;

    (*ppEnum)->AddRef() ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::EnumPropertyItems( 
    IN const PROPFOLDERITEM *pFolderItem,
    OUT IEnumPROPERTYITEM **ppEnum )
{
    //  The source's data object comes through PROPFOLDERITEM:lParam.
    if( ((*ppEnum) = new CEnumAVPropertyItem( *pFolderItem ))==NULL )
        return E_OUTOFMEMORY ;

    (*ppEnum)->AddRef() ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::EnumValidValues( 
    IN const PROPERTYITEM *pItem,
    OUT IEnumPROPVARIANT_DISPLAY **ppEnum )
{
    return E_NOTIMPL ;  // all of our properties are read-only.
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::PersistAdvanced( 
    IN OUT PROPERTYITEM *pItem )
{
    return E_NOTIMPL ;  // all of our properties are read-only.
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVFilePropServer::ReleaseAdvanced( 
    IN LPARAM lParamSrc,
    IN ULONG dwFlags )
{
    CAVPropSrc* pSrc = (CAVPropSrc*)lParamSrc ;

    if ( dwFlags & PTREL_DONE_ENUMERATING )
    {
        // Throw away any data associated with folder/property 
        // that enumeration that consumes memory or working set.
    }

    if( dwFlags & PTREL_CLOSE_SOURCE )
    {
        // Close the file until further notice.
    }

    if( dwFlags == PTREL_SOURCE_REMOVED )
    {
        // Destroy our source data object
        delete pSrc ;
    }

    return S_OK ;
}

//-------------------------------------------------------------------------//
//  Enumerator implementation
//-------------------------------------------------------------------------//

//  common enumerator IUnknown implementation:
#define IMPLEMENT_ENUM_UNKNOWN( classname, iid ) \
    ULONG classname::m_cObj = 0 ; \
    STDMETHODIMP classname::QueryInterface( REFIID _iid, void** ppvObj ) { \
        if( ppvObj == NULL ) return E_POINTER ; *ppvObj = NULL ; \
        if( IsEqualGUID( _iid, IID_IUnknown ) )   *ppvObj = this ; \
        else if( IsEqualGUID( _iid, iid ) ) *ppvObj = this ; \
        if( *ppvObj )    { AddRef() ; return S_OK ;  } \
        return E_NOINTERFACE ; } \
    STDMETHODIMP_( ULONG ) classname::AddRef() { \
        if( InterlockedIncrement( (LONG*)&m_cRef )==1 )  \
            _Module.Lock() ; \
        return m_cRef ; } \
    STDMETHODIMP_( ULONG ) classname::Release() { \
        if( InterlockedDecrement( (LONG*)&m_cRef )==0 ) \
            { _Module.Unlock() ; delete this ; return 0 ; } \
        return m_cRef ;  } \

//-------------------------------------------------------------------------//
// CEnumAVFolderItem
IMPLEMENT_ENUM_UNKNOWN( CEnumAVFolderItem, IID_IEnumPROPFOLDERITEM ) ;
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumAVFolderItem::Next( ULONG cItems, PROPFOLDERITEM * rgFolderItem, ULONG * pcItemsFetched )
{
    if( !( pcItemsFetched && rgFolderItem ) ) 
        return E_POINTER ;
    *pcItemsFetched = 0 ;

    if( cItems <=0 )
        return E_INVALIDARG ;

    for( ; m_iPos < ARRAYSIZE(folders) && *pcItemsFetched < cItems; InterlockedIncrement( &m_iPos ) )
    {
        if( m_pSrc->IsSupportedFolderID( *folders[m_iPos].ppfid ) )
        {
            USES_CONVERSION ;

            rgFolderItem[*pcItemsFetched].cbStruct    = sizeof(*rgFolderItem) ;
            rgFolderItem[*pcItemsFetched].mask        = PTFI_ALL ;
            rgFolderItem[*pcItemsFetched].bstrName    =     SysAllocString( T2W( folders[m_iPos].pszName ) ) ;
            rgFolderItem[*pcItemsFetched].bstrDisplayName = BstrFromResource( folders[m_iPos].nIdsDisplay ) ;
            rgFolderItem[*pcItemsFetched].bstrQtip        = BstrFromResource( folders[m_iPos].nIdsQtip ) ;
            rgFolderItem[*pcItemsFetched].dwAccess    = folders[m_iPos].dwAccess ;
            rgFolderItem[*pcItemsFetched].iOrder      = folders[m_iPos].iOrder ;
            rgFolderItem[*pcItemsFetched].lParam      = (LPARAM)m_pSrc ;
            rgFolderItem[*pcItemsFetched].pfid        = *folders[m_iPos].ppfid ;
            rgFolderItem[*pcItemsFetched].dwFlags     = folders[m_iPos].dwFlags ;

            InterlockedIncrement( (LONG*)pcItemsFetched ) ;
        }
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
// CEnumAVPropertyItem
IMPLEMENT_ENUM_UNKNOWN( CEnumAVPropertyItem, IID_IEnumPROPERTYITEM ) ;
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumAVPropertyItem::Next( 
    ULONG cItems, 
    PROPERTYITEM * rgPropertyItem, 
    ULONG * pcItemsFetched )
{
    HRESULT hr ;

    if( !( pcItemsFetched && rgPropertyItem ) ) 
        return E_POINTER ;

    *pcItemsFetched = 0 ;
    while( *pcItemsFetched < cItems )
    {
        hr = m_pSrc->GetPropertyItem( m_pfid, m_iPos, &rgPropertyItem[*pcItemsFetched] ) ;
        InterlockedIncrement( &m_iPos ) ;

        if( S_OK == hr )
            (*pcItemsFetched)++ ;
        else
            break ;
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
//  CAVPropSrc - base class for property source data object.
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CAVPropSrc::CreateInstance( 
    const VARIANT* pvarSrc, 
    OUT CAVPropSrc** ppSrc )
{
    TCHAR szFile[MAX_PATH] ;
    USES_CONVERSION ;

    ASSERT( ppSrc ) ;
    *ppSrc = NULL ;

    if( VT_BSTR != pvarSrc->vt )
        return E_NOTIMPL ;

    lstrcpyn( szFile, W2T(pvarSrc->bstrVal), ARRAYSIZE(szFile) ) ;
    LPCTSTR pszExt = PathFindExtension( szFile ) ;

    HRESULT hr ;
    if( FAILED( (hr = CreateInstance( pszExt, ppSrc )) ) )
        return hr ;

    (*ppSrc)->Attach( szFile ) ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAVPropSrc::CreateInstance( LPCTSTR pszExt, OUT CAVPropSrc** ppSrc )
{
    //  Instantiate the correct class of data object based on
    //  the file extension.
    if( pszExt )
    {
        if( 0 == lstrcmpi( pszExt, szWAVFILE_EXT ) )
            *ppSrc = new CWaveSrc ;
        else if( 0 == lstrcmpi( pszExt, szAVIFILE_EXT ) )
            *ppSrc = new CAviSrc ;
        else if( 0 == lstrcmpi( pszExt, szMIDIFILE_EXT ) )
            *ppSrc = new CMidiSrc ;
        else
            return E_FAIL ;
    }
    return ( *ppSrc ) ? S_OK : E_OUTOFMEMORY ;
}

//-------------------------------------------------------------------------//
void CAVPropSrc::Attach( LPCTSTR pszFile )
{
    lstrcpyn( _szFile, pszFile, ARRAYSIZE(_szFile) ) ;
}

//-------------------------------------------------------------------------//
//  helper: pre-allocates a member array of PROPERTYITEMs
PPROPERTYITEM CAVPropSrc::AllocItems( int nCount )
{
    PPROPERTYITEM rgPropItems ;
    if( NULL != (rgPropItems = new PROPERTYITEM[nCount]) )
    {
        FreeItems() ;

        ZeroMemory( rgPropItems, sizeof(*rgPropItems) * nCount ) ;
        _rgPropItems = rgPropItems ;
        _cPropItems = nCount ;
        return _rgPropItems ;
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
//  helper: frees member array of PROPERTYITEMs
void CAVPropSrc::FreeItems()   
{
    if( _rgPropItems )
    {
        for( int i=0; i<_cPropItems; i++ )
            PropVariantClear( &_rgPropItems[i].val ) ;

        delete [] _rgPropItems ; 
        _rgPropItems = NULL ; 
        _cPropItems = 0 ; 
    }
}

//-------------------------------------------------------------------------//
//  retrieves the specified element of the member PROPERTYITEM array.
STDMETHODIMP CAVPropSrc::GetPropertyItem( 
    IN REFPFID pfid, 
    IN LONG iProperty, 
    OUT PPROPERTYITEM pItem )
{
    ASSERT( iProperty >= 0 ) ;

    if( iProperty >= _cPropItems )
        return S_FALSE ;

    CopyPTPropertyItem( pItem, &_rgPropItems[iProperty] ) ;
    return S_OK ;    
}

STDMETHODIMP CAVPropSrc::GetPropertyValue( LPCSHCOLUMNID pscid, OUT VARIANT* pVal )
{
    for( int i=0; i < (int)_cPropItems; i++ )
    {
        if( pscid->pid == _rgPropItems[i].puid.propid &&
            IsEqualGUID( pscid->fmtid, _rgPropItems[i].puid.fmtid ) )
        {
            return PropVariantToVariant( &_rgPropItems[i].val, pVal ) ;
        }
    }
    return E_UNEXPECTED ;
}


//-------------------------------------------------------------------------//
//  CWaveSrc : public CAVPropSrc   - wave audio file data object
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CWaveSrc::Acquire()   
{ 
    WAVEDESC wave ;
    ZeroMemory( &wave, sizeof(wave) ) ;

    HRESULT hr = GetWaveInfo( _szFile, &wave ) ;

    if( SUCCEEDED( hr ) )
    {
        //  Allocate an array of PROPERTYITEM blocks
        PPROPERTYITEM rgitems = AllocItems( ARRAYSIZE(audiopropdefs) ) ;
        if( !rgitems )
            return E_OUTOFMEMORY ;

        for( int i=0, cnt=0; i< ARRAYSIZE(audiopropdefs); i++ )
        {
            
            
            //  Initialize 
            if( SUCCEEDED( PTPropertyItemFromPropDef( audiopropdefs + i, rgitems + cnt ) ) )
            {
                rgitems[cnt].iOrder = cnt ;
                rgitems[cnt].pfid   = PFID_AudioProperties ;

                //  Stuff values
                if( SUCCEEDED( GetWaveProperty( *audiopropdefs[i].pfmtid, audiopropdefs[i].pid, 
                                                &wave, &rgitems[cnt].val ) ) )
                {
                    rgitems[cnt].puid.vt = rgitems[cnt].val.vt ;
                    cnt++ ;
                }
                else
                    ClearPTPropertyItem( &rgitems[cnt] ) ; // don't leak these BSTRs.
            }
        }
        _cPropItems = cnt ;
        FreeWaveInfo( &wave ) ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
//  CAviSrc : public CAVPropSrc  - AVI file data object
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CAviSrc::Acquire()   
{ 
    AVIDESC avi ;
    ZeroMemory( &avi, sizeof(avi) );

    HRESULT hr = GetAviInfo( _szFile, &avi ) ;

    if( SUCCEEDED( hr ) )
    {
        //  Allocate an array of PROPERTYITEM blocks
        PPROPERTYITEM rgitems = AllocItems( ARRAYSIZE(videopropdefs) + ARRAYSIZE(audiopropdefs) ) ;
        if( !rgitems )
            return E_OUTOFMEMORY ;

        int i, cnt = 0 ;

        for( i=0; i< ARRAYSIZE(videopropdefs); i++ )
        {
            //  Initialize 
            if( SUCCEEDED( PTPropertyItemFromPropDef( videopropdefs + i, rgitems + cnt ) ) )
            {
                rgitems[cnt].iOrder = cnt ;
                rgitems[cnt].pfid   = PFID_VideoProperties ;

                //  Stuff values
                if( SUCCEEDED( GetAviProperty( *videopropdefs[i].pfmtid, videopropdefs[i].pid, 
                                                &avi, &rgitems[cnt].val ) ) )
                {
                    rgitems[cnt].puid.vt = rgitems[cnt].val.vt ;
                    m_bVideoStream = TRUE ;
                    cnt++ ;
                }
                else
                    ClearPTPropertyItem( rgitems + cnt ) ; // don't leak
            }
        }

        for( i=0; i< ARRAYSIZE(audiopropdefs); i++ )
        {
            //  Initialize 
            if( SUCCEEDED( PTPropertyItemFromPropDef( audiopropdefs + i, rgitems + cnt ) ) )
            {
                rgitems[cnt].iOrder = cnt ;
                rgitems[cnt].pfid   = PFID_AudioProperties ;

                //  Stuff values
                if( SUCCEEDED( GetAviProperty( *audiopropdefs[i].pfmtid, audiopropdefs[i].pid, 
                                                &avi, &rgitems[cnt].val ) ) )
                {
                    rgitems[cnt].puid.vt = rgitems[cnt].val.vt ;
                    m_bAudioStream = TRUE ;
                    cnt++ ;
                }
                else
                    ClearPTPropertyItem( rgitems + cnt ) ; // don't leak
            }
        }
        
        _cPropItems = cnt ;
        FreeAviInfo( &avi ) ;
    }

    return hr ;
}

//-------------------------------------------------------------------------//
//  CMidiSrc : public CAVPropSrc   - MIDI file data object
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
STDMETHODIMP CMidiSrc::Acquire()   
{ 
    return GetMidiInfo( _szFile, &_midi ) ;
}


//-------------------------------------------------------------------------//
//  Utilities 
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
BSTR BstrFromResource( UINT nIDS )
{
    WCHAR   wszBuf[MAX_DESCRIPTOR] ;
     
    *wszBuf = NULL ;

    if( !LoadStringW( _Module.GetResourceInstance(), nIDS, wszBuf, ARRAYSIZE(wszBuf) ) )
        return NULL ;

    return SysAllocString( wszBuf ) ;
}

//-------------------------------------------------------------------------//
HRESULT PTPropertyItemFromPropDef( const PROPERTY_DEF* ppd, IN OUT PPROPERTYITEM pItem )
{
    HRESULT hr = S_OK ;
    USES_CONVERSION ;

    ZeroMemory( pItem, sizeof(*pItem) ) ;

    pItem->cbStruct     = sizeof(*pItem) ;
    pItem->mask         = PTPI_ALL ;
    pItem->bstrName     = SysAllocString( T2W((LPTSTR)ppd->pszName) ) ; 
    pItem->bstrDisplayName = BstrFromResource( ppd->nIdsDisplay ) ;
    pItem->bstrQtip     = BstrFromResource( ppd->nIdsTip ) ;
    pItem->dwAccess     = PTIA_READONLY ;
    pItem->iOrder       = -1 ;
    //pItem->lParam       = NULL;
    pItem->puid.fmtid   = *ppd->pfmtid ;
    pItem->puid.propid  = ppd->pid ;
    
    //pItem->dwFolderID 
    //pItem->dwFlags;
    //pItem->ctlID;
    //pItem->bstrDisplayFmt;
    //pItem->val;
    return hr ;
}

//-------------------------------------------------------------------------//
void CopyPTPropertyItem( OUT PPROPERTYITEM pDest, IN const PPROPERTYITEM pSrc )
{
    ZeroMemory( pDest, sizeof(*pDest) ) ;
    *pDest = *pSrc ;

    // deep copy...
    pDest->bstrName = ( pSrc->bstrName ) ? SysAllocString( pSrc->bstrName ) : NULL ;
    pDest->bstrDisplayName = ( pSrc->bstrDisplayName ) ? SysAllocString( pSrc->bstrDisplayName ) : NULL ;
    pDest->bstrQtip = ( pSrc->bstrQtip ) ? SysAllocString( pSrc->bstrQtip ) : NULL ;
    
    PropVariantInit( &pDest->val ) ;
    PropVariantCopy( &pDest->val, &pSrc->val ) ;
}

#define SAFE_SYSFREESTRING( bstr )  if((bstr)){ SysFreeString((bstr)); (bstr)=NULL; }

//-------------------------------------------------------------------------//
void ClearPTPropertyItem( struct tagPROPERTYITEM* pItem )
{
    if( pItem != NULL && sizeof(*pItem) == pItem->cbStruct )
    {
        SAFE_SYSFREESTRING( pItem->bstrName ) ;
        SAFE_SYSFREESTRING( pItem->bstrDisplayName ) ;
        SAFE_SYSFREESTRING( pItem->bstrQtip ) ;
        PropVariantClear( &pItem->val ) ;

        ZeroMemory( pItem, sizeof(*pItem) ) ;
    }
}


// {884EA37B-37C0-11d2-BE3F-00A0C9A83DA1}
static const GUID CLSID_AVColumnProvider = 
{ 0x884ea37b, 0x37c0, 0x11d2, { 0xbe, 0x3f, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

//-------------------------------------------------------------------------//
STDMETHODIMP CAVColumnProvider::Initialize( LPCSHCOLUMNINIT psci )
{
    return *psci->wszFolder ? S_OK : E_FAIL ;   // only file-system folders
}

//-------------------------------------------------------------------------//
typedef struct {
    const PROPERTY_DEF* prgPropDef ;
    int iPropDef ;
} COLUMN_INDEX ;

STDMETHODIMP CAVColumnProvider::GetColumnInfo( DWORD dwIndex, LPSHCOLUMNINFO psci )
{
    static COLUMN_INDEX columnIndices[ARRAYSIZE(audiopropdefs) + ARRAYSIZE(videopropdefs)] ;
    static int cnt = 0 ;

    const PROPERTY_DEF* prgPropDef = NULL ;
    int i ;

    ZeroMemory(psci, sizeof(*psci));

    //  Initialize our index
    if( 0 == cnt )
    {
        for( i = 0; i < ARRAYSIZE(audiopropdefs); i++ )
        {
            if( audiopropdefs[i].bShellColumn )
            {
                columnIndices[cnt].prgPropDef = audiopropdefs ;
                columnIndices[cnt].iPropDef   = i ;
                cnt++ ;
            }
        }        
        for( i = 0; i < ARRAYSIZE(videopropdefs); i++ )
        {
            if( videopropdefs[i].bShellColumn )
            {
                columnIndices[cnt].prgPropDef = videopropdefs ;
                columnIndices[cnt].iPropDef   = i ;
                cnt++ ;
            }
        }        
    }

    if( 0 == cnt || cnt <= (int)dwIndex )
        return S_FALSE ;

    //  Adjust index
    prgPropDef = columnIndices[dwIndex].prgPropDef ;
    i = columnIndices[dwIndex].iPropDef ;

    psci->scid.fmtid = *prgPropDef[i].pfmtid ;
    psci->scid.pid   =  prgPropDef[i].pid ;

    psci->vt       = VT_BSTR ;
    psci->fmt      = LVCFMT_LEFT ;
    psci->cChars   = 30 ;
    psci->csFlags = SHCOLSTATE_TYPE_STR;
    LoadStringW(_Module.GetResourceInstance(), prgPropDef[i].nIdsDisplay,
                 psci->wszTitle, ARRAYSIZE(psci->wszTitle) ) ;

    return S_OK ;
}

STDMETHODIMP CAVColumnProvider::GetItemData( LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, LPVARIANT pvarData )
{
    USES_CONVERSION ;
    HRESULT hr = E_UNEXPECTED ;
    
    AcquireMutex() ;

    if( (pscd->dwFlags & SHCDF_UPDATEITEM) || (m_pSrc && lstrcmpi( m_pSrc->Path(), W2CT(pscd->wszFile) ) != 0 ))
        Flush() ;

    if( NULL == m_pSrc )
    {
        if( SUCCEEDED( (hr = CAVPropSrc::CreateInstance( W2CT(pscd->pwszExt), &m_pSrc )) ) )
        {
            m_pSrc->Attach( W2CT(pscd->wszFile) ) ;
            
            if( FAILED( (hr = m_pSrc->Acquire()) ) )
                Flush() ;
        }
    }

    if( m_pSrc )
        hr = m_pSrc->GetPropertyValue( pscid, pvarData ) ;

    ReleaseMutex() ;    
    
    return hr ;
}
