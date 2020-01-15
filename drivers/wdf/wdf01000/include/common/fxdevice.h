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


struct FxWdmDeviceExtension {
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    WUDF_IO_REMOVE_LOCK IoRemoveLock;
#else
    IO_REMOVE_LOCK  IoRemoveLock;
#endif
    ULONG           RemoveLockOptionFlags;
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

//
// Base class for all devices.
//
class FxDeviceBase : public FxNonPagedObject {

protected:
    FxDriver* m_Driver;

    MxDeviceObject m_DeviceObject;
    MxDeviceObject m_AttachedDevice;
    MxDeviceObject m_PhysicalDevice;

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

    WDFDEVICE
    __inline
    GetHandle(
        VOID
        )
    {
        return (WDFDEVICE) GetObjectHandle();
    }

};

class FxDevice : public FxDeviceBase {

   friend class FxDriver;
   friend class FxIrp;
   friend class FxFileObject;
   friend class FxPkgPnp;

private:

    // TRUE if a Filter
    BOOLEAN m_Filter;

    //
    // If TRUE, m_DeviceObject was deleted in FxDevice::DeleteObject and should
    // not be deleted again later in the destroy path.
    //
    BOOLEAN m_DeviceObjectDeleted;

    //
    // Maintain the current device states.
    //
    WDF_DEVICE_PNP_STATE            m_CurrentPnpState;
    WDF_DEVICE_POWER_STATE          m_CurrentPowerState;
    WDF_DEVICE_POWER_POLICY_STATE   m_CurrentPowerPolicyState;

    static
    _Must_inspect_result_
    NTSTATUS
    _AcquireOptinRemoveLock(
        __in MdDeviceObject DeviceObject,
        __in MdIrp Irp
        );

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

};

#endif //_FXDEVICE_H_
