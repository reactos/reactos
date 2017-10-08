/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for FsRtlDissectName/FsRtlDissectDbcs
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

static struct
{
    PCSTR Name;
    INT Offset1;
    INT Offset2;
    INT Length1;
    INT Length2;
} Tests[] =
{
    { NULL,         -1, -1       },
    { "",           -1, -1       },
    { "a",           0, -1, 1, 1 },
    { "a\\b",        0,  2, 1, 1 },
    { "a\\",         0,  2, 1, 0 },
    { "\\b",         1, -1, 1    },
    { "\\",          1, -1, 0    },
    { "a\\b\\c",     0,  2, 1, 3 },
    { "\\a\\b\\c",   1,  3, 1, 3 },
    /* Forward slashes are not separators */
    { "/",           0, -1, 1    },
    { "/a",          0, -1, 2    },
    { "/a/b",        0, -1, 4    },
    /* Normal parsing cycle */
    { "Good Morning!\\Good Evening!\\Good Night",   0,  14, 13, 24 },
    { "Good Evening!\\Good Night",                  0,  14, 13, 10 },
    { "Good Night",                                 0,  -1, 10 },
    /* Double backslashes */
    { "\\\\",        1,  2, 0, 0 },
    { "a\\\\",       0,  2, 1, 1 },
    { "\\\\b",       1,  2, 0, 1 },
    { "a\\\\b",      0,  2, 1, 2 },
    /* Even more backslashes */
    { "\\\\\\",      1,  2, 0, 1 },
    { "a\\\\\\",     0,  2, 1, 2 },
    { "\\\\\\b",     1,  2, 0, 2 },
    { "a\\\\\\b",    0,  2, 1, 3 },
    { "a\\\\\\\\b",  0,  2, 1, 4 },
};

