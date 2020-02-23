#ifndef _FXDEVICE_H_
#define _FXDEVICE_H_


#include "common/fxnonpagedobject.h"
#include "common/fxdisposelist.h"
#include "common/mxdeviceobject.h"
#include "common/fxpackage.h"
#include "common/fxpkggeneral.h"
#include "common/fxpkgio.h"
#include "common/fxpkgpnp.h"
#include "common/fxwmiirphandler.h"
#include "common/fxdefaultirphandler.h"
#include "common/fxirp.h"
#include "common/fxpkgfdo.h"
#include "common/fxcxpnppowercallbacks.h"
#include "common/fxcxdeviceinfo.h"
#include "common/fxiotarget.h"
#include "common/fxpkgpdo.h"


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
    FxDmaPacketTransactionCompleted = 0,
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

class FxDriver;
class IFxHasCallbacks;

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
class FxDeviceBase : public FxNonPagedObject {

protected:
    FxDriver* m_Driver;

    MxDeviceObject m_DeviceObject;
    MxDeviceObject m_AttachedDevice;
    MxDeviceObject m_PhysicalDevice;

    //
    // Used to serialize packet dma transactions on this device.
    //
    LONG m_DmaPacketTransactionStatus;

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

    FxCallbackLock* m_CallbackLockPtr;
    FxObject* m_CallbackLockObjectPtr;

    WDF_EXECUTION_LEVEL m_ExecutionLevel;
    WDF_SYNCHRONIZATION_SCOPE m_SynchronizationScope;

public:

    //
    // This is used to defer items that must be cleaned up at passive
    // level, and FxDevice waits on this list to empty in DeviceRemove.
    //
    FxDisposeList* m_DisposeList;

    VOID
    AddToDisposeList(
        __inout FxObject* Object
        )
    {
        m_DisposeList->Add(Object);
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

    WDFDEVICE
    __inline
    GetHandle(
        VOID
        )
    {
        return (WDFDEVICE) GetObjectHandle();
    }

    NTSTATUS
    ConfigureConstraints(
        __in_opt PWDF_OBJECT_ATTRIBUTES ObjectAttributes
        );

    _Must_inspect_result_
    NTSTATUS
    AllocateTarget(
        _Out_ FxIoTarget** Target,
        _In_  BOOLEAN SelfTarget
        );

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

    VOID
    GetConstraints(
        __out_opt WDF_EXECUTION_LEVEL*       ExecutionLevel,
        __out_opt WDF_SYNCHRONIZATION_SCOPE* SynchronizationScope
        ) ;

    FxCallbackLock*
    GetCallbackLockPtr(
        __out_opt FxObject** LockObject
        );

    MdDeviceObject
    __inline
    GetAttachedDevice(
        VOID
        )
    {
        return m_AttachedDevice.GetObject();
    }

    MdDeviceObject
    __inline
    GetPhysicalDevice(
        VOID
        )
    {
        return m_PhysicalDevice.GetObject();
    }

    __inline
    FxDriver*
    GetDriver(
        VOID
        )
    {
        return m_Driver;
    }

    static
    FxDeviceBase*
    _SearchForDevice(
        __in FxObject* Object,
        __out_opt IFxHasCallbacks** Callbacks
        );

};

class FxDevice : public FxDeviceBase {

   friend class FxDriver;
   friend class FxIrp;
   friend class FxFileObject;
   friend class FxPkgPnp;

private:

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
    // If TRUE, m_DeviceObject was deleted in FxDevice::DeleteObject and should
    // not be deleted again later in the destroy path.
    //
    BOOLEAN m_DeviceObjectDeleted;

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
    // Maintain the current device states.
    //
    WDF_DEVICE_PNP_STATE            m_CurrentPnpState;
    WDF_DEVICE_POWER_STATE          m_CurrentPowerState;
    WDF_DEVICE_POWER_POLICY_STATE   m_CurrentPowerPolicyState;

    //
    // This boost will be used in IoCompleteRequest
    // for read, write and ioctl requests if the client driver
    // completes the request without specifying the boost.
    //
    //
    CHAR m_DefaultPriorityBoost;
    static const CHAR m_PriorityBoosts[];

    //
    // bit-map of device info for Telemetry
    //
    USHORT m_DeviceTelemetryInfoFlags;


    static
    _Must_inspect_result_
    NTSTATUS
    _AcquireOptinRemoveLock(
        __in MdDeviceObject DeviceObject,
        __in MdIrp Irp
        );

    VOID
    SetFilterIoType(
        VOID
        );

    //
    // A method called by the constructor(s) to initialize the device state.
    //
    VOID
    SetInitialState(
        VOID
        );

    VOID
    DestructorInternal(
        VOID
        )
    {
        // NOTHING TO DO
    }

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
    // Object attributes to apply to each FxRequest* returned by
    // m_RequestLookasideList
    //
    WDF_OBJECT_ATTRIBUTES m_RequestAttributes;

    //
    // Total size of an FxRequest + driver context LookasideList element.
    //
    size_t m_RequestLookasideListElementSize;

public:

