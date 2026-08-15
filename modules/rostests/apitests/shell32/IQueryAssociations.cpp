/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for IQueryAssociations
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include "shelltest.h"
#include <shellutils.h>

#define UNIQUEEXT L".RosTestExtShell32"
#define UNIQUEPROGID L"88D6FB7F96C34598A94D7C1CBB37E70B"
#define UNIQUEPROGID2 L"990B516360E4451EA6BE2156CB3819E4"
#define UNIQUEGUID L"{FF9A10E8-E0E8-47B9-A2BC-36C5E435E5E1}"

#define HKCR HKEY_CLASSES_ROOT
#define HKCU HKEY_CURRENT_USER
#define E_ANY ((0xFF << 24) | 42)
#define S_ANY (~UINT(0) >> 24)
enum
{
    HR_FNF = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
    HR_NF = HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
    HR_NOASSOC = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION),
    HR_ACCDENIED = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),
    AF_DEFSTAR = ASSOCF_INIT_DEFAULTTOSTAR,
    AF_DEFFOLDER = ASSOCF_INIT_DEFAULTTOFOLDER,
    AF_INITMASK = AF_DEFSTAR | AF_DEFFOLDER | ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_INIT_NOREMAPCLSID,
    AF_EXT = ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOFIXUPS | ASSOCF_NOTRUNCATE,
    AF_PID = ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOFIXUPS | ASSOCF_NOTRUNCATE,
    AF_GUID = ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOFIXUPS | ASSOCF_NOTRUNCATE,
};

struct RESULT
{
    HRESULT hr;
    LONG reg;
    BOOL match;
    WCHAR regdata[MAX_PATH * 2], querydata[_countof(regdata)];
};

static void Cleanup()
{
    SHDeleteKeyW(HKEY_CLASSES_ROOT, UNIQUEEXT), SHDeleteKeyW(HKEY_CLASSES_ROOT, UNIQUEEXT);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, UNIQUEPROGID), SHDeleteKeyW(HKEY_CLASSES_ROOT, UNIQUEPROGID);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, L"CLSID\\" UNIQUEGUID), SHDeleteKeyW(HKEY_CLASSES_ROOT, L"CLSID\\" UNIQUEGUID);
}

static HRESULT InitTest(IQueryAssociations **ppQA = NULL)
{
    Cleanup();
    return ppQA ? AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, ppQA)) : S_FALSE;
}

static HRESULT Init(UINT Flags, LPCWSTR pszAssoc, CComPtr<IQueryAssociations> &pQA, HKEY hProgId = NULL)
{
    if (pQA)
        pQA = NULL;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pQA));
    return SUCCEEDED(hr) ? pQA->Init((ASSOCF)Flags, pszAssoc, hProgId, NULL) : hr;
}

static HRESULT GetString(ASSOCSTR Str, UINT Flags, LPCWSTR pszAssoc, LPCWSTR pszExtra, PWSTR Out, HKEY hProgId = NULL)
{
    CComPtr<IQueryAssociations> pQA;
    HRESULT hr = Init(ASSOCF(Flags & AF_INITMASK), pszAssoc, pQA, hProgId);
    DWORD cch = MAX_PATH;
    lstrcpyW(Out, L"!InvalidData!");
    return SUCCEEDED(hr) ? pQA->GetString(ASSOCF(Flags & ~AF_INITMASK), Str, pszExtra, Out, &cch) : hr;
}

