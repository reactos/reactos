/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for LoadImageW using DLL compiled with MSVC
 * COPYRIGHT:   Copyright 2024 Doug Lyons <douglyons@douglyons.com>
 *
 * NOTES:
 * Works on ReactOS, but not on Windows 2003 Server SP2.
 */

#include "precomp.h"
#include "resource.h"
#include <stdio.h>
#include <versionhelpers.h>

WCHAR szWindowClass[] = L"testclass";

BOOL FileExistsW(PCWSTR FileName)
{
    DWORD Attribute = GetFileAttributesW(FileName);

    return (Attribute != INVALID_FILE_ATTRIBUTES &&
            !(Attribute & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL ResourceToFileW(INT i, PCWSTR FileName)
{
    FILE *fout;
    HGLOBAL hData;
    HRSRC hRes;
    PVOID pResLock;
    UINT iSize;

    if (FileExistsW(FileName))
    {
        /* We should only be using %temp% paths, so deleting here should be OK */
        printf("Deleting '%S' that already exists.\n", FileName);
        DeleteFileW(FileName);
    }

    hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
    if (hRes == NULL)
    {
        skip("Could not locate resource (%d). Exiting now\n", i);
        return FALSE;
    }

    iSize = SizeofResource(NULL, hRes);

    hData = LoadResource(NULL, hRes);
    if (hData == NULL)
    {
        skip("Could not load resource (%d). Exiting now\n", i);
        return FALSE;
    }

    // Lock the resource into global memory.
    pResLock = LockResource(hData);
    if (pResLock == NULL)
    {
        skip("Could not lock resource (%d). Exiting now\n", i);
        return FALSE;
    }

    fout = _wfopen(FileName, L"wb");
    fwrite(pResLock, iSize, 1, fout);
    fclose(fout);
    return TRUE;
}

static struct
{
    PCWSTR FileName;
    INT ResourceId;
} DataFiles[] =
{
    {L"%SystemRoot%\\bin\\image.dll", IDR_DLL_NORMAL},
};

/* MessageBox statements are left commented out for debugging purposes */
static LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hBmp;
    HANDLE handle;
    CHAR buffer[80];

    switch (message)
    {
        case WM_CREATE:
        {
            handle = LoadLibraryExW(L"image.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
            sprintf(buffer, "%p", handle);
//            MessageBoxA(NULL, buffer, "handle", 0);
            hBmp = (HBITMAP)LoadImage(handle, MAKEINTRESOURCE(130), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            sprintf(buffer, "%p", hBmp);
//            MessageBoxA(NULL, buffer, "Bmp", 0);
            sprintf(buffer, "%ld", GetLastError());
//            MessageBoxA(NULL, buffer, "LastError", 0);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;
            BITMAP bitmap;
            BITMAPINFO bmi;
            hdc = BeginPaint(hWnd, &ps);
            HGLOBAL hMem;
            LPVOID lpBits;
            CHAR img[8] = { 0 };
            UINT size;

            hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, hBmp);
            GetObject(hBmp, sizeof(BITMAP), &bitmap);
            sprintf(buffer, "H = %ld, W = %ld", bitmap.bmHeight, bitmap.bmWidth);
//            MessageBoxA(NULL, buffer, "test", 0);
            memset(&bmi, 0, sizeof(bmi));
            bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth       = bitmap.bmWidth;
            bmi.bmiHeader.biHeight      = bitmap.bmHeight;
            bmi.bmiHeader.biPlanes      = bitmap.bmPlanes;
            bmi.bmiHeader.biBitCount    = bitmap.bmBitsPixel;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage   = 0;

            size = ((bitmap.bmWidth * bmi.bmiHeader.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;
            sprintf(buffer, "size = %d", size);
//            MessageBoxA(NULL, buffer, "size", 0);

            hMem = GlobalAlloc(GMEM_MOVEABLE, size);
            lpBits = GlobalLock(hMem);
            GetDIBits(hdc, hBmp, 0, bitmap.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);

            /* Get bottom line of bitmap */
            memcpy(img, lpBits, 8);
            sprintf(buffer, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
              img[0] & 0xff, img[1] & 0xff, img[2] & 0xff, img[3] & 0xff,
              img[4] & 0xff, img[5] & 0xff, img[6] & 0xff, img[7] & 0xff);
//            MessageBoxA(NULL, buffer, "chars 1st Line", 0);

            /* Get one line above bottom line of bitmap */
            memcpy(img, (VOID *)((INT_PTR)lpBits + 4 * bitmap.bmWidth), 8);
            sprintf(buffer, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
              img[0] & 0xff, img[1] & 0xff, img[2] & 0xff, img[3] & 0xff,
              img[4] & 0xff, img[5] & 0xff, img[6] & 0xff, img[7] & 0xff);
//            MessageBoxA(NULL, buffer, "chars 2nd Line", 0);

            ok(img[0] == 0, "Byte 0 Bad\n");
            ok(img[1] == 0, "Byte 1 Bad\n");
            ok(img[2] == 0, "Byte 2 Bad\n");
            ok(img[3] == 0, "Byte 3 Bad\n");

            GlobalUnlock(hMem);
            GlobalFree(hMem);              

            DeleteDC(hdcMem);			

            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

static ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = NULL;

    return RegisterClassExW(&wcex);
}


static BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hWnd = CreateWindowExW(0,
                          szWindowClass,
                          L"Bmp test",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          300,
                          120,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

START_TEST(LoadImageGCC)
{
    UINT i;
    WCHAR PathBuffer[MAX_PATH];

    /* Windows 2003 cannot run this test. Testman shows CRASH, so skip it. */
    if (!IsReactOS())
        return;

    /* Extract Data Files */
    for (i = 0; i < _countof(DataFiles); ++i)
    {
        ExpandEnvironmentStringsW(DataFiles[i].FileName, PathBuffer, _countof(PathBuffer));

        if (!ResourceToFileW(DataFiles[i].ResourceId, PathBuffer))
        {
            printf("ResourceToFile Failed. Exiting now\n");
            goto Cleanup;
        }
    }

    MyRegisterClass(NULL);

    if (!InitInstance(NULL, SW_NORMAL))  // SW_NORMAL is needed or test does not work
    {
        printf("InitInstance Failed. Exiting now\n");
    }

    /* We would normally have a message loop here, but we don't want to wait in this test case */

Cleanup:
    for (i = 0; i < _countof(DataFiles); ++i)
    {
        ExpandEnvironmentStringsW(DataFiles[i].FileName, PathBuffer, _countof(PathBuffer));
        DeleteFileW(PathBuffer);
    }
}
