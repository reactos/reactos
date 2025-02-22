/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite UUIDs test
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

START_TEST(ExUuid)
{
    UUID Uuid;
    NTSTATUS Status;
    ULONG i;

    for (i = 0; i < 1000; i++)
    {
        Status = ExUuidCreate(&Uuid);
        ok(Status == STATUS_SUCCESS || Status == RPC_NT_UUID_LOCAL_ONLY,
           "ExUuidCreate returned unexpected status: 0x%lx\n", Status);
        ok((Uuid.Data3 & 0xF000) == 0x1000, "Invalid UUID version: 0x%x\n", (Uuid.Data3 & 0xF000));
        ok((Uuid.Data4[0] & 0xC0) == 0x80, "Invalid UUID variant: 0x%x\n", (Uuid.Data4[0] & 0xF0));
    }
}
