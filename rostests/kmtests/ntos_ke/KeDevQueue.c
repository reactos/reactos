/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite for Device Queues
 * PROGRAMMERS:     Pavel Batusov, Moscow State Technical University
 *                  Denis Volhonsky, Moscow State Technical University
 *                  Alexandra Safonova, Moscow State Technical University
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define NUMBER 255
#define INSERT_COUNT 5

int Check_mem(void* a, int n, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (*((unsigned char*)a + i) != n) {
            return 0;
        }
    }
    return 1;
}

void Test_Initialize()
{
    PKDEVICE_QUEUE testing_queue;
    void* double_queue;

    trace("******* Testing KeInitializeDeviceQueue ************\n");
    DPRINT1("\nStart test for KeInitializeDeviceQueue function\n");

    testing_queue = ExAllocatePool(NonPagedPool, sizeof(KDEVICE_QUEUE));

    testing_queue->Busy = TRUE;
    testing_queue->Size = 0;

    KeInitializeDeviceQueue(testing_queue);

    /* Check for properly setting up fields */
    ok(!testing_queue->Busy, "(Initialize testing) Test 1:\tExpected 'not busy' status\n");
    DPRINT1("Test 1 completed\n");

    ok(testing_queue->Size == sizeof(KDEVICE_QUEUE), "(Initialize testing) Test 2:\tExpected another size for KDEVICE_QUEUE\n");
    DPRINT1("Test 2 completed\n");

    ok(testing_queue->Type == DeviceQueueObject, "(Initialize testing) Test 3:\tExpected type == DeviceQueueObject\n");
    DPRINT1("Test 3 completed\n");

    /* Make sure it does not write outside allocated buffer */
    double_queue = ExAllocatePool(NonPagedPool, sizeof(KDEVICE_QUEUE) * 2);

    memset(double_queue, NUMBER, sizeof(KDEVICE_QUEUE) * 2);
    KeInitializeDeviceQueue(double_queue);

    ok(Check_mem((void*)((char*)double_queue + sizeof(KDEVICE_QUEUE)), NUMBER, sizeof(KDEVICE_QUEUE)), "(Initialize testing) Test 4:\tFunction uses someone else's memory \n");
    DPRINT1("Test 4 completed\n");

//====================================================================

    ExFreePool(testing_queue);
    ExFreePool(double_queue);

    DPRINT1("KeInitializeDeviceQueue test finished\n");
}

