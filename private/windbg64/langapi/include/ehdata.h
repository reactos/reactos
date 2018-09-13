/***
*ehdata.h -
*
*	Copyright (c) 1993-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Declare misc. types, macros, etc. for implementation
*	of C++ Exception Handling for the run-time and the compiler.
*	Hardware independent, assumes Windows NT.
*
* Portions of this header file can be disabled by defining the following
* macros:
*	_EHDATA_NOHEADERS - suppresses inclusion of standard header files
*		If this is specified, then appropriate typedefs or macros must
*		be provided by some other means.
*	_EHDATA_NOTHROW - suppresses definitions used only to describe a throw
*	_EHDATA_NOFUNCINFO - suppresses definitions for the frame descriptor
*	_EHDATA_NONT - suppresses definitions of our version of NT's stuff
*
* Other conditional compilation macros:
*    CC_EXPLICITFRAME - if true, representation of registration node includes
*	the value of the frame-pointer for that frame, making the location
*	of the registration node on the frame flexible.  This is intended
*	primarily for early testing.
*
*       [Internal]
*
*Revision History:
*       05-20-93  BS	Module Created.
*	03-01-94  CFW	Remove CONTEXT def for x86 for TiborL.
*	03-03-94  TL	Mips (_M_MRX000 >= 4000) changes
*	09-02-94  SKS	This header file added.
*	09-12-94  GJF	Merged in changes from/for DEC (Al Doser, dated 6/20,
*			and Bill Baxter, dated 6/28).
*	11-06-94  GJF	Changed pack pragma to 8 byte alignment.
*       02-14-95  CFW   Clean up Mac merge.
*       03-22-95  PML   Add const for read-only structs
*       03-29-95  CFW   Add error message to internal headers.
*	04-14-95  JWM	Added EH_ABORT_FRAME_UNWIND_PART for EH/SEH exception handling.
*	04-20-95  TGL	Added iFrameNestLevel field to MIPS FuncInfo
*	04-27-95  JWM	EH_ABORT_FRAME_UNWIND_PART now #ifdef ALLOW_UNWIND_ABORT.
*	06-08-95  JWM	Merged CRT version of ehdata.h into langapi source.
*
****/

#ifndef _INC_EHDATA
#define _INC_EHDATA

#ifndef _CRTBLD
#ifndef _VC_VER_INC
#ifdef _M_ALPHA
#include "vcver.h"
#else
#include "..\include\vcver.h"
#endif
#endif
#endif /* _CRTBLD */


#if _M_IX86 >= 300 /*IFSTRIP=IGN*/
# ifndef CC_EXPLICITFRAME
#  define CC_EXPLICITFRAME	0	// If non-zero, we're using a hack version of the
								// registration node.
# endif
#endif

#ifndef _EHDATA_NOHEADERS
#include <stddef.h>
#include <excpt.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <nowin.h>
#endif
#endif /* _EHDATA_NOHEADERS */

#pragma pack(push, ehdata, 4)

#define EH_EXCEPTION_NUMBER	('msc' | 0xE0000000)	// The NT Exception # that we use
#define EH_MAGIC_NUMBER1	0x19930520		// The magic # identifying this version
							// As magic numbers increase, we have to keep track of
							// the versions that we are backwards compatible with.
#define EH_EXCEPTION_PARAMETERS 3			// Number of parameters in exception record

#ifdef ALLOW_UNWIND_ABORT
#define EH_ABORT_FRAME_UNWIND_PART EH_EXCEPTION_NUMBER+1
#endif

//
// PMD - Pointer to Member Data: generalized pointer-to-member descriptor
//

typedef struct PMD
{
	ptrdiff_t	mdisp;		// Offset of intended data within base
	ptrdiff_t	pdisp;		// Displacement to virtual base pointer
	ptrdiff_t	vdisp;		// Index within vbTable to offset of base
	} PMD;

//
// PMFN - Pointer to Member Function
//			M00REVIEW: we may need something more than this, but this will do for now.
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
#pragma warning(disable:4200)		// get rid of obnoxious nonstandard extension warning

