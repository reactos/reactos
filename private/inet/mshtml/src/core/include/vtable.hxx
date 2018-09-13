//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       vtable.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_VTABLE_HXX_
#define I_VTABLE_HXX_
#pragma INCMSG("--- Beg 'vtable.hxx'")

class CVoid;

#if defined(SPARC) || defined (hp700)
// IEUNIX - Sun and HP specific vtable structs...

//
// This this pointer is always a pointer to a struct TEAROFF_THUNK *
// Use TEAROFF_THIS as the type.
//

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

typedef struct TEAROFF_THUNK *TEAROFF_THIS;

//
// Tearoff tables pointing at virtual table methods.  As described
// bellow some will point at function addresses, others at vtable
// entries.
//

typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFNTEAROFF)(); 
typedef HRESULT (STDMETHODCALLTYPE *PLAINPFNTEAROFF)(TEAROFF_THIS);

//
// Plain tearoffs are NONVIRTUAL_VTABLE_ENTRYs used to build a tearoff
// interface to functions as opposed to methods.  
// For now this is only used in the construction of the tearoff
// pointing to our thunks
//

#if defined(hp700)
extern PLAINPFNTEAROFF g_unixTearOffThunkTable[]; 
#endif

#if defined(hp700) && !defined(__APOGEE__)
// IEUNIX - HP vtable reqmts.

#define NONVIRTUAL_METHOD(fn, args) \
    { (PLAINPFNTEAROFF)(HRESULT (STDMETHODCALLTYPE *)args)fn },

#define NULL_METHOD  {0},{0}, {(PLAINPFNTEAROFF)0x12345678}, {0} 

#else

#define NONVIRTUAL_METHOD(fn, args) \
    { 0,0, (PLAINPFNTEAROFF)(HRESULT (STDMETHODCALLTYPE *)args)fn },

#define NULL_METHOD {0,0,0}

#endif

//
// Macros for calling a misc vtbl method
//

//
// NOTE!!!  CALL_METHOD_THIS must be the first argument in args
//

#define CALL_METHOD_THIS &tmpThunk

//
// pThis == this for Object
// pVtbl == vtable
// iMethod == method index into pVtbl (not counting NULL_VTBL padd entries)
// args == arguments to pass with CALL_METHOD_THIS as first arg.
//         CALL_METHOD_THIS will be substituted with pThis at the appropriate
//         time.
//

#if defined(hp700)
#define CALL_TEAROFF_THUNK(iMethod)  (*g_unixTearOffThunkTable[iMethod])
#else
// Sparc
#define CALL_TEAROFF_THUNK(iMethod)  TearOffThunk##iMethod 
#endif

#define CALL_VTBL_METHOD( pThis, pVtbl, iMethod, args ) \
    do {                                                \
        TEAROFF_THUNK tmpThunk;                         \
                                                        \
        tmpThunk.pvObject1 = pThis;                     \
        tmpThunk.apfnVtblObject1 = pVtbl;               \
        tmpThunk.dwMask = 0;                            \
                                                        \
        CALL_TEAROFF_THUNK(iMethod) args;               \
    } while (0)


//
// VTABLE structure definitions
//

typedef struct VIRTUAL_VTABLE_ENTRY {

        SHORT thisOffset;
        WORD  vtblMethodIndex;
        INT   thisVtblOffset;
        
} VIRTUAL_VTABLE_ENTRY;


typedef struct NONVIRTUAL_VTABLE_ENTRY {
#if !defined(hp700) || (defined(hp700)&&defined(__APOGEE__))
// ieunix - sparc.
        SHORT           thisOffset;
        WORD            unused;
#endif 
        PLAINPFNTEAROFF pfnPlain;
        
} NONVIRTUAL_VTABLE_ENTRY;


typedef union _VTABLE_ENTRY {
    
    //
    // pfn is actually an 8 byte pointer as pointers to methods
    //     are 8 bytes.  The 2 DWORDS contain:
    //     o For virtual methods:
    //          HIGH DWORD - HIGH WORD - Offset to add to this for target method
    //          HIGH DWORD - LOW WORD  - Index of method in target vtable
    //          LO DWORD   - Offset from this to target vtable
    //     o For non-virtual methods:
    //          HIGH DWORD - HIGH WORD - Offset to add to this for target method
    //          HIGH DWORD - LOW WORD  - Always 0
    //          LO DWORD - function address
    //

#if !defined(hp700) || (defined(hp700) && defined(__APOGEE__))
// IEUNIX - Sparc or HP/Apogee.
    PFNTEAROFF pfn;
    VIRTUAL_VTABLE_ENTRY _virtual;
#endif
    NONVIRTUAL_VTABLE_ENTRY _nonvirtual;

} VTABLE_ENTRY;

#if defined(hp700) && !(defined(__APOGEE__))
// IEUNIX HP and sun specific
#define FIRST_VTABLE_OFFSET 0x10

#define VTBL_PFN(pVtbl) (pVtbl->_nonvirtual.pfnPlain)
#define VTBL_THIS(pVtbl,pThis) (pThis)

#else
#define FIRST_VTABLE_OFFSET sizeof( VTABLE_ENTRY )

#define VTBL_PFN(pVtbl) (pVtbl->_nonvirtual.pfnPlain)
#define VTBL_THIS(pVtbl,pThis) ((void*)((char*)(pThis) + pVTbl->_nonvirtual.thisOffset))
#endif

#else

#ifdef _MAC
typedef HRESULT (STDMETHODCALLTYPE *PFNVTABLE)(void);
#endif

// Win32 / Win16

typedef HRESULT (STDMETHODCALLTYPE CVoid::*PFNTEAROFF)(void);

typedef union _VTABLE_ENTRY {

#ifdef _MAC
    PFNVTABLE pfn;
#else
    PFNTEAROFF pfn;
#endif
    void *pvfn;

} VTABLE_ENTRY;

#ifdef _MAC
#define FIRST_VTABLE_OFFSET sizeof( VTABLE_ENTRY )
#else
#define FIRST_VTABLE_OFFSET 0
#endif

#define VTBL_PFN(pVtbl) (pVtbl->pvfn)
#define VTBL_THIS(pVtbl,pThis) (pThis)

#endif

#pragma INCMSG("--- End 'vtable.hxx'")
#else
#pragma INCMSG("*** Dup 'vtable.hxx'")
#endif
