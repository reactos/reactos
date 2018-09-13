//+---------------------------------------------------------------------------
//
//  Microsoft IE
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       wintearoff.hxx
//
//  Contents:   
//
//----------------------------------------------------------------------------

#ifndef I_WINTEAROFF_HXX_
#define I_WINTEAROFF_HXX_
#pragma INCMSG("--- Beg 'wintoff.hxx'")

#ifndef DEBUG_TEAROFFS

// Ordinary tearoffs

#define DECLARE_CLASS_TYPES(klass, supa)\
    typedef supa super;\
    typedef klass thisclass;\
    typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFNTEAROFF)(void);

#define DECLARE_TEAROFF_METHOD(fn, FN, args)\
    STDMETHOD(fn)args

#define DECLARE_TEAROFF_METHOD_(ret, fn, FN, args)\
    STDMETHOD_(ret, fn)args

#define DECLARE_TEAROFF_TABLE(itf)\
    static HRESULT (STDMETHODCALLTYPE CVoid::*const s_apfn##itf[])(void);

#define DECLARE_TEAROFF_TABLE_PROPDESC(itf)\
    static HRESULT (STDMETHODCALLTYPE CVoid::*const s_apfnpd##itf[])(void);

#define DECLARE_TEAROFF_TABLE_NAMED(apfname)\
    static HRESULT (STDMETHODCALLTYPE CVoid::*apfname[])(void);

#define NV_DECLARE_TEAROFF_METHOD(fn, FN, args)\
    HRESULT STDMETHODCALLTYPE fn args

#define NV_DECLARE_TEAROFF_METHOD_(ret, fn, FN, args)\
    ret STDMETHODCALLTYPE fn args

#define BEGIN_TEAROFF_TABLE(klass, itf)\
    HRESULT (STDMETHODCALLTYPE  CVoid::*const klass::s_apfn##itf[])(void) = {\
    TEAROFF_METHOD(klass, PrivateQueryInterface, privatequeryinterface, (REFIID, void **))\
    TEAROFF_METHOD_(klass, PrivateAddRef, privateaddref, ULONG, ())\
    TEAROFF_METHOD_(klass, PrivateRelease, privaterelease, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_PROPDESC(klass, itf)\
    HRESULT (STDMETHODCALLTYPE  CVoid::*const klass::s_apfnpd##itf[])(void) = {\
    TEAROFF_METHOD(klass, PrivateQueryInterface, privatequeryinterface, (REFIID, void **))\
    TEAROFF_METHOD_(klass, PrivateAddRef, privateaddref, ULONG, ())\
    TEAROFF_METHOD_(klass, PrivateRelease, privaterelease, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_(klass, itf)\
    HRESULT (STDMETHODCALLTYPE  CVoid::*const klass::s_apfn##itf[])(void) = {\
    TEAROFF_METHOD(klass, QueryInterface, queryinterface, (REFIID, void **))\
    TEAROFF_METHOD_(klass, AddRef, addref, ULONG, ())\
    TEAROFF_METHOD_(klass, Release, release, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_SUB_(klass, subklass, itf)\
    HRESULT (STDMETHODCALLTYPE  CVoid::*const klass::subklass::s_apfn##itf[])(void) = {\
    TEAROFF_METHOD (klass::subklass, klass::subklass::QueryInterface, klass::subklass::queryinterface, (REFIID, void **))\
    TEAROFF_METHOD_(klass::subklass, klass::subklass::AddRef, klass::subklass::addref, ULONG, ())\
    TEAROFF_METHOD_(klass::subklass, klass::subklass::Release, klass::subklass::release, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_NAMED(klass, name)\
    HRESULT (STDMETHODCALLTYPE  CVoid::*klass::name[])(void) = {\

#define END_TEAROFF_TABLE()\
    };

#define _TEAROFF_METHOD(fn, FN, args)\
    (PFNTEAROFF)(HRESULT (STDMETHODCALLTYPE CVoid::*)args)fn,

#define _TEAROFF_METHOD_(fn, FN, ret, args)\
    (PFNTEAROFF)(ret (STDMETHODCALLTYPE CVoid::*)args)fn,

#define TEAROFF_METHOD(klass, fn, FN, args)\
    (PFNTEAROFF)(HRESULT (STDMETHODCALLTYPE CVoid::*)args)fn,

#define TEAROFF_METHOD_(klass, fn, FN, ret, args)\
    (PFNTEAROFF)(ret (STDMETHODCALLTYPE CVoid::*)args)fn,

#define TEAROFF_METHOD_SUB(klass, subklass, fn, FN, args)\
    TEAROFF_METHOD(klass::subklass, klass::subklass::fn, klass::subklass::FN, args)

#define TEAROFF_METHOD_NULL\
    (PFNTEAROFF)NULL,

#else // !DEBUG_TEAROFFS

// Debug version of tearoffs: check against multiple inheritance

struct DEBUG_TEAROFF_NOTE
{
    DEBUG_TEAROFF_NOTE *pnoteNext;
    void *apfn;
    char *pchDebug;
};

template <class KLASS>
union DEBUG_TEAROFF_METHOD
{
    HRESULT (STDMETHODCALLTYPE KLASS::*m)(void);

    struct
    {
        DWORD fn;
        DWORD off;
    } d;
};

class CTrivial : public CVoid
{
public:
    virtual void Break() { _asm { int 3 }}
};

int DeferDebugCheckTearoffTable(DEBUG_TEAROFF_NOTE *pnote, void *apfn, char *string);

#define DECLARE_CLASS_TYPES(klass, supa)\
    typedef supa super;\
    typedef klass thisclass;\
    typedef HRESULT (STDMETHODCALLTYPE thisclass::*PFNTEAROFF)(void); \

#define DECLARE_TEAROFF_TABLE(itf)\
    static DEBUG_TEAROFF_METHOD<thisclass> s_apfn##itf[];\
    static int s_iDebugCheck##itf;\
    static DEBUG_TEAROFF_NOTE s_noteDebugCheck##itf;\

#define DECLARE_TEAROFF_TABLE_NAMED(name)\
    static DEBUG_TEAROFF_METHOD<thisclass> name[];\
    static int s_iDebugCheck##name;\
    static DEBUG_TEAROFF_NOTE s_noteDebugCheck##name;\

#define BEGIN_TEAROFF_TABLE(klass, itf)\
    DEBUG_TEAROFF_NOTE klass::s_noteDebugCheck##itf = { 0 };\
    int klass::s_iDebugCheck##itf = DeferDebugCheckTearoffTable(\
        &klass::s_noteDebugCheck##itf, klass::s_apfn##itf, "Multiple inheritance problem with tearoff " #itf " in " #klass);\
    DEBUG_TEAROFF_METHOD<klass> klass::s_apfn##itf[] = {\
    TEAROFF_METHOD(PrivateQueryInterface, (REFIID, void **))\
    TEAROFF_METHOD_(PrivateAddRef, ULONG, ())\
    TEAROFF_METHOD_(PrivateRelease, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_(klass, itf)\
    DEBUG_TEAROFF_NOTE klass::s_noteDebugCheck##itf = { 0 };\
    int klass::s_iDebugCheck##itf = DeferDebugCheckTearoffTable(\
        &klass::s_noteDebugCheck##itf, klass::s_apfn##itf, "Multiple inheritance problem with tearoff " #itf " in " #klass);\
    DEBUG_TEAROFF_METHOD<klass> klass::s_apfn##itf[] = {\
    TEAROFF_METHOD(QueryInterface, (REFIID, void **))\
    TEAROFF_METHOD_(AddRef, ULONG, ())\
    TEAROFF_METHOD_(Release, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_NAMED(klass, name)\
    DEBUG_TEAROFF_NOTE klass::s_noteDebugCheck##name = { 0 };\
    int klass::s_iDebugCheck##name = DeferDebugCheckTearoffTable(\
        &klass::s_noteDebugCheck##name, klass::name, "Multiple inheritance problem with tearoff " #name " in " #klass);\
    DEBUG_TEAROFF_METHOD<klass> klass::name[] = {\

#define END_TEAROFF_TABLE()\
    NULL };\

#define TEAROFF_METHOD(fn, args)\
    { (PFNTEAROFF)(HRESULT (STDMETHODCALLTYPE thisclass::*)args)fn },\

#define TEAROFF_METHOD_(fn, ret, args)\
    { (PFNTEAROFF)(ret (STDMETHODCALLTYPE thisclass::*)args)fn },\

#define TEAROFF_METHOD_NULL\
    { (PFNTEAROFF)CTrivial::Break },\

#endif // DEBUG_TEAROFFS


//-------------------------------------------------------------------------
// This is the inline assembly to decode the eax register
// and put it into a variable.  This is only used in hdl files
// and the CBase::IDispatch implementation.
//-------------------------------------------------------------------------

#if !defined(WIN16) && !defined(UNIX) && defined(_M_IX86)
#define CONTEXTTHUNK_SETCONTEXT            \
{                                               \
    /* set pUnkContext to eax                */ \
    __asm  mov pUnkContext, eax                 \
}


#elif defined(_M_ALPHA)
#define CONTEXTTHUNK_SETCONTEXT            \
{                                               \
    pUnkContext = (IUnknown *)_GetTearoff();    \
}


#else // defined(WIN16) || defined(UNIX) || (!defined(_M_IX86) && !defined(_M_ALPHA))
#define CONTEXTTHUNK_SETCONTEXT            \
{                                               \
    pUnkContext = NULL;                         \
}

#endif // defined(WIN16) || defined(UNIX) || (!defined(_M_IX86) && !defined(_M_ALPHA))

#define CONTEXTTHUNK_SETTREENODE \
    IUnknown * pUnkContext;                                             \
    CONTEXTTHUNK_SETCONTEXT                                             \
    Assert(pUnkContext);                                                \
    pNode = NULL;                                                       \
    pUnkContext->QueryInterface(CLSID_CTreeNode, (void**)&pNode);

#pragma INCMSG("--- End 'wintoff.hxx'")
#else
#pragma INCMSG("*** Dup 'wintoff.hxx'")
#endif

