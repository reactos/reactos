/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDevice.hpp

Abstract:

    This is the definition of the FxDevice object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXDEVICE_H_
#define _FXDEVICE_H_

#include "FxCxDeviceInit.hpp"
#include "FxDeviceInit.hpp"
#include "FxTelemetry.hpp"

struct FxWdmDeviceExtension {
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    WUDF_IO_REMOVE_LOCK IoRemoveLock;
#else
    IO_REMOVE_LOCK  IoRemoveLock;
#endif
    ULONG           RemoveLockOptionFlags;
};

//
// The following enum is used in serializing packet based DMA transactions.
// According to the DDK docs:
// Only one DMA request can be queued for a device object at any
// one time. Therefore, the driver should not call AllocateAdapterChannel
// again for another DMA operation on the same device object until the
// AdapterControl routine has completed execution. In addition,
// a driver must not call AllocateAdapterChannel from within its
// AdapterControl routine.
//
// This is because when AllocateAdapterChannel blocks waiting for
// map registers, it obtains its wait context block from the device object.
// If AllocateAdapterChannel is then called through a different adapter
// object attached to the same device the wait block will be reused and the
// map register wait list will be corrupted.
//
// For this reason, we need to make sure that for a device used in creating
// DMA enablers, there can be only one packet base DMA transaction
// queued at any one time.
//
// In WDM, one can workaround this limitation by creating dummy deviceobject.
// We can also workaround this limitation by creating a control-device on the
// side for additional enabler objects. Since packet based multi-channel
// devices are rarity these days, IMO, we will defer this feature until there
// is a big demand for it.
//
enum FxDmaPacketTransactionStatus {
    FxDmaPacketTransactionCompleted =0,
    FxDmaPacketTransactionPending,
};

//
// The following enum is used in determining whether the RemLock for a device
// object needs to be held while processing an IRP. For processing certain
// IRPs, it might not be necessary to hold the RemLock, but it might be
// necessary to just test whether the RemLock can be acquired and released.
//
enum FxDeviceRemLockAction {
    FxDeviceRemLockNotRequired = 0,
    FxDeviceRemLockRequired,
    FxDeviceRemLockTestValid,
    FxDeviceRemLockOptIn
};

enum FxPropertyType {
    FxDeviceProperty = 0,
    FxInterfaceProperty,
};

//
// This mask is used to validate the WdfDeviceWdmDispatchIrp's Flags.
//
#define FX_DISPATCH_IRP_TO_IO_QUEUE_FLAGS_MASK \
    (WDF_DISPATCH_IRP_TO_IO_QUEUE_INVOKE_INCALLERCTX_CALLBACK |\
     WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP)

//
// The following inline functions are used for extracting the normalized file
// object class value and checking the file object class's flags.
//
WDF_FILEOBJECT_CLASS
__inline
FxFileObjectClassNormalize(
    __in WDF_FILEOBJECT_CLASS FileObjectClass
    )
{
    return (WDF_FILEOBJECT_CLASS)(FileObjectClass & ~WdfFileObjectCanBeOptional);
}

BOOLEAN
__inline
FxIsFileObjectOptional(
    __in WDF_FILEOBJECT_CLASS FileObjectClass
    )
{
    return (FileObjectClass & WdfFileObjectCanBeOptional) ? TRUE : FALSE;
}

//
// Base class for all devices.
//
class FxDeviceBase : public FxNonPagedObject, public IFxHasCallbacks {

protected:
    FxDeviceBase(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxDriver* Driver,
        __in WDFTYPE Type,
        __in USHORT Size
        );

    ~FxDeviceBase(
        VOID
        );

    VOID
    Init(
        __in MdDeviceObject DeviceObject,
        __in MdDeviceObject AttachedDevice,
        __in MdDeviceObject PhysicalDevice
        );

public:
    NTSTATUS
    ConfigureConstraints(
        __in_opt PWDF_OBJECT_ATTRIBUTES ObjectAttributes
        );

    // begin IFxHasCallbacks overrides
    VOID
    GetConstraints(
        __out_opt WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out_opt WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) ;

    FxCallbackLock*
    GetCallbackLockPtr(
        __out_opt FxObject** LockObject
        );
    // end IFxHasCallbacks overrides

    __inline
    FxDriver*
    GetDriver(
        VOID
        )
    {
        return m_Driver;
    }


    MdDeviceObject
    __inline
    GetDeviceObject(
        VOID
        )
    {
        return m_DeviceObject.GetObject();
    }

    __inline
    MxDeviceObject*
    GetMxDeviceObject(
        VOID
        )
    {
        return &m_DeviceObject;
    }

    ULONG
    __inline
    GetDeviceObjectFlags(
        VOID
        )
    {
        return m_DeviceObject.GetFlags();
    }

    VOID
    __inline
    SetDeviceObjectFlags(
        _In_ ULONG Flags
        )
    {
        m_DeviceObject.SetFlags(Flags);
    }

    MdDeviceObject
    __inline
    GetAttachedDevice(
        VOID
        )
    {
        return m_AttachedDevice.GetObject();
    }

    ULONG
    __inline
    GetAttachedDeviceObjectFlags(
        VOID
        )
    {
        return m_AttachedDevice.GetFlags();
    }

    MdDeviceObject
    __inline
    GetPhysicalDevice(
        VOID
        )
    {
        return m_PhysicalDevice.GetObject();
    }

    WDFDEVICE
    __inline
    GetHandle(
        VOID
        )
    {
        return (WDFDEVICE) GetObjectHandle();
    }

    virtual
    _Must_inspect_result_
    NTSTATUS
    AddIoTarget(
        __inout FxIoTarget* IoTarget
        )
    {
        UNREFERENCED_PARAMETER(IoTarget);

        //
        // Intentionally does nothing
        //
        return STATUS_SUCCESS;
    }

    virtual
    VOID
    RemoveIoTarget(
        __inout FxIoTarget* IoTarget
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(IoTarget);
    }

    virtual
    _Must_inspect_result_
    NTSTATUS
    AllocateEnumInfo(
        VOID
        )
    {
        return STATUS_SUCCESS;
    }

    virtual
    VOID
    AddChildList(
        __inout FxChildList* List
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(List);
    }

    virtual
    VOID
    RemoveChildList(
        __inout FxChildList* List
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(List);
    }

    virtual
    _Must_inspect_result_
    NTSTATUS
    AllocateDmaEnablerList(
        VOID
        )
    {
        return STATUS_SUCCESS;
    }

