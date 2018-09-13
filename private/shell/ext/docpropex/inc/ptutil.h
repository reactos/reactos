//-------------------------------------------------------------------------//
//
//	PTutil.h
//
//-------------------------------------------------------------------------//

#ifndef __PTUTIL_H__
#define __PTUTIL_H__

//  Forwards
struct tagPROPFOLDERITEM ;
struct tagPROPERTYITEM ;
struct tagBASICPROPITEM ;

//-------------------------------------------------------------------------//
EXTERN_C void InitPropFolderItem( struct tagPROPFOLDERITEM* pItem ) ;
EXTERN_C BOOL CopyPropFolderItem( struct tagPROPFOLDERITEM* pDest, const struct tagPROPFOLDERITEM* pSrc ) ;
EXTERN_C void ClearPropFolderItem( struct tagPROPFOLDERITEM* pItem ) ;
EXTERN_C BOOL IsValidPropFolderItem( const struct tagPROPFOLDERITEM* pItem ) ;

EXTERN_C void InitPropertyItem( struct tagPROPERTYITEM* pItem ) ;
EXTERN_C BOOL CopyPropertyItem( struct tagPROPERTYITEM* pDest, const struct tagPROPERTYITEM* pSrc ) ;
EXTERN_C void ClearPropertyItem( struct tagPROPERTYITEM* pItem ) ;
EXTERN_C BOOL IsValidPropertyItem( const struct tagPROPERTYITEM* pItem ) ;

EXTERN_C void InitBasicPropertyItem( struct tagBASICPROPITEM* pItem ) ;
EXTERN_C BOOL CopyBasicPropertyItem( struct tagBASICPROPITEM* pDest, const struct tagBASICPROPITEM* pSrc ) ;
EXTERN_C void ClearBasicPropertyItem( struct tagBASICPROPITEM* pItem ) ;
EXTERN_C BOOL IsValidBasicPropertyItem( const struct tagBASICPROPITEM* pItem ) ;

EXTERN_C void InitPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pItem ) ;
EXTERN_C BOOL CopyPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pDest, const struct tagPROPVARIANT_DISPLAY* pSrc ) ;
EXTERN_C void ClearPropVariantDisplay( struct tagPROPVARIANT_DISPLAY* pItem ) ;
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
EXTERN_C BOOL PTCheckWriteAccessA( LPCSTR pszFile );
EXTERN_C BOOL PTCheckWriteAccessW( LPCWSTR pwszFile );

EXTERN_C ULONG PTGetFileAttributesA( LPCSTR pszFile );
EXTERN_C ULONG PTGetFileAttributesW( LPCWSTR pwszFile );

#ifdef UNICODE
#define PTCheckWriteAccess  PTCheckWriteAccessW
#define PTGetFileAttributes PTGetFileAttributesW
#else
#define PTCheckWriteAccess  PTCheckWriteAccessA
#define PTGetFileAttributes PTGetFileAttributesA
#endif

//-------------------------------------------------------------------------//
EXTERN_C LPWSTR WINAPI StrA2W( UINT nCodePage, LPWSTR pwsz, LPCSTR pasz, int nChars) ;
EXTERN_C LPSTR  WINAPI StrW2A( UINT nCodePage, LPSTR pasz, LPCWSTR pwsz, int nChars) ;
EXTERN_C BOOL   WINAPI StrRoundTripsW( UINT nCodePage, LPCWSTR pwsz ) ;
EXTERN_C BOOL   WINAPI StrRoundTripsA( UINT nCodePage, LPCSTR pasz ) ;

#if 0
//  See bug# #329259 for info on these methods.
//-------------------------------------------------------------------------//
EXTERN_C BOOL   IsValidSystemTime( SYSTEMTIME* pst );
EXTERN_C BOOL   IsValidFileTime( FILETIME* pft );
EXTERN_C BOOL   IsValidVariantTime( DATE* pvt );
//-------------------------------------------------------------------------//
#endif 0

//-------------------------------------------------------------------------//
//  Recursion-compatible string tokenizing routine.
typedef struct tagSTRTOK
{
    LPCTSTR            pszTok ;
    int                cchTok ;
    struct tagSTRTOK*  pNext ;
    ULONG              Reserved ;
} STRTOK, *PSTRTOK, *LPSTRTOK ;


EXTERN_C int GetTokens( LPTSTR pszString, LPCTSTR pszDelims, OUT PSTRTOK *ppList ) ;
EXTERN_C int FreeTokens( PSTRTOK *ppList ) ;
//-------------------------------------------------------------------------//

#define STRICT_COMPARE   (PVCF_IGNORETIME)
#define LAX_COMPARE      (PVCF_IGNORECASE|PVCF_IGNORETIME)


