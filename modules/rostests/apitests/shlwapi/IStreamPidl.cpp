/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for g_fnIStream_ReadPidl and g_fnIStream_WritePidl
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlwapi_undoc.h>

typedef HRESULT (WINAPI *FN_IStream_ReadPidl)(IStream *, _Out_ LPITEMIDLIST *);
typedef HRESULT (WINAPI *FN_IStream_WritePidl)(IStream *, LPCITEMIDLIST);
typedef UINT (WINAPI *FN_ILGetSize)(LPCITEMIDLIST);

static FN_IStream_ReadPidl g_fnIStream_ReadPidl = NULL;
static FN_IStream_WritePidl g_fnIStream_WritePidl = NULL;
static FN_ILGetSize g_fnILGetSize = NULL;

static LPITEMIDLIST MakeSimplePidl(const BYTE *data, UINT dataLen)
{
    UINT cbItem = (UINT)(sizeof(USHORT) + dataLen);
    UINT cbTotal = cbItem + sizeof(USHORT);
    LPITEMIDLIST pidl = (LPITEMIDLIST)CoTaskMemAlloc(cbTotal);
    if (!pidl)
        return NULL;
    ZeroMemory(pidl, cbTotal);
    pidl->mkid.cb = (USHORT)cbItem;
    if (dataLen)
        memcpy(pidl->mkid.abID, data, dataLen);
    return pidl;
}

static void RewindStream(IStream *pstm)
{
    LARGE_INTEGER li;
    li.QuadPart = 0;
    pstm->Seek(li, STREAM_SEEK_SET, NULL);
}

static void Test_RoundTrip_SimplePidl(void)
{
    const BYTE data[] = {0x11, 0x22, 0x33};
    LPITEMIDLIST pidlSrc = MakeSimplePidl(data, sizeof(data));
    LPITEMIDLIST pidlDst = NULL;
    HRESULT hr;
    IStream *pstm;

    ok(pidlSrc != NULL, "pidlSrc was NULL.\n");
    if (!pidlSrc)
    {
        skip("pidlSrc was NULL.\n");
        return;
    }

    pstm = SHCreateMemStream(NULL, 0);
    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    hr = g_fnIStream_WritePidl(pstm, pidlSrc);
    ok_hr(hr, S_OK);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidlDst);
    ok_hr(hr, S_OK);
    ok(pidlDst != NULL, "pidlDst was NULL\n");

    if (pidlDst)
    {
        UINT cbSrc = g_fnILGetSize(pidlSrc);
        UINT cbDst = g_fnILGetSize(pidlDst);
        ok_int(cbSrc, cbDst);
        ok_int(memcmp(pidlSrc, pidlDst, cbSrc), 0);
        CoTaskMemFree(pidlDst);
    }

    pstm->Release();
    CoTaskMemFree(pidlSrc);
}

static void Test_RoundTrip_EmptyPidl(void)
{
    LPITEMIDLIST pidlSrc = (LPITEMIDLIST)CoTaskMemAlloc(sizeof(USHORT));
    LPITEMIDLIST pidlDst = NULL;
    HRESULT hr;
    IStream *pstm;

    ok(pidlSrc != NULL, "CoTaskMemAlloc failed\n");
    if (!pidlSrc)
    {
        skip("pidlSrc was NULL\n");
        return;
    }

    ZeroMemory(pidlSrc, sizeof(USHORT));

    pstm = SHCreateMemStream(NULL, 0);
    ok(pstm != NULL, "pstm was NULL\n");
    if (!pstm)
    {
        skip("pstm was NULL\n");
        return;
    }

    hr = g_fnIStream_WritePidl(pstm, pidlSrc);
    ok_hr(hr, S_OK);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidlDst);
    ok_hr(hr, S_OK);
    ok(pidlDst != NULL, "pidlDst was NULL\n");

    if (pidlDst)
    {
        UINT cbSrc = g_fnILGetSize(pidlSrc);
        UINT cbDst = g_fnILGetSize(pidlDst);
        ok_int(cbSrc, cbDst);
        ok_int(memcmp(pidlSrc, pidlDst, cbSrc), 0);
        CoTaskMemFree(pidlDst);
    }

    pstm->Release();
    CoTaskMemFree(pidlSrc);
}

