/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Object Handle test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#define NDEBUG
#include <debug.h>

#define CheckObject(Handle, Pointers, Handles, Attrib, Access) do   \
{                                                                   \
    PUBLIC_OBJECT_BASIC_INFORMATION ObjectInfo;                     \
    Status = ZwQueryObject(Handle, ObjectBasicInformation,          \
                           &ObjectInfo, sizeof ObjectInfo, NULL);   \
    ok_eq_hex(Status, STATUS_SUCCESS);                              \
    ok_eq_hex(ObjectInfo.Attributes, Attrib);                       \
    ok_eq_hex(ObjectInfo.GrantedAccess, Access);                    \
    ok_eq_ulong(ObjectInfo.PointerCount, Pointers);                 \
    ok_eq_ulong(ObjectInfo.HandleCount, Handles);                   \
} while (0)

#define KERNEL_HANDLE_FLAG ((ULONG_PTR)0xFFFFFFFF80000000)
#define IsUserHandle(h)   (((ULONG_PTR)(h) & KERNEL_HANDLE_FLAG) == 0)
#define IsKernelHandle(h) (((ULONG_PTR)(h) & KERNEL_HANDLE_FLAG) == KERNEL_HANDLE_FLAG)

static HANDLE SystemProcessHandle;

static
VOID
TestDuplicate(
    _In_ HANDLE Handle)
{
    NTSTATUS Status;
    HANDLE NewHandle;
    struct
    {
        ACCESS_MASK DesiredAccess;
        ULONG RequestedAttributes;
        ULONG Options;
        ACCESS_MASK GrantedAccess;
        ULONG ExpectedAttributes;
    } Tests[] =
    {
        { DIRECTORY_ALL_ACCESS, 0,                  0,
          DIRECTORY_ALL_ACCESS, 0 },
        { DIRECTORY_ALL_ACCESS, OBJ_KERNEL_HANDLE,  0,
          DIRECTORY_ALL_ACCESS, 0 },
        { DIRECTORY_QUERY,      0,                  0,
          DIRECTORY_QUERY,      0 },
        { DIRECTORY_QUERY,      OBJ_INHERIT,        0,
          DIRECTORY_QUERY,      OBJ_INHERIT },
        { DIRECTORY_QUERY,      OBJ_INHERIT,        DUPLICATE_SAME_ACCESS,
          DIRECTORY_ALL_ACCESS, OBJ_INHERIT },
        /* 5 */
        { DIRECTORY_QUERY,      OBJ_INHERIT,        DUPLICATE_SAME_ATTRIBUTES,
          DIRECTORY_QUERY,      0 },
        { DIRECTORY_QUERY,      OBJ_INHERIT,        DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES,
          DIRECTORY_ALL_ACCESS, 0 },
    };
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        trace("Test %lu\n", i);
        Status = ZwDuplicateObject(ZwCurrentProcess(),
                                   Handle,
                                   ZwCurrentProcess(),
                                   &NewHandle,
                                   Tests[i].DesiredAccess,
                                   Tests[i].RequestedAttributes,
                                   Tests[i].Options);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (!skip(NT_SUCCESS(Status), "DuplicateHandle failed\n"))
        {
            ok(IsUserHandle(NewHandle), "New handle = %p\n", NewHandle);
            CheckObject(NewHandle, 3UL, 2UL, Tests[i].ExpectedAttributes, Tests[i].GrantedAccess);
            CheckObject(Handle, 3UL, 2UL, 0UL, DIRECTORY_ALL_ACCESS);

            Status = ObCloseHandle(NewHandle, UserMode);
            ok_eq_hex(Status, STATUS_SUCCESS);
            CheckObject(Handle, 2UL, 1UL, 0UL, DIRECTORY_ALL_ACCESS);
        }
    }

    /* If TargetProcess is the System process, we do get a kernel handle */
    Status = ZwDuplicateObject(ZwCurrentProcess(),
                               Handle,
                               SystemProcessHandle,
                               &NewHandle,
                               DIRECTORY_ALL_ACCESS,
                               OBJ_KERNEL_HANDLE,
                               0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "DuplicateHandle failed\n"))
    {
        ok(IsKernelHandle(NewHandle), "New handle = %p\n", NewHandle);
        CheckObject(NewHandle, 3UL, 2UL, 0, DIRECTORY_ALL_ACCESS);
        CheckObject(Handle, 3UL, 2UL, 0UL, DIRECTORY_ALL_ACCESS);

        Status = ObCloseHandle(NewHandle, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        CheckObject(NewHandle, 3UL, 2UL, 0, DIRECTORY_ALL_ACCESS);
        CheckObject(Handle, 3UL, 2UL, 0UL, DIRECTORY_ALL_ACCESS);

        if (IsKernelHandle(NewHandle))
        {
            Status = ObCloseHandle(NewHandle, KernelMode);
            ok_eq_hex(Status, STATUS_SUCCESS);
            CheckObject(Handle, 2UL, 1UL, 0UL, DIRECTORY_ALL_ACCESS);
        }
    }
}