    virtual
    VOID
    AddDmaEnabler(
        __inout FxDmaEnabler* Enabler
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(Enabler);
    }

    virtual
    VOID
    RemoveDmaEnabler(
        __inout FxDmaEnabler* Enabler
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(Enabler);
    }

    virtual
    VOID
    SetDeviceTelemetryInfoFlags(
        _In_ FxDeviceInfoFlags Flag
        )
    {
        //
        // Intentionally does nothing
        //
        UNREFERENCED_PARAMETER(Flag);
    }

    __inline
    _Must_inspect_result_
    NTSTATUS
    AcquireDmaPacketTransaction(
        VOID
        )
    {
        //
        // Set the status to Pending only if the previous transaction is Completed.
        //
        if (InterlockedCompareExchange(
                &m_DmaPacketTransactionStatus,
                FxDmaPacketTransactionPending,
                FxDmaPacketTransactionCompleted) == FxDmaPacketTransactionCompleted) {
            return STATUS_SUCCESS;
        } else {
            return STATUS_WDF_BUSY;
        }
    }

    __inline
    VOID
    ReleaseDmaPacketTransaction(
        VOID
        )
    {
        LONG val;

        val = InterlockedExchange(&m_DmaPacketTransactionStatus,
                                  FxDmaPacketTransactionCompleted);

        ASSERT(val == FxDmaPacketTransactionPending); // To catch double release
        UNREFERENCED_PARAMETER(val);
    }

    VOID
    AddToDisposeList(
        __inout FxObject* Object
        )
    {
        m_DisposeList->Add(Object);
    }

    // begin FxObject overrides
    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        );
    // end FxObject overrides

    static
    FxDeviceBase*
    _SearchForDevice(
        __in FxObject* Object,
        __out_opt IFxHasCallbacks** Callbacks
        );

    static
    FxDeviceBase*
    _SearchForDevice(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes
        );

    _Must_inspect_result_
    NTSTATUS
    QueryForInterface(
        __in const GUID* InterfaceType,
        __out PINTERFACE Interface,
        __in USHORT Size,
        __in USHORT Version,
        __in PVOID InterfaceSpecificData,
        __in_opt MdDeviceObject TargetDevice = NULL
        );

    __inline
    MdDeviceObject
    GetAttachedDeviceReference(
        VOID
        )
    {
        return Mx::MxGetAttachedDeviceReference(m_DeviceObject.GetObject());
    }

    virtual
    FxIoTarget*
    GetDefaultIoTarget(
        VOID
        )
    {
        return NULL;
    }

    _Must_inspect_result_
    NTSTATUS
    AllocateTarget(
        _Out_ FxIoTarget** Target,
        _In_  BOOLEAN SelfTarget
        );

    //
    // Note: these fields are carefully aligned to minimize space. If you add
    // additional fields make sure to insert them correctly. Always
    // double check your assumptions by loading the amd64 image and
    // comparing the size of this type before and after. For example the
    // m_RequestLookasideList is aligned on SYSTEM_CACHE_ALIGNMENT_SIZE (64/128),
    // a simple change can increase the size by 64/128 bytes.
    //

public:
    //
    // This is used to defer items that must be cleaned up at passive
    // level, and FxDevice waits on this list to empty in DeviceRemove.
    //
    FxDisposeList* m_DisposeList;

protected:
    FxDriver* m_Driver;

    MxDeviceObject m_DeviceObject;
    MxDeviceObject m_AttachedDevice;
    MxDeviceObject m_PhysicalDevice;

    FxCallbackLock* m_CallbackLockPtr;
    FxObject* m_CallbackLockObjectPtr;

    WDF_EXECUTION_LEVEL m_ExecutionLevel;
    WDF_SYNCHRONIZATION_SCOPE m_SynchronizationScope;

    //
    // Used to serialize packet dma transactions on this device.
    //
    LONG m_DmaPacketTransactionStatus;
};

class FxDevice : public FxDeviceBase {
   friend VOID GetTriageInfo(VOID);
   friend class FxDriver;
   friend class FxIrp;
   friend class FxFileObject;
   friend class FxPkgPnp;

   //
   // Note: these fields are carefully aligned to minimize space. If you add
   // additional fileds make sure to insert them correctly. Always
   // double check your assumptions by loading the amd64 image and
   // comparing the size of this type before and after. For example the
   // m_RequestLookasideList is aligned on SYSTEM_CACHE_ALIGNMENT_SIZE (64/128),
   // a simple change can increase the size by 64/128 bytes.
   //

private:
    //
    // Maintain the current device states.
    //
    WDF_DEVICE_PNP_STATE            m_CurrentPnpState;
    WDF_DEVICE_POWER_STATE          m_CurrentPowerState;
    WDF_DEVICE_POWER_POLICY_STATE   m_CurrentPowerPolicyState;

    //
    // Store the IO type for read/write
    //
    WDF_DEVICE_IO_TYPE m_ReadWriteIoType;

    //
    // Bit-flags, see FxDeviceCallbackFlags for definitions.
    //
    BYTE m_CallbackFlags;

    // TRUE if a Filter
    BOOLEAN m_Filter;

    //
    // If TRUE, DO_POWER_PAGABLE can be set on m_DeviceObject->Flags if we are
    // not in a special usage path.
    //
    // ***Ignored for filters***
    //
    BOOLEAN m_PowerPageableCapable;

    //
    // TRUE if the parent is removed while the child is still around
    //
    BOOLEAN m_ParentWaitingOnChild;

    //
    // TRUE if the device only allows one create to succeed at any given time
    //
    BOOLEAN m_Exclusive;

    //
    // More deterministic the m_PkgPnp == NULL since m_PkgPnp can be == NULL
    // if there is an allocation failure and during deletion due to insufficient
    // resources we need to know if the device is legacy or not.
    //
    BOOLEAN m_Legacy;

    //
    // If TRUE, m_DeviceObject was deleted in FxDevice::DeleteObject and should
    // not be deleted again later in the destroy path.
    //
    BOOLEAN m_DeviceObjectDeleted;

    //
    // This boost will be used in IoCompleteRequest
    // for read, write and ioctl requests if the client driver
    // completes the request without specifying the boost.
    //
    //
    CHAR m_DefaultPriorityBoost;

    static const CHAR m_PriorityBoosts[];

public:
    //
    // Track the parent if applicable
    //
    CfxDevice *m_ParentDevice;

    //
    // Properties used during Device Creation
    //

