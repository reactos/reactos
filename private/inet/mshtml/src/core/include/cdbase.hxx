//+---------------------------------------------------------------------------
//
//  Microsoft Forms3
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdbase.hxx
//
//  Contents:   The OLE Server base class set
//
//----------------------------------------------------------------------------

#ifndef I_CDBASE_HXX_
#define I_CDBASE_HXX_
#pragma INCMSG("--- Beg 'cdbase.hxx'")

#pragma warning(disable:4244) // conversion from 'type' to 'type', possible loss of data

interface IDispatchEx;
interface IOleUndoManager;
interface IRecalcEngine;
interface IHTMLWindow2;

MtExtern(CInPlace)
MtExtern(CFunctionPointer)
MtExtern(CAttrArray)
MtExtern(CAttrArray_pv)
MtExtern(CEnumConnections)
MtExtern(CRecalcInfo)
MtExtern(CAttrArrayHeader)

ExternTag(tagRecalcDetail);


// Mandatory VariantChangeTypeEx localeID
#define LCID_SCRIPTING 0x0409

// maximum url length for string props \ method args that are of type url in the .pdl files
#define pdlUrlLen 4096
#define pdlToken 128            // strings that really are some form of a token
#define pdlLength 128           // strings that really are numeric lengths
#define pdlColor 128            // strings that really are color values
#define pdlNoLimit 0xFFFF       // strings that have no limit on their max lengths
#define pdlEvent pdlNoLimit     // strings that could be assigned to onfoo event properties

//  Public window messages

extern UINT g_WM_GETOBJECTID;

// Forward declarations:
class CBase;
class CUndoPropChange;
class CParentUndoUnit;
class CAtomTable;
class CAttrArray;
struct IHTMLDocument2;
class CDoc;
class CElement;

#define VBSERR_OutOfStack   0x800a001c
#define VBSERR_OutOfMemory  0x800a0007

// The index into the list of event iid's that is the primary event iid.
#define CPI_OFFSET_PRIMARYDISPEVENT 1

// flag for determine when gion is called from ie40. was in dispex.idl
#define fdexFromGetIdsOfNames       0x80000000L

// flags for get/Set Attribute/Expression
#define GETMEMBER_CASE_SENSITIVE  0x00000001
#define GETMEMBER_AS_SOURCE  0x00000002

//+------------------------------------------------------------------------
//
//  struct CONNECTION_POINT_INFO (cpi) contains information about
//  an object's connection points.
//
//-------------------------------------------------------------------------

struct CONNECTION_POINT_INFO
{
    const IID * piid;                  // iid of connection point
    DISPID      dispid;                // dispid under which to store sinks
};

#define CPI_ENTRY(iid, dispid)  \
    {                           \
        &iid,                   \
        dispid,                 \
    },                          \

#define CPI_ENTRY_NULL          \
    {                           \
        NULL,                   \
        DISPID_UNKNOWN,         \
    },                          \
    

//  Private window messages

#define WM_SERVER_FIRST         (WM_APP +1)
#define WM_SERVER_LAST          (WM_SERVER_FIRST + 1)

//+---------------------------------------------------------------------------
//
//  Macros for declaring Non-Virtual methods in support of tearoff
//              implementation of standard OLE interfaces.  (Tearoffs require
//              STDMETHODCALLTYPE.)
//
//----------------------------------------------------------------------------
#define NV_STDMETHOD(method)        HRESULT STDMETHODCALLTYPE method
#define NV_STDMETHOD_(type, method) type STDMETHODCALLTYPE method

struct PROPERTYDESC;
struct VTABLEDESC
{
    const PROPERTYDESC     *pPropDesc;
    UINT                    uVTblEntry;
};

// Sorted property desc array.
struct HDLDESC
{
    // Mondo disp interface IID
    const GUID                 *_piidOfMondoDispInterface;

    // PropDesc array
    const PROPERTYDESC * const *ppPropDescs;
    UINT                        uNumPropDescs;  // Max value binary search.

    //VTable array
    VTABLEDESC const           *pVTableArray;
    UINT                        uVTblIndex;     // Max value binary search.
    // PropDesc array in Vtbl order
    const PROPERTYDESC * const *ppVtblPropDescs;
    const PROPERTYDESC * FindPropDescForName ( LPCTSTR szName, BOOL fCaseSensitive = FALSE, long *pidx = NULL ) const;

};

#ifdef _MAC 
#define MAX_ENUMPAIRS 32
#else
#define MAX_ENUMPAIRS
#endif

struct ENUMDESC
{
    WORD cEnums;
    DWORD dwMask;
    struct ENUMPAIR
    {
        TCHAR * pszName;
        long     iVal;
    } aenumpairs[MAX_ENUMPAIRS];

    HRESULT EnumFromString ( LPCTSTR pbstr, long *lValue, BOOL fCaseSensitive = FALSE ) const;
    HRESULT StringFromEnum ( long lValue, BSTR *pbstr ) const;
    LPCTSTR StringPtrFromEnum ( long lValue ) const;
};

struct PROPERTYDESC;

struct BASICPROPPARAMS
{
    DWORD   dwPPFlags;      // PROPPARAM_FLAGS |'d together
    DISPID  dispid;         // automation id
    DWORD   dwFlags;        // flags pass to OnPropertyChange (ELEMCHNG_FLAG)
    WORD    wInvFunc;       // Index into function Invoke table
    WORD    wMaxstrlen;     // maxlength for string and Variant types that can contain strings

    const PROPERTYDESC *GetPropertyDesc ( void ) const;
    HRESULT GetAvString (void * pObject, const void * pvParams, CStr *pstr, BOOL *pfValuePresent = NULL ) const;
    DWORD   GetAvNumber (void * pObject, const void * pvParams, UINT uNumBytes, BOOL *pfValuePresent = NULL  ) const;
    HRESULT SetAvNumber ( void *pObject, DWORD dwNumber, const void *pvParams, UINT uNumberBytes, WORD wFlags ) const;

    void GetGetMethodP(const void * pvParams, void * pfn) const;
    void GetSetMethodP(const void * pvParams, void * pfn) const;

    HRESULT GetColor(CVoid * pObject, BSTR * pbstrColor, BOOL fReturnAsHex=FALSE) const;
    HRESULT GetColor(CVoid * pObject, CStr * pcstr, BOOL fReturnAsHex=FALSE, BOOL *pfValuePresent = NULL) const;
    HRESULT SetColor(CVoid * pObject, TCHAR * pch, WORD wFlags) const;
    HRESULT GetColor(CVoid * pObject, DWORD * pdwValue) const;
    HRESULT SetColor(CVoid * pObject, DWORD dwValue, WORD wFlags) const;
    HRESULT GetColorProperty(VARIANT * pbstr, CBase * pObject, CVoid * pSubObject) const;
    HRESULT SetColorProperty(VARIANT bstr, CBase * pObject, CVoid * pSubObject, WORD wFlags = 0)  const;

    HRESULT GetString(CVoid * pObject, CStr * pcstr, BOOL *pfValuePresent = NULL) const;
    HRESULT SetString(CVoid * pObject, TCHAR * pch, WORD wFlags) const;
    HRESULT GetStringProperty(BSTR *pbstr, CBase *pObject, CVoid *pSubObject) const;
    HRESULT SetStringProperty(BSTR bstr, CBase *pObject, CVoid *pSubObject, WORD wFlags = 0) const;

    HRESULT GetUrl(CVoid * pObject, CStr * pcstr) const;
    HRESULT SetUrl(CVoid * pObject, TCHAR * pch, WORD wFlags) const;
    HRESULT GetUrlProperty(BSTR *pbstr, CBase *pObject, CVoid *pSubObject) const;
    HRESULT SetUrlProperty(BSTR bstr, CBase *pObject, CVoid *pSubObject, WORD wFlags = 0) const;

    HRESULT SetCodeProperty(VARIANT *pvarCode, CBase * pObject, CVoid *) const;
    HRESULT GetCodeProperty (VARIANT *pvarCode, CBase * pObject, CVoid * pSubObject = NULL) const;

    HRESULT GetStyleComponentProperty(BSTR *pbstr, CBase *pObject, CVoid *pSubObject) const;
    HRESULT SetStyleComponentProperty(BSTR bstr,   CBase *pObject, CVoid *pSubObject, WORD wFlags = 0) const;
    HRESULT GetStyleComponentBooleanProperty( VARIANT_BOOL * p, CBase *pObject, CVoid *pSubObject) const;
    HRESULT SetStyleComponentBooleanProperty( VARIANT_BOOL v,   CBase *pObject, CVoid *pSubObject) const;
};



struct NUMPROPPARAMS
{
    BASICPROPPARAMS bpp;
    BYTE            vt;
    BYTE            cbMember;   // Only used for member access
    LONG            lMin;
    LONG_PTR        lMax;

    const PROPERTYDESC *GetPropertyDesc ( void ) const;

    HRESULT GetNumber(CBase * pObject, CVoid * pSubObject, long * plNum, BOOL* pfValuePresent = NULL) const;
    HRESULT SetNumber(CBase * pObject, CVoid * pSubObject, long plNum, WORD wFlags) const;

    HRESULT ValidateNumberProperty(long *lArg, CBase * pObject ) const;
    HRESULT GetNumberProperty(void * pv, CBase * pObject, CVoid * pSubObject) const;
    HRESULT SetNumberProperty(long lValueNew, CBase * pObject, CVoid * pSubObject, BOOL fValidate = TRUE, WORD wFlags = 0 ) const;
    HRESULT GetEnumStringProperty(BSTR *pbstr, CBase * pObject, CVoid * pSubObject) const;
    HRESULT SetEnumStringProperty(BSTR bstr,   CBase *pObject, CVoid *pSubObject, WORD wFlags = 0) const;

};

//+---------------------------------------------------------------------------
//
//  Class:      CEnumConnections (cenumc)
//
//  Purpose:    Enumerates connections per IEnumConnections.
//
//----------------------------------------------------------------------------

class CEnumConnections : public CBaseEnum
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CEnumConnections))

    //  IEnum methods
    STDMETHOD(Next) (ULONG celt, void * reelt, ULONG * pceltFetched);
    STDMETHOD(Clone) (CBaseEnum ** ppenum);

    //  CEnumGeneric methods
    static HRESULT Create(
            int ccd,
            CONNECTDATA * pcd,
            CEnumConnections ** ppenum);

protected:
    CEnumConnections();
    CEnumConnections(const CEnumConnections & enumc);
    ~CEnumConnections();

    CEnumConnections& operator=(const CEnumConnections & enumc); // don't define
};


struct InlineEvts {
public:
    InlineEvts() { cScriptlets = 0; }

    HRESULT Connect(CDoc *pDoc, CElement *pElement);

    ULONG   aOffsetScriptlets[DISPID_EVPROPS_COUNT];
    ULONG   aLineScriptlets  [DISPID_EVPROPS_COUNT];
    DISPID  adispidScriptlets[DISPID_EVPROPS_COUNT];

    int     cScriptlets;
};


typedef ULONG   AAINDEX;
const AAINDEX  AA_IDX_UNKNOWN = (AAINDEX)~0UL;


//+---------------------------------------------------------------------------
//
// CAttrValue.
//
//----------------------------------------------------------------------------

class CAttrArrayHeader
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAttrArrayHeader))

    // Last DISPID handed out by GetIDsOfNames or GetDispID used by 
    // custom invoke in CBase::Invoke to eliminate search to find PROPDESC.
    DISPID              _cachedDispidGIN;
    const VTABLEDESC   *_pCachedVTableDesc;
    InlineEvts         *_pEventsToHook;
};

class CAttrValue
{
private:
public:
    //
    //  31              24  23              16  15             8  7           0
    //  -----------------------------------------------------------------------
    //  |                 |                   |                 |             |
    //  |     VT_TYPE     |              AAExtraBits            |    AATYPE   |
    //  |                 |                   |                 |             |
    //  -----------------------------------------------------------------------
    //
    struct AttrFlags {
        BYTE    _aaType;
        WORD    _aaExtraBits;
        BYTE    _aaVTType;
    };

    enum AATYPE { AA_Attribute,
                  AA_UnknownAttr, 
                  AA_Expando,
                  AA_Internal, 
                  AA_AttrArray,   
                  AA_StyleAttribute, // Used to simplify get/set
                  AA_Expression,
                  AA_AttachEvent,
                  AA_Undefined = ~0UL };

    enum AAExtraBits {
        AA_Extra_Empty        = 0x0000,   // No bits set in extrabits area

        AA_Extra_Style        = 0x0001,   // The bit used to indicate AA_StyleAttribute
        AA_Extra_Important    = 0x0002,   // Stylesheets: "! important"
        AA_Extra_Implied      = 0x0004,   // Stylesheets: implied property, e.g. backgroundColor: transparent in "background: none"
        AA_Extra_Propdesc     = 0x0008,   // Does the union u1 contain a propdesc * or a dispid?

        AA_Extra_TridentEvent = 0x0010,   // Event sink should use ITridentEventSink.
        AA_Extra_DefaultValue = 0x0020,   // This AV stores a "tag not asssigned" default value
        AA_Extra_OldEventStyle= 0x0040,   // Event sink supports old style w/o eventObj argument.
        AA_Extra_Expression   = 0x0080,   // This value is the result of an expression
    };

    // other AV types that we use. These are stored in the higher byte of _wFlags
    // in CAttrValue just like the normal VARTYPEs. Their values fall within the range
    // of normal VARIANT types, but are currently not pre-defined.
    enum { VT_ATTRARRAY = 0x80,
           VT_AAHEADER  = 0x81 };

    inline BOOL IsPropdesc() const { return _wFlags._aaExtraBits & AA_Extra_Propdesc; }
    inline BOOL IsStyleAttribute ( void ) const { return _wFlags._aaExtraBits & AA_Extra_Style; } 
    inline BOOL IsImportant( void ) const { return _wFlags._aaExtraBits & AA_Extra_Important; } 
    inline BOOL IsImplied( void ) const { return _wFlags._aaExtraBits & AA_Extra_Implied; } 
    inline BOOL IsTridentSink( void ) const { return _wFlags._aaExtraBits & AA_Extra_TridentEvent; } 
    inline BOOL IsDefault( void ) const { return _wFlags._aaExtraBits & AA_Extra_DefaultValue; } 
    inline BOOL IsOldEventStyle( void ) const { return _wFlags._aaExtraBits & AA_Extra_OldEventStyle; } 
    inline BOOL IsExpression( void ) const { return _wFlags._aaExtraBits & AA_Extra_Expression; }

    inline void SetImportant( BOOL fImportant ) {
        if ( fImportant )
            _wFlags._aaExtraBits |= AA_Extra_Important;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_Important;
    }
    inline void SetImplied( BOOL fImplied ) {
        if ( fImplied )
            _wFlags._aaExtraBits |= AA_Extra_Implied;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_Implied;
    }
    inline void SetTridentSink( BOOL fTridentSink ) {
        if ( fTridentSink )
            _wFlags._aaExtraBits |= AA_Extra_TridentEvent;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_TridentEvent;
    }
    inline void SetDefault( BOOL fDefault ) {
        if ( fDefault )
            _wFlags._aaExtraBits |= AA_Extra_DefaultValue;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_DefaultValue;
    }
    inline void SetOldEventStyle( BOOL fOldEventStyle ) {
        if ( fOldEventStyle )
            _wFlags._aaExtraBits |= AA_Extra_OldEventStyle;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_OldEventStyle;
    }
    inline void SetExpression(BOOL fExpression) {
        if (fExpression)
            _wFlags._aaExtraBits |= AA_Extra_Expression;
        else
            _wFlags._aaExtraBits &= ~AA_Extra_Expression;
    }

    // Internal wrapper for AAType - use when you don't care about the style bit
    AATYPE AAType ( void ) const
    {
        return (AATYPE)(_wFlags._aaType);
    }
    // External wrapper to get AAType - call when need to crack the style bit
    AATYPE  GetAAType ( void ) const
    { 
        AATYPE aa = AAType();
        if ( aa == AA_Attribute && IsStyleAttribute() )
        {
            aa = AA_StyleAttribute;
        }
        return aa;
    }
    void SetAAType(AATYPE aaType);
    DISPID  GetDISPID ( void ) const;
    void    SetDISPID ( DISPID dispID )
        { u1._dispid = dispID; _wFlags._aaExtraBits &= ~AA_Extra_Propdesc; }

    const PROPERTYDESC *GetPropDesc() const;
    void SetPropDesc(const PROPERTYDESC *pPropdesc)
        { u1._pPropertyDesc = pPropdesc; _wFlags._aaExtraBits |= AA_Extra_Propdesc; }

    VARTYPE GetAVType() const
    {
        return (VARTYPE)(_wFlags._aaVTType);
    }
    void SetAVType(VARTYPE avType)
    {
        _wFlags._aaVTType = avType;
    }

    // NOTE (SRamani/TerryLu): If you need to use more AAtypes, or extra bits,
    // I've increased _wFlags to a DWORD (the extra WORD is used on win32 due
    // to padding) if you need more then the 16 extra bits do the following: 
    //
    //      1) Compress the AVType which is the higher byte of _wFlags into the
    //         top nibble and use some sort of a look up table to map the
    //         VARTYPEs to the AVTYPEs, thus gaining 4 more bits. I chose not
    //         to do this now due to perf reasons.

    AttrFlags   _wFlags;

    union
    {
        const PROPERTYDESC *_pPropertyDesc;
        struct {
            DISPID _dispid;
        #ifdef _WIN64
            DWORD _dwCookie;        // Used for Advise/Unadvise on Win64 platforms
        #endif
        };
    } u1;
    union
    {
        IUnknown *_pUnk;
        IDispatch *_pDisp;
        LONG _lVal;
        BOOL _boolVal;
        SHORT _iVal;
        FLOAT _fltVal;
        LPTSTR _lpstrVal;
        BSTR _bstrVal;
        VARIANT *_pvarVal;
        double *_pdblVal;
        CAttrArray *_pAA;
        CAttrArrayHeader *_pAAHeader;
        void *_pvVal;
    } u2;

    CAttrValue() {}

    int CompareWith (DISPID dispID, AATYPE aaType);

    HRESULT Copy(const CAttrValue *pAV);
    void    CopyEmpty(const CAttrValue *pAV);
    BOOL    Compare (const CAttrValue *pAV) const;
    void    Free();
    HRESULT InitVariant ( const VARIANT *pvarNew, BOOL fCloning = FALSE );
    HRESULT GetIntoVariant (VARIANT * pvar) const;
    HRESULT GetIntoString ( BSTR *pbStr, LPCTSTR *ppStr ) const;
    // copies the attr value into a variant, w/o copying strings or addref'ing objects
    void GetAsVariantNC (VARIANT *pvar) const;

    // Sets the value of the AV to the given variant by copying it into a newly allocated
    // VARIANT referenced as a ptr
    HRESULT SetVariant (const VARIANT *pvar);
    inline VARIANT *GetVariant() const { Assert(GetAVType() == VT_VARIANT); return u2._pvarVal; }
    HRESULT SetDouble(double dblVal);
    inline double GetDouble() const { Assert(u2._pdblVal); Assert(GetAVType() == VT_R8); return *(u2._pdblVal); }
    inline double *GetDoublePtr() const { Assert(u2._pdblVal); Assert(GetAVType() == VT_R8); return u2._pdblVal; }
    
    // inline put helpers that set the value and type of the AV
    // use these when changing the value as well as the type of an AV
    inline void SetLong(LONG lVal, VARTYPE avType) { u2._lVal = lVal; SetAVType(avType); }
    inline void SetShort(SHORT iVal, VARTYPE avType) { u2._iVal = iVal; SetAVType(avType); }
    inline void SetBSTR(BSTR bstrVal) { u2._bstrVal = bstrVal; SetAVType(VT_BSTR); }
    inline void SetLPWSTR(LPWSTR lpstrVal) { u2._lpstrVal = lpstrVal; SetAVType(VT_LPWSTR); }
    inline void SetPointer(void * pvVal, VARTYPE avType) { u2._pvVal = pvVal; SetAVType(avType); }
    inline void SetAA(CAttrArray *pAA) { u2._pAA = pAA; SetAVType(VT_ATTRARRAY); }
    inline void SetAAHeader(CAttrArrayHeader *pAAHeader) { u2._pAAHeader = pAAHeader; SetAVType(VT_AAHEADER); }

#ifdef _WIN64
    inline void SetCookie(DWORD dwCookie) { u1._dwCookie = dwCookie; }
#endif

    // inline put helpers that Assert the type of the AV and then set its value
    // use these when changing the value of an AV of the same type
    inline void PutLong(LONG lVal) { Assert(GetAVType() == VT_I4); u2._lVal = lVal; }