typedef struct TypeDescriptor
{
#if _RTTI
	const void *	pVFTable;	// Field overloaded by RTTI
#else
	DWORD	hash;			// Hash value computed from type's decorated name
#endif
	void *	spare;			// reserved, possible for RTTI
	char	name[];			// The decorated name of the type; 0 terminated.
	} TypeDescriptor;
#pragma warning(default:4200)

#define TD_HASH(td)		((td).hash)
#define TD_NAME(td)		((td).name)

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
// 				the particular variant thrown.
// CatchableTypeArray is an array of pointers to type descriptors.  It will
//				be shared between objects thrown by reference but with varying
//				qualifiers.
// CatchableType is the description of an individual type, and how to effect
//				the conversion from a given type.
//
//---------------------------------------------------------------------------


//
// CatchableType - description of a type that can be caught.
//
// Note:  although isSimpleType can be part of ThrowInfo, it is more
//		  convenient for the run-time to have it here.
//

typedef const struct _s_CatchableType {
	unsigned int	properties;				// Catchable Type properties (Bit field)
	TypeDescriptor *pType;					// Pointer to the type descriptor for this type
	PMD 			thisDisplacement;		// Pointer to instance of catch type within
											//		thrown object.
	int				sizeOrOffset;			// Size of simple-type object or offset into
											//  buffer of 'this' pointer for catch object
	PMFN			copyFunction;			// Copy constructor or CC-closure
} CatchableType;

#define CT_IsSimpleType			0x00000001		// type is a simple type
#define CT_ByReferenceOnly		0x00000002		// type must be caught by reference
#define CT_HasVirtualBase		0x00000004		// type is a class with virtual bases

#define CT_PROPERTIES(ct)	((ct).properties)
#define CT_PTD(ct)			((ct).pType)
#define CT_THISDISP(ct)		((ct).thisDisplacement)
#define CT_SIZE(ct)			((ct).sizeOrOffset)
#define CT_COPYFUNC(ct)		((ct).copyFunction)
#define CT_OFFSET(ct)		((ct).sizeOrOffset)
#define CT_HASH(ct)			(TD_HASH(*CT_PTD(ct)))
#define CT_NAME(ct)			(TD_NAME(*CT_PTD(ct)))

#define SET_CT_ISSIMPLETYPE(ct)		(CT_PROPERTIES(ct) |= CT_IsSimpleType)
#define SET_CT_BYREFONLY(ct)		(CT_PROPERTIES(ct) |= CT_ByReferenceOnly)
#define SET_CT_HASVB(ct)			(CT_PROPERTIES(ct) |= CT_HasVirtualBase)

#define CT_ISSIMPLETYPE(ct)			(CT_PROPERTIES(ct) & CT_IsSimpleType)		// Is it a simple type?
#define CT_BYREFONLY(ct)			(CT_PROPERTIES(ct) & CT_ByReferenceOnly)	// Must it be caught by reference?
#define CT_HASVB(ct)				(CT_PROPERTIES(ct) & CT_HasVirtualBase)		// Is this type a class with virtual bases?

