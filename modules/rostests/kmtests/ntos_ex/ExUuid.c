/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite UUIDs test
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 *                  Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

START_TEST(ExUuid)
{
    UUID Uuid;
    NTSTATUS Status;

    Status = ExUuidCreate(&Uuid);
    // FIXME: Why does Test WHS report RPC_NT_UUID_LOCAL_ONLY instead? (ROSTESTS-359)
    ok(Status == STATUS_SUCCESS, "ExUuidCreate returned unexpected status: %lx\n", Status);

    ok((Uuid.Data3 & 0xF000) >> 12 == 1,
       "UUID version: expected 1, got %u\n",
       (Uuid.Data3 & 0xF000) >> 12);

    ok(((Uuid.Data4[0] & 0xF0) >> 4 & 0b1100) == 0b1000,
       "UUID variant: expected 0b10nn (v1), got 0x%X\n",
       (Uuid.Data4[0] & 0xF0) >> 4);
}