    // inline get helpers that Assert the type of the AV and then return its value. 
    // use these when getting the value of an AV whose type is known
    inline LONG GetLong() const { Assert(GetAVType() == VT_I4); return u2._lVal; }
    inline SHORT GetShort() const { Assert(GetAVType() == VT_I2); return u2._iVal; }
    inline FLOAT GetFloat() const { Assert(GetAVType() == VT_R4); return u2._fltVal; }
    inline BSTR GetBSTR() const { Assert(GetAVType() == VT_BSTR); return u2._bstrVal; }
    inline LPTSTR GetLPWSTR() const { Assert(GetAVType() == VT_LPWSTR); return u2._lpstrVal; }
    inline BOOL GetBool() const { Assert(GetAVType() == VT_BOOL); return u2._boolVal; }
    inline void *GetPointer() const { Assert(GetAVType() == VT_PTR); return u2._pvVal; }
    inline IUnknown *GetUnknown() const { Assert(GetAVType() == VT_UNKNOWN); return u2._pUnk; }
    inline IDispatch *GetDispatch() const { Assert(GetAVType() == VT_DISPATCH); return u2._pDisp; }
    inline CAttrArray *GetAA() const { Assert(GetAVType() == VT_ATTRARRAY); return u2._pAA; }
    inline CAttrArrayHeader *GetAAHeader() const { Assert(GetAVType() == VT_AAHEADER); return u2._pAAHeader; }

#ifdef _WIN64
    inline DWORD GetCookie() { return(u1._dwCookie); }
#endif

    // inline get helpers that get the value of the common VARIANT types, w/o asserting
    // use these when getting the value of an AV whose type is not known
    inline LONG GetlVal() const { return u2._lVal; }
    inline SHORT GetiVal() const { return u2._iVal; }
    inline IUnknown *GetpUnkVal() const { return u2._pUnk; }
    inline LPTSTR GetString() const { return u2._lpstrVal; }
    inline void * GetPointerVal() const { return u2._pvVal; }
    inline CAttrArray **GetppAA() const { return (CAttrArray **)(&(u2._pAA)); }

    // inline helpers that access the AA header members
    inline void SetCachedDispid(DISPID cachedDispidGIN) { GetAAHeader()->_cachedDispidGIN = cachedDispidGIN; }
    inline void SetCachedVTblDesc(const VTABLEDESC *pCachedVTableDesc) { GetAAHeader()->_pCachedVTableDesc = pCachedVTableDesc; }
    inline DISPID GetCachedDispid() const { return GetAAHeader()->_cachedDispidGIN; }
    inline const VTABLEDESC *GetCachedVTblDesc() const { return GetAAHeader()->_pCachedVTableDesc; }
    inline void SetEventsToHook(InlineEvts *pEventsToHook) { GetAAHeader()->_pEventsToHook = pEventsToHook; }
    inline InlineEvts * GetEventsToHook() const { return GetAAHeader()->_pEventsToHook; }
};


//+---------------------------------------------------------------
//
//  Class:      CDispParams
//
//  Synopsis:   Helper class to manipulate the DISPPARAMS structure
//              used by Invoke/InvokeEx.
//
//----------------------------------------------------------------

MtExtern(CDispParams)
MtExtern(CDispParams_rgvarg)
MtExtern(CDispParams_rgdispidNamedArgs)

class CDispParams : public DISPPARAMS
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispParams))
    CDispParams (UINT cTotalArgs = 0, UINT cTotalNamedArgs = 0)
        { rgvarg = NULL; rgdispidNamedArgs = NULL; cArgs = cTotalArgs; cNamedArgs = cTotalNamedArgs; }
    ~CDispParams ()
        { delete [] rgvarg; delete [] rgdispidNamedArgs; }

    void Initialize (UINT cTotalArgs, UINT cTotalNamedArgs)
        { Assert(!rgvarg && !rgdispidNamedArgs); cArgs = cTotalArgs; cNamedArgs = cTotalNamedArgs; }

    HRESULT Create (DISPPARAMS *pOrgDispParams);

    HRESULT MoveArgsToDispParams (DISPPARAMS *pOutDispParams, UINT cNumArgs, BOOL fFromEnd = TRUE);

    void ReleaseVariants ();
};


//+---------------------------------------------------------------------------
//
// CAttrArray.
//
//----------------------------------------------------------------------------
class CAttrArray : public CDataAry<CAttrValue>
{
    friend CBase;

private:
    // Maintain a simple checksum so we can quickly compare to attrarrays
    // Checksum is computed by adding DISPID's of attr array members
    // NOTE: The low bit of the checksum indicates the presence of 
    // style ptr caches in the array.  This speeds up shutdown.
    DWORD               _dwChecksum;

    HRESULT Set(
        DISPID dispID,
        const PROPERTYDESC *pPropDesc,
        VARIANT *varNew,
        CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute,
        WORD wExtraBits = 0,            // Modifiers like "important" or "implied"
        BOOL fAllowMultiple = FALSE);

protected:
    void Destroy(int iIndex);

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CAttrArray))
     CAttrArray();
    ~CAttrArray()
      { Free(); }

    void Free(void);
    void FreeSpecial();
    void Clear(CAttrArray *pAAUndo);
    
    BOOL Compare ( CAttrArray *pAA, DISPID * pdispIDDifferent = NULL ) const;

    BOOL HasAnyAttribute(BOOL fCountExpandosToo = FALSE);

    static HRESULT Set (
        CAttrArray **ppAA, DISPID dispID,
        VARIANT *varNew,
        const PROPERTYDESC *pPropDesc = NULL,
        CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute,
        WORD wExtraBits = 0,            // Modifiers like "important" or "implied"
        BOOL fAllowMultiple = FALSE);

    HRESULT SetAt (AAINDEX aaIdx, VARIANT *pvarNew);

    CAttrValue* FindAt(AAINDEX aaIdx)
       { return ((aaIdx >= 0) && (aaIdx < (ULONG)Size())) ? ((CAttrValue*)*this) + aaIdx : NULL; }
    CAttrValue* Find(
        DISPID dispID, 
        CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, 
        AAINDEX *paaIdx = NULL,
        BOOL fAllowMultiple = FALSE);
    static BOOL FindSimple ( CAttrArray *pAA, const PROPERTYDESC *pPropertyDesc, DWORD *pdwValue ) ;
    static BOOL FindString ( CAttrArray *pAA, const PROPERTYDESC *pPropertyDesc, LPCTSTR *ppStr ) ;

    BOOL FindString ( DISPID dispID, LPCTSTR *ppStr, CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, const PROPERTYDESC **ppPropertyDesc = NULL  ) ;
    BOOL FindSimple ( DISPID dispID, DWORD *pdwValue, CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, const PROPERTYDESC **ppPropertyDesc = NULL  ) ;
    BOOL FindSimpleInt4AndDelete( DISPID dispID, DWORD *pdwValue, CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, const PROPERTYDESC **ppPropertyDesc = NULL );
    BOOL FindSimpleAndDelete( DISPID dispID, CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, const PROPERTYDESC **ppPropertyDesc = NULL );

    static HRESULT SetSimple(CAttrArray **ppAA, const PROPERTYDESC *pPropertyDesc, DWORD dwSimple, WORD wFlags = 0);
    static HRESULT SetString(CAttrArray **ppAA, const PROPERTYDESC *pPropertyDesc, LPCTSTR pch, BOOL fIsUnknown = FALSE, WORD wFlags = 0 );

    AAINDEX FindAAIndex(
        DISPID dispID, 
        CAttrValue::AATYPE aaType, 
        AAINDEX aaLastOne = AA_IDX_UNKNOWN,
        BOOL fAllowMultiple = FALSE);

    HRESULT GetSimpleAt(AAINDEX aaIdx, DWORD *pdwValue);
    static HRESULT AddSimple( CAttrArray **ppAA, DISPID dispID, DWORD dwSimple,
        CAttrValue::AATYPE aaType )
    {
        VARIANT varNew;

        varNew.vt = VT_I4;
        varNew.lVal = (long)dwSimple;

        RRETURN(CAttrArray::Set(ppAA, dispID, &varNew, NULL, aaType ));
    }

    HRESULT Clone(CAttrArray **ppAA) const;
    HRESULT Merge(CAttrArray **ppAA, CBase *pTarget, CAttrArray *pAAUndo, BOOL fFromUndo = FALSE, BOOL fCopyID = FALSE) const;

    // Copy expandos from given attribute array to current one
    HRESULT CopyExpandos(CAttrArray *pAA);

    // Routines to get and set propDesc and dispid from last GIN or GINEx
    const VTABLEDESC * FindInGINCache(DISPID dispid)
    { return (dispid == GetCachedDispidGIN()) ? GetCachedVTableDesc() : 0; }

    void SetGINCache(DISPID dispid, const VTABLEDESC *pVTblDesc, BOOL fCreate = TRUE);

    DWORD   GetChecksum() const { return _dwChecksum >> 1; }
    // This method is only for the data cache and casts the checksum down to a word
    WORD ComputeCrc() const { return (WORD)GetChecksum(); }

    CAttrValue *EnsureHeader(BOOL fCreate = TRUE);
    HRESULT SetHeader();
    DISPID GetCachedDispidGIN();
    const VTABLEDESC *GetCachedVTableDesc();

    BOOL IsStylePtrCachePossible()   { return _dwChecksum & 1; }
    void SetStylePtrCachePossible()  { _dwChecksum |= 1; }
};

// Flag set on Invoke if "new" keyword supplied
#define DISPATCH_CONSTRUCT  0x4000


//+---------------------------------------------------------------------------
//
//  Class:      CDummyUndoManager (DUM)
//
//  Purpose:    Dummy undo manager that CServer initially points to until it
//              gets a real one. If no real one is provided or needed then
//              no undo behavior is available.
//
//----------------------------------------------------------------------------

#ifndef NO_EDIT
class CDummyUndoManager : public IOleUndoManager
{
public:

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv)  { return E_NOINTERFACE; }
    STDMETHOD_(ULONG, AddRef)(void)                       { return 0; }
    STDMETHOD_(ULONG, Release)(void)                      { return 0; }

    //
    // IOleUndoManager methods
    //
    STDMETHOD(Open)(IOleParentUndoUnit *pPUU)         { return S_OK; }
    STDMETHOD(Close)(IOleParentUndoUnit *pPUU, BOOL fCommit) { return S_OK; }
    STDMETHOD(Add)(IOleUndoUnit * pUU)                  { return S_OK; }
    STDMETHOD(GetOpenParentState)(DWORD * pdwState)
                 { *pdwState = UAS_BLOCKED; return S_OK; }
    STDMETHOD(DiscardFrom)(IOleUndoUnit * pUU)          { return S_OK; }
    STDMETHOD(UndoTo)(IOleUndoUnit * pUU)               { return S_OK; }
    STDMETHOD(RedoTo)(IOleUndoUnit * pUU)               { return S_OK; }
    STDMETHOD(EnumUndoable)(IEnumOleUndoUnits **ppEnum) { return E_NOTIMPL; }
    STDMETHOD(EnumRedoable)(IEnumOleUndoUnits **ppEnum) { return E_NOTIMPL; }
    STDMETHOD(GetLastUndoDescription)(BSTR *pstr)         { return E_FAIL; }
    STDMETHOD(GetLastRedoDescription)(BSTR *pstr)         { return E_FAIL; }
    STDMETHOD(Enable)(BOOL fEnable)                       { return S_OK; }
};

extern CDummyUndoManager g_DummyUndoMgr;
#endif // NO_EDIT

//+---------------------------------------------------------------------------
//
//  Class:      CBaseCF
//
//  Purpose:    Standard implementation of a class factory object for
//              any object derived from CBase.  It's declared as a static
//              variable. The implementation of Release does not call
//              delete.
//
//              To use this class, declare a variable of type CBaseCF
//              and initialize it with an instance creation function.
//              The instance creation function is of type FNCREATE defined
//              below.  The creation is in two steps.  First the creation
//              function is called and then the virtual function Init (See
//              CBase below).
//
//+---------------------------------------------------------------------------

class CBase;
class CBaseCF : public CClassFactory
{
public:
                                         // pUnkOuter for aggregation
    //
    // The parens are removed because BoundsChecker blows up on them. (lylec)
    //  
    typedef CBase * FNCREATE(IUnknown *pUnkOuter);

    typedef HRESULT FNINITCLASS(void);

    CBaseCF(FNCREATE *pfnCreate, FNINITCLASS *pfnInitClass = NULL)
            { _pfnCreate = pfnCreate; _pfnInitClass = pfnInitClass; }

    // IClassFactory methods
    STDMETHOD(CreateInstance)(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj);

protected:
    FNCREATE *_pfnCreate;
    FNINITCLASS * _pfnInitClass;
};

class CBaseLockCF : public CBaseCF
{
public:
    CBaseLockCF(FNCREATE *pfnCreate, FNINITCLASS *pfnInitClass = NULL) : CBaseCF(pfnCreate, pfnInitClass) { }
    STDMETHOD(LockServer)(BOOL fLock);
};



//+---------------------------------------------------------------------------
//
//  Flags values for CBase::CLASSDESC::_dwFlags
//
//----------------------------------------------------------------------------

enum BASEDESC_FLAG
{
    BASEDESC_NOAGG          = 1,
    BASEDESC_DUALINTERFACE  = 2,
    BASEDESC_LAST           = BASEDESC_DUALINTERFACE,
    BASEDESC_MAX            = LONG_MAX    // needed to force enum to be dword
};

//+---------------------------------------------------------------------------
//
//  Flag values for CBase::OnPropertyChange
//
//----------------------------------------------------------------------------

enum BASECHNG_FLAG
{
    BASECHNG_LAST = 1,
    BASECHNG_MAX  = LONG_MAX    // needed to force enum to be dword
};

//+-------------------------------------------------------------------------
//
//  PrivateQueryInterface delegating helpers - required only for Win16
//  because of the pointer displacement while casting ptrs form base class
//  to derived class.
//
//--------------------------------------------------------------------------
#ifdef WIN16
#define DECLARE_PRIVATE_QI_FUNCS(cls)                                   \
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, ())    \
    { return cls::PrivateAddRef(); }                                    \
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, ())  \
    { return cls::PrivateRelease(); }                                    \
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
#else
#define DECLARE_PRIVATE_QI_FUNCS(cls)                                   \
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
#endif

//+-------------------------------------------------------------------------
//
//  IDispatch delegating helpers
//
//--------------------------------------------------------------------------

#define DECLARE_DERIVED_DISPATCH(cls)                                       \
    DECLARE_TEAROFF_METHOD(GetTypeInfo, gettypeinfo,                    \
            (UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo))                \
        { return cls::GetTypeInfo(itinfo, lcid, pptinfo); }                 \
    DECLARE_TEAROFF_METHOD(GetTypeInfoCount, gettypeinfocount,          \
            (UINT * pctinfo))                                               \
        { return cls::GetTypeInfoCount(pctinfo); }                          \
    DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames,                \
                            (REFIID riid,                                   \
                             LPOLESTR * rgszNames,                          \
                             UINT cNames,                                   \
                             LCID lcid,                                     \
                             DISPID * rgdispid))                            \
        { return cls::GetIDsOfNames(                                        \
                            riid,                                           \
                            rgszNames,                                      \
                            cNames,                                         \
                            lcid,                                           \
                            rgdispid); }                                    \
    DECLARE_TEAROFF_METHOD(Invoke, invoke,                              \
                      (DISPID dispidMember,                                 \
                      REFIID riid,                                          \
                      LCID lcid,                                            \
                      WORD wFlags,                                          \
                      DISPPARAMS * pdispparams,                             \
                      VARIANT * pvarResult,                                 \
                      EXCEPINFO * pexcepinfo,                               \
                      UINT * puArgErr))                                     \
        { return cls::Invoke(                                               \
                        dispidMember,                                       \
                        riid,                                               \
                        lcid,                                               \
                        wFlags,                                             \
                        pdispparams,                                        \
                        pvarResult,                                         \
                        pexcepinfo,                                         \
                        puArgErr); }                                        


//+-------------------------------------------------------------------------
//
//  IDispatchEx delegating helpers
//
//--------------------------------------------------------------------------

#define DECLARE_DERIVED_DISPATCHEx2(cls)                                    \
    DECLARE_DERIVED_DISPATCH(cls)                                           \
    STDMETHOD(GetDispID)(BSTR bstrName,                                     \
                               DWORD grfdex,                                \
                               DISPID *pid)                                 \
        { return cls::GetDispID(                                            \
                        bstrName,                                           \
                        grfdex,                                             \
                        pid); }                                             \
      STDMETHOD(InvokeEx)(DISPID dispidMember,                              \
                      LCID lcid,                                            \
                      WORD wFlags,                                          \
                      DISPPARAMS * pdispparams,                             \
                      VARIANT * pvarResult,                                 \
                      EXCEPINFO * pexcepinfo,                               \
                      IServiceProvider *pSrvProvider)                       \
        { return cls::InvokeEx(                                             \
                        dispidMember,                                       \
                        lcid,                                               \
                        wFlags,                                             \
                        pdispparams,                                        \
                        pvarResult,                                         \
                        pexcepinfo,                                         \
                        pSrvProvider); }                                    \
    STDMETHOD(DeleteMemberByName)(BSTR bstr,DWORD grfdex)                   \
        { return cls::DeleteMemberByName(bstr,grfdex); }                    \
    STDMETHOD(DeleteMemberByDispID)(DISPID id)                              \
        { return cls::DeleteMemberByDispID(id); }                           \
    STDMETHOD(GetMemberProperties)(DISPID id,                               \
                                   DWORD grfdexFetch,                       \
                                   DWORD *pgrfdex)                          \
        { return cls::GetMemberProperties(                                  \
                        id,                                                 \
                        grfdexFetch,                                        \
                        pgrfdex); }                                         \
    STDMETHOD(GetNextDispID)(DWORD grfdex,                                  \
                             DISPID id,                                     \
                             DISPID *pid)                                   \
        { return cls::GetNextDispID(                                        \
                        grfdex,                                             \
                        id,                                                 \
                        pid); }                                             \
    STDMETHOD(GetMemberName)(DISPID id,                                     \
                             BSTR *pbstrName)                               \
        { return cls::GetMemberName(                                        \
                        id,                                                 \
                        pbstrName); }                                       \
    STDMETHOD(GetNameSpaceParent)(IUnknown **ppunk)                         \
        { return cls::GetNameSpaceParent(ppunk); }


//+-------------------------------------------------------------------------
//
//  Helper for calculating offset of primary interface
//
//--------------------------------------------------------------------------

#define OFFSETOFITF(cls, itf)   ((DWORD)(((DWORD_PTR)(itf *)(cls *)16 - 16)))

//+---------------------------------------------------------------------------
//
//  Class:      CBase
//
//  Purpose:    A generally useful base class.
//
//----------------------------------------------------------------------------

class NOVTABLE CBase : public CVoid, public IPrivateUnknown
{
    DECLARE_CLASS_TYPES(CBase, CVoid)
    
private:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

public:

    // Construct/Destruct
                    CBase();
    virtual         ~CBase();
    virtual HRESULT Init();
    virtual void    Passivate();

    // IPrivateUnknown methods
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateAddRef, privateaddref, ());
    DECLARE_TEAROFF_METHOD_(ULONG, PrivateRelease, privaterelease, ());
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
#ifdef WIN16
    // these need to real virtuals.
    //static ULONG STDMETHODCALLTYPE privateaddref(CBase *pObj)
    //{ return pObj->PrivateAddRef(); }
    //static ULONG STDMETHODCALLTYPE privaterelease(CBase *pObj)
    //{ return pObj->PrivateRelease(); }
    static HRESULT STDMETHODCALLTYPE privatequeryinterface(CBase *pObj, REFIID refiid, void **pUnk)
    { return pObj->PrivateQueryInterface(refiid, pUnk); }
    static NV_STDMETHOD( gettypeinfo )(CBase *pObj, UINT itinfo, ULONG lcid, ITypeInfo ** ppTI)
        {return DispatchGetTypeInfo(
                ( *(pObj->BaseDesc())->_apHdlDesc && *(pObj->BaseDesc())->_apHdlDesc->_piidOfMondoDispInterface) ?
                    *(pObj->BaseDesc())->_apHdlDesc->_piidOfMondoDispInterface) :     
                    *(pObj->BaseDesc())->_piidDispinterface,
               itinfo,
                lcid,
                ppTI);}
    static NV_STDMETHOD(gettypeinfocount)(CBase *pObj, UINT *pcTinfo)
        {return DispatchGetTypeInfoCount(pcTinfo);}