    //
    // Store the device name that is used during device creation.
    //
    UNICODE_STRING m_DeviceName;

    UNICODE_STRING m_SymbolicLinkName;

    //
    // Store the name of the resource that is used to store the MOF data
    //
    UNICODE_STRING m_MofResourceName;

    //
    // When reporting a PDO via query device relations, there is a period of
    // time where it is an "official" PDO as recognized by the pnp subsystem.
    // In that period of time, we cannot use the soon to be PDO in any export
    // which expects a PDO as an input parameter.  Once this is set to TRUE,
    // the PDO can be used for such exports.
    //
    // No need to use a lock when comparing against this field.  Once set, it
    // will never revert back to FALSE.
    //
    // This field is always TRUE for FDOs (in relation to the PDO for its stack).
    //
    BOOLEAN m_PdoKnown;

    //
    // If TRUE, then create/cleanup/close are forwarded down the stack
    // If FALSE, then create/cleanup/close are completed at this device
    //
    BOOLEAN m_AutoForwardCleanupClose;

    //
    // If TRUE, an Io Target to the client itself is created to support
    // Self Io Targets.
    //
    BOOLEAN m_SelfIoTargetNeeded;

private:
    //
    // bit-map of device info for Telemetry
    //
    USHORT m_DeviceTelemetryInfoFlags;

public:

    WDF_FILEOBJECT_CLASS m_FileObjectClass;

    FxSpinLockTransactionedList m_IoTargetsList;

    //
    // We'll maintain the prepreocess table "per device" so that it is possible
    // to have different callbacks for each device.
    // Note that each device may be associted with multiple class extension in the future.
    //
    LIST_ENTRY          m_PreprocessInfoListHead;

    //
    // Optional, list of additional class extension settings.
    //
    LIST_ENTRY          m_CxDeviceInfoListHead;

protected:

    //
    // This is used by the FxFileObject class to manage
    // the list of FxFileObject's for this FxDevice
    //
    LIST_ENTRY m_FileObjectListHead;

    //
    // Lookaside list to allocate FxRequests from
    //
    NPAGED_LOOKASIDE_LIST m_RequestLookasideList;

    //
    // Total size of an FxRequest + driver context LookasideList element.
    //
    size_t m_RequestLookasideListElementSize;

    //
    // Object attributes to apply to each FxRequest* returned by
    // m_RequestLookasideList
    //
    WDF_OBJECT_ATTRIBUTES m_RequestAttributes;

public:

    //
    // This is the set of packages used by this device.  I am simply using
    // FxPackage pointers rather than using the actual types because I want
    // to allow fredom for FDOs, PDOs, and control objects to use
    // differnet packages.
    //
    FxPkgIo*            m_PkgIo;
    FxPkgPnp*           m_PkgPnp;
    FxPkgGeneral*       m_PkgGeneral;
    FxWmiIrpHandler*    m_PkgWmi;
    FxDefaultIrpHandler* m_PkgDefault;

    //
    // Note on approaches to having mode-agnoctic code that works for KM and UM
    // and avoids code with lots of #ifdef which becomes a maintenance nightmare.
    // To avoid #ifdef such as below, one approach would have been to have a
    // base class with common data members and virtual funtions , and have
    // derived classes for km and um,each having data members specific to their
    // mode, implementing virtual funcions in mode specific manner. This
    // approach was not taken for following reasons:
    //
    // 1. Avoid confusion between logical hierarchy and organizational hierarchy
    // of objects. E.g. fdo and pdo package is derived from pnp package (logical
    // hierarchy). However, both pdo and fdo package can also be organized into
    // fdokm/fdoum deriving from fdo, and pdokm/pdoum deriving from pdo for km
    // and um flavors, and that would be organizational hierarchy. Mixing these
    // two approaches may create more confusion. If we were to extend the
    // classes in future (for whatever reason), this may become more complex.
    //
    // 2. Even with organizational hierarchy, we need to have #ifdef at the
    // point of creation.
    //
    // Luckily, we don't have many objects that need to be have mode specific
    // data members (currently only FxDevice and interrupt to some extent).
    // Note that member functions are already implemented in mode specific
    // manner, for example, FxDevice::CreateDevice is implemented for UM and KM
    // in FxDeviceUm.cpp and FxDeviceKm.cpp. So #ifdef usage is not a whole lot
    // but we can definitely improve on it.
    //
    // With the current approach, we can do better by avoiding #ifdef as much as
    // possible. We can achieve that with better abstraction, but also having
    // host provide more interfaces so as to mimic closely  those interfaces
    // that kernel provides would also help (this way framework has to maintain
    // less info, because it can always get it from host the way kernel
    // framework would).
    //
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
public:
    //
    // On failed create during AddDevice, KMDF sends a simulated remove event
    // to pnp state machine and thereafter detaches from stack so that windows
    // I/O manager can't send a remove irp. UMDF imitates windows I/O manager
    // in that when AddDevice sent by host is failed by driver, host sends a
    // simulated remove irp to Fx so that it can cleanup.
    //
    // This causes a conflict in merged code because for UMDF,  Fx doesn't
    // detach from stack as part of remove event (since lifetime of umdf stack

    // is controlled by host including detach and deletiton), so unless we
    // prevent, Fx will end up processing remove event twice, once by Pnp sm's
    // simulated event and another by host simulated remove irp.
    //
    // The solution is to allow one remove event to be processed and that would
    // be Fx's remove event (to minimize disparity between KM and UM Fx). The
    // field below tracks the fact that create failed and allows the Fx remove
    // event to be processed and then also allows the device object to detach
    // before returning from failure so that host is not able to send simulated
    // remove to the device.
    //
    BOOLEAN m_CleanupFromFailedCreate;

    //
    // This object implements the IFxMessageDispatch that contains entry points
    // to driver, and is used by host to dispatch irp and other messages.
    //
    FxMessageDispatch* m_Dispatcher;

    //
    //Weak reference to host side device stack
    //
    IWudfDeviceStack*   m_DevStack;

    //
    // PnP devinode hw key handle
    //
    HKEY m_PdoDevKey;

    //
    // Device key registry path
    //
    PWSTR m_DeviceKeyPath;

    //
    // Kernel redirector's side object name.
    //
    PWSTR m_KernelDeviceName;

    //
    // PDO Instance ID
    //
    PWSTR m_DeviceInstanceId;

