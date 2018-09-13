//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       f3util.hxx
//
//  Contents:   Header file defining utilities for use internally
//              by Forms3 and by language integration clients.
//
//----------------------------------------------------------------------------

#ifndef I_F3UTIL_HXX_
#define I_F3UTIL_HXX_
#pragma INCMSG("--- Beg 'f3util.hxx'")

interface IConnectionPointContainer;
interface IProvideClassInfo;

typedef TCHAR * LPTSTR;
typedef const TCHAR *LPCTSTR;

union ZERO_STRUCTS
{
    POINT       pt;
    POINTL      ptl;
    SIZE        size;
    SIZEL       sizel;
    RECTL       rcl;
    RECT        rc;
    TCHAR       ach[1];
    OLECHAR     achole[1];
    CHAR        acha[1];
    char        rgch[1];
    GUID        guid;
    IID         iid;
    CLSID       clsid;
    DISPPARAMS  dispparams;
    BYTE        ab[256];
};

extern ZERO_STRUCTS g_Zero;

//+---------------------------------------------------------------------------
//
//  Dispatch utilties
//
//----------------------------------------------------------------------------


#define VB_TRUE     ((VARIANT_BOOL)-1)           // TRUE for VARIANT_BOOL
#define VB_FALSE    ((VARIANT_BOOL)0)            // FALSE for VARIANT_BOOL

#undef VARIANT_TRUE            // The original versions of these cause
#undef VARIANT_FALSE           //   level 4 warnings.
#define VARIANT_TRUE VB_TRUE
#define VARIANT_FALSE VB_FALSE

#ifndef VT_TYPEMASK
#define VT_TYPEMASK (VARENUM) 0x0fff            // old value is 0x3ff
                                                // new value is from oleext.h
#endif

#define EVENTPARAMS_MAX 16

// don't define these if MFC has been included
#ifndef VTS_I2

#define EVENT_PARAM(vtsParams) (BYTE FAR*)(vtsParams)

// define reference variant types equal to (type | VT_BYREF)
#define VT_PI2              0x42        // a 'short*'
#define VT_PI4              0x43        // a 'long*'
#define VT_PR4              0x44        // a 'float*'
#define VT_PR8              0x45        // a 'double*'
#define VT_PCY              0x46        // a 'CY*'
#define VT_PDATE            0x47        // a 'DATE*'
#define VT_PBSTR            0x48        // a 'BSTR*'
#define VT_PDISPATCH        0x49        // an 'IDispatch**'
#define VT_PERROR           0x4A        // an 'SCODE*'
#define VT_PBOOL            0x4B        // a 'VARIANT_BOOL*'
#define VT_PVARIANT         0x4C        // a 'VARIANT*'
#define VT_PUNKNOWN         0x4D        // an 'IUnknown**'
#define VT_PUI1             0x51        // an 'unsigned char *'

// parameter types: by value VTs
#define VTS_I2              "\x02"      // a 'short'
#define VTS_I4              "\x03"      // a 'long'
#define VTS_R4              "\x04"      // a 'float'
#define VTS_R8              "\x05"      // a 'double'
#define VTS_CY              "\x06"      // a 'CY' or 'CY*'
#define VTS_DATE            "\x07"      // a 'DATE'
#define VTS_BSTR            "\x08"      // an 'LPCTSTR'
#define VTS_DISPATCH        "\x09"      // an 'IDispatch*'
#define VTS_ERROR           "\x0A"      // an 'ERROR'
#define VTS_BOOL            "\x0B"      // a 'BOOL'
#define VTS_VARIANT         "\x0C"      // a 'const VARIANT&' or 'VARIANT*'
#define VTS_UNKNOWN         "\x0D"      // an 'IUnknown*'
#define VTS_UI1             "\x11"      // an unsigned char

// parameter types: by reference VTs
#define VTS_PI2             "\x42"      // a 'short*'
#define VTS_PI4             "\x43"      // a 'long*'
#define VTS_PR4             "\x44"      // a 'float*'
#define VTS_PR8             "\x45"      // a 'double*'
#define VTS_PCY             "\x46"      // a 'CY*'
#define VTS_PDATE           "\x47"      // a 'DATE*'
#define VTS_PBSTR           "\x48"      // a 'BSTR*'
#define VTS_PDISPATCH       "\x49"      // an 'IDispatch**'
#define VTS_PERROR          "\x4A"      // an 'SCODE*'
#define VTS_PBOOL           "\x4B"      // a  'VARIANT_BOOL *'
#define VTS_PVARIANT        "\x4C"      // a 'VARIANT*'
#define VTS_PUNKNOWN        "\x4D"      // an 'IUnknown**'
#define VTS_PUI1            "\x51"      // an 'unsigned char *'

