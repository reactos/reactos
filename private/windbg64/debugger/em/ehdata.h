/////////////////////////////////////////////////////////////////////////////
//
// THIS IS THE SAME FILE AS IN CRTWIN32\H\EHDATA.H.  IF MODIFICATIONS ARE
// MADE TO IT, THEN THEY MUST BE PROPAGATED TO THIS COPY.
//
// EHDATA.H -  Declare misc. types, macros, etc. for implementation
//             of C++ Exception Handling for the run-time and the compiler.
//             Hardware independent, assumes Windows NT.
//
// Portions of this header file can be disabled by defining the following
// macros:
//  _EHDATA_NOHEADERS - suppresses inclusion of standard header files
//          If this is specified, then appropriate typedefs or macros must
//          be provided by some other means.
//  _EHDATA_NOTHROW - suppresses definitions used only to describe a throw
//  _EHDATA_NOFUNCINFO - suppresses definitions for the frame descriptor
//  _EHDATA_NONT - suppresses definitions of our version of NT's stuff
//
// Other conditional compilation macros:
//  CC_EXPLICITFRAME - if true, representation of registration node includes
//          the value of the frame-pointer for that frame, making the location
//          of the registration node on the frame flexible.  This is intended
//          primarily for early testing.
//
// Created By:  Ben Schreiber, 20 May, 1993
// Edited  By:  ChrisWei, 01 March, 1994
//                  Remove CONTEXT def for x86 for TiborL.
// Edited By:   TiborL 03 March, 1994
//                  Mips (_M_MRX000 >= 4000) changes
// Edited By:   Al Dosser, 20 June, 1994
//                  Alpha changes
// Edited By:   Bill Baxter, 28 June, 1994
//                  Made compile w/C FE
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __EHDATA__DEFINED__
#define __EHDATA__DEFINED__

#if _M_IX86 >= 300
# ifndef CC_EXPLICITFRAME
#  define CC_EXPLICITFRAME  0   // If non-zero, we're using a hack version of the
                                // registration node.
# endif
#endif

#ifndef _EHDATA_NOHEADERS
#include <stddef.h>
#include <excpt.h>
#include <windef.h>
#endif

#pragma pack(push, ehdata, 4)

#define EH_EXCEPTION_NUMBER ('msc' | 0xE0000000)    // The NT Exception # that we use
#define EH_MAGIC_NUMBER1    0x19930520      // The magic # identifying this version
                            // As magic numbers increase, we have to keep track of
                            // the versions that we are backwards compatible with.
#define EH_EXCEPTION_PARAMETERS 3           // Number of parameters in exception record

//
// PMD - Pointer to Member Data: generalized pointer-to-member descriptor
//

typedef struct PMD
{
    ptrdiff_t   mdisp;      // Offset of intended data within base
    ptrdiff_t   pdisp;      // Displacement to virtual base pointer
    ptrdiff_t   vdisp;      // Index within vbTable to offset of base
    } PMD;

//
// PMFN - Pointer to Member Function
//          M00REVIEW: we may need something more than this, but this will do for now.
//

typedef void (*PMFN)(void);


//
// TypeDescriptor - per-type record which uniquely identifies the type.
//
// Each type has a decorated name which uniquely identifies it, and a hash
// value which is computed by the compiler.  The hash function used is not
// important; the only thing which is essential is that it be the same for
// all time.
//
// The special type '...' (ellipsis) is represented by a null name.
//
#pragma warning(disable:4200)       // get rid of obnoxious nonstandard extension warning

typedef struct TypeDescriptor
{
    DWORD   hash;           // Hash value computed from type's decorated name
    void *  spare;          // reserved, possible for RTTI
    char    name[];         // The decorated name of the type; 0 terminated.
    } TypeDescriptor;
#pragma warning(default:4200)

#define TD_HASH(td)     ((td).hash)
#define TD_NAME(td)     ((td).name)

#define TD_IS_TYPE_ELLIPSIS(td) ((td == NULL) || (TD_NAME(*td)[0] == '\0'))


#ifndef _EHDATA_NOTHROW