void Tests_Insert_And_Delete()
{
    ULONG i, j;
    PKDEVICE_QUEUE testing_queue;
    PKDEVICE_QUEUE_ENTRY element;
    KIRQL OldIrql;
    PKDEVICE_QUEUE_ENTRY* elem_array;
    PKDEVICE_QUEUE_ENTRY return_value;
    PLIST_ENTRY next;
    ULONG key;

    trace("******* Testing KeInsertDeviceQueue **************** \n");
    DPRINT1("\nStart KeInsertDeviceQueue test\n");

    testing_queue = ExAllocatePool(NonPagedPool, sizeof(KDEVICE_QUEUE));
    KeInitializeDeviceQueue(testing_queue);

    element = ExAllocatePool(NonPagedPool, sizeof(KDEVICE_QUEUE_ENTRY));

    /* Raise to dispatch level */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeInsertDeviceQueue(testing_queue, element);
    ok(!element->Inserted, "Wrong 'Inserted' status\n");
    DPRINT1("Test 1 completed\n");

    /* Fill the queue*/
    elem_array = ExAllocatePool(NonPagedPool, sizeof(PKDEVICE_QUEUE_ENTRY) * INSERT_COUNT);

    DPRINT1("Arrow of tests starting\n");
    for (i = 0; i < INSERT_COUNT; i++) {
        elem_array[i] = ExAllocatePool(NonPagedPool, sizeof(KDEVICE_QUEUE_ENTRY));
        elem_array[i]->Inserted = FALSE;
        elem_array[i]->SortKey = i;
        KeInsertDeviceQueue(testing_queue, elem_array[i]);
        ok(elem_array[i]->Inserted, "Element was not inserted\n");
    }
    DPRINT1("Arrow of tests complete\n");

    ok(testing_queue->Size == sizeof(KDEVICE_QUEUE), "Wrong size of queue\n");

    /* Check how the queue was filled */
    next = &testing_queue->DeviceListHead;

    DPRINT1("Bunch of tests starting\n");
    for (i = 0; i < INSERT_COUNT; i++) {
        next = next->Flink;
        key = CONTAINING_RECORD(next, KDEVICE_QUEUE_ENTRY, DeviceListEntry)->SortKey;
        ok(key == i, "Sort key was changed\n");
    }
    DPRINT1("Bunch of tests completed\n");

    trace("****************************************************\n\n");
    DPRINT1("KeInsertDeviceQueue test finish\n");

    /* Test deletion */
    trace("******* Testing KeRemoveDeviceQueue **************** \n");
    DPRINT1("\nStart KeRemoveDeviceQueue test\n");

    DPRINT1("Start deleting elements from queue\n");
    for (i = 0; i < INSERT_COUNT; i++) {
        return_value = KeRemoveDeviceQueue(testing_queue);
        ok(return_value == elem_array[i], "Returning element != head element\n");
        ok(return_value->Inserted == FALSE, "Returning element is still in queue\n");
        next = &testing_queue->DeviceListHead;
        for (j = i + 1; j < INSERT_COUNT; j++) {
            next = next->Flink;
            ok(CONTAINING_RECORD(next, KDEVICE_QUEUE_ENTRY, DeviceListEntry)->SortKey == j, "Queue was damaged\n");
        }
    }
    DPRINT1("Deleting finish. Queue must be empty\n");

    ok(KeRemoveDeviceQueue(testing_queue) == NULL, "Queue is not empty\n");
    ok(testing_queue->Busy == FALSE, "Queue is busy\n");

    trace("****************************************************\n\n");
    DPRINT1("Finish KeRemoveDeviceQueue test\n");

    trace("******* Testing KeRemoveEntryDeviceQueue *********** \n");
    DPRINT1("\nStart KeRemoveEntryDeviceQueue test\n");

    DPRINT1("Filling queue\n");
    for (i = 0; i < INSERT_COUNT; i++) {
        elem_array[i]->SortKey = i;
        elem_array[i]->Inserted = FALSE;
        KeInsertDeviceQueue(testing_queue, elem_array[i]);
    }

    /* Delete half of all elements in the queue */
    DPRINT1("Deleting elements\n");
    for (i = 0; i < INSERT_COUNT / 2; i++) {
        ok(KeRemoveEntryDeviceQueue(testing_queue, elem_array[i * 2 + 1]), "Element is not deleted\n");
    }

    /* Checking queue */
    DPRINT1("Checking\n");
    next = &testing_queue->DeviceListHead;
    for (i = 0; i < INSERT_COUNT / 2 + 1; i++) {
        ok(CONTAINING_RECORD(next, KDEVICE_QUEUE_ENTRY, DeviceListEntry)->SortKey == i * 2, "Queue was damaged\n");
        next = next->Flink;
    }

    /* Trying delete elements, which are not in this queue */
    DPRINT1("Trying delete nonexistent elements\n");
    for (i = 0; i < INSERT_COUNT / 2; i++) {
        ok(!KeRemoveEntryDeviceQueue(testing_queue, elem_array[i * 2 + 1]), "Wrong remove operation\n");
    }

    trace("****************************************************\n\n");
    /* Freeing memory */
    for (i = 0; i < INSERT_COUNT; i++) {
        ExFreePool(elem_array[i]);
    }

    /* Return back to previous IRQL */
    KeLowerIrql(OldIrql);

    ExFreePool(testing_queue);
    ExFreePool(element);
}

START_TEST(KeDeviceQueue)
{
    Test_Initialize();
    Tests_Insert_And_Delete();
}

