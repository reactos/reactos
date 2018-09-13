//-------------------------------------------------------------------------//
// DefSrv32.h : Declaration of the CPTDefSrv32
//-------------------------------------------------------------------------//

#ifndef __PTDEFAULTSERVER32_H_
#define __PTDEFAULTSERVER32_H_

#include "resource.h"       // resource symbols
#include "propvar.h"
#include "dictbase.h"
#include "defprop.h"
#include "imageprop.h"

#define            PTREGROOTKEY   HKEY_CURRENT_USER
extern const TCHAR PTREGSUBKEY[] ;

//-------------------------------------------------------------------------//
//  Hash matrix collection to house our properties, while being able
//  to dig up elements rippin quick.
class CPropertyMap : public TMultiKeyDictionaryBase< PROPERTYITEM, 3 >
//-------------------------------------------------------------------------//
{
public:
    CPropertyMap() {}
    virtual ~CPropertyMap() {}

    enum KEYS   {
        iPropID,
        iFolderID,
        iShColID,                    
    } ;

//  overrides of TMultiKeyDictionaryBase
protected:
    virtual ULONG   HashValue( UCHAR iKey, const PROPERTYITEM& val ) const ;
    virtual ULONG   HashKey( UCHAR iKey, const void* pvKey ) const ;
    virtual int     Compare( UCHAR iKey, const void* pvKey, const PROPERTYITEM& val ) const ;
    virtual PVOID   GetKey( UCHAR iKey, const PROPERTYITEM& val ) const ;
    virtual BOOLEAN AllowDuplicates( UCHAR iKey ) const ;
    virtual void    OnDelete( PROPERTYITEM& val ) ;
} ;

//-------------------------------------------------------------------------//
class CPropertySource
//-------------------------------------------------------------------------//
{
public:
    CPropertySource() ;
    virtual ~CPropertySource() ;

    //  Retrieves properties from the source.
    HRESULT       Acquire( const VARIANT* pvarSrc ) ;
    HRESULT       Acquire( const WCHAR* pwszSrc ) ;
    HRESULT       Persist( const PROPERTYITEM& item ) ;

    //  Reports the number of properties acquired from the source.
    int           PropertyCount() const ;
    CPropertyMap& PropertyMap() ;

    //  Retrieves the storage name (if root stg, the file path).
    LPCWSTR       StgName() const   { return m_szStgName ; }

    //  Reports whether the source uses the indicated property folder.
    BOOL          UsesFolder( IN LONG dwFolderID ) ;

    //  Retrieves the address of the specialized server for the source.
    IAdvancedPropertyServer* GetSpecializedServer( LPARAM* plParamSpecialized )  
    {
        if( plParamSpecialized ) 
            *plParamSpecialized = m_lParamSpecialized ;
        return m_pSpecialized ;
    }

    //  Closes the source.
    VOID          Close( BOOL bPermanent ) ;

protected:
    virtual void   PreMapPropItem( PROPERTYITEM* pItem ) ;
    virtual BOOL   MapPropertyItem( PROPERTYITEM* pItem, int* pCount = NULL ) ;
    static HRESULT FindServerForSource( LPCWSTR pwszSrc, IAdvancedPropertyServer** pServer ) ;


public:
    ULONG                   m_dwSize ;          // for rude type/sanity check (leave as first data member!)
private:
    ULONG                   m_dwFileAttr,       // WIN32 file attributes.
                            m_dwSrcType,        // source classification (PST_ flags)
                            m_dwStgWriteMode,   // storage read/write mode (STGM_READ, STGM_WRITE, SRGM_READWRITE)
                            m_dwStgShareMode ;  // storage share mode (STGM_SHARE_ flags)
    IAdvancedPropertyServer*    m_pSpecialized ;    // Address of file type-specific server.
    LPARAM                  m_lParamSpecialized ;// LPARAM cookie owned by file type-specific server

    IPropertySetStorage     *m_pPSS ;           // address of crucial interface
    CPropertyMap            m_map ;             // collection of properties
    WCHAR                   m_szStgName[MAX_PATH] ; // storage name, if applicable.

#ifdef _X86_
    IFLPROPERTIES           m_IflProps ;
    ULONG                   m_cIflProps ;
#endif

private:
    int     GatherProperties() ;
    int     GatherImageProperties() ;

    //  Private utility methods:
    static HRESULT OpenPropertySetStorage( LPCWSTR, IPropertySetStorage**, ULONG, ULONG, ULONG* ) ;
    static HRESULT OpenPropertyStorage( IPropertySetStorage*, REFFMTID, ULONG, ULONG, IPropertyStorage**, UINT* ) ;
} ;