// special VT_ and VTS_ values
#define VTS_NONE            ""          // used for members with 0 params

// Ole control types
#define VTS_COLOR           VTS_I4      // OLE_COLOR
#define VTS_XPOS_PIXELS     VTS_I4      // OLE_XPOS_PIXELS
#define VTS_YPOS_PIXELS     VTS_I4      // OLE_YPOS_PIXELS
#define VTS_XSIZE_PIXELS    VTS_I4      // OLE_XSIZE_PIXELS
#define VTS_YSIZE_PIXELS    VTS_I4      // OLE_YSIZE_PIXELS
#define VTS_XPOS_HIMETRIC   VTS_I4      // OLE_XPOS_HIMETRIC
#define VTS_YPOS_HIMETRIC   VTS_I4      // OLE_YPOS_HIMETRIC
#define VTS_XSIZE_HIMETRIC  VTS_I4      // OLE_XSIZE_HIMETRIC
#define VTS_YSIZE_HIMETRIC  VTS_I4      // OLE_YSIZE_HIMETRIC
#define VTS_TRISTATE        VTS_I2      // OLE_TRISTATE
#define VTS_OPTEXCLUSIVE    VTS_BOOL    // OLE_OPTEXCLUSIVE

#define VTS_PCOLOR          VTS_PI4     // OLE_COLOR*
#define VTS_PXPOS_PIXELS    VTS_PI4     // OLE_XPOS_PIXELS*
#define VTS_PYPOS_PIXELS    VTS_PI4     // OLE_YPOS_PIXELS*
#define VTS_PXSIZE_PIXELS   VTS_PI4     // OLE_XSIZE_PIXELS*
#define VTS_PYSIZE_PIXELS   VTS_PI4     // OLE_YSIZE_PIXELS*
#define VTS_PXPOS_HIMETRIC  VTS_PI4     // OLE_XPOS_HIMETRIC*
#define VTS_PYPOS_HIMETRIC  VTS_PI4     // OLE_YPOS_HIMETRIC*
#define VTS_PXSIZE_HIMETRIC VTS_PI4     // OLE_XSIZE_HIMETRIC*
#define VTS_PYSIZE_HIMETRIC VTS_PI4     // OLE_YSIZE_HIMETRIC*
#define VTS_PTRISTATE       VTS_PI2     // OLE_TRISTATE*
#define VTS_POPTEXCLUSIVE   VTS_PBOOL   // OLE_OPTEXCLUSIVE*

#define VTS_FONT            VTS_DISPATCH    // IFontDispatch*
#define VTS_PICTURE         VTS_DISPATCH    // IPictureDispatch*

#define VTS_HANDLE          VTS_I4      // OLE_HANDLE
#define VTS_PHANDLE         VTS_PI4     // OLE_HANDLE*

#endif // #ifndef VTS_I2

inline BOOL DISPID_NOT_FOUND(HRESULT hr)
    { return hr==DISP_E_MEMBERNOTFOUND || hr==TYPE_E_LIBNOTREGISTERED; }

HRESULT SetErrorInfoFromEXCEPINFO(EXCEPINFO *pexcepinfo);

HRESULT GetNamedProp(
        IDispatch * pDisp,
        BSTR        bstrName,
        LCID        lcid,
        VARIANT   * pvarg,
        DISPID    * pdispid = NULL,
        EXCEPINFO * pexcepinfo = NULL,
        BOOL        fMethodCall = FALSE,
        BOOL        fCaseSensitive = FALSE);

HRESULT GetDispProp(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARIANT *   pvarg,
        EXCEPINFO * pexcepinfo = NULL,
        DWORD       dwFlags = DISPATCH_PROPERTYGET);

HRESULT SetDispProp(
        IDispatch *     pDisp,
        DISPID          dispid,
        LCID            lcid,
        VARIANTARG *    pvarg,
        EXCEPINFO *     pexcepinfo = NULL,
        DWORD           dwFlags = DISPATCH_PROPERTYPUT);

