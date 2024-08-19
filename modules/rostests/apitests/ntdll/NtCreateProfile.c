/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtCreateProfile
 * COPYRIGHT:   Copyright 2021 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

#define SIZEOF_MDL (5 * sizeof(PVOID) + 2 * sizeof(ULONG))
typedef ULONG_PTR PFN_NUMBER;
/* Maximum size that can be described by an MDL on 2003 and earlier */
#define MAX_MDL_BUFFER_SIZE ((MAXUSHORT - SIZEOF_MDL) / sizeof(PFN_NUMBER) * PAGE_SIZE + PAGE_SIZE - 1)

#define broken(cond) (strcmp(winetest_platform, "windows") ? 0 : cond)

static BOOL IsWow64;

static
void
TestParameterValidation(void)
{
    NTSTATUS Status;
    HANDLE ProfileHandle;

    Status = NtCreateProfile(NULL,
                             NULL,
                             NULL,
                             0,
                             0,
                             NULL,
                             0,
                             ProfileTime,
                             1);
    ok_hex(Status, STATUS_INVALID_PARAMETER_7);

    /* For addresses below 0x10000, there's a special check for BufferSize<4 -- on x86 only */
    {
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0xFFFF,
                                 0,
                                 0,
                                 NULL,
                                 3,
                                 ProfileTime,
                                 1);
        if (sizeof(PVOID) > sizeof(ULONG) || IsWow64)
        {
            ok_hex(Status, STATUS_INVALID_PARAMETER);
        }
        else
        {
            ok_hex(Status, STATUS_INVALID_PARAMETER_7);
        }

        /* Increasing the pointer gets us past this */
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0,
                                 0,
                                 NULL,
                                 3,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);

        /* So does increasing the size */
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0xFFFF,
                                 0,
                                 0,
                                 NULL,
                                 4,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);

        /* ... or, specifying a bucket size */
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0xFFFF,
                                 0,
                                 1,
                                 NULL,
                                 4,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);
    }

    /* Bucket sizes less than two or larger than 31 are invalid */
    {
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x80000000,
                                 1,
                                 NULL,
                                 1,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);

        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x80000000,
                                 32,
                                 NULL,
                                 1,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_PARAMETER);

        /* But 2 and 31 are valid */
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x80000000,
                                 2,
                                 NULL,
                                 1,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_BUFFER_TOO_SMALL);

        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x80000000,
                                 31,
                                 NULL,
                                 1,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_BUFFER_TOO_SMALL);
    }

    /* RangeSize validation has its own function */

    /* RangeBase+RangeSize can overflow into kernel space, but can't wrap around.
     * Note that a Wow64 test will never achieve overflow.
     */
    {
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 SIZE_MAX / 2,
                                 31,
                                 NULL,
                                 0x80000000,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_ACCESS_VIOLATION);

        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 SIZE_MAX - 0x10000,
                                 31,
                                 NULL,
                                 0x80000000,
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_ACCESS_VIOLATION);

        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 SIZE_MAX - 0x10000 + 1,
                                 31,
                                 NULL,
                                 0x80000000,
                                 ProfileTime,
                                 1);
        ok_hex(Status, IsWow64 ? STATUS_ACCESS_VIOLATION : STATUS_BUFFER_OVERFLOW);

        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 SIZE_MAX,
                                 31,
                                 NULL,
                                 0x80000000,
                                 ProfileTime,
                                 1);
        ok_hex(Status, IsWow64 ? STATUS_ACCESS_VIOLATION : STATUS_BUFFER_OVERFLOW);
    }

    /* Handle is probed first and requires no alignment, buffer requires ULONG alignment */
    {
        ULONG Buffer[1];

        Status = NtCreateProfile((PHANDLE)(ULONG_PTR)1,
                                 (HANDLE)(ULONG_PTR)1,
                                 (PVOID)(ULONG_PTR)0x10002,
                                 0x1000,
                                 31,
                                 (PVOID)(ULONG_PTR)2,
                                 sizeof(ULONG),
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_ACCESS_VIOLATION);

        Status = NtCreateProfile(&ProfileHandle,
                                 (HANDLE)(ULONG_PTR)1,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x1000,
                                 31,
                                 (PVOID)(ULONG_PTR)2,
                                 sizeof(ULONG),
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_DATATYPE_MISALIGNMENT);

        Status = NtCreateProfile(&ProfileHandle,
                                 (HANDLE)(ULONG_PTR)1,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x1000,
                                 31,
                                 (PVOID)(ULONG_PTR)4,
                                 sizeof(ULONG),
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_ACCESS_VIOLATION);

        Status = NtCreateProfile(&ProfileHandle,
                                 (HANDLE)(ULONG_PTR)1,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 0x1000,
                                 31,
                                 Buffer,
                                 sizeof(ULONG),
                                 ProfileTime,
                                 1);
        ok_hex(Status, STATUS_INVALID_HANDLE);
    }
}

