// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:  Macros and functions used to check HRESULTs with.  The
//              behavior of these checks can me modified by changing
//              the instrumentation configuration in InstrumentationConfig.h.
//
//------------------------------------------------------------------------

//+--------------------------------------------------------------------------------------
//
//  Macro:          MIL_THR and MIL_THRX
//
//  Synopsis:       Checks an HRESULT expression using the current instrumentation 
//                  configuration.  Expression result is assigned to hr for MIL_THR
//                  or specified location for MIL_THRX;
//
//  Returns:        n/a
//
//  Example Usage:  To assign and check an HRESULT.
//
//                  MIL_THR(HrFunc());
//                  MIL_THRX(hr2, HrFunc());
//
//---------------------------------------------------------------------------------------
#define MIL_THR_ADDFLAGS(hrDest,    /* HRESULT recipient */ \
                         hrExpr,    /* HRESULT to check */ \
                         dwAddFlags /* Flags to add */) \
    MilCheckHR( \
        /* Note: ASSIGN_HR_PREFASTOKAY is used here to prevent expanding */     \
        /*       hrDest more than once, which may have side-effects.     */     \
        ASSIGN_HR_PREFASTOKAY((hrDest), (hrExpr)), \
        MILINSTRUMENTATIONFLAGS | (dwAddFlags), \
        MILINSTRUMENTATIONHRESULTLIST, \
        MILINSTRUMENTATIONHRESULTLISTSIZE, \
        __LINE__)

#define MIL_THR(hrExpr)          MIL_THR_ADDFLAGS(hr, (hrExpr), 0)
#define MIL_THRX(hrDest, hrExpr) MIL_THR_ADDFLAGS((hrDest), (hrExpr), 0)

//+--------------------------------------------------------------------------------------
//
//  Macro:          MIL_THR_SECONDARY
//
//  Synopsis:       Traces a secondary HR that has a lower priority than 'hr', assigning 
//                  it to 'hr' only when 'hr' is success.  This avoids overwriting 'hr'
//                  with the secondary hr when 'hr' is a failure.  It is primarily used
//                  when a HR-returning method has to be called during Cleanup, but
//                  we don't want to override any failure HR's that got us into the Cleanup
//                  section in the first place.
//
//  Returns:        n/a
//
//  Example Usage: 
//                      IFC(SomeFunc());
//
//                  Cleanup:
//                      {
//                          // Use MIL_THR_SECONDARY to avoid overwriting 'hr' if 'hr' is a 
//                          // failure
//                          MIL_THR_SECONDARY(SomeOtherHrFunc());
//                      }
//
//                      RRETURN(hr);
//
//---------------------------------------------------------------------------------------
#define MIL_THR_SECONDARY(hrExpr) \
    do { \
        HRESULT __hr2; \
        MIL_THRX(__hr2, (hrExpr)); \
        if (SUCCEEDED(hr) && FAILED(__hr2)) \
        { hr = __hr2; } \
    } while (UNCONDITIONAL_EXPR(0))

//+----------------------------------------------------------------------------
//
//  Macro:          MIL_TW32, MIL_TW32_NOSLE
//
//  Synopsis:       Wrapper used on Win32 expressions which assigns an error
//                  HRESULT to hr upon failure.  If expr evaluates to false it
//                  invokes the instrumentation check function and uses
//                  GetLastError to set hr to the appropriate HRESULT.  If
//                  GetLastError returns SUCCESS, hr is set to a generic error
//                  HRESULT (WGXERR_WIN32ERROR).
//
//                  MIL_TW32_NOSLE does not use SetLastError(ERROR_SUCCESS) to
//                  clear last error state before evaluating expr (which should
//                  always contain a Win32 call in the case of MIL_TW32.)
//
//                  MIL_TW32 does use SetLastError(ERROR_SUCCESS) before
//                  evaluating expr, because some Win32 PIs don't properly set
//                  the last error when returning failure.  This means any
//                  previously set error will remain and our call to
//                  GetLastError may be mislead.
//
//  Returns:        N/A
//
//  Example Usage:  To assign and check an HRESULT from a Win32 function.
//
//                  MIL_TW32(NTFunc());
//
//-----------------------------------------------------------------------------
#define MIL_TW32(expr) \
    do  { \
            SetLastError(ERROR_SUCCESS); \
            MIL_TW32_NOSLE(expr); \
        } while (UNCONDITIONAL_EXPR(0))