HRESULT GetDispPropByDispid(
        IDispatch *     pDisp,
        DISPID          dispidMember,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT  *         puArgErr);

HRESULT CDECL CallDispMethod(
        IServiceProvider   *pSrvProvider,
        IDispatch          *pDisp,
        DISPID              dispid,
        LCID                lcid,
        VARTYPE             vtReturn,
        void               *pvReturn,
        BYTE               *pbParams,
        ...);

HRESULT CDECL DispParamsToCParams(
        IServiceProvider   *pSrvProvider,
        DISPPARAMS         *pDP,
        ULONG              *pAlloc,
        WORD                wMaxstrlen,
        VARTYPE            *pVT,
        ...);

SAFEARRAY * DispParamsToSAFEARRAY (DISPPARAMS *pdispparams);

void CParamsToDispParams(
        DISPPARAMS *    pDispParams,
        BYTE *          pb,
        va_list         va);

BOOL IsVariantEqual(VARIANTARG FAR* pvar1, VARIANTARG FAR* pvar2);

HRESULT VariantChangeTypeSpecial(VARIANT *pvargDest, VARIANT *pVArg, VARTYPE vt, IServiceProvider *pSrvProvider = NULL, DWORD dwFlags = 0);

HRESULT VARIANTARGToCVar(VARIANT * pvarg, BOOL *pfAlloc, VARTYPE vt, void * pv, IServiceProvider *pSrvProvider = NULL, WORD wMaxstrlen = 0);

HRESULT VARIANTARGToIndex(VARIANT * pvarg, long * plIndex);

void CVarToVARIANTARG(void* pv, VARTYPE vt, VARIANTARG * pvarg);

BOOL IsVariantOmitted ( const VARIANT * pvarg );

// extension of VARTYPE;  used by databinding
class CVarType
{
public:
    CVarType(VARTYPE vt=VT_EMPTY, BOOL fInVariant=FALSE, BOOL fLocalized=FALSE):
                _vtBaseType(vt), _fInVariant(!!fInVariant), _fLocalized(!!fLocalized) {}
    CVarType& operator=(VARTYPE vt) { _vtBaseType=vt, _fInVariant=0, _fLocalized=0; return *this; }
    operator VARTYPE() const        { return _fInVariant? VT_VARIANT : _vtBaseType; }
    VARTYPE BaseType() const        { return _vtBaseType; }
    BOOL FInVariant() const         { return _fInVariant; }
    BOOL FLocalized() const         { return _fLocalized; }
    void SetLocalized(BOOL fLocalized) { _fLocalized = !!fLocalized; }
    BOOL IsEqual(const CVarType& vt)
        { return vt._vtBaseType==_vtBaseType && vt._fInVariant==_fInVariant &&
            vt._fLocalized==_fLocalized; }
private:
    VARTYPE     _vtBaseType;
    USHORT      _fInVariant:1;      // should value be wrapped in a variant?
    USHORT      _fLocalized:1;      // should value be localized?
};

STDAPI FormsOpenReadOnlyStorageOnMemory(
    LPVOID pv, IStorage ** ppStg);

STDAPI FormsOpenReadOnlyStorageOnResource(
    HINSTANCE hInst, LPCOLESTR lpstrID, IStorage ** ppStg);

STDAPI FormsCreateReadOnlyStorageOnStream(
    IStorage * pStgSrc, IStream * pStmDst);


//+------------------------------------------------------------------------
//
//  Resource utilities
//
//-------------------------------------------------------------------------

LPVOID GetResource(HINSTANCE hinst, LPCTSTR lpstrId, LPCTSTR lpstrType, ULONG * pcbSize);

//+------------------------------------------------------------------------
//
//  Connection point utilities
//
//-------------------------------------------------------------------------

HRESULT ConnectSink(
            IUnknown *  pUnkSource,
            REFIID      iid,
            IUnknown *  pUnkSink,
            DWORD *     pdwCookie);

HRESULT ConnectSinkWithCPC(
            IConnectionPointContainer * pCPC,
            REFIID                      iid,
            IUnknown *                  pUnkSink,
            DWORD *                     pdwCookie);

HRESULT DisconnectSink(
            IUnknown *  pUnkSource,
            REFIID      iid,
            DWORD *     pdwCookie);