/////////////////////////////////////////////////////////////////////////////
//
// Description of the thrown object.  (M00REVIEW: not final)
//
// This information is broken down into three levels, to allow for maximum
// comdat folding (at the cost of some extra pointers).
//
// ThrowInfo is the head of the description, and contains information about
//              the particular variant thrown.
// CatchableTypeArray is an array of pointers to type descriptors.  It will
//              be shared between objects thrown by reference but with varying
//              qualifiers.
// CatchableType is the description of an individual type, and how to effect
//              the conversion from a given type.
//
//---------------------------------------------------------------------------


//
// CatchableType - description of a type that can be caught.
//
// Note:  although isSimpleType can be part of ThrowInfo, it is more
//        convenient for the run-time to have it here.
//

typedef struct CatchableType {
    unsigned        isSimpleType    : 1;    // Is it a simple type?
    unsigned        byReferenceOnly : 1;    // Must it be caught by reference?
    unsigned        hasVirtualBase  : 1;    // Is this type a class with virtual bases?
    TypeDescriptor *pType;                  // Pointer to the type descriptor for this type
    PMD             thisDisplacement;       // Pointer to instance of catch type within
                                            //      thrown object.
    int             sizeOrOffset;           // Size of simple-type object or offset into
                                            //  buffer of 'this' pointer for catch object
    PMFN            copyFunction;           // Copy constructor or CC-closure
} CatchableType;

#define CT_ISSIMPLETYPE(ct) ((ct).isSimpleType)
#define CT_BYREFONLY(ct)    ((ct).byReferenceOnly)
#define CT_HASVB(ct)        ((ct).hasVirtualBase)
#define CT_PTD(ct)          ((ct).pType)
#define CT_THISDISP(ct)     ((ct).thisDisplacement)
#define CT_SIZE(ct)         ((ct).sizeOrOffset)
#define CT_COPYFUNC(ct)     ((ct).copyFunction)
#define CT_OFFSET(ct)       ((ct).sizeOrOffset)
#define CT_HASH(ct)         (TD_HASH(*CT_PTD(ct)))
#define CT_NAME(ct)         (TD_NAME(*CT_PTD(ct)))


//
// CatchableTypeArray - array of pointers to catchable types, with length
//
#pragma warning(disable:4200)       // get rid of obnoxious nonstandard extension warning
typedef struct CatchableTypeArray {
    int nCatchableTypes;
    CatchableType   *arrayOfCatchableTypes[];
    } CatchableTypeArray;
#pragma warning(default:4200)

//
// ThrowInfo - information describing the thrown object, staticly built
// at the throw site.
//
// pExceptionObject (the dynamic part of the throw; see below) is always a
// reference, whether or not it is logically one.  If 'isSimpleType' is true,
// it is a reference to the simple type, which is 'size' bytes long.  If
// 'isReference' and 'isSimpleType' are both false, then it's a UDT or
// a pointer to any type (ie pExceptionObject points to a pointer).  If it's
// a pointer, copyFunction is NULL, otherwise it is a pointer to a copy
// constructor or copy constructor closure.
//
// The pForwardCompat function pointer is intended to be filled in by future
// versions, so that if say a DLL built with a newer version (say C10) throws,
// and a C9 frame attempts a catch, the frame handler attempting the catch (C9)
// can let the version that knows all the latest stuff do the work.
//

typedef struct ThrowInfo {
    unsigned    isConst     : 1;        // Is the object thrown 'const' qualified
    unsigned    isVolatile  : 1;        // Is the object thrown 'volatile' qualified
    unsigned    isUnaligned : 1;        // Is the object thrown 'unaligned' qualified
    PMFN        pmfnUnwind;             // Destructor to call when exception
                                        // has been handled or aborted.
#pragma message("HACK:  Baxter changed so that it compile with C")
#ifdef __cplusplus
    int (__cdecl*pForwardCompat)(...);  // Forward compatibility frame handler
#else
    int (__cdecl*pForwardCompat)(int __dummy,...);  // Forward compatibility frame handler
#endif
    CatchableTypeArray  *pCatchableTypeArray;   // Pointer to list of pointers to types.
} ThrowInfo;