START_TEST(FsRtlDissect)
{
    NTSTATUS Status;
    ANSI_STRING NameA;
    ANSI_STRING FirstA;
    ANSI_STRING RemainingA;
    UNICODE_STRING NameU;
    UNICODE_STRING FirstU;
    UNICODE_STRING RemainingU;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        RtlInitAnsiString(&NameA, Tests[i].Name);
        RtlFillMemory(&FirstA, sizeof(FirstA), 0x55);
        RtlFillMemory(&RemainingA, sizeof(RemainingA), 0x55);
        FsRtlDissectDbcs(NameA, &FirstA, &RemainingA);
        if (Tests[i].Offset1 == -1)
        {
            ok(FirstA.Buffer == NULL, "[%s] First=%p, expected NULL\n", Tests[i].Name, FirstA.Buffer);
            ok(FirstA.Length == 0, "[%s] FirstLen=%u, expected 0\n", Tests[i].Name, FirstA.Length);
            ok(FirstA.MaximumLength == 0, "[%s] FirstMaxLen=%u, expected 0\n", Tests[i].Name, FirstA.MaximumLength);
        }
        else
        {
            ok(FirstA.Buffer == NameA.Buffer + Tests[i].Offset1, "[%s] First=%p, expected %p\n", Tests[i].Name, FirstA.Buffer, NameA.Buffer + Tests[i].Offset1);
            ok(FirstA.Length == Tests[i].Length1, "[%s] FirstLen=%u, expected %d\n", Tests[i].Name, FirstA.Length, Tests[i].Length1);
            ok(FirstA.MaximumLength == Tests[i].Length1, "[%s] FirstMaxLen=%u, expected %d\n", Tests[i].Name, FirstA.MaximumLength, Tests[i].Length1);
        }
        if (Tests[i].Offset2 == -1)
        {
            ok(RemainingA.Buffer == NULL, "[%s] Remaining=%p, expected NULL\n", Tests[i].Name, RemainingA.Buffer);
            ok(RemainingA.Length == 0, "[%s] RemainingLen=%u, expected 0\n", Tests[i].Name, RemainingA.Length);
            ok(RemainingA.MaximumLength == 0, "[%s] RemainingMaxLen=%u, expected 0\n", Tests[i].Name, RemainingA.MaximumLength);
        }
        else
        {
            ok(RemainingA.Buffer == NameA.Buffer + Tests[i].Offset2, "[%s] Remaining=%p, expected %p\n", Tests[i].Name, RemainingA.Buffer, NameA.Buffer + Tests[i].Offset2);
            ok(RemainingA.Length == Tests[i].Length2, "[%s] RemainingLen=%u, expected %d\n", Tests[i].Name, RemainingA.Length, Tests[i].Length2);
            ok(RemainingA.MaximumLength == Tests[i].Length2, "[%s] RemainingMaxLen=%u, expected %d\n", Tests[i].Name, RemainingA.MaximumLength, Tests[i].Length2);
        }

        Status = RtlAnsiStringToUnicodeString(&NameU, &NameA, TRUE);
        if (skip(NT_SUCCESS(Status), "Conversion failed with %lx\n", Status))
            continue;
        RtlFillMemory(&FirstU, sizeof(FirstU), 0x55);
        RtlFillMemory(&RemainingU, sizeof(RemainingU), 0x55);
        FsRtlDissectName(NameU, &FirstU, &RemainingU);
        if (Tests[i].Offset1 == -1)
        {
            ok(FirstU.Buffer == NULL, "[%s] First=%p, expected NULL\n", Tests[i].Name, FirstU.Buffer);
            ok(FirstU.Length == 0, "[%s] FirstLen=%u, expected 0\n", Tests[i].Name, FirstU.Length);
            ok(FirstU.MaximumLength == 0, "[%s] FirstMaxLen=%u, expected 0\n", Tests[i].Name, FirstU.MaximumLength);
        }
        else
        {
            ok(FirstU.Buffer == NameU.Buffer + Tests[i].Offset1, "[%s] First=%p, expected %p\n", Tests[i].Name, FirstU.Buffer, NameU.Buffer + Tests[i].Offset1);
            ok(FirstU.Length == Tests[i].Length1 * sizeof(WCHAR), "[%s] FirstLen=%u, expected %d\n", Tests[i].Name, FirstU.Length, Tests[i].Length1 * sizeof(WCHAR));
            ok(FirstU.MaximumLength == Tests[i].Length1 * sizeof(WCHAR), "[%s] FirstMaxLen=%u, expected %d\n", Tests[i].Name, FirstU.MaximumLength, Tests[i].Length1 * sizeof(WCHAR));
        }
        if (Tests[i].Offset2 == -1)
        {
            ok(RemainingU.Buffer == NULL, "[%s] Remaining=%p, expected NULL\n", Tests[i].Name, RemainingU.Buffer);
            ok(RemainingU.Length == 0, "[%s] RemainingLen=%u, expected 0\n", Tests[i].Name, RemainingU.Length);
            ok(RemainingU.MaximumLength == 0, "[%s] RemainingMaxLen=%u, expected 0\n", Tests[i].Name, RemainingU.MaximumLength);
        }
        else
        {
            ok(RemainingU.Buffer == NameU.Buffer + Tests[i].Offset2, "[%s] Remaining=%p, expected %p\n", Tests[i].Name, RemainingU.Buffer, NameU.Buffer + Tests[i].Offset2);
            ok(RemainingU.Length == Tests[i].Length2 * sizeof(WCHAR), "[%s] RemainingLen=%u, expected %d\n", Tests[i].Name, RemainingU.Length, Tests[i].Length2 * sizeof(WCHAR));
            ok(RemainingU.MaximumLength == Tests[i].Length2 * sizeof(WCHAR), "[%s] RemainingMaxLen=%u, expected %d\n", Tests[i].Name, RemainingU.MaximumLength, Tests[i].Length2 * sizeof(WCHAR));
        }
        RtlFreeUnicodeString(&NameU);
    }
}