HRESULT DisconnectSinkWithoutCookie(
            IUnknown *  pUnkSource,
            REFIID      iid,
            IUnknown *  pUnkConnection);

HRESULT GetDefaultDispatchTypeInfo(
            IProvideClassInfo * pPCI,
            BOOL                fSource,
            ITypeInfo **        ppTI);

//+------------------------------------------------------------------------
//
//  BSTR utilities
//
//-------------------------------------------------------------------------

#ifdef UNICODE
HRESULT FormsAllocStringW(LPCWSTR pch, BSTR * pBSTR);
HRESULT FormsAllocStringLenW(LPCWSTR pch, UINT uc, BSTR * pBSTR);
HRESULT FormsReAllocStringW(BSTR * pBSTR, LPCWSTR pch);
HRESULT FormsReAllocStringLenW(BSTR * pBSTR, LPCWSTR pch, UINT uc);
#else
HRESULT FormsAllocStringW(LPCTSTR pch, BSTR * pBSTR);
HRESULT FormsAllocStringLenW(LPCTSTR pch, UINT uc, BSTR * pBSTR);
HRESULT FormsReAllocStringW(BSTR * pBSTR, LPCWSTR pch);
HRESULT FormsReAllocStringLenW(BSTR * pBSTR, LPCSTR pch, UINT uc);
#endif // UNICODE
UINT    FormsStringLen(const BSTR bstr);
UINT    FormsStringByteLen(const BSTR bstr);
int     FormsStringCmp(LPCTSTR bstr1, LPCTSTR bstr2);
int     FormsStringNCmp(LPCTSTR bstr1, int cch1, LPCTSTR bstr2, int cch2);
int     FormsStringICmp(LPCTSTR bstr1, LPCTSTR bstr2);
int     FormsStringNICmp(LPCTSTR bstr1, int cch1, LPCTSTR bstr2, int cch2);
int     FormsStringCmpCase(LPCTSTR bstr1, LPCTSTR bstr2, BOOL fCaseSensitive);
void    FormsSplitAtDelimiter(LPCTSTR bstrName, BSTR *pbstrHead, BSTR *pbstrTail,
                                BOOL fFirst=TRUE, TCHAR tchDelim=_T('.'));

#if !defined(WIN16)
#   define  FormsAllocString FormsAllocStringW
#   define  FormsAllocStringLen FormsAllocStringLenW
#   define  FormsReAllocString FormsReAllocStringW
#   define  FormsReAllocStringLen FormsReAllocStringLenW

#else // _MAC && WIN16
HRESULT FormsAllocStringA(LPCSTR pch, BSTR * pBSTR);
HRESULT FormsAllocStringA(LPCWSTR pch, BSTR * pBSTR);
HRESULT FormsAllocStringLenA(LPCSTR pch, UINT uc, BSTR * pBSTR);
HRESULT FormsAllocStringLenA(LPCWSTR pch, UINT uc, BSTR * pBSTR);
HRESULT FormsReAllocStringA(BSTR * pBSTR, LPCSTR pch);
HRESULT FormsReAllocStringLenA(BSTR * pBSTR, LPCSTR pch, UINT uc);

#       define  FormsAllocString FormsAllocStringA
#       define  FormsReAllocString FormsReAllocStringA
#       define  FormsAllocStringLen FormsAllocStringLenA
#       define  FormsReAllocStringLen FormsReAllocStringLenA

int     FormsStringCmpA(LPCSTR bstr1, LPCSTR bstr2);  // 'A' suffix to catch unintended ANSI compares
int     FormsStringCmp(LPCSTR bstr1, LPCTSTR bstr2);  // for comparing BSTRs to Unicode strings
int     FormsStringCmp(LPCTSTR bstr1, LPCSTR bstr2);  // ditto

int     FormsStringICmpA(LPCSTR bstr1, LPCSTR bstr2);
int     FormsStringICmp(LPCSTR bstr1, LPCTSTR bstr2);
int     FormsStringICmp(LPCTSTR bstr1, LPCSTR bstr2);

#endif // _MAC && WIN16
//+---------------------------------------------------------------------------
//
//  Function:   STRVAL
//
//  Synopsis:   Returns the string unless it is NULL, in which case the empty
//              string is returned.
//
//----------------------------------------------------------------------------

