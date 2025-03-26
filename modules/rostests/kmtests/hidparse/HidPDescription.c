/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         HidParser description test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include <hidpddi.h>

#define NDEBUG
#include <debug.h>

#include "HidP.h"

static UCHAR ExampleKeyboardDescriptor[] = {
    0x05, 0x01,       /* Usage Page (Generic Desktop), */
    0x09, 0x06,       /* Usage (Keyboard), */
    0xA1, 0x01,       /* Collection (Application), */
    0x05, 0x07,       /*   Usage Page (Key Codes); */
    0x19, 0xE0,       /*   Usage Minimum (224), */
    0x29, 0xE7,       /*   Usage Maximum (231), */
    0x15, 0x00,       /*   Logical Minimum (0), */
    0x25, 0x01,       /*   Logical Maximum (1), */
    0x75, 0x01,       /*   Report Size (1), */
    0x95, 0x08,       /*   Report Count (8), */
    0x81, 0x02,       /*   Input (Data, Variable, Absolute), ;Modifier byte */
    0x95, 0x01,       /*   Report Count (1), */
    0x75, 0x08,       /*   Report Size (8), */
    0x81, 0x01,       /*   Input (Constant), ;Reserved byte */
    0x95, 0x05,       /*   Report Count (5), */
    0x75, 0x01,       /*   Report Size (1), */
    0x05, 0x08,       /*   Usage Page (Page# for LEDs), */
    0x19, 0x01,       /*   Usage Minimum (1), */
    0x29, 0x05,       /*   Usage Maximum (5), */
    0x91, 0x02,       /*   Output (Data, Variable, Absolute), ;LED report */
    0x95, 0x01,       /*   Report Count (1), */
    0x75, 0x03,       /*   Report Size (3), */
    0x91, 0x01,       /*   Output (Constant), ;LED report padding */
    0x95, 0x06,       /*   Report Count (6), */
    0x75, 0x08,       /*   Report Size (8), */
    0x15, 0x00,       /*   Logical Minimum (0), */
    0x25, 0x65,       /*   Logical Maximum (101), */
    0x05, 0x07,       /*   Usage Page (Key Codes), */
    0x19, 0x00,       /*   Usage Minimum (0), */
    0x29, 0x65,       /*   Usage Maximum (101) */
    0x81, 0x00,       /*   Input (Data, Array), ;Key arrays (6 bytes) */
    0xC0              /* End Collection */
};

