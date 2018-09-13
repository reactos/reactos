
class COleVariant;          // OLE VARIANT wrapper
class COleDateTime;         // Based on OLE DATE
class CLongBinary;          // forward reference (see afxdb_.h)
/*
class COleCurrency;         // Based on OLE CY
class COleDateTimeSpan;     // Based on a double
class COleSafeArray;        // Based on OLE VARIANT
*/
/////////////////////////////////////////////////////////////////////////////

// AFXDLL support
#undef AFX_DATA
#define AFX_DATA AFX_OLE_DATA

#include <delaydll.h>


// parameter types: by value VTs
#define VTS_I2              "\x02"      // a 'short'
#define VTS_I4              "\x03"      // a 'long'
#define VTS_R4              "\x04"      // a 'float'
#define VTS_R8              "\x05"      // a 'double'
#define VTS_CY              "\x06"      // a 'CY' or 'CY*'
#define VTS_DATE            "\x07"      // a 'DATE'
#define VTS_WBSTR           "\x08"      // an 'LPCOLESTR'
#define VTS_DISPATCH        "\x09"      // an 'IDispatch*'
#define VTS_SCODE           "\x0A"      // an 'SCODE'
#define VTS_BOOL            "\x0B"      // a 'BOOL'
#define VTS_VARIANT         "\x0C"      // a 'const VARIANT&' or 'VARIANT*'
#define VTS_UNKNOWN         "\x0D"      // an 'IUnknown*'
#if defined(_UNICODE) || defined(OLE2ANSI)
        #define VTS_BSTR            VTS_WBSTR// an 'LPCOLESTR'
        #define VT_BSTRT            VT_BSTR
#else
        #define VTS_BSTR            "\x0E"  // an 'LPCSTR'
        #define VT_BSTRA            14
        #define VT_BSTRT            VT_BSTRA
#endif

// parameter types: by reference VTs
#define VTS_PI2             "\x42"      // a 'short*'
#define VTS_PI4             "\x43"      // a 'long*'
#define VTS_PR4             "\x44"      // a 'float*'
#define VTS_PR8             "\x45"      // a 'double*'
#define VTS_PCY             "\x46"      // a 'CY*'
#define VTS_PDATE           "\x47"      // a 'DATE*'
#define VTS_PBSTR           "\x48"      // a 'BSTR*'
#define VTS_PDISPATCH       "\x49"      // an 'IDispatch**'
#define VTS_PSCODE          "\x4A"      // an 'SCODE*'
#define VTS_PBOOL           "\x4B"      // a 'VARIANT_BOOL*'
#define VTS_PVARIANT        "\x4C"      // a 'VARIANT*'
#define VTS_PUNKNOWN        "\x4D"      // an 'IUnknown**'

// special VT_ and VTS_ values
#define VTS_NONE            NULL        // used for members with 0 params
#define VT_MFCVALUE         0xFFF       // special value for DISPID_VALUE
#define VT_MFCBYREF         0x40        // indicates VT_BYREF type
#define VT_MFCMARKER        0xFF        // delimits named parameters (INTERNAL USE)

// variant handling (use V_BSTRT when you have ANSI BSTRs, as in DAO)
#ifndef _UNICODE
        #define V_BSTRT(b)  (LPSTR)V_BSTR(b)
#else
        #define V_BSTRT(b)  V_BSTR(b)
#endif


/////////////////////////////////////////////////////////////////////////////
// COleVariant class - wraps VARIANT types

typedef const VARIANT* LPCVARIANT;

class COleVariant : public tagVARIANT
{
// Constructors
public:
        COleVariant();

        COleVariant(const VARIANT& varSrc);
        COleVariant(LPCVARIANT pSrc);
        COleVariant(const COleVariant& varSrc);

        COleVariant(LPCTSTR lpszSrc);
        COleVariant(LPCTSTR lpszSrc, VARTYPE vtSrc); // used to set to ANSI string
        COleVariant(CString& strSrc);

