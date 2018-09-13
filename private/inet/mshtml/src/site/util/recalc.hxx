#ifndef I_RECALC_HXX_
#define I_RECALC_HXX_
#pragma INCMSG("--- Beg 'recalc.hxx'")

#ifndef X_RECALC_H_
#define X_RECALC_H_
#pragma INCMSG("--- Beg <recalc.h>")
#include <recalc.h>
#pragma INCMSG("--- End <recalc.h>")
#endif

#ifndef X_ACTIVAUT_H
#define X_ACTIVAUT_H
#pragma INCMSG("--- Beg <activaut.h>")
#include <activaut.h>
#pragma INCMSG("--- End <activaut.h>")
#endif

#ifndef X_ACTIVDBG_H
#define X_ACTIVDBG_H
#pragma INCMSG("--- Beg <activdbg.h>")
#include <activdbg.h>
#pragma INCMSG("--- End <activdbg.h>")
#endif

#if DBG == 1
#define DECLARE_SERIALNO_MEMBERS()  \
    static unsigned s_serialNumber; \
    unsigned _serialNumber;         \

#else
#define DECLARE_SERIALNO_MEMBERS()
#endif

ExternTag(tagRecalcInfo);
ExternTag(tagRecalcDetail);
ExternTag(tagRecalcEngine);

// A helper function to canonicalize properties
HRESULT GetCanonicalProperty(IUnknown *pUnk, DISPID dispid, IUnknown **ppUnkCanonical, DISPID *pdispidCanonical);

MtExtern(CRecalcEngine)
MtExtern(CRecalcObject)
MtExtern(CRecalcProperty)

class CRecalcEngine;
class CRecalcObject;
class CRecalcProperty;

DECLARE_CPtrAry(CRecalcPropertyArray, CRecalcProperty *, Mt(CRecalcEngine), Mt(CRecalcEngine))
DECLARE_CPtrAry(CSzArray, LPOLESTR, Mt(CRecalcEngine), Mt(CRecalcEngine))
DECLARE_CPtrAry(CRecalcObjectArray, CRecalcObject *, Mt(CRecalcEngine), Mt(CRecalcEngine))

#ifndef RECALC_USE_SCRIPTDEBUG
MtExtern(CScriptAuthorHolder)
class CScriptAuthorHolder;
DECLARE_CPtrAry(CScriptAuthorHolderArray, CScriptAuthorHolder *, Mt(CRecalcEngine), Mt(CRecalcEngine))

class CScriptAuthorHolder
{
public:
    DECLARE_SERIALNO_MEMBERS()
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CScriptAuthorHolder))

    CScriptAuthorHolder();
    ~CScriptAuthorHolder();
    HRESULT Init(LPOLESTR szLanguage);
    void Detach();

    BOOL Compare(LPOLESTR pszLanguage);
    IActiveScriptAuthor *ScriptAuthor() { return _pScriptAuthor; }
    LPOLESTR Language() { return _sLanguage; }
private:
    CStr _sLanguage;
    IActiveScriptAuthor *_pScriptAuthor;

};

#endif

#if DBG == 1
//
// Dumping helpers
//
#define r_p(sz)   RecalcDumpFormat(_T("<0s>"), sz)
#define r_pf(t)   RecalcDumpFormat t
#define r_pn(n)   RecalcDumpFormat(_T(#n) _T("=<0d> "), n)
#define r_pb(b)   RecalcDumpFormat(_T(#b) _T("=<0d> "), b)
#define r_pp(p)   RecalcDumpFormat(_T(#p) _T("=<0x> "), p)
#define r_ps(s)   RecalcDumpFormat(_T(#s) _T("='<0s>' "), (LPOLESTR)s)
#define r_pv(v)   RecalcDumpVariant(_T(#v), v)

void RecalcDumpVariant(LPOLESTR szName, VARIANT *pv);
extern HANDLE g_hfileRecalcDump;
void RecalcDumpFormat(LPOLESTR szFormat, ...);
#endif

class CRecalcProperty : public CVoid
{
public:
    DECLARE_SERIALNO_MEMBERS()
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecalcProperty))

    CRecalcProperty(CRecalcObject *pObject, DISPID dispid);
    ~CRecalcProperty();
    static HRESULT CreateProperty(CRecalcObject *pObject, DISPID dispid, CRecalcProperty **ppProperty);

    HRESULT SetExpression(LPCOLESTR szExpression, LPCOLESTR szLanguage);
    HRESULT GetExpression(BSTR *pstrExpression, BSTR *pstrLanguage);
    HRESULT ClearExpression();
    HRESULT Eval(BOOL fForce);
    HRESULT EvalExpression(VARIANT *pv);
    HRESULT RemoveAllDependencies();
    void OnChanged();
    BOOL HasNoDependents() { return (_dependents.Size() == 0); }
    BOOL HasNoAliases() { return (_aliases.Size() == 0); }
    void DeleteIfEmpty();

    HRESULT SetCanonicalProperty(CRecalcProperty *pProperty);
    BOOL IsCanonical() { return !_fAlias; }
    BOOL IsAlias() { return _fAlias; }
    BOOL IsStyleProp() { return _fStyleProp; }
    BOOL IsTopLevel() { return IsCanonical() && HasNoDependents(); }
    BOOL CheckAndClearStyleSet();
    DISPID GetDispid() { return _dispid; }
    LPOLESTR GetExpression() { return (LPOLESTR) _sExpression; }
    IUnknown *GetUnknown();