#define MIL_TW32_NOSLE(expr) \
    do  { \
            /* Set to var to avoid potential compiler warnings */ \
            const BOOL __fSuccess = (TW32(0, expr) != 0); /* Evaluate expr here*/ \
            if (!__fSuccess) { \
                const DWORD __dwLastError = GetLastError(); \
                /* No THR because we already did TW32 */ \
                hr = HRESULT_FROM_WIN32(__dwLastError); \
                if (SUCCEEDED(hr)) { \
                    hr = THR(WGXERR_WIN32ERROR); \
                } \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
            } \
        } while (UNCONDITIONAL_EXPR(0))


//+--------------------------------------------------------------------------------------
//
//  Macro:          MIL_CHECKHR_ADDFLAGS
//
//  Synopsis:       Checks an HRESULT variable and allows for instrumentation flags
//                  to be specified in addition to the current configuration's flags.
//
//  Returns:        nothing
//
//  Example Usage:  To check an HR and ensure the stack is captured if it is a failure:
//
//                  MIL_CHECKHR_ADDFLAGS(hr, MILINSTRUMENTATIONFLAGS_CAPTUREONFAIL);
//
//---------------------------------------------------------------------------------------
#define MIL_CHECKHR_ADDFLAGS(hrVar,     /* HRESULT to check */ \
                             dwAddFlags /* Flags to add */) \
     /* ASSIGN_HR does two important things here: */ \
     /*  1) Triggers error if hrVar is not an l-value (hopefully a variable) */ \
     /*  2) Traces the HRESULT */ \
    (ASSIGN_HR(hrVar, hrVar), \
     MilCheckHR(hrVar, \
                MILINSTRUMENTATIONFLAGS | (dwAddFlags), \
                MILINSTRUMENTATIONHRESULTLIST, \
                MILINSTRUMENTATIONHRESULTLISTSIZE, \
                __LINE__) \
    )

//+--------------------------------------------------------------------------------------
//
//  Macro:          MIL_CHECKHR
//
//  Synopsis:       Checks an HRESULT variable using the current instrumentation 
//                  configuration.
//
//  Returns:        nothing
//
//  Example Usage:  To check an HR.
//
//                  MIL_CHECKHR(hr);
//
//---------------------------------------------------------------------------------------
#define MIL_CHECKHR(hrVar /* HRESULT to check */) MIL_CHECKHR_ADDFLAGS(hrVar, 0)



// RRETURN_ADDFLAGS can be used to alter the instrumentation flags of instrumented 
// return macros only
#define RRETURN_ADDFLAGS MILINSTRUMENTATIONFLAGS_DONOTHING
//+--------------------------------------------------------------------------------------
//
//  Macro:          MILCHECK_RETURNVALUE
//
//  Synopsis:       Checks a return value using the current instrumentation configuration.  
//                  Non-S_OK SUCCESS HRESULTs are checked for in debug builds and
//                  not permitted unless explicitly specified using RRETURNX.
//
//  Returns:        N/A
//
//  Example Usage:  To return an HRESULT.
//
//                  MILCHECK_RETURNVALUE(hr);
//
//---------------------------------------------------------------------------------------

#define MILCHECK_RETURNVALUE( \
                    hrExpr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2, /* Second allowed non-S_OK success HRESULT */ \
                    s3  /* Third allowed non-S_OK success HRESULT */) \
    { \
        C_ASSERT(SUCCEEDED(s1)); \
        C_ASSERT(SUCCEEDED(s2)); \
        C_ASSERT(SUCCEEDED(s3)); \
        MilCheckReturnValue( \
                (hrExpr), \
                MILINSTRUMENTATIONFLAGS | RRETURN_ADDFLAGS, \
                MILINSTRUMENTATIONHRESULTLIST, \
                MILINSTRUMENTATIONHRESULTLISTSIZE, \
                __LINE__, \
                s1, s2, s3); \
    }


//+--------------------------------------------------------------------------------------
//
//  Macro:          RRETURN
//
//  Synopsis:       Replacement for a 'return hr' statement that checks an HRESULT using
//                  the current instrumentation configuration.  
//                  Non-S_OK success results are Assert'd in chk'd builds.  This rule is 
//                  enforced in chk'd builds because non-S_OK (e.g., S_FALSE) HR's tend
//                  to be blindly propagated to methods.  
//
//                  Consider the following descriptions of methods A, B, and C that 
//                  illustrate how blind propagation of non-S_OK success codes can be 
//                  a problem:
// 
//                  + Method A - Returns S_FALSE to indicate some special result.
//                  + Method B - Calls method A, gets S_FALSE, and just blindly propagates.
//                               This method also returns S_FALSE for some other special case
//                  + Method C - Calls method B, gets S_FALSE, and interprets it as the
//                               special value that B can return, when in fact A was the method
//                               that returned S_FALSE.
//
//  Returns:        N/A
//
//  Example Usage:  To return an HRESULT.
//
//                  RRETURN(hr);
//
//---------------------------------------------------------------------------------------
#define RRETURN(hr /* HRESULT to check */) \
    RRETURN3(hr, S_OK, S_OK, S_OK)