    //
    // The retrieval mode and i/o type preferences requested
    // by this device. Note that ReadWriteIoType is common to both KMDF and UMDF
    // so no new UM-specific field is required.
    //
    UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL m_RetrievalMode;
    WDF_DEVICE_IO_TYPE             m_IoctlIoType;
    ULONG                          m_DirectTransferThreshold;

    //
    // Tells whether hardware access is allowed.
    //
    WDF_DIRECT_HARDWARE_ACCESS_TYPE m_DirectHardwareAccess;

    //
    // Tells whether hardware register read/write is done using user-mode
    // mapped virtual addresses
    //
    WDF_REGISTER_ACCESS_MODE_TYPE m_RegisterAccessMode;

    //
    // File object policy set through INF directive
    //
    WDF_FILE_OBJECT_POLICY_TYPE m_FileObjectPolicy;

    //
    // Fs context use policy set through INF directive
    //
    WDF_FS_CONTEXT_USE_POLICY_TYPE m_FsContextUsePolicy;

    //
    // Thread pool for interrupt servicing
    //
    FxInterruptThreadpool* m_InteruptThreadpool;

#endif // (FX_CORE_MODE == FX_CORE_USER_MODE)

private:
    //
    // A method called by the constructor(s) to initialize the device state.
    //
    VOID
    SetInitialState(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PreprocessIrp(
        __in MdIrp Irp
        );

    _Must_inspect_result_
    NTSTATUS
    DeleteDeviceFromFailedCreateNoDelete(
        __in NTSTATUS FailedStatus,
        __in BOOLEAN UseStateMachine
        );

    VOID
    SetFilterIoType(
        VOID
        );

    static
    MdCompletionRoutineType
    _CompletionRoutineForRemlockMaintenance;

    static
    _Must_inspect_result_
    NTSTATUS
    _AcquireOptinRemoveLock(
        __in MdDeviceObject DeviceObject,
        __in MdIrp Irp
        );

    VOID
    DestructorInternal(
        VOID
        );

    NTSTATUS
    WmiPkgRegister(
        VOID
        );

    VOID
    WmiPkgDeregister(
        VOID
        );

    VOID
    WmiPkgCleanup(
        VOID
        );

public:

    FxDevice(
        __in FxDriver *ArgDriver
        );

    ~FxDevice(
        VOID
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PWDFDEVICE_INIT* DeviceInit,
        __in_opt PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
        __out FxDevice** Device
        );

    _Must_inspect_result_
    NTSTATUS
    DeleteDeviceFromFailedCreate(
        __in NTSTATUS FailedStatus,
        __in BOOLEAN UseStateMachine
        );

    __inline
    FxPackage*
    GetDispatchPackage(
        __in UCHAR MajorFunction
        )
    {
        switch (MajorFunction) {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        case IRP_MJ_CLEANUP:
        case IRP_MJ_SHUTDOWN:
            return (FxPackage*) m_PkgGeneral;

        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return (FxPackage*) m_PkgIo;

        case IRP_MJ_SYSTEM_CONTROL:
            return (FxPackage*) m_PkgWmi;

        case IRP_MJ_PNP:
        case IRP_MJ_POWER:
            if (m_PkgPnp != NULL) {
                return (FxPackage*) m_PkgPnp;
            }
            else {
                return (FxPackage*) m_PkgDefault;
            }
            break;

        default:
            return (FxPackage*) m_PkgDefault;
        }
    }

    MdRemoveLock
    GetRemoveLock(
        VOID
        );

    static
    FxDeviceRemLockAction
    __inline
    _RequiresRemLock(
        __in UCHAR MajorCode,
        __in UCHAR MinorCode
        )
    {
        switch (MajorCode) {
        //
        // We require remove locks for power irps because they can show
        // up after the device has been removed if the Power subysystem has
        // taken a reference on the device object that raced with the
        // remove irp (or if we are attached above the power policy owner
        // and the power policy owner requests a power irp during remove
        // processing.
        //
        // What it boils down to is that we do it for power because
        // that is the only valid irp which can be sent with an outstanding
        // reference w/out coordination to the device's pnp state.  We
        // assume that for all other irps, the sender has synchronized with
        // the pnp state of the device.
        //
        // We also acquire the remove lock for WMI IRPs because they can
        // come into the stack while we are processing a remove.  For
        // instance, a WMI irp can come into the stack to the attached
        // device before it has a change to process the remove device and
        // unregister with WMI.
        //
        // PNP irps can come in at any time as well.  For instance, query
        // device relations for removal or ejection relations can be sent
        // at any time (and there are pnp stress tests which send them
        // during remove).
        //
        case IRP_MJ_PNP:
            //
            // We special case remove device and only acquire the remove lock
            // in the minor code handler itself.  If handled remove device in
            // the normal way and there was a preprocess routine for it, then
            // we could deadlock if the irp was dispatched back to KMDF in the
            // preprocess routine with an extra outstandling remlock acquire
            // (which won't be released until the preprocess routine returns,
            // which will be too late).
            //
            if (MinorCode == IRP_MN_REMOVE_DEVICE) {
                return FxDeviceRemLockTestValid;
            }
        case IRP_MJ_POWER:
        case IRP_MJ_SYSTEM_CONTROL:
            return FxDeviceRemLockRequired;

        default:
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
            return FxDeviceRemLockOptIn;
#else
            //
            // There is no forseeable scenario where a UMDF driver would need to
            // need to support remove lock for IO IRPs. While this ifdef can be safely
            // removed and UMDF can also return FxDeviceRemLockOptIn, that is
            // being avoided here so that the caller does not need to test the
            // remove lock flags for IO which would never be set.
            //
            return FxDeviceRemLockNotRequired;
#endif
        }
    }

    static
    FxDevice*
    GetFxDevice(
        __in MdDeviceObject DeviceObject
        );

    MdDeviceObject
    __inline
    GetSafePhysicalDevice(
        VOID
        )
    {
        //
        // Makes sure that the PDO we think we have is
        // 1)  reported to pnp (m_PdoKnown check)
        // 2)  actually there (m_PhysicalDevice != NULL check)
        //
        if (m_PdoKnown && m_PhysicalDevice.GetObject() != NULL) {
            return m_PhysicalDevice.GetObject();
        }
        else {
            return NULL;
        }
    }

    static
    _Must_inspect_result_
    NTSTATUS
    Dispatch(
        __in MdDeviceObject DeviceObject,
        __in MdIrp OriginalIrp
        );

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    static
    VOID
    DispatchUm(
        _In_ MdDeviceObject DeviceObject,
        _In_ MdIrp Irp,
        _In_opt_ IUnknown* Context
        );

    static
    VOID
    DispatchWithLockUm(
        _In_ MdDeviceObject DeviceObject,
        _In_ MdIrp Irp,
        _In_opt_ IUnknown* Context
        );

    VOID
    SetInterruptThreadpool(
        _In_ FxInterruptThreadpool* Pool
        )
    {
        m_InteruptThreadpool = Pool;
    }

    FxInterruptThreadpool*
    GetInterruptThreadpool(
        VOID
        )
    {
        return m_InteruptThreadpool;
    }

#endif // (FX_CORE_MODE == FX_CORE_USER_MODE)

    static
    _Must_inspect_result_
    NTSTATUS
    DispatchWithLock(
        __in MdDeviceObject DeviceObject,
        __in MdIrp OriginalIrp
        );

    _Must_inspect_result_
    NTSTATUS
    DispatchPreprocessedIrp(
        __in MdIrp       Irp,
        __in PVOID      DispatchContext
        );

    __inline
    WDF_DEVICE_IO_TYPE
    GetIoType(
        VOID
        )
    {
        return m_ReadWriteIoType;
    }

    __inline
    WDF_DEVICE_IO_TYPE
    GetIoTypeForReadWriteBufferAccess(
        VOID
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return m_ReadWriteIoType;
#else
        //
        // For UM, both buffer-copy and direct-access i/o buffer access types
        // follow the same storage and retrieval model in internal structures
        // as in buffered I/O so always return WdfDeviceIoBuffered.
        //
        return WdfDeviceIoBuffered;
#endif
    }

    __inline
    CHAR
    GetDefaultPriorityBoost(
        VOID
        )
    {
        return m_DefaultPriorityBoost;
    }

    //
    // Return FileObjectClass
    //
    __inline
    WDF_FILEOBJECT_CLASS
    GetFileObjectClass(
        VOID
        )
    {
        return m_FileObjectClass;
    }

    //
    // Configuration time fileobject support setting
    //
    __inline
    VOID
    SetFileObjectClass(
        __in WDF_FILEOBJECT_CLASS FileObjectClass
        )
    {
        m_FileObjectClass = FileObjectClass;
    }

    VOID
    InstallPackage(
        __inout FxPackage *Package
        );

    __inline
    WDF_DEVICE_PNP_STATE
    GetDevicePnpState(
        )
    {
        return m_CurrentPnpState;
    }

    __inline
    WDF_DEVICE_POWER_STATE
    GetDevicePowerState(
        )
    {
        return m_CurrentPowerState;
    }

    __inline
    WDF_DEVICE_POWER_POLICY_STATE
    GetDevicePowerPolicyState(
        )
    {
        return m_CurrentPowerPolicyState;
    }

    __inline
    VOID
    SetDevicePnpState(
        __in WDF_DEVICE_PNP_STATE DeviceState
        )
    {
        m_CurrentPnpState = DeviceState;
    }

    __inline
    VOID
    SetDevicePowerState(
        __in WDF_DEVICE_POWER_STATE DeviceState
        )
    {
        m_CurrentPowerState = DeviceState;
    }

    __inline
    VOID
    SetDevicePowerPolicyState(
        __in WDF_DEVICE_POWER_POLICY_STATE DeviceState
        )
    {
        m_CurrentPowerPolicyState = DeviceState;
    }

    __inline
    BOOLEAN
    IsPnp(
        VOID
        )
    {
        return m_PkgPnp != NULL ? TRUE : FALSE;
    }

    __inline
    BOOLEAN
    IsLegacy(
        VOID
        )
    {
        return m_Legacy;
    }

    __inline
    BOOLEAN
    IsExclusive(
        VOID
        )
    {
        return m_Exclusive;
    }

    __inline
    BOOLEAN
    IsFdo(
        VOID
        )
    {
        return m_PkgPnp->GetType() == FX_TYPE_PACKAGE_FDO;
    }

    __inline
    FxPkgFdo*
    GetFdoPkg(
        VOID
        )
    {
        return (FxPkgFdo*) m_PkgPnp;
    }

    __inline
    BOOLEAN
    IsPdo(
        VOID
        )
    {
        return (IsPnp() && m_PkgPnp->GetType() == FX_TYPE_PACKAGE_PDO);
    }

    __inline
    FxPkgPdo*
    GetPdoPkg(
        VOID
        )
    {
        return (FxPkgPdo*) m_PkgPnp;
    }

    _Must_inspect_result_
    NTSTATUS
    CreateDevice(
        __in PWDFDEVICE_INIT DeviceInit
        );

    __inline
    VOID
    SetParentWaitingOnRemoval(
        VOID
        )
    {
        m_ParentWaitingOnChild = TRUE;
    }

    //

    // There are really three steps in device creation.
    //
    //  - Creating the device
    //  - Creating the device object that goes with the device (Initialize)
    //  - Finilizing the initialization after packages are installed, attached,
    //    etc.





    VOID
    FinishInitializing(
        VOID
        );

    VOID
    Destroy(
        VOID
        );

    // <begin> FxObject overrides
    virtual
    VOID
    DeleteObject(
        VOID
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );
    // <end> FxObject overrides

    __inline
    PWDF_OBJECT_ATTRIBUTES
    GetRequestAttributes(
        VOID
        )
    {
        return &m_RequestAttributes;
    }

    PVOID
    AllocateRequestMemory(
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes
        );

    VOID
    FreeRequestMemory(
        __in FxRequest* Request
        );

    // begin FxDeviceBase overrides
    virtual
    _Must_inspect_result_
    NTSTATUS
    AddIoTarget(
        __inout FxIoTarget* IoTarget
        );

    virtual
    VOID
    RemoveIoTarget(
        __inout FxIoTarget* IoTarget
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    AllocateEnumInfo(
        VOID
        );

    virtual
    VOID
    AddChildList(
        __inout FxChildList* List
        );

    virtual
    VOID
    RemoveChildList(
        __inout FxChildList* List
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    AllocateDmaEnablerList(
        VOID
        );

    virtual
    VOID
    AddDmaEnabler(
        __inout FxDmaEnabler* Enabler
        );

    virtual
    VOID
    RemoveDmaEnabler(
        __inout FxDmaEnabler* Enabler
        );

    virtual
    FxIoTarget*
    GetDefaultIoTarget(
        VOID
        );

    FxIoTargetSelf*
    GetSelfIoTarget(
        VOID
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        );
    // end FxDeviceBase overrides

    //
    // Filter Driver Support
    //
    __inline
    BOOLEAN
    IsFilter()
    {
        return m_Filter;
    }

    _Must_inspect_result_
    NTSTATUS
    SetFilter(
        __in BOOLEAN Value
        );

    __inline
    BOOLEAN
    IsPowerPageableCapable(
        VOID
        )
    {
        return m_PowerPageableCapable;
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit,
        __in_opt PWDF_OBJECT_ATTRIBUTES DeviceAttributes
        );

    VOID
    ConfigureAutoForwardCleanupClose(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    PostInitialize(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PdoInitialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    FdoInitialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    _Must_inspect_result_
    NTSTATUS
    ControlDeviceInitialize(
        __in PWDFDEVICE_INIT DeviceInit
        );

    VOID
    ControlDeviceDelete(
        VOID
        )
    {
        //
        // FxDevice::DeleteObject() has already run, so we must call the super
        // class's version of DeleteObject();
        //
        ASSERT(m_DeviceObjectDeleted);

        __super::DeleteObject();
    }

    _Must_inspect_result_
    NTSTATUS
    OpenSettingsKey(
        __out HANDLE* Key,
        __in ACCESS_MASK DesiredAccess = STANDARD_RIGHTS_ALL
        );

    VOID
    DeleteSymbolicLink(
        VOID
        );

    __inline
    BYTE
    GetCallbackFlagsLocked(
        VOID
        )
    {
        return m_CallbackFlags;
    }

    __inline
    BYTE
    GetCallbackFlags(
        VOID
        )
    {
        BYTE flags;
        KIRQL irql;

        Lock(&irql);
        flags =  GetCallbackFlagsLocked();
        Unlock(irql);

        return flags;
    }

    __inline
    VOID
    SetCallbackFlagsLocked(
        __in BYTE Flags
        )
    {
        m_CallbackFlags |= Flags;
    }

    __inline
    VOID
    SetCallbackFlags(
        __in BYTE Flags
        )
    {
        KIRQL irql;

        Lock(&irql);
        SetCallbackFlagsLocked(Flags);
        Unlock(irql);
    }

    __inline
    VOID
    ClearCallbackFlagsLocked(
        __in BYTE Flags
        )
    {
        m_CallbackFlags &= ~Flags;
    }

    __inline
    VOID
    ClearCallbackFlags(
        __in BYTE Flags
        )
    {
        KIRQL irql;

        Lock(&irql);
        ClearCallbackFlagsLocked(Flags);
        Unlock(irql);
    }

    FxCxDeviceInfo*
    GetCxDeviceInfo(
        __in FxDriver*  CxDriver
        )
    {
        FxCxDeviceInfo* cxDeviceInfo;
        PLIST_ENTRY     next;

        //
        // Check if we are using I/O class extensions.
        //
        for (next = m_CxDeviceInfoListHead.Flink;
             next != &m_CxDeviceInfoListHead;
             next = next->Flink) {

            cxDeviceInfo = CONTAINING_RECORD(next, FxCxDeviceInfo, ListEntry);
            if (cxDeviceInfo->Driver == CxDriver) {
                return cxDeviceInfo;
            }
        }

        return NULL;
    }

    __inline
    BOOLEAN
    IsCxDriverInIoPath(
        __in FxDriver* CxDriver
        )
    {
        return (GetCxDeviceInfo(CxDriver) != NULL) ? TRUE : FALSE;
    }

    __inline
    BOOLEAN
    IsCxInIoPath(
        VOID
        )
    {
        return IsListEmpty(&m_CxDeviceInfoListHead) ? FALSE : TRUE;
    }

#if DBG
    __inline
    FxCxDeviceInfo*
    GetFirstCxDeviceInfo(
        VOID
        )
    {
        if (IsListEmpty(&m_CxDeviceInfoListHead)) {
            return NULL;
        }
        else {
            return CONTAINING_RECORD(m_CxDeviceInfoListHead.Flink,
                                     FxCxDeviceInfo,
                                     ListEntry);
        }
    }

    __inline
    FxCxDeviceInfo*
    GetNextCxDeviceInfo(
        __in FxCxDeviceInfo* CxDeviceInfo
        )
    {
        ASSERT(CxDeviceInfo != NULL);
        if (CxDeviceInfo->ListEntry.Flink == &m_CxDeviceInfoListHead) {
            return NULL;
        }
        else {
            return CONTAINING_RECORD(CxDeviceInfo->ListEntry.Flink,
                                     FxCxDeviceInfo,
                                     ListEntry);
        }
    }

#endif

    __inline
    static
    CCHAR
    GetCxDriverIndex(
        __in FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL) {
            return CxDeviceInfo->Index;
        }
        else {
            return 0;
        }
    }

    __inline
    FxDriver*
    GetCxDriver(
        __in FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL) {
            return CxDeviceInfo->Driver;
        }
        else {
            return GetDriver();
        }
    }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)

    static
    __inline
    NTSTATUS
    _OpenDeviceRegistryKey(
        _In_ MdDeviceObject DeviceObject,
        _In_ ULONG DevInstKeyType,
        _In_ ACCESS_MASK DesiredAccess,
        _Out_ PHANDLE DevInstRegKey
        );

    __inline
    static
    NTSTATUS
    _GetDeviceProperty(
        _In_       MdDeviceObject DeviceObject,
        _In_       DEVICE_REGISTRY_PROPERTY DeviceProperty,
        _In_       ULONG BufferLength,
        _Out_opt_  PVOID PropertyBuffer,
        _Out_      PULONG ResultLength
        );

#elif (FX_CORE_MODE == FX_CORE_USER_MODE)

    static
    NTSTATUS
    _OpenDeviceRegistryKey(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ IWudfDeviceStack* DeviceStack,
        _In_ PWSTR DriverName,
        _In_ ULONG DevInstKeyType,
        _In_ ACCESS_MASK DesiredAccess,
        _Out_ PHANDLE DevInstRegKey
        );

    static
    NTSTATUS
    _GetDeviceProperty(
        _In_       PVOID DeviceStack,
        _In_       DEVICE_REGISTRY_PROPERTY DeviceProperty,
        _In_       ULONG BufferLength,
        _Out_opt_  PVOID PropertyBuffer,
        _Out_      PULONG ResultLength
        );

#endif

    static
    _Must_inspect_result_
    NTSTATUS
    _ValidateOpenKeyParams(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _OpenKey(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device,
        _In_ ULONG DeviceInstanceKeyType,
        _In_ ACCESS_MASK DesiredAccess,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES KeyAttributes,
        _Out_ WDFKEY* Key
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _AllocAndQueryProperty(
        _In_ PFX_DRIVER_GLOBALS Globals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device,
        _In_opt_ MdDeviceObject RemotePdo,
        _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
        _In_ POOL_TYPE PoolType,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
        _Out_ WDFMEMORY* PropertyMemory
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _QueryProperty(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device,
        _In_opt_ MdDeviceObject RemotePdo,
        _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
        _In_ ULONG BufferLength,
        _Out_opt_ PVOID PropertyBuffer,
        _Out_opt_ PULONG ResultLength
        );

    static
    VOID
    _InterfaceReferenceNoOp(
        __in_opt PVOID Context
        )
    {
        // NoOp reference stub for query interface
        UNREFERENCED_PARAMETER(Context);
    }

    static
    VOID
    _InterfaceDereferenceNoOp(
        __in_opt PVOID Context
        )
    {
        // NoOp dereference stub for query interface
        UNREFERENCED_PARAMETER(Context);
    }

    static
    FxWdmDeviceExtension*
    _GetFxWdmExtension(
        __in MdDeviceObject DeviceObject
        );

    BOOLEAN
    IsRemoveLockEnabledForIo(
        VOID
        );

    VOID
    FxLogDeviceStartTelemetryEvent(
        VOID
        )
    {
        LogDeviceStartTelemetryEvent(GetDriverGlobals(), this);
    }

    virtual
    VOID
    SetDeviceTelemetryInfoFlags(
        _In_ FxDeviceInfoFlags Flag
        )
    {
        m_DeviceTelemetryInfoFlags |= Flag;
    }

    USHORT
    GetDeviceTelemetryInfoFlags(
        VOID
        )
    {
        return m_DeviceTelemetryInfoFlags;
    }

    __inline
    CHAR
    GetStackSize(
        VOID
        )
    {
        return m_DeviceObject.GetStackSize();
    }

    __inline
    VOID
    SetStackSize(
        _In_ CHAR Size
        )
    {
        m_DeviceObject.SetStackSize(Size);
    }

    NTSTATUS
    UpdateInterruptThreadpoolLimits(
        VOID
        )
    {





        return STATUS_SUCCESS;
    }

    FxCmResList*
    GetTranslatedResources(
        )
    {
        return m_PkgPnp->GetTranslatedResourceList();
    }

    VOID
    DetachDevice(
        VOID
        );

    VOID
    InvalidateDeviceState(
        VOID
        );

    NTSTATUS
    CreateSymbolicLink(
        _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
        _In_ PCUNICODE_STRING SymbolicLinkName
        );

    VOID
    SetCleanupFromFailedCreate(
        BOOLEAN Value
        )
    {
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
        m_CleanupFromFailedCreate = Value;
#else
        UNREFERENCED_PARAMETER(Value);
#endif
    }

    BOOLEAN
    IsInterfaceRegistered(
        _In_ const GUID* InterfaceClassGUID,
        _In_opt_ PCUNICODE_STRING RefString
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _AllocAndQueryPropertyEx(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device,
        _In_ PVOID PropertyData,
        _In_ FxPropertyType FxPropertyType,
        _In_ POOL_TYPE PoolType,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
        _Out_ WDFMEMORY*  PropertyMemory,
        _Out_ PDEVPROPTYPE PropertyType
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _QueryPropertyEx(
        _In_ PFX_DRIVER_GLOBALS DriverGlobals,
        _In_opt_ PWDFDEVICE_INIT DeviceInit,
        _In_opt_ FxDevice* Device,
        _In_ PVOID PropertyData,
        _In_ FxPropertyType FxPropertyType,
        _In_ ULONG BufferLength,
        _Out_ PVOID PropertyBuffer,
        _Out_ PULONG ResultLength,
        _Out_ PDEVPROPTYPE PropertyType
        );

    _Must_inspect_result_
    NTSTATUS
    OpenDevicemapKeyWorker(
        _In_ PFX_DRIVER_GLOBALS pFxDriverGlobals,
        _In_ PCUNICODE_STRING KeyName,
        _In_ ACCESS_MASK DesiredAccess,
        _In_ FxRegKey* pKey
        );

    _Must_inspect_result_
    NTSTATUS
    AssignProperty (
        _In_ PVOID PropertyData,
        _In_ FxPropertyType FxPropertyType,
        _In_ DEVPROPTYPE Type,
        _In_ ULONG BufferLength,
        _In_opt_ PVOID PropertyBuffer
        );

#if (FX_CORE_MODE==FX_CORE_USER_MODE)

    _Must_inspect_result_
    NTSTATUS
    FxValidateInterfacePropertyData(
        _In_ PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData
        );

    VOID
    GetDeviceStackIoType (
        _Out_ WDF_DEVICE_IO_TYPE* ReadWriteIoType,
        _Out_ WDF_DEVICE_IO_TYPE* IoControlIoType
        );

    __inline
    UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL
    GetRetrievalMode(
        VOID
        )
    {
        return m_RetrievalMode;
    }

    __inline
    WDF_DEVICE_IO_TYPE
    GetPreferredRWTransferMode(
        VOID
        )
    {
        return m_ReadWriteIoType;
    }

    __inline
    WDF_DEVICE_IO_TYPE
    GetPreferredIoctlTransferMode(
        VOID
        )
    {
        return m_IoctlIoType;
    }

    __inline
    ULONG
    GetDirectTransferThreshold(
        VOID
        )
    {
        return m_DirectTransferThreshold;
    }

    static
    VOID
    GetPreferredTransferMode(
        _In_ MdDeviceObject DeviceObject,
        _Out_ UMINT::WDF_DEVICE_IO_BUFFER_RETRIEVAL *RetrievalMode,
        _Out_ WDF_DEVICE_IO_TYPE *RWPreference,
        _Out_ WDF_DEVICE_IO_TYPE *IoctlPreference
        );

    NTSTATUS
    ProcessWmiPowerQueryOrSetData (
        _In_ RdWmiPowerAction   Action,
        _Out_ BOOLEAN *         QueryResult
        );

    static
    WUDF_INTERFACE_CONTEXT
    RemoteInterfaceArrival (
        _In_    IWudfDevice *   DeviceObject,
        _In_    LPCGUID         DeviceInterfaceGuid,
        _In_    PCWSTR          SymbolicLink
        );

    static
    void
    RemoteInterfaceRemoval (
        _In_    IWudfDevice *   DeviceObject,
        _In_    WUDF_INTERFACE_CONTEXT RemoteInterfaceID
        );

    static
    void
    PoFxDevicePowerRequired (
        _In_ MdDeviceObject DeviceObject
        );

    static
    void
    PoFxDevicePowerNotRequired (
        _In_ MdDeviceObject DeviceObject
        );

    static
    BOOL
    TransportQueryId (
        _In_    IWudfDevice *   DeviceObject,
        _In_    DWORD           Id,
        _In_    PVOID           DataBuffer,
        _In_    SIZE_T          cbDataBufferSize
        );

    static
    NTSTATUS
    NtStatusFromHr (
        _In_ IWudfDeviceStack * DevStack,
        _In_ HRESULT Hr
        );

    NTSTATUS
    NtStatusFromHr (
        _In_ HRESULT Hr
        );

    IWudfDeviceStack*
    GetDeviceStack(
        VOID
        );

    IWudfDeviceStack2 *
    GetDeviceStack2(
        VOID
        );

    VOID
    RetrieveDeviceRegistrySettings(
        VOID
        );

    BOOLEAN
    IsDirectHardwareAccessAllowed(
        )
    {
        return (m_DirectHardwareAccess == WdfAllowDirectHardwareAccess);
    }

    BOOLEAN
    IsInterruptAccessAllowed(
        VOID
        )
    {
        //
        // Allow access to interrupts if the device has any connection resources,
        // regardless of the UmdfDirectHardwareAccess INF directive.
        //
        return IsDirectHardwareAccessAllowed() ||
            GetTranslatedResources()->HasConnectionResources();
    }

    BOOLEAN
    AreRegistersMappedToUsermode(
        VOID
        )
    {
        return (m_RegisterAccessMode == WdfRegisterAccessUsingUserModeMapping);
    }

    PVOID
    GetPseudoAddressFromSystemAddress(
        __in PVOID SystemAddress
        )
    {
        return SystemAddress;
    }

    PVOID
    GetSystemAddressFromPseudoAddress(
        __in PVOID PseudoAddress
        )
    {
        return PseudoAddress;
    }

    static
    ULONG
    __inline
    GetLength(
        __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size
        )
    {
        ULONG length = 0;

        switch(Size) {
        case WdfDeviceHwAccessTargetSizeUchar:
            length = sizeof(UCHAR);
            break;
        case WdfDeviceHwAccessTargetSizeUshort:
            length = sizeof(USHORT);
            break;
        case WdfDeviceHwAccessTargetSizeUlong:
            length = sizeof(ULONG);
            break;
        case WdfDeviceHwAccessTargetSizeUlong64:
            length = sizeof(ULONG64);
            break;
        default:
            ASSERT(FALSE);
        }

        return length;
    }

    BOOL
    IsRegister(
        __in WDF_DEVICE_HWACCESS_TARGET_TYPE Type
        )
    {
        if (Type == WdfDeviceHwAccessTargetTypeRegister ||
            Type == WdfDeviceHwAccessTargetTypeRegisterBuffer) {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    IsPort(
        __in WDF_DEVICE_HWACCESS_TARGET_TYPE Type
        )
    {
        if (Type == WdfDeviceHwAccessTargetTypePort ||
            Type == WdfDeviceHwAccessTargetTypePortBuffer) {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    IsBufferType(
        __in WDF_DEVICE_HWACCESS_TARGET_TYPE Type
        )
    {
        if (Type == WdfDeviceHwAccessTargetTypeRegisterBuffer ||
            Type == WdfDeviceHwAccessTargetTypePortBuffer) {
            return TRUE;
        }

        return FALSE;
    }

    SIZE_T
    ReadRegister(
        __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
        __in PVOID Register
        );

    VOID
    ReadRegisterBuffer(
        __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
        __in PVOID Register,
        __out_ecount_full(Count) PVOID Buffer,
        __in ULONG Count
        );

    VOID
    WriteRegister(
        __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
        __in PVOID Register,
        __in SIZE_T Value
        );

    VOID
    WriteRegisterBuffer(
        __in WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
        __in PVOID Register,
        __in_ecount(Count) PVOID Buffer,
        __in ULONG Count
        );

    VOID
    RetrieveDeviceInfoRegistrySettings(
        _Out_ PCWSTR* GroupId,
        _Out_ PUMDF_DRIVER_REGSITRY_INFO DeviceRegInfo
        );

#endif // (FX_CORE_MODE == FX_CORE_USER_MODE)

};

class FxMpDevice : public FxDeviceBase {
public:
    FxMpDevice(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxDriver* Driver,
        __in MdDeviceObject DeviceObject,
        __in MdDeviceObject AttachedDevice,
        __in MdDeviceObject PDO
        ) :
        FxDeviceBase(FxDriverGlobals, Driver, FX_TYPE_MP_DEVICE, sizeof(*this))
    {
        Init(DeviceObject, AttachedDevice, PDO);
        m_DefaultTarget = NULL;

        Mx::MxReferenceObject(m_DeviceObject.GetObject());

        MarkDisposeOverride(ObjectDoNotLock);
    }

    // begin FxObject overrides
    BOOLEAN
    Dispose(
        VOID
        )
    {
        //
        // Important that the cleanup routine be called while the MdDeviceObject
        // is valid!
        //
        CallCleanup();

        //
        // Manually destroy the children now so that by the time we wait on the
        // dispose empty out, all of the children will have been added to it.
        //
        DestroyChildren();

        if (m_DisposeList != NULL) {
            m_DisposeList->WaitForEmpty();
        }

        //
        // No device object to delete since the caller's own the
        // WDM device.  Simulate what FxDevice::Destroy does by NULL'ing out the
        // device objects.
        //
        Mx::MxDereferenceObject(m_DeviceObject.GetObject());
        m_DeviceObject = NULL;
        m_AttachedDevice = NULL;

        return FALSE;
    }
    // end FxObject overrides

    // begin FxDeviceBase overrides
    virtual
    FxIoTarget*
    GetDefaultIoTarget(
        VOID
        )
    {
        return m_DefaultTarget;
    }
    // end FxDeviceBase overrides

public:
    //
    // Default I/O target for this miniport device
    //
    FxIoTarget *m_DefaultTarget;
};

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
#include "FxDeviceKm.hpp"
#else
#include "FxDeviceUm.hpp"
#endif


#endif // _FXDEVICE_H_
