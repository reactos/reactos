#ifndef _MXDEVICEOBJECT_H_
#define _MXDEVICEOBJECT_H_

#include "mxgeneral.h"

class MxDeviceObject {

private:
    //
    // MdDeviceObject is typedef'ed to appropriate type for the mode
    // in the mode specific file
    //
    MdDeviceObject m_DeviceObject;

public:
    __inline
    MxDeviceObject(
        __in MdDeviceObject DeviceObject
        ) :
        m_DeviceObject(DeviceObject)
    {
    }
    
    __inline
    MxDeviceObject(
        VOID
        ) :
        m_DeviceObject(NULL)
    {
    }

    CCHAR
    GetStackSize(
        VOID
        )
    {
        return m_DeviceObject->StackSize;
    }

    VOID
    SetStackSize(
        _In_ CCHAR Size
        )
    {
        m_DeviceObject->StackSize = Size;
    }

    __inline
    PVOID
    GetDeviceExtension(
        VOID
        )
    {
        return m_DeviceObject->DeviceExtension;
    }

    __inline
    MdDeviceObject
    GetObject(
        VOID
        )
    {
        return m_DeviceObject;
    }

    __inline
    VOID
    SetDeviceExtension(
        PVOID Value
        )
    {
        m_DeviceObject->DeviceExtension = Value;
    }

    __inline
    VOID
    SetObject(
        __in_opt MdDeviceObject DeviceObject
        )
    {
        m_DeviceObject = DeviceObject;
    }

    __inline
    ULONG
    GetFlags(
        VOID
        )
    {
        #ifdef _MSC_VER
        #pragma warning(disable:28129)
        #endif
        return m_DeviceObject->Flags;
    }

    __inline
    VOID
    SetFlags(
        ULONG Flags
        )
    {
        #ifdef _MSC_VER
        #pragma warning(disable:28129)
        #endif
        m_DeviceObject->Flags = Flags;
    }

    __inline
    VOID
    SetDeviceType(
        DEVICE_TYPE Value
        )
    {
        m_DeviceObject->DeviceType = Value;
    }
    
    __inline
    VOID
    SetCharacteristics(
        ULONG Characteristics
        )
    {
        m_DeviceObject->Characteristics = Characteristics;
    }

    __inline
    DEVICE_TYPE
    GetDeviceType(
        VOID
        )
    {
        return m_DeviceObject->DeviceType;
    }

    __inline
    ULONG
    GetCharacteristics(
        VOID
        )
    {
        return m_DeviceObject->Characteristics;
    }

    __inline
    POWER_STATE
    SetPowerState(
        __in POWER_STATE_TYPE  Type,
        __in POWER_STATE  State
        )
    {
        return PoSetPowerState(m_DeviceObject, Type, State);
    }

    __inline
    VOID
    InvalidateDeviceState(
        __in MdDeviceObject Fdo
        )
    {
        //
        // UMDF currently needs Fdo for InvalidateDeviceState
        // FDO is not used in km. 
        //
        // m_DeviceObject holds PDO that is what is used below.
        //

        UNREFERENCED_PARAMETER(Fdo);

        IoInvalidateDeviceState(m_DeviceObject);
    }

    __inline
    VOID
    DereferenceObject(
        )
    {
        ObDereferenceObject(m_DeviceObject);
    }

    __inline
    MdDeviceObject
    GetAttachedDeviceReference(
        VOID
        )
    {
        return IoGetAttachedDeviceReference(m_DeviceObject);
    }

    __inline
    VOID
    InvalidateDeviceRelations(
        __in DEVICE_RELATION_TYPE Type
        )
    {
            IoInvalidateDeviceRelations(m_DeviceObject, Type);
    }

};

#endif //_MXDEVICEOBJECT_H_
