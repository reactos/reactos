
#if DBG==1 && defined(WIN32) && !defined(_CHICAGO_)
#define VDATEHEAP() if( !HeapValidate(GetProcessHeap(),0,0)){ DebugBreak();}
#else
#define VDATEHEAP()
#endif  //  DBG==1 && defined(WIN32) && !defined(_CHICAGO_)

#define IsValidPtrIn(pv,cb)  ((pv == NULL) || !IsBadReadPtr ((pv),(cb)))
#define IsValidPtrOut(pv,cb) (!IsBadWritePtr((pv),(cb)))

STDAPI_(BOOL) IsValidInterface( void FAR* pv );


#if DBG==1
// for performance, do not do in retail builds
STDAPI_(BOOL) IsValidIid( REFIID riid );
#else
#define IsValidIid(x) (TRUE)
#endif

#ifdef _DEBUG

//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid in ptr", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid in ptr", __FILE__, __LINE__), retval)
#define VOID_VDATEPTRIN( pv, TYPE ) \
        if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    FnAssert(#pv,"Invalid in ptr", __FILE__, __LINE__); return; }

//** POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = (FnAssert(#pv, "Invalid in ptr", __FILE__, __LINE__), ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = (FnAssert(#pv, "Invalid in ptr", __FILE__, __LINE__), retval); \
         goto label; }
#define VOID_VDATEPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { FnAssert(#pv, "Invalid in ptr", __FILE__, __LINE__); goto label; }

//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid out ptr", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTROUT( pv, TYPE, retval ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid out ptr", __FILE__, __LINE__), retval)

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTROUT_LABEL( pv, TYPE, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = (FnAssert(#pv,"Invalid out ptr", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEPTROUT_LABEL( pv, TYPE, retval, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = (FnAssert(#pv,"Invalid out ptr", __FILE__, __LINE__),retval); \
         goto label; }

//** INTERFACE validation macro:
#define GEN_VDATEIFACE( pv, retval ) \
        if (!IsValidInterface(pv)) \
    return (FnAssert(#pv,"Invalid interface", __FILE__, __LINE__), retval)
#define VDATEIFACE( pv ) \
        if (!IsValidInterface(pv)) \
    return (FnAssert(#pv,"Invalid interface", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) \
        if (!IsValidInterface(pv)) {\
    FnAssert(#pv,"Invalid interface", __FILE__, __LINE__); return; }

//** INTERFACE validation macros for single entry/single exit functions
//** uses a goto instead of return
#define GEN_VDATEIFACE_LABEL( pv, retval, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = (FnAssert(#pv,"Invalid interface", __FILE__, __LINE__),retval); \
         goto label; }
#define VDATEIFACE_LABEL( pv, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = (FnAssert(#pv,"Invalid interface", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define VOID_VDATEIFACE_LABEL( pv, label ) \
        if (!IsValidInterface(pv)) {\
        FnAssert(#pv,"Invalid interface", __FILE__, __LINE__); goto label; }

//** INTERFACE ID validation macro:
// Only do this in debug build
#define VDATEIID( iid ) if (!IsValidIid( iid )) \
    return (FnAssert(#iid,"Invalid iid", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEIID( iid, retval ) if (!IsValidIid( iid )) {\
    FnAssert(#iid,"Invalid iid", __FILE__, __LINE__); return retval; }

//** INTERFACE ID validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEIID_LABEL( iid, label, retVar ) if (!IsValidIid( iid )) \
        {retVar = (FnAssert(#iid,"Invalid iid", __FILE__, __LINE__),ResultFromScode(E_INVALIDARG)); \
         goto label; }
#define GEN_VDATEIID_LABEL( iid, retval, label, retVar ) if (!IsValidIid( iid )) {\
        FnAssert(#iid,"Invalid iid", __FILE__, __LINE__); retVar = retval;  goto label; }
#else



//  --assertless macros for non-debug case
//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (retval)
#define VOID_VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    return; }

//** POINTER IN validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTRIN_LABEL(pv, TYPE, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEPTRIN_LABEL(pv, TYPE, retval, label, retVar) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { retVar = retval; \
         goto label; }
#define VOID_VDATEPTRIN_LABEL(pv, TYPE, label) \
        if (!IsValidPtrIn((pv), sizeof(TYPE))) \
        { goto label; }

//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))

#define GEN_VDATEPTROUT( pv, TYPE, retval ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (retval)

//** POINTER OUT validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEPTROUT_LABEL( pv, TYPE, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEPTROUT_LABEL( pv, TYPE, retval, label, retVar ) \
        if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
        { retVar = retval; \
         goto label; }

//** INTERFACE validation macro:
#define VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return (ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return;
#define GEN_VDATEIFACE( pv, retval ) if (!IsValidInterface(pv)) \
    return (retval)

//** INTERFACE validation macros for single entry/single exit functions
//** uses a goto instead of return
#define GEN_VDATEIFACE_LABEL( pv, retval, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = retval; \
         goto label; }
#define VDATEIFACE_LABEL( pv, label, retVar ) \
        if (!IsValidInterface(pv)) \
        { retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define VOID_VDATEIFACE_LABEL( pv, label ) \
        if (!IsValidInterface(pv)) {\
         goto label; }

//** INTERFACE ID validation macro:
// do not do in retail build. This code USED to call a bogus version of
// IsValidIID that did no work. Now we are faster and no less stable than before.
#define VDATEIID( iid )             ((void)0)
#define GEN_VDATEIID( iid, retval ) ((void)0);

//** INTERFACE ID validation macros for single entry/single exit functions
//** uses a goto instead of return
#define VDATEIID_LABEL( iid, label, retVar ) if (!IsValidIid( iid )) \
        {retVar = ResultFromScode(E_INVALIDARG); \
         goto label; }
#define GEN_VDATEIID_LABEL( iid, retval, label, retVar ) if (!IsValidIid( iid )) {\
        retVar = retval;  goto label; }

#endif