#define THROW_ISCONST(t)        ((t).isConst)
#define THROW_ISVOLATILE(t)     ((t).isVolatile)
#define THROW_ISUNALIGNED(t)    ((t).isUnaligned)
#define THROW_UNWINDFUNC(t)     ((t).pmfnUnwind)
#define THROW_FORWARDCOMPAT(t)  ((t).pForwardCompat)
#define THROW_COUNT(t)          ((t).pCatchableTypeArray->nCatchableTypes)
#define THROW_CTLIST(t)         ((t).pCatchableTypeArray->arrayOfCatchableTypes)
#define THROW_PCTLIST(t)        (&THROW_CTLIST(t))
#define THROW_CT(t, n)          (*THROW_CTLIST(t)[n])
#define THROW_PCT(t, n)         (THROW_CTLIST(t)[n])


//
// Here's how to throw:
// M00HACK: _ThrowInfo is the name of the type that is 'pre-injected' into the
// compiler; since this prototype is known to the FE along with the pre-injected
// types, it has to match exactly.
//
#if _MSC_VER >= 900
#ifdef __cplusplus
extern "C" void __stdcall _CxxThrowException(void* pExceptionObject, _ThrowInfo* pThrowInfo);
#else
extern void __stdcall _CxxThrowException(void* pExceptionObject, ThrowInfo* pThrowInfo);
#endif
#else
#pragma message("HACK:  Baxter changed so that it compile with C")
#ifdef __cplusplus
// If we're not self-building, we need to use the name that we defined above.
extern "C" void __stdcall _CxxThrowException(void* pExceptionObject, ThrowInfo* pThrowInfo);
#else
extern void __stdcall _CxxThrowException(void* pExceptionObject, ThrowInfo* pThrowInfo);
#endif
#endif

#endif /* _EHDATA_NOTHROW */


#ifndef _EHDATA_NOFUNCINFO

/////////////////////////////////////////////////////////////////////////////
//
// Describing 'try/catch' blocks:
//
//---------------------------------------------------------------------------

//
// Current state of a function.
// -1 is the 'blank' state, ie there is nothing to unwind, no try blocks active.
//

typedef int __ehstate_t;        // The type of a state index

#define EH_EMPTY_STATE  -1


//
// HandlerType - description of a single 'catch'
//

typedef struct HandlerType {
    unsigned    isConst     : 1;    // Is the type referenced 'const' qualified
    unsigned    isVolatile  : 1;    // Is the type referenced 'volatile' qualified
    unsigned    isUnaligned : 1;    // Is the type referenced 'unaligned' qualified
    unsigned    isReference : 1;    // Is the catch type by reference
    unsigned    isResumable : 1;    // Might the catch choose to resume (Reserved)
    TypeDescriptor *pType;          // Pointer to the corresponding type descriptor
    ptrdiff_t   dispCatchObj;       // Displacement of catch object from base
                                    //      of current stack frame.
    void *      addressOfHandler;   // Address of 'catch' code
} HandlerType;

//
// HandlerType32 - 32 bit version of above.  This must be changed if above is 
//

typedef struct HandlerType32 {
    unsigned    isConst     : 1;    
    unsigned    isVolatile  : 1;    
    unsigned    isUnaligned : 1;    
    unsigned    isReference : 1;    
    unsigned    isResumable : 1;    
    LONG        pType;  
    LONG        dispCatchObj;       
    LONG        addressOfHandler;
} HandlerType32;

//
// HandlerType64 - 64 bit version of above.  This must be changed if above is 
//

typedef struct HandlerType64 {
    unsigned    isConst     : 1;    
    unsigned    isVolatile  : 1;    
    unsigned    isUnaligned : 1;    
    unsigned    isReference : 1;    
    unsigned    isResumable : 1;    
    LONGLONG    pType;  
    LONGLONG    dispCatchObj;       
    LONGLONG    addressOfHandler;
} HandlerType64;

#define HT_ISRESUMABLE(ht)      ((ht).isResumable)
#define HT_ISCONST(ht)          ((ht).isConst)
#define HT_ISVOLATILE(ht)       ((ht).isVolatile)
#define HT_ISUNALIGNED(ht)      ((ht).isUnaligned)
#define HT_ISREFERENCE(ht)      ((ht).isReference)
#define HT_PTD(ht)              ((ht).pType)
#define HT_DISPCATCH(ht)        ((ht).dispCatchObj)
#define HT_HANDLER(ht)          ((ht).addressOfHandler)
#define HT_NAME(ht)             (TD_NAME(*HT_PTD(ht)))
#define HT_HASH(ht)             (TD_HASH(*HT_PTD(ht)))
#define HT_IS_TYPE_ELLIPSIS(ht) TD_IS_TYPE_ELLIPSIS(HT_PTD(ht))