//+--------------------------------------------------------------------------------------
//
//  Macro:          RRETURN1
//
//  Synopsis:       Replacement for a 'return hr' statement that checks a HRESULT using
//                  the current instrumentation configuration, and specifies 1 allowed 
//                  non-S_OK success HR.  Other non-S_OK success results are Assert'd in
//                  chk'd builds.
//
//  Returns:        N/A
//
//  Example Usage:  To return an HRESULT, while allowing S_FALSE.
//
//                  RRETURN1(hr, S_FALSE);
//
//---------------------------------------------------------------------------------------
#define RRETURN1(   hr, /* HRESULT to check */ \
                    s1  /* First allowed non-S_OK success HRESULT */) \
        RRETURN3(hr, s1, S_OK, S_OK)

//+--------------------------------------------------------------------------------------
//
//  Macro:          RRETURN2
//
//  Synopsis:       Replacement for a 'return hr' statement that checks a HRESULT using
//                  the current instrumentation configuration, and specifies 2 allowed 
//                  non-S_OK success HR.  Other non-S_OK success results are Assert'd in
//                  chk'd builds.
//
//  Returns:        N/A
//
//  Example Usage:  To return an HRESULT, while allowing S_FALSE, S_SOMEFAILURE.
//
//                  RRETURN2(hr, S_FALSE, S_SOMEFAILURE);
//
//---------------------------------------------------------------------------------------
#define RRETURN2(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2  /* Second allowed non-S_OK success HRESULT */) \
        RRETURN3(hr, s1, s2, S_OK)

//+--------------------------------------------------------------------------------------
//
//  Macro:          RRETURN3
//
//  Synopsis:       Replacement for a 'return hr' statement that checks a HRESULT using
//                  the current instrumentation configuration, and specifies 3 allowed 
//                  non-S_OK success HR.  Other non-S_OK success results are Assert'd in
//                  chk'd builds.
//
//  Returns:        N/A
//
//  Example Usage:  To return an HRESULT, while allowing S_FALSE, S_SOMEFAILURE, and
//                  S_ANOTHERFAILURE
//
//                  RRETURN3(hr, S_FALSE, S_SOMEFAILURE, S_ANOTHERFAILURE);
//
//---------------------------------------------------------------------------------------

#define RRETURN3(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2, /* Second allowed non-S_OK success HRESULT */ \
                    s3  /* Third allowed non-S_OK success HRESULT */) \
    { \
        /* Use a local HRESULT in case hr is not a variable */ \
        const HRESULT __hr = (hr); \
        MILCHECK_RETURNVALUE(__hr, s1, s2, s3); \
        return __hr; \
    }

//+--------------------------------------------------------------------------------------
//
//  Macro:          INLINED_RRETURN
//
//  Synopsis:       RRETURN macro for use with inlined methods.  Because calls to inline
//                  methods are usually wrapped with IFC, using the normal RRETURN
//                  macro would cause the HR to be checked twice within a single function.
//
//                  To avoid this duplicate cost, INLINED_RRETURN evaluates to 'return hr'
//                  in free builds.
//
//  Returns:        N/A
//
//---------------------------------------------------------------------------------------
#ifdef DBG
#define INLINED_RRETURN(hr /* HRESULT to check */) \
        RRETURN(hr)
#else
#define INLINED_RRETURN(hr /* HRESULT to check */) \
        return hr
#endif

//+--------------------------------------------------------------------------------------
//
//  Macro:          INLINED_RRETURN1
//
//  Synopsis:       RRETURN1 macro for use with inlined methods.  Because calls to inline
//                  methods are usually wrapped with IFC, using the normal RRETURN1
//                  macro would cause the HR to be checked twice within a single function.
//
//                  To avoid this duplicate cost, INLINED_RRETURN1 evaluates to 'return hr'
//                  in free builds.
//
//  Returns:        N/A
//
//---------------------------------------------------------------------------------------
#ifdef DBG
#define INLINED_RRETURN1(   hr, /* HRESULT to check */ \
                    s1  /* First allowed non-S_OK success HRESULT */) \
        RRETURN1(hr, s1)
#else
#define INLINED_RRETURN1(   hr, /* HRESULT to check */ \
                    s1  /* First allowed non-S_OK success HRESULT */) \
        return hr
#endif

