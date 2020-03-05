#ifndef _FXINTERRUPT_H_
#define _FXINTERRUPT_H_

#include "common/fxnonpagedobject.h"

class FxPkgPnp;
class FxInterrupt;
//
// We need two parameters for KeSynchronizeExecution when enabling 
// and disabling interrupts, so we use this structure on the stack since its
// a synchronous call.
//
struct FxInterruptEnableParameters {
    FxInterrupt*        Interrupt;
    NTSTATUS            ReturnVal;
};

typedef FxInterruptEnableParameters FxInterruptDisableParameters;


class FxInterrupt : public FxNonPagedObject {

    friend FxPkgPnp;

private:

    //
    // User supplied configuration
    //
    WDF_TRI_STATE                   m_ShareVector;

    //
    // Kernel Interupt object
    //
    struct _KINTERRUPT*             m_Interrupt;

    //
    // Kernel spinlock for Interrupt
    //
    MdLock*                         m_SpinLock;        

    KIRQL                           m_OldIrql;
    volatile KIRQL                  m_SynchronizeIrql;

    //
    // Built in SpinLock/PassiveLock
    //
    MxLock                          m_BuiltInSpinLock;


    //
    // Interrupt policy
    //
    BOOLEAN                         m_SetPolicy;
    WDF_INTERRUPT_POLICY            m_Policy;
    WDF_INTERRUPT_PRIORITY          m_Priority;
    GROUP_AFFINITY                  m_Processors;

public:
    FxInterrupt(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    ~FxInterrupt(
        VOID
        );

    VOID
    FilterResourceRequirements(
        __inout PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor
        );

    static
    BOOLEAN
    _IsMessageInterrupt(
        __in USHORT ResourceFlags
        )
    {
        if (ResourceFlags & CM_RESOURCE_INTERRUPT_MESSAGE)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

protected:

    LIST_ENTRY  m_PnpList;
        
};

BOOLEAN 
_SynchronizeExecution(
    __in MdInterrupt  Interrupt,
    __in MdInterruptSynchronizeRoutine  SynchronizeRoutine,
    __in PVOID  SynchronizeContext
    );

#endif // _FXINTERRUPT_H_