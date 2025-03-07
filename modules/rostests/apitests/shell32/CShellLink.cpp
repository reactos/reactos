/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CShellLink
 * PROGRAMMER:      Andreas Maier
 */

#include "shelltest.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>
#include <shellutils.h>

/* Test IShellLink::SetPath with environment-variables, existing, non-existing, ...*/
typedef struct
{
    PCWSTR pathIn;
    HRESULT hrSetPath;

    /* Test 1 - hrGetPathX = IShellLink::GetPath(pathOutX, ... , flagsX); */
    PCWSTR pathOut1;
    DWORD flags1;
    HRESULT hrGetPath1;
    BOOL expandPathOut1;

    /* Test 2 */
    PCWSTR pathOut2;
    DWORD flags2;
    HRESULT hrGetPath2;
    BOOL expandPathOut2;
} TEST_SHELL_LINK_DEF;

static TEST_SHELL_LINK_DEF linkTestList[] =
{
    {
        L"%comspec%",                                 S_OK,
        L"%comspec%",             SLGP_SHORTPATH,     S_OK, TRUE,
        L"%comspec%",             SLGP_RAWPATH,       S_OK, FALSE
    },
    {
        L"%anyvar%",                                  E_INVALIDARG,
        L"",                      SLGP_SHORTPATH,     S_FALSE, FALSE,
        L"",                      SLGP_RAWPATH,       S_FALSE, FALSE
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
        L"%systemroot%\\non-existent-file", S_OK,
        L"%systemroot%\\non-existent-file", SLGP_SHORTPATH, S_OK, TRUE,
        L"%systemroot%\\non-existent-file", SLGP_RAWPATH,   S_OK, FALSE
    },
    {
        L"c:\\non-existent-path\\non-existent-file", S_OK,
        L"c:\\non-existent-path\\non-existent-file", SLGP_SHORTPATH, S_OK, FALSE,
        L"c:\\non-existent-path\\non-existent-file", SLGP_RAWPATH,   S_OK, FALSE
    },
    {
        L"non-existent-file",                        E_INVALIDARG,
        L"",                      SLGP_SHORTPATH,    S_FALSE, FALSE,
        L"",                      SLGP_RAWPATH,      S_FALSE, FALSE
    },
};

static
VOID
test_checklinkpath(UINT i, TEST_SHELL_LINK_DEF* testDef)
{
static WCHAR evVar[MAX_PATH];

    HRESULT hr, expectedHr;
    WCHAR wPathOut[MAX_PATH];
    BOOL expandPathOut;
    PCWSTR expectedPathOut;
    CComPtr<IShellLinkW> psl;
    UINT i1;
    DWORD flags;

    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLinkW, &psl));
    ok(hr == S_OK, "CoCreateInstance, hr = 0x%lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellLink\n");
        return;
    }

    hr = psl->SetPath(testDef->pathIn);
    ok(hr == testDef->hrSetPath, "IShellLink::SetPath(%d), got hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrSetPath);

    expectedPathOut = NULL;
    for (i1 = 0; i1 <= 1; i1++)
    {
        if (i1 == 0) /* Usually SLGP_SHORTPATH */
        {
            flags = testDef->flags1;
            expandPathOut = testDef->expandPathOut1;
            expectedPathOut = testDef->pathOut1;
            expectedHr = testDef->hrGetPath1;
        }
        else // if (i1 == 1) /* Usually SLGP_RAWPATH */
        {
            flags = testDef->flags2;
            expandPathOut = testDef->expandPathOut2;
            expectedPathOut = testDef->pathOut2;
            expectedHr = testDef->hrGetPath2;
        }

        /* Patch some variables */
        if (expandPathOut)
        {
            ExpandEnvironmentStringsW(expectedPathOut, evVar, _countof(evVar));
            DPRINT("** %S **\n",evVar);
            expectedPathOut = evVar;
        }

        hr = psl->GetPath(wPathOut, _countof(wPathOut), NULL, flags);
        ok(hr == expectedHr,
           "IShellLink::GetPath(%d), flags 0x%lx, got hr = 0x%lx, expected 0x%lx\n",
            i, flags, hr, expectedHr);
        ok(_wcsicmp(wPathOut, expectedPathOut) == 0,
           "IShellLink::GetPath(%d), flags 0x%lx, in %S, got %S, expected %S\n",
           i, flags, testDef->pathIn, wPathOut, expectedPathOut);
    }
}

static
VOID
TestShellLink(void)
{
    UINT i;

    /* Needed for test */
    SetEnvironmentVariableW(L"shell", L"cmd.exe");

    for (i = 0; i < _countof(linkTestList); ++i)
    {
        DPRINT("IShellLink-Test(%d): %S\n", i, linkTestList[i].pathIn);
        test_checklinkpath(i, &linkTestList[i]);
    }

    SetEnvironmentVariableW(L"shell",NULL);
}