//+--------------------------------------------------------------------------------------
//
//  Macro:          INLINED_RRETURN2
//
//  Synopsis:       RRETURN2 macro for use with inlined methods.  Because calls to inline
//                  methods are usually wrapped with IFC, using the normal RRETURN2
//                  macro would cause the HR to be checked twice within a single function.
//
//                  To avoid this duplicate cost, INLINED_RRETURN2 evaluates to 'return hr'
//                  in free builds.
//
//  Returns:        N/A
//
//---------------------------------------------------------------------------------------
#ifdef DBG
#define INLINED_RRETURN2(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2  /* Second allowed non-S_OK success HRESULT */) \
        RRETURN2(hr, s1, s2)
#else
#define INLINED_RRETURN2(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2  /* Second allowed non-S_OK success HRESULT */) \
        return hr
#endif

//+--------------------------------------------------------------------------------------
//
//  Macro:          INLINED_RRETURN3
//
//  Synopsis:       RRETURN3 macro for use with inlined methods.  Because calls to inline
//                  methods are usually wrapped with IFC, using the normal RRETURN3
//                  macro would cause the HR to be checked twice within a single function.
//
//                  To avoid this duplicate cost, INLINED_RRETURN3 evaluates to 'return hr'
//                  in free builds.
//
//  Returns:        N/A
//
//---------------------------------------------------------------------------------------
#ifdef DBG
#define INLINED_RRETURN3(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2, /* Second allowed non-S_OK success HRESULT */ \
                    s3  /* Third allowed non-S_OK success HRESULT */) \
        RRETURN3(hr, s1, s2, s3)    
#else
#define INLINED_RRETURN3(   hr, /* HRESULT to check */ \
                    s1, /* First allowed non-S_OK success HRESULT */ \
                    s2, /* Second allowed non-S_OK success HRESULT */ \
                    s3  /* Third allowed non-S_OK success HRESULT */) \
        return hr   
#endif
   
//+--------------------------------------------------------------------------------------
//
//  Macro:          MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION
//
//  Synopsis:       Calls the HRESULT check function using the current instrumentation
//                  configuration.  This macro is used when it is already known
//                  that hr is an unsuccessful HRESULT, but the HRESULT needs to be 
//                  compared to the HRESULT list to determine if it is an unexpected failure.
//                  A unsuccessful HRESULT is an HRESULT that isn't explictly checked for
//                  inline -- currently FAILED(hr) in free builds or != S_OK in
//                  checked builds.
//
//  Returns:        void
//
//  Example Usage:  To check a unsuccessful HRESULT.
//
//                  MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr);
//
//---------------------------------------------------------------------------------------
#define MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr /* FAILED HR to check */) \
    MilInstrumentationCallHRCheckFunction( \
        (hr), \
        MILINSTRUMENTATIONFLAGS, \
        MILINSTRUMENTATIONHRESULTLIST, \
        MILINSTRUMENTATIONHRESULTLISTSIZE, \
        __LINE__ \
        ) \

//+--------------------------------------------------------------------------------------
//
//  Macro:          MILINSTRUMENTATION_HANDLEFAILEDHR
//
//  Synopsis:       Triggers the instrumentation based on the current configuration
//                  for the a unexpected failure hr.  This method is used when it is already 
//                  known that hr is an unexpected failure HRESULT.  hrFailure must be a
//                  non-S_OK HRESULT.
//
//  Returns:        void
//
//  Example Usage:  To trigger the instrumentation for a failed HRESULT.
//
//                  MILINSTRUMENTATION_HANDLEFAILEDHR(hr);
//
//---------------------------------------------------------------------------------------
#define MILINSTRUMENTATION_HANDLEFAILEDHR(hrFailure /* Failure HR to trigger instrumentation for */) \
    MilInstrumentationHandleFailure(0, (hrFailure), MILINSTRUMENTATIONFLAGS, __LINE__)

//+--------------------------------------------------------------------------------------
//
//  Macro:          CHECKPTRHRGOTO
//
//  Synopsis:       Macro that checks a pointer (ptr).  If it evaluates to NULL
//                  it sets hr to hrFailed and jumps to dest.  
//                  This macro is typically wrapped by other macros and rarely used
//                  directly.
//
//  Returns:        N/A
//
//  Example Usage:  To check pArgument for NULL.  If NULL, hr is set to E_INVALIDARG
//                  and it jumps to the Foo label.
//
//                  CHECKPTRHRGOTO(Foo, pArgument, E_INVALIDARG);
//
//---------------------------------------------------------------------------------------
#define CHECKPTRHRGOTO( dest,    /* Label to jump to upon failure */ \
                        ptr,     /* Pointer to evaluate */ \
                        hrFailed /* FAILED HRESULT to set upon failure */) \
    do { \
        if(NULL == (ptr)) { \
            hr = THR((hrFailed)); \
            MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
            goto dest; \
            } \
    } while (UNCONDITIONAL_EXPR(0))

