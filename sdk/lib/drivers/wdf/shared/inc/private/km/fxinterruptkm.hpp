/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxInterruptKm.hpp

Abstract:

    This module implements a frameworks managed interrupt object

Author:




Environment:

    Kernel mode only

Revision History:


--*/

#ifndef _FXINTERRUPTKM_H_
#define _FXINTERRUPTKM_H_

#include "FxInterrupt.hpp"

__inline
struct _KINTERRUPT*
FxInterrupt::GetInterruptPtr(
    VOID
    )
{
    struct _KINTERRUPT* interrupt = m_Interrupt;

    if (interrupt == NULL) {
        interrupt = m_InterruptCaptured;
    }

    return interrupt;
}

__inline
VOID
FxInterrupt::ResetInternal(
    VOID
    )
{
    //
    // Does nothing for KMDF
    //
}

__inline
VOID
FxInterrupt::RevokeResourcesInternal(
    VOID
    )
{
    //
    // Does nothing for KMDF
    //
}

__inline
VOID
FxInterrupt::AssignResourcesInternal(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescRaw,
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescTrans,
    __in PWDF_INTERRUPT_INFO InterruptInfo
    )
{
    UNREFERENCED_PARAMETER(CmDescRaw);
    UNREFERENCED_PARAMETER(CmDescTrans);
    UNREFERENCED_PARAMETER(InterruptInfo);

    //
    // Does nothing for KMDF
    //
}

__inline
VOID
FxInterrupt::SetPolicyInternal(
    __in WDF_INTERRUPT_POLICY   Policy,
    __in WDF_INTERRUPT_PRIORITY Priority,
    __in PGROUP_AFFINITY        TargetProcessorSet
    )
{
    UNREFERENCED_PARAMETER(Policy);
    UNREFERENCED_PARAMETER(Priority);
    UNREFERENCED_PARAMETER(TargetProcessorSet);

    //
    // Does nothing for KMDF
    //
}

__inline
BOOLEAN
_SynchronizeExecution(
    __in MdInterrupt  Interrupt,
    __in MdInterruptSynchronizeRoutine  SynchronizeRoutine,
    __in PVOID  SynchronizeContext
    )
{
    return KeSynchronizeExecution(Interrupt,
                                  SynchronizeRoutine,
                                  SynchronizeContext);
}

#endif // _FXINTERRUPTKM_H_