static void CheckHelper(RESULT &r, UINT check, UINT type, UINT flags, LPCWSTR initstr, HKEY initprogid,
                        LPCWSTR extra, HKEY rhk, LPCWSTR rpath, LPCWSTR rname)
{
    CComPtr<IQueryAssociations> pQA;
    r.hr = Init(ASSOCF(flags & AF_INITMASK), initstr, pQA, initprogid);
    flags &= ~AF_INITMASK;
    ZeroMemory(r.querydata, sizeof(r.querydata));

    ZeroMemory(r.regdata, sizeof(r.regdata));
    DWORD cbReg = sizeof(r.regdata);
    SRRF srrf = check <= REG_EXPAND_SZ ? SRRF_RT_REG_SZ : SRRF_RT_ANY;
    r.reg = SHRegGetValueW(rhk, rpath, rname, srrf, NULL, r.regdata, &cbReg);

    if (SUCCEEDED(r.hr) && r.reg == ERROR_SUCCESS)
    {
        if (check <= REG_EXPAND_SZ)
        {
            lstrcpyW(r.querydata, L"!InvalidData!");
            DWORD cch = _countof(r.querydata);
            r.hr = pQA->GetString(ASSOCF(flags), ASSOCSTR(type), extra, r.querydata, &cch);
            r.match = SUCCEEDED(r.hr) && !lstrcmpiW(r.querydata, r.regdata);
        }
        else
        {
            *(UINT64*)&r.querydata = 0xBAAAAAAAAAAAAAADull;
            DWORD cb = sizeof(r.querydata);
            r.hr = pQA->GetData(ASSOCF(flags), ASSOCDATA(type), extra, r.querydata, &cb);
            r.match = SUCCEEDED(r.hr) && cb == cbReg && !memcmp(r.querydata, r.regdata, cb);
        }
    }
    r.match |= (FAILED(r.hr) && r.reg != ERROR_SUCCESS);
}

#define CheckRegValue(hrexpect, reg, type, flags, initstr, initprogid, extra, rhk, rpath, rname) do \
{ \
    RESULT r; \
    CheckHelper(r, (reg), (type), (flags), (initstr), (initprogid), (extra), (rhk), (rpath), (rname)); \
    if ((hrexpect) == E_ANY) \
    { \
        ok(FAILED(r.hr), "Expected failure, got %#x\n", r.hr); \
    } \
    else \
    { \
        ok_hr(r.hr, (hrexpect)); \
        if (SUCCEEDED(r.hr)) \
            ok(r.match, "ASSOC %d did not match %ls |%ls|%ls|\n", (type), RegNameDisp(rname), r.querydata, r.regdata); \
    } \
} while(0)

#define CheckRegStr(hrexpect, type, flags, initstr, initprogid, extra, rhk, rpath, rname) \
    CheckRegValue((hrexpect), REG_SZ, (type), (flags), (initstr), (initprogid), (extra), (rhk), (rpath), (rname))
#define CheckRegData(hrexpect, type, flags, initstr, initprogid, extra, rhk, rpath, rname) \
    CheckRegValue((hrexpect), REG_BINARY, (type), (flags), (initstr), (initprogid), (extra), (rhk), (rpath), (rname))

