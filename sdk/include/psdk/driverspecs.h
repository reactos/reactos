/*
 * PROJECT:         ReactOS DDK
 * COPYRIGHT:       This file is in the Public Domain.
 * FILE:            include/psdk/driverspecs.h
 * ABSTRACT:        This header stubs out Driver Verifier annotations to
 *                  allow drivers using them to compile with our header set.
 */

#pragma once
#define DRIVERSPECS_H

#ifndef SPECSTRINGS_H
#include <specstrings.h>
#endif

//#include "sdv_driverspecs.h"
#include <concurrencysal.h>

#ifdef _PREFAST_

/* IRQL annotations are only valid when included from kernelspecs.h */
#define _IRQL_always_function_max_(irql)
#define _IRQL_always_function_min_(irql)
#define _IRQL_is_cancel_
#define _IRQL_raises_(irql)
#define _IRQL_requires_(irql)
#define _IRQL_requires_max_(irql)
#define _IRQL_requires_max_defined
#define _IRQL_requires_min_(irql)
#define _IRQL_requires_same_
#define _IRQL_restores_
#define _IRQL_restores_global_(kind,param)
#define _IRQL_saves_
#define _IRQL_saves_global_(kind,param)
#define _IRQL_uses_cancel_
#define __drv_setsIRQL(irql)