//-------------------------------------------------------------------------//
inline int CPropertySource::PropertyCount() const       { 
    return m_map.Count() ;
}
inline CPropertyMap&  CPropertySource::PropertyMap()    { 
    return m_map ;
}

//-------------------------------------------------------------------------//
//  Global OLESS/NSS property helper functions (see impl for descr).
//-------------------------------------------------------------------------//
STDMETHODIMP SSAcquireMultiple ( LONG, VARIANT*, ULONG*, HRESULT*, REFFMTID, LONG, PROPSPEC*, PROPVARIANT*, ULONG* ) ;
STDMETHODIMP SSPersistMultiple ( LONG, VARIANT*, HRESULT *, REFFMTID, LONG, PROPSPEC*, PROPVARIANT*, PROPID) ;


//-------------------------------------------------------------------------//
// CPTDefSrv32
class ATL_NO_VTABLE CPTDefSrv32 : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CPTDefSrv32, &CLSID_PTDefaultServer32>,
    public ISupportErrorInfo,
    public IAdvancedPropertyServer,
    public IBasicPropertyServer,
    public IPropertyServer
//-------------------------------------------------------------------------//
{
public:
    CPTDefSrv32() ;
    virtual ~CPTDefSrv32() {}
    HRESULT FinalConstruct() ;
    void FinalRelease() ;
    
public:
    //  IBasicPropertyServer methods
    STDMETHOD( AcquireBasic )( IN const VARIANT* pvarSrc, IN OUT BASICPROPITEM rgItems[], IN LONG cItems ) ;
    STDMETHOD( PersistBasic )( IN const VARIANT* pvarSrc, IN OUT BASICPROPITEM rgItems[], IN LONG cItems ) ;
    
    // IAdvancedPropertyServer methods
    STDMETHOD( QueryServerInfo   )( /*[in, out]*/ PROPSERVERINFO* pInfo ) ;
    STDMETHOD( AcquireAdvanced   )( /*[in]*/ const VARIANT* pvarSrc, /*[out]*/ LPARAM* plParamSrc ) ;
    STDMETHOD( EnumFolderItems   )( /*[in*/ LPARAM lParamSrc, /*[out]*/ IEnumPROPFOLDERITEM** ppEnum ) ;
    STDMETHOD( EnumPropertyItems )( /*[in]*/ const PROPFOLDERITEM* pFolderItem, /*[out]*/ IEnumPROPERTYITEM ** ppEnum ) ;
    STDMETHOD( EnumValidValues   )( /*[in]*/ const PROPERTYITEM* pItem, /*[out]*/ IEnumPROPVARIANT_DISPLAY ** ppEnum ) ;
    STDMETHOD( PersistAdvanced   )( /*[in, out]*/ PROPERTYITEM* pItem ) ;
    STDMETHOD( ReleaseAdvanced   )( /*[in]*/ LPARAM lParamSrc, /*[in]*/ ULONG dwFlags ) ;
    
    //  IPropertyServer methods
    STDMETHOD( AcquireMultiple  )( LONG, VARIANT*, ULONG*, HRESULT*, REFFMTID, LONG, PROPSPEC*, PROPVARIANT*, ULONG* ) ;
    STDMETHOD( PersistMultiple  )( LONG, VARIANT*, HRESULT *, REFFMTID, LONG, PROPSPEC*, PROPVARIANT*, PROPID) ;
    STDMETHOD( GetDisplayText   )( REFFMTID, LONG, PROPSPEC*, PROPVARIANT*, BSTR*, ULONG*, BSTR*) ;
    STDMETHOD( AssignFromDisplayText )( REFFMTID, LONG, PROPSPEC*, BSTR*, BSTR*, ULONG*, PROPVARIANT* ) ;
    
    // ISupportsErrorInfo methods
    STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid ) ;
    
    
    DECLARE_REGISTRY_RESOURCEID(IDR_PTDEFAULTSERVER32)
    DECLARE_GET_CONTROLLING_UNKNOWN()
        
    BEGIN_COM_MAP(CPTDefSrv32)
        COM_INTERFACE_ENTRY(IAdvancedPropertyServer)
        COM_INTERFACE_ENTRY(IBasicPropertyServer)
        COM_INTERFACE_ENTRY(IPropertyServer)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()
        