START_TEST(ObHandle)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KernelDirectoryHandle;
    HANDLE UserDirectoryHandle;

    Status = ObOpenObjectByPointer(PsInitialSystemProcess,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &SystemProcessHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No handle for system process\n"))
    {
        SystemProcessHandle = NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    Status = ZwCreateDirectoryObject(&UserDirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "No directory handle\n"))
    {
        ok(IsUserHandle(UserDirectoryHandle), "User handle = %p\n", UserDirectoryHandle);
        CheckObject(UserDirectoryHandle, 2UL, 1UL, 0UL, DIRECTORY_ALL_ACCESS);

        TestDuplicate(UserDirectoryHandle);

        Status = ObCloseHandle(UserDirectoryHandle, UserMode);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateDirectoryObject(&KernelDirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status), "No directory handle\n"))
    {
        ok(IsKernelHandle(KernelDirectoryHandle), "Kernel handle = %p\n", KernelDirectoryHandle);
        CheckObject(KernelDirectoryHandle, 2UL, 1UL, 0UL, DIRECTORY_ALL_ACCESS);

        TestDuplicate(KernelDirectoryHandle);

        Status = ObCloseHandle(KernelDirectoryHandle, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        CheckObject(KernelDirectoryHandle, 2UL, 1UL, 0UL, DIRECTORY_ALL_ACCESS);

        Status = ObCloseHandle(KernelDirectoryHandle, KernelMode);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    /* Tests for closing handles */
    KmtStartSeh()
        /* NtClose must accept everything */
        DPRINT("Closing null handle (NtClose)\n");
        Status = NtClose(NULL);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)0x80000000);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 handle (NtClose)\n");
        Status = NtClose((HANDLE)0x7FFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)0xFFFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 handle (NtClose)\n");
        Status = NtClose((HANDLE)123);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 kernel handle (NtClose)\n");
        Status = NtClose((HANDLE)(123 | 0x80000000));
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);

        /* ObCloseHandle with UserMode accepts everything */
        DPRINT("Closing null handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle(NULL, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0x80000000, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0x7FFFFFFF, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing -1 kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)0xFFFFFFFF, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)123, UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing 123 kernel handle (ObCloseHandle, UserMode)\n");
        Status = ObCloseHandle((HANDLE)(123 | 0x80000000), UserMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);

        /* ZwClose only accepts 0 and -1 */
        DPRINT("Closing null handle (ZwClose)\n");
        Status = ZwClose(NULL);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ZwClose)\n");
        Status = ZwClose((HANDLE)0x80000000);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7FFFFFFF
        Status = ZwClose((HANDLE)0x7FFFFFFF);*/
        DPRINT("Closing -1 kernel handle (ZwClose)\n");
        Status = ZwClose((HANDLE)0xFFFFFFFF);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7B, 1, 0, 0
        Status = ZwClose((HANDLE)123);
        Status = ZwClose((HANDLE)(123 | 0x80000000));*/

        /* ObCloseHandle with KernelMode accepts only 0 and -1 */
        DPRINT("Closing null handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle(NULL, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        DPRINT("Closing null kernel handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle((HANDLE)0x80000000, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7FFFFFFF, 1, 0, 0
        Status = ObCloseHandle((HANDLE)0x7FFFFFFF, KernelMode);*/
        DPRINT("Closing -1 kernel handle (ObCloseHandle, KernelMode)\n");
        Status = ObCloseHandle((HANDLE)0xFFFFFFFF, KernelMode);
        ok_eq_hex(Status, STATUS_INVALID_HANDLE);
        /* INVALID_KERNEL_HANDLE, 0x7B, 1, 0, 0
        Status = ObCloseHandle((HANDLE)123, KernelMode);
        Status = ObCloseHandle((HANDLE)(123 | 0x80000000), KernelMode);*/
    KmtEndSeh(STATUS_SUCCESS);

    if (SystemProcessHandle)
    {
        ObCloseHandle(SystemProcessHandle, KernelMode);
    }
}