//
// HandlerMapEntry - associates a handler list (sequence of catches) with a
//  range of eh-states.
//

typedef struct TryBlockMapEntry {
    __ehstate_t tryLow;             // Lowest state index of try
    __ehstate_t tryHigh;            // Highest state index of try
#if !defined(TARGET_ALPHA) && !defined(TARGET_AXP64) && !defined(TARGET_IA64)
    __ehstate_t catchHigh;          // Highest state index of any associated catch
#endif
    int         nCatches;           // Number of entries in array
    HandlerType *pHandlerArray;     // List of handlers for this try
} TryBlockMapEntry;

//
// HandlerMapEntry32 - 32 bit version of above datastructure
// This must be changed if above is changed
//

typedef struct TryBlockMapEntry32 {
    __ehstate_t tryLow;             
    __ehstate_t tryHigh;            
#if !defined(TARGET_ALPHA) && !defined(TARGET_AXP64) && !defined(TARGET_IA64)
    __ehstate_t catchHigh;          
#endif
    int         nCatches;           
    LONG        pHandlerArray;     
} TryBlockMapEntry32;

//
// HandlerMapEntry64 - 64 bit version of above datastructure
// This must be changed if above is changed
//

typedef struct TryBlockMapEntry64 {
    __ehstate_t tryLow;             
    __ehstate_t tryHigh;            
#if !defined(TARGET_ALPHA) && !defined(TARGET_AXP64) && !defined(TARGET_IA64)
    __ehstate_t catchHigh;          
#endif
    int         nCatches;           
    LONGLONG    pHandlerArray;     
} TryBlockMapEntry64;

#define TBME_LOW(hm)        ((hm).tryLow)
#define TBME_HIGH(hm)       ((hm).tryHigh)
#define TBME_CATCHHIGH(hm)  ((hm).catchHigh)
#define TBME_NCATCHES(hm)   ((hm).nCatches)
#define TBME_PLIST(hm)      ((hm).pHandlerArray)
#define TBME_CATCH(hm, n)   (TBME_PLIST(hm)[n])
#define TBME_PCATCH(hm, n)  (&(TBME_PLIST(hm)[n]))


/////////////////////////////////////////////////////////////////////////////
//
// Description of the function:
//
//---------------------------------------------------------------------------

//
// UnwindMapEntry - Description of each state transition for unwinding
//  the stack (ie destructing objects).
//
// The unwind map is an array, indexed by current state.  Each entry specifies
// the state to go to during unwind, and the action required to get there.
// Note that states are represented by a signed integer, and that the 'blank'
// state is -1 so that the array remains 0-based (because by definition there
// is never any unwind action to be performed from state -1).  It is also
// assumed that state indices will be dense, ie that there will be no gaps of
// unused state indices in a function.
//

typedef struct UnwindMapEntry {
    __ehstate_t     toState;            // State this action takes us to
    void            (*action)(void);    // Funclet to call to effect state change
} UnwindTransitionEntry;

#define UWE_TOSTATE(uwe)    ((uwe).toState)
#define UWE_ACTION(uwe)     ((uwe).action)

//
// FuncInfo - all the information that describes a function with exception
//  handling information.
//

typedef struct FuncInfo
{
    unsigned int        magicNumber;        // Identifies version of compiler
    __ehstate_t         maxState;           // Highest state number plus one (thus
                                            // number of entries in unwind map)
#pragma message("HACK: Baxter changed so that it compile with C")
#ifdef __cplusplus
    UnwindMapEntry      *pUnwindMap;        // Where the unwind map is
#else
    UnwindTransitionEntry      *pUnwindMap; // Where the unwind map is
#endif
    unsigned int        nTryBlocks;         // Number of 'try' blocks in this function
    TryBlockMapEntry    *pTryBlockMap;      // Where the handler map is
#if defined(TARGET_ALPHA) || defined(TARGET_AXP64) || defined(TARGET_IA64) 
    signed int          EHContextDelta;     // Frame offset of EHContext record
#endif
    unsigned int        nIPMapEntries;      // # entries in the IP-to-state map. NYI (reserved)
    void                *pIPtoStateMap;     // An IP to state map.  NYI (reserved).
} FuncInfo;

