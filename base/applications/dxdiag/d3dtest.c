/*
 * PROJECT:         ReactX Diagnosis Application
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/dxdiag/d3dtest.c
 * PURPOSE:         ReactX Direct3D 7, 8 and 9 tests
 * PROGRAMMERS:     Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */

#include "precomp.h"

#define WIDTH   800
#define HEIGHT  600

BOOL D3D7Test(HWND hWnd);
BOOL D3D8Test(HWND hWnd);
BOOL D3D9Test(HWND hWnd);

BOOL StartD3DTest(HWND hWnd, HINSTANCE hInstance, WCHAR* pszCaption, INT TestNr)
{
    WCHAR szTestDescriptionRaw[256];
    WCHAR szTestDescription[256];
    WCHAR szCaption[256];
    WCHAR szResult[256];
    WCHAR szError[256];
    BOOL Result;

    LoadStringW(hInstance, IDS_MAIN_DIALOG, szCaption, sizeof(szCaption) / sizeof(WCHAR));
    LoadStringW(hInstance, IDS_DDTEST_ERROR, szError, sizeof(szError) / sizeof(WCHAR));
    LoadStringW(hInstance, IDS_D3DTEST_D3Dx, szTestDescriptionRaw, sizeof(szTestDescriptionRaw) / sizeof(WCHAR));
    //LoadStringW(hInstance, resResult, szResult, sizeof(szResult) / sizeof(WCHAR));

    swprintf(szTestDescription, szTestDescriptionRaw, TestNr);
    if (MessageBox(NULL, szTestDescription, szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);

    switch(TestNr){
        case 7:
            Result = D3D7Test(hWnd);
            break;
        case 8:
            Result = D3D8Test(hWnd);
            break;
        case 9:
            Result = D3D9Test(hWnd);
            break;
        default:
            Result = FALSE;
    }

    ShowWindow(hWnd, SW_HIDE);

    if(!Result)
    {
        MessageBox(NULL, szError, szCaption, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if(MessageBox(NULL, szResult, szCaption, MB_YESNO | MB_ICONQUESTION) == IDYES)
        return TRUE;

    return FALSE;
}

static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

VOID D3DTests()
{
    WNDCLASSEX winClass;
    HWND hWnd;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WCHAR szDescription[256];
    WCHAR szCaption[256];

    winClass.cbSize = sizeof(WNDCLASSEX);
    winClass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = WindowProc;
    winClass.cbClsExtra = 0;
    winClass.cbWndExtra = 0;
    winClass.hInstance = hInstance;
    winClass.hIcon = 0;
    winClass.hCursor = 0;
    winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    winClass.lpszMenuName = NULL;
    winClass.lpszClassName = L"d3dtest";
    winClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&winClass))
        return;

    hWnd = CreateWindowEx(
        0,
        winClass.lpszClassName,
        NULL,
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - WIDTH)/2,
        (GetSystemMetrics(SM_CYSCREEN) - HEIGHT)/2,
        WIDTH,
        HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hWnd)
        goto cleanup;

    LoadStringW(hInstance, IDS_D3DTEST_DESCRIPTION, szDescription, sizeof(szDescription) / sizeof(WCHAR));
    LoadStringW(hInstance, IDS_MAIN_DIALOG, szCaption, sizeof(szCaption) / sizeof(WCHAR));
    if(MessageBox(NULL, szDescription, szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
        goto cleanup;

    StartD3DTest(hWnd, hInstance, szCaption, 7);
    StartD3DTest(hWnd, hInstance, szCaption, 8);
    StartD3DTest(hWnd, hInstance, szCaption, 9);

cleanup:
    DestroyWindow(hWnd);
    UnregisterClass(winClass.lpszClassName, hInstance);
}
