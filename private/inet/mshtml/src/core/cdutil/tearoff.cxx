//+------------------------------------------------------------------------
//
//  File:       toff.cxx
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

MtDefine(TearOff, PerProcess, "TearOff Thunk")
DeclareTag(tagNoTearoffCache, "Tearoff", "Disable tearoff cache")
DeclareTag(tagLeakTearoffs, "Tearoff", "Leak all tearoffs on purpose")
DeclareTag(tagZapTearoffs, "Tearoff", "Zap contents of tearoffs on release")
DeclareTag(tagTearoffSymbols, "!Memory", "Leaks: Stacktraces and Symbols for Tearoffs")

#ifdef WIN16
typedef HRESULT STDMETHODCALLTYPE (FNQI)(void *pv, REFIID iid, void **ppv);
typedef ULONG   STDMETHODCALLTYPE (FNAR)(void *pv);
#else
typedef HRESULT (FNQI)(void *pv, REFIID iid, void **ppv);
typedef ULONG   (FNAR)(void *pv);
#endif

#if defined(_M_IX86) && !defined(WIN16) && !defined(_MAC)

#define THUNK_IMPLEMENT_COMPARE(n)\
void __declspec(naked) STDMETHODCALLTYPE TearoffThunk##n()\
{                                           \
    /* this = thisArg                   */  \
    __asm mov eax, [esp + 4]                \
    /* save original this               */  \
    __asm push eax                          \
    /*  if this->_dwMask & 1 << n       */  \
    __asm test dword ptr [eax + 28], 1<<n   \
    /* jmp if < to Else */                  \
    __asm je $+9                            \
    /* increment offset to pvObject2    */  \
    __asm add eax, 8                        \
    /* increment offset to pvObject1    */  \
    __asm add eax, 12                       \
    /* pvObject = this->_pvObject       */  \
    __asm mov ecx, [eax]                    \
    /* thisArg = pvObject2              */  \
    __asm mov [esp + 8], ecx                \
    /* apfnObject = this->_apfnVtblObj  */  \
    __asm mov ecx, [eax + 4]                \
    /* pfn = apfnObject[n]              */  \
    __asm mov ecx, [ecx + (4 * n)]          \
    /* set eax back to the tearoff      */  \
    __asm pop eax                           \
    /* remember vtbl index of method    */  \
    __asm mov dword ptr [eax + 32], n       \
    /* jump....                         */  \
    __asm jmp ecx                           \
}

#define THUNK_IMPLEMENT_SIMPLE(n)\
void __declspec(naked) STDMETHODCALLTYPE TearoffThunk##n()\
{                                           \
    /* this = thisArg                   */  \
    __asm mov eax, [esp + 4]                \
    /* remember vtbl index of method    */  \
    __asm mov dword ptr [eax + 32], n       \
    /* pvObject = this->_pvObject       */  \
    __asm mov ecx, [eax + 12]               \
    /* thisArg = pvObject               */  \
    __asm mov [esp + 4], ecx                \
    /* apfnObject = this->_apfnVtblObj  */  \
    __asm mov ecx, [eax + 16]               \
    /* pfn = apfnObject[n]              */  \
    __asm mov ecx, [ecx + (4 * n)]          \
    /* jump....                         */  \
    __asm jmp ecx                           \
}

// Single step a few times for the function you are calling.

THUNK_ARRAY_3_TO_15(IMPLEMENT_COMPARE);
THUNK_ARRAY_16_AND_UP(IMPLEMENT_SIMPLE);

#elif defined(_MAC)

#define offsetof_pvObject    12      // Keep in sync w/ TOFF.CXX
#define offsetof_apfn        16
#define offsetof_Mask        28

//* ON ENTRY
//*
//*      r3  - this
//*      The stack frame is of the original caller.
//*
//* REGISTER USAGE
//*
//*      r11 - temp
//*
//* ON EXIT
//*
//*      r3 - this
//*      r12 - The transition vector address (thunk function being called)
//*      r31 - address of thunk

