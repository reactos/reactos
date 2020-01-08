#ifndef _MXDRIVEROBJECT_H_
#define _MXDRIVEROBJECT_H_

#include "common/mxgeneral.h"

//
// Forward declare enum 
//
enum FxDriverObjectUmFlags;

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

};

#endif //_MXDRIVEROBJECT_H_