static
VOID
TestDescription(void)
{
    HRESULT hr;
    CComPtr<IShellLinkW> psl;
    WCHAR buffer[64];
    PCWSTR testDescription = L"This is a test description";

    /* Test SetDescription */
    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLinkW, &psl));
    ok(hr == S_OK, "CoCreateInstance, hr = 0x%lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellLink\n");
        return;
    }

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = 0x%lx\n", hr);
    ok(buffer[0] == 0, "buffer[0] = %x\n", buffer[0]);
    ok(buffer[1] == 0x5555, "buffer[1] = %x\n", buffer[1]);

    hr = psl->SetDescription(testDescription);
    ok(hr == S_OK, "IShellLink::SetDescription returned hr = 0x%lx\n", hr);

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = 0x%lx\n", hr);
    ok(buffer[wcslen(testDescription)] == 0, "buffer[n] = %x\n", buffer[wcslen(testDescription)]);
    ok(buffer[wcslen(testDescription) + 1] == 0x5555, "buffer[n+1] = %x\n", buffer[wcslen(testDescription) + 1]);
    ok(!wcscmp(buffer, testDescription), "buffer = '%ls'\n", buffer);

    hr = psl->SetDescription(NULL);
    ok(hr == S_OK, "IShellLink::SetDescription returned hr = 0x%lx\n", hr);

    memset(buffer, 0x55, sizeof(buffer));
    hr = psl->GetDescription(buffer, RTL_NUMBER_OF(buffer));
    ok(hr == S_OK, "IShellLink::GetDescription returned hr = 0x%lx\n", hr);
    ok(buffer[0] == 0, "buffer[0] = %x\n", buffer[0]);
    ok(buffer[1] == 0x5555, "buffer[1] = %x\n", buffer[1]);
}


/* Test IShellLink::Get/SetIconLocation and IExtractIcon::GetIconLocation */
typedef struct
{
    PCWSTR FilePath;

    /* Expected results */
    HRESULT hrDefIcon;  // Return value for GIL_DEFAULTICON
    HRESULT hrForShrt;  // Return value for GIL_FORSHORTCUT
    /* Return values for GIL_FORSHELL */
    HRESULT hrForShell;
    PCWSTR  IconPath;
    UINT    Flags;
} TEST_SHELL_ICON;

static TEST_SHELL_ICON ShIconTests[] =
{
    /* Executable with icons */
    {L"%SystemRoot%\\system32\\cmd.exe", S_FALSE, E_INVALIDARG,
     S_OK, L"%SystemRoot%\\system32\\cmd.exe", GIL_NOTFILENAME | GIL_PERINSTANCE},

    /* Executable without icon */
    {L"%SystemRoot%\\system32\\autochk.exe", S_FALSE, E_INVALIDARG,
     S_OK, L"%SystemRoot%\\system32\\autochk.exe", GIL_NOTFILENAME | GIL_PERINSTANCE},

    /* Existing file */
    {L"%SystemRoot%\\system32\\shell32.dll", S_FALSE, E_INVALIDARG,
     S_OK, L"*", GIL_NOTFILENAME | GIL_PERCLASS},

    /* Non-existing files */
    {L"%SystemRoot%\\non-existent-file.sdf", S_FALSE, E_INVALIDARG,
     S_OK, L"*", GIL_NOTFILENAME | GIL_PERCLASS},
    {L"c:\\non-existent-path\\non-existent-file.sdf", S_FALSE, E_INVALIDARG,
     S_OK, L"*", GIL_NOTFILENAME | GIL_PERCLASS},
};

