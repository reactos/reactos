/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceToMxInterface.hpp

Abstract:

    Classes in this file are there to enable reaching into those classes that
    have not yet been made mode agnostic such as FxDevice, FxDeviceInit, FxPkgIo
    and others. These will not be needed once all objects have been made mode
    agnostic

    These classes will be implemented separately by respective frameworks

    Convention used:
    - Typically the first parameter to a method exposed by these classes is a
      pointer to the object that is being exposed. e.g. methods in FxDeviceToMx
      class have FxDevice* as the first parameter.

    - The name of method is same as the original method wherever applicable.

    - For a method of another object that is a member of the object being
      exposed, the name comprises member object's class name.
       e.g.  m_Device->m_PkgIo->StopProcessingForPower() is exposed as
       FxDeviceToMx::FxPkgIo_StopProcessingForPower( )
       The name of method begins with FxPkgIo_ because StopProcessingForPower is
       a method of FxPkgIo and not FxDevice.

Author:



Environment:


Revision History:

--*/

#pragma once





WDF_FILEOBJECT_CLASS
__inline
FxFileObjectClassNormalize(
    __in WDF_FILEOBJECT_CLASS FileObjectClass
    );

BOOLEAN
__inline
FxIsFileObjectOptional(
    __in WDF_FILEOBJECT_CLASS FileObjectClass
    );