#if DBG == 1
    int Dump(DWORD dwFlags);
#endif

    HRESULT UpdateDependencies();

private:
    HRESULT ParseExpressionDependencies();

    HRESULT AddAlias(CRecalcProperty *pProperty);
    HRESULT RemoveAlias(CRecalcProperty *pProperty);

    HRESULT AddDependent(CRecalcProperty *pProperty, BOOL *pfNoNotify);
    HRESULT RemoveDependent(CRecalcProperty *pProperty);
    HRESULT AddDependency(IDispatch *pDispatch, DISPID dispid);

    HRESULT SetValue(VARIANT *pv, BOOL fForce);
    HRESULT clearExpressionHelper();

    void NotifyDependents();
    void SetDirty(BOOL fDirty, BOOL fRequestRecalc = TRUE);

    CRecalcEngine *GetEngine();
    IRecalcHost *GetHost();
#if DBG == 1
    IRecalcHostDebug *GetDebugHost();
#endif

    CRecalcObject *_pObject;
    DISPID _dispid;

    CVariant _vCurrent;
    CStr _sExpression;
    CStr _sExpressionNames;
    CStr _sLanguage;
    union
    {
        IDispatch *_pdispExpression;    // The "compiled" expression
        IDispatchEx *_pDEXExpression;   // Use the IDispatchEx when _pdispThis (below) is set
    };
    IDispatch *_pdispThis;              // The this pointer to use with the expression

    // Flags.  Some of these flags may overlap until I figure out just what is needed
    union
    {
        struct
        {
            DWORD _flags;
        };
        struct
        {
            unsigned _fDirty:1;         // The expression or a dependent has changed
            unsigned _fNoNotify:1;      // My object or one of my depdencies' object couldn't setup a notify sink
            unsigned _fUnresolved:1;    // Couldn't resolve a name in the expression to an IDispatch/dispid
            unsigned _fInEval:1;        // Catching circular references
            unsigned _fSetValue:1;      // Checking up on SetValue (to catch hidden circularity)
            unsigned _fCircular:1;      // Circular expression
            unsigned _fSetDirty:1;      // Checking up on SetDirty (to catch circular expressions)
            unsigned _fStyleProp:1;     // This expression is on a style
            unsigned _fStyleSet:1;      // This expression has been set (during BeginStyle/EndStyle)
            unsigned _fAlias:1;         // I am an alias
            unsigned _fDirtyDeps:1;     // Dependencies are dirty and need updating
            unsigned _fError:1;         // This expression has generated an error
        };
    };

    CSzArray _dependencyNames;
    union
    {
        CRecalcProperty *_pPropertyCanonical;   // I am an alias, this is my canonical property
        CRecalcProperty *_pPropertyExpression;  // I am not an alias, this is the property that has the expression
    };
    CRecalcPropertyArray _dependents;       // Properties that depend on this property
    CRecalcPropertyArray _dependencies;     // Properties that this property's expression depends on
    CRecalcPropertyArray _aliases;          // Properties that are an alias of this one
#if DBG == 1
    unsigned _cChanged;                     // Count how many OnChanged events are fired during a put
    BSTR _bstrName;                          // The property name
#endif
};

class CRecalcObject : public CVoid , public IPropertyNotifySink
{
public:
    DECLARE_SERIALNO_MEMBERS()
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecalcObject))

    CRecalcObject(CRecalcEngine *pEngine, IUnknown *pUnk);
    ~CRecalcObject();

    //IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPropertyNotifySink methods
    STDMETHOD(OnChanged)(DISPID dispid);
    STDMETHOD(OnRequestEdit)(DISPID dispid);

    HRESULT BeginStyle();
    HRESULT EndStyle();
    HRESULT FindProperty(DISPID dispid, BOOL fCreate, CRecalcProperty **ppProperty);
    void RemoveProperty(CRecalcProperty *pProperty);

    HRESULT RecalcAll(BOOL fForce);

    void Detach();
    IRecalcHost *GetHost();
#if DBG == 1
    IRecalcHostDebug *GetDebugHost();
    BSTR GetID() { return _bstrID; }
    BSTR GetTag() { return _bstrTag; }
    int Dump(DWORD dwFlags);
#endif
    CRecalcEngine *GetEngine() { return _pEngine; }

    IUnknown *GetUnknown() { return _pUnk; }

    BOOL GotSink() { return _fGotSink; }

    BOOL InStyle() { return _fDoingStyle; }

