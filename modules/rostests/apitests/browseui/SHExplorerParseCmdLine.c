/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for SHExplorerParseCmdLine
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>

//#define UNICODE
#include <strsafe.h>
#include <shlobj.h>
#include <browseui_undoc.h>

// Macro parameters are only expanded in the second nesting...
#define _WIDEN(x) L##x
#define WIDEN(x) _WIDEN(x)

#define TEST_FILENAMEA "SHExplorerParseCmdLine.test"
#define TEST_FILENAMEW WIDEN(TEST_FILENAMEA)

#define TEST_PATHA "C:\\SHExplorerParseCmdLine.test"
#define TEST_PATHW WIDEN(TEST_PATHA)

typedef UINT_PTR (WINAPI *SHExplorerParseCmdLine_Type)(PEXPLORER_CMDLINE_PARSE_RESULTS);
static SHExplorerParseCmdLine_Type pSHExplorerParseCmdLine;

#define PIDL_IS_UNTOUCHED -1
#define PIDL_IS_NULL -2
#define PIDL_IS_PATH -3
#define PIDL_IS_EMPTY -4
#define PIDL_PATH_EQUALS_PATH -5

#define InvalidPointer ((PVOID)0x5555555555555555ULL)

static
DWORD ReplaceSubstr(
_Out_ PWCHAR OutputStr,
_In_ DWORD OutputLen,
_In_ PCWSTR InputStr,
_In_ PCWSTR ReplaceStr,
_In_ PCWSTR ReplaceWith)
{
    DWORD result = 0;
    PCWSTR pos;
    PCWSTR pwc;

    if (!OutputLen)
        return result;

    OutputStr[0] = 0;
    pos = InputStr;
    pwc = wcsstr(pos, ReplaceStr);
    while (pwc)
    {
        if (StringCchCatNW(OutputStr, OutputLen, pos, pwc - pos) == STRSAFE_E_INSUFFICIENT_BUFFER)
            break;
        if (StringCchCatW(OutputStr, OutputLen, ReplaceWith) == STRSAFE_E_INSUFFICIENT_BUFFER)
            break;
        result++;
        pos = pwc + wcslen(ReplaceStr);
        pwc = wcsstr(pos, ReplaceStr);
    }
    StringCchCatW(OutputStr, OutputLen, pos);
    return result;
}

