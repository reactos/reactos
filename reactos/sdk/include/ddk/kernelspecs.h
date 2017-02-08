/*
 * kernelspecs.h
 *
 * SAL 2 annotations for kernel mode drivers
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributor:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#pragma once

#define KERNELSPECS_H

#include "driverspecs.h"

#ifdef _PREFAST_

/* Make sure we have IRQL level definitions early */
#define DISPATCH_LEVEL 2
#define APC_LEVEL 1
#define PASSIVE_LEVEL 0
#if defined(_X86_)
#define HIGH_LEVEL 31
#elif defined(_AMD64_)
#define HIGH_LEVEL 15
#elif defined(_ARM_)
#define HIGH_LEVEL 15
#elif defined(_IA64_)
#define HIGH_LEVEL 15
#endif

#undef _IRQL_always_function_max_
#undef _IRQL_always_function_min_
#undef _IRQL_raises_
#undef _IRQL_requires_
#undef _IRQL_requires_max_
#undef _IRQL_requires_min_
#undef _IRQL_requires_same_
#undef _IRQL_restores_
#undef _IRQL_restores_global_
#undef _IRQL_saves_
#undef _IRQL_saves_global_
#undef _IRQL_uses_cancel_
#undef _IRQL_is_cancel_
#undef __drv_setsIRQL
#undef __drv_raisesIRQL
#undef __drv_requiresIRQL
#undef __drv_maxIRQL
#undef __drv_minIRQL
#undef __drv_savesIRQL
#undef __drv_savesIRQLGlobal
#undef __drv_restoresIRQL
#undef __drv_restoresIRQLGlobal
#undef __drv_minFunctionIRQL
#undef __drv_maxFunctionIRQL
#undef __drv_sameIRQL
#undef __drv_useCancelIRQL
#undef __drv_isCancelIRQL

#define _IRQL_always_function_max_(irql)    _Pre_ _SA_annotes1(SAL_maxFunctionIrql,irql)
#define _IRQL_always_function_min_(irql)    _Pre_ _SA_annotes1(SAL_minFunctionIrql,irql)
#define _IRQL_raises_(irql)                 _Post_ _SA_annotes1(SAL_raiseIRQL,irql)
#define _IRQL_requires_(irql)               _Pre_ _SA_annotes1(SAL_IRQL,irql)
#define _IRQL_requires_max_(irql)           _Pre_ _SA_annotes1(SAL_maxIRQL,irql)
#define _IRQL_requires_min_(irql)           _Pre_ _SA_annotes1(SAL_minIRQL,irql)
#define _IRQL_requires_same_                _Post_ _SA_annotes0(SAL_sameIRQL)
#define _IRQL_restores_                     _Post_ _SA_annotes0(SAL_restoreIRQL)
#define _IRQL_restores_global_(kind,param)  _Post_ _SA_annotes2(SAL_restoreIRQLGlobal, #kind, param\t)
#define _IRQL_saves_                        _Post_ _SA_annotes0(SAL_saveIRQL)
#define _IRQL_saves_global_(kind,param)     _Post_ _SA_annotes2(SAL_saveIRQLGlobal,#kind, param\t)
#define _IRQL_uses_cancel_                  _Post_ _SA_annotes0(SAL_UseCancelIrql)
#define _IRQL_is_cancel_                    _IRQL_uses_cancel_ _Releases_nonreentrant_lock_(_Global_cancel_spin_lock_) \
                                                _At_(return, _IRQL_always_function_min_(DISPATCH_LEVEL) _IRQL_requires_(DISPATCH_LEVEL))
#define __drv_setsIRQL(irql)                _Post_ _SA_annotes1(SAL_IRQL,irql)
#define __drv_raisesIRQL(irql)              _IRQL_raises_(irql)
#define __drv_requiresIRQL(irql)            _IRQL_requires_(irql)
#define __drv_maxIRQL(irql)                 _IRQL_requires_max_(irql)
#define __drv_minIRQL(irql)                 _IRQL_requires_min_(irql)
#define __drv_savesIRQL                     _IRQL_saves_
#define __drv_savesIRQLGlobal(kind,param)   _IRQL_saves_global_(kind,param)
#define __drv_restoresIRQL                  _IRQL_restores_
#define __drv_restoresIRQLGlobal(kind,param) _IRQL_restores_global_(kind,param)
#define __drv_minFunctionIRQL(irql)         _IRQL_always_function_min_(irql)
#define __drv_maxFunctionIRQL(irql)         _IRQL_always_function_max_(irql)
#define __drv_sameIRQL                      _IRQL_requires_same_
#define __drv_useCancelIRQL                 _IRQL_uses_cancel_
#define __drv_isCancelIRQL                  _IRQL_is_cancel_

#ifdef __cplusplus
extern "C" {
#endif

__ANNOTATION(SAL_IRQL(__int64);)
__ANNOTATION(SAL_raiseIRQL(__int64);)
__ANNOTATION(SAL_maxIRQL(__int64);)
__ANNOTATION(SAL_minIRQL(__int64);)
__ANNOTATION(SAL_saveIRQL(void);)
__ANNOTATION(SAL_saveIRQLGlobal(_In_ char *, ...);)
__ANNOTATION(SAL_restoreIRQL(void);)
__ANNOTATION(SAL_restoreIRQLGlobal(_In_ char *, ...);)
__ANNOTATION(SAL_minFunctionIrql(__int64);)
__ANNOTATION(SAL_maxFunctionIrql(__int64);)
__ANNOTATION(SAL_sameIRQL(void);)
__ANNOTATION(SAL_UseCancelIrql(void);)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _PREFAST_ */