#endif // ndef WIN16

    //  IDispatch methods
    NV_STDMETHOD( GetTypeInfo )(UINT itinfo, ULONG lcid, ITypeInfo ** ppTI)
        {
        return DispatchGetTypeInfo(
                // Use the Mondo tearoff dispatch interface IID if we have one
                BaseDesc()->_apHdlDesc && BaseDesc()->_apHdlDesc->_piidOfMondoDispInterface ?
                    *(BaseDesc()->_apHdlDesc->_piidOfMondoDispInterface) :     
                    *(BaseDesc()->_piidDispinterface),
                itinfo,
                lcid,
                ppTI);}

    NV_STDMETHOD(GetTypeInfoCount)(UINT *pcTinfo)
        {return DispatchGetTypeInfoCount(pcTinfo);}

    NV_DECLARE_TEAROFF_METHOD(GetIDsOfNames, getidsofnames, (
            REFIID riid,
            LPOLESTR * rgszNames,
            UINT cNames,
            LCID lcid,
            DISPID * rgdispid));

    NV_DECLARE_TEAROFF_METHOD(Invoke, invoke, (
            DISPID dispidMember,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS * pdispparams,
            VARIANT * pvarResult,
            EXCEPINFO * pexcepinfo,
            UINT * puArgErr));

    // The following 5 methods are IDispatchEx:

    // Get dispID for names, with options
    NV_DECLARE_TEAROFF_METHOD(GetDispID, getdispid, (
            BSTR bstrName,
            DWORD grfdex,
            DISPID *pid));

    NV_DECLARE_TEAROFF_METHOD(InvokeEx, invokeex, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider))
    {
        return ContextInvokeEx(id,
                               lcid,
                               wFlags,
                               pdp,
                               pvarRes,
                               pei,
                               pSrvProvider,
                               (IUnknown*)this);
    }

    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex));
    NV_DECLARE_TEAROFF_METHOD(DeleteMemberByDispID, deletememberbydispid, (DISPID id));

    NV_DECLARE_TEAROFF_METHOD(GetMemberProperties, getmemberproperties, (
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex));

    // Enumerate dispIDs and their associated "names".
    // Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
    // error code if the call fails.
    NV_DECLARE_TEAROFF_METHOD(GetNextDispID, getnextdispid, (
                DWORD grfdex,
                DISPID id,
                DISPID *prgid));

    NV_DECLARE_TEAROFF_METHOD(GetMemberName, getmembername, (DISPID id,
                                            BSTR *pbstrName));

    NV_DECLARE_TEAROFF_METHOD(GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk));

    NV_DECLARE_TEAROFF_METHOD(ContextInvokeEx, contextinvokeex, (
            DISPID id,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pdp,
            VARIANT *pvarRes,
            EXCEPINFO *pei,
            IServiceProvider *pSrvProvider,
            IUnknown *pUnkContext));

    // Get dispID for name with proper scoping and name matching.
    NV_DECLARE_TEAROFF_METHOD(GetInternalDispID, getinternaldispid, (BSTR bstrName,
            DISPID *pid,
            DWORD grfDex,
            IDispatch *pDisp      = NULL,
            IDispatchEx  *pDispEx = NULL));

    // Get dispID for expando name.
#ifdef VSTUDIO7
    NV_DECLARE_TEAROFF_METHOD(GetExpandoDispID, getexpandodispid, (
            LPOLESTR pchName,
            DISPID *pid,
            DWORD grfdex,
            CAttrArray *pAA = NULL));
#else
    NV_DECLARE_TEAROFF_METHOD(GetExpandoDispID, getexpandodispid, (
            BSTR bstrName,
            DISPID *pid,
            DWORD grfdex,
            CAttrArray *pAA = NULL));
