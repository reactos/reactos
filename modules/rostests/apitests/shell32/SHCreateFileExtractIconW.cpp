/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHCreateFileExtractIconW
 * COPYRIGHT:   Copyright 2017,2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "shelltest.h"

#include <wincon.h>
#include <wingdi.h>
#include <stdio.h>
#include <shellutils.h>

HRESULT (STDAPICALLTYPE *pSHCreateFileExtractIconW)(LPCWSTR pszFile, DWORD dwFileAttributes, REFIID riid, void **ppv);

struct TestData
{
    const WCHAR* Name;
    DWORD dwFlags;
};

static TestData IconTests[] =
{
    { L"xxx.zip", FILE_ATTRIBUTE_NORMAL },
    { L"xxx.zip", FILE_ATTRIBUTE_DIRECTORY },
    { L"xxx.exe", FILE_ATTRIBUTE_NORMAL },
    { L"xxx.exe", FILE_ATTRIBUTE_DIRECTORY },
    { L"xxx.dll", FILE_ATTRIBUTE_NORMAL },
    { L"xxx.dll", FILE_ATTRIBUTE_DIRECTORY },
    { L"xxx.txt", FILE_ATTRIBUTE_NORMAL },
    { L"xxx.txt", FILE_ATTRIBUTE_DIRECTORY },
    { NULL, FILE_ATTRIBUTE_NORMAL },
    { NULL, FILE_ATTRIBUTE_DIRECTORY },
};

struct TestIID
{
    const GUID* IID;
    HRESULT ExpectedCreate;
    HRESULT ExpectedQueryInterface;
};

static TestIID InterfaceTests[] =
{
    { &IID_IDefaultExtractIconInit, E_NOINTERFACE, E_NOINTERFACE },
    { &IID_IExtractIconW, S_OK, S_OK },
    { &IID_IExtractIconA, S_OK, S_OK },
    { &IID_IPersist, E_NOINTERFACE, E_NOINTERFACE },
    { &IID_IPersistFile, E_NOINTERFACE, E_NOINTERFACE },
};