static
VOID
test_iconlocation(UINT i, TEST_SHELL_ICON* testDef)
{
    HRESULT hr;
    CComPtr<IShellLinkW> psl;
    CComPtr<IExtractIconW> pei;
    INT iIcon;
    UINT wFlags;
    PCWSTR pszExplorer = L"%SystemRoot%\\explorer.exe";
    WCHAR szPath[MAX_PATH];
    WCHAR szPath2[MAX_PATH];

    hr = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellLinkW, &psl));
    ok(hr == S_OK, "CoCreateInstance, hr = 0x%lx\n", hr);
    if (FAILED(hr))
    {
        skip("Could not instantiate CShellLink\n");
        return;
    }

    /* Set the path to a file */
    ExpandEnvironmentStringsW(testDef->FilePath, szPath, _countof(szPath));
    hr = psl->SetPath(szPath);
    ok(hr == S_OK, "IShellLink::SetPath failed, hr = 0x%lx\n", hr);

    /*
     * This test shows that this does not imply that the icon is automatically
     * set and be retrieved naively by a call to IShellLink::GetIconLocation.
     */
    iIcon = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = psl->GetIconLocation(szPath, _countof(szPath), &iIcon);
    ok(hr == S_OK, "IShellLink::GetIconLocation(%d) failed, hr = 0x%lx\n", i, hr);
    ok(*szPath == L'\0', "IShellLink::GetIconLocation(%d) returned '%S'\n", i, szPath);
    ok(iIcon == 0, "IShellLink::GetIconLocation(%d) returned %d, expected %d\n", i, iIcon, 0);

    /* Try to grab the IExtractIconW interface */
    hr = psl->QueryInterface(IID_PPV_ARG(IExtractIconW, &pei));
    ok(hr == S_OK, "IShellLink::QueryInterface(IExtractIconW)(%d) failed, hr = 0x%lx\n", i, hr);
    if (!pei)
    {
        win_skip("No IExtractIconW interface\n");
        return;
    }

    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_DEFAULTICON, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrDefIcon, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrDefIcon);
    ok(*szPath == L'\0', "IExtractIcon::GetIconLocation(%d) returned '%S'\n", i, szPath);
    // ok(iIcon == 0, "IExtractIcon::GetIconLocation(%d) returned %d\n", i, iIcon);

    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_FORSHORTCUT, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrForShrt, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrForShrt);
    // Here, both szPath and iIcon are untouched...

    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_FORSHELL, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrForShell, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrForShell);
    ok(wFlags == testDef->Flags, "IExtractIcon::GetIconLocation(%d) returned wFlags = 0x%x, expected 0x%x\n", i, wFlags, testDef->Flags);
    /*
     * Actually, even if wFlags specifies GIL_NOTFILENAME, a correct file name is returned
     * for executables only (at least...), otherwise we can get an asterix '*'.
     */
    ExpandEnvironmentStringsW(testDef->IconPath, szPath2, _countof(szPath2));
    ok(_wcsicmp(szPath2, szPath) == 0, "IExtractIcon::GetIconLocation(%d) returned '%S', expected '%S'\n", i, szPath, szPath2);

    // ok(*szPath == L'\0', "IExtractIcon::GetIconLocation returned '%S'\n", szPath);
    // ok(iIcon == 0, "IExtractIcon::GetIconLocation returned %d\n", iIcon);
    // ok(FALSE, "hr = 0x%lx, szPath = '%S', iIcon = %d, wFlags = %d\n", hr, szPath, iIcon, wFlags);


    /*
     * Now we test what happens when we explicitly set an icon to the shortcut.
     * Note that actually, SetIconLocation() does not verify whether the file
     * really exists.
     */
    hr = psl->SetIconLocation(pszExplorer, 1);
    ok(hr == S_OK, "IShellLink::SetIconLocation(%d) failed, hr = 0x%lx\n", i, hr);

    /*
     * First, we call IShellLink::GetIconLocation. We retrieve
     * exactly what we specified with SetIconLocation.
     */
    iIcon = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = psl->GetIconLocation(szPath, _countof(szPath), &iIcon);
    ok(hr == S_OK, "IShellLink::GetIconLocation(%d) failed, hr = 0x%lx\n", i, hr);
    ok(wcscmp(szPath, pszExplorer) == 0, "IShellLink::GetIconLocation(%d) returned '%S', expected '%S'\n", i, szPath, pszExplorer);
    ok(iIcon == 1, "IShellLink::GetIconLocation(%d) returned %d, expected %d\n", i, iIcon, 1);

    /*
     * Now we test what happens with IExtractIcon::GetIconLocation.
     * We see that it retrieves the icon of the shortcut's underlying file.
     */
    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_DEFAULTICON, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrDefIcon, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrDefIcon);
    ok(*szPath == L'\0', "IExtractIcon::GetIconLocation(%d) returned '%S'\n", i, szPath);
    // ok(iIcon == 0, "IExtractIcon::GetIconLocation(%d) returned %d\n", i, iIcon);

    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_FORSHORTCUT, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrForShrt, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrForShrt);
    // Here, both szPath and iIcon are untouched...

    iIcon = wFlags = 0xdeadbeef;
    wcscpy(szPath, L"garbage");
    hr = pei->GetIconLocation(GIL_FORSHELL, szPath, _countof(szPath), &iIcon, &wFlags);
    ok(hr == testDef->hrForShell, "IExtractIcon::GetIconLocation(%d) returned hr = 0x%lx, expected 0x%lx\n", i, hr, testDef->hrForShell);
    ok(wFlags == testDef->Flags, "IExtractIcon::GetIconLocation(%d) returned wFlags = 0x%x, expected 0x%x\n", i, wFlags, testDef->Flags);
    /*
     * Actually, even if wFlags specifies GIL_NOTFILENAME, a correct file name is returned
     * for executables only (at least...), otherwise we can get an asterix '*'.
     */
    ExpandEnvironmentStringsW(testDef->IconPath, szPath2, _countof(szPath2));
    ok(_wcsicmp(szPath2, szPath) == 0, "IExtractIcon::GetIconLocation(%d) returned '%S', expected '%S'\n", i, szPath, szPath2);

    // ok(*szPath == L'\0', "IExtractIcon::GetIconLocation returned '%S'\n", szPath);
    // ok(iIcon == 0, "IExtractIcon::GetIconLocation returned %d\n", iIcon);
    // ok(FALSE, "hr = 0x%lx, szPath = '%S', iIcon = %d, wFlags = %d\n", hr, szPath, iIcon, wFlags);
}

static
VOID
TestIconLocation(void)
{
    UINT i;

    for (i = 0; i < _countof(ShIconTests); ++i)
    {
        test_iconlocation(i, &ShIconTests[i]);
    }
}


START_TEST(CShellLink)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    TestShellLink();
    TestDescription();
    TestIconLocation();

    CoUninitialize();
}
