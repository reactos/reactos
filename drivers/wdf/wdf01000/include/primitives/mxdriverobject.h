#ifndef _MXDRIVEROBJECT_H_
#define _MXDRIVEROBJECT_H_

#include "mxgeneral.h"


typedef DRIVER_ADD_DEVICE MdDriverAddDeviceType, *MdDriverAddDevice;
typedef DRIVER_UNLOAD MdDriverUnloadType, *MdDriverUnload;
typedef DRIVER_DISPATCH MdDriverDispatchType, *MdDriverDispatch;

//
// Forward declare enum 
//
//enum FxDriverObjectUmFlags;

class MxDriverObject {

private:
    //
    // MdDeviceObject is typedef'ed to appropriate type for the mode
    // in the mode specific file
    //
    MdDriverObject m_DriverObject;

public:
    __inline
    MxDriverObject(
        __in MdDriverObject DriverObject
        ) :
        m_DriverObject(DriverObject)
    {
    }
    
    __inline
    MxDriverObject(
        VOID
        ) :
        m_DriverObject(NULL)
    {
    }

    __inline
    MdDriverObject
    GetObject(
        VOID
        )
    {
        return m_DriverObject;
    }

    VOID
    SetDriverExtensionAddDevice(
        _In_ MdDriverAddDevice Value
        )
    {
        m_DriverObject->DriverExtension->AddDevice = Value;
    }

    __inline
    VOID
    SetDriverUnload(
        _In_ MdDriverUnload Value
        )
    {
        m_DriverObject->DriverUnload = Value;
    }

    __inline
    VOID
    SetMajorFunction(
        _In_ UCHAR i,
        _In_ MdDriverDispatch Value
        )
    {
        m_DriverObject->MajorFunction[i] = Value;
    }

};

#endif //_MXDRIVEROBJECT_H_