static UCHAR PowerProEliteDescriptor[] = {
    0x05, 0x01,       /* Usage Page (Generic Desktop), */
    0x09, 0x04,       /* Usage (Joystick), */
    0xa1, 0x01,       /* Collection (Application), */
    0xa1, 0x02,       /*   Collection (Logical), */
    0x85, 0x01,       /*     Report ID (1) */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x01,       /*     Report Count (1), */
    0x15, 0x00,       /*     Logical Minimum (0), */
    0x26, 0xff, 0x00, /*     Logical Maximum (255), */
    0x81, 0x03,       /*     Input (Constant, Variable, Absolute), */
    0x75, 0x01,       /*     Report Size (1), */
    0x95, 0x13,       /*     Report Count (19), */
    0x15, 0x00,       /*     Logical Minimum (0), */
    0x25, 0x01,       /*     Logical Maximum (1), */
    0x35, 0x00,       /*     Physical Minimum (0), */
    0x45, 0x01,       /*     Physical Maximum (1), */
    0x05, 0x09,       /*     Usage Page (Button), */
    0x19, 0x01,       /*     Usage Minimum (1), */
    0x29, 0x13,       /*     Usage Maximum (19), */
    0x81, 0x02,       /*     Input (Data, Variable, Absolute), */
    0x75, 0x01,       /*     Report Size (1), */
    0x95, 0x0d,       /*     Report Count (13), */
    0x06, 0x00, 0xff, /*     Usage Page (Vendor-defined FF00), */
    0x81, 0x03,       /*     Input (Constant, Variable, Absolute), */
    0x15, 0x00,       /*     Logical Minimum (0), */
    0x26, 0xff, 0x00, /*     Logical Maximum (255), */
    0x05, 0x01,       /*     Usage Page (Generic Desktop), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0xa1, 0x00,       /*     Collection (Physical), */
    0x75, 0x08,       /*       Report Size (8), */
    0x95, 0x04,       /*       Report Count (4), */
    0x35, 0x00,       /*       Physical Minimum (0), */
    0x46, 0xff, 0x00, /*       Physical Maximum (255), */
    0x09, 0x30,       /*       Usage (X), */
    0x09, 0x31,       /*       Usage (Y), */
    0x09, 0x32,       /*       Usage (Z), */
    0x09, 0x35,       /*       Usage (Rz), */
    0x81, 0x02,       /*       Input (Data, Variable, Absolute), */
    0xc0,             /*     End Collection */
    0x05, 0x01,       /*     Usage Page (Generic Desktop), */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x27,       /*     Report Count (39), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0x81, 0x02,       /*     Input (Data, Variable, Absolute), */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x30,       /*     Report Count (48), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0x91, 0x02,       /*     Output (Data, Variable, Absolute), */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x30,       /*     Report Count (48), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0xb1, 0x02,       /*     Feature (Data, Variable, Absolute), */
    0xc0,             /*   End Collection */

    0xa1, 0x02,       /*   Collection (Logical), */
    0x85, 0x02,       /*     Report ID (2) */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x30,       /*     Report Count (48), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0xb1, 0x02,       /*     Feature (Data, Variable, Absolute), */
    0xc0,             /*   End Collection */
    0xa1, 0x02,       /*   Collection (Logical), */
    0x85, 0xee,       /*     Report ID (238) */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x30,       /*     Report Count (48), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0xb1, 0x02,       /*     Feature (Data, Variable, Absolute), */
    0xc0,             /*   End Collection */
    0xa1, 0x02,       /*   Collection (Logical), */
    0x85, 0xef,       /*     Report ID (239) */
    0x75, 0x08,       /*     Report Size (8), */
    0x95, 0x30,       /*     Report Count (48), */
    0x09, 0x01,       /*     Usage (Pointer), */
    0xb1, 0x02,       /*     Feature (Data, Variable, Absolute), */
    0xc0,             /*   End Collection */
    0xc0,             /* End Collection */
};
C_ASSERT(sizeof(PowerProEliteDescriptor) == 148);