    //
    // Track the parent if applicable
    //
    CfxDevice *m_ParentDevice;

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
    // We'll maintain the prepreocess table "per device" so that it is possible
    // to have different callbacks for each device.
    // Note that each device may be associted with multiple class extension in the future.
    //
    LIST_ENTRY          m_PreprocessInfoListHead;

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

    FxSpinLockTransactionedList m_IoTargetsList;

    WDF_FILEOBJECT_CLASS m_FileObjectClass;

    //
    // Optional, list of additional class extension settings.
    //
    LIST_ENTRY          m_CxDeviceInfoListHead;


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
    _Must_inspect_result_
    NTSTATUS
    Dispatch(
        __in MdDeviceObject DeviceObject,
        __in MdIrp OriginalIrp
        );

    __inline
    static
    FxDevice*
    GetFxDevice(
        __in MdDeviceObject DeviceObject
        )
    {
        MxDeviceObject deviceObject((MdDeviceObject)DeviceObject);

        //
        // DeviceExtension points to the start of the first context assigned to
        // WDFDEVICE.  We walk backwards from the context to the FxDevice*.
        //
        return (FxDevice*) CONTAINING_RECORD(deviceObject.GetDeviceExtension(),
                                             FxContextHeader,
                                             Context)->Object;
    }

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
            if (m_PkgPnp != NULL)
            {
                return (FxPackage*) m_PkgPnp;
            }
            else
            {
                return (FxPackage*) m_PkgDefault;
            }
            break;