START_TEST(IQueryAssociations)
{
    CCoInit ComInit;
    BOOL nt5 = LOBYTE(GetVersion()) < 6, patch;
    RESULT r;
    LPCWSTR pszPath, pszValue;
    WCHAR buf[MAX_PATH];

    CComPtr<IQueryAssociations> pQA;
    HRESULT hr = InitTest(&pQA);
    if (FAILED(hr))
    {
        skip("Could not create IQueryAssociations\n");
        return;
    }

    hr = Init(AF_EXT, L".exe", pQA);
    ok_hr(hr, S_OK);

    hr = Init(AF_EXT, UNIQUEEXT, pQA);
    ok_hr(hr, nt5 ? S_FALSE : S_OK); // Initialized but it has no HKEYs

    CheckRegStr(S_OK, ASSOCSTR_COMMAND, AF_EXT, L".bat", NULL, NULL, HKCR, L"batfile\\shell\\open\\command", NULL);
    CheckRegStr(S_OK, ASSOCSTR_CONTENTTYPE, AF_EXT, L".bmp", NULL, NULL, HKCR, L".bmp", L"Content Type");
    CheckRegStr(S_OK, ASSOCSTR_DEFAULTICON, AF_EXT, L".bat", NULL, NULL, HKCR, L"batfile\\DefaultIcon", NULL);
    CheckRegData(S_OK, ASSOCDATA_EDITFLAGS, AF_EXT, L".bat", NULL, NULL, HKCR, L"batfile", L"EditFlags");

    // ASSOCF_INIT_DEFAULTTOFOLDER
    patch = is_reactos() && SHRegGetValueW(HKCR, L"Folder", L"EditFlags", SRRF_RT_ANY, NULL, NULL, NULL);
    if (patch)
        RegSetString(HKCR, L"Folder", L"EditFlags", L"FixBrokenRos"); // Temporary fix
    CheckRegData(S_OK, ASSOCDATA_EDITFLAGS, AF_PID | AF_DEFFOLDER, UNIQUEPROGID L"NotExist", NULL, NULL, HKCR, L"Folder", L"EditFlags");
    if (patch)
        SHDeleteValueW(HKCR, L"Folder", L"EditFlags");

    // ASSOCDATA_VALUE in .ext without ProgId
    pszPath = L"Software\\Classes\\" UNIQUEEXT, pszValue = L"TestVal";
    RegSetString(HKCU, pszPath, pszValue, L"Ext1234");
    CheckHelper(r, REG_BINARY, ASSOCDATA_VALUE, AF_EXT, UNIQUEEXT, NULL, pszValue, HKCU, pszPath, pszValue);
    ok(!r.hr && !lstrcmpiW(r.querydata, r.regdata), "VALUE mismatch\n");

    // ASSOCDATA_VALUE in .ext with non-existing ProgId
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEEXT, NULL, UNIQUEPROGID);
    CheckHelper(r, REG_BINARY, ASSOCDATA_VALUE, AF_EXT, UNIQUEEXT, NULL, pszValue, HKCU, pszPath, pszValue);
    ok(!r.hr && !lstrcmpiW(r.querydata, r.regdata), "VALUE mismatch\n");

    // ASSOCDATA_VALUE in .ext without value in ProgId
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEPROGID, L"IgnoreMe", L"");
    CheckHelper(r, REG_BINARY, ASSOCDATA_VALUE, AF_EXT, UNIQUEEXT, NULL, pszValue, HKCU, pszPath, pszValue);
    ok(FAILED(r.hr), "Should not fall back to .ext\n");

    // ASSOCDATA_VALUE in ProgId
    pszPath = L"Software\\Classes\\" UNIQUEPROGID;
    RegSetString(HKCU, pszPath, pszValue, L"Pid1234");
    CheckHelper(r, REG_BINARY, ASSOCDATA_VALUE, AF_EXT, UNIQUEEXT, NULL, pszValue, HKCU, pszPath, pszValue);
    ok(!r.hr && !lstrcmpiW(r.querydata, r.regdata), "VALUE mismatch\n");

    // ASSOCF_INIT_DEFAULTTOSTAR
    pszValue = L"AlwaysShowExt";
    CheckRegData(E_ANY, ASSOCDATA_VALUE, AF_EXT | AF_DEFSTAR, UNIQUEEXT L"NotExist", NULL, NULL, HKCR, L"*", pszValue);

    CheckHelper(r, REG_BINARY, ASSOCDATA_VALUE, AF_EXT | AF_DEFSTAR, UNIQUEEXT, NULL, pszValue, HKCU, L"*", pszValue);
    ok(!r.hr && !lstrcmpiW(r.querydata, r.regdata), "VALUE mismatch\n");

    // ASSOCF_REMAPRUNDLL
    pszValue = L"%SystemRoot%\\System32\\rundll32.exe \"%SystemRoot%\\System32\\shell32.dll\", Func %1";
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEEXT, NULL, UNIQUEPROGID);
    RegSetStringEx(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\shell\\open\\command", NULL, pszValue, REG_EXPAND_SZ);
    
    hr = GetString(ASSOCSTR_COMMAND, AF_EXT, UNIQUEEXT, NULL, buf);
    ok(!hr && StrStrIW(buf, L"\\rundll32.exe"), "REMAPRUNDLL is not implied for COMMAND, got %ls\n", buf);

    hr = GetString(ASSOCSTR_COMMAND, AF_EXT | ASSOCF_REMAPRUNDLL, UNIQUEEXT, NULL, buf);
    ok(!hr && StrStrIW(buf, L"\\rundll32.exe"), "REMAPRUNDLL has no effect on COMMAND, got %ls\n", buf);

    // ASSOCSTR_EXECUTABLE
    hr = GetString(ASSOCSTR_EXECUTABLE, AF_EXT, UNIQUEEXT, NULL, buf);
    ok(!hr && !StrStrIW(buf, L"\\rundll32.exe") && StrStrIW(buf, L".dll"), "REMAPRUNDLL implied, got %ls\n", buf);

    pszValue = L"\"%SystemRoot%\\System32\\rundll32.exe\" \"%SystemRoot%\\System32\\shell32.dll\", Func %1"; // Quotes
    RegSetStringEx(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\shell\\open\\command", NULL, pszValue, REG_EXPAND_SZ);
    hr = GetString(ASSOCSTR_EXECUTABLE, AF_EXT, UNIQUEEXT, NULL, buf);
    ok(!hr && !StrStrIW(buf, L"\\rundll32.exe") && StrStrIW(buf, L".dll"), "REMAPRUNDLL implied, got %ls\n", buf);

    pszValue = L"rundll32.exe shell32.dll, Func %1";
    RegSetStringEx(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\shell\\open\\command", NULL, pszValue, REG_EXPAND_SZ);
    hr = GetString(ASSOCSTR_EXECUTABLE, AF_EXT, UNIQUEEXT, NULL, buf);
    ok(!hr && !StrStrIW(buf, L"\\rundll32.exe") && StrStrIW(buf, L".dll"), "REMAPRUNDLL implied, got %ls\n", buf);

    pszValue = L"%SystemRoot%\\System32\\rundll32.exe";
    RegSetStringEx(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\shell\\open\\command", NULL, pszValue, REG_EXPAND_SZ);
    hr = GetString(ASSOCSTR_EXECUTABLE, AF_EXT, UNIQUEEXT, NULL, buf);
    ok_hr(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)); // A bit surprising but true

    pszValue = L"\"%SystemRoot%\\System32\\cmd.exe\" /C \"%SystemRoot%\\System32\\rundll32.exe\"";
    RegSetStringEx(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\shell\\open\\command", NULL, pszValue, REG_EXPAND_SZ);
    hr = GetString(ASSOCSTR_EXECUTABLE, AF_EXT, UNIQUEEXT, NULL, buf);
    ok_hr(hr, S_OK);
    ok(!hr && StrStrIW(buf, L"\\cmd.exe") && !StrStrIW(buf, L"\\rundll32.exe"), "Just the first path, got %ls\n", buf);

    // ASSOCSTR_INFOTIP ProgId
    pszPath = L"Software\\Classes\\" UNIQUEPROGID, pszValue = L"InfoTip";
    RegSetString(HKCU, pszPath, pszValue, L"Pid1234");
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_EXT, UNIQUEEXT, NULL, NULL, HKCU, pszPath, pszValue);

    // CLSID
    pszPath = L"Software\\Classes\\CLSID\\" UNIQUEGUID, pszValue = L"InfoTip";
    RegSetString(HKCU, pszPath, pszValue, L"CLSID1234");
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_GUID, UNIQUEGUID, NULL, NULL, HKCU, pszPath, pszValue);

    // CLSID ignore ProgId redirection
    RegSetString(HKCU, L"Software\\Classes\\CLSID\\" UNIQUEGUID L"\\ProgId", NULL, UNIQUEPROGID);
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEPROGID, pszValue, L"Pid1234");
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_GUID | ASSOCF_INIT_NOREMAPCLSID, UNIQUEGUID, NULL, NULL, HKCU, pszPath, pszValue);

    // ASSOCF_INIT_NOREMAPCLSID seems to be implied on XP+?
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_GUID, UNIQUEGUID, NULL, NULL, HKCU, pszPath, pszValue);

    // ProgId CurVer redirection
    pszPath = L"Software\\Classes\\" UNIQUEPROGID2, pszValue = L"InfoTip";
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEPROGID L"\\CurVer", NULL, UNIQUEPROGID2);
    RegSetString(HKCU, L"Software\\Classes\\" UNIQUEPROGID2 L"\\shell", L"JustWantTheKeyToExist", NULL);
    RegSetString(HKCU, pszPath, pszValue, L"CV1234");
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_PID, UNIQUEPROGID, NULL, NULL, HKCU, pszPath, pszValue);

    // ASSOCSTR_INFOTIP .ext without ProgId
    Cleanup();
    pszPath = L"Software\\Classes\\" UNIQUEEXT, pszValue = L"InfoTip";
    RegSetString(HKCU, pszPath, pszValue, L"Ext1234");
    CheckRegStr(S_OK, ASSOCSTR_INFOTIP, AF_EXT, UNIQUEEXT, NULL, NULL, HKCU, pszPath, pszValue);

    Cleanup();
}
