//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FX_MACROS_H_
#define _FX_MACROS_H_

//
// at is for argument type
// a is for argument
// rt is for return type
// fn or FN is for function
// qf is for qualifiers
// rtDef is for return type default value
//
#define WDF_FX_VF_SECTION_NAME PAGEWdfV
#define QUOTE_EXPANSION(tok) #tok
#define FX_VF_SECTION_NAME_QUOTED(tok) QUOTE_EXPANSION(tok)







#if defined(_M_ARM)
#define FX_VF_PAGING
#else
#define FX_VF_PAGING __declspec(code_seg(FX_VF_SECTION_NAME_QUOTED(WDF_FX_VF_SECTION_NAME)))
#endif

#define FX_VF_NAME_TO_IMP_NAME( fnName ) Vf_##fnName
#define FX_VF_NAME_TO_SCOPED_IMP_NAME( classname, fnName ) classname##::Vf_##fnName
#define FX_VF_QF_VOID
#define FX_VF_QF_NTSTATUS _Must_inspect_result_
#define FX_VF_DEFAULT_RT_VOID
#define FX_VF_DEFAULT_RT_NTSTATUS STATUS_SUCCESS

#define FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, ...)       \
qf                                                           \
rt                                                           \
FX_VF_FUNCTION( fnName )( _In_ PFX_DRIVER_GLOBALS, ##__VA_ARGS__ );

#define FX_VF_METHOD( classname, fnName )   \
FX_VF_PAGING                                \
FX_VF_NAME_TO_SCOPED_IMP_NAME( classname, fnName )

#define FX_VF_FUNCTION( fnName )  \
FX_VF_PAGING                      \
FX_VF_NAME_TO_IMP_NAME( fnName )

#define FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, params )  \
{                                                              \
    if( FxDriverGlobals->FxVerifierOn ) {                      \
        return FX_VF_NAME_TO_IMP_NAME( fnName ) params;        \
    }                                                          \
    else {                                                     \
        return rtDef;                                          \
    }                                                          \
}

//
// zero parameters
//
#define FX_VF_STUB( qf, rt, rtDef, fnName )                        \
__inline                                                           \
qf                                                                 \
rt                                                                 \
fnName( _In_ PFX_DRIVER_GLOBALS FxDriverGlobals )                  \
FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, ( FxDriverGlobals ) )

#define FX_DECLARE_VF_FUNCTION_EX( qf, rt, rtDef, fnName ) \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName )                 \
FX_VF_STUB( qf, rt, rtDef, fnName )

#define FX_DECLARE_VF_FUNCTION( rt, fnName )      \
FX_DECLARE_VF_FUNCTION_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName )

//
// 1-Parameter Stub Macro
//
#define FX_VF_STUB_P1( qf, rt, rtDef, fnName, at1 )                  \
__inline                                                             \
qf                                                                   \
rt                                                                   \
fnName( _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,  at1 a1 )           \
FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, (FxDriverGlobals, a1 ))

// 1-Parameter Extended FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P1_EX( qf, rt, rtDef, fnName, at1 ) \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, at1 )                    \
FX_VF_STUB_P1( qf, rt, rtDef, fnName, at1 )

// 1-Parameter FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P1( rt, fnName, at1 )           \
FX_DECLARE_VF_FUNCTION_P1_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName, at1 )

//
// 2-Parameter Stub Macro
//
#define FX_VF_STUB_P2( qf, rt, rtDef, fnName, at1, at2 )                  \
__inline                                                               \
qf                                                                        \
rt                                                                        \
fnName( _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,  at1 a1, at2 a2 )        \
FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, (FxDriverGlobals, a1, a2 ))

// 2-Parameter Extended FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P2_EX( qf, rt, rtDef, fnName, at1, at2 )  \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, at1, at2 )                     \
FX_VF_STUB_P2( qf, rt, rtDef, fnName, at1, at2 )

// 2-Parameter FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P2( rt, fnName, at1, at2 )        \
FX_DECLARE_VF_FUNCTION_P2_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName, at1, at2 )

//
// 3-Parameter Stub Macro
//
#define FX_VF_STUB_P3( qf, rt, rtDef, fnName, at1, at2, at3 )              \
__inline                                                                \
qf                                                                         \
rt                                                                         \
fnName( _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,  at1 a1, at2 a2, at3 a3 ) \
FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, (FxDriverGlobals, a1, a2, a3))

// 3-Parameter Extended FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P3_EX( qf, rt, rtDef, fnName, at1, at2,  at3 ) \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, at1, at2, at3)                      \
FX_VF_STUB_P3( qf, rt, rtDef, fnName, at1, at2, at3 )

// 3-Parameter FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P3( rt, fnName, at1, at2, at3 )       \
FX_DECLARE_VF_FUNCTION_P3_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName, at1, at2, at3 )

//
// 4-Parameter Stub Macro
//
#define FX_VF_STUB_P4( qf, rt, rtDef, fnName, at1, at2, at3, at4 )                  \
__inline                                                                         \
qf                                                                                  \
rt                                                                                  \
fnName( _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,  at1 a1, at2 a2, at3 a3,  at4 a4 ) \
FX_VF_GLOBAL_CHECK_SCOPE( rt, rtDef, fnName, (FxDriverGlobals, a1, a2, a3, a4))

// 4-Parameter Extended FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P4_EX( qf, rt, rtDef, fnName, at1, at2,  at3, at4 ) \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, at1, at2, at3, at4 )                     \
FX_VF_STUB_P4( qf, rt, rtDef, fnName, at1, at2, at3, at4 )

