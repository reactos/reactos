#pragma once

#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidclass.h>
#include <hidpddi.h>
#include <hidpi.h>
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
    // usage list length
    //
    USHORT UsageListLength;

    //
    // current usage list length
    //
    PUSAGE_AND_PAGE CurrentUsageList;

    //
    // previous usage list
    //
    PUSAGE_AND_PAGE PreviousUsageList;

    //
    // removed usage item list
    //
    PUSAGE_AND_PAGE BreakUsageList;

    //
    // new item usage list
    //
    PUSAGE_AND_PAGE MakeUsageList;

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
    PUCHAR Report;

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

}KBDHID_DEVICE_EXTENSION, *PKBDHID_DEVICE_EXTENSION;


NTSTATUS
KbdHid_InitiateRead(
    IN PKBDHID_DEVICE_EXTENSION DeviceExtension);