static void Test_RoundTrip_MultiItemPidl(void)
{
    UINT cbTotal = 4 + 5 + 2;
    LPITEMIDLIST pidlSrc = (LPITEMIDLIST)CoTaskMemAlloc(cbTotal);
    LPITEMIDLIST pidlDst = NULL;
    PBYTE p;
    HRESULT hr;
    IStream *pstm;

    ok(pidlSrc != NULL, "CoTaskMemAlloc failed\n");
    if (!pidlSrc)
    {
        skip("pidlSrc was NULL\n");
        return;
    }

    ZeroMemory(pidlSrc, cbTotal);
    p = (PBYTE)pidlSrc;

    *(USHORT *)p = 4; p += 2;
    *p++ = 0xAA; *p++ = 0xBB;

    *(USHORT *)p = 5; p += 2;
    *p++ = 0x01; *p++ = 0x02; *p++ = 0x03;

    pstm = SHCreateMemStream(NULL, 0);
    ok(pstm != NULL, "pstm was NULL\n");
    if (!pstm)
    {
        skip("pstm was NULL\n");
        return;
    }

    hr = g_fnIStream_WritePidl(pstm, pidlSrc);
    ok_hr(hr, S_OK);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidlDst);
    ok_hr(hr, S_OK);
    ok(pidlDst != NULL, "pidlDst was NULL\n");

    if (pidlDst)
    {
        ok_int(g_fnILGetSize(pidlSrc), g_fnILGetSize(pidlDst));
        ok_int(memcmp(pidlSrc, pidlDst, g_fnILGetSize(pidlSrc)), 0);
        CoTaskMemFree(pidlDst);
    }

    pstm->Release();
    CoTaskMemFree(pidlSrc);
}

static void Test_Read_EmptyStream(void)
{
    IStream *pstm = SHCreateMemStream(NULL, 0);
    LPITEMIDLIST pidl = NULL;
    HRESULT hr;

    ok(pstm != NULL, "pstm was NULL\n");
    if (!pstm)
    {
        skip("pstm was NULL\n");
        return;
    }

    hr = g_fnIStream_ReadPidl(pstm, &pidl);
    ok(FAILED(hr), "hr was wrongly succeeded\n");
    ok(pidl == NULL, "pidl was not NULL\n");

    pstm->Release();
}

static void Test_Read_TooSmallCbSize(void)
{
    IStream *pstm = SHCreateMemStream(NULL, 0);
    LPITEMIDLIST pidl = NULL;
    UINT cbSize = 1;
    ULONG cbWritten;
    HRESULT hr;

    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    pstm->Write(&cbSize, sizeof(cbSize), &cbWritten);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidl);
    ok(FAILED(hr), "hr was 0x%X\n", hr);
    ok(pidl == NULL, "pidl was not NULL\n");

    pstm->Release();
}

static void Test_Read_TruncatedData(void)
{
    IStream *pstm = SHCreateMemStream(NULL, 0);
    LPITEMIDLIST pidl = NULL;
    UINT cbSize = 100;
    ULONG cbWritten;
    HRESULT hr;

    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    pstm->Write(&cbSize, sizeof(cbSize), &cbWritten);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidl);
    ok(FAILED(hr), "hr was 0x%X\n", hr);
    ok(pidl == NULL, "pidl was not NULL\n");

    pstm->Release();
}

