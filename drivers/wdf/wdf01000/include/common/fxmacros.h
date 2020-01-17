#ifndef _FX_MACROS_H_
#define _FX_MACROS_H_

#define FX_TAG 'rDxF'

#define WDFEXPORT(a) imp_ ## a
#define VFWDFEXPORT(a) imp_Vf ## a

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_x) (sizeof((_x))/sizeof((_x)[0]))
#endif

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

#define FX_DECLARE_VF_FUNCTION_P1_EX( qf, rt, rtDef, fnName, at1 ) \
FX_VF_FUNCTION_PROTOTYPE( qf, rt, fnName, at1 )                    \
FX_VF_STUB_P1( qf, rt, rtDef, fnName, at1 )

// 1-Parameter FUNCTION Declaration Macro
#define FX_DECLARE_VF_FUNCTION_P1( rt, fnName, at1 )           \
FX_DECLARE_VF_FUNCTION_P1_EX( FX_VF_QF_ ## rt, rt, FX_VF_DEFAULT_RT_ ## rt, fnName, at1 )

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

#define FX_MAKE_WSTR_WORKER(x) L ## #x
#define FX_MAKE_WSTR(x) FX_MAKE_WSTR_WORKER(x)

#endif //_FX_MACROS_H_