//
// CatchableTypeArray - array of pointers to catchable types, with length
//
#pragma warning(disable:4200)		// get rid of obnoxious nonstandard extension warning
typedef const struct _s_CatchableTypeArray {
	int	nCatchableTypes;
	CatchableType	*arrayOfCatchableTypes[];
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

typedef const struct _s_ThrowInfo {
	unsigned int	attributes;			// Throw Info attributes (Bit field)
	PMFN			pmfnUnwind;			// Destructor to call when exception
										// has been handled or aborted.

	int	(__cdecl*pForwardCompat)(...);	// Forward compatibility frame handler

	CatchableTypeArray	*pCatchableTypeArray;	// Pointer to list of pointers to types.
} ThrowInfo;

#define TI_IsConst			0x00000001		// thrown object has const qualifier
#define TI_IsVolatile		0x00000002		// thrown object has volatile qualifier
#define TI_IsUnaligned		0x00000004		// thrown object has unaligned qualifier

#define THROW_ATTRS(t)			((t).attributes)
#define THROW_UNWINDFUNC(t)		((t).pmfnUnwind)
#define THROW_FORWARDCOMPAT(t)	((t).pForwardCompat)
#define THROW_COUNT(t)			((t).pCatchableTypeArray->nCatchableTypes)
#define THROW_CTLIST(t)			((t).pCatchableTypeArray->arrayOfCatchableTypes)
#define THROW_PCTLIST(t)		(&THROW_CTLIST(t))
#define THROW_CT(t, n)			(*THROW_CTLIST(t)[n])
#define THROW_PCT(t, n)			(THROW_CTLIST(t)[n])

#define SET_TI_ISCONST(t)		(THROW_ATTRS(t) |= TI_IsConst)		// Is the object thrown 'const' qualified
#define SET_TI_ISVOLATILE(t)	(THROW_ATTRS(t) |= TI_IsVolatile)	// Is the object thrown 'volatile' qualified
#define SET_TI_ISUNALIGNED(t)	(THROW_ATTRS(t) |= TI_IsUnaligned)	// Is the object thrown 'unaligned' qualified

#define THROW_ISCONST(t)		(THROW_ATTRS(t) & TI_IsConst)
#define THROW_ISVOLATILE(t)		(THROW_ATTRS(t) & TI_IsVolatile)
#define THROW_ISUNALIGNED(t)	(THROW_ATTRS(t) & TI_IsUnaligned)

//
// Here's how to throw:
// M00HACK: _ThrowInfo is the name of the type that is 'pre-injected' into the
// compiler; since this prototype is known to the FE along with the pre-injected
// types, it has to match exactly.
//
#if _MSC_VER >= 900 /*IFSTRIP=IGN*/
extern "C" void __stdcall _CxxThrowException(void* pExceptionObject, _ThrowInfo* pThrowInfo);
#else
// If we're not self-building, we need to use the name that we defined above.
extern "C" void __stdcall _CxxThrowException(void* pExceptionObject, ThrowInfo* pThrowInfo);
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

typedef int __ehstate_t;		// The type of a state index

#define EH_EMPTY_STATE	-1


//
// HandlerType - description of a single 'catch'
//

typedef const struct _s_HandlerType {
	unsigned int	adjectives;			// Handler Type adjectives (bitfield)
	TypeDescriptor	*pType;				// Pointer to the corresponding type descriptor
	ptrdiff_t		dispCatchObj;		// Displacement of catch object from base
										//		of current stack frame.
#if _M_MRX000 >= 4000	 /*IFSTRIP=IGN*/
	ULONG			frameNestLevel;		// The static nesting level of parent function
#endif
	void *			addressOfHandler;	// Address of 'catch' code
} HandlerType;

#define HT_IsConst			0x00000001		// type referenced is 'const' qualified
#define HT_IsVolatile		0x00000002		// type referenced is 'volatile' qualified
#define HT_IsUnaligned		0x00000004		// type referenced is 'unaligned' qualified
#define HT_IsReference		0x00000008		// catch type is by reference
#define HT_IsResumable		0x00000010		// the catch may choose to resume (Reserved)

#define HT_ADJECTIVES(ht)		((ht).adjectives)
#define HT_PTD(ht)				((ht).pType)
#define HT_DISPCATCH(ht)		((ht).dispCatchObj)
#if _M_MRX000 >= 4000	 /*IFSTRIP=IGN*/
#define HT_FRAMENEST(ht)		((ht).frameNestLevel)
#endif
#define HT_HANDLER(ht)			((ht).addressOfHandler)
#define HT_NAME(ht)				(TD_NAME(*HT_PTD(ht)))
#define HT_HASH(ht)				(TD_HASH(*HT_PTD(ht)))
#define HT_IS_TYPE_ELLIPSIS(ht)	TD_IS_TYPE_ELLIPSIS(HT_PTD(ht))

#define SET_HT_ISCONST(ht)		(HT_ADJECTIVES(ht) |= HT_IsConst)
#define SET_HT_ISVOLATILE(ht)	(HT_ADJECTIVES(ht) |= HT_IsVolatile)
#define SET_HT_ISUNALIGNED(ht)	(HT_ADJECTIVES(ht) |= HT_IsUnaligned)
#define SET_HT_ISREFERENCE(ht)	(HT_ADJECTIVES(ht) |= HT_IsReference)
#define SET_HT_ISRESUMABLE(ht)	(HT_ADJECTIVES(ht) |= HT_IsResumable)

#define HT_ISCONST(ht)			(HT_ADJECTIVES(ht) & HT_IsConst)		// Is the type referenced 'const' qualified
#define HT_ISVOLATILE(ht)		(HT_ADJECTIVES(ht) & HT_IsVolatile)		// Is the type referenced 'volatile' qualified
#define HT_ISUNALIGNED(ht)		(HT_ADJECTIVES(ht) & HT_IsUnaligned)	// Is the type referenced 'unaligned' qualified
#define HT_ISREFERENCE(ht)		(HT_ADJECTIVES(ht) & HT_IsReference)	// Is the catch type by reference
#define HT_ISRESUMABLE(ht)		(HT_ADJECTIVES(ht) & HT_IsResumable)	// Might the catch choose to resume (Reserved)

//
// HandlerMapEntry - associates a handler list (sequence of catches) with a
//	range of eh-states.
//

typedef const struct _s_TryBlockMapEntry {
	__ehstate_t	tryLow;				// Lowest state index of try
	__ehstate_t	tryHigh;			// Highest state index of try
#if !defined(_M_ALPHA) && !defined(_M_IA64)
	__ehstate_t	catchHigh;			// Highest state index of any associated catch
#endif
	int			nCatches;			// Number of entries in array
	HandlerType *pHandlerArray;		// List of handlers for this try
} TryBlockMapEntry;

#define TBME_LOW(hm)		((hm).tryLow)
#define TBME_HIGH(hm)		((hm).tryHigh)
#if  !defined(_M_IA64)
#define TBME_CATCHHIGH(hm)	((hm).catchHigh)
#endif // !defined(_M_IA64)
#define TBME_NCATCHES(hm)	((hm).nCatches)
#define TBME_PLIST(hm)		((hm).pHandlerArray)
#define TBME_CATCH(hm, n)	(TBME_PLIST(hm)[n])
#define TBME_PCATCH(hm, n)	(&(TBME_PLIST(hm)[n]))


/////////////////////////////////////////////////////////////////////////////
//
// Description of the function:
//
//---------------------------------------------------------------------------

//
// UnwindMapEntry - Description of each state transition for unwinding
//	the stack (ie destructing objects).
//
// The unwind map is an array, indexed by current state.  Each entry specifies
// the state to go to during unwind, and the action required to get there.
// Note that states are represented by a signed integer, and that the 'blank'
// state is -1 so that the array remains 0-based (because by definition there
// is never any unwind action to be performed from state -1).  It is also
// assumed that state indices will be dense, ie that there will be no gaps of
// unused state indices in a function.
//

typedef const struct _s_UnwindMapEntry {
	__ehstate_t		toState;			// State this action takes us to
	void			(*action)(void);	// Funclet to call to effect state change
} UnwindMapEntry;

#define UWE_TOSTATE(uwe)	((uwe).toState)
#define UWE_ACTION(uwe)		((uwe).action)

#if _M_MRX000 >= 4000 || defined(_M_MPPC) || defined(_M_PPC)	 /*IFSTRIP=IGN*/
typedef struct IptoStateMapEntry {
	ULONG		Ip;
	__ehstate_t	State;
} IptoStateMapEntry;
#endif

//
// FuncInfo - all the information that describes a function with exception
//	handling information.
//

typedef const struct _s_FuncInfo
{
	unsigned int		magicNumber;		// Identifies version of compiler
	__ehstate_t			maxState;			// Highest state number plus one (thus
											// number of entries in unwind map)
	UnwindMapEntry		*pUnwindMap;		// Where the unwind map is
	unsigned int		nTryBlocks;			// Number of 'try' blocks in this function
	TryBlockMapEntry	*pTryBlockMap;		// Where the handler map is
#if defined(_M_ALPHA)
    signed int          EHContextDelta;     // Frame offset of EHContext record
#endif
	unsigned int		nIPMapEntries;		// # entries in the IP-to-state map. NYI (reserved)
#if _M_MRX000 >= 4000	 /*IFSTRIP=IGN*/
	IptoStateMapEntry	*pIPtoStateMap;     // An IP to state map..
	ptrdiff_t			dispUnwindHelp;		// Displacement of unwind helpers from base
	int					iTryBlockIndex;		// Used by catch functions only
	int					iFrameNestLevel;	// The static nesting level of parent function
#elif defined(_M_MPPC) || defined(_M_PPC)
	IptoStateMapEntry	*pIPtoStateMap;     // An IP to state map..
#else
	void				*pIPtoStateMap;		// An IP to state map.  NYI (reserved).
#endif
} FuncInfo;

#define FUNC_MAGICNUM(fi)			((fi).magicNumber)
#define FUNC_MAXSTATE(fi)		((fi).maxState)
#define FUNC_NTRYBLOCKS(fi)		((fi).nTryBlocks)
#define FUNC_NIPMAPENT(fi)		((fi).nIPMapEntries)
#define FUNC_PUNWINDMAP(fi)		((fi).pUnwindMap)
#define FUNC_PHANDLERMAP(fi)	((fi).pTryBlockMap)
#if defined(_M_ALPHA)
#define FUNC_EHCONTEXTDELTA(fi) ((fi).EHContextDelta)
#endif
#define FUNC_IPMAP(fi)			((fi).pIPtoStateMap)
#define FUNC_UNWIND(fi, st)		((fi).pUnwindMap[st])
#define FUNC_PUNWIND(fi, st)	(&FUNC_UNWIND(fi, st))
#define FUNC_TRYBLOCK(fi,n)		((fi).pTryBlockMap[n])
#define FUNC_PTRYBLOCK(fi,n)	(&FUNC_TRYBLOCK(fi, n))
#if _M_MRX000 >= 4000		 /*IFSTRIP=IGN*/
#define FUNC_IPTOSTATE(fi,n)	((fi).pIPtoStateMap[n])
#define FUNC_PIPTOSTATE(fi,n)	(&FUNC_IPTOSTATE(fi,n))
#define FUNC_DISPUNWINDHELP(fi)	((fi).dispUnwindHelp)
#define FUNC_TRYBLOCKINDEX(fi)	((fi).iTryBlockIndex)
#define FUNC_FRAMENEST(fi)		((fi).iFrameNestLevel)
#elif defined(_M_MPPC) || defined(_M_PPC)
#define FUNC_IPTOSTATE(fi,n)	((fi).pIPtoStateMap[n])
#define FUNC_PIPTOSTATE(fi,n)	(&FUNC_IPTOSTATE(fi,n))
#elif defined(_M_IA64)
#define FUNC_UNWIND(fi, st)		((fi).pUnwindMap[st])
#define FUNC_PUNWIND(fi, st)	(&FUNC_UNWIND(fi, st))
#define FUNC_TRYBLOCK(fi,n)		((fi).pTryBlockMap[n])
#define FUNC_PTRYBLOCK(fi,n)	(&FUNC_TRYBLOCK(fi, n))
#else
#define FUNC_IPTOSTATE(fi,n) 	__ERROR_NYI__
#endif

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

#if _M_IX86 >= 300 /*IFSTRIP=IGN*/
struct EHRegistrationNode {
	/* void *			stackPtr */		// Stack ptr at entry to try (below address point)
	EHRegistrationNode	*pNext;			// Next node in the chain
	void *				frameHandler;	// The handler function for this frame
	__ehstate_t			state;			// The current state of this function
#if CC_EXPLICITFRAME
	void *				frame;			// Value of ebp for this frame
#endif
};

#if !CC_EXPLICITFRAME
				// Cannonical offset
# define FRAME_OFFSET	sizeof(EHRegistrationNode)
#endif

#define PRN_NEXT(prn)		((prn)->pNext)
#define PRN_HANDLER(prn)	((prn)->frameHandler)
#define PRN_STATE(prn)		((prn)->state)
#define PRN_STACK(prn)		(((void**)(prn))[-1])
#if CC_EXPLICITFRAME
# define PRN_FRAME(prn)		((prn)->frame)
#else
# define PRN_FRAME(prn)		((void*)(((char*)prn) + FRAME_OFFSET))
#endif

typedef void DispatcherContext;		// Meaningless on Intel

#elif _M_MRX000 >= 4000 /*IFSTRIP=IGN*/
//
// On MIPS we don't have a registration node, just a pointer to the stack frame base
//
typedef ULONG EHRegistrationNode;

#define PRN_NEXT(prn)		__ERROR__
#define PRN_HANDLER(prn)	__ERROR__
#define PRN_STATE(prn)		__ERROR__
#define PRN_STACK(prn)		__ERROR__
#define PRN_FRAME(prn)		__ERROR__

#define FRAME_OFFSET		0
#if !defined(_NTSUBSET_)
typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    EXCEPTION_DISPOSITION (*ExceptionHandler)();
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;
#endif

typedef struct _xDISPATCHER_CONTEXT {
    ULONG ControlPc;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG EstablisherFrame;
    PCONTEXT ContextRecord;
} DispatcherContext;					// changed the case of the name to conform to EH conventions

#elif defined(_M_ALPHA)
//
// On Alpha we don't have a registration node,
//     just a pointer to the stack frame base
//
typedef ULONG_PTR EHRegistrationNode;

#define PRN_NEXT(prn)           __ERROR__
#define PRN_HANDLER(prn)        __ERROR__
#define PRN_STATE(prn)          __ERROR__
#define PRN_STACK(prn)          __ERROR__
#define PRN_FRAME(prn)          __ERROR__

#define FRAME_OFFSET            0
#if !defined(_NTSUBSET_)
typedef struct _RUNTIME_FUNCTION {
    ULONG_PTR BeginAddress;
    ULONG_PTR EndAddress;
    EXCEPTION_DISPOSITION (*ExceptionHandler)();
    PVOID HandlerData;    // ptr to FuncInfo record
    ULONG_PTR PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;
#endif

typedef struct _xDISPATCHER_CONTEXT {
    ULONG_PTR ControlPc;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG_PTR EstablisherFrame;  // Virtual Frame Pointer
    PCONTEXT ContextRecord;
} DispatcherContext;            // changed the case of the name to conform to EH conventions

//
// _EHCONTEXT is a struct built in the frame by the compiler.
// On entry to a function, compiler generated code stores the
// address of the base of the fixed frame area (the so-called
// Real Frame Pointer) into the Rfp. On every state transition,
// compiler generated code stores the current state index into
// the State field.
//
// The FuncInfo record for the function contains the offset of
// the _EHCONTEXT record from the Virtual Frame Pointer - a
// pointer to the highest address of the frame so this offset
// is negative (frames grow down in the address space).
//
typedef struct _EHCONTEXT {
    ULONG State;
    PVOID Rfp;
} EHContext;

#define VIRTUAL_FP(pDC) (pDC->EstablisherFrame)

#define REAL_FP(VirtualFP, pFuncInfo)           \
    (((EHContext *)((char *)VirtualFP           \
     + pFuncInfo->EHContextDelta)) -> Rfp)

#define EH_STATE(VirtualFP, pFuncInfo)          \
    (((EHContext *)((char *)VirtualFP           \
     + pFuncInfo->EHContextDelta)) -> State)

#elif defined(_M_M68K)
struct EHRegistrationNode {
/*	void * 				_sp;			// The stack pointer for the entry of try/catch	*/
	void *				frameHandler;	// The handler function for this frame
	__ehstate_t			state;			// The current state of this function
};

#define PRN_HANDLER(prn)	((prn)->frameHandler)
#define PRN_STATE(prn)		((prn)->state)

typedef void DispatcherContext;		// Meaningless on Mac


#elif defined(_M_PPC) || defined(_M_MPPC)
//
// On PowerPC we don't have a registration node, just a pointer to the stack
// frame base
//
typedef ULONG EHRegistrationNode;

#define PRN_NEXT(prn)		__ERROR__
#define PRN_HANDLER(prn)	__ERROR__
#define PRN_STATE(prn)		__ERROR__
#define PRN_STACK(prn)		__ERROR__
#define PRN_FRAME(prn)		__ERROR__

#define FRAME_OFFSET		0

#if !defined(_NTSUBSET_)
typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    EXCEPTION_DISPOSITION (*ExceptionHandler)(...);
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;
#endif

typedef struct _xDISPATCHER_CONTEXT {
    ULONG ControlPc;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG EstablisherFrame;
    PCONTEXT ContextRecord;
} DispatcherContext;
    // changed the case of the name to conform to EH conventions

#if defined(_M_MPPC)
typedef struct _ftinfo {
    ULONG dwMagicNumber;                // magic number
    void *pFrameInfo;			// pointer to runtime frame info
    PRUNTIME_FUNCTION rgFuncTable;	// function table
    ULONG cFuncTable;			// number of function entry
    ULONG dwEntryCF;			// address of starting of the code fragment
    ULONG dwSizeCF;			// size of the code fragment
} FTINFO, *PFTINFO;

#define offsFTINFO              64
#endif

#elif defined(_M_IA64)

#if !defined(_NTSUBSET_)

struct _CONTEXT;
struct _EXCEPTION_RECORD;
typedef __int64 ULONGLONG;

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
    ULONGLONG FramePointers;           // used to force 8-byte alignment
} FRAME_POINTERS, *PFRAME_POINTERS;

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PEXCEPTION_ROUTINE ExceptionHandler;
    PVOID HandlerData;
    PUNWIND_INFO UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

#endif

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
    LONG  State;
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
	DWORD		ExceptionCode;			// The code of this exception. (= EH_EXCEPTION_NUMBER)
	DWORD		ExceptionFlags;			// Flags determined by NT
    struct _EXCEPTION_RECORD *ExceptionRecord;	// An extra exception record (not used)
    void * 		ExceptionAddress;		// Address at which exception occurred
    DWORD 		NumberParameters;		// Number of extended parameters. (= EH_EXCEPTION_PARAMETERS)
	struct EHParameters {
		DWORD		magicNumber;		// = EH_MAGIC_NUMBER1
		void *		pExceptionObject;	// Pointer to the actual object thrown
		ThrowInfo	*pThrowInfo;		// Description of thrown object
		} params;
} EHExceptionRecord;

