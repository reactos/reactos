#ifndef _FXIRPKM_H_
#define _FXIRPKM_H_

#include <ntddk.h>
#include <wdf.h>

typedef PIRP MdIrp;

class FxIrp {

private:
    MdIrp m_irp;

public:

    FxIrp() {}

    FxIrp(MdIrp irp) : m_irp(irp)
    {
    }

    UCHAR
    GetMajorFunction()
    {
        PIO_STACK_LOCATION  irpStack;
        irpStack = IoGetCurrentIrpStackLocation(m_irp);
        return irpStack->MajorFunction;
    }

    UCHAR
    GetMinorFunction()
    {
        PIO_STACK_LOCATION  irpStack;
        irpStack = IoGetCurrentIrpStackLocation(m_irp);
        return irpStack->MinorFunction;
    }

    VOID
    SetStatus(NTSTATUS status)
    {
        m_irp->IoStatus.Status = status;
    }
            
            
    VOID
    CompleteRequest(CCHAR priority)
    {
        IoCompleteRequest(m_irp, priority);
    }

    VOID
    SetIrp(MdIrp irp)
    {
        m_irp = irp;
    }

    MdIrp
    GetIrp()
    {
        return m_irp;
    }

    NTSTATUS
    GetStatus()
    {
        return m_irp->IoStatus.Status;
    }

};

#endif //_FXIRPKM_H_