/* There are bugs in this validation all the way through early Win10.
 * Therefore we test this more thoroughly.
 * See https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/profile/bugdemo.htm
 */
static
void
TestBufferSizeValidation(void)
{
    static const struct
    {
        INT Line;
        SIZE_T RangeSize;
        ULONG BucketSize;
        ULONG BufferSize;
        NTSTATUS ExpectedStatus;
        NTSTATUS BrokenStatus;
    } Tests[] =
    {
        /* RangeSize=(1 << BucketSize) means we'll need exactly one ULONG */
        { __LINE__,        0x4,  2, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL },
        { __LINE__,        0x4,  2, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,        0x8,  3, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL },
        { __LINE__,        0x8,  3, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,      0x400, 10, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL },
        { __LINE__,      0x400, 10, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__, 0x40000000, 30, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL },
        { __LINE__, 0x40000000, 30, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__, 0x80000000, 31, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL },
        { __LINE__, 0x80000000, 31, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },

        /* RangeSize<(1 << BucketSize) also means we'll need one ULONG.
         * However, old Windows versions get this wrong.
         */
        { __LINE__,          3,  2, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,          3,  2, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,          1,  2, sizeof(ULONG) - 1,  STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },

        /* Various sizes to show that the bug allows buffers that are a quarter of a bucket too big. */
        { __LINE__,          8,  3, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,          9,  3, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         10,  3, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                        
        { __LINE__,         16,  4, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,         17,  4, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         18,  4, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         19,  4, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         20,  4, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                        
        { __LINE__,         32,  5, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,         33,  5, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         39,  5, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,         40,  5, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                        
        { __LINE__,        256,  8, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,        257,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,        319,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,        320,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                        
        { __LINE__,        256,  8, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__,        257,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,        319,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__,        320,  8, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                        
        { __LINE__, 0x80000000, 31, sizeof(ULONG),      STATUS_ACCESS_VIOLATION },
        { __LINE__, 0x80000001, 31, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__, 0xBFFFFFFF, 31, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL, STATUS_ACCESS_VIOLATION },
        { __LINE__, 0xA0000000, 31, sizeof(ULONG),      STATUS_BUFFER_TOO_SMALL },
                                                                   
        /* Nothing checks against the max MDL size */              
        { __LINE__,          3,  2, MAX_MDL_BUFFER_SIZE,           STATUS_ACCESS_VIOLATION },
        { __LINE__,          3,  2, MAX_MDL_BUFFER_SIZE + 1,       STATUS_ACCESS_VIOLATION },
        { __LINE__,          3,  2, (MAX_MDL_BUFFER_SIZE + 1) * 2, STATUS_ACCESS_VIOLATION },

    };
    NTSTATUS Status;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        Status = NtCreateProfile(NULL,
                                 NULL,
                                 (PVOID)(ULONG_PTR)0x10000,
                                 Tests[i].RangeSize,
                                 Tests[i].BucketSize,
                                 NULL,
                                 Tests[i].BufferSize,
                                 ProfileTime,
                                 1);
        if (Tests[i].BrokenStatus)
        {
            ok(Status == Tests[i].ExpectedStatus ||
               broken(Status == Tests[i].BrokenStatus),
               "[L%d] For RangeSize 0x%Ix, BucketSize %lu, BufferSize %lu, expected 0x%lx, got 0x%lx\n",
               Tests[i].Line, Tests[i].RangeSize, Tests[i].BucketSize, Tests[i].BufferSize, Tests[i].ExpectedStatus, Status);
        }
        else
        {
            ok(Status == Tests[i].ExpectedStatus,
               "[L%d] For RangeSize 0x%Ix, BucketSize %lu, BufferSize %lu, expected 0x%lx, got 0x%lx\n",
               Tests[i].Line, Tests[i].RangeSize, Tests[i].BucketSize, Tests[i].BufferSize, Tests[i].ExpectedStatus, Status);
        }
    }
}

START_TEST(NtCreateProfile)
{
    IsWow64Process(GetCurrentProcess(), &IsWow64);
    TestParameterValidation();
    TestBufferSizeValidation();
}
