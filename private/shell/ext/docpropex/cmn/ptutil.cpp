//-------------------------------------------------------------------------//
//
//	PTutil.cpp
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "PTutil.h"
#include "PTserver.h"

#define SAFE_SYSFREESTRING( bstr )  if((bstr)){ SysFreeString((bstr)); (bstr)=NULL; }
#define SAFE_COPYSYSSTRING( dest, src )  if((src)){ (dest)=SysAllocString((src)); }

//-------------------------------------------------------------------------//
//  Property Folder ID definitions
//  HACKHACK: These should go in shlguid

// {19469210-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_Description =
    { 0x19469210, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469211-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_Origin =
{ 0x19469211, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469212-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_ImageProperties =
{ 0x19469212, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469213-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_AudioProperties =
{ 0x19469213, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469214-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_VideoProperties =
{ 0x19469214, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469214-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_MidiProperties =
{ 0x19469215, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {4C927CBB-7994-11d2-BE78-00A0C9A83DA1}
static const PFID PFID_FaxProperties = 
{ 0x4c927cbb, 0x7994, 0x11d2, { 0xbe, 0x78, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

//-------------------------------------------------------------------------//
LPWSTR WINAPI StrA2W( UINT nCodePage, LPWSTR pwsz, LPCSTR pasz, int cch )
{
    ASSERT(pwsz != NULL);
    ASSERT(pasz != NULL);

    pwsz[0] = '\0';
    MultiByteToWideChar( nCodePage, 0, pasz, -1, pwsz, cch );
    return pwsz;
}

//-------------------------------------------------------------------------//
LPSTR WINAPI StrW2A( UINT nCodePage, LPSTR pasz, LPCWSTR pwsz, int cch )
{
    ASSERT(pasz != NULL);
    ASSERT(pwsz != NULL);

    pasz[0] = '\0';
    WideCharToMultiByte( nCodePage, 0, pwsz, -1, pasz, cch, NULL, NULL ) ;
    return pasz ;
}

//-------------------------------------------------------------------------//
BOOL WINAPI StrRoundTripsW( UINT nCodePage, LPCWSTR pwsz )
{
    ASSERT( pwsz );
    
    int    cch = lstrlenW( pwsz ) ;
    CHAR*  paszTest = (CHAR*) alloca( sizeof(CHAR)  * (cch+1) ) ;
    WCHAR* pwszTest = (WCHAR*)alloca( sizeof(WCHAR) * (cch+1) ) ;

    StrA2W( nCodePage, 
            pwszTest,
            StrW2A( nCodePage, paszTest, pwsz, cch ),
            cch ) ;

    return 0 == memcmp( pwsz, pwszTest, cch * sizeof(WCHAR) ) ;
}

//-------------------------------------------------------------------------//
BOOL WINAPI StrRoundTripsA( UINT nCodePage, LPCSTR pasz )
{
    ASSERT( pwsz );
    
    int    cch = lstrlenA( pasz ) ;
    CHAR*  paszTest = (CHAR*)alloca( sizeof(CHAR)  * (cch+1) ) ;
    WCHAR* pwszTest = (WCHAR*)alloca( sizeof(WCHAR) * (cch+1) ) ;

    StrW2A( nCodePage,
            paszTest,
            StrA2W( nCodePage, pwszTest, pasz, cch ),
            cch ) ;

    return 0 == memcmp( pasz, paszTest, cch * sizeof(CHAR) ) ;
}

//-------------------------------------------------------------------------//
void InitPropFolderItem( struct tagPROPFOLDERITEM* pItem )
{
    if( pItem )
    {
        memset( pItem, 0, sizeof(*pItem) ) ;
        pItem->cbStruct = sizeof(*pItem) ;
        pItem->pfid     = PFID_NULL ;
        pItem->iOrder   = -1 ;
    }
}

//-------------------------------------------------------------------------//
void ClearPropFolderItem( struct tagPROPFOLDERITEM* pItem )
{
    if( IsValidPropFolderItem( pItem ) )
    {
        SAFE_SYSFREESTRING( pItem->bstrName ) ;
        SAFE_SYSFREESTRING( pItem->bstrDisplayName ) ;
        SAFE_SYSFREESTRING( pItem->bstrQtip ) ;
        InitPropFolderItem( pItem ) ;
    }
}

//-------------------------------------------------------------------------//
BOOL CopyPropFolderItem( struct tagPROPFOLDERITEM* pDest, const struct tagPROPFOLDERITEM* pSrc )
{
    if( !( pDest && IsValidPropFolderItem( pDest ) && IsValidPropFolderItem( pSrc ) ) )
    {
        ASSERT( FALSE ) ; // invalid arg (pDest) or unitialized dest or src.
        return FALSE ;
    }
    ClearPropFolderItem( pDest ) ;
    
    *pDest = *pSrc ;
    SAFE_COPYSYSSTRING( pDest->bstrName, pSrc->bstrName ) ;
    SAFE_COPYSYSSTRING( pDest->bstrDisplayName, pSrc->bstrDisplayName ) ;
    SAFE_COPYSYSSTRING( pDest->bstrQtip, pSrc->bstrQtip ) ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
BOOL IsValidPropFolderItem( const struct tagPROPFOLDERITEM* pItem )
{
    return pItem!=NULL && pItem->cbStruct==sizeof(*pItem) ;
}

//-------------------------------------------------------------------------//
void InitPropertyItem( struct tagPROPERTYITEM* pItem )
{
    if( pItem )
    {
        memset( pItem, 0, sizeof(*pItem) ) ;
        pItem->cbStruct = sizeof(*pItem) ;
        pItem->pfid     = PFID_NULL ;
        pItem->iOrder   = -1 ;
    }
}

//-------------------------------------------------------------------------//
void ClearPropertyItem( struct tagPROPERTYITEM* pItem )
{
    if( IsValidPropertyItem( pItem ) )
    {
        SAFE_SYSFREESTRING( pItem->bstrName ) ;
        SAFE_SYSFREESTRING( pItem->bstrDisplayName ) ;
        SAFE_SYSFREESTRING( pItem->bstrQtip ) ;
        PropVariantClear( &pItem->val ) ;
    }
    InitPropertyItem( pItem ) ;
}

//-------------------------------------------------------------------------//
BOOL CopyPropertyItem( struct tagPROPERTYITEM* pDest, const struct tagPROPERTYITEM* pSrc )
{
    if( !( pDest && IsValidPropertyItem( pSrc ) && IsValidPropertyItem( pDest ) ) )
    {
        ASSERT( FALSE ) ;// invalid arg (pDest) or unitialized source, dest.
        return FALSE ;
    }
    ClearPropertyItem( pDest ) ;
        
    *pDest = *pSrc ;
    SAFE_COPYSYSSTRING( pDest->bstrName, pSrc->bstrName ) ;
    SAFE_COPYSYSSTRING( pDest->bstrDisplayName, pSrc->bstrDisplayName ) ;
    SAFE_COPYSYSSTRING( pDest->bstrQtip, pSrc->bstrQtip ) ;
    PropVariantInit( &pDest->val ) ;
    PropVariantCopy( &pDest->val, &pSrc->val ) ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
BOOL IsValidPropertyItem( const struct tagPROPERTYITEM* pItem )
{
    return pItem!=NULL && pItem->cbStruct==sizeof(*pItem) ;
}


//-------------------------------------------------------------------------//
void InitBasicPropertyItem( struct tagBASICPROPITEM* pItem )
{
    if( pItem )
    {
        memset( pItem, 0, sizeof(*pItem) ) ;
        pItem->cbStruct = sizeof(*pItem) ;
    }
}

//-------------------------------------------------------------------------//
void ClearBasicPropertyItem( struct tagBASICPROPITEM* pItem )
{
    if( IsValidBasicPropertyItem( pItem ) )
    {
        PropVariantClear( &pItem->val ) ;
    }
    InitBasicPropertyItem( pItem ) ;
}

//-------------------------------------------------------------------------//
BOOL CopyBasicPropertyItem( struct tagBASICPROPITEM* pDest, const struct tagBASICPROPITEM* pSrc )
{
    if( !( pDest && IsValidBasicPropertyItem( pSrc ) && IsValidBasicPropertyItem( pDest ) ) )
    {
        ASSERT( FALSE ) ;// invalid arg (pDest) or unitialized source, dest.
        return FALSE ;
    }
    ClearBasicPropertyItem( pDest ) ;
        
    *pDest = *pSrc ;
    PropVariantInit( &pDest->val ) ;
    PropVariantCopy( &pDest->val, &pSrc->val ) ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
BOOL IsValidBasicPropertyItem( const struct tagBASICPROPITEM* pItem )
{
    return pItem!=NULL && pItem->cbStruct==sizeof(*pItem) ;
}

//-------------------------------------------------------------------------//
void InitPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pItem )
{
    if( pItem )
    {
        PropVariantInit( &pItem->val ) ;
        pItem->bstrDisplay = NULL ;
    }
}

//-------------------------------------------------------------------------//
BOOL CopyPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pDest, const struct tagPROPVARIANT_DISPLAY* pSrc )
{
    if( pDest && pSrc )
    {
        ClearPropVariantDisplay( pDest ) ;
        PropVariantCopy( &pDest->val, &pSrc->val ) ;

        if( pSrc->bstrDisplay )
            pDest->bstrDisplay =  SysAllocString( pSrc->bstrDisplay ) ;
        
        return TRUE ;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
void ClearPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pItem )
{
    if( pItem )
    {
        PropVariantClear( &pItem->val ) ;
        if( pItem->bstrDisplay )
            SysFreeString( pItem->bstrDisplay ) ;
        InitPropVariantDisplay( pItem ) ;
    }
}

//-------------------------------------------------------------------------//
BOOL PTCheckWriteAccessA( LPCSTR pszFile )
{
    //  Test write access.
    HANDLE hFile = CreateFileA( pszFile, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_NO_RECALL, NULL );
    if( INVALID_HANDLE_VALUE != hFile )
    {
        CloseHandle( hFile );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL PTCheckWriteAccessW( LPCWSTR pwszFile )
{
    //  Test write access.
    HANDLE hFile = CreateFileW( pwszFile, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_NO_RECALL, NULL );
    if( INVALID_HANDLE_VALUE != hFile )
    {
        CloseHandle( hFile );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
ULONG PTGetFileAttributesA( LPCSTR pszFile )
{
    ULONG dwAttr = GetFileAttributesA( pszFile );
    if( dwAttr != 0xFFFFFFFF && (dwAttr & FILE_ATTRIBUTE_READONLY) == 0 )
    {
        if( !PTCheckWriteAccessA( pszFile ) )
            dwAttr |= FILE_ATTRIBUTE_READONLY; 
    }
    return dwAttr;
}

//-------------------------------------------------------------------------//
ULONG PTGetFileAttributesW( LPCWSTR pszFile )
{
    ULONG dwAttr = GetFileAttributesW( pszFile );
    
    if( dwAttr != 0xFFFFFFFF && (dwAttr & FILE_ATTRIBUTE_READONLY) == 0 )
    {
        if( !PTCheckWriteAccessW( pszFile ) )
            dwAttr |= FILE_ATTRIBUTE_READONLY; 
    }
    return dwAttr;
}

//-------------------------------------------------------------------------//
//  Tokenizes a string into a linked list of token descriptors.  On return, 
//  all delimiters present in the original input string will be
//  replaced with NULL characters.
int GetTokens( LPTSTR pszString, LPCTSTR pszDelims, OUT PSTRTOK *ppList )
{
    PSTRTOK pNext = NULL,
            pTok  = NULL ;
    int     cRet  = 0 ;

    if( !( pszString && *pszString && pszDelims && *pszDelims && ppList) )
        return NULL ;

    *ppList = NULL ;

    LPTSTR  pszTok = pszString ;
    int     cchString = lstrlen( pszString ),
            cchDelims = lstrlen( pszDelims ) ;

    for( int i=0; i < cchString; i++ )
    {
        //  Is the character a delimiter
        if( _tcschr( pszDelims, pszTok[i] )!=NULL )
        {
            pszTok[i] = (TCHAR)0 ;  // zero it in place.
            if( i > 0 ) continue ;  // go to next character if we're into the string
        }

        //  Allocate and initialize a token descriptor
        if( (pTok = new STRTOK)!=NULL )
        {
            memset( pTok, 0, sizeof(*pTok) ) ;
            pTok->pszTok = &pszTok[i] ;
            
            //  skip to next character
            for( ; i < cchString ; i++ )
            {
                if( _tcschr( pszDelims, pszTok[i] )!=NULL )
                {
                    pszTok[i] = (TCHAR)0 ;  // Zero the delimiter in place
                    break ;                 // We've reached the end of the token;
                                            // bust out of the loop and link it to the list.
                }
                pTok->cchTok++ ;  // not a delimiter; increment char count for the token.
            }

            //  link the token to the list
            if( pNext == NULL )
                *ppList = pNext = pTok ; // no list yet; establish our head.
            else
            {
                pNext->pNext = pTok ;
                pNext = pTok ;
            }
            cRet++ ;  // increment token count
        }
    }
    return cRet ;
}

//-------------------------------------------------------------------------//
//  Frees a list of token descriptors generated by GetTokens().
int FreeTokens( PSTRTOK* ppList )
{
    int cTokens = 0 ;

    if( ppList )
    {
        while( *ppList )
        {
            PSTRTOK pNext = (*ppList)->pNext ;
            delete (*ppList) ;
            cTokens++ ;
            (*ppList) = pNext ;
        }
    }
    
    return cTokens ;
}


//-------------------------------------------------------------------------//
//  Time validation routines.   These will remain necessary until
//  SystemTimeToVariantTime() and VariantTimeToSystemTime() to date validation.
//  See bug# #329259 for more info.
//-------------------------------------------------------------------------//
#if 0
//-------------------------------------------------------------------------//
EXTERN_C BOOL IsValidSystemTime( SYSTEMTIME* pst )
{
    FILETIME ft ;
    return SystemTimeToFileTime( pst, &ft ); // validates the SYSTEMTIME arg
}

//-------------------------------------------------------------------------//
EXTERN_C BOOL IsValidFileTime( FILETIME* pft )
{
    SYSTEMTIME st;
    return FileTimeToSystemTime( pft, &st ); // validates the FILETIME arg
}

//-------------------------------------------------------------------------//
EXTERN_C BOOL IsValidVariantTime( DATE* pvt )
{
    SYSTEMTIME st;
    FILETIME   ft;
    VariantTimeToSystemTime( pvt, &st );
    return SystemTimeToFileTime( &st, &ft );// validates the SYSTEMTIME arg
}
#endif 0


//-------------------------------------------------------------------------//
//  Accessibility primitives
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
typedef HRESULT (WINAPI * PFNCREATESDTACCESSIBLEPROXY)( HWND, LPCTSTR, LONG, REFIID, void ** );
typedef LRESULT (WINAPI * PFNLRESULTFROMOBJECT)( REFIID, WPARAM, LPUNKNOWN );

#ifdef UNICODE
#define CREATESTDACCESSIBLEPROXY    "CreateStdAccessibleProxyW"
#else
#define CREATESTDACCESSIBLEPROXY    "CreateStdAccessibleProxyA"
#endif
#define LRESULTFROMOBJECT           "LresultFromObject"


//-------------------------------------------------------------------------//
class COleAcc
//-------------------------------------------------------------------------//
{
public:
    COleAcc()
        :   _hinstOleAcc(NULL),
            _pfnCreateStdAccessibleProxy(NULL),
            _pfnLresultFromObject(NULL) {}

    ~COleAcc()    {
        if( _hinstOleAcc )  {
            FreeLibrary( _hinstOleAcc );
            _hinstOleAcc = NULL;
        }
    }

    HRESULT _CreateStdAccessibleProxy( HWND, WPARAM, IAccessible** );
    LRESULT _LresultFromObject( REFIID, WPARAM, LPUNKNOWN );

private:
    BOOL Init();

    HINSTANCE                    _hinstOleAcc;
    PFNCREATESDTACCESSIBLEPROXY  _pfnCreateStdAccessibleProxy;
    PFNLRESULTFROMOBJECT         _pfnLresultFromObject;    

} OleAcc ;

//-------------------------------------------------------------------------//
BOOL COleAcc::Init()
{
    if( NULL == _hinstOleAcc )
    {
        if( NULL == (_hinstOleAcc = LoadLibrary( TEXT("oleacc.dll") )) )
            return FALSE;
    }
    
    if( NULL == _pfnCreateStdAccessibleProxy )
    {
        if( NULL == (_pfnCreateStdAccessibleProxy = (PFNCREATESDTACCESSIBLEPROXY)GetProcAddress(
                            _hinstOleAcc, CREATESTDACCESSIBLEPROXY )) )
            return FALSE;
    }

    if( NULL == _pfnLresultFromObject )
    {
        if( NULL == (_pfnLresultFromObject = (PFNLRESULTFROMOBJECT)GetProcAddress(
                            _hinstOleAcc, LRESULTFROMOBJECT )) )
            return FALSE;
    }
   
    return TRUE;
}

//-------------------------------------------------------------------------//
HRESULT COleAcc::_CreateStdAccessibleProxy( HWND hwnd, WPARAM wParam, IAccessible** ppacc )
{
#if DBG
    // Validate that we declared the function correctly
    if (_pfnCreateStdAccessibleProxy == CreateStdAccessibleProxy) { }
#endif
    if( !Init() )
        return E_UNEXPECTED;
    return _pfnCreateStdAccessibleProxy( hwnd, WC_TREEVIEW, (LONG)wParam,
                                         IID_IAccessible, (void **)ppacc );
}

//-------------------------------------------------------------------------//
HRESULT _CreateStdAccessibleProxy( HWND hwnd, WPARAM wParam, IAccessible** ppacc )
{
    return OleAcc._CreateStdAccessibleProxy( hwnd, wParam, ppacc );
}

//-------------------------------------------------------------------------//
LRESULT COleAcc::_LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN punk )
{
#if DBG
    // Validate that we declared the function correctly
    if (_pfnLresultFromObject == LresultFromObject) { }
#endif
    if( !Init() )
        return E_UNEXPECTED;
    return _pfnLresultFromObject( riid, wParam, punk );
}

//-------------------------------------------------------------------------//
LRESULT _LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN punk )
{
    return OleAcc._LresultFromObject( riid, wParam, punk );
}

//-------------------------------------------------------------------------//
//  CAccessibleBase
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  CAccessibleBase IUnknown impl
STDMETHODIMP CAccessibleBase::QueryInterface( REFIID riid, void** ppvObj )
{
    HRESULT hres;
    if( !ppvObj ) return E_POINTER;
    *ppvObj = NULL;

    if( IsEqualIID( riid, IID_IUnknown ) )
        *ppvObj = (IUnknown*)(IAccessible*)this;
    else if( IsEqualIID( riid, IID_IDispatch ) )
        *ppvObj = (IDispatch*)this;
    else if( IsEqualIID( riid, IID_IAccessible ) )
        *ppvObj = (IAccessible*)this;
    else if( IsEqualIID( riid, IID_IOleWindow ) )
        *ppvObj = (IOleWindow*)this;

    if( *ppvObj )
    {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CAccessibleBase::AddRef()
{
    return InterlockedIncrement( (LONG*)&_cRef );
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CAccessibleBase::Release()
{
    ULONG cRef = InterlockedDecrement( (LONG*)&_cRef );
    if( cRef <= 0 )
    {
        DllRelease();
        delete this;
    }
    return cRef;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IOleWindow impl
STDMETHODIMP CAccessibleBase::GetWindow( HWND* phwnd )
{
    *phwnd = _hwnd;
    return IsWindow( _hwnd ) ? S_OK : S_FALSE;
}

//-------------------------------------------------------------------------//
//  CAccessibleBase IDispatch impl
//-------------------------------------------------------------------------//

UINT WMU_GETACCSTRING()
{
    static UINT _wmu_getaccstring = 0;
    if( 0 == _wmu_getaccstring )
        _wmu_getaccstring = RegisterWindowMessage( TEXT("PropTreeGetAccString") );
    return _wmu_getaccstring;
}

inline BSTR _GetAccString( HWND hwnd, DISPID dispid, LPARAM lParam ) 
{
    if( lParam )
        return (BSTR)SendMessage( hwnd, WMU_GETACCSTRING(), (WPARAM)dispid, lParam );
    return NULL;
}

static BOOL _accLoadTypeInfo( ITypeInfo** ppti )
{
    ITypeLib* ptl;
    HRESULT hr = LoadTypeLib(L"oleacc.dll", &ptl);

    if( SUCCEEDED( hr ) )
    {
        hr = ptl->GetTypeInfoOfGuid( IID_IAccessible, ppti );
        ATOMICRELEASE( ptl );
    }
    return hr;
}

STDMETHODIMP CAccessibleBase::GetTypeInfoCount( UINT * pctinfo ) 
{ 
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo )
{ 
    HRESULT hr = E_FAIL;
    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    *pptinfo = _ptiAcc;
    (*pptinfo)->AddRef();
    return S_OK;
}

STDMETHODIMP CAccessibleBase::GetIDsOfNames( 
    REFIID riid, 
    OLECHAR** rgszNames, 
    UINT cNames,
    LCID lcid, DISPID * rgdispid )
{
    HRESULT hr = E_FAIL;

    if( IID_NULL != riid && IID_IAccessible != riid )
        return DISP_E_UNKNOWNINTERFACE;

    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    return _ptiAcc->GetIDsOfNames( rgszNames, cNames, rgdispid );
}

STDMETHODIMP CAccessibleBase::Invoke( 
    DISPID  dispidMember, 
    REFIID  riid, 
    LCID    lcid, 
    WORD    wFlags,
    DISPPARAMS * pdispparams, 
    VARIANT * pvarResult, 
    EXCEPINFO * pexcepinfo, 
    UINT * puArgErr )
{
    HRESULT hr = E_FAIL;
    if( IID_NULL != riid && IID_IAccessible != riid )
        return DISP_E_UNKNOWNINTERFACE;

    if( NULL == _ptiAcc && FAILED( (hr = _accLoadTypeInfo( &_ptiAcc )) ) )
        return hr;

    return _ptiAcc->Invoke( this, dispidMember, wFlags, pdispparams, 
                            pvarResult, pexcepinfo, puArgErr );
}

STDMETHODIMP CAccessibleBase::get_accParent( IDispatch ** ppdispParent )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accParent( ppdispParent );
}

STDMETHODIMP CAccessibleBase::get_accChildCount( long * pcChildren )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accChildCount( pcChildren );
}

STDMETHODIMP CAccessibleBase::get_accChild( VARIANT varChildIndex, IDispatch ** ppdispChild)
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accChild( varChildIndex, ppdispChild );
}

STDMETHODIMP CAccessibleBase::get_accName( VARIANT varChild, BSTR* pbstrName)
{
    ASSERT(_paccProxy);
    *pbstrName = _GetAccString( _hwnd, DISPID_ACC_NAME, varChild.lVal );
    return *pbstrName ? S_OK :S_FALSE ;
}

STDMETHODIMP CAccessibleBase::get_accValue( VARIANT varChild, BSTR* pbstrValue)
{
    ASSERT(_paccProxy);
    *pbstrValue = _GetAccString( _hwnd, DISPID_ACC_VALUE, varChild.lVal );
    return *pbstrValue ? S_OK : S_FALSE;
}

STDMETHODIMP CAccessibleBase::get_accDescription( VARIANT varChild, BSTR * pbstrDescription)
{
    ASSERT(_paccProxy);
    *pbstrDescription = _GetAccString( _hwnd, DISPID_ACC_DESCRIPTION, varChild.lVal );
    return *pbstrDescription ? S_OK :S_FALSE ;
}

STDMETHODIMP CAccessibleBase::get_accRole( VARIANT varChild, VARIANT *pvarRole )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accRole( varChild, pvarRole );
}

STDMETHODIMP CAccessibleBase::get_accState( VARIANT varChild, VARIANT *pvarState )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accState( varChild, pvarState );
}

STDMETHODIMP CAccessibleBase::get_accHelp( VARIANT varChild, BSTR* pbstrHelp )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accHelp( varChild, pbstrHelp );
}

STDMETHODIMP CAccessibleBase::get_accHelpTopic( BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accHelpTopic( pbstrHelpFile, varChild, pidTopic );
}

STDMETHODIMP CAccessibleBase::get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accKeyboardShortcut( varChild, pbstrKeyboardShortcut );
}

STDMETHODIMP CAccessibleBase::get_accFocus( VARIANT FAR * pvarFocusChild )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accFocus( pvarFocusChild );
}

STDMETHODIMP CAccessibleBase::get_accSelection( VARIANT FAR * pvarSelectedChildren )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accSelection( pvarSelectedChildren );
}

STDMETHODIMP CAccessibleBase::get_accDefaultAction( VARIANT varChild, BSTR* pbstrDefaultAction )
{
    ASSERT(_paccProxy);
    return _paccProxy->get_accDefaultAction( varChild, pbstrDefaultAction );
}

STDMETHODIMP CAccessibleBase::accSelect( long flagsSelect, VARIANT varChild )
{
    ASSERT(_paccProxy);
    return _paccProxy->accSelect( flagsSelect, varChild );
}

STDMETHODIMP CAccessibleBase::accLocation( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild )
{
    ASSERT(_paccProxy);
    return _paccProxy->accLocation( pxLeft, pyTop, pcxWidth, pcyHeight, varChild );
}

STDMETHODIMP CAccessibleBase::accNavigate( long navDir, VARIANT varStart, VARIANT * pvarEndUpAt )
{
    ASSERT(_paccProxy);
    return _paccProxy->accNavigate( navDir, varStart, pvarEndUpAt );
}

STDMETHODIMP CAccessibleBase::accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint )
{
    ASSERT(_paccProxy);
    return _paccProxy->accHitTest( xLeft, yTop, pvarChildAtPoint );
}

STDMETHODIMP CAccessibleBase::accDoDefaultAction( VARIANT varChild )
{
    ASSERT(_paccProxy);
    return _paccProxy->accDoDefaultAction( varChild );
}

STDMETHODIMP CAccessibleBase::put_accName( VARIANT varChild, BSTR bstrName )
{
    ASSERT(_paccProxy);
    return _paccProxy->put_accName( varChild, bstrName );
}

STDMETHODIMP CAccessibleBase::put_accValue( VARIANT varChild, BSTR bstrValue )
{
    ASSERT(_paccProxy);
    return _paccProxy->put_accValue( varChild, bstrValue );
}