        COleVariant(BYTE nSrc);
        COleVariant(short nSrc, VARTYPE vtSrc = VT_I2);
        COleVariant(long lSrc, VARTYPE vtSrc = VT_I4);
        //COleVariant(const COleCurrency& curSrc);

        COleVariant(float fltSrc);
        COleVariant(double dblSrc);
        COleVariant(const COleDateTime& timeSrc);

        //COleVariant(const CByteArray& arrSrc);
        COleVariant(const CLongBinary& lbSrc);

// Operations
public:
        void Clear();
        void ChangeType(VARTYPE vartype, LPVARIANT pSrc = NULL);
        void Attach(VARIANT& varSrc);
        VARIANT Detach();

        BOOL operator==(const VARIANT& varSrc) const;
        BOOL operator==(LPCVARIANT pSrc) const;

        const COleVariant& operator=(const VARIANT& varSrc);
        const COleVariant& operator=(LPCVARIANT pSrc);
        const COleVariant& operator=(const COleVariant& varSrc);

        const COleVariant& operator=(const LPCTSTR lpszSrc);
        const COleVariant& operator=(const CString& strSrc);

        const COleVariant& operator=(BYTE nSrc);
        const COleVariant& operator=(short nSrc);
        const COleVariant& operator=(long lSrc);
        //const COleVariant& operator=(const COleCurrency& curSrc);

        const COleVariant& operator=(float fltSrc);
        const COleVariant& operator=(double dblSrc);
        const COleVariant& operator=(const COleDateTime& dateSrc);

        //const COleVariant& operator=(const CByteArray& arrSrc);
        const COleVariant& operator=(const CLongBinary& lbSrc);

        void SetString(LPCTSTR lpszSrc, VARTYPE vtSrc); // used to set ANSI string

        HRESULT Save(IStream *pStm, BOOL fClearDirty = FALSE);
        HRESULT Load(IStream *pStm);
        //HRESULT GetSizeMax(ULARGE_INTEGER *pcbSize);
        HRESULT GetSizeMax(ULONG *pcbSize);


        operator LPVARIANT();
        operator LPCVARIANT() const;

// Implementation
public:
        ~COleVariant();
};

// COleVariant diagnostics and serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleVariant varSrc);
#endif
//CArchive& AFXAPI operator<<(CArchive& ar, COleVariant varSrc);
//CArchive& AFXAPI operator>>(CArchive& ar, COleVariant& varSrc);

// Helper for initializing VARIANT structures
void AFXAPI AfxVariantInit(LPVARIANT pVar);

class COleSafeArray;

/////////////////////////////////////////////////////////////////////////////
// Helper for initializing COleSafeArray
void AFXAPI AfxSafeArrayInit(COleSafeArray* psa);

/////////////////////////////////////////////////////////////////////////////
// CSafeArray class

typedef const SAFEARRAY* LPCSAFEARRAY;

class COleSafeArray : public tagVARIANT
{
//Constructors
public:
        COleSafeArray();
        COleSafeArray(const SAFEARRAY& saSrc, VARTYPE vtSrc);
        COleSafeArray(LPCSAFEARRAY pSrc, VARTYPE vtSrc);
        COleSafeArray(const COleSafeArray& saSrc);
        COleSafeArray(const VARIANT& varSrc);
        COleSafeArray(LPCVARIANT pSrc);
        COleSafeArray(const COleVariant& varSrc);

// Operations
public:
        void Clear();
        void Attach(VARIANT& varSrc);
        VARIANT Detach();

        COleSafeArray& operator=(const COleSafeArray& saSrc);
        COleSafeArray& operator=(const VARIANT& varSrc);
        COleSafeArray& operator=(LPCVARIANT pSrc);
        COleSafeArray& operator=(const COleVariant& varSrc);

        BOOL operator==(const SAFEARRAY& saSrc) const;
        BOOL operator==(LPCSAFEARRAY pSrc) const;
        BOOL operator==(const COleSafeArray& saSrc) const;
        BOOL operator==(const VARIANT& varSrc) const;
        BOOL operator==(LPCVARIANT pSrc) const;
        BOOL operator==(const COleVariant& varSrc) const;