#define PER_CODE(per)		((per)->ExceptionCode)
#define PER_FLAGS(per)		((per)->ExceptionFlags)
#define PER_NEXT(per)		((per)->ExceptionRecord)
#define PER_ADDRESS(per)	((per)->ExceptionAddress)
#define PER_NPARAMS(per)	((per)->NumberParameters)
#define PER_MAGICNUM(per)	((per)->params.magicNumber)
#define PER_PEXCEPTOBJ(per)	((per)->params.pExceptionObject)
#define PER_PTHROW(per)		((per)->params.pThrowInfo)
#define PER_THROW(per)		(*PER_PTHROW(per))

#define PER_ISSIMPLETYPE(t)	(PER_THROW(t).isSimpleType)
#define PER_ISREFERENCE(t)	(PER_THROW(t).isReference)
#define PER_ISCONST(t)		(PER_THROW(t).isConst)
#define PER_ISVOLATILE(t)	(PER_THROW(t).isVolatile)
#define PER_ISUNALIGNED(t)	(PER_THROW(t).isUnaligned)
#define PER_UNWINDFUNC(t)	(PER_THROW(t).pmfnUnwind)
#define PER_PCTLIST(t)		(PER_THROW(t).pCatchable)
#define PER_CTLIST(t)		(*PER_PCTLIST(t))