//
// FuncInfo32 - 32 bit version of FuncInfo above.  This must be modified if
// the above is modified.
//

typedef struct FuncInfo32
{
    unsigned int        magicNumber;        
    __ehstate_t         maxState;           
    LONG                pUnwindMap;        
    unsigned int        nTryBlocks;         
    LONG                pTryBlockMap;      
#if defined(TARGET_ALPHA) || defined(TARGET_AXP64) || defined(TARGET_IA64)
    signed int          EHContextDelta;     
#endif
    unsigned int        nIPMapEntries;      
    LONG                pIPtoStateMap;     
} FuncInfo32;

//
// FuncInfo64 - 64 bit version of FuncInfo above.  This must be modified if
// the above is modified.
//

typedef struct FuncInfo64
{
    unsigned int        magicNumber;        
    __ehstate_t         maxState;           
    LONGLONG            pUnwindMap;        
    unsigned int        nTryBlocks;         
    LONGLONG            pTryBlockMap;      
#if defined(TARGET_ALPHA) || defined(TARGET_AXP64) || defined(TARGET_IA64)
    signed int          EHContextDelta;     
#endif
    unsigned int        nIPMapEntries;      
    LONGLONG            pIPtoStateMap;     
} FuncInfo64;

#define FUNC_MAGICNUM(fi)           ((fi).magicNumber)
#define FUNC_MAXSTATE(fi)       ((fi).maxState)
#define FUNC_NTRYBLOCKS(fi)     ((fi).nTryBlocks)
#define FUNC_NIPMAPENT(fi)      ((fi).nIPMapEntries)
#define FUNC_PUNWINDMAP(fi)     ((fi).pUnwindMap)
#define FUNC_PHANDLERMAP(fi)    ((fi).pTryBlockMap)
#if defined(TARGET_ALPHA) || defined(AXP64) || defined(TARGET_IA64)  //v-vadimp - IA64?
#define FUNC_EHCONTEXTDELTA(fi) ((fi).EHContextDelta)
#endif
#define FUNC_IPMAP(fi)          ((fi).pIPtoStateMap)
#define FUNC_UNWIND(fi, st)     ((fi).pUnwindMap[st])
#define FUNC_PUNWIND(fi, st)    (&FUNC_UNWIND(fi, st))
#define FUNC_TRYBLOCK(fi,n)     ((fi).pTryBlockMap[n])
#define FUNC_PTRYBLOCK(fi,n)    (&FUNC_TRYBLOCK(fi, n))
#define FUNC_IPTOSTATE(fi,n)    __ERROR_NYI__

#endif /* _EHDATA_NOFUNCINFO */


#ifndef _EHDATA_NONT

/////////////////////////////////////////////////////////////////////////////
//
// Data types that are variants of data used by NT (and Chicago) to manage
// exception handling.
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//
// A stack registration node (i386 only)
//

#if defined(TARGET_i386)
struct EHRegistrationNode {
    /* void *           stackPtr */     // Stack ptr at entry to try (below address point)
    EHRegistrationNode  *pNext;         // Next node in the chain
    void *              frameHandler;   // The handler function for this frame
    __ehstate_t         state;          // The current state of this function
#if CC_EXPLICITFRAME
    void *              frame;          // Value of ebp for this frame
#endif
};

#if !CC_EXPLICITFRAME
                // Cannonical offset
# define FRAME_OFFSET   sizeof(EHRegistrationNode)
#endif

#define PRN_NEXT(prn)       ((prn)->pNext)
#define PRN_HANDLER(prn)    ((prn)->frameHandler)
#define PRN_STATE(prn)      ((prn)->state)
#define PRN_STACK(prn)      (((void**)(prn))[-1])
#if CC_EXPLICITFRAME
# define PRN_FRAME(prn)     ((prn)->frame)
#else
# define PRN_FRAME(prn)     ((void*)(((char*)prn) + FRAME_OFFSET))
#endif

