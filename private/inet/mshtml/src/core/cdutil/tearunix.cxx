//+------------------------------------------------------------------------
//
//  File:       utearoff.cxx
//
//  Contents:   Tear off interfaces.
//
//  History:
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

typedef HRESULT (FNQI)(void *pv, REFIID iid, void **ppv);
typedef ULONG   (FNAR)(void *pv);

void TearoffCheck()
{
    AssertSz( 0, "Tearoff table too small" );
}

typedef HRESULT (STDMETHODCALLTYPE *PFNQI)(TEAROFF_THIS, REFIID, void **);

HRESULT STDMETHODCALLTYPE
PlainQueryInterface(TEAROFF_THUNK * pthunk, REFIID iid, void **ppv);

ULONG STDMETHODCALLTYPE
PlainAddRef (TEAROFF_THUNK * pthunk);

ULONG STDMETHODCALLTYPE
PlainRelease (TEAROFF_THUNK * pthunk);

typedef void (*PFNVOID)();

#define THUNK_EXTERN(n) EXTERN_C void TearoffThunk##n(TEAROFF_THIS, ...);

THUNK_EXTERN(0) // QI
THUNK_EXTERN(1) // ADDREF
THUNK_EXTERN(2) // RELEASE

THUNK_ARRAY_3_TO_15(EXTERN)
THUNK_ARRAY_16_AND_UP(EXTERN)

#define TEAROFFCHECK_THUNK NONVIRTUAL_METHOD(TearoffCheck, (TEAROFF_THIS))
#define THUNK_ADDRESS(n) NONVIRTUAL_METHOD(TearoffThunk##n, (TEAROFF_THIS))

NONVIRTUAL_VTABLE_ENTRY s_unixTearOffVtable[] = {
    NULL_METHOD,
    THUNK_ADDRESS(0)
    NONVIRTUAL_METHOD(PlainAddRef, (TEAROFF_THIS))
    NONVIRTUAL_METHOD(PlainRelease, (TEAROFF_THIS))
    THUNK_ARRAY_3_TO_15(ADDRESS)
    THUNK_ARRAY_16_AND_UP(ADDRESS)
#if DBG==1
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK  
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK  TEAROFFCHECK_THUNK
    TEAROFFCHECK_THUNK
#endif
};

static void * s_pvCache1 = NULL;
static void * s_pvCache2 = NULL;

void DeinitTearOffCache()
{
    MemFree(s_pvCache1);
    MemFree(s_pvCache2);
}

ULONG STDMETHODCALLTYPE
PlainAddRef (TEAROFF_THUNK * pthunk)
{
    return ++pthunk->ulRef;
}

ULONG STDMETHODCALLTYPE
PlainRelease (TEAROFF_THUNK * pthunk)
{
    WHEN_DBG(static long l = 0; l++;)

    Assert( pthunk->ulRef > 0 );

    if (--pthunk->ulRef == 0)
    {
        if (pthunk->pvObject1 && (pthunk->dwMask & METHOD_MASK( METHOD_RELEASE )) == 0)
        {
            CALL_VTBL_METHOD( pthunk->pvObject1, pthunk->apfnVtblObject1, 2 /*METHOD_RELEASE*/, (CALL_METHOD_THIS));
        }
        if (pthunk->pvObject2)
        {
            CALL_VTBL_METHOD( pthunk->pvObject2, pthunk->apfnVtblObject2, 2 /*METHOD_RELEASE*/, (CALL_METHOD_THIS));
        }

        pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache1, pthunk );

        if (pthunk)
        {
            pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache2, pthunk );

            if (pthunk)
            {
                MemFree( pthunk );
            }
        }

        return 0;
    }

    return pthunk->ulRef;
}


//+------------------------------------------------------------------------
//
//  Function:   CreateTearOffTunk
//
//  Synopsis:   Create a tearoff interface thunk. The returned object
//              must be AddRef'ed by the caller.
//
//  Arguments:  pvObject    Delegate to this object using...
//              apfnObject    ...this array of pointers to member functions.
//              pUnkOuter   Delegate IUnknown methods to this object.
//              ppvThunk    The returned thunk.
//              pvObject2   Delegate to this object instead...
//              apfnObject2   ... this array of pointers to functions...
//              dwMask        ... when the index of the method call is
//                            marked in this mask.
//
//  Notes:      The basic implementation here consists of a thunk with
//              a pointer to two different objects.  If the second object
//              is NULL, it is assumed to be the first object.  This
//              is the logic of the thunks:
//
//                  i is the index of the method that is called.
//
//                  if (i < 16)
//                  {
//                      if (dwMask & 2^i)
//                      {
//                          Delegate to pvObject2 using apfnObject2
//                      }
//                  }
//                  Delegate to pvObject1 using apfnObject1
//
//-------------------------------------------------------------------------

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void *apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID)
{
#   define ADD_REL_MASK (METHOD_MASK( METHOD_ADDREF ) || METHOD_MASK( METHOD_RELEASE ))

    TEAROFF_THUNK * pthunk;

    Assert(ppvThunk);
    Assert(apfn1);
    Assert((!pvObject2 && !apfn2) || (pvObject2 && apfn2));
    Assert(!(dwMask & 0xFFFF0000) && "Only 16 overrides allowed");
    Assert(!dwMask || (dwMask && pvObject2));
    Assert(!pUnkOuter || (dwMask == 0 && pvObject2 == 0));
    Assert((dwMask & ADD_REL_MASK) == 0 || ((dwMask & ADD_REL_MASK) == ADD_REL_MASK));

    if (pUnkOuter)
    {
        pvObject2 = pUnkOuter;
        apfn2 = *(void **)pUnkOuter;
        dwMask = METHOD_MASK( METHOD_QI );
    }

    pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache1, NULL);

    if (!pthunk)
    {
        pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache2, NULL);

        if (!pthunk)
        {
            pthunk = (TEAROFF_THUNK *) MemAlloc(sizeof(TEAROFF_THUNK));

            MemSetName((pthunk, "Tear-Off Thunk - owner=%08x", pvObject1));
        }
    }

    if (!pthunk)
    {
        *ppvThunk = NULL;
        RRETURN(E_OUTOFMEMORY);
    }

    pthunk->papfnVtblThis = s_unixTearOffVtable;
    pthunk->ulRef = 0;
    pthunk->pvObject1 = pvObject1;
    pthunk->apfnVtblObject1 = apfn1;
    pthunk->pvObject2 = pvObject2;
    pthunk->apfnVtblObject2 = apfn2;
    pthunk->dwMask = dwMask;
    pthunk->apIID = apIID ? apIID : (const IID * const *)&g_Zero;

    if (pvObject1 && (dwMask & METHOD_MASK( METHOD_ADDREF )) == 0)
    {
        CALL_VTBL_METHOD( pthunk->pvObject1, pthunk->apfnVtblObject1, 1 /*METHOD_ADDREF*/, (CALL_METHOD_THIS));
    }
    if (pvObject2)
    {
        CALL_VTBL_METHOD( pthunk->pvObject2, pthunk->apfnVtblObject2, 1 /*METHOD_ADDREF*/, (CALL_METHOD_THIS));
    }
    *ppvThunk = pthunk;

    return S_OK;
}

// Short argument list version saves space in calling functions.

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk)
{
    return CreateTearOffThunk(
            pvObject1, 
            apfn1, 
            pUnkOuter, 
            ppvThunk, 
            NULL, 
            NULL, 
            0, 
            NULL);
}
