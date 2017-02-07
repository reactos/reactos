/*
 * PROJECT:         ReactOS DDK
 * COPYRIGHT:       This file is in the Public Domain.
 * FILE:            driverspecs.h
 * ABSTRACT:        This header stubs out Driver Verifier annotations to
 *                  allow drivers using them to compile with our header set.
 */

#pragma once
#define DRIVERSPECS_H

#ifndef SPECSTRINGS_H
#include <specstrings.h>
#endif

#include <concurrencysal.h>

//
// Stubs
//
#define ___drv_unit_internal_kernel_driver
#define ___drv_unit_kernel_code
#define ___drv_unit_kernel_driver
#define ___drv_unit_user_code
#define ___drv_unit_user_driver
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
#define __drv_functionClass
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
#define __drv_IoGetDmaAdapter
#define __drv_isCancelIRQL
#define __drv_isObjectPointer
#define __drv_KMDF
#define __drv_maxFunctionIRQL
#define __drv_maxIRQL
#define __drv_minFunctionIRQL
#define __drv_minIRQL
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
#define __drv_raisesIRQL
#define __drv_releasesCancelSpinLock
#define __drv_releasesCriticalRegion
#define __drv_releasesExclusiveResource(kind)
#define __drv_releasesExclusiveResourceGlobal(kind,param)
#define __drv_releasesPriorityRegion
#define __drv_releasesResource(kind)
#define __drv_releasesResourceGlobal(kind,param)
#define __drv_reportError(why)
#define __drv_requiresIRQL
#define __drv_restoresIRQL
#define __drv_restoresIRQLGlobal
#define __drv_ret(annotes)
#define __drv_sameIRQL
#define __drv_savesIRQL
#define __drv_savesIRQLGlobal
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
#define _Dispatch_type_
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