#define THUNK_IMPLEMENT_COMPARE(n)\
asm void __declspec(naked) STDMETHODCALLTYPE TearoffThunk##n()\
{                                           \
       stw     r3,-4(SP);                /* save this pointer on stack */ \
       li      r12, 1;                  /* load 1 into r12, and... */\
       slwi    r12, r12, n;             /* ...shift left by the index */\
       lwz     r11, offsetof_Mask(r3);  /* load mask dword into r11 */\
       and     r11, r11, r12;           \
       cmpli   cr1, 0, r11, 0;            /* compare */\
       li      r12, n;                  /* Put the method # in r12 */\
       stw     r12, 32(r3);             /* save method # in thunk */ \
       bc      12, 6, Obj;          \
       addi    r3, r3, 8;               /* add for object 1 */\
Obj:   addi    r3, r3, 12;             /* add offset */\
                        /* r3 is now the pvObject ptr, i.e. the intermediate 'this' ptr */\
       lwz     r11, 0x4(r3);            /* Copy this->apfn to r11 */\
       lwz     r3,0(r3);                    /* set r3 to object pointer */\
       lwz     r0,0(r11);                  /* get first vtable entry */\
       cmpi    crf0,0,r0,0;                 /* check for c++ vtable */\
       bc      4,2,tear;                /* bne - tearoff non virtual */\
       lwz     r0,8(r11);                  /* get first vtable entry */\
       cmpi    crf0,0,r0,0;                 /* check for c++ vtable */\
       bc      4,2,realvt;                /* bne - c++ vtable */\
tear:  li      r12, (n*12);             /* Put the method # * 12 in r12 */\
       add     r12,r12,r11  ;                   /* r12 points to correct tearoff vtbl entry */\
       lwz     r11,4(r12);                  /* get vtbl offset / flag */\
       lwz     r12,8(r12);                  /* get address in non virtual */\
       cmpi    crf0,0,r11,0;                    /* check if virtual */\
       bc      12, 0, Non;                      /* branch if not virtual */\
       lwz     r12,0(r3);               \
       lwzx    r12,r12,r11;                 /* get function vector address */\
Non:   lwz     r0,0(r12);                   /* get function address */\
       mtctr   r0;  \
       lwz     r2,4(r12);                   /* load new toc */ \
       lwz     r11,-4(SP);              /* restore this pointer */\
       bctr;                            /* branch to counter */\
realvt: li     r12,(n*4+4);             /* offset into c++ vtable */\
       lwzx    r12,r11,r12;              /* get vtable entry */\
       b       Non;                     \
}

/*
Checking for type of vtable works as follows
long               0      2
_______________________________
tearoff virtual    0      0
tearoff non virt   -1     non 0
c++ vtable         0      non 0
*/

#define THUNK_IMPLEMENT_SIMPLE(n)\
asm void __declspec(naked) STDMETHODCALLTYPE TearoffThunk##n() \
{                                                \
       li      r12, n;         /* Put the method # in r12 */    \
       stw     r12, 32(r3);    /* save method # in thunk */ \
       stw     r3,-4(SP);                /* save this pointer on stack */ \
       lwz     r11, offsetof_apfn(r3);      /* Copy this->apfn to r11 */\
       lwz     r3,offsetof_pvObject(r3);                    /* set r3 to object pointer */\
       lwz     r0,0(r11);                  /* get first vtable entry */\
       cmpi    crf0,0,r0,0;                 /* check for c++ vtable */\
       bc      4,2,tear;                /* bne - tearoff non virtual */\
       lwz     r0,8(r11);                  /* get first vtable entry */\
       cmpi    crf0,0,r0,0;                 /* check for c++ vtable */\
       bc      4,2,realvt;                /* bne - c++ vtable */\
tear:  li      r12, (n*12);             /* Put the method # * 12 in r12 */\
       add     r12,r12,r11  ;                   /* r12 points to correct tearoff vtbl entry */\
       lwz     r11,4(r12);                  /* get vtbl offset / flag */\
       lwz     r12,8(r12);                  /* get address in non virtual */\
       cmpi    crf0,0,r11,0;                    /* check if virtual */\
       bc      12, 0, Non;                      /* branch if not virtual */\
       lwz     r12,0(r3);               \
       lwzx    r12,r12,r11;                 /* get function vector address */\
Non:   lwz     r0,0(r12);                   /* get function address */\
       mtctr   r0;  \
       lwz     r2,4(r12);                   /* load new toc */ \
       lwz     r11,-4(SP);              /* restore this pointer */\
       bctr;                                /* branch to counter */\
realvt: li     r12,(n*4+4);             /* offset into c++ vtable */\
       lwzx    r12,r11,r12;              /* get vtable entry */\
       b       Non;                     \
}

#pragma require_prototypes off

THUNK_ARRAY_3_TO_15(IMPLEMENT_COMPARE);
THUNK_ARRAY_16_TO_101(IMPLEMENT_SIMPLE);
THUNK_ARRAY_102_TO_145(IMPLEMENT_SIMPLE);
THUNK_ARRAY_146_AND_UP(IMPLEMENT_SIMPLE);