inline LPCTSTR
STRVAL(LPCTSTR lpcwsz)
{
    return lpcwsz ? lpcwsz : g_Zero.ach;
}
#ifdef _MAC
#   ifdef UNICODE
inline LPCSTR
STRVAL(LPCSTR lpcsz)
{
    return lpcsz ? lpcsz : g_Zero.acha;
}
#   else
inline LPCWSTR
STRVAL(LPCWSTR lpcwsz)
{
    return lpcwsz ? lpcwsz : g_Zero.achw;
}
#   endif
#endif

//+---------------------------------------------------------------------------
//
//  Function:   FormsIsEmptyString
//
//  Synopsis:   Returns TRUE if the string is empty, FALSE otherwise.
//
//----------------------------------------------------------------------------

inline BOOL
FormsIsEmptyString(LPCTSTR lpcwsz)
{
    return !(lpcwsz && lpcwsz[0]);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsFreeString
//
//  Synopsis:   Frees a BSTR.
//
//----------------------------------------------------------------------------

inline void
FormsFreeString(BSTR bstr)
{
    SysFreeString(bstr);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsSplitFirstComponent
//
//  Synopsis:   Split a string into the first component (up to the first
//              delimiter) and everything else.
//
//----------------------------------------------------------------------------

inline void
FormsSplitFirstComponent(LPCTSTR bstrName, BSTR *pbstrHead, BSTR *pbstrTail,
                                TCHAR tchDelim=_T('.'))
{
    FormsSplitAtDelimiter(bstrName, pbstrHead, pbstrTail, TRUE, tchDelim);
}

//+---------------------------------------------------------------------------
//
//  Function:   FormsSplitLastComponent
//
//  Synopsis:   Split a string into everything else and the last component
//              (after the first delimiter).
//
//----------------------------------------------------------------------------

inline void
FormsSplitLastComponent(LPCTSTR bstrName, BSTR *pbstrHead, BSTR *pbstrTail,
                                TCHAR tchDelim=_T('.'))
{
    FormsSplitAtDelimiter(bstrName, pbstrHead, pbstrTail, FALSE, tchDelim);
}


//+------------------------------------------------------------------------
//
//  Interface and Class pointer manipulation
//
//-------------------------------------------------------------------------

//  Prototypes for actual out-of-line implementations of the
//    pointer management functions

void ClearInterfaceFn(IUnknown ** ppUnk);
void ClearClassFn(void ** ppv, IUnknown * pUnk);
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
    Assert((void *) pUnk == (void *) *ppI);
#endif

    ClearInterfaceFn((IUnknown **) ppI);
}
#endif


//+------------------------------------------------------------------------
//
//  Function:   ClearClass
//
//  Synopsis:   Nulls a pointer to a class, releasing the class via the
//              provided IUnknown implementation if the original pointer
//              is non-NULL.
//
//  Arguments:  [ppT]       Pointer to a class
//              [pUnk]      Pointer to the class's public IUnknown impl
//
//-------------------------------------------------------------------------

template <class PT>
inline void
ClearClass(PT * ppT, IUnknown * pUnk)
{
    ClearClassFn((void **) ppT, pUnk);
}



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
    Assert((void *) pUnk == (void *) *ppI);
#endif

    ReplaceInterfaceFn((IUnknown **) ppI, pI);
}
#endif

//
// Test for COM identity
//
BOOL IsSameObject(IUnknown *pUnkLeft, IUnknown *pUnkRight);


//+------------------------------------------------------------------------
//
//  Misc.
//
//-------------------------------------------------------------------------


#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

inline HRESULT
ClampITFResult(HRESULT hr)
{
    return (HRESULT_FACILITY(hr) == FACILITY_ITF) ? E_FAIL : hr;
}


#ifdef WIN16
#define OFFSET_PTR(t, p, cb) ((t) ((BYTE *)(void *)(p) + (cb) - ((p)->m_baseOffset)) )
#else
#define OFFSET_PTR(t, p, cb) ((t) ((BYTE *)(void *)(p) + (cb)))
#endif

inline BOOL
InRange(const TCHAR ch, const TCHAR chMin, const TCHAR chMax)
{
    return (unsigned)(ch - chMin) <= (unsigned)(chMax - chMin);
}

#pragma INCMSG("--- End 'f3util.hxx'")
#else
#pragma INCMSG("*** Dup 'f3util.hxx'")
#endif