static
VOID
TestCommandLine(
_In_ ULONG TestLine,
_In_ UINT_PTR ExpectedRet,
_In_ INT ExpectedCsidl,
_In_ DWORD ExpectedFlags,
_In_ PCWSTR ExpectedFileName,
_In_ PCWSTR PidlPath,
_In_ PCWSTR pclsid,
_Out_opt_ PUINT PWriteEnd)
{
    EXPLORER_CMDLINE_PARSE_RESULTS Info;
    UINT_PTR Ret;
    ULONG i;
    PDWORD InfoWords = (PDWORD) &Info;

    FillMemory(&Info, sizeof(Info), 0x55);
    Info.dwFlags = 0x00000000;
    Ret = pSHExplorerParseCmdLine(&Info);

    // Special case for empty cmdline: Ret is the PIDL for the selected folder.
    if (ExpectedRet == -1)
    {
        ok(Ret == (UINT_PTR)Info.pidlPath, "Line %lu: Ret = %p, expected %p\n", TestLine, (PVOID)Ret, Info.pidlPath);
    }
    else
    {
        ok(Ret == ExpectedRet, "Line %lu: Ret = 0x%Ix, expected 0x%Ix\n", TestLine, Ret, ExpectedRet);
    }

    if (ExpectedFileName == NULL)
    {
        ok(Info.strPath == InvalidPointer, "Line %lu: strPath = %p\n", TestLine, Info.strPath);
    }
    else
    {
        ok(Info.strPath != InvalidPointer, "Line %lu: strPath = InvalidPointer\n", TestLine);
        ok(Info.strPath != NULL, "Line %lu: strPath = NULL\n", TestLine);
        if (Info.strPath != NULL && Info.strPath != InvalidPointer)
        {
            ok(!wcscmp(Info.strPath, ExpectedFileName), "Line %lu: strPath = %ls, expected %ls\n", TestLine, Info.strPath, ExpectedFileName);
            LocalFree(Info.strPath);
        }
    }

    ok(Info.dwFlags == ExpectedFlags, "Line %lu: dwFlags = %08lx, expected %08lx\n", TestLine, Info.dwFlags, ExpectedFlags);

    if (ExpectedCsidl == PIDL_IS_UNTOUCHED)
    {
        ok(Info.pidlPath == InvalidPointer, "Line %lu: pidlPath = %p\n", TestLine, Info.pidlPath);
    }
    else if (ExpectedCsidl == PIDL_IS_NULL)
    {
        ok(Info.pidlPath == NULL, "Line %lu: pidlPath = %p\n", TestLine, Info.pidlPath);
    }
    else
    {
        PIDLIST_ABSOLUTE ExpectedPidl;
        HRESULT hr;

        ok(Info.pidlPath != InvalidPointer, "Line %lu: pidlPath = InvalidPointer\n", TestLine);
        ok(Info.pidlPath != NULL, "Line %lu: pidlPath = NULL\n", TestLine);
        if (Info.pidlPath != NULL && Info.pidlPath != InvalidPointer)
        {
            WCHAR pidlPathName[MAX_PATH] = L"";
            WCHAR pidlPathTest[MAX_PATH] = L"";
            WCHAR rootDir[MAX_PATH] = L"";
            WCHAR curDir[MAX_PATH] = L"";
            WCHAR replaceName[MAX_PATH];

            GetFullPathNameW(L"\\", _countof(rootDir), rootDir, NULL);

            GetCurrentDirectoryW(_countof(curDir), curDir);
            if (wcslen(curDir) != 0 && curDir[wcslen(curDir) - 1] != L'\\')
                StringCchCatW(curDir, _countof(curDir), L"\\");

            if (PidlPath)
            {
                StringCchCopyW(pidlPathTest, _countof(pidlPathTest), PidlPath);

                if (wcsstr(pidlPathTest, L"::ROOT::") != NULL && wcslen(rootDir) > 0)
                {
                    if (ReplaceSubstr(replaceName, _countof(replaceName), pidlPathTest, L"::ROOT::", rootDir))
                        StringCchCopyW(pidlPathTest, _countof(pidlPathTest), replaceName);
                }

                if (wcsstr(pidlPathTest, L"::CURDIR::") != NULL && wcslen(curDir) > 0)
                {
                    if (ReplaceSubstr(replaceName, _countof(replaceName), pidlPathTest, L"::CURDIR::", curDir))
                        StringCchCopyW(pidlPathTest, _countof(pidlPathTest), replaceName);
                }
            }

            SHGetPathFromIDListW(Info.pidlPath, pidlPathName);

            if (ExpectedCsidl == PIDL_PATH_EQUALS_PATH)
            {
                ok(_wcsicmp(pidlPathName, pidlPathTest) == 0, "Line %lu: Path from pidl does not match; pidlPathName=%S\n", TestLine, pidlPathName);
            }
            else if (ExpectedCsidl == PIDL_IS_EMPTY)
            {
                ok(wcslen(pidlPathName) == 0, "Line %lu: Unexpected non-empty path from pidl; pidlPathName=%S\n", TestLine, pidlPathName);
            }
            else
            {
                if (ExpectedCsidl == PIDL_IS_PATH)
                {
                    ExpectedPidl = SHSimpleIDListFromPath(pidlPathTest);
                    hr = ExpectedPidl == NULL ? E_FAIL : S_OK;
                    ok(ExpectedPidl != NULL, "Line %lu: SHSimpleIDListFromPath(%S) failed. pidlPathName=%S\n", TestLine, pidlPathTest, pidlPathName);
                    if (SUCCEEDED(hr))
                    {
                        ok(ILIsEqual(Info.pidlPath, ExpectedPidl), "Line %lu: Unexpected pidlPath value %p; pidlPathName=%S pidlPathTest=%S\n", TestLine, Info.pidlPath, pidlPathName, pidlPathTest);
                        ILFree(ExpectedPidl);
                    }
                }
                else
                {
                    hr = SHGetFolderLocation(NULL, ExpectedCsidl, NULL, 0, &ExpectedPidl);
                    ok(hr == S_OK, "Line %lu: SHGetFolderLocation returned %08lx\n", TestLine, hr);
                    if (SUCCEEDED(hr))
                    {
                        BOOL eq = ILIsEqual(Info.pidlPath, ExpectedPidl);
                        ILFree(ExpectedPidl);

                        ok(eq, "Line %lu: Unexpected pidlPath value %p; pidlPathName=%S CSIDL=%d\n", TestLine, Info.pidlPath, pidlPathName, ExpectedCsidl);
                    }
                }
            }

            ILFree(Info.pidlPath);
        }
    }

    for (i = 0; i < sizeof(Info) / sizeof(DWORD); i++)
    {
        switch (i * sizeof(DWORD))
        {
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, strPath):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, pidlPath):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, dwFlags):
            // TODO: 'case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, nCmdShow):'?
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, pidlRoot):
            // TODO: 'case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, clsid):'?
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, guidInproc) + (0 * sizeof(DWORD)):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, guidInproc) + (1 * sizeof(DWORD)):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, guidInproc) + (2 * sizeof(DWORD)):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, guidInproc) + (3 * sizeof(DWORD)):
#ifdef _WIN64
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, strPath) + sizeof(DWORD):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, pidlPath) + sizeof(DWORD):
            case FIELD_OFFSET(EXPLORER_CMDLINE_PARSE_RESULTS, pidlRoot) + sizeof(DWORD):