private:
    int Find(DISPID dispid);
    void RemovePropertyHelper(int iProperty);
    int _iFoundLast;                // A cache of the most recently found object
    DISPID _dispidFoundLast;        // so that we can be a bit lazier elsewhere
    CRecalcPropertyArray _properties;

    unsigned _ulRefs;
    IUnknown *_pUnk;
    CRecalcEngine *_pEngine;
    DWORD _dwCookie;
    union
    {
        struct
        {
            DWORD _flags;
        };
        struct
        {
            unsigned _fGotSink:1;       // We have successfully established a notify sink
            unsigned _fDoingStyle:1;    // Are we in the middle of setting style expressions?
            unsigned _fInRecalc:1;      // Are in the middle of doing recalc on this object?
        };
    };
#if DBG == 1
    BSTR _bstrID;                   // the id
    BSTR _bstrTag;                  // the tagName
#endif
};

class CRecalcEngine : public CVoid, public IRecalcEngine , public IObjectWithSite
{
public:
    DECLARE_SERIALNO_MEMBERS()
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecalcProperty))

    CRecalcEngine();
    ~CRecalcEngine();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IObjectWithSite methods

    STDMETHOD(SetSite)(IUnknown *pSite);
    STDMETHOD(GetSite)(REFIID iid, LPVOID *ppvSite);

    // IRecalcEngine methods
    STDMETHOD(RecalcAll)(BOOL fForce);
    STDMETHOD(OnNameSpaceChange)(IUnknown *pUnk) { return E_NOTIMPL; };
    STDMETHOD(SetExpression)(IUnknown *pObject, DISPID dispid, LPOLESTR szExpression, LPOLESTR szlanguage);
    STDMETHOD(GetExpression)(IUnknown *pObject, DISPID dispid, BSTR *pstrExpression, BSTR *pstrLanguage);
    STDMETHOD(ClearExpression)(IUnknown *pObject, DISPID dispid);

    STDMETHOD(BeginStyle)(IUnknown *pObject);
    STDMETHOD(EndStyle)(IUnknown *pObject);

    void RemoveObject(CRecalcObject *pObject);

    HRESULT FindObject(IUnknown *pUnk, BOOL fCreate, CRecalcObject **ppObject, unsigned *pIndex = 0);
    HRESULT FindProperty(IUnknown *pObject, DISPID dispid, BOOL fCreate, CRecalcProperty **ppProperty);

    HRESULT AddDependencyUpdateRequest(CRecalcProperty *pProperty);
    HRESULT RemoveDependencyUpdateRequest(CRecalcProperty *pProperty);

    BOOL InRecalc() { return _fInRecalc; }

#ifndef RECALC_USE_SCRIPTDEBUG
    HRESULT GetScriptAuthor(LPOLESTR szLanguage, IActiveScriptAuthor **ppScriptAuthor);
#endif

    IRecalcHost *GetHost() { return _pHost; }
#if DBG == 1
    IRecalcHostDebug *GetDebugHost() { return _pDebugHost; }

#define RECALC_DUMP_EVAL  0x00000000
#define RECALC_DUMP_PROPS 0x00000001

    int Dump(DWORD dwFlags);
#endif


private:
    int _iFoundLast;                // A cache of the most recently found object
    IUnknown *_pUnkFoundLast;       // so that we can be a bit lazier elsewhere

    int Find(IUnknown *pUnk);
    void Detach();

    IRecalcHost *_pHost;
#if DBG == 1
    IRecalcHostDebug *_pDebugHost;
#endif
    
    ULONG _ulRefs;

    CRecalcObjectArray _objects;
    CRecalcPropertyArray _dirtyDeps;

#ifndef RECALC_USE_SCRIPTDEBUG
    CScriptAuthorHolderArray _authors;
#endif

    // Flags
    unsigned _fRecalcRequested:1;    // Recalc pending
    unsigned _fInRecalc:1;           // We are actually doing recalc
    unsigned _fInDeps:1;             // We are updating dependencies
};

inline CRecalcEngine *CRecalcProperty::GetEngine()
{
    return _pObject->GetEngine();
}

inline IRecalcHost *CRecalcProperty::GetHost()
{
    return GetEngine()->GetHost();
}

inline IRecalcHost *CRecalcObject::GetHost()
{
    return _pEngine->GetHost();
}

inline IUnknown *CRecalcProperty::GetUnknown()
{
    return _pObject->GetUnknown();
}

#if DBG == 1
inline IRecalcHostDebug *CRecalcProperty::GetDebugHost()
{
    return GetEngine()->GetDebugHost();
}

inline IRecalcHostDebug *CRecalcObject::GetDebugHost()
{
    return GetEngine()->GetDebugHost();
}

#endif

#pragma INCMSG("--- End 'recalc.hxx'")
#else
#pragma INCMSG("*** Dup 'recalc.hxx'")
#endif
