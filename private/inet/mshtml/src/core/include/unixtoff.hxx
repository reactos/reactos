//+---------------------------------------------------------------------------
//
//  Microsoft IE Unix
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       unixtearoff.hxx
//
//  Contents:   
//
//----------------------------------------------------------------------------

#ifndef I_UNIXTOFF_HXX_
#define I_UNIXTOFF_HXX_
#pragma INCMSG("--- Beg 'unixoff.hxx'")

extern "C" void* _GetTearoff();

#define DECLARE_CLASS_TYPES(klass, supa)\
nested_cls_access: \
    typedef supa super;\
    typedef klass thisclass;\
    typedef HRESULT (STDMETHODCALLTYPE thisclass::*PFNTEAROFF)(void);

#define DECLARE_TEAROFF_METHOD(fn, FN, args)\
    STDMETHOD(fn)args

#define DECLARE_TEAROFF_METHOD_(ret, fn, FN, args)\
    STDMETHOD_(ret, fn)args

#define DECLARE_TEAROFF_TABLE(itf)\
    static PFNTEAROFF const s_apfn##itf[];

#define DECLARE_TEAROFF_TABLE_PROPDESC(itf)\
    static PFNTEAROFF const s_apfnpd##itf[];

#define DECLARE_TEAROFF_TABLE_NAMED(apfname)\
    static PFNTEAROFF const apfname[];

#define NV_DECLARE_TEAROFF_METHOD(fn, FN, args)\
    HRESULT STDMETHODCALLTYPE fn args

#define NV_DECLARE_TEAROFF_METHOD_(ret, fn, FN, args)\
    ret STDMETHODCALLTYPE fn args

#define BEGIN_TEAROFF_TABLE(klass, itf)                                                 \
    klass::PFNTEAROFF const klass::s_apfn##itf[] = {                                    \
            TEAROFF_METHOD_NULL                                                         \
            TEAROFF_METHOD(klass, PrivateQueryInterface, win16, (REFIID, void **))      \
            TEAROFF_METHOD_(klass, PrivateAddRef, win16, ULONG, ())                     \
            TEAROFF_METHOD_(klass, PrivateRelease, win16, ULONG, ())

#define BEGIN_TEAROFF_TABLE_PROPDESC(klass, itf)\
    klass::PFNTEAROFF const klass::s_apfnpd##itf[] = {\
            TEAROFF_METHOD_NULL\
            TEAROFF_METHOD(klass, PrivateQueryInterface, win16, (REFIID, void **))\
            TEAROFF_METHOD_(klass, PrivateAddRef, win16, ULONG, ())\
            TEAROFF_METHOD_(klass, PrivateRelease, win16, ULONG, ())\

#define BEGIN_TEAROFF_TABLE_(klass, itf)                                        \
    klass::PFNTEAROFF const klass::s_apfn##itf[] = {                            \
            TEAROFF_METHOD_NULL                                                 \
            TEAROFF_METHOD(klass, QueryInterface, win16, (REFIID, void **))     \
            TEAROFF_METHOD_(klass, AddRef, win16, ULONG, ())                    \
            TEAROFF_METHOD_(klass, Release, win16, ULONG, ())

#define BEGIN_TEAROFF_TABLE_SUB_(klass, subklass, itf)                                        \
    klass::subklass::PFNTEAROFF const klass::subklass::s_apfn##itf[] = {                            \
            TEAROFF_METHOD_NULL                                                 \
            TEAROFF_METHOD(klass::subklass, klass::subklass::QueryInterface, win16, (REFIID, void **))     \
            TEAROFF_METHOD_(klass::subklass, klass::subklass::AddRef, win16, ULONG, ())                    \
            TEAROFF_METHOD_(klass::subklass, klass::subklass::Release, win16, ULONG, ())

#define BEGIN_TEAROFF_TABLE_NAMED(klass, name)                  \
    klass::PFNTEAROFF const klass::name[] = {                   \
            TEAROFF_METHOD_NULL                                                    

#define END_TEAROFF_TABLE()\
    };

#define _TEAROFF_METHOD(fn, FN, args)\
    (PFNTEAROFF)(HRESULT (STDMETHODCALLTYPE thisclass::*)args)fn,

#define _TEAROFF_METHOD_(fn, FN, ret, args)\
    (PFNTEAROFF)(ret (STDMETHODCALLTYPE thisclass::*)args)fn,

#define TEAROFF_METHOD(klass, fn, FN, args)\
    (PFNTEAROFF)(HRESULT (STDMETHODCALLTYPE thisclass::*)args)fn,

#define TEAROFF_METHOD_(klass, fn, FN, ret, args)\
    (PFNTEAROFF)(ret (STDMETHODCALLTYPE thisclass::*)args)fn,

#define TEAROFF_METHOD_SUB(klass, subklass, fn, FN, args)\
    TEAROFF_METHOD(klass::subklass, klass::subklass::fn, klass::subklass::FN, args)

#define TEAROFF_METHOD_NULL \
    _TEAROFF_METHOD(NULL,NULL,(void))

#define CONTEXTTHUNK_SETCONTEXT                 \
{                                               \
    pUnkContext = (IUnknown *)_GetTearoff();           \
}

#define CONTEXTTHUNK_SETTREENODE                \
    IUnknown * pUnkContext;                     \
    CONTEXTTHUNK_SETCONTEXT                     \
    Assert (pUnkContext);                       \
    pNode = NULL;                               \
    pUnkContext->QueryInterface(CLSID_CTreeNode, (void**)&pNode);

#pragma INCMSG("--- End 'unixoff.hxx'")
#else
#pragma INCMSG("*** Dup 'unixoff.hxx'")
#endif
