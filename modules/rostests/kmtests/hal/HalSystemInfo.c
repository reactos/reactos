/*
 * PROJECT:     ReactOS Kernel-Mode Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for HalQuerySystemInformation
 * COPYRIGHT:   Copyright 2020 Thomas Faber (thomas.faber@reactos.org)
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static HAL_AMLI_BAD_IO_ADDRESS_LIST ExpectedList[] =
{
    { 0x0000, 0x10, 1, NULL },
    { 0x0020, 0x02, 0, NULL },
    { 0x0040, 0x04, 1, NULL },
    { 0x0048, 0x04, 1, NULL },
    { 0x0070, 0x02, 1, NULL },
    { 0x0074, 0x03, 1, NULL },
    { 0x0081, 0x03, 1, NULL },
    { 0x0087, 0x01, 1, NULL },
    { 0x0089, 0x01, 1, NULL },
    { 0x008A, 0x02, 1, NULL },
    { 0x008F, 0x01, 1, NULL },
    { 0x0090, 0x02, 1, NULL },
    { 0x0093, 0x02, 1, NULL },
    { 0x0096, 0x02, 1, NULL },
    { 0x00A0, 0x02, 0, NULL },
    { 0x00C0, 0x20, 1, NULL },
    { 0x04D0, 0x02, 0, NULL },
    /* We obviously don't have the expected pointer. Just use a non-null value */
    { 0x0CF8, 0x08, 1, (PVOID)1 },
    { 0x0000, 0x00, 0, NULL },
};

static
void
TestAMLIllegalIOPortAddresses(void)
{
    NTSTATUS Status;
    PHAL_AMLI_BAD_IO_ADDRESS_LIST AddressList;
    ULONG AddressListLength;
    ULONG ReturnedLength;

    /* Query required size and check that it's valid */
    ReturnedLength = 0x55555555;
    Status = HalQuerySystemInformation(HalQueryAMLIIllegalIOPortAddresses,
                                       0,
                                       NULL,
                                       &ReturnedLength);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok(ReturnedLength % sizeof(*AddressList) == 0, "List size %lu is not a multiple of %Iu\n", ReturnedLength, sizeof(*AddressList));
    if (skip(ReturnedLength > 0 && ReturnedLength < 0x10000000, "Invalid length\n"))
    {
        return;
    }
    AddressListLength = ReturnedLength;
    AddressList = ExAllocatePoolWithTag(NonPagedPool,
                                        AddressListLength,
                                        'OImK');
    if (skip(AddressList != NULL, "Failed to alloc %lu bytes\n", AddressListLength))
    {
        return;
    }

    if (ReturnedLength != sizeof(*AddressList))
    {
        /* Try with space for exactly one entry and make sure we get
         * the same return code
         */
        RtlFillMemory(AddressList, AddressListLength, 0x55);
        ReturnedLength = 0x55555555;
        Status = HalQuerySystemInformation(HalQueryAMLIIllegalIOPortAddresses,
                                           sizeof(*AddressList),
                                           AddressList,
                                           &ReturnedLength);
        ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
        ok_eq_ulong(ReturnedLength, AddressListLength);
        ok_eq_hex(AddressList[0].BadAddrBegin, 0x55555555UL);
    }

    /* One byte less than required should still return no data */
    RtlFillMemory(AddressList, AddressListLength, 0x55);
    ReturnedLength = 0x55555555;
    Status = HalQuerySystemInformation(HalQueryAMLIIllegalIOPortAddresses,
                                       AddressListLength - 1,
                                       AddressList,
                                       &ReturnedLength);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok_eq_ulong(ReturnedLength, AddressListLength);
    ok_eq_hex(AddressList[0].BadAddrBegin, 0x55555555UL);

    /* Specify required size */
    RtlFillMemory(AddressList, AddressListLength, 0x55);
    ReturnedLength = 0x55555555;
    Status = HalQuerySystemInformation(HalQueryAMLIIllegalIOPortAddresses,
                                       AddressListLength,
                                       AddressList,
                                       &ReturnedLength);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(ReturnedLength, AddressListLength);

    /* Validate the table against our expectations */
    ok_eq_ulong(ReturnedLength, sizeof(ExpectedList));
    for (ULONG i = 0;
         i < min(ReturnedLength, sizeof(ExpectedList)) / sizeof(*AddressList);
         i++)
    {
        ok(AddressList[i].BadAddrBegin == ExpectedList[i].BadAddrBegin,
           "[%lu] BadAddrBegin 0x%lx, expected 0x%lx\n",
           i, AddressList[i].BadAddrBegin, ExpectedList[i].BadAddrBegin);
        ok(AddressList[i].BadAddrSize == ExpectedList[i].BadAddrSize,
           "[%lu] BadAddrSize 0x%lx, expected 0x%lx\n",
           i, AddressList[i].BadAddrSize, ExpectedList[i].BadAddrSize);
        ok(AddressList[i].OSVersionTrigger == ExpectedList[i].OSVersionTrigger,
           "[%lu] OSVersionTrigger 0x%lx, expected 0x%lx\n",
           i, AddressList[i].OSVersionTrigger, ExpectedList[i].OSVersionTrigger);
        if (ExpectedList[i].IOHandler != NULL)
        {
            ok(AddressList[i].IOHandler != NULL,
               "[%lu] IOHandler = %p\n", i, AddressList[i].IOHandler);
        }
        else
        {
            ok(AddressList[i].IOHandler == NULL,
               "[%lu] IOHandler = %p\n", i, AddressList[i].IOHandler);
        }

        /* If we got an I/O handler, try to call it */
        if (AddressList[i].IOHandler != NULL)
        {
            ULONG Data = 0x55555555;

            /* We don't want to break devices, so call it with an address
             * outside of its range, and it should return failure
             */
            Status = AddressList[i].IOHandler(TRUE,
                                              AddressList[i].BadAddrBegin - 1,
                                              1,
                                              &Data);
            ok(Status == STATUS_UNSUCCESSFUL,
               "[%lu] IOHandler returned 0x%lx\n", i, Status);
            ok(Data == 0x55555555,
               "[%lu] IOHandler returned Data 0x%lx\n", i, Data);
        }
    }
    ExFreePoolWithTag(AddressList, 'OImK');
}

START_TEST(HalSystemInfo)
{
    TestAMLIllegalIOPortAddresses();
}