static void Test_Read_MissingTerminator(void)
{
    IStream *pstm = SHCreateMemStream(NULL, 0);
    LPITEMIDLIST pidl = NULL;
    UINT cbSize = 4;
    BYTE rawData[4] = {0x04, 0x00, 0x11, 0x22};
    ULONG cbWritten;
    HRESULT hr;

    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    pstm->Write(&cbSize, sizeof(cbSize), &cbWritten);
    pstm->Write(rawData, cbSize, &cbWritten);

    RewindStream(pstm);
    hr = g_fnIStream_ReadPidl(pstm, &pidl);
    ok(FAILED(hr), "hr was 0x%X\n", hr);
    ok(pidl == NULL, "pidl was not NULL\n");

    pstm->Release();
}

static void Test_Write_StreamPosition(void)
{
    const BYTE data[] = {0xDE, 0xAD};
    LPITEMIDLIST pidlSrc = MakeSimplePidl(data, sizeof(data));
    IStream *pstm = SHCreateMemStream(NULL, 0);
    ULARGE_INTEGER pos;
    LARGE_INTEGER zero;
    HRESULT hr;
    UINT expectedPos;

    ok(pidlSrc != NULL, "pidlSrc was NULL\n");
    if (!pidlSrc)
    {
        skip("pidlSrc was NULL\n");
        return;
    }

    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    zero.QuadPart = 0;

    hr = g_fnIStream_WritePidl(pstm, pidlSrc);
    ok_hr(hr, S_OK);

    pstm->Seek(zero, STREAM_SEEK_CUR, &pos);

    expectedPos = sizeof(UINT) + g_fnILGetSize(pidlSrc);
    ok_eq_longlong(pos.QuadPart, expectedPos);

    pstm->Release();
    CoTaskMemFree(pidlSrc);
}

static void Test_Read_OutputNullOnFailure(void)
{
    IStream *pstm = SHCreateMemStream(NULL, 0);
    LPITEMIDLIST pidl = (LPITEMIDLIST)UlongToPtr(0xDEADBEEF);
    HRESULT hr;

    ok(pstm != NULL, "pstm was NULL.\n");
    if (!pstm)
    {
        skip("pstm was NULL.\n");
        return;
    }

    hr = g_fnIStream_ReadPidl(pstm, &pidl);
    ok(FAILED(hr), "hr was 0x%X\n", hr);
    ok(pidl == NULL, "pidl was not NULL\n");

    pstm->Release();
}

START_TEST(IStreamPidl)
{
    HINSTANCE hSHLWAPI = LoadLibraryW(L"shlwapi");
    if (!hSHLWAPI)
    {
        skip("shlwapi not found\n");
        return;
    }

    g_fnIStream_ReadPidl = (FN_IStream_ReadPidl)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(512));
    g_fnIStream_WritePidl = (FN_IStream_WritePidl)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(513));
    if (!g_fnIStream_ReadPidl || !g_fnIStream_WritePidl)
    {
        skip("IStream_ReadPidl or IStream_WritePidl not found\n");
        FreeLibrary(hSHLWAPI);
        return;
    }

    HINSTANCE hSHELL32 = LoadLibraryW(L"shell32");
    if (!hSHELL32)
    {
        skip("shell32 not found\n");
        FreeLibrary(hSHLWAPI);
        return;
    }

    g_fnILGetSize = (FN_ILGetSize)GetProcAddress(hSHELL32, "ILGetSize");
    if (!g_fnILGetSize)
    {
        skip("ILGetSize not found\n");
        FreeLibrary(hSHELL32);
        FreeLibrary(hSHLWAPI);
        return;
    }

    HRESULT hrCoInit = CoInitialize(NULL);

    Test_RoundTrip_SimplePidl();
    Test_RoundTrip_EmptyPidl();
    Test_RoundTrip_MultiItemPidl();
    Test_Read_EmptyStream();
    Test_Read_TooSmallCbSize();
    Test_Read_TruncatedData();
    Test_Read_MissingTerminator();
    Test_Write_StreamPosition();
    Test_Read_OutputNullOnFailure();

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    FreeLibrary(hSHELL32);
    FreeLibrary(hSHLWAPI);
}