typedef void DispatcherContext;     // Meaningless on Intel

#elif defined(TARGET_ALPHA) || defined(TARGET_AXP64)
//
// On Alpha we don't have a registration node,
//     just a pointer to the stack frame base
// On Alpha, no need for the following declarations, so remove them to
//     avoid 32 bit/64 bit changes
//     EHRegistrationNode;
//     RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;
//     DispatcherContext;
//

#define PRN_NEXT(prn)           __ERROR__
#define PRN_HANDLER(prn)        __ERROR__
#define PRN_STATE(prn)          __ERROR__
#define PRN_STACK(prn)          __ERROR__
#define PRN_FRAME(prn)          __ERROR__

#define FRAME_OFFSET            0

//
// The following three declarations are all related and all must be changed
// if one is changed.
//

typedef struct _EHCONTEXT {
    ULONG State;
    PVOID Rfp;
} EHContext;

typedef struct _EHCONTEXT32 {
    ULONG State;
    LONG Rfp;
} EHContext32;

typedef struct _EHCONTEXT64 {
    ULONG State;
    LONGLONG Rfp;
} EHContext64;

// Macro to extract the saved frame pointer from a C++ stack frame
#define REAL_FP(pDC, pFuncInfo)                      \
    (((EHContext *)((char *)(pDC->EstablisherFrame)  \
     + pFuncInfo->EHContextDelta)) -> Rfp)

// Macro to extract the function state value from a C++ stack frame
#define EH_STATE(pDC, pFuncInfo)                     \
    (((EHContext *)((char *)(pDC->EstablisherFrame)  \
     + pFuncInfo->EHContextDelta)) -> State)

#elif defined(TARGET_IA64)
        
struct _CONTEXT;
struct _EXCEPTION_RECORD;

typedef
EXCEPTION_DISPOSITION
(*PEXCEPTION_ROUTINE) (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT struct _CONTEXT *ContextRecord,
    IN OUT PVOID DispatcherContext
    );

typedef struct _UNWIND_INFO {
    USHORT Version;                            // Version Number
    USHORT Flags;                              // Flags
    ULONG DataLength;                          // Length of Descriptor Data
    PVOID Descriptors;                         // Unwind Descriptors
} UNWIND_INFO, *PUNWIND_INFO;

typedef union _FRAME_POINTERS {
    struct {
        ULONG MemoryStackFp;
        ULONG BackingStoreFp;
    };
    LONGLONG FramePointers;           // used to force 8-byte alignment
} FRAME_POINTERS, *PFRAME_POINTERS;

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PEXCEPTION_ROUTINE ExceptionHandler;
    PVOID HandlerData;
    PUNWIND_INFO UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

//
// On IA64 we don't have a registration node, just a pointer to the stack
// frame base and backing store base.
//
typedef FRAME_POINTERS EHRegistrationNode;

#define PRN_NEXT(prn)		__ERROR__
#define PRN_HANDLER(prn)	__ERROR__
#define PRN_STATE(prn)		__ERROR__
#define PRN_STACK(prn)		__ERROR__
#define PRN_FRAME(prn)		__ERROR__

typedef struct _xDISPATCHER_CONTEXT {
    FRAME_POINTERS EstablisherFrame;
    ULONG ControlPc;
    struct _RUNTIME_FUNCTION *FunctionEntry;
    PCONTEXT ContextRecord;
} DispatcherContext, *pDispatcherContext;

typedef struct _EHContext {
    PVOID Psp;
    ULONG  State;
} EHContext;

#define EH_STATE_OFFSET -12

#define EH_STATE(pRN) \
    (*(int*)(pRN->MemoryStackFp + EH_STATE_OFFSET))
        
#else
#error "Machine not supported"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// The NT Exception record that we use to pass information from the throw to
// the possible catches.
//
// The constants in the comments are the values we expect.
// This is based on the definition of EXCEPTION_RECORD in winnt.h.
//