#endif
                break;
            default:
                ok(InfoWords[i] == 0x55555555, "Line %lu: Word 0x%02lx has been set to 0x%08lx\n", TestLine, i * sizeof(DWORD), InfoWords[i]);
        }
    }

    {
        LPOLESTR psz;
        BYTE ab[sizeof(CLSID)];

        StringFromCLSID(&Info.clsid, &psz);
        if (pclsid == NULL)
        {
            FillMemory(ab, sizeof(ab), 0x55);
            ok(memcmp(ab, &Info.clsid, sizeof(ab)) == 0, "Line %lu: CLSID was %ls.\n", TestLine, psz);
        }
        else
        {
            ok(lstrcmpiW(psz, pclsid) == 0, "Line %lu: CLSID was %ls.\n", TestLine, psz);
        }
        CoTaskMemFree(psz);
    }

    if (PWriteEnd)
    {
        PBYTE data = (PBYTE)&Info;

        *PWriteEnd = 0;

        for (i = sizeof(Info); i > 0; i--)
        {
            if (data[i - 1] != 0x55)
            {
                *PWriteEnd = i;
                break;
            }
        }
    }
}

START_TEST(SHExplorerParseCmdLine)
{
    static struct
    {
        INT TestLine;
        PCWSTR CommandLine;
        UINT_PTR ExpectedRet;
        INT ExpectedCsidl;
        DWORD ExpectedFlags;
        PCWSTR ExpectedFileName;
        PCWSTR PidlPath;
        PCWSTR pclsid;
    } Tests [] =
    {
        { __LINE__, L"", -1, CSIDL_MYDOCUMENTS, 0x00000009 },
        { __LINE__, L"/e", TRUE, PIDL_IS_UNTOUCHED, 0x00000008 },
        { __LINE__, L"/n", TRUE, PIDL_IS_UNTOUCHED, 0x00004001 },
        { __LINE__, L"/x", TRUE, PIDL_IS_NULL, 0x02000000, L"/x" },
        { __LINE__, L"-e", TRUE, PIDL_IS_NULL, 0x02000000, L"-e" },
        { __LINE__, L"C:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/e,C:\\", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"/select,C:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/e,::{20d04fe0-3aea-1069-a2d8-08002b30309d}", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"::{20d04fe0-3aea-1069-a2d8-08002b30309d}" },
        { __LINE__, L"::{645ff040-5081-101b-9f08-00aa002f954e}", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::{645ff040-5081-101b-9f08-00aa002f954e}" },
        { __LINE__, L"/select,::{450d8fba-ad25-11d0-98a8-0800361b1103}", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}" },
        { __LINE__, L"=", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::{20d04fe0-3aea-1069-a2d8-08002b30309d}" },
//        { __LINE__, L".", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings\\gigaherz\\Desktop" },
//        { __LINE__, L"..", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings\\gigaherz" },
        { __LINE__, L"wrongpath", TRUE, PIDL_IS_NULL, 0x02000000, L"wrongpath"},
        { __LINE__, L"%wrongdir%", TRUE, PIDL_IS_NULL, 0x02000000, L"%wrongdir%"},
        { __LINE__, L"%programfiles#", TRUE, PIDL_IS_NULL, 0x02000000, L"%programfiles#"},
        { __LINE__, L",", TRUE, PIDL_IS_EMPTY, 0x00000200},
        { __LINE__, L"\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::ROOT::" }, // disk letter depends on current directory
        { __LINE__, L"c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"c:", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"c", TRUE, PIDL_IS_NULL, 0x02000000, L"c"},
        { __LINE__, L"c:\\Program Files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\Program Files\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\Program Files/", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\Program Files/"},
        { __LINE__, L"c:/Program Files/", TRUE, PIDL_IS_NULL, 0x02000000, L"c:/Program Files/"},
        { __LINE__, L"fonts", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::fonts" }, // this would not fail if we are in Windows directory
        { __LINE__, L"winsxs", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::winsxs" },
        { __LINE__, L"system32", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"::CURDIR::system32" },
        { __LINE__, L"drivers", TRUE, PIDL_IS_NULL, 0x02000000, L"drivers" }, // this would fail since we are not in system32 directory
        { __LINE__, L"spool", TRUE, PIDL_IS_NULL, 0x02000000, L"spool" },
 //       { __LINE__, L"wbem", TRUE, PIDL_IS_NULL, 0x02000000, L"wbem" },
        { __LINE__, TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000200, NULL, TEST_PATHW },
        { __LINE__, L"\"c:\\\"\"program files\"", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\\"program files"},
        { __LINE__, L"\"c:\\\"program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"\"c:\\ \"program files", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\ program files"},
        { __LINE__, L"\"c:\\\" program files", TRUE, PIDL_IS_NULL, 0x02000000, L"c:\\ program files"},
        { __LINE__, L"\"c:\\\", \"c:\\program files\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\,c:\\program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"\"/root\"", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,c", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,\"\"", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,wrong", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,0", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"/root,\"c:\\\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"/root \"c:\\\"", TRUE, PIDL_IS_NULL, 0x02000000, L"/root c:\\"},
        { __LINE__, L"/root,\"c:\\\"\"program files\"", TRUE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/root,\"c:\\\"program files", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\Program Files" },
        { __LINE__, L"/root,c:\\,c:\\Program Files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root,c:\\,Program Files", TRUE, PIDL_IS_NULL, 0x02000000, L"Program Files"},
        { __LINE__, L"/root,\"c:\\\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"c:\\" },
        { __LINE__, L"c:\\Program Files,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"c:\\" },
//        { __LINE__, L"a:\\,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"c:\\" },
//        { __LINE__, L"a:\\,/root,c", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"A:\\" },
        { __LINE__, L"c:\\,/root,c", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/select", TRUE, PIDL_IS_UNTOUCHED, 0x00000040 },
        { __LINE__, L"/select,", TRUE, CSIDL_DRIVES, 0x00000240 },
        { __LINE__, L"/select,c", TRUE, PIDL_IS_NULL, 0x02000040, L"c"},
        { __LINE__, L"/select,0", TRUE, PIDL_IS_NULL, 0x02000040, L"0"},
        { __LINE__, L"/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"c:\\,/select", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"/select,c:\\Program Files,c:\\Documents and settings", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\Documents and Settings" },
        { __LINE__, L"c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000240, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
//        { __LINE__, L"a:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"z:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x02000240, L"z:\\", TEST_PATHW },
        { __LINE__, L"select,c:\\ ", TRUE, PIDL_IS_PATH, 0x02000200, L"select", L"C:\\" },
        { __LINE__, L"/select c:\\ ", TRUE, PIDL_IS_NULL, 0x02000000, L"/select c:\\"},
//        { __LINE__, L"a:\\,/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"a:\\,/select,c", TRUE, PIDL_IS_NULL, 0x02000040, L"c"},
        { __LINE__, L"c:\\,/select,c", TRUE, PIDL_IS_NULL, 0x02000240, L"c"},
        { __LINE__, L"/e", TRUE, PIDL_IS_UNTOUCHED, 0x00000008 },
        { __LINE__, L"/e,", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,\"", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,\"\"", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e,c:\\", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"c:\\,/e", TRUE, PIDL_IS_PATH, 0x00000208, NULL, L"C:\\" },
        { __LINE__, L"/e,c", TRUE, PIDL_IS_NULL, 0x02000008, L"c"},
        { __LINE__, L"/root,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000040, TEST_FILENAMEW },
        { __LINE__, L"/select," TEST_FILENAMEW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x02000040, TEST_FILENAMEW, L"c:\\" },
        { __LINE__, L"/root,c:\\,/select," TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000240, NULL, TEST_PATHW },
        { __LINE__, L"/select," TEST_PATHW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000240, NULL, L"c:\\" },
        { __LINE__, L"/e,/select," TEST_FILENAMEW L",/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x02000048, TEST_FILENAMEW, L"c:\\" },
        { __LINE__, L"/e,/root,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000048, TEST_FILENAMEW },
        { __LINE__, L"/e,/root,c:\\", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000008, NULL, L"c:\\" },
        { __LINE__, L"/e,c:\\,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/e,/select," TEST_FILENAMEW, TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"c:\\,/select," TEST_FILENAMEW L",/e", TRUE, PIDL_IS_NULL, 0x02000248, TEST_FILENAMEW },
        { __LINE__, L"http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000000, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/e,http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000008, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/root,c:\\,http:\\\\www.reactos.org", TRUE, PIDL_IS_NULL, 0x02000000, L"http:\\\\www.reactos.org"},
        { __LINE__, L"/separate ", TRUE, PIDL_IS_UNTOUCHED, 0x00020000 },
        { __LINE__, L"/separate,c:\\ program files", TRUE, PIDL_IS_NULL, 0x02020000, L"c:\\ program files"},
        { __LINE__, L"/separate,           c:\\program files", TRUE, PIDL_IS_PATH, 0x00020200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/separate,           c:\\program files           ,/e", TRUE, PIDL_IS_PATH, 0x00020208, NULL, L"C:\\Program Files" },
        { __LINE__, L"/separate,           c:\\program files           ,\\e", TRUE, PIDL_IS_NULL, 0x02020200, L"\\e"},
        { __LINE__, L"c:\\Documents and settings,/separate,/n,/e,/root,c:\\,/select,c:\\Program files,", TRUE, CSIDL_DRIVES, 0x00024249 },
        { __LINE__, L"c:\\Documents and settings,/separate,/n,/e,/root,{450D8FBA-AD25-11D0-98A8-0800361B1103, 0},test,/select,c:\\Program files,", TRUE, CSIDL_DRIVES, 0x02024249, L"test" },
        { __LINE__, L"c:\\Documents and settings,/inproc,/noui,/s,/separate,/n,/e,/root,{450D8FBA-AD25-11D0-98A8-0800361B1103, 0},test,/select,c:\\Program files,", FALSE, PIDL_PATH_EQUALS_PATH, 0x00000200, NULL, L"C:\\Documents and Settings" },
        { __LINE__, L"=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"=" TEST_PATHW, TRUE, PIDL_IS_PATH, 0x00000200, NULL, TEST_PATHW },
        { __LINE__, L"/root,=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/root=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/root=c:\\Program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root=\"c:\\Program files\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"/root=\"\"c:\\Program files\"\"", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"c:\\=/root=\"c:\\Program files\"", TRUE, PIDL_PATH_EQUALS_PATH, 0x00000000, NULL, L"C:\\Program Files" },
        { __LINE__, L"/select=c:\\", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\" },
        { __LINE__, L"/select=c:\\Program files", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\Program Files" },
        { __LINE__, L"=,/select,c:\\", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\" },
        { __LINE__, L"/select,c:\\,=", TRUE, CSIDL_DRIVES, 0x00000240 },
        { __LINE__, L"c:\\=/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000200, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"=/select=c:\\windows\\system32", TRUE, PIDL_IS_PATH, 0x00000240, NULL, L"C:\\WINDOWS\\system32" },
        { __LINE__, L"/e,=", TRUE, CSIDL_DRIVES, 0x00000208 },
        { __LINE__, L"/e=", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/e=\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"/e=\"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"=\"=\"", TRUE, PIDL_IS_NULL, 0x02000000, L"="},
        { __LINE__, L"==\"=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"="},
        { __LINE__, L"===\"=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"="},
        { __LINE__, L"=\"=\"", TRUE, PIDL_IS_NULL, 0x02000000, L"="},
        { __LINE__, L"==\"==\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=="},
        { __LINE__, L"===\"===\"", TRUE, PIDL_IS_NULL, 0x02000200, L"==="},
        { __LINE__, L"=\"=a\"", TRUE, PIDL_IS_NULL, 0x02000000, L"=a"},
        { __LINE__, L"==\"=a=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=a="},
        { __LINE__, L"===\"=a=a=\"", TRUE, PIDL_IS_NULL, 0x02000200, L"=a=a="},
        { __LINE__, L"=\"=a\"==", TRUE, CSIDL_DRIVES, 0x02000200, L"=a" },
        { __LINE__, L"==\"=a=\"=", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=" },
        { __LINE__, L"===\"=a=a=\"===", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=a=" },
        { __LINE__, L"=,=,=,\"=a=a=\",=,=,=", TRUE, CSIDL_DRIVES, 0x02000200, L"=a=a=" },
        { __LINE__, L"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"\"\"", TRUE, CSIDL_DRIVES, 0x00000200 },
        { __LINE__, L"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\""},
        { __LINE__, L"\"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\""},
        { __LINE__, L"\"\"\"\"\"", TRUE, PIDL_IS_NULL, 0x02000000, L"\"\""},
        { __LINE__, L"/s", TRUE, PIDL_IS_UNTOUCHED, 0x00000002 },
        { __LINE__, L"/noui", TRUE, PIDL_IS_UNTOUCHED, 0x00001000 },
        { __LINE__, L"/idlist", TRUE, PIDL_IS_UNTOUCHED, 0x00000000},
        { __LINE__, L"-embedding", TRUE, PIDL_IS_UNTOUCHED, 0x00000080 },
        { __LINE__, L"/inproc", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,1", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,a", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,c:\\", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,\"c:\\\"", FALSE, PIDL_IS_UNTOUCHED, 0x00000000 },
        { __LINE__, L"/inproc,{20d04fe0-3aea-1069-a2d8-08002b30309d}", TRUE, PIDL_IS_UNTOUCHED, 0x00000400 },
        { __LINE__, L"shell:::{450D8FBA-AD25-11D0-98A8-0800361B1103}", TRUE, CSIDL_MYDOCUMENTS, 0x00000200 },
        { __LINE__, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}", TRUE, CSIDL_MYDOCUMENTS, 0x00000200 },
    };
    const int TestCount = sizeof(Tests) / sizeof(Tests[0]);
    PWSTR CommandLine;
    WCHAR OriginalCommandLine[1024];
    int i;
    UINT maxWrite = 0;
    FILE * ff;
    WCHAR winDir[MAX_PATH];

    HMODULE browseui = LoadLibraryA("browseui.dll");
    pSHExplorerParseCmdLine = (SHExplorerParseCmdLine_Type)GetProcAddress(browseui, MAKEINTRESOURCEA(107));
    if (!pSHExplorerParseCmdLine)
    {
        skip("SHExplorerParseCmdLine not found, as on NT6+\n");
        return;
    }

    CommandLine = GetCommandLineW();
    StringCbCopyW(OriginalCommandLine, sizeof(OriginalCommandLine), CommandLine);

    ff = fopen(TEST_PATHA, "wb");
    fclose(ff);

    GetWindowsDirectoryW(winDir, _countof(winDir));
    SetCurrentDirectoryW(winDir);

    for (i = 0; i < TestCount; i++)
    {
        UINT cWrite;

        wcscpy(CommandLine, L"browseui_apitest.exe ");
        wcscat(CommandLine, Tests[i].CommandLine);
        trace("Command line (%d): %ls\n", Tests[i].TestLine, CommandLine);
        TestCommandLine(Tests[i].TestLine,
                        Tests[i].ExpectedRet,
                        Tests[i].ExpectedCsidl,
                        Tests[i].ExpectedFlags,
                        Tests[i].ExpectedFileName,
                        Tests[i].PidlPath,
                        Tests[i].pclsid,
                        &cWrite);

        if (cWrite > maxWrite)
            maxWrite = cWrite;
    }

    trace("Writes reached the byte right before 0x%08x\n", maxWrite);

    wcscpy(CommandLine, OriginalCommandLine);

    remove(TEST_PATHA);
}
