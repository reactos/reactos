/*
* PROJECT:         ReactOS API tests
* LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
* PURPOSE:         Test for atexit
* PROGRAMMER:      Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <apitest.h>

#include <stdlib.h>
#include <stdio.h>
#include <ntndk.h>

int g_sequence = 0;
HANDLE g_hSemaphore;

void exitfunc1(void)
{
    ok_int(g_sequence, 1);
    g_sequence++;
    ReleaseSemaphore(g_hSemaphore, 1, NULL);
}

void exitfunc2(void)
{
    ok_int(g_sequence, 2);
    g_sequence++;
    ReleaseSemaphore(g_hSemaphore, 1, NULL);
}

void exitfunc3(void)
{
    ok_int(g_sequence, 0);
    g_sequence++;
    ReleaseSemaphore(g_hSemaphore, 1, NULL);
    printf("exitfunc3\n");
}

typedef int (__cdecl *PFN_atexit)(void (__cdecl*)(void));

void Test_atexit()
{
    HMODULE hmod;
    PFN_atexit patexit;

    /* Open the named sempahore to count atexit callbacks */
    g_hSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "atext_apitest_sempahore");
    ok(g_hSemaphore != NULL, "couldn't open semaphore.\n");

    /* Load atexit from msvcrt.dll */
    hmod = GetModuleHandleA("msvcrt.dll");
    patexit = (PFN_atexit)GetProcAddress(hmod, "atexit");
    ok(patexit != NULL, "failed to get atexit from msvcrt.dll\n");

    /* Register 3 exit functions, the second one in msvcrt. */
    ok_int(atexit(exitfunc1), 0);
    if (patexit != NULL)
    {
        ok_int(patexit(exitfunc2), 0);
    }
    ok_int(atexit(exitfunc3), 0);
}

START_TEST(atexit)
{
    CHAR Buffer[MAX_PATH];
    PSTR CommandLine;
    int result;
    HANDLE hSemaphore;
    SEMAPHORE_BASIC_INFORMATION SemInfo;
    NTSTATUS Status;

    /* Check recursive call */
    CommandLine = GetCommandLineA();
    if (strstr(CommandLine, "-run") != NULL)
    {
        Test_atexit();
        return;
    }

    /* Create a named semaphore to count atexit callbacks in remote process */
    hSemaphore = CreateSemaphoreA(NULL, 1, 20, "atext_apitest_sempahore");

    /* Run the actual test in a new process */
    sprintf(Buffer, "%s -run", CommandLine);
    result = system(Buffer);
    ok_int(result, 0);

    /* Check the new semaphore state */
    Status = NtQuerySemaphore(hSemaphore, SemaphoreBasicInformation, &SemInfo, sizeof(SemInfo), NULL);
    ok(NT_SUCCESS(Status), "NtQuerySemaphore failed: 0x%lx\n", Status);
    ok_int(SemInfo.CurrentCount, 4);
}
