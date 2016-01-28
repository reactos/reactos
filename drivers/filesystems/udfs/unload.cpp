////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
#include "udffs.h"

VOID
NTAPI
UDFDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
//    UNICODE_STRING uniWin32NameString;
    LARGE_INTEGER delay;

    //
    // All *THIS* driver needs to do is to delete the device object and the
    // symbolic link between our device name and the Win32 visible name.
    //
    // Almost every other driver ever written would need to do a
    // significant amount of work here deallocating stuff.
    //

    KdPrint( ("UDF: Unloading!!\n") );

    // prevent mount oparations
    UDFGlobalData.UDFFlags |= UDF_DATA_FLAGS_BEING_UNLOADED;

    // wait for all volumes to be dismounted
    delay.QuadPart = 10*1000*1000*10;
    while(TRUE) {
        KdPrint(("Poll...\n"));
        KeDelayExecutionThread(KernelMode, FALSE, &delay);
    }

    // Create counted string version of our Win32 device name.


//    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );


    // Delete the link from our device name to a name in the Win32 namespace.

    
//    IoDeleteSymbolicLink( &uniWin32NameString );


    // Finally delete our device object


//    IoDeleteDevice( DriverObject->DeviceObject );
}
