/*
 * PROJECT:     ReactOS PathCch Library - Unit-tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     C/C++ compilation tests
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * REMARKS:
 * The purpose of these tests is to verify that the PathCch* functions can
 * be used either via static linking, or dynamic linking, in both C and C++
 * languages. Thus, only minimal testing of functionality is done here.
 */

#ifdef __cplusplus
extern "C"
void test_CPP_PathCch(void)
#else
void test_C_PathCch(void)
#endif
{
    const WCHAR szTestPath[] = L"C:\\ReactOS\\System32\\dllcache\\..\\..\\..\\freeldr.ini";
    const WCHAR szCanonPath[] = L"C:\\freeldr.ini";
    const WCHAR szExtraPath0[] = L"C:\\ReactOS\\System32";
    const WCHAR szExtraPath1[] = L"C:\\ReactOS\\System32\\";
    const WCHAR szExtraPath2[] = L"\\dllcache\\..\\..\\..\\freeldr.ini";
    const WCHAR szUNCPath[] = L"\\\\?\\UNC\\ReactOS_Server\\Root\\freeldr.ini";

    HRESULT hr;
    BOOL res;
#ifdef __cplusplus
    PWSTR psz;
#endif
    PCWSTR pcsz;
    PWSTR pszPathOut;
    WCHAR szPathOut[MAX_PATH];

    pszPathOut = NULL;
    hr = PathAllocCanonicalize(szTestPath, PATHCCH_NONE, &pszPathOut);
    ok_hr(hr, S_OK);
    ok(pszPathOut != NULL, "pszPathOut is NULL\n");
    ok_wstr(pszPathOut, szCanonPath);
    LocalFree(pszPathOut);

    *szPathOut = UNICODE_NULL;
    hr = PathCchCanonicalize(szPathOut, _countof(szPathOut), szTestPath);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    *szPathOut = UNICODE_NULL;
    hr = PathCchCanonicalizeEx(szPathOut, _countof(szPathOut), szTestPath, PATHCCH_NONE);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    pszPathOut = NULL;
    hr = PathAllocCombine(szExtraPath1, szExtraPath2, PATHCCH_NONE, &pszPathOut);
    ok_hr(hr, S_OK);
    ok(pszPathOut != NULL, "pszPathOut is NULL\n");
    ok_wstr(pszPathOut, szCanonPath);
    LocalFree(pszPathOut);

    *szPathOut = UNICODE_NULL;
    hr = PathCchCombine(szPathOut, _countof(szPathOut), szExtraPath1, szExtraPath2);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    *szPathOut = UNICODE_NULL;
    hr = PathCchCombineEx(szPathOut, _countof(szPathOut), szExtraPath1, szExtraPath2, PATHCCH_NONE);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath0);
    ok_hr(hr, S_OK);
    hr = PathCchAddBackslash(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szExtraPath1);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath0);
    ok_hr(hr, S_OK);
    hr = PathCchAddBackslashEx(szPathOut, _countof(szPathOut), NULL, NULL);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szExtraPath1);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), L"C:\\freeldr");
    ok_hr(hr, S_OK);
    hr = PathCchAddExtension(szPathOut, _countof(szPathOut), L"ini"); // The extension period is added if needed.
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath1);
    ok_hr(hr, S_OK);
    hr = PathCchAppend(szPathOut, _countof(szPathOut), szExtraPath2);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath1);
    ok_hr(hr, S_OK);
    hr = PathCchAppendEx(szPathOut, _countof(szPathOut), szExtraPath2, PATHCCH_NONE);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

#ifdef __cplusplus // Won't compile in C; is OK in C++
    psz = NULL;
    hr = PathCchFindExtension(const_cast<PWSTR>(szCanonPath), _countof(szCanonPath), &psz);
    ok_hr(hr, S_OK);
    ok_ptr(psz, &szCanonPath[10]);
#endif
    pcsz = NULL;
    hr = PathCchFindExtension(szCanonPath, _countof(szCanonPath), &pcsz);
    ok_hr(hr, S_OK);
    ok_ptr(pcsz, &szCanonPath[10]);

    res = PathCchIsRoot(szCanonPath);
    ok_bool_false(res, "PathCchIsRoot returned");
    res = PathCchIsRoot(L"C:\\");
    ok_bool_true(res, "PathCchIsRoot returned");

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath1);
    ok_hr(hr, S_OK);
    hr = PathCchRemoveBackslash(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szExtraPath0);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szExtraPath1);
    ok_hr(hr, S_OK);
    hr = PathCchRemoveBackslashEx(szPathOut, _countof(szPathOut), NULL, NULL);
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szExtraPath0);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szCanonPath);
    ok_hr(hr, S_OK);
    hr = PathCchRemoveExtension(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, L"C:\\freeldr");

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szCanonPath);
    ok_hr(hr, S_OK);
    hr = PathCchRemoveFileSpec(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, L"C:\\");

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szCanonPath);
    ok_hr(hr, S_OK);
    hr = PathCchRenameExtension(szPathOut, _countof(szPathOut), L"txt"); // The extension period is added if needed.
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, L"C:\\freeldr.txt");

#ifdef __cplusplus // Won't compile in C; is OK in C++
    psz = NULL;
    hr = PathCchSkipRoot(const_cast<PWSTR>(szCanonPath), &psz);
    ok_hr(hr, S_OK);
    ok_ptr(psz, &szCanonPath[3]);
#endif
    pcsz = NULL;
    hr = PathCchSkipRoot(szCanonPath, &pcsz);
    ok_hr(hr, S_OK);
    ok_ptr(pcsz, &szCanonPath[3]);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), L"\\\\?\\"); // Prefix to test its removal.
    ok_hr(hr, S_OK);
    hr = StringCchCatW(szPathOut, _countof(szPathOut), szCanonPath);
    ok_hr(hr, S_OK);
    hr = PathCchStripPrefix(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, szCanonPath);

    hr = StringCchCopyW(szPathOut, _countof(szPathOut), szTestPath);
    ok_hr(hr, S_OK);
    hr = PathCchStripToRoot(szPathOut, _countof(szPathOut));
    ok_hr(hr, S_OK);
    ok_wstr(szPathOut, L"C:\\");

#ifdef __cplusplus // Won't compile in C; is OK in C++
    psz = NULL;
    res = PathIsUNCEx(const_cast<PWSTR>(szCanonPath), &psz);
    ok_bool_false(res, "PathIsUNCEx returned");
    ok_ptr(psz, NULL);

    psz = NULL;
    res = PathIsUNCEx(const_cast<PWSTR>(szUNCPath), &psz);
    ok_bool_true(res, "PathIsUNCEx returned");
    ok_ptr(psz, &szUNCPath[8]);
#endif
    pcsz = NULL;
    res = PathIsUNCEx(szCanonPath, &pcsz);
    ok_bool_false(res, "PathIsUNCEx returned");
    ok_ptr(pcsz, NULL);

    pcsz = NULL;
    res = PathIsUNCEx(szUNCPath, &pcsz);
    ok_bool_true(res, "PathIsUNCEx returned");
    ok_ptr(pcsz, &szUNCPath[8]);
}

/* EOF */
