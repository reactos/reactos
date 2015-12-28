/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for sprintf
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <stdio.h>
#include <tchar.h>
#include <pseh/pseh2.h>
#include <ndk/mmfuncs.h>
#include <ndk/rtlfuncs.h>

#ifdef _MSC_VER
#pragma warning(disable:4778) // unterminated format string '%'
#else
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#pragma GCC diagnostic ignored "-Wnonnull"
#endif

static
PVOID
AllocateGuarded(
    SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

static
VOID
FreeGuarded(
    PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

/* NOTE: This test is not only used for all the CRT apitests, but also for
 *       user32's wsprintf. Make sure to test them all */
START_TEST(sprintf)
{
    int Length;
    CHAR Buffer[128];
    PCHAR String;

    /* basic parameter tests */
    StartSeh()
        Length = sprintf(NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        Length = sprintf(NULL, "");
        ok_int(Length, 0);
    EndSeh(STATUS_ACCESS_VIOLATION);

    StartSeh()
        Length = sprintf(NULL, "Hello");
        ok_int(Length, 5);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* some basic formats */
    Length = sprintf(Buffer, "abcde");
    ok_str(Buffer, "abcde");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%%");
    ok_str(Buffer, "%");
    ok_int(Length, 1);

    Length = sprintf(Buffer, "%");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%%%");
    ok_str(Buffer, "%");
    ok_int(Length, 1);

    Length = sprintf(Buffer, "%d", 8);
    ok_str(Buffer, "8");
    ok_int(Length, 1);

    Length = sprintf(Buffer, "%s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    /* field width for %s */
    Length = sprintf(Buffer, "%8s", "hello");
    ok_str(Buffer, "   hello");
    ok_int(Length, 8);

    Length = sprintf(Buffer, "%4s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%-8s", "hello");
    ok_str(Buffer, "hello   ");
    ok_int(Length, 8);

    Length = sprintf(Buffer, "%-5s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%0s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%-0s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%*s", -8, "hello");
#ifdef TEST_USER32
    ok_str(Buffer, "*s");
    ok_int(Length, 2);
#else
    ok_str(Buffer, "hello   ");
    ok_int(Length, 8);
#endif

    /* precision for %s */
    Length = sprintf(Buffer, "%.s", "hello");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%.0s", "hello");
    ok_str(Buffer, "");
    ok_int(Length, 0);

    Length = sprintf(Buffer, "%.10s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%.5s", "hello");
    ok_str(Buffer, "hello");
    ok_int(Length, 5);

    Length = sprintf(Buffer, "%.4s", "hello");
    ok_str(Buffer, "hell");
    ok_int(Length, 4);

    StartSeh()
        Length = sprintf(Buffer, "%.*s", -1, "hello");
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#endif
    EndSeh(STATUS_SUCCESS);

    String = AllocateGuarded(6);
    if (!String)
    {
        skip("Guarded allocation failure\n");
        return;
    }

    strcpy(String, "hello");
    StartSeh()
        Length = sprintf(Buffer, "%.8s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.6s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.5s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.4s", String);
        ok_str(Buffer, "hell");
        ok_int(Length, 4);
    EndSeh(STATUS_SUCCESS);

    String[5] = '!';
    StartSeh()
        Length = sprintf(Buffer, "%.5s", String);
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%.6s", String);
        ok_str(Buffer, "hello!");
        ok_int(Length, 6);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%.*s", 5, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello");
        ok_int(Length, 5);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%.*s", 6, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*s");
        ok_int(Length, 2);
#else
        ok_str(Buffer, "hello!");
        ok_int(Length, 6);
#endif
    EndSeh(STATUS_SUCCESS);

    /* both field width and precision */
    StartSeh()
        Length = sprintf(Buffer, "%8.5s", String);
        ok_str(Buffer, "   hello");
        ok_int(Length, 8);
#ifdef TEST_USER32
    EndSeh(STATUS_ACCESS_VIOLATION);
#else
    EndSeh(STATUS_SUCCESS);
#endif

    StartSeh()
        Length = sprintf(Buffer, "%-*.6s", -8, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*.6s");
        ok_int(Length, 4);
#else
        ok_str(Buffer, "hello!  ");
        ok_int(Length, 8);
#endif
    EndSeh(STATUS_SUCCESS);

    StartSeh()
        Length = sprintf(Buffer, "%*.*s", -8, 6, String);
#ifdef TEST_USER32
        ok_str(Buffer, "*.*s");
        ok_int(Length, 4);
#else
        ok_str(Buffer, "hello!  ");
        ok_int(Length, 8);
#endif
    EndSeh(STATUS_SUCCESS);

    FreeGuarded(String);
}