#define _Dispatch_type_(x)                          _Function_class_(x)
#define _Kernel_clear_do_init_(yesNo)               _Post_ _SA_annotes1(SAL_clearDoInit,yesNo)
#define _Kernel_float_restored_                     _Post_ _SA_annotes0(SAL_floatRestored)
#define _Kernel_float_saved_                        _Post_ _SA_annotes0(SAL_floatSaved)
#define _Kernel_float_used_                         _Post_ _SA_annotes0(SAL_floatUsed)
#define _Kernel_IoGetDmaAdapter_                    _Post_ _SA_annotes0(SAL_IoGetDmaAdapter)
#define _Kernel_releases_resource_(kind)            _Post_ _SA_annotes1(SAL_release, #kind)
#define _Kernel_requires_resource_held_(kind)       _Pre_ _SA_annotes1(SAL_mustHold, #kind)
#define _Kernel_requires_resource_not_held_(kind)   _Pre_ _SA_annotes1(SAL_neverHold, #kind)
#define _Kernel_acquires_resource_(kind)            _Post_ _SA_annotes1(SAL_acquire, #kind)
#define _Landmark_(name)
#define __drv_acquiresCancelSpinLock                _Acquires_nonreentrant_lock_(_Global_cancel_spin_lock_)
#define __drv_acquiresCriticalRegion                _Acquires_lock_(_Global_critical_region_)
#define __drv_acquiresExclusiveResource(kind)       _Acquires_nonreentrant_lock_(_Curr_)
#define __drv_acquiresExclusiveResourceGlobal(kind,param)   _Acquires_nonreentrant_lock_(param)
#define __drv_acquiresPriorityRegion                _Acquires_lock_(_Global_priority_region_)
#define __drv_acquiresResource(kind)                _Acquires_lock_(_Curr_)
#define __drv_acquiresResourceGlobal(kind,param)    _Acquires_lock_(param)
#define __drv_aliasesMem                            _Post_ _SA_annotes0(SAL_IsAliased)
#define __drv_allocatesMem(kind)                    _Post_ _SA_annotes1(SAL_NeedsRelease,__yes)
#define __drv_arg(expr,annotes)                     _At_(expr,annotes)
#define __drv_at(expr,annotes)                      _At_(expr,annotes)
#define __drv_callbackType(kind)                    _SA_annotes1(SAL_callbackType, #kind)
#define __drv_clearDoInit                           _Kernel_clear_do_init_
#define __drv_completionType(kindlist)              _SA_annotes1(SAL_completionType, #kindlist)
#define __drv_constant                              _Literal_
#define __drv_defined(x)                            _Macro_defined_(#x)
#define __drv_deref(annotes)                        __deref _Group_(annotes)
#define __drv_dispatchType_other                    _Dispatch_type_(IRP_MJ_OTHER)
#define __drv_dispatchType(x)                       _Dispatch_type_(x)
#define __drv_floatRestored                         _Kernel_float_restored_
#define __drv_floatSaved                            _Kernel_float_saved_
#define __drv_floatUsed                             _Kernel_float_used_
#define __drv_formatString(kind)                    _SA_annotes1(SAL_IsFormatString, #kind)
#define __drv_freesMem(kind)                        _Post_ _SA_annotes1(SAL_NeedsRelease,__no)
#define __drv_fun(annotes)                          _At_(return, annotes)
#define __drv_functionClass(x)                      _Function_class_(x)
#define __drv_holdsCancelSpinLock()                 _Holds_resource_global_("CancelSpinLock",)
#define __drv_holdsCriticalRegion()                 _Holds_resource_global_("CriticalRegion",)
#define __drv_holdsPriorityRegion()                 _Holds_resource_global_("PriorityRegion",)
#define __drv_in_deref(annotes)                     _Pre_ __deref _Group_(annotes)
#define __drv_in(annotes)                           _Pre_ _Group_(annotes)
#define __drv_innerAcquiresGlobal(kind,param)       _Post_ _SA_annotes2(SAL_acquireGlobal, #kind, param\t)
#define __drv_innerMustHoldGlobal(kind,param)
#define __drv_innerNeverHoldGlobal(kind,param)
#define __drv_innerReleasesGlobal(kind,param)
#define __drv_interlocked
#define __drv_inTry
#define __drv_IoGetDmaAdapter
#define __drv_isCancelIRQL                          _IRQL_is_cancel_
#define __drv_isObjectPointer
#define __drv_KMDF
#define __drv_maxFunctionIRQL(irql)
#define __drv_maxIRQL(irql)
#define __drv_minFunctionIRQL(irql)
#define __drv_minIRQL(irql)
#define __drv_Mode_impl(x)
#define __drv_mustHold(kind)
#define __drv_mustHoldCancelSpinLock
#define __drv_mustHoldCriticalRegion
#define __drv_mustHoldGlobal(kind,param)
#define __drv_mustHoldPriorityRegion
#define __drv_NDIS
#define __drv_neverHold(kind)
#define __drv_neverHoldCancelSpinLock
#define __drv_neverHoldCriticalRegion
#define __drv_neverHoldGlobal(kind,param)
#define __drv_neverHoldPriorityRegion
#define __drv_nonConstant
#define __drv_notInTry
#define __drv_notPointer
#define __drv_out_deref(annotes)
#define __drv_out(annotes)
#define __drv_preferredFunction(func,why)
#define __drv_raisesIRQL(irql)
#define __drv_releasesCancelSpinLock
#define __drv_releasesCriticalRegion
#define __drv_releasesExclusiveResource(kind)
#define __drv_releasesExclusiveResourceGlobal(kind,param)
#define __drv_releasesPriorityRegion
#define __drv_releasesResource(kind)
#define __drv_releasesResourceGlobal(kind,param)
#define __drv_reportError(why)
#define __drv_requiresIRQL(irql)
#define __drv_restoresIRQL
#define __drv_restoresIRQLGlobal(kind,param)
#define __drv_ret(annotes)
#define __drv_sameIRQL
#define __drv_savesIRQL
#define __drv_savesIRQLGlobal(kind,param)
#define __drv_strictType(typename,mode)
#define __drv_strictTypeMatch(mode)
#define __drv_unit(p)
#define __drv_useCancelIRQL
#define __drv_valueIs(arglist)
#define __drv_WDM
#define __drv_when(cond,annotes)
#define __internal_kernel_driver
#define __kernel_code
#define __kernel_driver
#define __prefast_operator_new_null \
    void* __cdecl operator new(size_t size) throw(); \
    void* __cdecl operator new[](size_t size) throw(); \
    _Analysis_mode_(_Analysis_operator_new_null_)
#define __prefast_operator_new_throws \
    void* __cdecl operator new(size_t size) throw(std::bad_alloc); \
    void* __cdecl operator new[](size_t size) throw(std::bad_alloc); \
    _Analysis_mode_(_Analysis_operator_new_throw_)
#define __user_code
#define __user_driver
#define ___drv_unit_internal_kernel_driver
#define ___drv_unit_kernel_code
#define ___drv_unit_kernel_driver
#define ___drv_unit_user_code
#define ___drv_unit_user_driver