typedef struct EHExceptionRecord {
    DWORD       ExceptionCode;          // The code of this exception. (= EH_EXCEPTION_NUMBER)
    DWORD       ExceptionFlags;         // Flags determined by NT
    struct _EXCEPTION_RECORD *ExceptionRecord;  // An extra exception record (not used)
    void *      ExceptionAddress;       // Address at which exception occurred
    DWORD       NumberParameters;       // Number of extended parameters. (= EH_EXCEPTION_PARAMETERS)
    struct EHParameters {
        DWORD       magicNumber;        // = EH_MAGIC_NUMBER1
        void *      pExceptionObject;   // Pointer to the actual object thrown
        ThrowInfo   *pThrowInfo;        // Description of thrown object
        } params;
} EHExceptionRecord;

#define PER_CODE(per)       ((per)->ExceptionCode)
#define PER_FLAGS(per)      ((per)->ExceptionFlags)
#define PER_NEXT(per)       ((per)->ExceptionRecord)
#define PER_ADDRESS(per)    ((per)->ExceptionAddress)
#define PER_NPARAMS(per)    ((per)->NumberParameters)
#define PER_MAGICNUM(per)   ((per)->params.magicNumber)
#define PER_PEXCEPTOBJ(per) ((per)->params.pExceptionObject)
#define PER_PTHROW(per)     ((per)->params.pThrowInfo)
#define PER_THROW(per)      (*PER_PTHROW(per))

#define PER_ISSIMPLETYPE(t) (PER_THROW(t).isSimpleType)
#define PER_ISREFERENCE(t)  (PER_THROW(t).isReference)
#define PER_ISCONST(t)      (PER_THROW(t).isConst)
#define PER_ISVOLATILE(t)   (PER_THROW(t).isVolatile)
#define PER_ISUNALIGNED(t)  (PER_THROW(t).isUnaligned)
#define PER_UNWINDFUNC(t)   (PER_THROW(t).pmfnUnwind)
#define PER_PCTLIST(t)      (PER_THROW(t).pCatchable)
#define PER_CTLIST(t)       (*PER_PCTLIST(t))

#define PER_IS_MSVC_EH(per) ((PER_CODE(per) == EH_EXCEPTION_NUMBER) &&          \
                             (PER_NPARAMS(per) == EH_EXCEPTION_PARAMETERS) &&   \
                             (PER_MAGICNUM(per) == EH_MAGIC_NUMBER1))



/////////////////////////////////////////////////////////////////////////////
//
// NT kernel routines and definitions required to implement exception handling:
//
// (from ntxcapi.h, which is not a public header file)
//
//---------------------------------------------------------------------------

#ifndef _NTXCAPI_

// begin_ntddk
//
// Exception flag definitions.
//

// begin_winnt
#define EXCEPTION_NONCONTINUABLE 0x1    // Noncontinuable exception
// end_winnt

// end_ntddk
#define EXCEPTION_UNWINDING 0x2         // Unwind is in progress
#define EXCEPTION_EXIT_UNWIND 0x4       // Exit unwind is in progress
#define EXCEPTION_STACK_INVALID 0x8     // Stack out of limits or unaligned
#define EXCEPTION_NESTED_CALL 0x10      // Nested exception handler call
#define EXCEPTION_TARGET_UNWIND 0x20    // Target unwind in progress
#define EXCEPTION_COLLIDED_UNWIND 0x40  // Collided exception handler call

#define EXCEPTION_UNWIND (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | \
                          EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)

#define IS_UNWINDING(Flag) ((Flag & EXCEPTION_UNWIND) != 0)
#define IS_DISPATCHING(Flag) ((Flag & EXCEPTION_UNWIND) == 0)
#define IS_TARGET_UNWIND(Flag) (Flag & EXCEPTION_TARGET_UNWIND)
#define IS_EXIT_UNWIND(Flag) (Flag & EXCEPTION_EXIT_UNWIND)

#ifdef __cplusplus
extern "C" {
#endif

void WINAPI
RtlUnwind (
    IN void * TargetFrame OPTIONAL,
    IN void * TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN void * ReturnValue
    );

#ifdef __cplusplus
}
#endif

#endif /* _NTXCAPI_ */

#endif /* _EHDATA_NONT */

#pragma pack(pop, ehdata)

#endif /* __EHDATA__DEFINED__ */
