#ifndef __UTILS_HXX__
#define __UTILS_HXX__


//=======================================================
//
//   File : utils.hxx 
//
//   purpose : some useful helper functions
//
//=========================================================


#define DATE_STR_LENGTH            30
#define INTERNET_COOKIE_SIZE_LIMIT 4096
#define MAX_SCRIPT_BLOCK_SIZE      4096
#define VB_TRUE                    ((VARIANT_BOOL)-1)     // TRUE for VARIANT_BOOL
#define VB_FALSE                   ((VARIANT_BOOL)0)      // FALSE for VARIANT_BOOL
#define LCID_SCRIPTING             0x0409                 // Mandatory VariantChangeTypeEx localeID
#define ISSPACE(ch) (((ch) == _T(' ')) || ((unsigned)((ch) - 9)) <= 13 - 9)



// VARIANT conversion interface exposed by script engines (VBScript/JScript).
EXTERN_C const GUID SID_VariantConversion;

//+------------------------------------------------------------------------
//
//  Interface and Class pointer manipulation
//
//-------------------------------------------------------------------------

//  Prototypes for actual out-of-line implementations of the
//    pointer management functions

void ClearInterfaceFn(IUnknown ** ppUnk);
void ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk);
void ReleaseInterface(IUnknown * pUnk);

//  Inline portions

//+------------------------------------------------------------------------
//
//  Function:   ClearInterface
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppI]   *ppI is cleared
//
//-------------------------------------------------------------------------

#ifdef WIN16
#define ClearInterface(p)	ClearInterfaceFn((IUnknown **)p)
#else
template <class PI>
inline void
ClearInterface(PI * ppI)
{
#if DBG == 1
    IUnknown * pUnk = *ppI;
#endif

    ClearInterfaceFn((IUnknown **) ppI);
}
#endif


//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterface
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppI is set to pI
//              = if pI is not NULL, it is AddRef'd
//              = if *ppI was not NULL initially, it is Release'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppI]     Destination pointer in *ppI
//              [pI]      Source pointer in pI
//
//-------------------------------------------------------------------------
#ifdef WIN16
#define ReplaceInterface(ppI, pI)	ReplaceInterfaceFn((IUnknown **)ppI, pI)
#else
template <class PI>
inline void
ReplaceInterface(PI * ppI, PI pI)
{
#if DBG == 1
    IUnknown * pUnk = *ppI;
#endif

    ReplaceInterfaceFn((IUnknown **) ppI, pI);
}
#endif



//================================================================
//
//    CLASS:  CBufferedStr  - helper class
//  
//================================================================

class CBufferedStr
{
public:
    CBufferedStr::CBufferedStr() 
    {
        _pchBuf = NULL;
        _cchBufSize = 0;
        _cchIndex =0;
    }
    CBufferedStr::~CBufferedStr() { Free(); }

    //
    //  Creates a buffer and initializes it with the supplied TCHAR*
    //---------------------------------------------------------------
    HRESULT Set( LPCTSTR pch = NULL, UINT cch=0 );
    void Free () { delete [] _pchBuf; }

    //
    //  Adds at the end of the buffer, growing it it necessary.
    //---------------------------------------------------------------
    HRESULT QuickAppend ( const TCHAR* pch ) { return(QuickAppend(pch, _tcslen(pch))); }
    HRESULT QuickAppend ( const TCHAR* pch , ULONG cch );
    HRESULT QuickAppend ( long lValue );

    //
    //  Returns current size of the buffer
    //---------------------------------------------------------------
    UINT    Size() { return _pchBuf ? _cchBufSize : 0; }

    //
    //  Returns current length of the buffer string
    //---------------------------------------------------------------
    UINT    Length() { return _pchBuf ? _cchIndex : 0; }

    operator LPTSTR () const { return _pchBuf; }

    TCHAR * _pchBuf;                // Actual buffer
    UINT    _cchBufSize;            // Size of _pchBuf
    UINT    _cchIndex;              // Length of _pchBuf
};


//================================================================
//
//    CLASS:  CVariant  - helper class
//  
//================================================================

class CVariant : public VARIANT
{
public:
    CVariant()  { ZeroVariant(); }
    CVariant(VARTYPE vt) { ZeroVariant(); V_VT(this) = vt; }
    ~CVariant() { Clear(); }

    void ZeroVariant()
    {
        ((DWORD *)this)[0] = 0; ((DWORD *)this)[1] = 0; ((DWORD *)this)[2] = 0;
    }

    HRESULT Copy(VARIANT *pVar)
    {
        return pVar ? VariantCopy(this, pVar) : E_POINTER ;
    }

    HRESULT Clear()
    {
        return VariantClear(this);
    }

    // Coerce from an arbitrary variant into this. (Copes with VATIANT BYREF's within VARIANTS).
    HRESULT CoerceVariantArg (VARIANT *pArgFrom, WORD wCoerceToType);
    // Coerce current variant into itself
    HRESULT CoerceVariantArg (WORD wCoerceToType);
    BOOL CoerceNumericToI4 ();
    BOOL IsEmpty() { return (vt == VT_NULL || vt == VT_EMPTY);}
    BOOL IsOptional() { return (IsEmpty() || vt == VT_ERROR);}
};


//============================================================
//
//  Class : CDataListEnumerator
//
//      A simple parseing helper -- this returns a token separated by spaces
//          or by a separation character that is passed into the constructor.
//
//============================================================
class CDataListEnumerator
{
public:
        CDataListEnumerator ( LPCTSTR pszString, TCHAR ch=_T(' '))
        {
            Assert ( pszString );
            pStr = pszString;
            chSeparator = ch;
        };

        BOOL    GetNext ( LPCTSTR *ppszNext, INT *pnLen );

private:
        LPCTSTR pStr;
        LPCTSTR pStart;
        TCHAR   chSeparator;

};

inline BOOL 
CDataListEnumerator::GetNext ( LPCTSTR *ppszNext, INT *pnLen )
{
    // Find the start, skipping spaces and separators
    while ( ISSPACE(*pStr) || *pStr == chSeparator ) pStr++;
    pStart = pStr;

    if ( !*pStr )
        return FALSE;

    // Find the end of the next token
    for (;!(*pStr == _T('\0') || *pStr == chSeparator || ISSPACE(*pStr)); pStr++);

    *pnLen = (INT)(pStr - pStart);
    *ppszNext = pStart;

    return TRUE;
}



//=============================================================
//
//  Helper functions
//
//============================================================

HRESULT ConvertGmtTimeToString(FILETIME Time, TCHAR * pchDateStr, DWORD cchDateStr);
HRESULT ParseDate(BSTR strDate, FILETIME * pftTime);
int     UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch=-1);
int     MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch=-1);
HRESULT VariantChangeTypeSpecial(VARIANT *pvargDest, 
                                 VARIANT *pVArg, 
                                 VARTYPE vt, 
                                 IServiceProvider *pSrvProvider = NULL, 
                                 WORD wFlags = 0);
const TCHAR * __cdecl _tcsistr (const TCHAR * tcs1,const TCHAR * tcs2);
HRESULT GetClientSiteWindow(IElementBehaviorSite *pSite, HWND *phWnd);
STDMETHODIMP GetHTMLDocument(IElementBehaviorSite * pSite, 
                             IHTMLDocument2 **ppDoc);
STDMETHODIMP GetHTMLWindow(IElementBehaviorSite * pSite, 
                           IHTMLWindow2 **ppWindow);


BOOL AccessAllowed(BSTR bstrUrl, IUnknown * pUnkSite);

#endif  //__UTILS_HXX__
