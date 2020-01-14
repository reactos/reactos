#ifndef _MXDEVICEOBJECT_H_
#define _MXDEVICEOBJECT_H_

#include "common/mxgeneral.h"

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
        #pragma warning(disable:28129)
        return m_DeviceObject->Flags;
    }

    __inline
    VOID
    SetFlags(
        ULONG Flags
        )
    {
        #pragma warning(disable:28129)
        m_DeviceObject->Flags = Flags;
    }

};

#endif //_MXDEVICEOBJECT_H_
