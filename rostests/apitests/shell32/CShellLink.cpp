/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CShellLink
 * PROGRAMMER:      Andreas Maier
 */

#include "shelltest.h"
#include <atlbase.h>
#include <atlcom.h>
#include <strsafe.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>
#include <shellutils.h>

typedef
struct _TestShellLinkDef
{
    const WCHAR* pathIn;
    HRESULT hrSetPath;
    /* Test 1 - hrGetPathX = IShellLink::GetPath(pathOutX, ... , flagsX); */
    const WCHAR* pathOut1;
    DWORD flags1;
    HRESULT hrGetPath1;
    bool expandPathOut1;
    /* Test 2 */
    const WCHAR* pathOut2;
    DWORD flags2;
    HRESULT hrGetPath2;
    bool expandPathOut2;
} TestShellLinkDef;

/* Test IShellLink::SetPath with environment-variables, existing, non-existing, ...*/
static struct _TestShellLinkDef linkTestList[] =
{
    {
        L"%comspec%",                                 S_OK,
        L"%comspec%",             SLGP_SHORTPATH,     S_OK, TRUE,
        L"%comspec%",             SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"%anyvar%",                                  E_INVALIDARG,
        L"",                      SLGP_SHORTPATH,     ERROR_INVALID_FUNCTION, FALSE,
        L"",                      SLGP_RAWPATH,       ERROR_INVALID_FUNCTION, FALSE
    },
    {
        L"%anyvar%%comspec%",                         S_OK,
        L"c:\\%anyvar%%comspec%", SLGP_SHORTPATH,     S_OK, TRUE,
        L"%anyvar%%comspec%",     SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"%temp%",                                    S_OK,
        L"%temp%",                SLGP_SHORTPATH,     S_OK, TRUE,
        L"%temp%",                SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"%shell%",                                   S_OK,
        L"%systemroot%\\system32\\%shell%",    SLGP_SHORTPATH,     S_OK, TRUE,
        L"%shell%",               SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"u:\\anypath\\%anyvar%",                     S_OK,
        L"u:\\anypath\\%anyvar%", SLGP_SHORTPATH,     S_OK, TRUE,
        L"u:\\anypath\\%anyvar%", SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"c:\\temp",                                  S_OK,
        L"c:\\temp",              SLGP_SHORTPATH,     S_OK, FALSE,
        L"c:\\temp",              SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"cmd.exe",                                  S_OK,
        L"%comspec%",             SLGP_SHORTPATH,    S_OK,  TRUE,
        L"%comspec%",             SLGP_RAWPATH,      S_OK,  TRUE
    },
    {
        L"c:\\non-existent-path\\non-existent-file", S_OK,
        L"c:\\non-existent-path\\non-existent-file", SLGP_SHORTPATH, S_OK, FALSE,
        L"c:\\non-existent-path\\non-existent-file", SLGP_RAWPATH,   S_OK, FALSE
    },
    {
        L"non-existent-file",                        E_INVALIDARG,
        L"",                      SLGP_SHORTPATH,    ERROR_INVALID_FUNCTION, FALSE,
        L"",                      SLGP_RAWPATH,      ERROR_INVALID_FUNCTION, FALSE
    },
};

static
VOID
test_checklinkpath(UINT i, TestShellLinkDef* testDef)
{
static WCHAR evVar[MAX_PATH];

    HRESULT hr, expectedHr;
    WCHAR wPathOut[MAX_PATH];
    bool expandPathOut;
    const WCHAR* expectedPathOut;
    IShellLinkW *psl;
    UINT i1;
    DWORD flags;

    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLinkW, &psl));
    ok(hr == S_OK, "CoCreateInstance, hr = %lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellLink\n");
        return;
    }

    hr = psl->SetPath(testDef->pathIn);
    ok(hr == testDef->hrSetPath, "IShellLink::SetPath(%lu), got hr = %lx, expected %lx\n", i, hr, testDef->hrSetPath);

    expectedPathOut = NULL;
    for (i1 = 0; i1 <= 1; i1++ )
    {
        if (i1 == 1)
        {
            flags = testDef->flags1;
            expandPathOut = testDef->expandPathOut1;
            expectedPathOut = testDef->pathOut1;
            expectedHr = testDef->hrGetPath1;
        }
        else
        {
            flags = testDef->flags2;
            expandPathOut = testDef->expandPathOut2;
            expectedPathOut = testDef->pathOut2;
            expectedHr = testDef->hrGetPath2;
        }

        /* Patch some variables */
        if (expandPathOut == TRUE)
        {
            ExpandEnvironmentStringsW(expectedPathOut, evVar, _countof(evVar));
            DPRINT("** %S **\n",evVar);
            expectedPathOut = evVar;
        }

        hr = psl->GetPath(wPathOut, _countof(wPathOut), NULL, flags);
        ok(hr == expectedHr,
           "IShellLink::GetPath(%lu), flags %lx, got hr = %lx, expected %lx\n",
            i, flags, hr, expectedHr);
        ok(wcsicmp(wPathOut, expectedPathOut) == 0,
           "IShellLink::GetPath(%lu), flags %lx, in %S, got %S, expected %S\n",
           i, flags, testDef->pathIn, wPathOut, expectedPathOut);
    }

    psl->Release();
}

static
VOID
TestDescription(void)
{
    HRESULT hr;
    IShellLinkW *psl;
    WCHAR buffer[64];
    PCWSTR testDescription = L"This is a test description";

    /* Test SetDescription */
    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLinkW, &psl));
    ok(hr == S_OK, "CoCreateInstance, hr = %lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellLink\n");
        return;
    }

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = %lx\n", hr);
    ok(buffer[0] == 0, "buffer[0] = %x\n", buffer[0]);
    ok(buffer[1] == 0x5555, "buffer[1] = %x\n", buffer[1]);

    hr = psl->SetDescription(testDescription);
    ok(hr == S_OK, "IShellLink::SetDescription returned hr = %lx\n", hr);

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = %lx\n", hr);
    ok(buffer[wcslen(testDescription)] == 0, "buffer[n] = %x\n", buffer[wcslen(testDescription)]);
    ok(buffer[wcslen(testDescription) + 1] == 0x5555, "buffer[n+1] = %x\n", buffer[wcslen(testDescription) + 1]);
    ok(!wcscmp(buffer, testDescription), "buffer = '%ls'\n", buffer);

    hr = psl->SetDescription(NULL);
    ok(hr == S_OK, "IShellLink::SetDescription returned hr = %lx\n", hr);

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = %lx\n", hr);
    ok(buffer[0] == 0, "buffer[0] = %x\n", buffer[0]);
    ok(buffer[1] == 0x5555, "buffer[1] = %x\n", buffer[1]);

    psl->Release();
}

static
VOID
TestShellLink(void)
{
    UINT i;

    /* Needed for test */
    SetEnvironmentVariableW(L"shell", L"cmd.exe");

    for (i = 0; i < ARRAYSIZE(linkTestList); ++i)
    {
        DPRINT("IShellLink-Test(%lu): %S\n", i, linkTestList[i].pathIn);
        test_checklinkpath(i, &linkTestList[i]);
    }

    SetEnvironmentVariableW(L"shell",NULL);
}

START_TEST(CShellLink)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    TestShellLink();
    TestDescription();
}
