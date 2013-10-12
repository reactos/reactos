/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Event creation test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#ifdef _WIN64
#define KERNEL_HANDLE_FLAG 0xFFFFFFFF80000000ULL
#else
#define KERNEL_HANDLE_FLAG 0x80000000
#endif

#define CheckEventObject(Handle, Pointers, Handles) do                  \
{                                                                       \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                         \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,              \
                            &ObjectInfo, sizeof ObjectInfo, NULL);      \
    ok_eq_hex(Status, STATUS_SUCCESS);                                  \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                     \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                       \
    ok_eq_ulong(ObjectInfo.Attributes, 0);                              \
    ok_eq_ulong(ObjectInfo.GrantedAccess, EVENT_ALL_ACCESS);            \
    ok(((ULONG_PTR)Handle & KERNEL_HANDLE_FLAG) == KERNEL_HANDLE_FLAG,  \
       "Handle %p is not a kernel handle\n", Handle);                   \
} while (0)

static
VOID
TestCreateEvent(
    PRKEVENT (NTAPI *CreateEvent)(PUNICODE_STRING, PHANDLE),
    PUNICODE_STRING EventName,
    EVENT_TYPE Type)
{
    NTSTATUS Status;
    PKEVENT Event, Event2;
    HANDLE EventHandle, EventHandle2;
    LONG State;

    /* Nameless event */
    KmtStartSeh()
        Event = CreateEvent(NULL, &EventHandle);
        ok(Event != NULL, "Event is NULL\n");
        ok(EventHandle != NULL, "EventHandle is NULL\n");
        if (!skip(EventHandle != NULL, "No event\n"))
        {
            ok_eq_uint(Event->Header.Type, Type);
            State = KeReadStateEvent(Event);
            ok_eq_long(State, 1L);
            CheckEventObject(EventHandle, 2UL, 1UL);
            Status = ZwClose(EventHandle);
            ok_eq_hex(Status, STATUS_SUCCESS);
        }
    KmtEndSeh(STATUS_SUCCESS);

    /* Named event */
    EventHandle = NULL;
    Event = CreateEvent(EventName, &EventHandle);
    ok(Event != NULL, "Event is NULL\n");
    ok(EventHandle != NULL, "EventHandle is NULL\n");
    if (!skip(EventHandle != NULL, "No event\n"))
    {
        ok_eq_uint(Event->Header.Type, Type);
        State = KeReadStateEvent(Event);
        ok_eq_long(State, 1L);
        CheckEventObject(EventHandle, 3UL, 1UL);

        /* Open the existing one */
        EventHandle2 = NULL;
        Event2 = CreateEvent(EventName, &EventHandle2);
        CheckEventObject(EventHandle, 4UL, 2UL);
        ok(Event2 != NULL, "Event is NULL\n");
        ok(EventHandle2 != NULL, "EventHandle is NULL\n");
        if (!skip(EventHandle2 != NULL, "No event\n"))
        {
            CheckEventObject(EventHandle2, 4UL, 2UL);
            ZwClose(EventHandle2);
        }

        CheckEventObject(EventHandle, 3UL, 1UL);
        Status = ZwClose(EventHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

START_TEST(IoEvent)
{
    PKEVENT Event, Event2;
    HANDLE EventHandle = NULL, EventHandle2 = NULL;
    UNICODE_STRING NotificationEventName = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\KmTestIoEventNotificationEvent");
    UNICODE_STRING SynchronizationEventName = RTL_CONSTANT_STRING(L"\\BaseNamedObjects\\KmTestIoEventSynchronizationEvent");

    TestCreateEvent(IoCreateNotificationEvent, &NotificationEventName, NotificationEvent);
    TestCreateEvent(IoCreateSynchronizationEvent, &SynchronizationEventName, SynchronizationEvent);

    /* Create different types with the same name */
    Event = IoCreateNotificationEvent(&NotificationEventName, &EventHandle);
    ok(Event != NULL, "Event is NULL\n");
    ok(EventHandle != NULL, "EventHandle is NULL\n");
    if (!skip(EventHandle != NULL, "No event\n"))
    {
        ok_eq_uint(Event->Header.Type, NotificationEvent);
        Event2 = IoCreateSynchronizationEvent(&NotificationEventName, &EventHandle2);
        ok(Event2 != NULL, "Event is NULL\n");
        ok(EventHandle2 != NULL, "EventHandle is NULL\n");
        if (!skip(EventHandle2 != NULL, "No event\n"))
        {
            ok_eq_uint(Event2->Header.Type, NotificationEvent);
            ZwClose(EventHandle2);
        }
        ZwClose(EventHandle);
    }

}