//+--------------------------------------------------------------------------------------
//
//  Macro:          IFGOTO
//
//  Synopsis:       Macro that evaluates a HRESULT expression and jumps to dest upon
//                  failure.  It sets hr to expr, and invokes the instrumentation 
//                  check function if FAILED.
//                  This macro is typically wrapped by other macros and rarely used
//                  directly.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate HrFunc and jump to label "Foo" upon failure
//
//                  IFGOTO(Foo, HrFunc());
//
//---------------------------------------------------------------------------------------
#define IFGOTO( dest, /* Label to jump to upon failure*/ \
                expr  /* HRESULT expression to evaluate */) \
    do  { \
            hr = THR((expr)); /* Evaluate expr here*/ \
            if (FAILED(hr)) { \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
                goto dest; \
            } \
        } while (UNCONDITIONAL_EXPR(0))   

//+----------------------------------------------------------------------------
//
//  Macro:          IFNTGOTO
//
//  Synopsis:       Macro that evaluates an NTSTATUS expression and jumps to
//                  dest upon failure.  It sets hr to HRESULT equivalent of
//                  expr, and invokes the instrumentation check function if
//                  !NT_SUCCESS. This macro is typically wrapped by other
//                  macros and rarely used directly.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate NtFunc and jump to label "Foo" upon failure
//
//                  IFNTGOTO(Foo, NtFunc());
//
//-----------------------------------------------------------------------------
#define IFNTGOTO( dest, /* Label to jump to upon failure*/ \
                  expr  /* NTSTATUS expression to evaluate */) \
    do  { \
            NTSTATUS __Status = TNT((expr)); /* Evaluate expr here*/ \
            if (!NT_SUCCESS(__Status)) { \
                hr = HRESULT_FROM_NT(__Status); \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
                goto dest; \
            } \
        } while (UNCONDITIONAL_EXPR(0))   

//+----------------------------------------------------------------------------
//
//  Macro:          IFRPCGOTO
//
//  Synopsis:       Macro that evaluates an RPC_STATUS expression and jumps to
//                  dest upon failure.  It sets hr to HRESULT equivalent of
//                  expr, and invokes the instrumentation check function if
//                  status != RPC_S_OK. This macro is typically wrapped by other
//                  macros and rarely used directly.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate RpcFunc and jump to label "Foo" upon failure
//
//                  IFRPCGOTO(Foo, RpcFunc());
//
//-----------------------------------------------------------------------------
#define IFRPCGOTO( dest, /* Label to jump to upon failure*/             \
                   expr  /* RPC_STATUS expression to evaluate */)       \
    do  {                                                               \
            RPC_STATUS __Status;                                        \
            ASSIGN_FAIL(__Status, 0, expr); /* Evaluate expr here*/     \
            hr = __HRESULT_FROM_WIN32(__Status);                        \
            if (FAILED(hr)) {                                           \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr);        \
                goto dest;                                              \
            }                                                           \
        } while (UNCONDITIONAL_EXPR(0))   

//+--------------------------------------------------------------------------------------
//
//  Macro:          IFW32GOTO_NOSETLASTERROR
//
//  Synopsis:       Macro used on Win32 expressions which jumps to dest upon failure.
//                  If expr evaluates to false it invokes the instrumentation check
//                  function and uses GetLastError to set hr to the appropriate HRESULT.  
//                  If GetLastError returns SUCCESS, hr is set to a generic failed 
//                  HRESULT (WGXERR_WIN32ERROR).
//                  This macro is typically wrapped by other macros and rarely used
//                  directly.
//
//                  The _NOSETLASTERROR prefix is here to highlight that no
//                  call to SetLastError(ERROR_SUCCESS) is made to clear Win32
//                  error state before evaluating expr.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate NTFunc and jump to label "Foo" upon failure
//
//                  IFW32GOTO_NOSETLASTERROR(Foo, NTFunc());
//
//---------------------------------------------------------------------------------------    
#define IFW32GOTO_NOSETLASTERROR( dest, /* Label to jump to upon failure*/ \
                                  expr  /* Win32 expression to evaluate */) \
    do  { \
            /* Set to var to avoid potential compiler warnings */ \
            const BOOL __fSuccess = (TW32(0, expr) != 0); /* Evaluate expr here*/ \
            if (!__fSuccess) { \
                const DWORD __dwLastError = GetLastError(); \
                /* No THR because we already did TW32 */ \
                hr = HRESULT_FROM_WIN32(__dwLastError); \
                if (SUCCEEDED(hr)) { \
                    hr = THR(WGXERR_WIN32ERROR); \
                } \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
                goto dest; \
            } \
        } while (UNCONDITIONAL_EXPR(0))