#pragma require_prototypes reset
asm TEAROFF_THUNK* GetThunk()
{
    addi    r3,r11,0        // return the saved thunk pointer
    blr
}


#endif

#ifdef WIN16
void TearoffCheck()
#else
void STDMETHODCALLTYPE TearoffCheck()
#endif
{
    AssertSz( 0, "Tearoff table too small" );
}

HRESULT STDMETHODCALLTYPE
PlainQueryInterface(TEAROFF_THUNK * pthunk, REFIID iid, void **ppv)
{
    void *pv;
    const void *apfnVtbl;
    IID const * const * ppIID;

    for (ppIID = pthunk->apIID; *ppIID; ppIID++)
    {
        if (**ppIID == iid)
        {
            *ppv = pthunk;
            pthunk->ulRef += 1;
            return S_OK;
        }
    }

    if (pthunk->dwMask & 1)
    {
        pv = pthunk->pvObject2;
        apfnVtbl = pthunk->apfnVtblObject2;
    }
    else
    {
        pv = pthunk->pvObject1;
        apfnVtbl = pthunk->apfnVtblObject1;
    }

#ifdef _MAC
	typedef HRESULT ( IUnknown::*const tFunc)(REFIID iid, void **ppv) ;
	

	if(((long*)apfnVtbl)[0] || (((long*)apfnVtbl)[2] == 0))
	{
		tFunc	qi = ((tFunc*)apfnVtbl)[0];
		
	    return (((IUnknown*)pv)->*qi)(iid, ppv);
	}
	else
    	return ((FNQI *)((void **)apfnVtbl)[1])(pv, iid, ppv);
	
#else
    return ((FNQI *)((void **)apfnVtbl)[0])(pv, iid, ppv);
#endif // _MAC
}


ULONG STDMETHODCALLTYPE
PlainAddRef (TEAROFF_THUNK * pthunk)
{
    return ++pthunk->ulRef;
}

static void * s_pvCache1 = NULL;
static void * s_pvCache2 = NULL;