        operator LPVARIANT();
        operator LPCVARIANT() const;

        // One dim array helpers
        void CreateOneDim(VARTYPE vtSrc, DWORD dwElements,
                void* pvSrcData = NULL, long nLBound = 0);
        DWORD GetOneDimSize();
        void ResizeOneDim(DWORD dwElements);

        // Multi dim array helpers
        void Create(VARTYPE vtSrc, DWORD dwDims, DWORD* rgElements);

        // SafeArray wrapper classes
        void Create(VARTYPE vtSrc, DWORD dwDims, SAFEARRAYBOUND* rgsabounds);
        void AccessData(void** ppvData);
        void UnaccessData();
        void AllocData();
        void AllocDescriptor(DWORD dwDims);
        void Copy(LPSAFEARRAY* ppsa);
        void GetLBound(DWORD dwDim, long* pLBound);
        void GetUBound(DWORD dwDim, long* pUBound);
        void GetElement(long* rgIndices, void* pvData);
        void PtrOfIndex(long* rgIndices, void** ppvData);
        void PutElement(long* rgIndices, void* pvData);
        void Redim(SAFEARRAYBOUND* psaboundNew);
        void Lock();
        void Unlock();
        DWORD GetDim();
        DWORD GetElemSize();
        void Destroy();
        void DestroyData();
        void DestroyDescriptor();

//Implementation
public:
        ~COleSafeArray();

        // Cache info to make element access (operator []) faster
        DWORD m_dwElementSize;
        DWORD m_dwDims;
};

// COleSafeArray diagnostics and serialization
#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, COleSafeArray saSrc);
#endif
//CArchive& AFXAPI operator<<(CArchive& ar, COleSafeArray saSrc);
//CArchive& AFXAPI operator>>(CArchive& ar, COleSafeArray& saSrc);

#define _AFXDISP_INLINE inline

#ifdef _AFX_INLINE

// COleVariant
_AFXDISP_INLINE COleVariant::COleVariant()
        { AfxVariantInit(this); }
_AFXDISP_INLINE COleVariant::~COleVariant()
        { ::VariantClear(this); }
_AFXDISP_INLINE COleVariant::COleVariant(LPCTSTR lpszSrc)
        { vt = VT_EMPTY; *this = lpszSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(CString& strSrc)
        { vt = VT_EMPTY; *this = strSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(BYTE nSrc)
        { vt = VT_UI1; bVal = nSrc; }
//_AFXDISP_INLINE COleVariant::COleVariant(const COleCurrency& curSrc)
//        { vt = VT_CY; cyVal = curSrc.m_cur; }
_AFXDISP_INLINE COleVariant::COleVariant(float fltSrc)
        { vt = VT_R4; fltVal = fltSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(double dblSrc)
        { vt = VT_R8; dblVal = dblSrc; }
//_AFXDISP_INLINE COleVariant::COleVariant(const COleDateTime& dateSrc)
//        { vt = VT_DATE; date = dateSrc.m_dt; }
//_AFXDISP_INLINE COleVariant::COleVariant(const CByteArray& arrSrc)
//        { vt = VT_EMPTY; *this = arrSrc; }
_AFXDISP_INLINE COleVariant::COleVariant(const CLongBinary& lbSrc)
        { vt = VT_EMPTY; *this = lbSrc; }
_AFXDISP_INLINE BOOL COleVariant::operator==(LPCVARIANT pSrc) const
        { return *this == *pSrc; }
_AFXDISP_INLINE COleVariant::operator LPVARIANT()
        { return this; }
_AFXDISP_INLINE COleVariant::operator LPCVARIANT() const
        { return this; }

#endif //_AFX_INLINE
/*
/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXDISP_INLINE inline
//#include <afxole.inl>
#undef _AFXDISP_INLINE
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif
#ifndef _AFX_FULLTYPEINFO
#pragma component(mintypeinfo, off)
#endif
*/

/////////////////////////////////////////////////////////////////////////////