//+----------------------------------------------------------------------------
//
//  Macro:          IFW32GOTO_CHECKOUTOFHANDLES
//
//  Synopsis:       Macro used on Win32k allocation calls which jumps to dest
//                  upon failure.  If expr evaluates to false it invokes the
//                  instrumentation check function and uses GetLastError to set
//                  hr to the appropriate HRESULT.  Before the Win32k call is
//                  made SetLastError is used to set the last error state for
//                  this thread to SUCCESS.  If GetLastError returns SUCCESS,
//                  GUI handle usage and quota are check to see if failure is
//                  likely do to reaching the limit, in which case hr is set to
//                  E_OUTOFMEMORY, other hr is set to a generic failed HRESULT
//                  (WGXERR_WIN32ERROR). This macro is typically wrapped by
//                  other macros and rarely used directly.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate UserCreateFunc and jump to label "Foo" upon
//                  failure
//
//                  IFW32GOTO_CHECKOUTOFHANDLES(GR_USEROBJECTS, Foo, UserCreateFunc());
//
//-----------------------------------------------------------------------------
#define IFW32GOTO_CHECKOUTOFHANDLES( type, /* GUI resource type to check use and limit */ \
                                     dest, /* Label to jump to upon failure*/ \
                                     expr  /* Win32 expression to evaluate */) \
    do  { \
            SetLastError(ERROR_SUCCESS); \
            /* Set to var to avoid potential compiler warnings */ \
            const BOOL __fSuccess = (TW32(0, expr) != 0); /* Evaluate expr here*/ \
            if (!__fSuccess) { \
                const DWORD __dwLastError = GetLastError(); \
                /* No THR because we already did TW32 */ \
                hr = HRESULT_FROM_WIN32(__dwLastError); \
                if (SUCCEEDED(hr)) { \
                    hr = THR(CheckGUIHandleQuota(type, E_OUTOFMEMORY, WGXERR_WIN32ERROR)); \
                } \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
                goto dest; \
            } \
        } while (UNCONDITIONAL_EXPR(0))

//+--------------------------------------------------------------------------------------
//
//  Macro:          CHECKPTRARG
//
//  Synopsis:       Wrapper that checks a pointer (ptr), which is typically passed into the
//                  function that uses this macro as an paraemter.  If the pointer 
//                  is NULL this macro sets hr to E_INVALIDARG and jumps to Cleanup.  
//
//  Returns:        N/A
//
//  Example Usage:  To check pArgument for NULL.  If NULL, hr is set to E_INVALIDARG
//                  and it jumps to Cleanup.
//
//                  CHECKPTRARG(pArgument));
//
//---------------------------------------------------------------------------------------
#define CHECKPTRARG(ptr) CHECKPTRHRGOTO(Cleanup, (ptr), E_INVALIDARG)               

//+--------------------------------------------------------------------------------------
//
//  Macro:          CHECKPTR
//
//  Synopsis:       Wrapper that checks a pointer (ptr).  If it is NULL this macro sets hr 
//                  to E_POINTER and jumps to Cleanup.  
//
//  Returns:        N/A
//
//  Example Usage:  To check pMemory for NULL.  If NULL, hr is set to E_POINTER
//                  and it jumps to Cleanup.
//
//                  CHECKPTR(pMemory));
//
//---------------------------------------------------------------------------------------
#define CHECKPTR(ptr) CHECKPTRHRGOTO(Cleanup, (ptr), E_POINTER)

//+--------------------------------------------------------------------------------------
//
//  Macro:          IFC
//
//  Synopsis:       Wrapper that evaluates a HRESULT expression and jumps to Cleanup upon
//                  failure.  It sets hr to expr, and invokes the instrumentation 
//                  check function if FAILED.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate HrFunc and jump to Cleanup upon failure
//
//                  IFC(HrFunc());
//
//---------------------------------------------------------------------------------------
#define IFC(expr)       IFGOTO(Cleanup, (expr))

//+----------------------------------------------------------------------------
//
//  Macro:          IFCW32, IFCW32_NOSLE
//
//  Synopsis:       Wrapper used on Win32 expressions which jumps to Cleanup
//                  upon failure. If expr evaluates to false it invokes the
//                  instrumentation check function and uses GetLastError to set
//                  hr to the appropriate HRESULT.  If GetLastError returns
//                  SUCCESS, hr is set to a generic failed HRESULT
//                  (WGXERR_WIN32ERROR).
//
//                  IFCW32_NOSLE does not use SetLastError(ERROR_SUCCESS) to
//                  clear last error state before evaluating expr (which should
//                  always contain a Win32 call in the case of IFCW32.)
//
//                  IFCW32 does use SetLastError(ERROR_SUCCESS) before
//                  evaluating expr, because some Win32 PIs don't properly set
//                  the last error when returning failure.  This means any
//                  previously set error will remain and our call to
//                  GetLastError may be mislead.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate NTFunc and jump to label "Cleanup" upon failure
//
//                  IFCW32(NTFunc());
//
//-----------------------------------------------------------------------------
#define IFCW32_NOSLE(expr)    IFW32GOTO_NOSETLASTERROR(Cleanup, (expr))
#define IFCW32(expr)    \
        do  { \
                SetLastError(ERROR_SUCCESS); \
                IFCW32_NOSLE(expr); \
            } while (UNCONDITIONAL_EXPR(0))

