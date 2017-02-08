#pragma once

#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidclass.h>
#include <hidpddi.h>
#include <hidpi.h>
#define NDEBUG
#include <debug.h>
#include <ntddmou.h>
#include <kbdmou.h>
#include <debug.h>


typedef struct
{
    //
    // lower device object
    //
    PDEVICE_OBJECT NextDeviceObject;

    //
    // irp which is used for reading input reports
    //
    PIRP Irp;

    //
    // event
    //
    KEVENT ReadCompletionEvent;

    //
    // device object for class callback
    //
    PDEVICE_OBJECT ClassDeviceObject;

    //
    // class callback
    //
    PVOID ClassService;

    //
    // mouse type
    //
    USHORT MouseIdentifier;

    //
    // wheel usage page
    //
    USHORT WheelUsagePage;

    //
    // buffer for the four usage lists below
    //
    PVOID UsageListBuffer;

    //
    // usage list length
    //
    USHORT UsageListLength;

    //
    // current usage list length
    //
    PUSAGE CurrentUsageList;

    //
    // previous usage list
    //
    PUSAGE PreviousUsageList;

    //
    // removed usage item list
    //
    PUSAGE BreakUsageList;

    //
    // new item usage list
    //
    PUSAGE MakeUsageList;

    //
    // preparsed data
    //
    PVOID PreparsedData;

    //
    // mdl for reading input report
    //
    PMDL ReportMDL;

    //
    // input report buffer
    //
    PCHAR Report;

    //
    // input report length
    //
    ULONG ReportLength;

    //
    // file object the device is reading reports from
    //
    PFILE_OBJECT FileObject;

    //
    // report read is active
    //
    UCHAR ReadReportActive;

    //
    // stop reading flag
    //
    UCHAR StopReadReport;

    //
    // mouse absolute
    //
    UCHAR MouseAbsolute;

    //
    // value caps x
    //
    HIDP_VALUE_CAPS ValueCapsX;

    //
    // value caps y button
    //
    HIDP_VALUE_CAPS ValueCapsY;

} MOUHID_DEVICE_EXTENSION, *PMOUHID_DEVICE_EXTENSION;

#define WHEEL_DELTA 120
#define VIRTUAL_SCREEN_SIZE_X (65536)
#define VIRTUAL_SCREEN_SIZE_Y (65536)

NTSTATUS
MouHid_InitiateRead(
    IN PMOUHID_DEVICE_EXTENSION DeviceExtension);

#define MOUHID_TAG 'diHM'