#endif

    // *** IObjectIdentity members ***
    NV_DECLARE_TEAROFF_METHOD(IsEqualObject, isequalobject, (IUnknown *ppunk));

    // Enumerate dispIDs and their associated "names".
    // Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
    // error code if the call fails.  This again assume correct scoping rules
    // as GetInternalDispID.
    HRESULT STDMETHODCALLTYPE GetInternalNextDispID(DWORD grfdex,
            DISPID id,
            DISPID *prgid,
            BSTR *prgbstr,
            IDispatch *pDisp = NULL);

    // Enumerate through properties of the pDisp typeinfo.
    HRESULT NextTypeInfoProperty(IDispatch *pDisp,
            DISPID id,
            BSTR *pbstrName,
            DISPID *pid);

    // Enumerate through expando properties.
    HRESULT GetNextDispIDExpando(DISPID currDispID,
            BSTR *pStrNextName,
            DISPID *pNextDispID);

    // Callbacks to implement special Get properties without typeinfo load.
    // BUGBUG (garybu) Move these to CServer::Invoke implementation.
    STDMETHOD(GetEnabled)(VARIANT_BOOL * pfEnabled);
    STDMETHOD(GetValid)(VARIANT_BOOL * pfEnabled);

    // IProvideMultipleClassInfo methods
    NV_DECLARE_TEAROFF_METHOD(GetClassInfo, getclassinfo, (ITypeInfo ** ppTI));
    NV_DECLARE_TEAROFF_METHOD(GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID));
    NV_DECLARE_TEAROFF_METHOD(GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti));
    NV_DECLARE_TEAROFF_METHOD(GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource));

    // IProvideMultipleClassInfo helpers
    HRESULT GetAggMultiTypeInfoCount(ULONG *pcti, IUnknown *pUnkAgg);
    HRESULT GetAggInfoOfIndex(
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource,
            IUnknown *pUnkAgg);

    // ISpecifyPropertyPages methods
    DECLARE_TEAROFF_METHOD(GetPages, getpages, (CAUUID * pPages));

    // ISpecifyPropertyPages helpers
    BOOL    HasPages();

    // IPerPropertyBrowsing methods
    NV_DECLARE_TEAROFF_METHOD(GetDisplayString, getdisplaystring, (DISPID dispID, BSTR *pBstr));
    NV_DECLARE_TEAROFF_METHOD(MapPropertyToPage, mappropertyTopage, (DISPID dispID, CLSID *pClsid));
    NV_DECLARE_TEAROFF_METHOD(GetPredefinedStrings, getpredefinedstrings, (DISPID dispID, CALPOLESTR  *pCaStringsOut, CADWORD *pCaCookiesOut));
    NV_DECLARE_TEAROFF_METHOD(GetPredefinedValue, getpredefinedvalue, (DISPID dispID, DWORD dwCookie, VARIANT *pVarOut));

    // IPersist methods
    NV_STDMETHOD(GetClassID)(LPCLSID lpClassID);

    // ISupportErrorInfo methods
    DECLARE_TEAROFF_METHOD(InterfaceSupportsErrorInfo, interfacesupportserrorinfo, (REFIID iid));

    // IOleCommandTarget methods

    STDMETHOD(QueryStatus)(
            GUID * pguidCmdGroup,
            ULONG cCmds,
            OLECMD rgCmds[],
            OLECMDTEXT * pcmdtext)
        {    return MSOCMDERR_E_NOTSUPPORTED; }
    STDMETHOD(Exec)(
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut)
        {    return MSOCMDERR_E_NOTSUPPORTED; }

    static int  IDMFromCmdID(
                const GUID *pguidCmdGroup,
                ULONG ulCmdID);

    static BOOL IsCmdGroupSupported(
                const GUID *pguidCmdGroup);

    // Fire an event and get the returned value
    HRESULT FireCancelableEvent(DISPID dispidMethod,
                                DISPID dispidProp,
                                IDispatch *pEventObject,
                                VARIANT_BOOL * pfRetVal,
                                BYTE * pbTypes,
                                ...);

    HRESULT queryCommandSupported(const BSTR cmdID,VARIANT_BOOL* pfRet);
    HRESULT queryCommandEnabled(const BSTR cmdID,VARIANT_BOOL* pfRet);
    HRESULT queryCommandState(const BSTR cmdID,VARIANT_BOOL* pfRet);
    HRESULT queryCommandIndeterm(const BSTR cmdID,VARIANT_BOOL* pfRet);
    HRESULT queryCommandText(const BSTR cmdID,BSTR* pcmdText);
    HRESULT queryCommandValue(const BSTR cmdID,VARIANT* pcmdValue);
    HRESULT execCommand(const BSTR cmdID,VARIANT_BOOL showUI,VARIANT value);
    HRESULT execCommandShowHelp(const BSTR cmdID);

    // Helpers for non-abstract property get\put
    //
    // NOTE: Any addtion of put/get must have a corresponding helper.
    //
    NV_DECLARE_TEAROFF_METHOD(put_StyleComponent, PUT_StyleComponent, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Url, PUT_Url, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_String, PUT_String, (BSTR v));
    NV_DECLARE_TEAROFF_METHOD(put_Short, PUT_Short, (short v));
    NV_DECLARE_TEAROFF_METHOD(put_Long, PUT_Long, (long v));
    NV_DECLARE_TEAROFF_METHOD(put_Bool, PUT_Bool, (VARIANT_BOOL v));
    NV_DECLARE_TEAROFF_METHOD(put_Variant, PUT_Variant, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(put_DataEvent, PUT_DataEvent, (VARIANT v));
    NV_DECLARE_TEAROFF_METHOD(get_Url, GET_Url, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_StyleComponent, GET_StyleComponent, (BSTR *p));
    NV_DECLARE_TEAROFF_METHOD(get_Property, GET_Property, (void *p));

    NV_DECLARE_TEAROFF_METHOD(put_StyleComponentHelper, PUT_StyleComponentHelper, (BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_UrlHelper, PUT_UrlHelper, (BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_StringHelper, PUT_StringHelper, (BSTR v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_ShortHelper, PUT_ShortHelper, (short v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_LongHelper, PUT_LongHelper, (long v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_BoolHelper, PUT_BoolHelper, (VARIANT_BOOL v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_VariantHelper, PUT_VariantHelper, (VARIANT v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(put_DataEventHelper, PUT_DataEventHelper, (VARIANT v, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(get_UrlHelper, GET_UrlHelper, (BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(get_StyleComponentHelper, GET_StyleComponentHelper, (BSTR *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));
    NV_DECLARE_TEAROFF_METHOD(get_PropertyHelper, GET_PropertyHelper, (void *p, const PROPERTYDESC *pPropDesc, CAttrArray ** ppAttr = NULL));

protected:
    HRESULT ExecSetGetProperty(
                VARIANTARG *    pvarargIn,
                VARIANTARG *    pvarargOut,
                UINT            uPropName,
                VARTYPE         vt);
    HRESULT ExecSetGetKnownProp(
                VARIANTARG *    pvarargIn, 
                VARIANTARG *    pvarargOut,
                DISPID          dispidProp, 
                VARTYPE         vt);
    HRESULT ExecSetGetHelper(
                VARIANTARG *    pvarargIn, 
                VARIANTARG *    pvarargOut,
                IDispatch  *    pDispatch, 
                DISPID          dispid,    
                VARTYPE         vt);

    HRESULT ExecSetPropertyCmd (UINT uPropName, DWORD value);
    HRESULT ExecToggleCmd      (UINT uPropName);

    HRESULT QueryStatusProperty(
                OLECMD *    pCmd,
                UINT        uPropName,
                VARTYPE     vt);

    HRESULT GetDispatchForProp(UINT uPropName, IDispatch ** ppDisp, DISPID * pdispid);

    // Translates command name into command ID
    static HRESULT CmdIDFromCmdName(BSTR cmdName, ULONG *pcmdValue);

    // Returns the expected VARIANT type of the command value (like VT_BSTR for font name)
    static VARTYPE GetExpectedCmdValueType(ULONG ulCmdID);

    // QueryCommandXXX helper function
    HRESULT QueryCommandHelper(BSTR cmdID, DWORD *cmdf, BSTR *pcmdTxt);

    struct CMDINFOSTRUCT
    {
        TCHAR   *cmdName;
        ULONG   cmdID;
    };

    HRESULT GetEnumDescFromDispID(DISPID dispID, const ENUMDESC **ppEnumDesc);

protected:

    HRESULT toString(BSTR *bstrString);

public:

    // Reference counting.
    ULONG GetRefs()       { return _ulAllRefs; }
    ULONG GetObjectRefs() { return _ulRefs; }

#if DBG==1
    ULONG SubAddRef();
#else
    ULONG SubAddRef() { CBaseCheckThread(); return ++_ulAllRefs; }
#endif
    ULONG SubRelease();

    IUnknown * PunkInner() { return (IUnknown *)(IPrivateUnknown *)this; }
    virtual IUnknown * PunkOuter() { return PunkInner(); }

    // virtual functions

    virtual BOOL DesignMode() { return FALSE; }

    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID, REFIID, void **ppv);

    // Connection Point Helpers.
    HRESULT DoAdvise(
        REFIID      riid,
        DISPID      dispidBase,
        IUnknown *  pUnkSinkAdvise,
        IUnknown ** ppUnkSinkOut,
        DWORD *     pdwCookie);
    HRESULT DoUnadvise(DWORD dwCookie, DISPID dispidBase);
    HRESULT FindAdviseIndex(DISPID dispidBase, DWORD dwCookie, IUnknown * pUnkCookie, AAINDEX * paaidx);

    HRESULT GetTheDocument(IHTMLDocument2 **ppDoc);

    HRESULT FindEventName(ITypeInfo *pTISrc, DISPID dispid, BSTR *pBSTR);

    HRESULT FireEventV(
        DISPID dispidEvent, 
        DISPID dispidProp, 
        IDispatch *pEventObject,
        VARIANT *pv, 
        BYTE * pbTypes, 
        va_list valParms);
    HRESULT FireEvent(
        DISPID dispidEvent, 
        DISPID dispidProp, 
        VARIANT *pvarResult, 
        DISPPARAMS *pdispparams,
        EXCEPINFO *pexcepinfo,
        UINT *puArgErr,
        ITypeInfo *pTIEventSrc = NULL,
        IDispatch *pEventObject = NULL);

    AAINDEX FindNextAttach(int idx, DISPID dispID);
    HRESULT FireAttachEventV(
        DISPID          dispidEvent,
        DISPID          dispidProp,
        IDispatch *     pEventObject,
        VARIANT *       pvRes,
        CBase *         pDocAccessObject,
        BYTE *          pbTypes,
        va_list         valParms);
    HRESULT FireAttachEvents(
        DISPID          dispidProp, 
        DISPPARAMS *    pdispparams = NULL,
        VARIANT *       pvarResult = NULL,
        CBase *         pDocAccessObject = NULL,
        EXCEPINFO *     pexcepinfo = NULL,
        UINT *          puArgErr = NULL,
        IDispatch *     pEventObject = NULL);

    HRESULT FirePropertyNotify(DISPID dispid, BOOL fOnChanged);
    HRESULT GetEventCode(DISPID dispidEvent, IDispatch ** ppDispCode); 
    HRESULT InvokeDispatchWithThis (
        IDispatch *     pDisp,
        VARIANT *       pExtraArg,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        IServiceProvider *pSrvProvider);
    HRESULT InvokeDispatchExtraParam (
        IDispatch *     pDisp,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        IServiceProvider *pSrvProvider,
        VARIANT          *pExtraParam);
    HRESULT InvokeAA(
        DISPID              dispidMember,
        CAttrValue::AATYPE  aaType,
        LCID                lcid,
        WORD                wFlags,
        DISPPARAMS *        pdispparams,
        VARIANT *           pvarResult,
        EXCEPINFO *         pexcepinfo,
        IServiceProvider *  pSrvProvider);
    HRESULT FireOnChanged(DISPID dispid)
               { return FirePropertyNotify(dispid, TRUE); }
    HRESULT FireRequestEdit(DISPID dispid)
               { return FirePropertyNotify(dispid, FALSE); }

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags)
                { return S_OK; }


    // Error Helpers

    virtual void PreSetErrorInfo()
                { }
    HRESULT SetErrorInfo(HRESULT hr);
    HRESULT SetErrorInfo(HRESULT hr, DISPID dispid, INVOKEKIND invkind, UINT ids, ...);
    HRESULT SetErrorInfoPSet(HRESULT hr, DISPID dispid);
    HRESULT SetErrorInfoPGet(HRESULT hr, DISPID dispid);
    HRESULT SetErrorInfoInvalidArg();
    HRESULT SetErrorInfoBadValue(DISPID dispid, UINT ids, ...);
    HRESULT SetErrorInfoPBadValue(DISPID dispid, UINT ids, ...);
    HRESULT CloseErrorInfo(HRESULT hr, DISPID id, INVOKEKIND invkind);
    HRESULT CloseErrorInfoPSet(HRESULT hr, DISPID dispid)
                { return CloseErrorInfo(hr, dispid, INVOKE_PROPERTYPUT); }
    HRESULT CloseErrorInfoPGet(HRESULT hr, DISPID dispid)
                { return CloseErrorInfo(hr, dispid, INVOKE_PROPERTYGET); }
    HRESULT CloseErrorInfoMCall(HRESULT hr, DISPID dispid)
                { return CloseErrorInfo(hr, dispid, INVOKE_FUNC); }
    virtual HRESULT CloseErrorInfo(HRESULT hr);

    // Property helpers

    //virtual void *  GetPropMemberPtr(const BASICPROPPARAMS * pbpp, const void * pvParams);
    virtual CVoid * GetSubObject(const BASICPROPPARAMS * ppp, const void * pvParams) { return this; }

    HRESULT SetCodeProperty (DISPID dispidCodeProp, IDispatch *  pDispCode, BOOL *pfAnyDeleted = NULL);

    HRESULT DefaultMembers();

    // Helper for removeAttribute (also called by recalc engine)
    BOOL removeAttributeDispid(DISPID dispid, const PROPERTYDESC * pPropDesc = NULL);     

#ifndef NO_EDIT
    // Undo helpers
    virtual IOleUndoManager * UndoManager(void) { return &g_DummyUndoMgr; }

    virtual BOOL QueryCreateUndo(BOOL fRequiresParent, BOOL fDirtyChange = TRUE);

    HRESULT CreatePropChangeUndo(DISPID             dispidProp,
                                 VARIANT *          pVar,
                                 CUndoPropChange ** ppUndo);

    CParentUndoUnit *            OpenParentUnit(
                                     CBase * pBase,
                                     UINT uiID,
                                     TCHAR * pchDescription = NULL);

    HRESULT                      CloseParentUnit(
                                     CParentUndoUnit *pCPUU,
                                     HRESULT hrCommit);

    HRESULT BlockNewUndoUnits(DWORD *pdwCookie);
    void    UnblockNewUndoUnits(DWORD dwCookie);

    HRESULT QueryStatusUndoRedo(BOOL fUndo, OLECMD * pcmd, OLECMDTEXT * pcmdtext);
    HRESULT EditUndo();
    HRESULT EditRedo();
#endif // NO_EDIT

    // Helper functions for IDispatchEx:
    HRESULT AddExpando (LPOLESTR pszExpandoName, DISPID *pdispID);
    HRESULT GetExpandoName ( DISPID expandoDISPID, LPCTSTR *lpPropName );
    HRESULT SetExpando(LPOLESTR pszExpandoName, VARIANT * pvarPropertyValue);

    HRESULT NextProperty (DISPID currDispID, BSTR *pStrNextName, DISPID *pNextDispID);

    BOOL IsExpandoDISPID (DISPID dispid, DISPID *pOLESiteExpando = NULL);

    inline const PROPERTYDESC *FindPropDescForName ( LPCTSTR szName, BOOL fCaseSensitive = FALSE, long *pidx = NULL )
    {
        if ( BaseDesc() && BaseDesc()->_apHdlDesc )
            return BaseDesc()->_apHdlDesc->FindPropDescForName ( szName, fCaseSensitive, pidx );
        return NULL;
    }

    const VTABLEDESC * FindVTableEntryForName (LPCTSTR szName, BOOL fCaseSensitive, WORD *pVTblOffset = NULL);

    HRESULT ExpandedRelativeUrlInVariant(VARIANT *pVariantURL);

    // Helper functions for custom Invoke:
    HRESULT FindPropDescFromDispID(DISPID dispidMember, PROPERTYDESC **ppDesc, WORD *pwEntry, WORD *pwClassType);

    //
    // Recalc helpers
    //

    // Call this when you want the scoped recalc engine.
    // Returns S_FALSE if the engine doesn't exist and fCreate is FALSE.
    HRESULT GetRecalcEngine(BOOL fCreate, IRecalcEngine **ppEngine);

    // Does the work for setExpression and the CSS parser
    HRESULT setExpressionHelper(DISPID dispid, BSTR strExpression, BSTR strLanguage);

    // REVIEW michaelw these need to go away
#define RECALC_PUT_HELPER(dispid)
#define RECALC_GET_HELPER(dispid)

    //+-----------------------------------------------------------------------
    //
    //  CBase::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        CLock(CBase *pBase);
        ~CLock();

    private:
        CBase *     _pBase;
    };
  
    typedef void (*LPFNGETSIZEL) (SIZEL *);

    // Class Descriptor
    struct CLASSDESC
    {
        const CLSID *           _pclsid;            // class's unique identifier
        WORD                    _idrBase;           // base resource identifier (see IDOFF_ )
#ifndef NO_PROPERTY_PAGE
        const CLSID **          _apclsidPages;      // property page CLSID's, NULL terminated.
#endif // NO_PROPERTY_PAGE
        const CONNECTION_POINT_INFO * _pcpi;        // connection pt info, NULL terminated
        DWORD                   _dwFlags;           // any combination of the above BASEDESC_FLAG
        const IID *             _piidDispinterface; // class's dispinterface IID
        HDLDESC               * _apHdlDesc;         // Description arrays (NULL terminated) in .HDL files

        BOOL TestFlag(BASEDESC_FLAG dw) const { return (_dwFlags & dw) != 0; }
    };

    virtual const CBase::CLASSDESC *GetClassDesc() const = 0;
    const CLASSDESC *BaseDesc() const { return GetClassDesc(); }

    const PROPERTYDESC * const *GetPropDescArray(void) 
    { 
        if ( BaseDesc() && BaseDesc()->_apHdlDesc ) 
            return BaseDesc()->_apHdlDesc->ppPropDescs;
        else
            return NULL;
    }
    UINT GetPropDescCount(void)
    { 
        if ( BaseDesc() && BaseDesc()->_apHdlDesc ) 
            return BaseDesc()->_apHdlDesc->uNumPropDescs;
        else
            return 0;
    }

    const VTABLEDESC * GetVTableArray(void) 
    { 
        if ( BaseDesc() && BaseDesc()->_apHdlDesc ) 
            return BaseDesc()->_apHdlDesc->pVTableArray;
        else
            return NULL;
    }
    UINT GetVTableCount(void)
    { 
        if ( BaseDesc() && BaseDesc()->_apHdlDesc ) 
            return BaseDesc()->_apHdlDesc->uVTblIndex;
        else
            return 0;
    }

    virtual CAtomTable * GetAtomTable (BOOL * pfExpando = NULL)
        { if (pfExpando) *pfExpando = TRUE; return NULL; }

    // AttrArray accessing functions:

    CAttrArray **GetAttrArray ( ) const { return const_cast<CAttrArray **>(&_pAA); }
    void SetAttrArray (CAttrArray *pAA) { _pAA = pAA; }

    AAINDEX FindAAIndex(
        DISPID dispID, 
        CAttrValue::AATYPE aaType, 
        AAINDEX aaLastOne = AA_IDX_UNKNOWN,
        BOOL fAllowMultiple = FALSE)
        {  return  (_pAA && _pAA->Find(dispID, aaType, &aaLastOne, fAllowMultiple)) ? 
                aaLastOne : AA_IDX_UNKNOWN; }

    AAINDEX FindAAIndex(
        DISPID dispID, 
        CAttrValue::AATYPE aaType, 
        AAINDEX aaLastOne,
        BOOL fAllowMultiple,
        CAttrArray *pAA)
        {  return (pAA && pAA->Find(dispID, aaType, &aaLastOne, fAllowMultiple)) ? 
                aaLastOne : AA_IDX_UNKNOWN; }

    BOOL DidFindAAIndexAndDelete (DISPID dispID, CAttrValue::AATYPE aaType);

    void FindAAIndexAndDelete (DISPID dispID, CAttrValue::AATYPE aaType)
    { DidFindAAIndexAndDelete (dispID, aaType); }

    AAINDEX FindAAType(CAttrValue::AATYPE aaType, AAINDEX lastIndex);
    AAINDEX FindNextAAIndex(
        DISPID dispid, 
        CAttrValue::AATYPE aatype,
        AAINDEX aaLastOne);

    BOOL FindString ( DISPID dispID, LPCTSTR *ppStr, CAttrValue::AATYPE aaType = CAttrValue::AA_Attribute, const PROPERTYDESC **ppPropertyDesc = NULL  )
    {
        if ( _pAA )
            return _pAA->FindString(dispID,ppStr,aaType,ppPropertyDesc);
        else
            return FALSE;
    }
    
    HRESULT GetStringAt(AAINDEX aaIdx, LPCTSTR *ppStr);
    HRESULT GetIntoStringAt(AAINDEX aaIdx, BSTR *pbStr, LPCTSTR *ppStr);
    HRESULT GetIntoBSTRAt(AAINDEX aaIdx, BSTR *pbStr );

    HRESULT GetSimpleAt(AAINDEX aaIdx, DWORD *pdwValue)
    {
        if ( _pAA )
            return _pAA->GetSimpleAt(aaIdx, pdwValue);
        else
            return  DISP_E_MEMBERNOTFOUND;
    }
    HRESULT GetPointerAt(AAINDEX aaIdx, void **pdwValue);
    HRESULT GetVariantAt(AAINDEX aaIdx, VARIANT *pVar, BOOL fAllowEmptyVariant = TRUE);
    HRESULT GetUnknownObjectAt(AAINDEX aaIdx, IUnknown **ppUnk)
        { RRETURN(GetObjectAt(aaIdx, VT_UNKNOWN, (void **)ppUnk)); }
    HRESULT GetDispatchObjectAt(AAINDEX aaIdx, IDispatch **ppDisp)
        { RRETURN(GetObjectAt(aaIdx, VT_DISPATCH, (void **)ppDisp)); }

    DISPID  GetDispIDAt(AAINDEX aaIdx)
        { 
            if (_pAA)
            {
                CAttrValue     *pAV;

                pAV = _pAA->FindAt(aaIdx);
                // Found AttrValue?
                if (pAV)
                    return pAV->GetDISPID();
            }
            return DISPID_UNKNOWN;
        }

    CAttrValue::AATYPE  GetAAtypeAt(AAINDEX aaIdx);

    CAttrValue * GetAttrValueAt (AAINDEX aaIdx)
        { return _pAA ? _pAA->FindAt(aaIdx) : NULL; }
  
    UINT GetVariantTypeAt ( AAINDEX aaIdx );

    void    DeleteAt(AAINDEX aaIdx)
        { if (_pAA) _pAA->Destroy(aaIdx); }

    HRESULT AddSimple(DISPID dispID,
                      DWORD dwSimple,
                      CAttrValue::AATYPE aaType);
    HRESULT AddPointer(DISPID dispID,
                      void *pValue,
                      CAttrValue::AATYPE aaType);
    HRESULT AddString(DISPID dispID,
                      LPCTSTR pch,
                      CAttrValue::AATYPE aaType);
    HRESULT AddBSTR(DISPID dispID,
                      LPCTSTR pch,
                      CAttrValue::AATYPE aaType);
    HRESULT AddUnknownObject(DISPID dispID,
                      IUnknown *pUnk,
                      CAttrValue::AATYPE aaType);
    HRESULT AddDispatchObject(DISPID dispID,
                      IDispatch *pDisp,
                      CAttrValue::AATYPE aaType);
    HRESULT AddVariant(DISPID dispID,
                       VARIANT *pVar,
                       CAttrValue::AATYPE aaType);
    HRESULT AddAttrArray(DISPID dispID,
                      CAttrArray *pAttrArray,
                      CAttrValue::AATYPE aaType);
    HRESULT AddUnknownObjectMultiple(
                      DISPID dispid,
                      IUnknown *pUnk,
                      CAttrValue::AATYPE aaType,
                      CAttrValue::AAExtraBits wFlags = CAttrValue::AA_Extra_Empty);
    HRESULT AddDispatchObjectMultiple(
                      DISPID dispid,
                      IDispatch *pDisp,
                      CAttrValue::AATYPE aaType);

    HRESULT ChangeSimpleAt(AAINDEX aaIdx, DWORD dwSimple);
    HRESULT ChangeStringAt(AAINDEX aaIdx, LPCTSTR pch);
    HRESULT ChangeUnknownObjectAt(AAINDEX aaIdx, IUnknown *pUnk);
    HRESULT ChangeDispatchObjectAt(AAINDEX aaIdx, IDispatch *pDisp);
    HRESULT ChangeVariantAt(AAINDEX aaIdx, VARIANT *pVar);
    HRESULT ChangeAATypeAt(AAINDEX aaIdx, CAttrValue::AATYPE aaType);

    HRESULT FetchObject(CAttrValue *pAV, VARTYPE vt, void **ppvoid);

    HRESULT GetObjectAt(AAINDEX aaIdx, VARTYPE vt, void **ppVoid);

#ifdef _WIN64
    HRESULT GetCookieAt(AAINDEX aaIdx, DWORD * pdwCookie);
    HRESULT SetCookieAt(AAINDEX aaIdx, DWORD dwCookie);
#endif

    HRESULT StoreEventsToHook(InlineEvts *pInlineEvts);
    InlineEvts * GetEventsToHook();

protected:

    ULONG       _ulRefs;
    ULONG       _ulAllRefs;
    CAttrArray *_pAA;

#ifdef OBJCNTCHK
public:
    DWORD       _dwTidDbg;
    void CBaseCheckThread();
#define CBASE_OBJCNTCHK_SIZE    (sizeof(DWORD))
#else
    void CBaseCheckThread() {}
#define CBASE_OBJCNTCHK_SIZE    (0)
#endif

#if DBG==1
public:
    // These "social security" numbers help us identify and track object
    // allocations and manipulations in the debugger.  At age 65 all objects retire.
    ULONG         _ulSSN;
    static ULONG  s_ulLastSSN;
#define CBASE_DBG_SIZE  (sizeof(ULONG))
#else
#define CBASE_DBG_SIZE  (0)
#endif

protected:

    // IHTMLObject methods
    #define _CBase_
    #include "types.hdl"

    // Tear off interfaces
    DECLARE_TEAROFF_TABLE(IDispatchEx)
    DECLARE_TEAROFF_TABLE(ISupportErrorInfo)
    DECLARE_TEAROFF_TABLE(IOleCommandTarget)
    DECLARE_TEAROFF_TABLE(ISpecifyPropertyPages)
    DECLARE_TEAROFF_TABLE(IObjectIdentity)
    DECLARE_TEAROFF_TABLE(IProvideMultipleClassInfo)

    static CMDINFOSTRUCT cmdTable[];

#ifdef WIN16
public:
    int m_baseOffset;
#endif
};

#define DECLARE_CLASSDESC_MEMBERS                       \
    static const CLASSDESC   s_classdesc;               \
    virtual const CBase::CLASSDESC *GetClassDesc() const      \
        { return (CBase::CLASSDESC *)&s_classdesc;}

#ifdef UNIX
#define COMPILE_TIME_ASSERT(x,y)
#else
#define COMPILE_TIME_ASSERT(x,y)  typedef int _farf_##x[sizeof(x) == (y)]
#endif

// Compile time "Assert".  The compiler can't allocate an array of 0 elements.
// If the expression inside [ ] is false, the array size will be zero!
#define CBASE_SIZE (sizeof(void *) + (2*sizeof(DWORD)) + sizeof(void *) + CBASE_DBG_SIZE + CBASE_OBJCNTCHK_SIZE)
COMPILE_TIME_ASSERT(CBase, CBASE_SIZE);

//
// The following flags specify which data follows relevant structures.  They
// follow in the order specified, member, indirect, etc...
//

enum PROPPARAM_FLAGS
{
    PROPPARAM_MEMBER                = 0x00000001,
    PROPPARAM_GETMFHandler          = 0x00000004,
    PROPPARAM_SETMFHandler          = 0x00000008,
    PROPPARAM_RESTRICTED            = 0x00000020,
    PROPPARAM_HIDDEN                = 0x00000040,
    // PROPTYPE_NOPERSIST is used to mark entries in the descriptor table which are
    // no longer supported by the object.  They are not written out, and are
    // only parsed (without actually reading the data) when read in.  This
    // flag should be OR'd together with the original type, so that the data
    // size can be inferred.
    //
    PROPPARAM_NOPERSIST             = 0x00000080,

    // Property supports Get and/or Put (Read-only, R/W, or Write-only).  This
    // also signals if the the item is a property or method (either set it's a
    // property otherwise a method).
    PROPPARAM_INVOKEGet             = 0x00000100,
    PROPPARAM_INVOKESet             = 0x00000200,

    //
    // The lMax member of NUMPROPPARAMS structure points to the ENUMDESC
    // structure if this flag is set
    //
    PROPPARAM_ENUM                  = 0x00000400,
    //
    // Value is a CUnitValue-encoded long
    PROPPARAM_UNITVALUE             = 0x00000800,

    // Validation masks for PROPPARAM_UNITVALUE types

    PROPPARAM_BITMASK               = 0x00001000,

    // Specifies that attribute can represent a relative fontsize 1..7 or -1..+7
    PROPPARAM_FONTSIZE              = 0x00002000,

    // Specifies that attribute can represent any legal unit measurement type e.g. 1em, 2px etc.
    PROPPARAM_LENGTH                = 0x00004000,

    // Specifies that attribute can represent a percentage value  e.g. 25%
    PROPPARAM_PERCENTAGE            = 0x00008000,

    // Specifies that attribute can indicate table relative i.e. *
    PROPPARAM_TIMESRELATIVE         = 0x00010000,

    // Specifies that attribute can be a number without a unit specified e.g. 1, +1, 23 etc
    PROPPARAM_ANUMBER               = 0x00020000,

    // specifies whether the ulTagNotAssignedDefault value should be used.  If not, its type will still be used for CUnitValues.
    PROPPARAM_DONTUSENOTASSIGNED    = 0x00040000,

    // specifies whether illegal values should be set to the MIN or to the DEFAULT
    PROPPARAM_MINOUT                = 0x00080000,

    // Specifies that attribute is located in the AttrArray
    PROPPARAM_ATTRARRAY             = 0x00100000,

    // Specifies that attribute is case sensitive (eg. TYPE_TYPE)
    PROPPARAM_CASESENSITIVE         = 0x00200000,

    // This marks a property as containing a scriptlet.  Uses DISPID_THIS for
    // when invoked.
    PROPPARAM_SCRIPTLET             = 0x01000000,

    // This marks a property as applying to a CF/PF/FF/SF
    PROPPARAM_STYLISTIC_PROPERTY    = 0x02000000,

    // This marks a property as being a stylesheet property (used for quoting, etc.)
    PROPPARAM_STYLESHEET_PROPERTY   = 0x04000000,

    // If this flag is set, when we see "TAG PROPERTY=" OR "PROPERTY"
    // we apply the default ( rather than the noassigndefault ) AND if
    // The property is invalid we apply the noassigndefault (rather than the default)
    PROPPARAM_INVALIDASNOASSIGN     = 0x08000000,
    PROPPARAM_NOTPRESENTASDEFAULT   = 0x10000000,
};

//
// Handy unit value combos
//
// Plain font
#define PP_UV_FONT (PROPPARAM_FONTSIZE | PROPPARAM_UNITVALUE )

// CSS font - does not allow unadorned numbers
#define PP_UV_UNITFONT (PROPPARAM_UNITVALUE | PROPPARAM_LENGTH | PROPPARAM_PERCENTAGE)

// HTML ATRIBUTES
#define PP_UV_LENGTH (PROPPARAM_UNITVALUE )
#define PP_UV_LENGTH_OR_PERCENT (PP_UV_LENGTH | PROPPARAM_PERCENTAGE )

// CSS values
#define PP_UV_SCALER (PROPPARAM_LENGTH | PROPPARAM_UNITVALUE )
#define PP_UV_SCALER_OR_PERCENT (PP_UV_SCALER | PROPPARAM_PERCENTAGE )

//
//  Our property types are the same as the first 32 VT_ VARENUM values
//  The private types start at 32
//
enum PROPTYPE_FLAGS
{
    PROPTYPE_EMPTY                      = 0,
        PROPTYPE_NULL                   = 1,
        PROPTYPE_I2                         = 2,
        PROPTYPE_I4                         = 3,
        PROPTYPE_R4                         = 4,
        PROPTYPE_R8                         = 5,
        PROPTYPE_CY                         = 6,
        PROPTYPE_DATE                   = 7,
        PROPTYPE_BSTR                   = 8,
        PROPTYPE_DISPATCH               = 9,
        PROPTYPE_ERROR                  = 10,
        PROPTYPE_BOOL                   = 11,
        PROPTYPE_VARIANT                = 12,
        PROPTYPE_UNKNOWN                = 13,
        PROPTYPE_DECIMAL                = 14,
        PROPTYPE_I1                         = 16,
        PROPTYPE_UI1                    = 17,
        PROPTYPE_UI2                    = 18,
        PROPTYPE_UI4                    = 19,
        PROPTYPE_I8                         = 20,
        PROPTYPE_UI8            = 21,
        PROPTYPE_INT            = 22,
        PROPTYPE_UINT           = 23,
        PROPTYPE_VOID           = 24,
        PROPTYPE_HRESULT            = 25,
        PROPTYPE_PTR            = 26,
        PROPTYPE_SAFEARRAY          = 27,
        PROPTYPE_CARRAY         = 28,
        PROPTYPE_USERDEFINED    = 29,
        PROPTYPE_LPSTR          = 30,
        PROPTYPE_LPWSTR         = 31,

    // our private types

    PROPTYPE_HTMLNUM        = 32,   // Private type -- HTML number: %50, etc
    PROPTYPE_ITEM_ARY       = 33,   // Private type -- array of ItemList's
    PROPTYPE_BSTR_ARY       = 34,   // Private type -- array of BSTRs
    PROPTYPE_CSTRING        = 35,   // Private type -- CStr
    PROPTYPE_CSTR_ARY       = 36,   // Private type -- array of CStr's

    PROPTYPE_LAST        = PROPTYPE_LPWSTR,
    PROPTYPE_MAX        = 0xFFFFFFFF
};

//
// these are helper macros for HandleProperty methods
//
#define PROPTYPE(dw)    (dw >> 16)
#define SETPROPTYPE(dw,t)   dw = (dw & 0x0000FFFF) | (t << 16)
#define OPCODE(dw)      (dw & HANDLEPROP_OPMASK)
#define OPERATION(dw)   (dw & HANDLEPROP_OPERATION)
#define SETOPCODE(dw,o) dw = (dw & ~HANDLEPROP_OPMASK) | o
#define ISSET(dw)       (dw & HANDLEPROP_SET)
#define ISSTREAM(dw)    (dw & HANDLEPROP_STREAM)
#define ISSAMPLING(dw) ( dw & HANDLEPROP_SAMPLE )
#define SETSAMPLING(dw) (dw |= HANDLEPROP_SAMPLE) 
//
//  These flags are used to indicate the operation in the HandleProperty methods
//
enum HANDLEPROP_FLAGS
{
    HANDLEPROP_DEFAULT      = 0x001,    // property value to default
    HANDLEPROP_VALUE        = 0x002,    // property value
    HANDLEPROP_AUTOMATION   = 0x004,    // property value with validation and extended error handling
    HANDLEPROP_COMPARE      = 0x008,    // property value compare
    HANDLEPROP_STREAM       = 0x010,    // use stream
    HANDLEPROP_OPMASK       = 0x01F,    // mask for operation code above

    HANDLEPROP_SET          = 0x020,    // set

    HANDLEPROP_DONTVALIDATE = 0x040,    // IMG width&height - don't validate when setting
    HANDLEPROP_SAMPLE       = 0x080,    // sniffing for a good parse or not
    HANDLEPROP_IMPORTANT    = 0x100,    // stylesheets: this property is "!important"
    HANDLEPROP_IMPLIED      = 0x200,    // stylesheets: this property has been implied by another.

    HANDLEPROP_MERGE        = 0x400,    // property value must case an onPropertyChange

    HANDLEPROP_OPERATION    = 0xFFFF,   // Opcode and extra bits
    HANDLEPROP_MAX          = 0xFFFFFFFF        // another enum we need to make max for win16.
};

#define HANDLEPROP_SETHTML (HANDLEPROP_SET | HANDLEPROP_VALUE | (PROPTYPE_LPWSTR << 16))
#define HANDLEPROP_MERGEHTML (HANDLEPROP_SET | HANDLEPROP_VALUE | HANDLEPROP_MERGE | (PROPTYPE_LPWSTR << 16))
#define HANDLEPROP_GETHTML (HANDLEPROP_STREAM | (PROPTYPE_LPWSTR << 16))

#ifdef WIN16
typedef HRESULT (BUGCALL *PFN_HANDLEPROPERTY)(PROPERTYDESC *, DWORD dwOpcode, void * pValue, CBase * pObject, CVoid * pSubObject);
#define PROPERTYDESC_METHOD(fn)\
        static HRESULT BUGCALL handle ## fn ## property(PROPERTYDESC *pObj, DWORD dwOpCode, void *pValue, CBase *pObject, CVoid *pSubObject);\
        HRESULT BUGCALL Handle ## fn ## Property(DWORD, void *, CBase *, CVoid *) const;
#else
typedef HRESULT (BUGCALL PROPERTYDESC::*PFN_HANDLEPROPERTY)(DWORD dwOpcode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
#define PROPERTYDESC_METHOD(fn)\
        HRESULT BUGCALL Handle ## fn ## Property(DWORD, void *, CBase *, CVoid *) const;
#endif

struct PROPERTYDESC
{
    //
    // These property handlers are the combination of the old Set/Get property helpers, the
    // persistance helpers and the new HTML string parsing helpers.
    //
    // The first DWORD is encoding the incoming type (PROPTYPE_FLAG) in the upper WORD and
    // the opcode in the lower WORD (HANDLERPROP_FLAG)
    //
    // The pValue is pointing to the 'media' the value is stored for the get and set
    //
    PROPERTYDESC_METHOD(Num)      //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(String)   //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Enum)     //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Color)    //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Variant)  //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(UnitValue)// (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Style)    //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(StyleComponent) //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Url)      //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;
    PROPERTYDESC_METHOD(Code)     //  (DWORD dwOpCode, void * pValue, CBase * pObject, CVoid * pSubObject) const;

    inline HRESULT Default(CBase * pObject) const
        {
#ifdef WIN16
            return (pfnHandleProperty ? 
                (pfnHandleProperty) ((PROPERTYDESC *)this,
                    HANDLEPROP_SET | HANDLEPROP_DEFAULT, 
                    NULL,
                    pObject,
                    CVOID_CAST(pObject))  : 
                S_OK);
#else
            return (pfnHandleProperty ? 
                CALL_METHOD(this, pfnHandleProperty, (
                    HANDLEPROP_SET | HANDLEPROP_DEFAULT, 
                    NULL,
                    pObject,
                    CVOID_CAST(pObject)))  : 
                S_OK);
#endif
        }

    ENUMDESC *GetEnumDescriptor ( void ) const
        {
            ENUMDESC *pEnumDesc;
            NUMPROPPARAMS *ppp = (NUMPROPPARAMS *)(this+1);

            if ( !(GetPPFlags() & PROPPARAM_ENUM ) )
                return NULL;

            if ( GetPPFlags() & PROPPARAM_ANUMBER )
            {
                // enumref ptr is after the sizeof() member
                pEnumDesc = *(ENUMDESC **)((BYTE *)(ppp+1)+ sizeof(DWORD_PTR));
            }
            else
            {
                // enumref is coded in the lMax
                pEnumDesc = (ENUMDESC *) ppp->lMax;
            }
            return pEnumDesc;
        }
    const BASICPROPPARAMS* GetBasicPropParams() const
        {
            return (const BASICPROPPARAMS*) (this+1);
        }
    const NUMPROPPARAMS* GetNumPropParams() const
        {
            return (const NUMPROPPARAMS*) (this+1);
        }

    const DWORD GetPPFlags() const
        {
            return GetBasicPropParams()->dwPPFlags;
        }
    const DWORD GetdwFlags() const
        {
            return GetBasicPropParams()->dwFlags;
        }
    const DISPID GetDispid() const
        {
            return GetBasicPropParams()->dispid;
        }
    HRESULT TryLoadFromString ( DWORD dwOpCode, LPCTSTR szString, CBase *pBaseObject, CAttrArray**ppAA ) const
        {
#ifdef WIN16
            return ( (pfnHandleProperty) ( (PROPERTYDESC *)this, dwOpCode|(PROPTYPE_LPWSTR<<16|HANDLEPROP_SAMPLE), 
                    const_cast<TCHAR *> (szString), pBaseObject, 
                    (CVoid *)(void *)ppAA ) );
#else
            return ( CALL_METHOD(this,pfnHandleProperty, ( dwOpCode|(PROPTYPE_LPWSTR<<16|HANDLEPROP_SAMPLE), 
                    const_cast<TCHAR *> (szString), pBaseObject, 
                    (CVoid *)(void *)ppAA )) );
#endif
        }
    HRESULT CallHandler ( CBase *pBaseObject, DWORD dwFlags, void *pvArg ) const
        {
            BASICPROPPARAMS *pbpp = (BASICPROPPARAMS *)(this+1);
#ifdef WIN16
            return ( (pfnHandleProperty) ( (PROPERTYDESC *)this, dwFlags, pvArg, pBaseObject, 
                pbpp->dwPPFlags & PROPPARAM_ATTRARRAY ?
                (CVoid *)(void *)pBaseObject->GetAttrArray() : (CVoid *)(void *)pBaseObject ) );
#else
            return ( CALL_METHOD(this, pfnHandleProperty, ( dwFlags, pvArg, pBaseObject, 
                pbpp->dwPPFlags & PROPPARAM_ATTRARRAY ?
                (CVoid *)(void *)pBaseObject->GetAttrArray() : (CVoid *)(void *)pBaseObject ) ));
#endif
    }
    HRESULT HandleLoadFromHTMLString ( CBase *pBaseObject, LPTSTR szString ) const
        {
            return  CallHandler ( pBaseObject, HANDLEPROP_SETHTML, (void *)szString ); 
        }
    HRESULT HandleMergeFromHTMLString ( CBase *pBaseObject, LPTSTR szString ) const
        {
            return  CallHandler ( pBaseObject, HANDLEPROP_MERGEHTML, (void *)szString ); 
        }
    HRESULT HandleSaveToHTMLStream ( CBase *pBaseObject, void *pStream ) const
        {
            return  CallHandler ( pBaseObject, HANDLEPROP_GETHTML, (void *)pStream ); 
        }
    HRESULT HandleCompare ( CBase *pBaseObject, void *pWith ) const
        {
            return  CallHandler ( pBaseObject, HANDLEPROP_COMPARE, (void *)pWith ); 
        }
    // Gets property into a VARIANT in most compact representation
    HRESULT HandleGetIntoVARIANT ( CBase *pBaseObject, VARIANT *pVariant ) const
        {
            return  CallHandler ( pBaseObject, (HANDLEPROP_VALUE | (PROPTYPE_VARIANT << 16)), 
                (void *)pVariant ); 
        }
    // Get property into VARIANT compatable with readinf property thru automation
    HRESULT HandleGetIntoAutomationVARIANT ( CBase *pBaseObject, VARIANT *pVariant ) const
        {
            return  CallHandler ( pBaseObject, (HANDLEPROP_AUTOMATION | (PROPTYPE_VARIANT << 16)), 
                (void *)pVariant ); 
        }
    HRESULT HandleGetIntoBSTR ( CBase *pBaseObject, BSTR *pBSTR ) const
        {
            return  CallHandler ( pBaseObject, (HANDLEPROP_VALUE | (PROPTYPE_BSTR << 16)), 
                (void *)pBSTR ); 
        }

    BOOL IsBOOLProperty ( void ) const;
    BOOL IsNotPresentAsDefaultSet ( void ) const 
        {
            return GetPPFlags() & PROPPARAM_NOTPRESENTASDEFAULT;
        }
    BOOL UseNotAssignedValue ( void ) const 
        {
            return !(GetPPFlags() & PROPPARAM_DONTUSENOTASSIGNED);
        }
    BOOL IsInvalidAsNoAssignSet ( void ) const 
        {
            return GetPPFlags() & PROPPARAM_INVALIDASNOASSIGN;
        }
    BOOL IsStyleSheetProperty ( void ) const 
        {
            return GetPPFlags() & PROPPARAM_STYLESHEET_PROPERTY;
        }

    ULONG GetNotPresentDefault ( void ) const
        {
            if ( IsNotPresentAsDefaultSet() || !UseNotAssignedValue() )
                return ulTagNotPresentDefault;
            else
                return ulTagNotAssignedDefault;
        }
    ULONG GetInvalidDefault ( void ) const
        {
            if ( IsInvalidAsNoAssignSet() && UseNotAssignedValue() )
                return ulTagNotAssignedDefault;
            else
                return ulTagNotPresentDefault;
        }
    PFN_HANDLEPROPERTY  pfnHandleProperty;  // Pointer to the handler function
    const TCHAR *       pstrName;           // Name of property
    const TCHAR *       pstrExposedName;    // Exposed name of the property
    // We have two possible "defaults"
    // The first one is applied if the Tag was just plain missing
    ULONG_PTR           ulTagNotPresentDefault;
    // The second is applied if the Tag is present, but not set equal to anything
    // e.g. "SOMETAG=" OR "SOMETAG"
    ULONG_PTR           ulTagNotAssignedDefault;
};

inline const PROPERTYDESC *BASICPROPPARAMS::GetPropertyDesc ( void ) const
{ 
    return (const PROPERTYDESC *)this -1; 
}

inline const PROPERTYDESC *NUMPROPPARAMS::GetPropertyDesc ( void ) const
{ 
    return (const PROPERTYDESC *)this -1; 
}

// in the new structures lMax points to this ENUMDESC


struct DEFAULTARGDESC
{
#if defined(UNIX) || defined(_MAC)
    DWORD_PTR  defargs[10];   // AR: Varma: IEUNIX BUGBUG: For now, 10 has been selected.
			   // This is a variable len struct, and needs to be hadled in
			   // appropriate way.  (Ref: DYNAMICARRAY_NOTSUPPORTED) 
#else
    DWORD_PTR  defargs[];
#endif
};

//+------------------------------------------------------------------------
//
//  The following macros help define get/set methods for properties
//  which can be accessed from the 'this' pointer and which are numeric.
//
//-------------------------------------------------------------------------

// Use VT_BOOL4 when automation type is not VARIANT_BOOL, but BOOL, which is
// 4 bytes, instead of 2

#define VT_BOOL4 0xfe   // 254 is not used for another common VT_*


// Handy macro for building a raw unit value, e.g.
// for defining enum value equivalents
class CUnitValue : public CVoid
{
public:
    static const ULONG uValue;
    // Unit-Value types, stored in bits 0..3 of the value long
    enum UNITVALUETYPE
    {
        UNIT_NULLVALUE,      // Value was not specified on input and should not be persisted
        UNIT_POINT,         // Points
        UNIT_PICA,          // Picas
        UNIT_INCH,          // Inches
        UNIT_CM,            // Centimeters
        UNIT_MM,            // Millimeters
        UNIT_EM,            // em's ( relative to width of an "m" character in current-font )
        UNIT_EN,            // en's ( relative to width of an "n" character in current-font )
        UNIT_EX,            // ex's ( relative to height of an "x" character in current-font )
        UNIT_PIXELS,        // Good old-fashioned pixels
        UNIT_PERCENT,       // Percentage
        UNIT_TIMESRELATIVE, // Times something - *
        UNIT_FLOAT,         // A float value (like integer, but with 4 digits of precision)
        UNIT_INTEGER,       // An integer
        UNIT_RELATIVE,      // A relative integer - the sign of the value was specified
        UNIT_ENUM,          // Font size keywords - "large", "small", etc.
        NUM_TYPES,
        MAX_TYPES = 0xFFFFFFFF  // To make this a long.
    };
    // When we get a value that's stored in pels, need to convert using log pixels
    // thus we need to know the direction
    enum DIRECTION
    {
        DIRECTION_CX,
        DIRECTION_CY
    };
    struct TypeDesc
    {
        TCHAR *pszName;
        UNITVALUETYPE uvt;
        WORD wShiftOffset; // Number of places right to shift dec point when scaling
        WORD wScaleMult; // Scale mult - equals 10^wShiftOffset
    };
    static CUnitValue::TypeDesc TypeNames[NUM_TYPES];
    struct ConvertTable
    {
        long lMul;
        long lDiv;
    };
    static ConvertTable BasicConversions[6][6];

    CUnitValue() { _lValue = 0; }
    CUnitValue( ULONG lValue ) { _lValue = lValue; }

private:
    // The _lValue is the actual long value that encodes the unit & value
    long _lValue;
    //
    enum { TYPE_MASK = 0x0F };
    HRESULT NumberFromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc );

    static long ConvertTo ( long lValue,
                                        UNITVALUETYPE uvtFromUnits,
                                        UNITVALUETYPE uvtTo,
                                        DIRECTION direction,
                                        long lFontHeight=1);

   // Set the value in current working units
    BOOL SetHimetricMeasureValue ( long lNewValue, DIRECTION direction, long lFontHeight );
    BOOL SetPercentValue ( long lNewValue, DIRECTION direction, long lPercentOfValue );
    float GetFloatValueInUnits ( UNITVALUETYPE uvt, DIRECTION dir, long lFontHeight=1 );
    HRESULT SetFloatValueKeepUnits ( float fValue,
                                     UNITVALUETYPE uvt,
                                     long lCurrentPixelValue,
                                     DIRECTION dir,
                                     long lFontHeight);
public:
    static BOOL IsScalerUnit ( UNITVALUETYPE uvt ) ;
    HRESULT FormatBuffer ( LPTSTR szBuffer, UINT uMaxLen, const PROPERTYDESC *pPropDesc, 
                            BOOL fAlwaysAppendUnit = FALSE) const;
    enum { NUM_TYPEBITS = 4 };
    long SetValue ( long lVal, UNITVALUETYPE uvt );
    long SetRawValue ( long lVal ) { return ( _lValue = lVal ); }
    long SetDefaultValue ( void ) { return SetValue ( 0, UNIT_NULLVALUE ); }
    long GetRawValue  ( void ) const { return _lValue; }

    // Get the value in current working units
    long GetMeasureValue ( DIRECTION direction,
                           UNITVALUETYPE uvtConvertToUnits = UNIT_PIXELS,
                           long lFontHiehgt=1 ) const;
    long GetPercentValue ( DIRECTION direction, long lPercentOfValue ) const;

    HRESULT ConvertToUnitType ( UNITVALUETYPE uvt, long lCurrentPixelSize, DIRECTION dir, long lFontHeight = 1 );
    HRESULT SetFloatUnitValue ( float fValue );
    float XGetFloatValueInUnits ( UNITVALUETYPE uvt, long lFontHeight=1)
    {
        return GetFloatValueInUnits ( uvt, DIRECTION_CX, lFontHeight);
    }
    float YGetFloatValueInUnits ( UNITVALUETYPE uvt, long lFontHeight=1)
    {
        return GetFloatValueInUnits ( uvt, DIRECTION_CY, lFontHeight  );
    }
    HRESULT XConvertToUnitType ( UNITVALUETYPE uvt, long lCurrentPixelSize, long lFontHeight )
    {
        return ConvertToUnitType ( uvt, lCurrentPixelSize, DIRECTION_CX, lFontHeight );
    }
    HRESULT YConvertToUnitType ( UNITVALUETYPE uvt, long lCurrentPixelSize, long lFontHeight )
    {
        return ConvertToUnitType ( uvt, lCurrentPixelSize, DIRECTION_CY, lFontHeight );
    }
    HRESULT XSetFloatValueKeepUnits ( float fValue, UNITVALUETYPE uvt,
                                      long lCurrentPixelValue, long lFontHeight=1 )
    {
        return SetFloatValueKeepUnits ( fValue, uvt, lCurrentPixelValue, DIRECTION_CX, lFontHeight );
    }
    HRESULT YSetFloatValueKeepUnits ( float fValue, UNITVALUETYPE uvt,
                                      long lCurrentPixelValue, long lFontHeight=1 )
    {
        return SetFloatValueKeepUnits ( fValue, uvt, lCurrentPixelValue, DIRECTION_CY, lFontHeight );
    }

    // Get the PIXEL equiv.
    long XGetPixelValue ( CTransform *pTransform, long lPercentOfValue, long yHeight ) const
    {
        return GetPixelValue ( pTransform, DIRECTION_CX, lPercentOfValue, yHeight );
    }
    long YGetPixelValue ( CTransform *pTransform, long lPercentOfValue, long yHeight) const
    {
         return GetPixelValue ( pTransform, DIRECTION_CY, lPercentOfValue, yHeight );
    }
    long ScaleTo ( UNITVALUETYPE uvt )
    {
        return SetValue ( MulDivQuick ( GetUnitValue(), TypeNames[uvt].wScaleMult , TypeNames[GetUnitType()].wScaleMult ),
            uvt );
    }
    // GetUnitValue - we need to know whether it was Null
    long GetUnitValue ( void ) const { return _lValue >> NUM_TYPEBITS; }
    HRESULT FromString ( LPCTSTR pStr, const PROPERTYDESC *pPropDesc );
    UNITVALUETYPE GetUnitType ( void ) const { return (UNITVALUETYPE)(_lValue & TYPE_MASK); }

    HRESULT Persist ( IStream *pStream, const PROPERTYDESC *pPropDesc ) const;
    HRESULT IsValid  ( const PROPERTYDESC *pPropDesc ) const;
    //
    // These are the 3 primary methods for interfacing
    //

    // Check for Null if you allowed Null values in your PROPDESC
    BOOL IsNull ( void ) const { return GetUnitType() == UNIT_NULLVALUE; }
    void SetNull ( void ) { SetValue ( 0, UNIT_NULLVALUE ); }
    BOOL IsNullOrEnum ( void ) const { return (GetUnitType() == UNIT_NULLVALUE) 
                                        || (GetUnitType() == UNIT_ENUM); }


    // Store the equiv of a HIMETRIC value
    BOOL SetFromHimetricValue ( long lNewValue,
                                DIRECTION direction = DIRECTION_CX,
                                long lPercentOfValue = 0,
                                long lFontHeight =1);

    BOOL IsPercent() const
    {
        return GetUnitType() == UNIT_PERCENT && GetUnitValue();
    }

    long GetPercent() const
    {
        Assert(GetUnitType() == UNIT_PERCENT);
        return GetUnitValue() / TypeNames[UNIT_PERCENT].wScaleMult;
    }

    void SetPercent(long l)
    {
        SetValue(l * TypeNames[UNIT_PERCENT].wScaleMult, UNIT_PERCENT);
    }
    // Set from a whole number of points
    void SetPoints(long l)
    {
        SetValue(l * TypeNames[UNIT_POINT].wScaleMult, UNIT_POINT);
    }
    // Get the whole number of points
    long GetPoints() const
    {
        Assert(GetUnitType() == UNIT_POINT);
        return GetUnitValue() / TypeNames[UNIT_POINT].wScaleMult;
    }

    // Get the whole number of EM
    long GetEM() const
    {
        Assert(GetUnitType() == UNIT_EM);
        return GetUnitValue() / TypeNames[UNIT_EM].wScaleMult;
    }

    // Get the whole number of EX
    long GetEX() const
    {
        Assert(GetUnitType() == UNIT_EX);
        return GetUnitValue() / TypeNames[UNIT_EX].wScaleMult;
    }

    // Get the whole number of PICA
    long GetPica() const
    {
        Assert(GetUnitType() == UNIT_PICA);
        return GetUnitValue() / TypeNames[UNIT_PICA].wScaleMult;
    }
    
    // Get the whole number of Inch
    long GetInch() const
    {
        Assert(GetUnitType() == UNIT_INCH);
        return GetUnitValue() / TypeNames[UNIT_INCH].wScaleMult;
    }
    
    // Get the whole number of CM
    long GetCM() const
    {
        Assert(GetUnitType() == UNIT_CM);
        return GetUnitValue() / TypeNames[UNIT_CM].wScaleMult;
    }
    
    // Get the whole number of MM
    long GetMM() const
    {
        Assert(GetUnitType() == UNIT_MM);
        return GetUnitValue() / TypeNames[UNIT_MM].wScaleMult;
    }
    
    long GetTimesRelative() const
    {
        Assert(GetUnitType() == UNIT_TIMESRELATIVE);
        return GetUnitValue() / TypeNames[UNIT_TIMESRELATIVE].wScaleMult;
    }

    void SetTimesRelative(long l)
    {
        SetValue(l * TypeNames[UNIT_TIMESRELATIVE].wScaleMult, UNIT_TIMESRELATIVE);
    }

    long GetPixelValue ( CTransform *pTransform = NULL ,
                         DIRECTION direction = DIRECTION_CX,
                         long lPercentOfValue = 0,
                         long lBaseFontHeight = 1 ) const
    {
        return (_lValue & ~TYPE_MASK) == 0 ?
                0 :
                GetPixelValueCore(pTransform,
                                  direction,
                                  lPercentOfValue,
                                  lBaseFontHeight );
    };

    long GetPixelValueCore ( CTransform *pTransform = NULL ,
                             DIRECTION direction = DIRECTION_CX,
                             long lPercentOfValue = 0,
                             long lBaseFontHeight = 1 ) const;
};


//+------------------------------------------------------------------------
//
//  The following macros help define get/set methods for properties
//  which can be accessed from helper get/set methods and which are numeric.
//
//-------------------------------------------------------------------------

#ifdef WIN16
typedef HRESULT (BUGCALL * PFN_NUMPROPGET) (CVoid *, long *pnVal );
typedef HRESULT (BUGCALL * PFN_NUMPROPSET) ( CVoid *, long   nVal  );

typedef HRESULT (BUGCALL * PFNB_NUMPROPGET) ( CBase *, long * pnVal );
typedef HRESULT (BUGCALL * PFNB_NUMPROPSET) ( CBase *, long   nVal  );

#define NV_DECLARE_PROPERTY_METHOD(fn, FN, args)\
            static HRESULT BUGCALL FN args;\
            HRESULT BUGCALL fn args

#define PROPERTY_METHOD(kind, getset, klass, fn, FN)\
            (PFN_##kind##PROP##getset)&klass::FN
#else
typedef HRESULT (BUGCALL CVoid::* PFN_NUMPROPGET) ( long * pnVal );
typedef HRESULT (BUGCALL CVoid::* PFN_NUMPROPSET) ( long   nVal  );

typedef HRESULT (BUGCALL CBase::* PFNB_NUMPROPGET) ( long * pnVal );
typedef HRESULT (BUGCALL CBase::* PFNB_NUMPROPSET) ( long   nVal  );

#define NV_DECLARE_PROPERTY_METHOD(fn, FN, args)\
            HRESULT BUGCALL fn args

#define PROPERTY_METHOD(kind, getset, klass, fn, FN)\
            (PFN_##kind##PROP##getset)(PFNB_##kind##PROP##getset)&klass::fn

#endif                        


//+------------------------------------------------------------------------
//
//  The following macros help define get/set methods for properties
//  which can be accessed from helper get/set methods and which are BSTR
//
//-------------------------------------------------------------------------



#ifdef WIN16
typedef HRESULT (BUGCALL * PFN_BSTRPROPGET) ( CVoid *, BSTR * pbstr );
typedef HRESULT (BUGCALL * PFN_BSTRPROPSET) ( CVoid *, BSTR   bstr );

typedef HRESULT (BUGCALL * PFN_CSTRPROPGET) ( CVoid *, CStr * pcstr );
typedef HRESULT (BUGCALL * PFN_CSTRPROPSET) ( CVoid *, CStr * pcstr );

typedef HRESULT (BUGCALL * PFNB_BSTRPROPGET) ( CBase *, BSTR * pbstr );
typedef HRESULT (BUGCALL * PFNB_BSTRPROPSET) ( CBase *, BSTR   bstr );

typedef HRESULT (BUGCALL * PFNB_CSTRPROPGET) ( CBase *, CStr * pcstr );
typedef HRESULT (BUGCALL * PFNB_CSTRPROPSET) ( CBase *, CStr * pcstr );

#else
typedef HRESULT (BUGCALL CVoid::* PFN_BSTRPROPGET) ( BSTR * pbstr );
typedef HRESULT (BUGCALL CVoid::* PFN_BSTRPROPSET) ( BSTR   bstr );

typedef HRESULT (BUGCALL CVoid::* PFN_CSTRPROPGET) ( CStr * pcstr );
typedef HRESULT (BUGCALL CVoid::* PFN_CSTRPROPSET) ( CStr * pcstr );

typedef HRESULT (BUGCALL CBase::* PFNB_BSTRPROPGET) ( BSTR * pbstr );
typedef HRESULT (BUGCALL CBase::* PFNB_BSTRPROPSET) ( BSTR   bstr );

typedef HRESULT (BUGCALL CBase::* PFNB_CSTRPROPGET) ( CStr * pcstr );
typedef HRESULT (BUGCALL CBase::* PFNB_CSTRPROPSET) ( CStr * pcstr );
#endif


//+------------------------------------------------------------------------
//
//  The following macros help define get/set methods for properties
//  which can be accessed from helper get/set methods and which are VARIANT
//
//-------------------------------------------------------------------------

#ifdef WIN16
typedef HRESULT (BUGCALL * PFN_VARIANTPROP) ( CVoid *, VARIANT * pbstr );
typedef HRESULT (BUGCALL * PFNB_VARIANTPROP) ( CBase *, VARIANT * pbstr );

#else
typedef HRESULT (BUGCALL CVoid::* PFN_VARIANTPROP) ( VARIANT * pbstr );
typedef HRESULT (BUGCALL CBase::* PFNB_VARIANTPROP) ( VARIANT * pbstr );
#endif


//+------------------------------------------------------------------------
//
//  Possible PROPERTY_DESC generated by pdlparser:
//
//      PROPERTYDESC_BASIC_ABSTRACT     (only if abstract set on class)
//      PROPERTYDESC_BASIC
//      PROPERTYDESC_NUMPROP_ABSTRACT   (only if abstract set on class)
//      PROPERTYDESC_NUMPROP
//      PROPERTYDESC_NUMPROP_GETSET
//      PROPERTYDESC_NUMPROP_ENUMREF
//
//+------------------------------------------------------------------------

struct PROPERTYDESC_METHOD
{
    PROPERTYDESC            a;
    BASICPROPPARAMS         b;
    const DEFAULTARGDESC   *c;      // pointer to default value arguments
    WORD                    d;      // total argument count
    WORD                    e;      // required count
};

struct PROPERTYDESC_BASIC_ABSTRACT
{
    PROPERTYDESC        a;
    BASICPROPPARAMS     b;
};

struct PROPERTYDESC_BASIC
{
    PROPERTYDESC        a;
    BASICPROPPARAMS     b;
    DWORD_PTR           c;
};

struct PROPERTYDESC_NUMPROP_ABSTRACT
{
    PROPERTYDESC        a;
    NUMPROPPARAMS       b;
};

struct PROPERTYDESC_NUMPROP
{
    PROPERTYDESC        a;
    NUMPROPPARAMS       b;
    DWORD_PTR           c;
};

struct PROPERTYDESC_NUMPROP_GETSET
{
    PROPERTYDESC        a;
    NUMPROPPARAMS       b;
    PFN_NUMPROPGET      c;
    PFN_NUMPROPSET      d;
};

struct PROPERTYDESC_NUMPROP_ENUMREF
{
    PROPERTYDESC        a;
    NUMPROPPARAMS       b;
    DWORD_PTR           c;
    const ENUMDESC     *pE;
};

struct PROPERTYDESC_CSTR_GETSET
{ 
    PROPERTYDESC a; 
    BASICPROPPARAMS b; 
    PFN_CSTRPROPGET c; 
    PFN_CSTRPROPSET d; 
};


//+------------------------------------------------------------------------
//
//  Class factory table
//
//-------------------------------------------------------------------------



class CInPlace;

//+---------------------------------------------------------------------------
//
//  Flag values for CServer::CLASSDESC::_dwFlags
//
//----------------------------------------------------------------------------

enum SERVERDESC_FLAG
{
    SERVERDESC_INVAL_ON_RESIZE      = BASEDESC_LAST                  << 1,
    SERVERDESC_CREATE_UNDOMGR       = SERVERDESC_INVAL_ON_RESIZE     << 1,
    SERVERDESC_ACTIVATEONENTRY      = SERVERDESC_CREATE_UNDOMGR      << 1,
    SERVERDESC_DEACTIVATEONLEAVE    = SERVERDESC_ACTIVATEONENTRY     << 1,
    SERVERDESC_ACTIVATEONDRAG       = SERVERDESC_DEACTIVATEONLEAVE   << 1,
    SERVERDESC_SUPPORT_DRAG_DROP    = SERVERDESC_ACTIVATEONDRAG      << 1,
    SERVERDESC_DRAGDROP_DESIGNONLY  = SERVERDESC_SUPPORT_DRAG_DROP   << 1,
    SERVERDESC_DRAGDROP_EVENTONLY   = SERVERDESC_DRAGDROP_DESIGNONLY << 1,
    SERVERDESC_HAS_MENU             = SERVERDESC_DRAGDROP_EVENTONLY  << 1,
    SERVERDESC_HAS_TOOLBAR          = SERVERDESC_HAS_MENU            << 1,
    SERVERDESC_UIACTIVE_DESIGNONLY  = SERVERDESC_HAS_TOOLBAR         << 1,
    SERVERDESC_LAST                 = SERVERDESC_UIACTIVE_DESIGNONLY,
    SERVERDESC_MAX                  = LONG_MAX    // needed to force enum to be dword
};

//+---------------------------------------------------------------------------
//
//  Flag values for CServer::OnPropertyChange
//
//----------------------------------------------------------------------------

enum SERVERCHNG_FLAG
{
    SERVERCHNG_NOPROPCHANGE = BASECHNG_LAST << 1,
    SERVERCHNG_NOVIEWCHANGE = SERVERCHNG_NOPROPCHANGE << 1,
    SERVERCHNG_NODATACHANGE = SERVERCHNG_NOVIEWCHANGE << 1,
    SERVERCHNG_LAST         = SERVERCHNG_NODATACHANGE,
    SERVERCHNG_MAX          = LONG_MAX    // needed to force enum to be dword
};

//+---------------------------------------------------------------------------
//
//  Flag values for CDoc::OnPropertyChange
//
//----------------------------------------------------------------------------

enum FORMCHNG_FLAG
{
    FORMCHNG_LAYOUT  = (SERVERCHNG_LAST << 1),
    FORMCHNG_NOINVAL = (FORMCHNG_LAYOUT << 1),
    FORMCHNG_LAST    = FORMCHNG_NOINVAL,
    FORMCHNG_MAX     = LONG_MAX    // needed to force enum to be dword on macintosh
};

//+---------------------------------------------------------------------------
//
//  Flag values for CServer::CLock
//
//----------------------------------------------------------------------------

enum SERVERLOCK_FLAG
{
    SERVERLOCK_STABILIZED       = 1,
    SERVERLOCK_TRANSITION       = SERVERLOCK_STABILIZED    << 1,
    SERVERLOCK_PROPNOTIFY       = SERVERLOCK_TRANSITION    << 1,
    SERVERLOCK_INONPROPCHANGE   = SERVERLOCK_PROPNOTIFY    << 1,
    // general purpose recursion blocking flag
    SERVERLOCK_BLOCKRECURSION   = SERVERLOCK_INONPROPCHANGE << 1,
    SERVERLOCK_BLOCKPAINT       = SERVERLOCK_BLOCKRECURSION << 1,
    SERVERLOCK_IGNOREERASEBKGND = SERVERLOCK_BLOCKPAINT     << 1,
    SERVERLOCK_VIEWCHANGE       = SERVERLOCK_IGNOREERASEBKGND << 1,
    SERVERLOCK_LAST             = SERVERLOCK_VIEWCHANGE,
    SERVERLOCK_MAX              = LONG_MAX    // Force enum to 32 bits
};

//+---------------------------------------------------------------
//
//  Class:      CServer
//
//  Purpose:    Base class for OLE Compound Document servers
//
//---------------------------------------------------------------

class NOVTABLE CServer :
        public CBase,
        public IViewObjectEx
{
    DECLARE_CLASS_TYPES(CServer, CBase)

private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))
public:

    // Tear off interfaces

    DECLARE_TEAROFF_TABLE(IOleObject)
    DECLARE_TEAROFF_TABLE(IOleControl)
    DECLARE_TEAROFF_TABLE(IRunnableObject)
    DECLARE_TEAROFF_TABLE(IExternalConnection)
    DECLARE_TEAROFF_TABLE(IPersistStreamInit)
    DECLARE_TEAROFF_TABLE(IPersistStorage)
    DECLARE_TEAROFF_TABLE(IDataObject)
    DECLARE_TEAROFF_TABLE(IOleDocument)
    DECLARE_TEAROFF_TABLE(IQuickActivate)
    DECLARE_TEAROFF_TABLE(IOleCache2)
    DECLARE_TEAROFF_TABLE(IPointerInactive)
    DECLARE_TEAROFF_TABLE(IPerPropertyBrowsing)
    DECLARE_TEAROFF_TABLE(IPersistPropertyBag)
    DECLARE_TEAROFF_TABLE(IOleInPlaceObjectWindowless)
    DECLARE_TEAROFF_TABLE(IOleDocumentView)
    DECLARE_TEAROFF_TABLE(IOleInPlaceActiveObject)

    //  Standard verb implementations

    typedef HRESULT (*PFNDOVERB) (CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);

    static HRESULT DoShow(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoHide(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoUIActivate(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoInPlaceActivate(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoProperties(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoUIActivateIfDesign(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);
    static HRESULT DoInPlaceActivateIfDesign(CServer *, LONG, LPMSG, IOleClientSite *, LONG, HWND, LPCRECT);

    //  Standard Get/Set clipboard format implementations

    typedef HRESULT (*LPFNGETDATA) (CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    typedef HRESULT (*LPFNSETDATA) (CServer *, LPFORMATETC, LPSTGMEDIUM);

    static HRESULT GetEMBEDDEDOBJECT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetMETAFILEPICT(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetOBJECTDESCRIPTOR(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetLINKSOURCE(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);
    static HRESULT GetENHMETAFILE(CServer *, LPFORMATETC, LPSTGMEDIUM, BOOL);

    // IRunnableObject methods

#if FANCY_CONNECTION_STUFF
    NV_STDMETHOD(GetRunningClass)(LPCLSID lpclsid);
    NV_STDMETHOD(Run)(LPBINDCTX pbc);
    NV_STDMETHOD_(BOOL, IsRunning)();
    NV_STDMETHOD(LockRunning)(BOOL fLock, BOOL fLastUnlockCloses);
    NV_STDMETHOD(SetContainedObject)(BOOL fContained);
#endif

    // IExternalConnection methods

#if FANCY_CONNECTION_STUFF
    NV_STDMETHOD_(DWORD,AddConnection)(DWORD extconn, DWORD reserved);
    NV_STDMETHOD_(DWORD,ReleaseConnection)(DWORD extconn,
            DWORD reserved, BOOL fLastReleaseCloses);
#endif

    // IPersist* methods

    // Over-ridden in CForm:
    STDMETHOD(IsDirty)();
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE isdirty(CServer *pObj)
    { return pObj->IsDirty(); }
#endif // ndef WIN16

    // IPersistStreamInit methods

    // Over-ridden in CPageSelector:
    DECLARE_TEAROFF_METHOD(Load, LOAD, (IStream *pStm));
    NV_DECLARE_TEAROFF_METHOD(Save, SAVE, (IStream *pStm, BOOL fClearDirty));
    NV_DECLARE_TEAROFF_METHOD(GetSizeMax, GETSIZEMAX, (ULARGE_INTEGER * pcbSize));
    // Over-ridden in CPageSelector, CMdcText:
    STDMETHOD(InitNew)();
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE initnew(CServer *pObj)
    {  return pObj->InitNew(); }
    static HRESULT STDMETHODCALLTYPE getclassid(CServer *pObj, CLSID *pclsid)
    {  return pObj->GetClassID(pclsid); }
#endif

    // IPersistStorage methods

    // Over-ridden in CForm, CCtrlSelector, CMdcText, CMultiPageControl:
    STDMETHOD(InitNew)(LPSTORAGE pStg);
    // Over-ridden in CMultiPageControl
    STDMETHOD(Load)(LPSTORAGE pStg);
    NV_STDMETHOD(Save)(LPSTORAGE pStgSave, BOOL fSameAsLoad);
    // Over-ridden in CForm:
    STDMETHOD(SaveCompleted)(LPSTORAGE pStgNew);
    STDMETHOD(HandsOffStorage)();

    // IPersistPropertyBag methods

    STDMETHOD(Save)(LPPROPERTYBAG pBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    STDMETHOD(Load)(LPPROPERTYBAG pBag, LPERRORLOG pErrLog);

    // IDataObject methods

    NV_DECLARE_TEAROFF_METHOD(GetData, getdata, (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium));
    NV_DECLARE_TEAROFF_METHOD(GetDataHere, getdatahere, (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium));
    NV_DECLARE_TEAROFF_METHOD(QueryGetData, querygetdata, (LPFORMATETC pformatetc));
    NV_DECLARE_TEAROFF_METHOD(GetCanonicalFormatEtc, getcanonicalformatetc, (
        LPFORMATETC pformatetc,
        LPFORMATETC pformatetcOut));
    NV_DECLARE_TEAROFF_METHOD(SetData, setdata, (
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fRelease));
    NV_DECLARE_TEAROFF_METHOD(EnumFormatEtc, enumformatetc, (DWORD dwDirection, LPENUMFORMATETC * ppenum));
    NV_DECLARE_TEAROFF_METHOD(DAdvise, dadvise, (
        FORMATETC * pFormatetc,
        DWORD advf,
        LPADVISESINK pAdvSink,
        DWORD * pdwConnection));
    NV_DECLARE_TEAROFF_METHOD(DUnadvise, dunadvise, (DWORD dwConnection));
    NV_DECLARE_TEAROFF_METHOD(EnumDAdvise, enumdadvise, (LPENUMSTATDATA * ppenumAdvise));

    //  IOleControl methods

    // over-ridden in CForm, various controls:
    STDMETHOD(GetControlInfo)(CONTROLINFO * pCI);
    STDMETHOD(OnMnemonic)(LPMSG pMsg);
    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid);
    // over-ridden in CForm:
    STDMETHOD(FreezeEvents)(BOOL fFreeze);
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE getcontrolinfo(CServer *pObj, CONTROLINFO * pCI)
    { return pObj->GetControlInfo(pCI); }
    static HRESULT STDMETHODCALLTYPE onmnemonic(CServer *pObj, LPMSG pMsg)
    { return pObj->OnMnemonic(pMsg); }
    static HRESULT STDMETHODCALLTYPE onambientpropertychange(CServer *pObj, DISPID dispid)
    { return pObj->OnAmbientPropertyChange(dispid); }
    // over-ridden in CForm:
    static HRESULT STDMETHODCALLTYPE freezeevents(CServer *pObj, BOOL fFreeze)
    { return pObj->FreezeEvents(fFreeze); }
#endif // ndef WIN16

    //  IOleObject interface methods

    // over-ridden in CForm, CWrappedControl:
    STDMETHOD(SetClientSite)(IOleClientSite * pClientSite);
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE setclientsite(CServer *pObj, IOleClientSite * pClientSite)
    { return pObj->SetClientSite(pClientSite); }
#endif // ndef WIN16
    NV_DECLARE_TEAROFF_METHOD(GetClientSite, getclientsite, (IOleClientSite * * ppClientSite));
    // over-ridden in CForm:
    STDMETHOD(SetHostNames)(LPCTSTR szContainerApp, LPCTSTR szContainerObj);
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE sethostnames(CServer *pObj, LPCTSTR szContainerApp, LPCTSTR szContainerObj)
    { return pObj->SetHostNames(szContainerApp, szContainerObj); }
    static HRESULT STDMETHODCALLTYPE close(CServer *pObj, DWORD dwSaveOption)
    { return pObj->Close(dwSaveOption); }
#endif // ndef WIN16
    // over-ridden in CForm and CMdcList:
    STDMETHOD(Close)(DWORD dwSaveOption);
    NV_DECLARE_TEAROFF_METHOD(SetMoniker, setmoniker, (DWORD dwWhichMoniker, LPMONIKER pmk));
    NV_DECLARE_TEAROFF_METHOD(GetMoniker, getmoniker, (
            DWORD dwAssign,
            DWORD dwWhichMoniker,
            LPMONIKER * ppmk));
    NV_DECLARE_TEAROFF_METHOD(InitFromData, initfromdata, (
            LPDATAOBJECT pDataObject,
            BOOL fCreation,
            DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD(GetClipboardData, getclipboarddata, (
            DWORD dwReserved,
            LPDATAOBJECT * ppDataObject));
    NV_DECLARE_TEAROFF_METHOD(DoVerb, doverb, (
            LONG iVerb,
            LPMSG lpmsg,
            IOleClientSite * pActiveSite,
            LONG lindex,
            HWND hwndParent,
            LPCOLERECT lprcPosRect));
    NV_DECLARE_TEAROFF_METHOD(EnumVerbs, enumverbs, (LPENUMOLEVERB * ppenumOleVerb));
    // over-ridden in CForm:
    STDMETHOD(Update)();
    STDMETHOD(IsUpToDate)();
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE update(CServer *pObj)
    { return pObj->Update(); }
    static HRESULT STDMETHODCALLTYPE isuptodate(CServer *pObj)
    { return pObj->IsUpToDate(); }
    static HRESULT STDMETHODCALLTYPE setextent(CServer *pObj, DWORD dwDrawAspect, LPSIZEL lpsizel)
    { return pObj->SetExtent(dwDrawAspect, lpsizel); }
    static HRESULT STDMETHODCALLTYPE getextent(CServer *pObj, DWORD dwDrawAspect, LPSIZEL lpsizel)
    { return pObj->GetExtent(dwDrawAspect, lpsizel); }
#endif // ndef WIN16

    DECLARE_TEAROFF_METHOD(GetUserClassID, getuserclassid, (CLSID * pClsid));
    NV_DECLARE_TEAROFF_METHOD(GetUserType, getusertype, (DWORD dwFormOfType, LPTSTR * pszUserType));
    // over-ridden in CForm, CScrollBar, CPageSelector, CMdcList:
    STDMETHOD(SetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);

    // over-ridden in CTxtBox (wrapped textbox):
    STDMETHOD(GetExtent)(DWORD dwDrawAspect, LPSIZEL lpsizel);

    NV_DECLARE_TEAROFF_METHOD(Advise, advise, (IAdviseSink * pAdvSink, DWORD * pdwConnection));
    NV_DECLARE_TEAROFF_METHOD(Unadvise, unadvise, (DWORD dwConnection));
    NV_DECLARE_TEAROFF_METHOD(EnumAdvise, enumadvise, (LPENUMSTATDATA * ppenumAdvise));
    NV_DECLARE_TEAROFF_METHOD(GetMiscStatus, getmiscstatus, (DWORD dwAspect, DWORD * pdwStatus));
    NV_DECLARE_TEAROFF_METHOD(SetColorScheme, setcolorscheme, (LPLOGPALETTE lpLogpal));

    //  IViewObject methods (NOTE - this is NOT a tear-off itf, so the methods
    //  should be virtual.)

    //
    // Override the other Draw method - the one that takes a DrawInfo, not
    // this one.
    //
    STDMETHOD(Draw)(
            DWORD dwDrawAspect,
            LONG lindex,
            void * pvAspect,
            DVTARGETDEVICE * ptd,
            HDC hicTargetDev,
            HDC hdcDraw,
            LPCRECTL lprcBounds,
            LPCRECTL lprcWBounds,
            BOOL (CALLBACK * pfnContinue) (ULONG_PTR),
            ULONG_PTR dwContinue);
    STDMETHOD(GetColorSet)(
            DWORD dwDrawAspect,
            LONG lindex,
            void * pvAspect,
            DVTARGETDEVICE * ptd,
            HDC hicTargetDev,
            LPLOGPALETTE * ppColorSet);
    STDMETHOD(Freeze)(
            DWORD dwDrawAspect,
            LONG lindex,
            void * pvAspect,
            DWORD * pdwFreeze);
    STDMETHOD(Unfreeze)(DWORD dwFreeze);
    STDMETHOD(SetAdvise)(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
    STDMETHOD(GetAdvise)(
            DWORD * pAspects,
            DWORD * pAdvf,
            LPADVISESINK * ppAdvSink);

    //  IViewObject2 methods

    STDMETHOD(GetExtent)(
            DWORD dwDrawAspect,
            LONG lindex,
            DVTARGETDEVICE* ptd,
            LPSIZEL lpsizel);

    //  IViewObjectEx methods

    STDMETHOD(GetRect)(DWORD dwAspect, LPRECTL pRect);
    STDMETHOD(GetViewStatus)(DWORD *pdwStatus);
    STDMETHOD(QueryHitPoint)(DWORD dwAspect, LPCRECT pRectBounds,
                             POINT ptLoc, LONG lCloseHint, DWORD *pHitResult);
    STDMETHOD(QueryHitRect)(DWORD dwAspect, LPCRECT pRectBounds,
                            LPCRECT prcLoc, LONG lCloseHint, DWORD *pHitResult);
    STDMETHOD(GetNaturalExtent)(DWORD dwAspect, LONG lindex, DVTARGETDEVICE * ptd, HDC hicTargetDev, DVEXTENTINFO * pExtentInfo, LPSIZEL psizel);

    //  IOleDocument methods

    NV_DECLARE_TEAROFF_METHOD(CreateView, createview,  (
            IOleInPlaceSite * pIPSite,
            IStream * pStm,
            DWORD dwReserved,
            IOleDocumentView ** ppView));
    NV_DECLARE_TEAROFF_METHOD(GetDocMiscStatus, getdocmiscstatus,  (DWORD * pdwStatus));
    NV_DECLARE_TEAROFF_METHOD(EnumViews, enumviews,  (IEnumOleDocumentViews ** ppEnumViews, IOleDocumentView ** ppView));

    //   IQuickActivate methods

    // over-ridden by CForm, CWrappedControl, CCommandButton, CCmdBtn:
    STDMETHOD(QuickActivate)(
            QACONTAINER *pqacontainer,
            QACONTROL *pqacontrol);
    NV_DECLARE_TEAROFF_METHOD(SetContentExtent, setcontentextent, (LPSIZEL lpsizel));
    NV_DECLARE_TEAROFF_METHOD(GetContentExtent, getcontentextent, (LPSIZEL lpsizel));

    //  IOleDocumentView methods 

    NV_DECLARE_TEAROFF_METHOD( SetInPlaceSite , setinplacesite ,  (IOleInPlaceSite * pIPSite));
    NV_DECLARE_TEAROFF_METHOD( GetInPlaceSite , getinplacesite ,  (IOleInPlaceSite ** ppIPSite));
    NV_DECLARE_TEAROFF_METHOD( GetDocument , getdocument , (IUnknown ** ppUnk));
    NV_DECLARE_TEAROFF_METHOD( SetRect , setrect , (LPRECT lprcView));
    NV_DECLARE_TEAROFF_METHOD( GetRect , getrect , (LPRECT lprcView));
    NV_DECLARE_TEAROFF_METHOD( SetRectComplex , setrectcomplex , (
                          LPRECT lprcView,
                          LPRECT lprcHScroll,
                          LPRECT lprcVScroll,
                          LPRECT lprcSizeBox));
    NV_DECLARE_TEAROFF_METHOD( Show , show , (BOOL fShow));
    NV_DECLARE_TEAROFF_METHOD( UIActivate , uiactivate , (BOOL fActivate));
    NV_DECLARE_TEAROFF_METHOD( Open , open , ());
    NV_DECLARE_TEAROFF_METHOD( CloseView , closeview , (DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD( SaveViewState , saveviewstate , (IStream * pStm));
    NV_DECLARE_TEAROFF_METHOD( ApplyViewState , applyviewstate , (IStream * pStm));
    NV_DECLARE_TEAROFF_METHOD( Clone , clone , (IOleInPlaceSite * pNewIPSite, IOleDocumentView ** ppNewView));

    //  IOleWindow methods 

    NV_DECLARE_TEAROFF_METHOD(GetWindow , getwindow , (HWND * lphwnd));
    NV_DECLARE_TEAROFF_METHOD(ContextSensitiveHelp , contextsensitivehelp , (BOOL fEnterMode));

    // IOleInPlaceObject methods

    NV_DECLARE_TEAROFF_METHOD( InPlaceDeactivate , inplacedeactivate , ());
    NV_DECLARE_TEAROFF_METHOD( UIDeactivate, uideactivate, ());
    STDMETHOD(SetObjectRects)(LPCOLERECT lprcPosRect, LPCOLERECT lprcClipRect);
    STDMETHOD(ReactivateAndUndo)();

#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE setobjectrects(CServer *pObj, LPCOLERECT lprcPosRect, LPCOLERECT lprcClipRect)
    { return pObj->SetObjectRects(lprcPosRect, lprcClipRect); }
    static HRESULT STDMETHODCALLTYPE reactivateandundo(CServer *pObj)
    { return pObj->ReactivateAndUndo(); }
#endif // ndef WIN16

    //  IOleInPlaceObjectWindowless methods 

    STDMETHOD(OnWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
    NV_DECLARE_TEAROFF_METHOD( GetDropTarget, getdroptarget, (IDropTarget **ppDropTarget));
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE onwindowmessage(CServer *pObj, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
    { return pObj->OnWindowMessage(msg, wParam, lParam, plResult); }
#endif // ndef WIN16

    //  IOleInPlaceActiveObject 

    STDMETHOD( TranslateAccelerator )(LPMSG lpmsg);
    STDMETHOD( OnFrameWindowActivate )(BOOL fActivate);
    STDMETHOD( OnDocWindowActivate )(BOOL fActivate);
    STDMETHOD(ResizeBorder)(LPCOLERECT prcBorder, LPOLEINPLACEUIWINDOW pUIWindow, BOOL fFrameWindow);
    STDMETHOD(EnableModeless)(BOOL fEnable);
#ifdef WIN16
    static HRESULT STDMETHODCALLTYPE translateaccelerator(CServer *pObj, LPMSG lpmsg)
    { return pObj->TranslateAccelerator(lpmsg); }
    static HRESULT STDMETHODCALLTYPE onframewindowactivate(CServer *pObj, BOOL fActivate)
    { return pObj->OnFrameWindowActivate(fActivate); }
    static HRESULT STDMETHODCALLTYPE ondocwindowactivate(CServer *pObj, BOOL fActivate)
    { return pObj->OnDocWindowActivate(fActivate); }
    static HRESULT STDMETHODCALLTYPE resizeborder(CServer *pObj, LPCOLERECT prcBorder, LPOLEINPLACEUIWINDOW pUIWindow, BOOL fFrameWindow)
    { return pObj->ResizeBorder(prcBorder, pUIWindow, fFrameWindow); }
    static HRESULT STDMETHODCALLTYPE enablemodeless(CServer *pObj, BOOL fEnable)
    { return pObj->EnableModeless(fEnable); }
#endif // ndef WIN16

    // IOleCache methods

    NV_DECLARE_TEAROFF_METHOD(Cache, cache, (FORMATETC *pformatetc, DWORD advf, DWORD *pdwConnection));
    NV_DECLARE_TEAROFF_METHOD(Uncache, uncache, (DWORD dwConnection));
    NV_DECLARE_TEAROFF_METHOD(EnumCache, enumcache, (IEnumSTATDATA **ppenumSTATDATA));
    NV_DECLARE_TEAROFF_METHOD(InitCache, initcache, (IDataObject *pDataObject));
    // SetData renamed to SetDataCache to avoid conflict IDataObject::SetData.
    NV_DECLARE_TEAROFF_METHOD(SetDataCache, setdatacache, (FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease));

    // IOleCache2 methods

    NV_DECLARE_TEAROFF_METHOD(UpdateCache, updatecache, (LPDATAOBJECT pDataObject, DWORD grfUpdf, LPVOID pReserved));
    NV_DECLARE_TEAROFF_METHOD(DiscardCache, discardcache, (DWORD dwDiscardOptions));

    // IDropTarget methods (delegated to by CDropTarget object)
    // need to be virtual, but don't need to be STDMETHODCALLTYPE;
    //  a pain to change.

    STDMETHOD   (DragEnter) (IDataObject * pIDataSource, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD   (DragOver)(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
    STDMETHOD   (DragLeave) (BOOL fDrop);
    STDMETHOD   (Drop) (IDataObject * pIDataSource, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

    // IPointerInactive methods

    NV_DECLARE_TEAROFF_METHOD(GetActivationPolicy, getactivationpolicy, (DWORD * pdwPolicy));
    DECLARE_TEAROFF_METHOD(OnInactiveSetCursor, oninactivesetcursor, (LPCRECT pRectBounds,
                                   long x, long y, DWORD dwMouseMsg, BOOL fSetAlways));
    NV_DECLARE_TEAROFF_METHOD(OnInactiveMouseMove, oninactivemousemove, (LPCRECT pRectBounds,
                                   long x, long y, DWORD grfKeyState));

    // IPerPropertyBrowsing methods

    // over-ridden by CMdcButton, CMdcText, CMdcList:
    DECLARE_TEAROFF_METHOD(GetDisplayString, getdisplaystring, (DISPID dispid, BSTR * lpbstr));
    NV_DECLARE_TEAROFF_METHOD(MapPropertyToPage, mappropertytopage, (DISPID dispid, LPCLSID lpclsid));
    // over-ridden by CMorphDataControl
    DECLARE_TEAROFF_METHOD(GetPredefinedStrings, getpredefinedstrings, (DISPID dispid, CALPOLESTR * lpcaStringsOut, CADWORD FAR* lpcaCookiesOut));
    DECLARE_TEAROFF_METHOD(GetPredefinedValue, getpredefinedvalue, (DISPID dispid, DWORD dwCookie, VARIANT * lpvarOut));

    // CBase overrides

    DECLARE_PRIVATE_QI_FUNCS(CBase)
    ULONG SubAddRef()       { return CBase::SubAddRef(); }
    ULONG SubRelease()      { return CBase::SubRelease();}
    ULONG GetRefs()         { return CBase::GetRefs();   }
    ULONG GetObjectRefs()   { return CBase::GetObjectRefs(); }

    HRESULT FireEvent(
        DISPID dispidEvent, 
        DISPID dispidProp,
        IDispatch *pEventObject,
        BYTE * pbTypes, 
        ...);

    STDMETHOD(GetEnabled)(VARIANT_BOOL * pfEnabled);

    // Helper functions

    virtual HRESULT Draw(CDrawInfo * pDI, RECT *prc) { RRETURN(DV_E_DVASPECT); }

    virtual HRESULT ActivateInPlace(LPMSG lpmsg);
    virtual HRESULT DeactivateInPlace();
    virtual HRESULT ActivateUI(LPMSG lpmsg);
    virtual HRESULT DeactivateUI();

    //
    // Main entrypoints for installing/removing ui which will call
    // the virtual helpers below for specific parts of the ui.
    //

    HRESULT InstallUI(BOOL fSetFocus = TRUE);
    void    RemoveUI();

#ifndef NO_OLEUI
    virtual HRESULT InstallDocUI();
    virtual void    RemoveDocUI();

    virtual HRESULT InstallFrameUI();
    virtual void    RemoveFrameUI();
#endif // NO_OLEUI

    STDMETHOD(QueryService)(REFGUID, REFIID, void **);

    void    DrawHighContrastBackground(HDC hdc, const RECT * prc, COLORREF crBack);

    HRESULT ActivateView();
    void    PrepareActivationMessage(LPMSG, LPMSG);
    void    SendActivationMessage(LPMSG);

    LPTSTR  GetMonikerDisplayName(DWORD dwAssign);

    //  helper functions for IOleInPlaceSiteWindowless

    HRESULT GetDC(LPRECT prc, DWORD dwFlags, HDC * phDC);
    HRESULT ReleaseDC(HDC hDC);
    HRESULT InvalidateRect(LPCRECT prc, BOOL fErase);
    HRESULT InvalidateRgn(HRGN hrgn, BOOL fErase);
    HRESULT Scroll(int dx, int dy, LPCRECT prcScroll, LPCRECT prcClip);
    HRESULT AdjustRect(LPRECT prc);

    HRESULT SetCapture(BOOL fCaptured);     // Capture or release the mouse.
    BOOL    GetCapture();                   // Have we have captured the mouse?
    BOOL    RequestUIActivate();                // Will container let ctrl UI activate?
    HRESULT SetFocus(BOOL fFocus);          // Set focus to self or release.
    BOOL    GetFocus();                     // Helper function to determine if client site has focus

    //  Helper function for IPointerInactive

    HRESULT SetCursor();

    //  Window management

    HWND            GetHWND();
    virtual HRESULT AttachWin(HWND hWndParent, RECT * prc, HWND * phWnd);
    virtual void    DetachWin();

    //  UI Active Border implementation

    void            ShowUIActiveBorder(BOOL fShowBorder);
    void            OnNCPaint();
    BOOL            OnNCSetCursor(HWND hwnd, int nHitTest, UINT msg);
    LONG            OnNCHitTest(POINTS);
    BOOL            OnNCLButtonDown(int, POINTS, RECT * prcCurrent = NULL);

    HRESULT         SetActiveObject();
    HRESULT         ClearActiveObject();

    // Window procedure

    virtual void    OnPaint();
    virtual BOOL    OnEraseBkgnd(HDC hdc);
    virtual void    OnDestroy();
    HRESULT         OnDefWindowMessage(UINT,WPARAM,LPARAM,LRESULT*);

    static  LRESULT CALLBACK WndProc(
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam);

    // Helper functions for translate accelerator

    virtual HRESULT DoTranslateAccelerator(LPMSG lpmsg);

    //  Inline access to internal state

    BOOL                TestLock(WORD wLockFlags);
    WORD                Unlock(WORD wLockFlags);
    void                Relock(WORD wLockFlags);

    OLE_SERVER_STATE    State();

    //  Constructors and destructors

    // CHROME
    // The CServer constuctor takes a flag which is true if the document
    // is being created via the new Chrome specific classid. For normal
    // MSHTML creations this flag will be false and the chrome specific
    // code will be disabled.
                    CServer(IUnknown *pUnkOuter, BOOL fChrome = FALSE);
    virtual         ~CServer();
    virtual void    Passivate();

    IUnknown * PunkOuter() { return _pUnkOuter; }
    BOOL IsAggregated(void) { return _pUnkOuter != PunkInner(); }

    //  Virtual methods implementing the state transitions

    virtual HRESULT PassiveToLoaded();

    virtual HRESULT LoadedToRunning();
    virtual HRESULT RunningToLoaded();

    virtual HRESULT RunningToInPlace(LPMSG lpmsg);
    virtual HRESULT InPlaceToRunning();

    virtual HRESULT InPlaceToUIActive(LPMSG lpmsg);
    virtual HRESULT UIActiveToInPlace();

    virtual HRESULT EnsureInPlaceObject();
    HRESULT         EnsureCache();

    //  Persistence callbacks

    virtual HRESULT LoadFromStream(LPSTREAM pStrm);
    virtual HRESULT SaveToStream(LPSTREAM pStrm);
    virtual DWORD   GetStreamSizeMax();

    virtual HRESULT LoadFromStorage(LPSTORAGE pStg);
    virtual HRESULT SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad);

    virtual HRESULT LoadFromBag(LPPROPERTYBAG pBag, LPERRORLOG pErrLog);
    virtual HRESULT SaveToBag(LPPROPERTYBAG pBag, BOOL fSaveAllProperties);

    //  Helper methods for derived classes

    HRESULT         TransitionTo(
                            OLE_SERVER_STATE state,
                            LPMSG lpmsg = NULL);

    BOOL            GetAmbientBool(
                            DISPID dispid,
                            BOOL fDefaultValue);

    HRESULT         GetAmbientBstr(
                            DISPID dispid,
                            BSTR *pbstr);

    HRESULT         GetAmbientVariant(
                            DISPID dispid,
                            VARIANT * pVar);

    HPALETTE        GetAmbientPalette();

    virtual HPALETTE    GetPalette(HDC hdc = 0, BOOL *pfHtPal = NULL);

    void            OnDataChange(BOOL fInvalidateView = TRUE);
    void            OnViewChange(DWORD dwAspect = DVASPECT_CONTENT);
    void STDMETHODCALLTYPE SendOnDataChange(DWORD_PTR dwAdvf);
    void STDMETHODCALLTYPE SendOnViewChange(DWORD_PTR dwAspects);
    virtual HRESULT OnPropertyChange(DISPID dispidProperty, DWORD dwFlags);

    // Overwritten by CDoc::ShowMessage.
    virtual HRESULT __cdecl ShowMessage(                
                int   * pnResult,
                DWORD dwFlags,
                DWORD dwHelpContext,
                UINT  idsMessage, ...) = 0;

#ifndef NO_EDIT
    HRESULT         GetCanUndo(VARIANT_BOOL * pfCanUndo);
    HRESULT         GetCanRedo(VARIANT_BOOL * pfCanRedo);

    //  Undo helpers

    virtual IOleUndoManager * UndoManager(void) { return _pUndoMgr; }

    HRESULT CreateUndoManager(void);
#endif // NO_EDIT

#ifdef _MAC
    void EnsureMacScrollbars(HDC hdc);
#endif

    //  Protected data members

    IUnknown *          _pUnkOuter;         //  Outer unknown

    IOleClientSite *    _pClientSite;       //  Pointer to client site

    SIZEL               _sizel;             //  Our Extent

    LPADVISESINK        _pAdvSink;          //  Current view advise sink

    LPSTORAGE           _pStg;              //  Pointer to our storage

    CInPlace *          _pInPlace;          //  Pointer to our inplace object.
    DWORD               _dwAspects;         //  Aspects of interest
    DWORD               _dwAdvf;            //  Advise flags

    IPicture *          _pPictureMouse;     //  Custom mouse icon

    IOleAdviseHolder *  _pOleAdviseHolder;  //  for IOleObject advises
    IDataAdviseHolder * _pDataAdviseHolder; //  for IDataObject advises

    IOleCache2 *        _pCache;            // Pointer to default cache implementation.
    IPersistStorage *   _pPStgCache;        // Pointer to cache's IPStg implementation.
    IViewObject2 *      _pViewObjectCache;  // Pointer to cache's IViewObj2 impl.

#ifndef NO_EDIT
    IOleUndoManager *   _pUndoMgr;          //  Undo manager object
#endif // NO_EDIT

#ifdef _MAC
    HANDLE              _hVertScroll;       // Vertical scroll bar control
    HANDLE              _hHorzScroll;       // Horizontal scroll bar control
#endif

#ifdef WIN16
    THREAD_HANDLE       _dwThreadId;        // used for impersonation in wndproc
#endif

#ifdef OBJCNTCHK
    DWORD               _dwObjCnt;          // Cookie for verifying object count
#endif

    WORD                _cStrongRefs;       //  Number of strong refs
    WORD                _wLockFlags;        //  Used by CLock class.

    long                _lDirtyVersion;     //  Dirty version number -- 0 means clean

#ifdef WIN16
    OLE_SERVER_STATE    _state;
#else
    OLE_SERVER_STATE    _state:4;           //  Current OLE state
#endif

    unsigned            _fViewFrozen:1;     //  is IViewObject frozen?
    unsigned            _fNoScribble:1;     //  between save and save completed?
    unsigned            _fSameAsLoad:1;     //  IPersistStorage::Save - fSameAsLoad?
    unsigned            _fInitNewed:1;      //  were we InitNew'd (rather
                                            //    than Load'd)?
    unsigned            _fHandsOff:1;       //  are we in Hands-Off mode?
    unsigned            _fHidden:1;         //  Set by DoVerb(OLEVERB_HIDE)
    unsigned            _fWindowless:1;     //  can we activate windowlessly?
    // CHROME
    unsigned            _fChrome:1;         //  TRUE if we are hosted by Chrome
                                            //  needing the CHROME windowless mshtml code
    unsigned            _fMsoViewExists:1;  //  allow only one view
    unsigned            _fMsoDocMode:1;     //  True if our container is providing
                                            //    a MsoDocument Site.
    unsigned            _fUseAdviseSinkEx:1; // do we use IAdviseSinkEx?
    unsigned            _fUserMode:1;       // True if user mode.

    // Flags added here, but used only in derived classes.  There are here so that we can use
    // bits in this class, rather than having the derived class allocate another flags word.
    // Typically, these are in the Morphable Data Control (MDC).

    unsigned             _fUseHighlightColors:1; // Use system highlight colors. Ignore Back and ForeColor.
    unsigned             _fCurrentPositionUnknown:1; // _dlbCurrentPosition is not valid, even if null.
    unsigned             _fAlreadyFiredClick:1;      // Click fired for this row, unambiguously.

    // Some flags used for dragdrop.
    unsigned             _fDragEnter:1;         // Used to check whether any current dragdrop operation
                                                // was officially entered by us.
    unsigned            _fDataChangePosted:1;   // Prevent firing OnDataChange multiple times
    unsigned            _fViewChangePosted:1;   // Prevent firing OnViewChange multiple times

    static ATOM         s_atomWndClass;

    //+-----------------------------------------------------------------------
    //
    //  CServer::CLock
    //
    //------------------------------------------------------------------------

    class CLock
    {
    public:
        CLock(CServer *pServer, WORD wLockFlags);
        CLock(CServer *pServer);
        ~CLock();

    private:
        CServer *   _pServer;
        WORD        _wLockFlags;
    };

    //+-----------------------------------------------------------------------
    //
    //  CServer::CLASSDESC
    //
    //------------------------------------------------------------------------

    struct CLASSDESC
    {
        CBase::CLASSDESC _classdescBase;

        DWORD           _dwMiscStatus;      // reg db: \CLSID\<clsid>\MiscStatus
        DWORD           _dwViewStatus;      // for IViewObjectEx:GetViewStatus

        int             _cOleVerbTable;     // number of entries in the verb table
        LPOLEVERB       _pOleVerbTable;     // pointer to list of verbs available
        PFNDOVERB *     _pfnDoVerb;         // DoVerb function table

        int             _cGetFmtTable;      // number of entries in the table
        LPFORMATETC     _pGetFmtTable;      // format table for IDataObject::GetData[Here]
        LPFNGETDATA *   _pGetFuncs;         // GetData(Here) function table

        int             _cSetFmtTable;      // number of entries in the table
        LPFORMATETC     _pSetFmtTable;      // format table for IDataObject::SetData
        LPFNSETDATA *   _pSetFuncs;         // SetData function table

        int             _ibItfPrimary;      // offset of primary interface

        DISPID          _dispidRowset;      // dispid of IRowset property
        WORD            _wVFFlags;          // VAR/FUNC flag in typeinfo
        DISPID          _dispIDBind;        // dispID of default property
        unsigned long   _uGetBindIndex;     // VTBL index for get property
        unsigned long   _uPutBindIndex;     // VTBL index for put property
        VARTYPE         _vtBindType;        // default property automation type
        unsigned long   _uGetValueIndex;    // DISPID_VALUE get vtable
        unsigned long   _uPutValueIndex;    // DISPID_VALUE put vtable
        VARTYPE         _vtValueType;       // DISPID_VALUE automation type
        unsigned long   _uSetRowset;        // vtable index of the IRowset set prop

        DWORD           _sef;               // Flags indicating which common
                                            //  events should be fired.

        BOOL TestFlag(SERVERDESC_FLAG dw) const { return (_classdescBase._dwFlags & dw) != 0; }

    };

    const CLASSDESC *ServerDesc() const { return (const CLASSDESC *)BaseDesc(); }

    // GetServerDescFlags -- This virtual method returns flags describing the currently
    // desired behavior of the server. It may change dynamically (e.g. Morphable Data
    // control.) You should inspect flags by going through this method, rather than
    // looking at the bits in the CLASSDESC, unless you are absolutely certain that the
    // bits never change and you have a performance need.
    // BUGBUG97: CServer currently looks in the CLASSDESC for many bits. This should be
    // changed, or else morphing to the various button styles will remain broken.
    // AndrewL -- Feb 16, 1996

    virtual SERVERDESC_FLAG GetServerDescFlags()
                        {return (SERVERDESC_FLAG)ServerDesc()->_classdescBase._dwFlags;};
    BOOL            TestServerDescFlag(SERVERDESC_FLAG dw)
        {return (GetServerDescFlags() & dw) != 0; }

    // CHROME
public:
    // Returns true if MSHTML was instantiated by Chrome for windowless
    // operation.
    BOOL IsChromeHosted() const
#ifdef _MAC  // bugbug: fix this to set the _fChrome flag to true for mac so we can eliminate this #ifdef
    {  return true;  }
#else
    {  return _fChrome;  }
#endif
};


//+---------------------------------------------------------------------------
//
//  Class:      CInPlace (cinpl)
//
//  Purpose:    Encapsulates the in-memory data storage of an in-place or
//              UI active object.  All interface methods are forwarded to
//              equivalents on CServer.  Any member data used only when inplace
//              should be put on this object.
//
//----------------------------------------------------------------------------

class CInPlace
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CInPlace))

    CInPlace();
    virtual ~CInPlace();

    // Member Data

    IOleInPlaceSite *       _pInPlaceSite;  //  Pointer to in-place client site

    LPOLEINPLACEFRAME       _pFrame;        //  our in-place active frame
    LPOLEINPLACEUIWINDOW    _pDoc;          //  our in-place active document

    OLEINPLACEFRAMEINFO     _frameInfo;     //  Accelerator et al info
                                            //    from our container

    POINT                   _ptWnd;         //  Location of window in container
    RECT                    _rcPos;         //  Position
    RECT                    _rcClip;        //  Clip

    HWND                    _hwnd;          //  Our window

    unsigned                _fIPNoDraw:1;
    unsigned                _fIPPaintBkgnd:1;
    unsigned                _fIPOffScreen:1;
    unsigned                _fUseExtendedSite:1;//  use IOleInPlaceSiteEx
    unsigned                _fCSHelpMode:1;     //  are we in context-sensitive help mode?
    unsigned                _fChildActivating:1;//  is an embedded obj going UIActive?
    unsigned                _fChildActive:1;    //  is an embedded object UI Active?
    unsigned                _fDeactivating:1;   //  are we deactivating?
    unsigned                _fWindowlessInplace:1;  //  are we windowless & inplace-active
    unsigned                _fShowBorder:1;     //  show UI Active border?
    unsigned                _fUIDown:1;         //  Are menu/tools integrated with frame?
    unsigned                _fFrameActive:1;    //  Is Frame window active?
    unsigned                _fDocActive:1;      //  Is Doc window active?
    unsigned                _fAfterDoubleClick:1;// For proper handling of dbl click events
    unsigned                _fFocus:1;          // True if we have the focus.
    unsigned                _fUsingWindowRgn:1; // True if clipping with window region.
    unsigned                _fBubbleInsideOut:1;// True if bubbling was caused from an inside control
    unsigned                _fMenusMerged:1;    // True if menus have been merged

    COffScreenContext *     _pOSC;
    RECT                    _rcPaint;
    IDataObject *           _pDataObj;          // saved data obj from begin of drop.
};

//+---------------------------------------------------------------
//
//  Common verb tables for use in class descriptor.
//
//---------------------------------------------------------------

// Standard - ClassDescriptor initializes with this table.

#ifdef PRODUCT_RT
    #define C_OLEVERB_STANDARD 5
#else
    #define C_OLEVERB_STANDARD 7
#endif

extern OLEVERB               g_aOleVerbStandard[C_OLEVERB_STANDARD];
extern CServer::PFNDOVERB    g_apfnDoVerbStandard[C_OLEVERB_STANDARD];

// NoEdit - Use for objects that don't support editing.

#ifdef PRODUCT_RT
    #define C_OLEVERB_NOEDIT 4
#else
    #define C_OLEVERB_NOEDIT 6
#endif

extern OLEVERB               g_aOleVerbNoEdit[C_OLEVERB_NOEDIT];
extern CServer::PFNDOVERB    g_apfnDoVerbNoEdit[C_OLEVERB_NOEDIT];

// EditDesign - Use for objects that support editing in design mode only.

#ifdef PRODUCT_RT
    #define C_OLEVERB_DESIGNEDIT 6
#else
    #define C_OLEVERB_DESIGNEDIT 8
#endif

extern OLEVERB               g_aOleVerbDesignEdit[C_OLEVERB_DESIGNEDIT];
extern CServer::PFNDOVERB    g_apfnDoVerbDesignEdit[C_OLEVERB_DESIGNEDIT];

//+---------------------------------------------------------------
//
//  Standard formats implemented by CServer
//
//---------------------------------------------------------------
#if !defined(_MAC) && !defined(WINCE)
#define STANDARD_FMTETCGET\
    { CF_COMMON(ICF_EMBEDDEDOBJECT),    NULL, DVASPECT_CONTENT, -1L, TYMED_ISTORAGE },\
    { CF_COMMON(ICF_EMBEDDEDOBJECT),    NULL, DVASPECT_ICON,    -1L, TYMED_ISTORAGE },\
    { CF_METAFILEPICT,                  NULL, DVASPECT_CONTENT, -1L, TYMED_MFPICT },\
    { CF_METAFILEPICT,                  NULL, DVASPECT_ICON,    -1L, TYMED_MFPICT },\
    { CF_COMMON(ICF_OBJECTDESCRIPTOR),  NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },\
    { CF_COMMON(ICF_OBJECTDESCRIPTOR),  NULL, DVASPECT_ICON,    -1L, TYMED_HGLOBAL },\
//    { CF_ENHMETAFILE,                   NULL, DVASPECT_CONTENT, -1L, TYMED_ENHMF },

#define STANDARD_PFNGETDATA\
    &CServer::GetEMBEDDEDOBJECT,\
    &CServer::GetEMBEDDEDOBJECT,\
    &CServer::GetMETAFILEPICT,\
    &CServer::GetMETAFILEPICT,\
    &CServer::GetOBJECTDESCRIPTOR,\
    &CServer::GetOBJECTDESCRIPTOR,\
//    &CServer::GetENHMETAFILE,

#define C_FORMATETC_GETSTANDARD 7
#else // !_MAC && !WINCE
// Mac Review: Mac OLE does not support CF_METAFILEPICT; can we use 'PICT'?
#define STANDARD_FMTETCGET\
    { CF_COMMON(ICF_EMBEDDEDOBJECT),    NULL, DVASPECT_CONTENT, -1L, TYMED_ISTORAGE },\
    { CF_COMMON(ICF_EMBEDDEDOBJECT),    NULL, DVASPECT_ICON,    -1L, TYMED_ISTORAGE },\
    { CF_COMMON(ICF_OBJECTDESCRIPTOR),  NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },\
    { CF_COMMON(ICF_OBJECTDESCRIPTOR),  NULL, DVASPECT_ICON,    -1L, TYMED_HGLOBAL },\

#define STANDARD_PFNGETDATA\
    &CServer::GetEMBEDDEDOBJECT,\
    &CServer::GetEMBEDDEDOBJECT,\
    &CServer::GetOBJECTDESCRIPTOR,\
    &CServer::GetOBJECTDESCRIPTOR,\

#define C_FORMATETC_GETSTANDARD 4
#endif  // !_MAC && !WINCE


#define MAX_USERTYPE_LEN 45


inline OLE_SERVER_STATE
CServer::State()
{
    return _state;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::TestLock
//
//  Synopsis:   Returns true if given resource is locked.
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

inline BOOL
CServer::TestLock(WORD wLockFlags)
{
    return wLockFlags & _wLockFlags;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::Unlock
//
//  Synopsis:   Unlock resource temporarily.
//
//  Returns:    BOOL
//
//-------------------------------------------------------------------------

inline WORD
CServer::Unlock(WORD wLockFlags)
{
    WORD wLockFlagsOld;

    wLockFlagsOld = _wLockFlags;
    _wLockFlags &= ~wLockFlags;
    return wLockFlagsOld;
}

//+------------------------------------------------------------------------
//
//  Member:     CServer::Relock
//
//  Synopsis:   Relock resource that was temporarily unlocked.
//
//-------------------------------------------------------------------------

inline void
CServer::Relock(WORD wLockFlags)
{
    _wLockFlags = wLockFlags;
}

ExternTag(tagCServer);

inline DISPID  CAttrValue::GetDISPID ( void ) const
{
    return IsPropdesc() ? u1._pPropertyDesc->GetDispid() : u1._dispid;
} 


inline const PROPERTYDESC * CAttrValue::GetPropDesc() const
{
    return IsPropdesc() ? u1._pPropertyDesc : NULL;
}


//
// These have to be here because they depend on the definition of CBase.
// They are used by WriteProps and ReadProps in the PROP_DESC structure.
//
#ifdef WIN16
typedef HRESULT (* pfnPropDescGet)(CBase *, ULONG *plVal);
typedef HRESULT (* pfnPropDescSet)(CBase *, ULONG lVal);
#else
typedef HRESULT (CBase::* pfnPropDescGet)(ULONG *plVal);
typedef HRESULT (CBase::* pfnPropDescSet)(ULONG lVal);
#endif

#ifdef _WIN64
//$ WIN64: Why is this structure sensitive to packing?
#pragma warning(disable:4121) /* alignment of a member was sensitive to packing */
#endif

struct PROP_DESC_GETSET
{
    pfnPropDescGet pfnGet;
    pfnPropDescSet pfnSet;
    const IID     *piid;    // Used only for WPI_UNKNOWN
};

#ifdef _WIN64
#pragma warning(default:4121)
#endif

//
// CColorValue related structure/functions/definitions
//

struct COLORVALUE_PAIR
{
    const TCHAR * szName;
    DWORD dwValue;
};

const struct COLORVALUE_PAIR * FindColorByValue( DWORD dwValue );
const struct COLORVALUE_PAIR * FindColorByColor( DWORD lColor );
const struct COLORVALUE_PAIR * FindColorByName( const TCHAR * szString );

class CColorValue : public CVoid
{
public:
    DWORD _dwValue;

    //
    // What a royal mess
    //
    // Some types are directly extractable simply by masking with MASK_FLAG.  These are:
    //
    //      TYPE_UNDEF          0xff
    //      TYPE_POUND1         0xfe
    //      TYPE_POUND2         0xfd
    //      TYPE_POUND3         0xfc
    //      TYPE_POUND4         0xfb
    //      TYPE_POUND5         0xfa
    //      TYPE_RGB            0xf9
    //      TYPE_TRANSPARENT    0xf8
    //      TYPE_UNIXSYSCOL     0x04
    //      TYPE_POUND6         0x00
    //
    // Some types pack some extra info (the color index) into the high byte (yum).  These are:
    //
    //      TYPE_SYSNAME    0xA0 - 0xBF
    //      TYPE_SYSINDEX   0xC0 - 0xDF
    //      TYPE_NAME       0x11 - 0x9C
    //
    // Believe it or not, there is actually room for more stuff the range 0xE0 - F8 is apparently not used!
    //
    // This scheme will blow when the number of named colors exceeds 143 or when the number of system colors
    // exceeds 31
    //
    // At the cost of an extra indirection, named colors could be handled is a much simpler manner.  A single
    // flag value for each type of named color and then the lower 3 bytes are the index into the table.
    //
    // 

    enum TYPE { TYPE_UNDEF     = 0xFF000000,    // color is undefined
                TYPE_POUND1    = 0xFE000000,
                TYPE_POUND2    = 0xFD000000,
                TYPE_POUND3    = 0xFC000000,
                TYPE_POUND4    = 0xFB000000,
                TYPE_POUND5    = 0xFA000000,
                TYPE_RGB       = 0xF9000000,    // color is rgb(r,g,b) or rgb(r%,g%,b%) (or mix)
                TYPE_TRANSPARENT = 0xF8000000,  // color is "transparent"
                TYPE_UNIXSYSCOL  = 0x04000000,  // color is unix system colormap entry index
                TYPE_NAME      = 0x11000000,    // color is named
                TYPE_SYSINDEX  = 0xC0000000,    // color is a system color (not named)
                TYPE_SYSNAME   = 0xA0000000,    // color is a named system color needs 29 in high byte
                TYPE_POUND6    = 0x00000000     // color is #rrggbb
    };

    enum MASK { MASK_COLOR      = 0x00ffffff,    // mask to extract color
                MASK_SYSCOLOR   = 0x1f000000,    // mask to extract the system color
#ifdef UNIX
                MASK_UNIX_COLOR = 0x04ffffff,
#endif
                MASK_FLAG       = 0xff000000 };  // mask to extract type

    #define VALUE_UNDEF 0xFFFFFFFF

    int Index(const DWORD dwValue) const;

#ifdef UNIX
    BOOL IsMotifColor() const { return (((_dwValue & 0xF0000000) != 0xF0000000) && (_dwValue & 0x04000000)); }
#endif

    static HRESULT FormatAsPound6Str(BSTR *pszColor, DWORD dwColor);
    HRESULT FormatBuffer ( LPTSTR szBuffer,
                           UINT uMaxLen,
                           const PROPERTYDESC *pPropDesc,
                           BOOL fReturnAsHex = FALSE) const;

    CColorValue() { _dwValue = VALUE_UNDEF; }
    CColorValue(DWORD dwValue) { _dwValue = dwValue; }
    CColorValue(VARIANT *);

    // NB: by setting fLookupName to TRUE, we will preferentially
    // map to color names, not numbers.

    long SetValue( long lColor, BOOL fLookupName, TYPE type);
    long SetValue( long lColor, BOOL fLookupName ){return SetValue(lColor, fLookupName, TYPE_POUND6);}
    long SetValue( const struct COLORVALUE_PAIR * pColorName );
    long SetRawValue( DWORD dwValue );
    long SetFromRGB(DWORD dwRGB) { return SetRawValue(((dwRGB & 0xffL) << 16) | (dwRGB & 0xff00L) | ((dwRGB & 0xff0000L) >> 16)); }
    long SetSysColor(int nIndex);

    OLE_COLOR GetOleColor() const;
    COLORREF  GetColorRef() const;
    DWORD GetIntoRGB(void) const;
    TYPE GetType() const;
#ifdef UNIX
    BOOL IsUnixSysColor() const { return ((_dwValue & MASK_FLAG) == TYPE_UNIXSYSCOL); }
#endif
    BOOL IsSysColor() const { return ((_dwValue & MASK_FLAG) >= TYPE_SYSNAME && (_dwValue & MASK_FLAG) < TYPE_TRANSPARENT); }
    long GetRawValue() const { return _dwValue; }
    
    HRESULT FromString( LPCTSTR pch, BOOL fValidOnly = FALSE , int iStrLen = -1);
    HRESULT RgbColor( LPCTSTR pch, int iStrLen);
    HRESULT HexColor( LPCTSTR pch, int iStrLen, BOOL fValidOnly );
    HRESULT NameColor( LPCTSTR pch );

    HRESULT Persist( IStream *pStream, const PROPERTYDESC *pPropDesc ) const;
    HRESULT IsValid() const;

    //
    // These are the 2 primary methods for interfacing
    //

    BOOL IsDefined ( void ) const
            { return ((VALUE_UNDEF != _dwValue)&&((_dwValue&MASK_FLAG)!=TYPE_TRANSPARENT));}
    BOOL IsNull ( void ) const
            { return VALUE_UNDEF == _dwValue; }
    void Undefine() { _dwValue = VALUE_UNDEF; }

    CColorValue& operator=(DWORD dwValue) { _dwValue = dwValue; return *this; }
    CColorValue(const CColorValue & other) { _dwValue = other._dwValue; }

};



#define ENUMFROMSTRING(enumname, pbstr, pEnumValue) s_enumdesc##enumname.EnumFromString ( (LPTSTR)pbstr, pEnumValue, FALSE )
#define ENUMFROMCASESENSITIVESTRING(enumname, pbstr, pEnumValue) s_enumdesc##enumname.EnumFromString ( (LPTSTR)pbstr, pEnumValue, TRUE )
#define STRINGFROMENUM(enumname, EnumValue, pbstr ) s_enumdesc##enumname.StringFromEnum ( EnumValue, pbstr )
#define ENUMSTRINGATINDEX(enumname, index) s_enumdesc##enumname.aenumpairs[index].pszName

#define STRINGPTRFROMENUM(enumname, EnumValue) s_enumdesc##enumname.StringPtrFromEnum ( EnumValue )

HRESULT LookupEnumString ( const NUMPROPPARAMS *ppp, LPCTSTR pstr, long *plNewValue );

//+-------------------------------------------------------------------------
//
//  HTML dialog
//
//--------------------------------------------------------------------------

// HTML dialog flags
#define HTMLDLG_NOUI        0x1     // run the dialog but don't show it
#define HTMLDLG_RESOURCEURL 0x2     // create the moniker for a resource
#define HTMLDLG_AUTOEXIT    0x4     // exit immediatly after running script
#define HTMLDLG_DONTTRUST   0x8     // don't trust the moniker passed in

STDAPI
ShowModalDialog(
    HWND        hwndParent,
    IMoniker *  pMk,
    VARIANT *   pvarArgIn,
    TCHAR *     pchOptions,
    VARIANT *   pvarArgOut);

STDAPI
ShowHTMLDialog(
    HWND        hwndParent,
    IMoniker *  pMk,
    VARIANT *   pvarArgIn,
    TCHAR *     pchOptions,
    VARIANT *   pvarArgOut);

STDAPI
ShowModelessHTMLDialog(
    HWND        hwndParent,
    IMoniker *  pMk,
    VARIANT *   pvarArgIn,
    VARIANT *   pvarOptions,
    IHTMLWindow2 **ppDialogWindow);

STDAPI
RunHTMLApplication(
    HINSTANCE hinst,
    HINSTANCE hPrevInst,
    LPSTR szCmdLine,
    int nCmdShow);

class COptionsHolder;

HRESULT
ShowModalDialogHelper(
    CDoc *      pDoc,
    TCHAR *     pchRID,
    IDispatch * pDisp,
    COptionsHolder* pcoh = NULL,
    VARIANT *   pvarArgOut = NULL,
    DWORD       dwFlags = HTMLDLG_RESOURCEURL);

class CFunctionPointer : public CBase
{
    DECLARE_CLASS_TYPES(CFunctionPointer, CBase)

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFunctionPointer))

    CFunctionPointer(CBase *pThis, DISPID dispid) : _pThis(pThis), _dispid(dispid)
      {  }
    ~CFunctionPointer()
      {  }

    #define _CFunctionPointer_
    #include "types.hdl"

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CFunctionPointer);

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);

    // IDispatchEx override.
    HRESULT STDMETHODCALLTYPE InvokeEx(DISPID id,
                                       LCID lcid,
                                       WORD wFlags,
                                       DISPPARAMS *pdp,
                                       VARIANT *pvarRes,
                                       EXCEPINFO *pei,
                                       IServiceProvider *pSrvProvider);

private:
    CBase      *_pThis;
    DISPID      _dispid;

    // Override from CBase, placed in private so no one calls this function.
    virtual const CBase::CLASSDESC *GetClassDesc() const
    {
        Assert(!"not implemented"); return 0;
    }
};


// Function used by shdocvw to do a case sensitive compare of a typelibrary.
STDAPI
MatchExactGetIDsOfNames(ITypeInfo *pTI,
                        REFIID riid,
                        LPOLESTR *rgszNames,
                        UINT cNames,
                        LCID lcid,
                        DISPID *rgdispid,
                        BOOL fCaseSensitive);


STDAPI CreateHTMLPropertyPage(       
        IMoniker *          pmk,
        IPropertyPage **    ppPP);

// Generic Parsing Helpers
TCHAR *NextParenthesizedToken( TCHAR *pszToken );

#pragma INCMSG("--- End 'cdbase.hxx'")
#else
#pragma INCMSG("*** Dup 'cdbase.hxx'")
#endif