//+-----------------------------------------------------------------------------
//
//  Macro:          IFCW32X
// 
//  Synopsis:       Wrapper used on Win32 expressions which jumps to Cleanup
//                  upon failure. If expr evaluates to failureCode it invokes the
//                  instrumentation check function and uses GetLastError to set
//                  hr to the appropriate HRESULT.  If GetLastError returns
//                  SUCCESS, hr is set to a generic failed HRESULT
//                  (WGXERR_WIN32ERROR). 
// 
//                  Contrary to IFCW32, the result of expr evaluation is available
//                  to the macro user. Also, the failure code can be explicitly
//                  specified (like WAIT_FAILED equal to -1 in the example below).
// 
//  Example usage:  To perform special action on WaitForSingleObject timeouts
//                  but jump to Cleanup when WAIT_FAILED is returned:
// 
//                  DWORD dwWaitResult;
//                  IFCW32X(dwWaitResult, WAIT_FAILED, ::WaitForSingleObject(hEvent, INFINITE));
//                  if (dwWaitResult == WAIT_OBJECT_0) { ... }
//                  else if (dwWaitResult == WAIT_TIMEOUT) { ... }
//                  else { Assert(dwWaitResult == WAIT_ABANDONED); } // jumped to Cleanup on WAIT_FAILED
//                  
//------------------------------------------------------------------------------

#define IFCW32X( result, /* Location to store evaulated expr */ \
                 failureCode, /* The failure code that will trigger instrumentation */ \
                 expr /* Win32 expression to evaluate */) \
        do { \
            SetLastError(ERROR_SUCCESS); \
            (result) = TW32(0, expr); /* Evaluate expr here*/ \
            if ((result) == (failureCode)) { \
                const DWORD __dwLastError = GetLastError(); \
                /* No THR because we already did TW32 */ \
                hr = HRESULT_FROM_WIN32(__dwLastError); \
                if (SUCCEEDED(hr)) { \
                    hr = THR(WGXERR_WIN32ERROR); \
                } \
                MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
                goto Cleanup; \
            } \
        } while (UNCONDITIONAL_EXPR(0))


//+----------------------------------------------------------------------------
//
//  Macro:     IFCW32_CHECKSAD
//
//  Synopsis:  Special IFC macro that checks for Screen Access Denied error
//             from GDI calls
//
//             GDI returns E_HANDLE for these cases, but that isn't very
//             specific so we promote it to the very specific
//             WGXERR_SCREENACCESSDENIED.  Note that if the failure is actually
//             due to a display state change then our parent desktop RT will
//             notice that and change the HRESULT appropriately.
//

#define IFCW32_CHECKSAD(expr) \
    do { \
        if (!TW32(0, expr)) { \
            DWORD dwLastError = GetLastError(); \
            hr = HRESULT_FROM_WIN32(dwLastError); \
            if (hr == E_HANDLE) { \
                hr = THR(WGXERR_SCREENACCESSDENIED); \
            } \
            else if (SUCCEEDED(hr)) { \
                hr = THR(WGXERR_WIN32ERROR); \
            } \
            MILINSTRUMENTATION_CALLHRESULTCHECKFUNCTION(hr); \
            goto Cleanup; \
        } \
    } while(UNCONDITIONAL_EXPR(0))

//+----------------------------------------------------------------------------
//
//  Macro:          IFCW32_CHECKOOH
//
//  Synopsis:       Macro used on Win32k allocation calls which jumps to dest
//                  upon failure.  If expr evaluates to false it invokes the
//                  instrumentation check function and uses GetLastError to set
//                  hr to the appropriate HRESULT.  Before the Win32k call is
//                  made SetLastError is used to set the last error state for
//                  this thread to SUCCESS.  If GetLastError returns SUCCESS,
//                  GUI handle usage and quota are check to see if failure is
//                  likely do to reaching the limit, in which case hr is set to
//                  E_OUTOFMEMORY, other hr is set to a generic failed HRESULT
//                  (WGXERR_WIN32ERROR). This macro is typically wrapped by
//                  other macros and rarely used directly.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate GDICreateFunc and jump to label "Cleanup" upon
//                  failure
//
//                  IFCW32_CHECKOOH(GR_GDIOBJECTS, GDICreateFunc());
//
//-----------------------------------------------------------------------------
#define IFCW32_CHECKOOH(GUIType, expr) IFW32GOTO_CHECKOUTOFHANDLES(GUIType, Cleanup, expr)

