//-------------------------------------------------------------------------//
// srv.h : CAVFilePropServer decl
//-------------------------------------------------------------------------//


#ifndef __AVFILEPROPSERVER_H_
#define __AVFILEPROPSERVER_H_

#include "resource.h"       // main symbols
#include "avprop.h"
#include "media.h"
#include "ptserver.h"

//-------------------------------------------------------------------------//
//  Types, constants

enum AV_MEDIA_TYPE  // media type identifiers
{
    MT_ERROR,
    MT_WAVE,
    MT_MIDI,
    MT_AVI,
} ;

//-------------------------------------------------------------------------//
//  Globals
EXTERN_C AV_MEDIA_TYPE  MediaTypeFromFileName( IN LPCTSTR pszPath ) ;
EXTERN_C BSTR           BstrFromResource( UINT nIDS ) ;

//-------------------------------------------------------------------------//
// CAVFilePropServer
class ATL_NO_VTABLE CAVFilePropServer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAVFilePropServer, &CLSID_AVFilePropServer>,
	public IAdvancedPropertyServer
//-------------------------------------------------------------------------//
{
public:
	CAVFilePropServer() {}
    virtual ~CAVFilePropServer() {}
    static STDMETHODIMP RegisterFileAssocs() ;

    DECLARE_REGISTRY_RESOURCEID(IDR_AVFILEPROPSERVER)
    DECLARE_NOT_AGGREGATABLE(CAVFilePropServer)

    BEGIN_COM_MAP(CAVFilePropServer)
	    COM_INTERFACE_ENTRY(IAdvancedPropertyServer)
    END_COM_MAP()

public:
    STDMETHOD( QueryServerInfo )( 
        IN OUT PROPSERVERINFO *pInfo) ;
    
    STDMETHOD( AcquireAdvanced )( 
        IN const VARIANT *pvarSrc,
        OUT LPARAM *plParamSrc) ;
    
    STDMETHOD( EnumFolderItems )( 
        IN LPARAM lParamSrc,
        OUT IEnumPROPFOLDERITEM **ppEnum) ;
    
    STDMETHOD( EnumPropertyItems )( 
        IN const PROPFOLDERITEM *pFolderItem,
        OUT IEnumPROPERTYITEM **ppEnum) ;
    
    STDMETHOD( EnumValidValues )( 
        IN const PROPERTYITEM *pItem,
        OUT IEnumPROPVARIANT_DISPLAY **ppEnum) ;
    
    STDMETHOD( PersistAdvanced )( 
        IN OUT PROPERTYITEM *pItem) ;
    
    STDMETHOD( ReleaseAdvanced )( 
        IN LPARAM lParamSrc,
        IN ULONG dwFlags) ;

};

//-------------------------------------------------------------------------//
//  Data object base class for all supported audio/video file types
class CAVPropSrc  
//-------------------------------------------------------------------------//
{
public:    
    static STDMETHODIMP  CreateInstance( const VARIANT* pvarSrc, OUT CAVPropSrc** ppSrc ) ;
    static STDMETHODIMP  CreateInstance( LPCTSTR pszExt, OUT CAVPropSrc** ppSrc ) ;
    AV_MEDIA_TYPE        MediaType() const   { return _iMediaType ; }
    void                 Attach( LPCTSTR pszFile ) ;
    LPCTSTR              Path() const { return _szFile ; }
    virtual STDMETHODIMP Acquire() = 0 ;



    virtual STDMETHODIMP_(BOOL) IsSupportedFolderID( REFPFID pfid ) const = 0 ;
    virtual STDMETHODIMP GetPropertyItem( IN REFPFID pfid, IN LONG iProperty, OUT PPROPERTYITEM pItem ) ;
    virtual STDMETHODIMP GetPropertyValue( LPCSHCOLUMNID pscid, OUT VARIANT* pVal ) ;
    virtual STDMETHODIMP GetPropertyCount() const   { return _cPropItems ; }

    virtual ~CAVPropSrc() { FreeItems() ; }

protected:
    CAVPropSrc( AV_MEDIA_TYPE iMediaType ) 
        :   _iMediaType(iMediaType),
            _rgPropItems(NULL), 
            _cPropItems(0)
    {
        *_szFile = 0 ;
    }

    PPROPERTYITEM   AllocItems( int nCount ) ;
    void              FreeItems() ;

    AV_MEDIA_TYPE        _iMediaType ;
    TCHAR                _szFile[MAX_PATH] ;
    PPROPERTYITEM      _rgPropItems ;  // array of PROPERTYITEMs
    LONG                 _cPropItems ;
} ;

