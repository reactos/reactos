//-------------------------------------------------------------------------//
// ext.h : CShellExt decl
//-------------------------------------------------------------------------//

#ifndef __SHELLEXT_H_
#define __SHELLEXT_H_

#include "resource.h"       // resource symbols
#include "prioritylst.h"
#include "shdocext.h"      // CLSID_ShDocPropExt
#include "ptserver.h"      // PROPERTYITEM, etc.

class CPage0;
interface IPropertyServer;
extern const TCHAR szEXTENSION_SETTINGS_REGKEY[];
extern const TCHAR szUIMODE_REGVAL[];

//-------------------------------------------------------------------------//
//  Error context identifiers
enum ERROR_CONTEXT
{
    ERRCTX_ALL,
    ERRCTX_ACQUIRE,
    ERRCTX_PERSIST_APPLY,
    ERRCTX_PERSIST_OK,
};

typedef struct _TARGET
{
    VARIANT  varFile;
    FILETIME ftAccess;
} TARGET;

//  Helper to restore access time to a specified target.
HRESULT _RestoreAccessTime( const TARGET& t ) ;

//-------------------------------------------------------------------------//
//  class CFileList - collection of property source specifiers
typedef TPriorityList<TARGET> CFileListBase;
class CFileList : public CFileListBase
//-------------------------------------------------------------------------//
{
public:
    CFileList() : CFileListBase( 4 /*block size*/ )  {}
    virtual void OnFreeElement( TARGET& t ) {
        VariantClear( &t.varFile );
    }
};

//-------------------------------------------------------------------------//
//  Property attribute array indices; must be zero-based and consecutive,
//  grouped by FMTIDs.
enum BASICPROPERTY
{
    //  FMTID_SummaryInformation
    BASICPROP_TITLE = 0,
    BASICPROP_SUBJECT,
    BASICPROP_AUTHOR,
    BASICPROP_KEYWORDS,
    BASICPROP_COMMENTS,

    //  FMTID_DocSummaryInformation
    BASICPROP_CATEGORY,

    BASICPROP_COUNT,  // placeholder
};

#define BASICPROP_SUMMARYINFO_FIRST       BASICPROP_TITLE
#define BASICPROP_SUMMARYINFO_LAST        BASICPROP_COMMENTS
#define BASICPROP_DOCSUMMARYINFO_FIRST    BASICPROP_CATEGORY
#define BASICPROP_DOCSUMMARYINFO_LAST     BASICPROP_CATEGORY

//-------------------------------------------------------------------------//
class CBasicPropertySource  // basic property block.
//-------------------------------------------------------------------------//
{
public:
    CBasicPropertySource();
    ~CBasicPropertySource() { Free(); }

    HRESULT Acquire( const VARIANT* pvarSource );
    HRESULT Persist();
    int     Compare( BASICPROPERTY nProp, CBasicPropertySource* pOther );
    BSTR    MakeDisplayString( BASICPROPERTY nProp );
    void    Free();
    HRESULT SetPropertyValue( BASICPROPERTY nProp, IN BSTR bstrValue );
    void    SetDirty( BASICPROPERTY nProp, BOOL bDirty );

protected:
    VARIANT               _varSource;
    IBasicPropertyServer* _pbps;
    BASICPROPITEM*        _rgItems;
};

//-------------------------------------------------------------------------//
//  BUGBUG: until DocSummaryInformation propids are defined in a public header...
#define PIDDSI_CATEGORY 0x00000002


//-------------------------------------------------------------------------//
//  class CShellExt - shell extension object, CLSID_
class ATL_NO_VTABLE CShellExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CShellExt, &CLSID_ShDocPropExt>,
    public IShellExtInit,
	public IShellPropSheetExt
//-------------------------------------------------------------------------//
{
public:
//  IShellExtInit methods
    HRESULT STDMETHODCALLTYPE Initialize( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID );	

//  IShellPropSheetExt methods
    HRESULT STDMETHODCALLTYPE AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam	);	
    HRESULT STDMETHODCALLTYPE ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplacePage, LPARAM lParam );	