        default:
            return (FxPackage*) m_PkgDefault;
        }
    }

    static
    _Must_inspect_result_
    NTSTATUS
    DispatchWithLock(
        __in MdDeviceObject DeviceObject,
        __in MdIrp OriginalIrp
        );

    __inline
    static
    FxWdmDeviceExtension*
    _GetFxWdmExtension(
        __in MdDeviceObject DeviceObject
        )
    {
        //
        // DeviceObject->DeviceExtension points to our FxDevice allocation.  We
        // get the underlying DeviceExtension allocated as part of the
        // PDEVICE_OBJECT by adding the sizeof(DEVICE_OBJECT) to get the start
        // of the DeviceExtension.  This is documented behavior on how the
        // DeviceExtension can be found.
        //
        return (FxWdmDeviceExtension*) WDF_PTR_ADD_OFFSET(DeviceObject,
                                                          sizeof(*DeviceObject));
    }

    MdRemoveLock
    GetRemoveLock(
        VOID
        )
    {
        return &FxDevice::_GetFxWdmExtension(
            GetDeviceObject())->IoRemoveLock;
    }

    _Must_inspect_result_
    NTSTATUS
    FxDevice::DeleteDeviceFromFailedCreate(
        __in NTSTATUS FailedStatus,
        __in BOOLEAN UseStateMachine
        );

    _Must_inspect_result_
    NTSTATUS
    DeleteDeviceFromFailedCreateNoDelete(
        __in NTSTATUS FailedStatus,
        __in BOOLEAN UseStateMachine
        );

    //
    // Filter Driver Support
    //
    __inline
    BOOLEAN
    IsFilter()
    {
        return m_Filter;
    }

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

    VOID
    Destroy(
        VOID
        );

    __inline
    VOID
    DetachDevice(
        VOID
        )
    {
        if (m_AttachedDevice.GetObject() != NULL)
        {
            Mx::MxDetachDevice(m_AttachedDevice.GetObject());
            m_AttachedDevice.SetObject(NULL);
        }
    }

    VOID
    __inline
    DeleteSymbolicLink(
        VOID
        )
    {
        if (m_SymbolicLinkName.Buffer != NULL)
        {
            //
            // Must be at PASSIVE_LEVEL for this call
            //
            if (m_SymbolicLinkName.Length)
            {
                Mx::MxDeleteSymbolicLink(&m_SymbolicLinkName);
            }

            FxPoolFree(m_SymbolicLinkName.Buffer);
            RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
        }
    }

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
        return IsACxPresent();
    }

    __inline
    BOOLEAN
    IsACxPresent(
        VOID
        )
    {
        return IsListEmpty(&m_CxDeviceInfoListHead) ? FALSE : TRUE;
    }

    VOID
    FinishInitializing(
        VOID
        );

    __inline
    BOOLEAN
    IsPnp(
        VOID
        )
    {
        return m_PkgPnp != NULL ? TRUE : FALSE;
    }

    __inline
    WDF_DEVICE_PNP_STATE
    GetDevicePnpState(
        )
    {
        return m_CurrentPnpState;
    }

    __inline
    VOID
    SetDevicePnpState(
        __in WDF_DEVICE_PNP_STATE DeviceState
        )
    {
        m_CurrentPnpState = DeviceState;
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PWDFDEVICE_INIT DeviceInit,
        __in_opt PWDF_OBJECT_ATTRIBUTES DeviceAttributes
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

    __inline
    FxDriver*
    GetDriver(
        VOID
        )
    {
        return m_Driver;
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

    VOID
    InstallPackage(
        __inout FxPackage *Package
        );

    VOID
    ConfigureAutoForwardCleanupClose(
        __in PWDFDEVICE_INIT DeviceInit
        );

    MdDeviceObject
    __inline
    GetAttachedDevice(
        VOID
        )
    {
        return m_AttachedDevice.GetObject();
    }

    _Must_inspect_result_
    NTSTATUS
    SetFilter(
        __in BOOLEAN Value
        );

    __inline
    FxPkgFdo*
    GetFdoPkg(
        VOID
        )
    {
        return (FxPkgFdo*) m_PkgPnp;
    }

    virtual
    VOID
    AddChildList(
        __inout FxChildList* List
        );

    MdDeviceObject
    __inline
    GetPhysicalDevice(
        VOID
        )
    {
        return m_PhysicalDevice.GetObject();
    }

    __inline
    FxCxDeviceInfo*
    GetFirstCxDeviceInfo(
        VOID
        )
    {
        if (IsListEmpty(&m_CxDeviceInfoListHead))
        {
            return NULL;
        }
        else
        {
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
        if (CxDeviceInfo->ListEntry.Flink == &m_CxDeviceInfoListHead)
        {
            return NULL;
        }
        else
        {
            return CONTAINING_RECORD(CxDeviceInfo->ListEntry.Flink,
                                     FxCxDeviceInfo,
                                     ListEntry);
        }
    }

    __inline
    static
    CCHAR
    GetCxDriverIndex(
        __in FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL)
        {
            return CxDeviceInfo->Index;
        }
        else
        {
            return 0;
        }
    }

    _Must_inspect_result_
    BOOLEAN
    IsCxUsingSelfManagedIo(
        VOID)
    {
        FxCxCallbackType callbackType;
        FxCxDeviceInfo *cxInfo;
        PFxCxPnpPowerCallbackContext context;
        BOOLEAN smIoUsed = FALSE;

        FxCxCallbackType smIoCallbackList[] =
        {
            FxCxCallbackSmIoInit,
            FxCxCallbackSmIoRestart,
            FxCxCallbackSmIoSuspend,
            FxCxCallbackSmIoFlush,
            FxCxCallbackSmIoCleanup
        };

        cxInfo = GetFirstCxDeviceInfo();

        while (cxInfo != NULL && smIoUsed == FALSE)
        {

            for (ULONG loop = 0; loop < ARRAYSIZE(smIoCallbackList); loop++)
            {
                callbackType = smIoCallbackList[loop];
                context = cxInfo->CxPnpPowerCallbackContexts[callbackType];

                if (context == NULL)
                {
                    continue;
                }

                if (context->IsSelfManagedIoUsed() == FALSE)
                {
                    continue;
                }

                //
                // Cx SmIo is used, so Self Managed Io State Machine is needed
                //
                smIoUsed = TRUE;
                break;
            }

            cxInfo = GetNextCxDeviceInfo(cxInfo);
        }

        return smIoUsed;
    }

    _Must_inspect_result_
    NTSTATUS
    CreateDevice(
        __in PWDFDEVICE_INIT DeviceInit
        );

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
    FxDriver*
    GetCxDriver(
        __in FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL) {
            return CxDeviceInfo->Driver;
        }
        else
        {
            return GetDriver();
        }
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

    __inline
    PWDF_OBJECT_ATTRIBUTES
    GetRequestAttributes(
        VOID
        )
    {
        return &m_RequestAttributes;
    }

    __inline
    CHAR
    GetDefaultPriorityBoost(
        VOID
        )
    {
        return m_DefaultPriorityBoost;
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

    _Must_inspect_result_
    NTSTATUS
    OpenSettingsKey(
        __out HANDLE* Key,
        __in ACCESS_MASK DesiredAccess = STANDARD_RIGHTS_ALL
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
        if (m_PdoKnown && m_PhysicalDevice.GetObject() != NULL)
        {
            return m_PhysicalDevice.GetObject();
        }
        else
        {
            return NULL;
        }
    }

    static
    __inline
    NTSTATUS
    _OpenDeviceRegistryKey(
        _In_ MdDeviceObject DeviceObject,
        _In_ ULONG DevInstKeyType,
        _In_ ACCESS_MASK DesiredAccess,
        _Out_ PHANDLE DevInstRegKey
        )
    {
        return IoOpenDeviceRegistryKey(DeviceObject, 
                                   DevInstKeyType,
                                   DesiredAccess,
                                   DevInstRegKey);
    }

    __inline
    VOID
    SetDevicePowerPolicyState(
        __in WDF_DEVICE_POWER_POLICY_STATE DeviceState
        )
    {
        m_CurrentPowerPolicyState = DeviceState;
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

    __inline
    VOID
    SetDevicePowerState(
        __in WDF_DEVICE_POWER_STATE DeviceState
        )
    {
        m_CurrentPowerState = DeviceState;
    }

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

    PVOID
    AllocateRequestMemory(
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes
        );

    VOID
    FreeRequestMemory(
        __in FxRequest* Request
        );

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

    

};

#endif //_FXDEVICE_H_