static
VOID
TestGetCollectionDescription(VOID)
{
    NTSTATUS Status;
    HIDP_DEVICE_DESC DeviceDescription;

    /* Empty report descriptor */
    RtlFillMemory(&DeviceDescription, sizeof(DeviceDescription), 0x55);
    Status = HidP_GetCollectionDescription(NULL,
                                           0,
                                           NonPagedPool,
                                           &DeviceDescription);
    ok_eq_hex(Status, STATUS_NO_DATA_DETECTED);
    ok_eq_pointer(DeviceDescription.CollectionDesc, NULL);
    ok_eq_ulong(DeviceDescription.CollectionDescLength, 0);
    ok_eq_pointer(DeviceDescription.ReportIDs, NULL);
    ok_eq_ulong(DeviceDescription.ReportIDsLength, 0);
    if (NT_SUCCESS(Status)) HidP_FreeCollectionDescription(&DeviceDescription);

    /* Sample keyboard report descriptor from the HID spec */
    Status = HidP_GetCollectionDescription(ExampleKeyboardDescriptor,
                                           sizeof(ExampleKeyboardDescriptor),
                                           NonPagedPool,
                                           &DeviceDescription);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(DeviceDescription.CollectionDescLength, 1);
    ok_eq_ulong(DeviceDescription.ReportIDsLength, 1);
    if (!skip(NT_SUCCESS(Status), "Parsing failure\n"))
    {
        if (!skip(DeviceDescription.CollectionDescLength >= 1, "No collection\n"))
        {
            ok_eq_uint(DeviceDescription.CollectionDesc[0].UsagePage, HID_USAGE_PAGE_GENERIC);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].Usage, HID_USAGE_GENERIC_KEYBOARD);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].InputLength, 9);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].OutputLength, 2);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].FeatureLength, 0);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].PreparsedDataLength, 476);
        }
        if (!skip(DeviceDescription.ReportIDsLength >= 1, "No report IDs\n"))
        {
            ok_eq_uint(DeviceDescription.ReportIDs[0].ReportID, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[0].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[0].InputLength, 8);
            ok_eq_uint(DeviceDescription.ReportIDs[0].OutputLength, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[0].FeatureLength, 0);
        }
        HidP_FreeCollectionDescription(&DeviceDescription);
    }

    /* Regression test for CORE-11538 */
    Status = HidP_GetCollectionDescription(PowerProEliteDescriptor,
                                           sizeof(PowerProEliteDescriptor),
                                           NonPagedPool,
                                           &DeviceDescription);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(DeviceDescription.CollectionDescLength, 1);
    ok_eq_ulong(DeviceDescription.ReportIDsLength, 4);
    if (!skip(NT_SUCCESS(Status), "Parsing failure\n"))
    {
        if (!skip(DeviceDescription.CollectionDescLength >= 1, "No collection\n"))
        {
            ok_eq_uint(DeviceDescription.CollectionDesc[0].UsagePage, HID_USAGE_PAGE_GENERIC);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].Usage, HID_USAGE_GENERIC_JOYSTICK);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].InputLength, 49);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].OutputLength, 49);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].FeatureLength, 49);
            ok_eq_uint(DeviceDescription.CollectionDesc[0].PreparsedDataLength, 1388);
        }
        if (!skip(DeviceDescription.ReportIDsLength >= 1, "No first report ID\n"))
        {
            ok_eq_uint(DeviceDescription.ReportIDs[0].ReportID, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[0].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[0].InputLength, 49);
            ok_eq_uint(DeviceDescription.ReportIDs[0].OutputLength, 49);
            ok_eq_uint(DeviceDescription.ReportIDs[0].FeatureLength, 49);
        }
        if (!skip(DeviceDescription.ReportIDsLength >= 2, "No second report ID\n"))
        {
            ok_eq_uint(DeviceDescription.ReportIDs[1].ReportID, 2);
            ok_eq_uint(DeviceDescription.ReportIDs[1].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[1].InputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[1].OutputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[1].FeatureLength, 49);
        }
        if (!skip(DeviceDescription.ReportIDsLength >= 3, "No third report ID\n"))
        {
            ok_eq_uint(DeviceDescription.ReportIDs[2].ReportID, 238);
            ok_eq_uint(DeviceDescription.ReportIDs[2].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[2].InputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[2].OutputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[2].FeatureLength, 49);
        }
        if (!skip(DeviceDescription.ReportIDsLength >= 4, "No fourth report ID\n"))
        {
            ok_eq_uint(DeviceDescription.ReportIDs[3].ReportID, 239);
            ok_eq_uint(DeviceDescription.ReportIDs[3].CollectionNumber, 1);
            ok_eq_uint(DeviceDescription.ReportIDs[3].InputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[3].OutputLength, 0);
            ok_eq_uint(DeviceDescription.ReportIDs[3].FeatureLength, 49);
        }
        HidP_FreeCollectionDescription(&DeviceDescription);
    }
}

NTSTATUS
TestHidPDescription(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlCode,
    IN PVOID Buffer OPTIONAL,
    IN SIZE_T InLength,
    IN OUT PSIZE_T OutLength)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(InLength);
    UNREFERENCED_PARAMETER(OutLength);

    PAGED_CODE();

    NT_VERIFY(ControlCode == IOCTL_TEST_DESCRIPTION);

    TestGetCollectionDescription();

    return STATUS_SUCCESS;
}
