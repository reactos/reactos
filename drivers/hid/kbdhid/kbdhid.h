#pragma once

#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidclass.h>
#include <hidpddi.h>
#include <hidpi.h>
#define NDEBUG
#include <debug.h>
#include <kbdmou.h>
//#include <kbd.h>
#include <ntddkbd.h>
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
    PHIDP_PREPARSED_DATA PreparsedData;

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
    // keyboard attributes
    //
    KEYBOARD_ATTRIBUTES Attributes;

    //
    // keyboard modifier state
    //
    HIDP_KEYBOARD_MODIFIER_STATE ModifierState;

    //
    // keyboard indicator state
    //
    KEYBOARD_INDICATOR_PARAMETERS KeyboardIndicator;

    //
    // keyboard type matic
    //
    KEYBOARD_TYPEMATIC_PARAMETERS KeyboardTypematic;

} KBDHID_DEVICE_EXTENSION, *PKBDHID_DEVICE_EXTENSION;

/* defaults from kbfiltr.h */
#define KEYBOARD_TYPEMATIC_RATE_MINIMUM 2
#define KEYBOARD_TYPEMATIC_RATE_MAXIMUM 30
#define KEYBOARD_TYPEMATIC_RATE_DEFAULT 30
#define KEYBOARD_TYPEMATIC_DELAY_MINIMUM 250
#define KEYBOARD_TYPEMATIC_DELAY_MAXIMUM 1000
#define KEYBOARD_TYPEMATIC_DELAY_DEFAULT 250

/* FIXME: write kbd.h */
#define MICROSOFT_KBD_FUNC              12
#define KEYBOARD_TYPE_UNKNOWN   (0x51)
#define MICROSOFT_KBD_101_TYPE           0


NTSTATUS
KbdHid_InitiateRead(
    IN PKBDHID_DEVICE_EXTENSION DeviceExtension);

#define KBDHID_TAG 'diHK'
