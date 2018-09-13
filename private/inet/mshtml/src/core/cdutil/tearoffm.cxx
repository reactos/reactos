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

struct TEAROFF_THUNK
{
    void *      papfnVtblThis;     // Thunk's vtable
    ULONG       ulRef;             // Reference count for this thunk.
    IUnknown *  pUnkOuter;         // Delegate IUnknown methods to this object.
    void *      pvObject1;         // Delegate other methods to this object using...
    void *      apfnVtblObject1;   // ...this array of pointers to member functions.
    void *      pvObject2;         // Delegate methods to this object using...
    void *      apfnVtblObject2;   // ...this array of pointers to member functions...
    DWORD       dwMask;            // ...the index of the method is set in the mask.
};

#if defined(_M_MRX000)

extern "C" void __asm(char *, ...);

//
// *** this implementation requires n < 16 !!!
//

#define THUNK_IMPLEMENT_COMPARE(n)\
EXTERN_C void TearoffThunk##n() \
{\
    __asm(  \
           ".set noreorder \n" \
           "lw   %t1, 28(%a0)       \n"   /* t1 = this->dwMask        */  \
           "andi %t1, %t1,(1<<"#n") \n"   /* if ( (t1 & (1<<n)) == 0 )*/  \
           "beql %t1, %zero, foo"#n" \n"  /*                          */  \
           " nop                    \n"   /*  branch delay slot       */  \
           "addi %a0,%a0,8          \n"   /* increment to pvObject2   */  \
 "foo"#n":  add  %t2,%a0,12         \n"   /* t2 = &this->_pvObject    */  \
           "lw   %a0,0(%t2)         \n"   /* thisArg = *t2            */  \
           "lw   %t1,4(%t2)         \n"   /* t1 = this->_apfnObject   */  \
           "lw   %t2,("#n"*4)(%t1)  \n"   /* t2 = apfnObject[n]       */  \
           "j %t2                   \n" \
           " nop                    \n"   /*  branch delay slot       */  \
           ".set reorder \n" \
    ); \
}


#define THUNK_IMPLEMENT_SIMPLE(n)\
EXTERN_C void TearoffThunk##n()\
{\
    __asm(  \
           ".set noreorder \n" \
           "lw %t1,12(%a0)          \n" /* t1 = this->pvObject      */  \
           "lw %t2,16(%a0)          \n" /* t2 = this->apfnObject    */  \
           "or %a0,%t1,%zero        \n" /* thisArg = pvObject       */  \
           "lw %t3,("#n"*4)(%t2)    \n" /* t3 = apfnObject[n]       */  \
           "j %t3                   \n" /*                          */  \
           " nop                    \n" /*  branch delay slot       */  \
           ".set reorder \n" \
         ); \
}

// Single step a few times for the function you are calling.

THUNK_ARRAY_3_TO_15(IMPLEMENT_COMPARE)
THUNK_ARRAY_16_AND_UP(IMPLEMENT_SIMPLE)

#endif