#define __drv_typeConst  0
#define __drv_typeCond   1
#define __drv_typeBitset 2
#define __drv_typeExpr   3

#ifdef __cplusplus
extern "C" {
#endif

__ANNOTATION(SAL_neverHold(_In_ char *);)
__ANNOTATION(SAL_neverHoldGlobal(__In_impl_ char *, ...);)
__ANNOTATION(SAL_acquire(_In_ char *);)
__ANNOTATION(SAL_acquireGlobal(__In_impl_ char *, ...);)
__ANNOTATION(SAL_floatUsed(void);)
__ANNOTATION(SAL_floatSaved(void);)
__ANNOTATION(SAL_floatRestored(void);)
__ANNOTATION(SAL_clearDoInit(enum __SAL_YesNo);)
__ANNOTATION(SAL_maxIRQL(__int64);)
__ANNOTATION(SAL_IsAliased(void);)
__ANNOTATION(SAL_NeedsRelease(enum __SAL_YesNo);)
__ANNOTATION(SAL_mustHold(_In_ char *);)
__ANNOTATION(SAL_mustHoldGlobal(__In_impl_ char *, ...);)
__ANNOTATION(SAL_release(_In_ char *);)
__ANNOTATION(SAL_releaseGlobal(__In_impl_ char *, ...);)
__ANNOTATION(SAL_IoGetDmaAdapter(void);)
__ANNOTATION(SAL_kernel();)
__ANNOTATION(SAL_nokernel();)
__ANNOTATION(SAL_driver();)
__ANNOTATION(SAL_nodriver();)
__ANNOTATION(SAL_internal_kernel_driver();)
__ANNOTATION(SAL_landmark(__In_impl_ char *);)
__ANNOTATION(SAL_return(__In_impl_ __AuToQuOtE char *);)
__ANNOTATION(SAL_strictType(__In_impl_ __AuToQuOtE char *);)
__ANNOTATION(SAL_strictTypeMatch(__int64);)
__ANNOTATION(SAL_preferredFunction(__In_impl_ __AuToQuOtE char *, __In_impl_ __AuToQuOtE char *);)
__ANNOTATION(SAL_preferredFunction3(__In_impl_ __AuToQuOtE char *, __In_impl_ __AuToQuOtE char *, __In_impl_ __int64);)
__ANNOTATION(SAL_error(__In_impl_ __AuToQuOtE char *);)
__ANNOTATION(SAL_error2(__In_impl_ __AuToQuOtE char *, __In_impl_ __int64);)
__ANNOTATION(SAL_IsFormatString(__In_impl_ char *);)
__ANNOTATION(SAL_completionType(__In_impl_ __AuToQuOtE char *);)
__ANNOTATION(SAL_callbackType(__In_impl_ __AuToQuOtE char *);)
//__PRIMOP(int, _Holds_resource_(__In_impl_ __deferTypecheck char *,__In_impl_ char *);)
//__PRIMOP(int, _Holds_resource_global_(__In_impl_ char *, ...);)
//__PRIMOP(int, _Is_kernel_(void);)
//__PRIMOP(int, _Is_driver_(void);)

#ifdef __cplusplus
}
#endif

#else