// 4-Parameter FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P4( rt, fnName, at1, at2, at3, at4 )        \
FX_DECLARE_VF_FUNCTION_P4_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName, at1, at2, at3, at4 )


#define FX_TAG 'rDxF'

#define WDFEXPORT(a) imp_ ## a
#define VFWDFEXPORT(a) imp_Vf ## a

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x) (sizeof((_x))/sizeof((_x)[0]))
#endif

// VOID
// FXVERIFY(
//     PFX_DRIVER_GLOBALS FxDriverGlobals,
//     <expression>
//     );

#define FXVERIFY(_globals, exp)                     \
{                                                   \
    if (!(exp)) {                                   \
        RtlAssert( #exp, __FILE__, __LINE__, NULL );\
    }                                               \
}

// These 2 macros are the equivalent of WDF_ALIGN_SIZE_DOWN and
// WDF_ALIGN_SIZE_UP.  The difference is that these can evaluate to a constant
// while the WDF versions will on a fre build, but not on a chk build.  By
// evaluating to a constant, we can use this in WDFCASSERT.

// size_t
// __inline
// FX_ALIGN_SIZE_DOWN_CONSTANT(
//     IN size_t Length,
//     IN size_t AlignTo
//     )
#define FX_ALIGN_SIZE_DOWN_CONSTANT(Length, AlignTo) ((Length) & ~((AlignTo) - 1))

// size_t
// __inline
// FX_ALIGN_SIZE_UP_CONSTANT(
//     IN size_t Length,
//     IN size_t AlignTo
//     )
#define FX_ALIGN_SIZE_UP_CONSTANT(Length, AlignTo)  \
    FX_ALIGN_SIZE_DOWN_CONSTANT((Length) + (AlignTo) - 1, (AlignTo))

//
// Macro which will declare a field within a structure.  This field can then be
// used to return a WDF handle to the driver since it contains the required
// FxContextHeader alongside the object itself.
//
// DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) is required because FxObject
// rounds up m_ObjectSize to be a multiple of MEMORY_ALLOCATION_ALIGNMENT.
// Since we cannot compute this value at runtime in operator new, we must
// be very careful here and force the alignment ourselves.
//
#define DEFINE_EMBEDDED_OBJECT_HANDLE(_type, _fieldname)                       \
DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _type _fieldname;                  \
DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) FxContextHeader _fieldname##Context

//
// Computes the size of an embedded object in a structure.  To be used on a
// _embeddedFieldName declared with DEFINE_EMBEDDED_OBJECT_HANDLE.
//
#define EMBEDDED_OBJECT_SIZE(_owningClass, _embeddedFieldName)          \
    (FIELD_OFFSET(_owningClass, _embeddedFieldName ## Context) -        \
        FIELD_OFFSET(_owningClass, _embeddedFieldName))
//
// Placeholder macro for a no-op
//
#define DO_NOTHING()                            (0)

// 4127 -- Conditional Expression is Constant warning
#define WHILE(constant) \
__pragma(warning(suppress: 4127)) while(constant)

#if DBG
  #if defined(_X86_)
    #define TRAP() {_asm {int 3}}
  #else
    #define TRAP() DbgBreakPoint()
  #endif
#else // DBG
  #define TRAP()
#endif // DBG

#if FX_SUPER_DBG
  #if defined(_X86_)
    #define COVERAGE_TRAP() {_asm {int 3}}
  #else
    #define COVERAGE_TRAP() DbgBreakPoint()
  #endif
#else // FX_SUPER_DBG
    #define COVERAGE_TRAP()
#endif // FX_SUPER_DBG

//
// This is a macro to keep it as cheap as possible.
//
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#define FxPointerNotNull(FxDriverGlobals, Ptr) \
    FX_VERIFY_WITH_NAME(DRIVER(BadArgument, TODO), CHECK_NOT_NULL(Ptr), \
       FxDriverGlobals->Public.DriverName)
#else
#define FxPointerNotNull(FxDriverGlobals, Ptr) \
    (((Ptr) == NULL) ? \
        FxVerifierNullBugCheck(FxDriverGlobals, _ReturnAddress()), FALSE : \
        TRUE )
#endif

#define FX_MAKE_WSTR_WORKER(x) L ## #x
#define FX_MAKE_WSTR(x) FX_MAKE_WSTR_WORKER(x)

#define FX_LITERAL_WORKER(a)    # a
#define FX_LITERAL(a) FX_LITERAL_WORKER(a)

//
// In some cases we assert for some condition to hold (such as asserting a ptr
// to be non-NULL before accessing it), but prefast will still complain
// (e.g., in the example given, about NULL ptr access).
//
// This macro combines the assert with corresponding assume for prefast.
//
#ifdef _PREFAST_
#define FX_ASSERT_AND_ASSUME_FOR_PREFAST(b) \
{ \
    bool result=(b); \
    ASSERTMSG(#b, result); \
    __assume(result == true); \
}
#else
#define FX_ASSERT_AND_ASSUME_FOR_PREFAST(b) \
{ \
    ASSERT(b); \
}
#endif

//
// Macro to make sure that the code is not invoked for UM.
//
// Although it is usually preferable to move such code to *um file
// so that it does not get compiled at all for um, in some cases such approach
// might fragment the code too much. In such situation this macro can be used.
//
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#define WDF_VERIFY_KM_ONLY_CODE() \
    Mx::MxAssertMsg("Unexpected invocation in user mode", FALSE);
#else
#define WDF_VERIFY_KM_ONLY_CODE()
#endif

//
// MIN, MAX macros.
//
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) if( NULL != p ) { ( p )->Release(); p = NULL; }
#endif

#endif // _FX_MACROS_H_