//  Implementation:
public:
	CShellExt();
    virtual ~CShellExt();

    BOOL        HasAdvancedProperties() { return m_fAdvancedProperties; }
    BOOL        HasBasicProperties() { return m_fBasicProperties; }
    CFileList&  FileList()   { return m_filelist; }
    BOOL        GetBasicPropFromIDC( UINT nIDC, BASICPROPERTY* pnProp ) const;
    BOOL        GetBasicPropInfo( BASICPROPERTY nProp, OUT const FMTID*& pFmtid, OUT PROPID& nPropID, OUT VARTYPE& vt );
    static BOOL GetBasicPropInfo( REFFMTID fmtid, OUT int&, OUT int&, OUT int& );
    UINT        ReadOnlyCount() const;
    BOOL        IsCompositeMismatch( BASICPROPERTY nProp ) const;
    BOOL        IsDirty( BASICPROPERTY nProp ) const;
    BOOL        IsDirty() const;
    void        SetDirty( BASICPROPERTY nProp, BOOL bDirty );
    void        SetDirty( BOOL bDirty );
    void        ChangeNotify( LONG wEventID );
    void        RestoreAccessTimes();

    int         DisplayError( HWND hwndOwner, UINT nIDCaption, ERROR_CONTEXT errctx, HRESULT hr );

    void        CacheUIValue( HWND hwndPage, BASICPROPERTY nProp );
    void        CacheUIValues( HWND hwndPage );
    void        UncacheUIValue( HWND hwndPage, BASICPROPERTY nProp );
    void        UncacheUIValues( HWND hwndPage );

    int         GetMaxPropertyTextLength( HWND hwndPage );
    BOOL        GetPropertyText( HWND hwndPage, BASICPROPERTY nProp, LPTSTR pszBuf, int cchBuf );
    void        SetPropertyText( HWND hwndDlg, BASICPROPERTY nProp, LPCTSTR pszText );

    STDMETHOD( Persist )();
    void        FinalRelease();

protected:
    static HRESULT 
                FilterFileFindData(LPCTSTR, WIN32_FIND_DATA*);
    BOOL        IsSourceSupported( LPTSTR pszPath, BOOL bAdvanced, OUT OPTIONAL LPCLSID pclsid = NULL );
    void        CacheDisplayText( BASICPROPERTY nProp, LPCTSTR pszText );
    STDMETHOD( Acquire )();


public:
    DECLARE_REGISTRY_RESOURCEID( IDR_SHELLEXT )
    BEGIN_COM_MAP(CShellExt)
	    COM_INTERFACE_ENTRY(IShellExtInit)
	    COM_INTERFACE_ENTRY(IShellPropSheetExt)
    END_COM_MAP()

//  Data
protected:
    CFileList             m_filelist;
    CBasicPropertySource* m_rgBasicSrc;        // array of basic property sources
    CPage0*               m_pPage0;

    BSTR             m_rgbstrDisplay[BASICPROP_COUNT]; // cached display text
    ULONG            m_rgvarFlags[BASICPROP_COUNT]; // attributes of cached values
    UINT             m_cReadOnly;              // count of read-only property sources
    BOOL             m_rgbDirty[BASICPROP_COUNT];  // dirty flags
    BOOL             m_fAdvancedProperties;               // 'Advanced' button enabled
    BOOL             m_fBasicProperties;                  // 'Basic' button enabled
};

//-------------------------------------------------------------------------//
inline BOOL CShellExt::IsDirty( BASICPROPERTY nProp ) const   {
    return m_rgbDirty[nProp];
}
//-------------------------------------------------------------------------//
inline BOOL CShellExt::IsDirty() const  {
    for( int i=0; i<BASICPROP_COUNT; i++ )
        if( IsDirty( (BASICPROPERTY)i ) )
            return TRUE;
    return FALSE;
}
//-------------------------------------------------------------------------//
//  Sets or clears dirty flags
inline void CShellExt::SetDirty( BOOL bDirty )  {
    for( int i=0; i<BASICPROP_COUNT; i++ )
        SetDirty( (BASICPROPERTY)i, bDirty );
}
inline UINT CShellExt::ReadOnlyCount() const {
    return m_cReadOnly;
}




#endif //__SHELLEXT_H_
