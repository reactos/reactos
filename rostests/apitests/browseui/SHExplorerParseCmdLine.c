/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SHExplorerParseCmdLine
 * PROGRAMMER:      Thomas Faber
 */

#define UNICODE
#include <wine/test.h>
#include <strsafe.h>
#include <shlobj.h>

#define UNKNOWN_SIZE 0x100

typedef struct _EXPLORER_INFO
{
    PWSTR FileName;
    PIDLIST_ABSOLUTE pidl;
    DWORD dwFlags; /* TODO: make some constants */
    ULONG Unknown[UNKNOWN_SIZE]; /* TODO: find out more */
} EXPLORER_INFO, *PEXPLORER_INFO;

PVOID
WINAPI
SHExplorerParseCmdLine(
    _Out_ PEXPLORER_INFO Info);

#define PIDL_IS_UNTOUCHED -1
#define PIDL_IS_NULL -2
#define PIDL_IS_PATH -3

#define InvalidPointer ((PVOID)0x5555555555555555ULL)

static
VOID
TestCommandLine(
    _In_ INT ExpectedRet,
    _In_ INT ExpectedCsidl,
    _In_ DWORD ExpectedFlags,
    _In_ PCWSTR ExpectedFileName,
    _In_ PCWSTR PidlPath)
{
    EXPLORER_INFO Info;
    PVOID Ret;
    ULONG i;

    FillMemory(&Info, sizeof(Info), 0x55);
    Info.dwFlags = 0x00000000;
    Ret = SHExplorerParseCmdLine(&Info);

    if (ExpectedRet == -1)
        ok(Ret == Info.pidl, "Ret = %p, expected %p\n", Ret, Info.pidl);
    else
        ok(Ret == (PVOID)ExpectedRet, "Ret = %p, expected %p\n", Ret, (PVOID)ExpectedRet);

    if (ExpectedFileName == NULL)
        ok(Info.FileName == InvalidPointer, "FileName = %p\n", Info.FileName);
    else
    {
        ok(Info.FileName != NULL && Info.FileName != InvalidPointer, "FileName = %p\n", Info.FileName);
        if (Info.FileName != NULL && Info.FileName != InvalidPointer)
        {
            ok(!wcscmp(Info.FileName, ExpectedFileName), "FileName = %ls, expected %ls\n", Info.FileName, ExpectedFileName);
            LocalFree(Info.FileName);
        }
    }

    if (ExpectedCsidl == PIDL_IS_UNTOUCHED)
        ok(Info.pidl == InvalidPointer, "pidl = %p\n", Info.pidl);
    else if (ExpectedCsidl == PIDL_IS_NULL)
        ok(Info.pidl == NULL, "pidl = %p\n", Info.pidl);
    else
    {
        PIDLIST_ABSOLUTE ExpectedPidl;
        HRESULT hr;

        ok(Info.pidl != NULL, "pidl = %p\n", Info.pidl);
        if (Info.pidl != NULL && Info.pidl != InvalidPointer)
        {
            if (ExpectedCsidl == PIDL_IS_PATH)
            {
                ExpectedPidl = SHSimpleIDListFromPath(PidlPath);
                hr = ExpectedPidl == NULL ? E_FAIL : S_OK;
                ok(ExpectedPidl != NULL, "SHSimpleIDListFromPath failed\n");
            }
            else
            {
                hr = SHGetFolderLocation(NULL, ExpectedCsidl, NULL, 0, &ExpectedPidl);
                ok(hr == S_OK, "SHGetFolderLocation returned %08lx\n", hr);
            }
            if (SUCCEEDED(hr))
            {
                ok(ILIsEqual(Info.pidl, ExpectedPidl), "Unexpected pidl value\n");
                ILFree(ExpectedPidl);
            }
            ILFree(Info.pidl);
        }
    }

    ok(Info.dwFlags == ExpectedFlags, "dwFlags = %08lx, expected %08lx\n", Info.dwFlags, ExpectedFlags);
    for (i = 0; i < sizeof(Info.Unknown) / sizeof(Info.Unknown[0]); i++)
        ok(Info.Unknown[i] == 0x55555555, "Unknown[%lu] = %08lx\n", i, Info.Unknown[i]);
}

START_TEST(SHExplorerParseCmdLine)
{
    struct
    {
        PCWSTR CommandLine;
        INT ExpectedRet;
        INT ExpectedCsidl;
        DWORD ExpectedFlags;
        PCWSTR ExpectedFileName;
        PCWSTR PidlPath;
    } Tests[] =
    {
        { L"",            -1, CSIDL_MYDOCUMENTS, 0x00000009 },
        { L"/e",        TRUE, PIDL_IS_UNTOUCHED, 0x00000008 },
        { L"/n",        TRUE, PIDL_IS_UNTOUCHED, 0x00004001 },
        { L"/x",        TRUE,      PIDL_IS_NULL, 0x02000000, L"/x" },
        { L"-e",        TRUE,      PIDL_IS_NULL, 0x02000000, L"-e" },
        { L"C:\\",      TRUE,      PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { L"/e,C:\\",   TRUE,      PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        /* TODO: needs a lot more testcases */
    };
    const int TestCount = sizeof(Tests) / sizeof(Tests[0]);
    PWSTR CommandLine;
    WCHAR OriginalCommandLine[1024];
    int i;

    CommandLine = GetCommandLine();
    (VOID)StringCbCopy(OriginalCommandLine, sizeof(OriginalCommandLine), CommandLine);

    for (i = 0; i < TestCount; i++)
    {
        wcscpy(CommandLine, L"browseui_apitest.exe ");
        wcscat(CommandLine, Tests[i].CommandLine);
        trace("Command line: %ls\n", CommandLine);
        TestCommandLine(Tests[i].ExpectedRet,
                        Tests[i].ExpectedCsidl,
                        Tests[i].ExpectedFlags,
                        Tests[i].ExpectedFileName,
                        Tests[i].PidlPath);
    }

    wcscpy(CommandLine, OriginalCommandLine);
}