#define PER_IS_MSVC_EH(per)	((PER_CODE(per) == EH_EXCEPTION_NUMBER) && 			\
		 					 (PER_NPARAMS(per) == EH_EXCEPTION_PARAMETERS) &&	\
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

#if !defined(_M_M68K)
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

#if defined(_M_ALPHA)
#define STATUS_UNWIND 0xc0000027

void WINAPI
RtlUnwindRfp (
    IN void * TargetRealFrame OPTIONAL,
    IN void * TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN void * ReturnValue
    );
#endif

#if defined(_M_PPC)
ULONG WINAPI
RtlVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PVOID ContextPointers OPTIONAL,
    IN ULONG LowStackLimit,
    IN ULONG HighStackLimit
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG ControlPc
    );
#endif

#if defined(_M_MPPC)
ULONG WINAPI
RtlVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PVOID ContextPointers OPTIONAL,
    IN ULONG LowStackLimit,
    IN ULONG HighStackLimit
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN PRUNTIME_FUNCTION RuntimeFunction,
    IN ULONG ControlPc,
    IN ULONG Rtoc
    );

VOID
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    );
#endif

#ifdef __cplusplus
}
#endif
#endif

#endif /* _NTXCAPI_ */

#endif /* _EHDATA_NONT */

#pragma pack(pop, ehdata)

#endif /* _INC_EHDATA */