//+----------------------------------------------------------------------------
//
//  Macro:          IFCNT
//
//  Synopsis:       Wrapper that evaluates a NTSTATUS expression and jumps to
//                  Cleanup upon failure.  It sets hr to HRESULT equivalent of
//                  expr, and invokes the instrumentation check function if
//                  !NT_SUCCESS.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate NtFunc and jump to Cleanup upon failure
//
//                  IFCNT(NtFunc());
//
//-----------------------------------------------------------------------------
#define IFCNT(expr)     IFNTGOTO(Cleanup, (expr))

//+----------------------------------------------------------------------------
//
//  Macro:          IFCRPC
//
//  Synopsis:       Wrapper that evaluates an RPC_STATUS expression and jumps to
//                  Cleanup upon failure.  It sets hr to HRESULT equivalent of
//                  expr, and invokes the instrumentation check function if
//                  status != RPC_S_OK.
//
//  Returns:        N/A
//
//  Example Usage:  To evaluate RpcFunc and jump to Cleanup upon failure
//
//                  IFCRPC(RpcFunc());
//
//-----------------------------------------------------------------------------
#define IFCRPC(expr)     IFRPCGOTO(Cleanup, (expr))

//+--------------------------------------------------------------------------------------
//
//  Macro:          IFCOOM
//
//  Synopsis:       Wrapper that is typically used to determine if a memory allocation 
//                  succeeded by checking a pointer (obj).  If it is NULL this macro sets
//                  hr to E_OUTOFMEMORY and jumps to Cleanup.  
//
//  Returns:        N/A
//
//  Example Usage:  To check pObj for NULL.  If NULL, hr is set to E_OUTOFMEMORY
//                  and it jumps to Cleanup.
//
//                  IFCOOM(pObj));
//
//---------------------------------------------------------------------------------------
#define IFCOOM(obj)     CHECKPTRHRGOTO(Cleanup, (obj), E_OUTOFMEMORY)

//+--------------------------------------------------------------------------------------
//
//  Macro:          IFCNULL
//
//  Synopsis:       Wrapper that is typically used to determine if a handle allocation 
//                  succeeded by checking a pointer (obj).  If it is NULL this macro sets
//                  hr to E_HANDLE and jumps to Cleanup.  
//
//  Returns:        N/A
//
//  Example Usage:  To check pRes for NULL.  If NULL, hr is set to E_HANDLE
//                  and it jumps to Cleanup.
//
//                  IFCOOM(pRes));
//
//---------------------------------------------------------------------------------------
#define IFCNULL(obj)    CHECKPTRHRGOTO(Cleanup, (obj), E_HANDLE)

//+--------------------------------------------------------------------------------------
//
//  Macro:      All *SUB* wrappers
//  
//  Synopsis:   The following "Sub" wrappers are used by functions that have multiple 
//              cleanup blocks.  Upon failure the macro jumps to SubCleanupX instead of
//              Cleanup.  
//
//  Example Usage:
//    HRESULT HrFunc()
//    {
//        HRESULT hr;
//
//        IFC(SomeCall(...));
//
//        {
//            // allocate some resources local to this
//            // block
//            IFCSUB1(SomeOtherCall(...));
//
//    SubCleanup1:
//            // clean up block local resources//           
//        }
//
//        // more code goes here
//
//    Cleanup:
//        // function level cleanup
//        RRETURN(hr);
//    }
//---------------------------------------------------------------------------------------
#define IFCSUB(x, expr)         IFGOTO(SubCleanup##x, expr)
#define IFCOOMSUB(x, obj)       CHECKPTRHRGOTO(SubCleanup##x, obj, E_OUTOFMEMORY)

#define IFCSUB1(expr)           IFCSUB(1, expr)
#define IFCSUB2(expr)           IFCSUB(2, expr)
#define IFCSUB3(expr)           IFCSUB(3, expr)
#define IFCSUB4(expr)           IFCSUB(4, expr)
#define IFCSUB5(expr)           IFCSUB(5, expr)

#define IFCOOMSUB1(obj)         IFCOOMSUB(1, obj)
#define IFCOOMSUB2(obj)         IFCOOMSUB(2, obj)
#define IFCOOMSUB3(obj)         IFCOOMSUB(3, obj)
#define IFCOOMSUB4(obj)         IFCOOMSUB(4, obj)
#define IFCOOMSUB5(obj)         IFCOOMSUB(5, obj)