ULONG STDMETHODCALLTYPE
PlainRelease (TEAROFF_THUNK * pthunk)
{
    WHEN_DBG(static long l = 0; l++;)

    Assert( pthunk->ulRef > 0 );

    if (--pthunk->ulRef == 0)
    {
#ifdef _MAC    
	    void *pv;
        const void *apfnVtbl;
    
	    if (pthunk->dwMask & 4)
	    {
	        pv = pthunk->pvObject2;
     	    apfnVtbl = pthunk->apfnVtblObject2;
	    }
	    else
	    {
	        pv = pthunk->pvObject1;
  	        apfnVtbl = pthunk->apfnVtblObject1;
	    }

		typedef unsigned long ( IUnknown::*const tFunc)(void) ;
		
		// if no null entry, vtable is tearoff
		if(((long*)apfnVtbl)[0] || (((long*)apfnVtbl)[2] == 0))
		{
			tFunc	rel = ((tFunc*)apfnVtbl)[2];
			
	    	return (((IUnknown*)pv)->*rel)();
		}
		else	// must be c++ vtable
            ((FNAR *)((void **)apfnVtbl)[3])(pv);
	    	
#else
        if (pthunk->pvObject1 && (pthunk->dwMask & 4) == 0)
        {
            ((FNAR *)((void **)pthunk->apfnVtblObject1)[2])(pthunk->pvObject1);
        }
        if (pthunk->pvObject2)
        {
            ((FNAR *)((void **)pthunk->apfnVtblObject2)[2])(pthunk->pvObject2);
        }
#endif

#if DBG==1
        if (IsTagEnabled(tagNoTearoffCache) || IsTagEnabled(tagTearoffSymbols))
        {
            MemFree(pthunk);
            return(0);
        }
        else
        {
            if (IsTagEnabled(tagZapTearoffs))
            {
                memset(pthunk, 0xFE, sizeof(TEAROFF_THUNK));
            }

            if (IsTagEnabled(tagLeakTearoffs))
            {
                return(0);
            }
        }
#endif

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

#ifdef WIN16
typedef void (*PFNVOID)();
#else
typedef void (STDMETHODCALLTYPE *PFNVOID)();
#endif

#if defined(_M_IX86) && !defined(WIN16)
#define THUNK_EXTERN(n) extern void STDMETHODCALLTYPE TearoffThunk##n();
#else
#define THUNK_EXTERN(n) EXTERN_C void TearoffThunk##n();
#endif

THUNK_ARRAY_3_TO_15(EXTERN)
#ifdef _MAC
THUNK_ARRAY_16_TO_101(EXTERN)
THUNK_ARRAY_102_TO_145(EXTERN)
THUNK_ARRAY_146_AND_UP(EXTERN)
#else
THUNK_ARRAY_16_AND_UP(EXTERN)
#endif // _MAC

#define THUNK_ADDRESS(n) & TearoffThunk##n,

#ifdef WIN16
static void (*s_apfnPlainTearoffVtable[])() =
#else
static void (STDMETHODCALLTYPE *s_apfnPlainTearoffVtable[])() =
#endif
{
#ifdef _MAC
    PFNVOID( NULL ),
#endif
    PFNVOID( & PlainQueryInterface ),
    PFNVOID( & PlainAddRef ),
    PFNVOID( & PlainRelease ),
    THUNK_ARRAY_3_TO_15(ADDRESS)
#ifdef _MAC
    THUNK_ARRAY_16_TO_101(ADDRESS)
    THUNK_ARRAY_102_TO_145(ADDRESS)
    THUNK_ARRAY_146_AND_UP(ADDRESS)
#else
    THUNK_ARRAY_16_AND_UP(ADDRESS)
#endif
#if DBG==1
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck,  &TearoffCheck, \
    &TearoffCheck,  &TearoffCheck,  &TearoffCheck
#endif
};

void DeinitTearOffCache()
{
    MemFree(s_pvCache1);
    MemFree(s_pvCache2);
}


//+------------------------------------------------------------------------
//
//  Function:   CreateTearOffThunk
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

#ifdef DEBUG_TEAROFFS
BOOL g_fDoneTearoffCompression = FALSE;
DEBUG_TEAROFF_NOTE *g_pnoteFirst = NULL;
#endif

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      pvObject2,
        void *      apfn2,
        DWORD       dwMask,
        const IID * const * apIID,
        void *      appropdescsInVtblOrder)
{
    TEAROFF_THUNK * pthunk;

    Assert(ppvThunk);
    Assert(apfn1);
    Assert((!pvObject2 && !apfn2) || (pvObject2 && apfn2));
    Assert(!(dwMask & 0xFFFF0000) && "Only 16 overrides allowed");
    Assert(!dwMask || (dwMask && pvObject2));
    Assert(!pUnkOuter || (dwMask == 0 && pvObject2 == 0));
    Assert((dwMask & (2|4)) == 0 || (dwMask & (2|4)) == (2|4));

#ifdef DEBUG_TEAROFFS
    Assert(g_fDoneTearoffCompression);
    Assert(((DWORD**)apfn1)[1] && "Tearoff compression not yet done!");
    Assert(!apfn2 || ((DWORD**)apfn2)[1] && "Tearoff compression not yet done!");
#endif

    if (pUnkOuter)
    {
        pvObject2 = pUnkOuter;
        apfn2 = *(void **)pUnkOuter;
        dwMask = 1;
    }

    pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache1, NULL);

    if (!pthunk)
    {
        pthunk = (TEAROFF_THUNK *) InterlockedExchangePointer(&s_pvCache2, NULL);

        if (!pthunk)
        {
#if DBG==1
            #define LEAKS_TRACE_TAG "Leaks: Stacktraces & symbols"
            AssertSz(DbgExFindTag(LEAKS_TRACE_TAG), "Please update the LEAKS_TRACE_TAG definition"); 
            BOOL fEnabled = FALSE;
            if (IsTagEnabled(tagTearoffSymbols))
            {
                fEnabled = DbgExEnableTag(DbgExFindTag(LEAKS_TRACE_TAG), TRUE);
            }
#endif

            pthunk = (TEAROFF_THUNK *) MemAlloc(Mt(TearOff), sizeof(TEAROFF_THUNK));

            MemSetName((pthunk, "Tear-Off Thunk - owner=%08x", pvObject1));

#if DBG==1
            if (IsTagEnabled(tagTearoffSymbols))
            {
                DbgExEnableTag(DbgExFindTag(LEAKS_TRACE_TAG), fEnabled);
            }
#endif

        }
    }

    if (!pthunk)
    {
        *ppvThunk = NULL;
        RRETURN(E_OUTOFMEMORY);
    }

    pthunk->papfnVtblThis = s_apfnPlainTearoffVtable;
    pthunk->ulRef = 0;
    pthunk->pvObject1 = pvObject1;
    pthunk->apfnVtblObject1 = apfn1;
    pthunk->pvObject2 = pvObject2;
    pthunk->apfnVtblObject2 = apfn2;
    pthunk->dwMask = dwMask;
    pthunk->apIID = apIID ? apIID : (const IID * const *)&g_Zero;
    pthunk->apVtblPropDesc = appropdescsInVtblOrder;

#ifdef _MAC
	((IUnknown*)pthunk)->AddRef();
#else
    if (pvObject1 && (dwMask & 2) == 0)
    {
        ((FNAR *)((void**)apfn1)[1])(pvObject1);
    }
    if (pvObject2)
    {
        ((FNAR *)((void**)apfn2)[1])(pvObject2);
    }
#endif // _MAC
    *ppvThunk = pthunk;

    return S_OK;
}