//-------------------------------------------------------------------------//
//  Data object class for wave files
class CWaveSrc : public CAVPropSrc
//-------------------------------------------------------------------------//
{
protected:
    CWaveSrc() : CAVPropSrc( MT_WAVE )  {}
    ~CWaveSrc() {}

//  Overrides of CAVPropSrc:    
    STDMETHOD   (Acquire)() ;
    STDMETHODIMP_(BOOL) IsSupportedFolderID( REFPFID pfid ) const { 
        return IsEqualPFID( PFID_AudioProperties, pfid ) ;
    }

    friend HRESULT CAVPropSrc::CreateInstance( LPCTSTR, OUT CAVPropSrc** ) ;
} ;

//-------------------------------------------------------------------------//
//  Data object class for AVI files
class CAviSrc : public CAVPropSrc
//-------------------------------------------------------------------------//
{
protected:
    CAviSrc() :   CAVPropSrc( MT_AVI ), m_bVideoStream(FALSE), m_bAudioStream(FALSE) {}
    ~CAviSrc() { }

//  Overrides of CAVPropSrc:    
    STDMETHOD   (Acquire)() ;
    STDMETHODIMP_(BOOL) IsSupportedFolderID( REFPFID pfid ) const 
    { 
        if( IsEqualPFID( PFID_VideoProperties, pfid ) )
            return m_bVideoStream ;
        if( IsEqualPFID( PFID_AudioProperties, pfid ) )
            return m_bAudioStream ;
        return FALSE ;
    }

    BOOL    m_bVideoStream,
            m_bAudioStream ;

    friend HRESULT CAVPropSrc::CreateInstance( LPCTSTR, OUT CAVPropSrc** ) ;
} ;

//-------------------------------------------------------------------------//
//  Data object class for MIDI files
class CMidiSrc : public CAVPropSrc
//-------------------------------------------------------------------------//
{
protected:
    CMidiSrc()  : CAVPropSrc( MT_MIDI )
    {
        ZeroMemory( &_midi, sizeof(_midi) ) ;
    }

    ~CMidiSrc ()   {}

//  Overrides of CAVPropSrc:    
    STDMETHOD  (Acquire)() ;
    STDMETHODIMP_(BOOL) IsSupportedFolderID( REFPFID pfid ) const { 
        return IsEqualPFID( PFID_MidiProperties, pfid ) ; 
    }

    MIDIDESC        _midi ;
    friend HRESULT  CAVPropSrc::CreateInstance( LPCTSTR, OUT CAVPropSrc** ) ;
} ;


//-------------------------------------------------------------------------//
//  Common enumerator IUnknown decls
#define DECLARE_ENUM_UNKNOWN() \
    public: \
    STDMETHOD( QueryInterface )( REFIID, void** ) ; \
    STDMETHOD_( ULONG, AddRef )() ; \
    STDMETHOD_( ULONG, Release )() ; \
    private: ULONG   m_cRef ; static ULONG    m_cObj ;
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
// CEnumAVFolderItem - Folder enumerator class
class CEnumAVFolderItem : public IEnumPROPFOLDERITEM
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
    CEnumAVFolderItem( CAVPropSrc* pSrc )
        :   m_pSrc(pSrc),
            m_iPos(0),
            m_cMax(1 /*inserting one folder*/),
            m_cRef(0)
    {
        ASSERT( pSrc ) ;
    }

    virtual ~CEnumAVFolderItem() {}

    HRESULT Create( ULONG dwSrcDescr, const VARIANT& varSrc ) ;


    // IEnumPROPFOLDERITEM methods:
public:
	STDMETHOD( Next ) (ULONG cItems, PROPFOLDERITEM * rgFolderItem, ULONG* pcItemsFetched ) ;
	
    STDMETHOD( Skip ) (/*[in]*/ ULONG cItems )    
    {
        InterlockedExchangeAdd( &m_iPos, cItems ) ;
	    return S_OK;
    }
	
    STDMETHOD( Reset )( )
    {
	    InterlockedExchange( &m_iPos, 0 ) ;
        return S_OK;
    }
	
    STDMETHOD( Clone )(/*[out]*/ IEnumPROPFOLDERITEM ** ppEnum )
    {
        return E_NOTIMPL ;
    }