/* Dummys */
#define _Dispatch_type_(type)
#define _IRQL_always_function_max_(irql)
#define _IRQL_always_function_min_(irql)
#define _IRQL_is_cancel_
#define _IRQL_raises_(irql)
#define _IRQL_requires_(irql)
#define _IRQL_requires_max_(irql)
#define _IRQL_requires_min_(irql)
#define _IRQL_requires_same_
#define _IRQL_restores_
#define _IRQL_restores_global_(kind,param)
#define _IRQL_saves_
#define _IRQL_saves_global_(kind,param)
#define _IRQL_uses_cancel_
#define _Kernel_clear_do_init_(yesNo)
#define _Kernel_float_restored_
#define _Kernel_float_saved_
#define _Kernel_float_used_
#define _Kernel_IoGetDmaAdapter_
#define _Kernel_releases_resource_(kind)
#define _Kernel_requires_resource_held_(kind)
#define _Kernel_requires_resource_not_held_(kind)
#define _Kernel_acquires_resource_(kind)
#define _Landmark_(name)
#define __drv_acquiresCancelSpinLock
#define __drv_acquiresCriticalRegion
#define __drv_acquiresExclusiveResource(kind)
#define __drv_acquiresExclusiveResourceGlobal(kind,param)
#define __drv_acquiresPriorityRegion
#define __drv_acquiresResource(kind)
#define __drv_acquiresResourceGlobal(kind,param)
#define __drv_aliasesMem
#define __drv_allocatesMem(kind)
#define __drv_arg(expr,annotes)
#define __drv_at(expr,annotes)
#define __drv_callbackType(kind)
#define __drv_clearDoInit
#define __drv_completionType(kindlist)
#define __drv_constant
#define __drv_defined(x)
#define __drv_deref(annotes)
#define __drv_dispatchType_other
#define __drv_dispatchType(x)
#define __drv_floatRestored
#define __drv_floatSaved
#define __drv_floatUsed
#define __drv_formatString(kind)
#define __drv_freesMem(kind)
#define __drv_fun(annotes)
#define __drv_functionClass(x)
#define __drv_holdsCancelSpinLock()
#define __drv_holdsCriticalRegion()
#define __drv_holdsPriorityRegion()
#define __drv_in_deref(annotes)
#define __drv_in(annotes)
#define __drv_innerAcquiresGlobal(kind,param)
#define __drv_innerMustHoldGlobal(kind,param)
#define __drv_innerNeverHoldGlobal(kind,param)
#define __drv_innerReleasesGlobal(kind,param)
#define __drv_interlocked
#define __drv_inTry
#define __drv_IoGetDmaAdapter
#define __drv_isCancelIRQL
#define __drv_isObjectPointer
#define __drv_KMDF
#define __drv_maxFunctionIRQL(irql)
#define __drv_maxIRQL(irql)
#define __drv_minFunctionIRQL(irql)
#define __drv_minIRQL(irql)
#define __drv_Mode_impl(x)
#define __drv_mustHold(kind)
#define __drv_mustHoldCancelSpinLock
#define __drv_mustHoldCriticalRegion
#define __drv_mustHoldGlobal(kind,param)
#define __drv_mustHoldPriorityRegion
#define __drv_NDIS
#define __drv_neverHold(kind)
#define __drv_neverHoldCancelSpinLock
#define __drv_neverHoldCriticalRegion
#define __drv_neverHoldGlobal(kind,param)
#define __drv_neverHoldPriorityRegion
#define __drv_nonConstant
#define __drv_notInTry
#define __drv_notPointer
#define __drv_out_deref(annotes)
#define __drv_out(annotes)
#define __drv_preferredFunction(func,why)
#define __drv_raisesIRQL(irql)
#define __drv_releasesCancelSpinLock
#define __drv_releasesCriticalRegion
#define __drv_releasesExclusiveResource(kind)
#define __drv_releasesExclusiveResourceGlobal(kind,param)
#define __drv_releasesPriorityRegion
#define __drv_releasesResource(kind)
#define __drv_releasesResourceGlobal(kind,param)
#define __drv_reportError(why)
#define __drv_requiresIRQL(irql)
#define __drv_restoresIRQL
#define __drv_restoresIRQLGlobal(kind,param)
#define __drv_ret(annotes)
#define __drv_sameIRQL
#define __drv_savesIRQL
#define __drv_savesIRQLGlobal(kind,param)
#define __drv_setsIRQL(irql)
#define __drv_strictType(typename,mode)
#define __drv_strictTypeMatch(mode)
#define __drv_unit(p)
#define __drv_useCancelIRQL
#define __drv_valueIs(arglist)
#define __drv_WDM
#define __drv_when(cond,annotes)
#define __internal_kernel_driver
#define __kernel_code
#define __kernel_driver
#define __prefast_operator_new_null
#define __prefast_operator_new_throws
#define __user_code
#define __user_driver
#define ___drv_unit_internal_kernel_driver
#define ___drv_unit_kernel_code
#define ___drv_unit_kernel_driver
#define ___drv_unit_user_code
#define ___drv_unit_user_driver

#endif

