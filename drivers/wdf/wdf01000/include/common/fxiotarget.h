/*
    Encapsulation of the target to which FxRequest are sent to.  For example,
    an FxTarget could represent the next device object in the pnp stack.
    Derivations from this class could include bus specific formatters or device
    objects outside of the pnp stack of the device.
*/

#ifndef _FXIOTARGET_H_
#define _FXIOTARGET_H_

#include "fxnonpagedobject.h"
#include "primitives/mxdeviceobject.h"
#include "fxwaitlock.h"
#include "fxirpqueue.h"
#include "fxtransactionedlist.h"
#include "fxrequestbase.h"



class FxIoTarget : public FxNonPagedObject {

    friend FxRequestBase;

public:

    FxIoTarget(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize
        );

    FxIoTarget(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in WDFTYPE WdfType
        );

    _Must_inspect_result_
    NTSTATUS
    Init(
        __in CfxDeviceBase* Device
        );

    __inline
    MdDeviceObject
    GetTargetPDO(
        VOID
        )
    {
        return m_TargetPdo;
    }

    VOID
    UpdateTargetIoType(
        VOID
        );

    WDFIOTARGET
    GetHandle(
        VOID
        )
    {
        return (WDFIOTARGET) GetObjectHandle();
    }

    virtual
    _Must_inspect_result_
    MdDeviceObject
    GetTargetDeviceObject(
        _In_ CfxDeviceBase* Device
        );

    //
    BOOLEAN m_InStack;

    //
    // Transaction entry for FxDevice to queue this target on
    //
    FxTransactionedEntry m_TransactionedEntry;

    //
    // TRUE when FxDevice::AddIoTarget has been called
    //
    BOOLEAN m_AddedToDeviceList;

protected:

    //
    // The PDO for m_TargetDevice.  For this class, it would be the same PDO
    // as the owning WDFDEVICE.  In a derived class (like FxIoTargetRemote),
    // this would not be the PDO of the owning WDFDEVICE, rather the PDO for
    // the other stack.
    //
    MdDeviceObject m_TargetPdo;

    //
    // This is used to track the I/O's sent to the lower driver
    // and is used to make sure all I/Os are completed before disposing the
    // Iotarget.
    //
    LONG  m_IoCount;

    //
    // Cached value of m_TargetDevice->StackSize.  The value is cached so that
    // we can still format to the target during query remove transitions.
    //
    CCHAR m_TargetStackSize;

    //
    // Cached value of m_TargetDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO)
    // which uses WDF_DEVICE_IO_TYPE to indicate state.
    //
    UCHAR m_TargetIoType;

    //
    // TRUE if we are in the processing of stopping/purging and there are
    // requests that have been sent and must be waited upon for completion.
    //
    BOOLEAN m_WaitingForSentIo;

    BOOLEAN m_Removing;

    //
    // Back link to the object that represents our devobj
    //
    FxDriver* m_Driver;

    //
    // The PDEVICE_OBJECT that is owned by m_Device
    //
    MdDeviceObject m_InStackDevice;

    //
    // The device object which is our "target"
    //
    MdDeviceObject m_TargetDevice;

    //
    // List of requests that have been sent to the target
    //
    LIST_ENTRY m_SentIoListHead;

    //
    // List of requests which were sent ignoring the state of the target
    //
    LIST_ENTRY m_IgnoredIoListHead;

    //
    // Current state
    //
    WDF_IO_TARGET_STATE m_State;

    //
    // File object that is attached to all I/O sent to m_TargetDevice
    //
    MdFileObject m_TargetFileObject;

    //
    // Event used to wait by Dispose to make sure all I/O's are completed.
    // This is required to make sure that all the I/O are completed before
    // disposing the target. This acts like remlock.
    //
    FxCREvent *m_DisposeEvent;

    FxIrpQueue m_PendedQueue;

    //
    // Event used to wait for sent I/O to complete
    //
    FxCREvent m_SentIoEvent;


    UCHAR
    GetTargetIoType(
        VOID
        )
    {
        ULONG flags;
        MxDeviceObject deviceObject(m_TargetDevice);

        flags = deviceObject.GetFlags();

        if (flags & DO_BUFFERED_IO)
        {
            return WdfDeviceIoBuffered;
        }
        else if (flags & DO_DIRECT_IO)
        {
            return WdfDeviceIoDirect;
        }
        else
        {
            return WdfDeviceIoNeither;
        }
    }

    static
    VOID
    _RequestCancelled(
        __in FxIrpQueue* Queue,
        __in MdIrp Irp,
        __in PMdIoCsqIrpContext pCsqContext,
        __in KIRQL CallerIrql
        );

    _Must_inspect_result_
    NTSTATUS
    InitModeSpecific(
        __in CfxDeviceBase* Device
        )
    {
        UNREFERENCED_PARAMETER(Device);

        DO_NOTHING();

        return STATUS_SUCCESS;
    }

    //
    // Hide destructor since we are reference counted object
    //
    ~FxIoTarget();

    VOID
    FailPendedRequest(
        __in FxRequestBase* Request,
        __in NTSTATUS Status
        );

    //
    // Generic I/O completion routine and its static caller.
    //
    VOID
    RequestCompletionRoutine(
        __in FxRequestBase* Request
        );

    BOOLEAN
    RemoveCompletedRequestLocked(
        __in FxRequestBase* Request
        );

    __inline
    VOID
    CompleteRequest(
        __in FxRequestBase* Request
        )
    {
        //
        // This will remove the reference taken by this object on the request
        //
        Request->CompleteSubmitted();
    }

    __inline
    VOID
    DecrementIoCount(
        VOID
        )
    {
        LONG ret;

        ret = InterlockedDecrement(&m_IoCount);
        ASSERT(ret >= 0);

        if (ret == 0)
        {
            PrintDisposeMessage();
            ASSERT(m_DisposeEvent != NULL);
            m_DisposeEvent->Set();
        }
    }

    VOID
    PrintDisposeMessage(
        VOID
        );

    virtual
    VOID
    ClearTargetPointers(
        VOID
        )
    {
        m_TargetDevice = NULL;
        m_TargetPdo = NULL;
        m_TargetFileObject = NULL;

        m_TargetStackSize = 0;
        m_TargetIoType = WdfDeviceIoUndefined;
    }

private:
    
    VOID
    Construct(
        VOID
        );

    VOID
    ClearCompletedRequestVerifierFlags(
        __in FxRequestBase* Request
        )
    {
        if (GetDriverGlobals()->FxVerifierOn &&
            GetDriverGlobals()->FxVerifierIO)
        {
            KIRQL irql;

            Request->Lock(&irql);
            //
            // IF we are completing a request that was pended in the target,
            // this flag was not set.
            //
            // ASSERT(Request->GetVerifierFlagsLocked() & FXREQUEST_FLAG_SENT_TO_TARGET);
            Request->ClearVerifierFlagsLocked(FXREQUEST_FLAG_SENT_TO_TARGET);
            Request->Unlock(irql);
        }
    }

};

#endif //_FXIOTARGET_H_