//-------------------------------------------------------------------------//
//  Accessibility primitives
//-------------------------------------------------------------------------//

#include <oleacc.h>

//-------------------------------------------------------------------------//
HRESULT _CreateStdAccessibleProxy( HWND hwnd, WPARAM wParam, IAccessible** ppacc );
LRESULT _LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN punk );

//-------------------------------------------------------------------------//
//  class CAccessibleBase
//
//  common IAccessible implementation.
//
class CAccessibleBase : public IAccessible, public IOleWindow
//-------------------------------------------------------------------------//
{
public:
    static BOOL Init();
    static BOOL Uninit();

public:
    CAccessibleBase()
        :   _cRef(1), _ptiAcc(NULL), _hwnd(NULL), _paccProxy(NULL)
    { 
        DllAddRef();
    }
    
    virtual ~CAccessibleBase()
    { 
        ATOMICRELEASE(_paccProxy);
        ATOMICRELEASE(_ptiAcc);
    }

    void Initialize( HWND hwnd, IAccessible* paccProxy )
    { 
        ASSERT(IsWindow( hwnd ));
        ASSERT(paccProxy);
        _hwnd = hwnd;
        if( (_paccProxy = paccProxy) != NULL )
            _paccProxy->AddRef();
    }

    //  IUnknown
    STDMETHODIMP         QueryInterface( REFIID riid, void** ppvObj );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //  IOleWindow
    STDMETHODIMP GetWindow( HWND* phwnd );
    STDMETHODIMP ContextSensitiveHelp( BOOL fEnterMode ) { return E_NOTIMPL; }

    // IDispatch
    STDMETHODIMP GetTypeInfoCount( UINT * pctinfo );
    STDMETHODIMP GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** pptinfo );
    STDMETHODIMP GetIDsOfNames( REFIID riid, OLECHAR** rgszNames, UINT cNames,
                                LCID lcid, DISPID * rgdispid );
    STDMETHODIMP Invoke( DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                         DISPPARAMS * pdispparams, VARIANT * pvarResult, 
                         EXCEPINFO * pexcepinfo, UINT * puArgErr );
    //  IAccessible
    STDMETHODIMP get_accParent( IDispatch ** ppdispParent);
    STDMETHODIMP get_accChildCount( long * pcChildren );
    STDMETHODIMP get_accChild( VARIANT varChildIndex, IDispatch ** ppdispChild);
    STDMETHODIMP get_accName( VARIANT varChild, BSTR* pbstrName);
    STDMETHODIMP get_accValue( VARIANT varChild, BSTR* pbstrValue);
    STDMETHODIMP get_accDescription( VARIANT varChild, BSTR * pbstrDescription);
    STDMETHODIMP get_accRole( VARIANT varChild, VARIANT *pvarRole );
    STDMETHODIMP get_accState( VARIANT varChild, VARIANT *pvarState );
    STDMETHODIMP get_accHelp( VARIANT varChild, BSTR* pbstrHelp );
    STDMETHODIMP get_accHelpTopic( BSTR* pbstrHelpFile, VARIANT varChild, long* pidTopic );
    STDMETHODIMP get_accKeyboardShortcut( VARIANT varChild, BSTR* pbstrKeyboardShortcut );
    STDMETHODIMP get_accFocus( VARIANT FAR * pvarFocusChild );
    STDMETHODIMP get_accSelection( VARIANT FAR * pvarSelectedChildren );
    STDMETHODIMP get_accDefaultAction( VARIANT varChild, BSTR* pbstrDefaultAction );
    STDMETHODIMP accSelect( long flagsSelect, VARIANT varChild );
    STDMETHODIMP accLocation( long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild );
    STDMETHODIMP accNavigate( long navDir, VARIANT varStart, VARIANT * pvarEndUpAt );
    STDMETHODIMP accHitTest( long xLeft, long yTop, VARIANT * pvarChildAtPoint );
    STDMETHODIMP accDoDefaultAction( VARIANT varChild );
    STDMETHODIMP put_accName( VARIANT varChild, BSTR bstrName );
    STDMETHODIMP put_accValue( VARIANT varChild, BSTR bstrValue );

protected:
    virtual UINT GetDefaultActionStringID() const { return 0; }

    IAccessible*    _paccProxy;
    HWND            _hwnd;
private:
    ULONG           _cRef;
    ITypeInfo*      _ptiAcc;

    #define VALIDATEACCCHILD( varChild, idChild, hrFail ) \
        if( !(VT_I4 == varChild.vt && idChild == varChild.lVal) ) {return hrFail;}
};


UINT WMU_GETACCSTRING();    
    // wparam: disp id of requested string
    // lparam: HTREEITEM.



#endif __PTUTIL_H__
