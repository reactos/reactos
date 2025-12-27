/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     x64 C++ V-tables for concurrency.c
 * COPYRIGHT:   Copyright 2014-2024 Wine team
 *              Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <asm.inc>

// See msvcrt/concurrency.c

.const

MACRO(START_VTABLE, shortname, cxxname)
EXTERN shortname&_rtti:PROC
    .quad shortname&_rtti
PUBLIC &shortname&_vtable
&shortname&_vtable:
PUBLIC &cxxname
&cxxname:
ENDM

MACRO(VTABLE_ADD_FUNC_MACRO, name)
    EXTERN &name:PROC
    .quad &name
ENDM
#define VTABLE_ADD_FUNC(x) VTABLE_ADD_FUNC_MACRO x

START_VTABLE cexception, __dummyname_cexception
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE improper_lock, __dummyname_improper_lock
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE improper_scheduler_attach, __dummyname_improper_scheduler_attach
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE improper_scheduler_detach, __dummyname_improper_scheduler_detach
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE invalid_multiple_scheduling, __dummyname_invalid_multiple_scheduling
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE invalid_scheduler_policy_key, __dummyname_invalid_scheduler_policy_key
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE invalid_scheduler_policy_thread_specification, __dummyname_invalid_scheduler_policy_thread_specification
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE invalid_scheduler_policy_value, __dummyname_invalid_scheduler_policy_value
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE missing_wait, __dummyname_missing_wait
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)

START_VTABLE scheduler_resource_allocation_error, __dummyname_scheduler_resource_allocation_error
    VTABLE_ADD_FUNC(cexception_vector_dtor)
    VTABLE_ADD_FUNC(cexception_what)


START_VTABLE ExternalContextBase, __dummyname_ExternalContextBase
    VTABLE_ADD_FUNC(ExternalContextBase_GetId)
    VTABLE_ADD_FUNC(ExternalContextBase_GetVirtualProcessorId)
    VTABLE_ADD_FUNC(ExternalContextBase_GetScheduleGroupId)
    VTABLE_ADD_FUNC(ExternalContextBase_Unblock)
    VTABLE_ADD_FUNC(ExternalContextBase_IsSynchronouslyBlocked)
    VTABLE_ADD_FUNC(ExternalContextBase_vector_dtor)
    VTABLE_ADD_FUNC(ExternalContextBase_Block)
    VTABLE_ADD_FUNC(ExternalContextBase_Yield)
    VTABLE_ADD_FUNC(ExternalContextBase_SpinYield)
    VTABLE_ADD_FUNC(ExternalContextBase_Oversubscribe)
    VTABLE_ADD_FUNC(ExternalContextBase_Alloc)
    VTABLE_ADD_FUNC(ExternalContextBase_Free)
    VTABLE_ADD_FUNC(ExternalContextBase_EnterCriticalRegionHelper)
    VTABLE_ADD_FUNC(ExternalContextBase_EnterHyperCriticalRegionHelper)
    VTABLE_ADD_FUNC(ExternalContextBase_ExitCriticalRegionHelper)
    VTABLE_ADD_FUNC(ExternalContextBase_ExitHyperCriticalRegionHelper)
    VTABLE_ADD_FUNC(ExternalContextBase_GetCriticalRegionType)
    VTABLE_ADD_FUNC(ExternalContextBase_GetContextKind)

START_VTABLE ThreadScheduler, __dummyname_ThreadScheduler
    VTABLE_ADD_FUNC(ThreadScheduler_vector_dtor)
    VTABLE_ADD_FUNC(ThreadScheduler_Id)
    VTABLE_ADD_FUNC(ThreadScheduler_GetNumberOfVirtualProcessors)
    VTABLE_ADD_FUNC(ThreadScheduler_GetPolicy)
    VTABLE_ADD_FUNC(ThreadScheduler_Reference)
    VTABLE_ADD_FUNC(ThreadScheduler_Release)
    VTABLE_ADD_FUNC(ThreadScheduler_RegisterShutdownEvent)
    VTABLE_ADD_FUNC(ThreadScheduler_Attach)
#if _MSVCR_VER > 100
    VTABLE_ADD_FUNC(ThreadScheduler_CreateScheduleGroup_loc)
#endif
    VTABLE_ADD_FUNC(ThreadScheduler_CreateScheduleGroup)
#if _MSVCR_VER > 100
    VTABLE_ADD_FUNC(ThreadScheduler_ScheduleTask_loc)
#endif
    VTABLE_ADD_FUNC(ThreadScheduler_ScheduleTask)
#if _MSVCR_VER > 100
    VTABLE_ADD_FUNC(ThreadScheduler_IsAvailableLocation)
#endif

START_VTABLE _Timer, __dummyname__Timer
    VTABLE_ADD_FUNC(_Timer_vector_dtor)

END