// Short argument list version saves space in calling functions.

HRESULT
CreateTearOffThunk(
        void *      pvObject1,
        const void * apfn1,
        IUnknown *  pUnkOuter,
        void **     ppvThunk,
        void *      appropdescsInVtblOrder)
{
    return CreateTearOffThunk(
            pvObject1, 
            apfn1, 
            pUnkOuter, 
            ppvThunk, 
            NULL, 
            NULL, 
            0, 
            NULL,
            appropdescsInVtblOrder);
}

HRESULT
InstallTearOffObject(void * pvthunk, void * pvObject, void *apfn, DWORD dwMask)
{
    TEAROFF_THUNK *pthunk = (TEAROFF_THUNK*)pvthunk;

    Assert(pthunk);
    Assert(!pthunk->pvObject2);
    Assert(!pthunk->apfnVtblObject2);
    Assert(!pthunk->dwMask);

    pthunk->pvObject2 = pvObject;
    pthunk->apfnVtblObject2 = apfn;
    pthunk->dwMask = dwMask;

    if (pvObject)
    {
        ((FNAR *)((void**)apfn)[1])(pvObject);
    }

    return S_OK;
}


#ifdef DEBUG_TEAROFFS

//+------------------------------------------------------------------------
//
// DEBUG_TEAROFFs check for multiple inheritance problems with tearoffs.
//
// With multiple inheritance with MSVC on Win32/X86 the calling
// convention for a virtual function in vtable X is to adjust the
// "this" pointer to point to offset in the object containing the
// pointer to vtable X before doing the call. Since the offset is
// needed to make the call correctly, a pointer to a method of a
// class with virtual methods and multiple inheritance is stored
// as two DWORDs: the first is the function pointer for the method,
// and the second is the offset to apply to the "this" pointer.
//
// Tearoffs tables are a table of ordinary function pointers to
// methods. The function pointers are obtained from full method
// pointers using a cast that truncates a possible 8-byte value
// to its first 4 bytes.
//
// Since any information about the "this" offset is lost when casting
// a method pointer down to a single-DWORD function pointer, tearoffs
// assume that the adjustment to "this" is zero. This means that
// tearoffs cannot refer to virtual methods which appear in multiple-
// inheritance vtables; only nonvirtual methods and methods in the
// primary vtable work.
//
// The DEBUG_TEAROFFS code verifies that multiple-inheritance problems
// do not occur by construcing actual 8-byte method pointers for every
// tearoff method and asserting that the second DWORD (the "this"
// offset) is zero.  While the check is being done, the table is
// compressed down to a table of 4-byte function pointers as required
// by the rest of the tearoff code.
//
//-------------------------------------------------------------------------


// Note: The check must be deferred until ProcessAttach time because at
// VC Runtime init time, method pointers are not initialized yet and
// cannot be read.

int DeferDebugCheckTearoffTable(DEBUG_TEAROFF_NOTE *pnote, void *apfn, char *string)
{
    pnote->pnoteNext = g_pnoteFirst;
    g_pnoteFirst = pnote;
    pnote->apfn = apfn;
    pnote->pchDebug = string;

    return 1;
}

void DebugCheckAllTearoffTables()
{
    Assert(!g_fDoneTearoffCompression);

    DEBUG_TEAROFF_NOTE *pnote;

    DEBUG_TEAROFF_METHOD<CVoid> *pfnFrom;
    DWORD *pfnTo;
    
    for (pnote=g_pnoteFirst; pnote; pnote=pnote->pnoteNext)
    {
        pfnFrom = (DEBUG_TEAROFF_METHOD<CVoid>*)(pnote->apfn);
        pfnTo = (DWORD*)pnote->apfn;

        while (pfnFrom->d.fn)
        {
            AssertSz(!pfnFrom->d.off, pnote->pchDebug);
            *pfnTo = pfnFrom->d.fn;
            pfnFrom++;
            pfnTo++;
        }
    }
    
    g_fDoneTearoffCompression = TRUE;
}

#endif