public:
    //  Implementation helpers
    HRESULT MakePropertyTreeSource( IN LPARAM, OUT CPropertySource** ) ;
    int AcquireBasicImageProperties( IN LPCWSTR pwszSrc, IN OUT BASICPROPITEM rgItems[], IN LONG cItems ) ; 
};

//-------------------------------------------------------------------------//
inline CPTDefSrv32::CPTDefSrv32()   {
}
inline HRESULT CPTDefSrv32::FinalConstruct()    {
        return S_OK ;
}
inline void CPTDefSrv32::FinalRelease() {
}

//-------------------------------------------------------------------------//
class ATL_NO_VTABLE CExeVerColumnProvider : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CExeVerColumnProvider, &CLSID_ExeVerColumnProvider>,
    public IColumnProviderX,  /* dummy interface to keep ATL happy */
    public IColumnProvider,
    public IPersist
//-------------------------------------------------------------------------//
{
public:
        CExeVerColumnProvider() : 
        m_pvAllTheInfo(NULL)
    {
        m_szFileCache[0] = 0;
    }

    virtual ~CExeVerColumnProvider() 
    {
        _ClearCache();
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_COLUMNPROVIDERS)
    DECLARE_NOT_AGGREGATABLE(CExeVerColumnProvider)

    BEGIN_COM_MAP(CExeVerColumnProvider)
        COM_INTERFACE_ENTRY(IColumnProviderX)
        COM_INTERFACE_ENTRY(IColumnProvider)
        COM_INTERFACE_ENTRY(IPersist)
    END_COM_MAP()

    //  IPersist
    STDMETHOD ( GetClassID )( CLSID *pClassID ) 
    {
        *pClassID = CLSID_ExeVerColumnProvider ;
        return S_OK ;
    }

    //  IColumnProvider
    STDMETHOD ( Initialize )( LPCSHCOLUMNINIT psci ) { return S_OK ; }
    STDMETHOD ( GetColumnInfo )( DWORD dwIndex, LPSHCOLUMNINFO psci ) ;
    STDMETHOD ( GetItemData )( LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, LPVARIANT pvarData ) ;

private:
    FARPROC _GetVerProc(LPCSTR pszName);
    HRESULT _CacheFileVerInfo(LPCWSTR pszFile);
    void _ClearCache();

    //  Import wraps
    BOOL VerQueryValue( const void *,  LPTSTR, void * *, PUINT ) ;
    BOOL GetFileVersionInfoSize( LPTSTR, LPDWORD ) ;
    BOOL GetFileVersionInfo( LPTSTR, DWORD, DWORD, void * ) ;

    WCHAR m_szFileCache[MAX_PATH];
    void  *m_pvAllTheInfo;
    HRESULT m_hrCache;
} ;

#endif //__PTDEFAULTSERVER32_H_