protected:
    CAVPropSrc*          m_pSrc ;
    LONG                 m_iPos,
                         m_cMax ;
};

//-------------------------------------------------------------------------//
// CEnumAVPropertyItem - Per-folder property enumerator class
class CEnumAVPropertyItem : public IEnumPROPERTYITEM
//-------------------------------------------------------------------------//
{
    DECLARE_ENUM_UNKNOWN() ;

public:
    CEnumAVPropertyItem( const PROPFOLDERITEM& parent )
        :   m_pSrc( (CAVPropSrc*)parent.lParam ),
            m_iPos(0),
            m_pfid(parent.pfid),
            m_cRef(0)
    {
        ASSERT( m_pSrc ) ;
    }

    virtual ~CEnumAVPropertyItem() {}

    // IEnumPROPERTYITEM methods.
public:
	STDMETHOD( Next ) (/*[in]*/ ULONG cItems, /*[out]*/ PROPERTYITEM * rgPropertyItem, /*[out]*/ ULONG* pcItemsFetched ) ;

    STDMETHOD( Skip ) (/*[in]*/ ULONG cItems )    
    {
        InterlockedExchangeAdd( &m_iPos, cItems ) ;
	    return S_OK;
    }
	
    STDMETHOD( Reset )( )
    {
	    InterlockedExchange( &m_iPos, 0 ) ;
        return S_OK;
    }
	
    STDMETHOD( Clone )(/*[out]*/ IEnumPROPERTYITEM ** ppEnum )
    {
        return E_NOTIMPL ;
    }

protected:
    LONG        m_iPos ;
    PFID        m_pfid ;
    CAVPropSrc* m_pSrc ;
};

//-------------------------------------------------------------------------//
//  Docprop column provider - // {884EA37B-37C0-11d2-BE3F-00A0C9A83DA1}
//  
extern const GUID CLSID_AVColumnProvider ;

//-------------------------------------------------------------------------//
class ATL_NO_VTABLE CAVColumnProvider : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAVColumnProvider, &CLSID_AVColumnProvider>,
    public IColumnProvider,
    public IPersist
//-------------------------------------------------------------------------//
{
public:
	CAVColumnProvider()
        :   m_pSrc(NULL), m_hMutex(NULL)
    {
        m_hMutex = CreateMutex( NULL, FALSE, NULL ) ;
    }
    
    virtual ~CAVColumnProvider()
    {
        Flush() ;
        if( m_hMutex )
            CloseHandle( m_hMutex ) ;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_AVCOLUMNPROVIDER)
    DECLARE_NOT_AGGREGATABLE(CAVColumnProvider)

    BEGIN_COM_MAP(CAVColumnProvider)
	    COM_INTERFACE_ENTRY(IColumnProvider)
        COM_INTERFACE_ENTRY(IPersist)
    END_COM_MAP()

    //  IPersist
    STDMETHOD ( GetClassID )( CLSID *pClassID ) {
        *pClassID = CLSID_AVColumnProvider ;
        return S_OK ;
    }
    //  IColumnProvider
    STDMETHOD ( Initialize )( LPCSHCOLUMNINIT psci ) ;
    STDMETHOD ( GetColumnInfo )( DWORD dwIndex, LPSHCOLUMNINFO psci ) ;
    STDMETHOD ( GetItemData )( LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, LPVARIANT pvarData ) ;

protected:
    
    void    Flush()    {
        if( m_pSrc )        {
            delete m_pSrc ; 
            m_pSrc = NULL ;
        }
    }

    ULONG   AcquireMutex( ULONG dwTimeout = INFINITE ) ;
    BOOL    ReleaseMutex() ;

private:
    CAVPropSrc* m_pSrc ;
    HANDLE      m_hMutex ;
} ;

//-------------------------------------------------------------------------//
inline ULONG CAVColumnProvider::AcquireMutex( ULONG dwTimeout )    {
    return WaitForSingleObject( m_hMutex, dwTimeout ) ;
}
inline BOOL CAVColumnProvider::ReleaseMutex()  {
    return ::ReleaseMutex( m_hMutex ) ;
}



#endif //__AVFILEPROPSERVER_H_
