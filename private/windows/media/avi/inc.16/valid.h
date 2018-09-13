#define IsValidPtrIn(pv,cb)  (!IsBadReadPtr ((pv),(cb)))
#define IsValidPtrOut(pv,cb) (!IsBadWritePtr((pv),(cb)))

STDAPI_(BOOL) IsValidInterface( void FAR* pv );
STDAPI_(BOOL) IsValidIid( REFIID riid );

 
#ifdef _DEBUG

//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid in ptr", _szAssertFile, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid in ptr", _szAssertFile, __LINE__), retval)
#define VOID_VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    FnAssert(#pv,"Invalid in ptr", _szAssertFile, __LINE__); return; }

//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid out ptr", _szAssertFile, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTROUT( pv, TYPE, retval ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (FnAssert(#pv,"Invalid out ptr", _szAssertFile, __LINE__), retval)

//** INTERFACE validation macro:
#define GEN_VDATEIFACE( pv, retval ) if (!IsValidInterface(pv)) \
    return (FnAssert(#pv,"Invalid interface", _szAssertFile, __LINE__), retval)
#define VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return (FnAssert(#pv,"Invalid interface", _szAssertFile, __LINE__),ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) if (!IsValidInterface(pv)) {\
    FnAssert(#pv,"Invalid interface", _szAssertFile, __LINE__); return; }

//** INTERFACE ID validation macro:
#define VDATEIID( iid ) if (!IsValidIid( iid )) \
    return (FnAssert(#iid,"Invalid iid", _szAssertFile, __LINE__),ResultFromScode(E_INVALIDARG))
#define GEN_VDATEIID( iid, retval ) if (!IsValidIid( iid )) {\
    FnAssert(#iid,"Invalid iid", _szAssertFile, __LINE__); return retval; }
#else



//  --assertless macros for non-debug case
//** POINTER IN validation macros:
#define VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))
#define GEN_VDATEPTRIN( pv, TYPE, retval ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) \
    return (retval)
#define VOID_VDATEPTRIN( pv, TYPE ) if (!IsValidPtrIn( (pv), sizeof(TYPE))) {\
    return; }

//** POINTER OUT validation macros:
#define VDATEPTROUT( pv, TYPE ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (ResultFromScode(E_INVALIDARG))

#define GEN_VDATEPTROUT( pv, TYPE, retval ) if (!IsValidPtrOut( (pv), sizeof(TYPE))) \
    return (retval)

//** INTERFACE validation macro:
#define VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return (ResultFromScode(E_INVALIDARG))
#define VOID_VDATEIFACE( pv ) if (!IsValidInterface(pv)) \
    return; 
#define GEN_VDATEIFACE( pv, retval ) if (!IsValidInterface(pv)) \
    return (retval)

//** INTERFACE ID validation macro:
#define VDATEIID( iid ) if (!IsValidIid( iid )) \
    return (ResultFromScode(E_INVALIDARG))
#define GEN_VDATEIID( iid, retval ) if (!IsValidIid( iid )) \
    return retval; 

#endif