static void ExtractOneBitmap(HBITMAP hbm, CComHeapPtr<BYTE>& data, DWORD& size)
{
    HDC hdc = CreateCompatibleDC(NULL);
    HGDIOBJ obj = SelectObject(hdc, hbm);

    CComHeapPtr<BITMAPINFO> pInfoBM;

    pInfoBM.AllocateBytes(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    memset(pInfoBM, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    pInfoBM->bmiHeader.biSize = sizeof(pInfoBM->bmiHeader);
    if (!GetDIBits(hdc, hbm, 0, 0, NULL, pInfoBM, DIB_RGB_COLORS))
        return;

    size = pInfoBM->bmiHeader.biSizeImage;
    data.Allocate(size);
    GetDIBits(hdc, hbm, 0, pInfoBM->bmiHeader.biHeight, data, pInfoBM, DIB_RGB_COLORS);

    SelectObject(hdc, obj);
    DeleteDC(hdc);
}

static bool GetIconData(HICON icon, CComHeapPtr<BYTE>& colorData, DWORD& colorSize, CComHeapPtr<BYTE>& maskData, DWORD& maskSize)
{
    ICONINFO iconinfo;

    if (!GetIconInfo(icon, &iconinfo))
        return false;

    ExtractOneBitmap(iconinfo.hbmColor, colorData, colorSize);
    ExtractOneBitmap(iconinfo.hbmMask, maskData, maskSize);

    DeleteObject(iconinfo.hbmColor);
    DeleteObject(iconinfo.hbmMask);

    return true;
}


START_TEST(SHCreateFileExtractIconW)
{
    WCHAR CurrentModule[MAX_PATH];
    HMODULE shell32 = LoadLibraryA("shell32.dll");
    HICON myIcon;
    pSHCreateFileExtractIconW = (HRESULT (__stdcall *)(LPCWSTR, DWORD, REFIID, void **))GetProcAddress(shell32, "SHCreateFileExtractIconW");

    /* Show that icons returned are always the same */
    UINT tryFlags[4] = { 0, GIL_FORSHORTCUT, GIL_OPENICON };

    CoInitialize(NULL);

    GetModuleFileNameW(NULL, CurrentModule, _countof(CurrentModule));
    {
        SHFILEINFOW shfi;
        ULONG_PTR firet = SHGetFileInfoW(CurrentModule, 0, &shfi, sizeof(shfi), SHGFI_ICON);
        myIcon = shfi.hIcon;
        if (!firet)
        {
            skip("Unable to get my own icon\n");
            return;
        }
    }

    if (!pSHCreateFileExtractIconW)
    {
        skip("SHCreateFileExtractIconW not available\n");
        return;
    }

    for (size_t n = 0; n < _countof(InterfaceTests); ++n)
    {
        {
            CComPtr<IUnknown> spUnknown;
            HRESULT hr = pSHCreateFileExtractIconW(L"test.txt", FILE_ATTRIBUTE_NORMAL, *InterfaceTests[n].IID, (void**)&spUnknown);
            ok(hr == InterfaceTests[n].ExpectedCreate, "Expected hr to be 0x%lx, was 0x%lx for %u\n", InterfaceTests[n].ExpectedCreate, hr, n);
        }

        {
            CComPtr<IUnknown> spUnknown, spUnknown2;
            HRESULT hr = pSHCreateFileExtractIconW(L"test.txt", FILE_ATTRIBUTE_NORMAL, IID_PPV_ARG(IUnknown, &spUnknown));
            ok(hr == S_OK, "Expected hr to be S_OK, was 0x%lx for %u\n", hr, n);

            hr = spUnknown->QueryInterface(*InterfaceTests[n].IID, (void**)&spUnknown2);
            ok(hr == InterfaceTests[n].ExpectedQueryInterface, "Expected hr to be 0x%lx, was 0x%lx for %u\n", InterfaceTests[n].ExpectedQueryInterface, hr, n);
        }
    }

    for (size_t n = 0; n < _countof(IconTests); ++n)
    {
        TestData& cur = IconTests[n];
        bool useMyIcon = false;

        if (cur.Name == NULL)
        {
            cur.Name = CurrentModule;
            useMyIcon = true;
        }

        CComPtr<IExtractIconW> spExtract;
        HRESULT hr = pSHCreateFileExtractIconW(cur.Name, cur.dwFlags, IID_PPV_ARG(IExtractIconW, &spExtract));
        ok(hr == S_OK, "Expected hr to be S_OK, was 0x%lx for %S(%lx)\n", hr, cur.Name, cur.dwFlags);

        if (!SUCCEEDED(hr))
            continue;

        /* Show that GIL_DEFAULTICON does not work. */
        {
            int ilIndex = -1;
            UINT wFlags = 0xdeaddead;
            WCHAR Buffer[MAX_PATH];

            hr = spExtract->GetIconLocation(GIL_DEFAULTICON, Buffer, _countof(Buffer), &ilIndex, &wFlags);
            ok(hr == S_FALSE, "Expected hr to be S_FALSE, was 0x%lx for %S(0x%lx)\n", hr, cur.Name, cur.dwFlags);
        }


        for (UINT idFlags = 0; idFlags < _countof(tryFlags); ++idFlags)
        {
            int ilIndex = -1;
            UINT wFlags = 0xdeaddead;
            WCHAR Buffer[MAX_PATH];

            hr = spExtract->GetIconLocation(tryFlags[idFlags], Buffer, _countof(Buffer), &ilIndex, &wFlags);
            ok(hr == S_OK, "Expected hr to be S_OK, was 0x%lx for %S(0x%lx,0x%x)\n", hr, cur.Name, cur.dwFlags, tryFlags[idFlags]);
            if (!SUCCEEDED(hr))
                continue;

            ok(wFlags & (GIL_NOTFILENAME|GIL_PERCLASS), "Expected GIL_NOTFILENAME|GIL_PERCLASS to be set for %S(0x%lx,0x%x)\n", cur.Name, cur.dwFlags, tryFlags[idFlags]);
            ok(!wcscmp(Buffer, L"*"), "Expected '*', was '%S' for %S(0x%lx,0x%x)\n", Buffer, cur.Name, cur.dwFlags, tryFlags[idFlags]);

            HICON ico;
            hr = spExtract->Extract(Buffer, ilIndex, &ico, NULL, 0);

            /* Visualize the icon extracted for whoever is stepping through this code. */
            HWND console = GetConsoleWindow();
            SendMessage(console, WM_SETICON, ICON_BIG, (LPARAM)ico);
            SendMessage(console, WM_SETICON, ICON_SMALL, (LPARAM)ico);

            CComHeapPtr<BYTE> colorData, maskData;
            DWORD colorSize = 0, maskSize = 0;

            GetIconData(ico, colorData, colorSize, maskData, maskSize);

            if (!colorSize || !maskSize)
                continue;

            SHFILEINFOW shfi;
            ULONG_PTR firet = SHGetFileInfoW(cur.Name, cur.dwFlags, &shfi, sizeof(shfi), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SYSICONINDEX);

            if (!firet)
                continue;

            ok(shfi.iIcon == ilIndex, "Expected ilIndex to be 0%x, was 0x%x for %S(0x%lx,0x%x)\n", shfi.iIcon, ilIndex, cur.Name, cur.dwFlags, tryFlags[idFlags]);


            CComHeapPtr<BYTE> colorDataRef, maskDataRef;
            DWORD colorSizeRef = 0, maskSizeRef = 0;
            GetIconData(shfi.hIcon, colorDataRef, colorSizeRef, maskDataRef, maskSizeRef);

            ok(colorSizeRef == colorSize, "Expected %lu, was %lu for %S(0x%lx,0x%x)\n", colorSizeRef, colorSize, cur.Name, cur.dwFlags, tryFlags[idFlags]);
            ok(maskSizeRef == maskSize, "Expected %lu, was %lu for %S(0x%lx,0x%x)\n", maskSizeRef, maskSize, cur.Name, cur.dwFlags, tryFlags[idFlags]);

            if (colorSizeRef == colorSize)
            {
                ok(!memcmp(colorData, colorDataRef, colorSize), "Expected equal colorData for %S(0x%lx,0x%x)\n", cur.Name, cur.dwFlags, tryFlags[idFlags]);
            }

            if (maskSizeRef == maskSize)
            {
                ok(!memcmp(maskData, maskDataRef, maskSize), "Expected equal maskData for %S(0x%lx,0x%x)\n", cur.Name, cur.dwFlags, tryFlags[idFlags]);
            }

            if (useMyIcon)
            {
                colorDataRef.Free();
                maskDataRef.Free();
                colorSizeRef = maskSizeRef = 0;
                GetIconData(myIcon, colorDataRef, colorSizeRef, maskDataRef, maskSizeRef);

                ok(colorSizeRef == colorSize, "Expected %lu, was %lu for %S(0x%lx,0x%x)\n", colorSizeRef, colorSize, cur.Name, cur.dwFlags, tryFlags[idFlags]);
                ok(maskSizeRef == maskSize, "Expected %lu, was %lu for %S(0x%lx,0x%x)\n", maskSizeRef, maskSize, cur.Name, cur.dwFlags, tryFlags[idFlags]);

                if (colorSizeRef == colorSize)
                {
                    /* In case requested filetype does not match, the exe icon is not used! */
                    if (cur.dwFlags == FILE_ATTRIBUTE_DIRECTORY)
                    {
                        ok(memcmp(colorData, colorDataRef, colorSize), "Expected colorData to be changed for %S(0x%lx,0x%x)\n", cur.Name, cur.dwFlags, tryFlags[idFlags]);
                    }
                    else
                    {
                        ok(!memcmp(colorData, colorDataRef, colorSize), "Expected equal colorData for %S(0x%lx,0x%x)\n", cur.Name, cur.dwFlags, tryFlags[idFlags]);
                    }
                }

                // Mask is not reliable for some reason
                //if (maskSizeRef == maskSize)
                //{
                //    ok(!memcmp(maskData, maskDataRef, maskSize), "Expected equal maskData for %S(0x%lx,0x%lx)\n", cur.Name, cur.dwFlags);
                //}
            }
        }
    }
}
