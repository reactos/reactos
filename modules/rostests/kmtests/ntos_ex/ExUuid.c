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

    Status = ExUuidCreate(&Uuid);
    ok(Status == STATUS_SUCCESS, "ExUuidCreate returned unexpected status: %lx\n", Status);
    ok((Uuid.Data3 & 0x1000) == 0x1000, "Invalid UUID version: %x\n", (Uuid.Data3 & 0xF000));
}
