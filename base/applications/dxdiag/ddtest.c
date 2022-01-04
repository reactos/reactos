/*
 * PROJECT:     ReactX Diagnosis Application
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/dxdiag/ddtest.c
 * PURPOSE:     ReactX DirectDraw tests
 * COPYRIGHT:   Copyright 2008 Kamil Hornicek
 *
 */

#include "precomp.h"

BOOL DDPrimarySurfaceTest(HWND hWnd);
BOOL DDOffscreenBufferTest(HWND hWnd, BOOL Fullscreen);
VOID DDRedrawFrame(LPDIRECTDRAWSURFACE lpDDSurface);
VOID DDUpdateFrame(LPDIRECTDRAWSURFACE lpDDPrimarySurface ,LPDIRECTDRAWSURFACE lpDDBackBuffer, BOOL Fullscreen, INT *posX, INT *posY, INT *gainX, INT *gainY, RECT *rectDD);

#define TEST_DURATION 10000
#define DD_TEST_WIDTH 640
#define DD_TEST_HEIGHT 480
#define DD_TEST_STEP 5
#define DD_SQUARE_SIZE 100
#define DD_SQUARE_STEP 2


BOOL StartDDTest(HWND hWnd, HINSTANCE hInstance, INT resTestDescription, INT resResult, INT TestNr)
{
    WCHAR szTestDescription[256];
    WCHAR szCaption[256];
    WCHAR szResult[256];
    WCHAR szError[256];
    BOOL Result;

    LoadStringW(hInstance, IDS_MAIN_DIALOG, szCaption, sizeof(szCaption) / sizeof(WCHAR));
    LoadStringW(hInstance, IDS_DDTEST_ERROR, szError, sizeof(szError) / sizeof(WCHAR));
    LoadStringW(hInstance, resTestDescription, szTestDescription, sizeof(szTestDescription) / sizeof(WCHAR));
    LoadStringW(hInstance, resResult, szResult, sizeof(szResult) / sizeof(WCHAR));

    if(MessageBox(NULL, szTestDescription, szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);

    switch(TestNr){
        case 1:
            Result = DDPrimarySurfaceTest(hWnd);
            break;
        case 2:
            Result = DDOffscreenBufferTest(hWnd, FALSE);
            break;
        case 3:
            Result = DDOffscreenBufferTest(hWnd, TRUE);
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

VOID DDTests()
{
    WNDCLASSEX winClass;
    HWND hWnd;
    HINSTANCE hInstance = NULL;
    WCHAR szDescription[256];
    WCHAR szCaption[256];

    winClass.cbSize = sizeof(WNDCLASSEX);
    winClass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = DefWindowProc;
    winClass.cbClsExtra = 0;
    winClass.cbWndExtra = 0;
    winClass.hInstance = hInstance;
    winClass.hIcon = 0;
    winClass.hCursor = 0;
    winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    winClass.lpszMenuName = NULL;
    winClass.lpszClassName = L"ddtest";
    winClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&winClass))
        return;

    hWnd = CreateWindowEx(0, winClass.lpszClassName, NULL,WS_POPUP,
                          (GetSystemMetrics(SM_CXSCREEN) - DD_TEST_WIDTH)/2,
                          (GetSystemMetrics(SM_CYSCREEN) - DD_TEST_HEIGHT)/2,
                          DD_TEST_WIDTH, DD_TEST_HEIGHT, NULL, NULL, hInstance, NULL);

    if (!hWnd){
        return;
    }

    LoadStringW(hInstance, IDS_DDTEST_DESCRIPTION, szDescription, sizeof(szDescription) / sizeof(WCHAR));
    LoadStringW(hInstance, IDS_MAIN_DIALOG, szCaption, sizeof(szCaption) / sizeof(WCHAR));
    if(MessageBox(NULL, szDescription, szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
        return;

    StartDDTest(hWnd, hInstance, IDS_DDPRIMARY_DESCRIPTION, IDS_DDPRIMARY_RESULT, 1);
    StartDDTest(hWnd, hInstance, IDS_DDOFFSCREEN_DESCRIPTION, IDS_DDOFFSCREEN_RESULT, 2);
    StartDDTest(hWnd, hInstance, IDS_DDFULLSCREEN_DESCRIPTION, IDS_DDFULLSCREEN_RESULT, 3);

    DestroyWindow(hWnd);
    UnregisterClass(winClass.lpszClassName, hInstance);
}

BOOL DDPrimarySurfaceTest(HWND hWnd){
    UINT TimerID;
    MSG msg;

    LPDIRECTDRAW lpDD = NULL;
    LPDIRECTDRAWSURFACE lpDDSurface = NULL;
    DDSURFACEDESC DDSurfaceDesc;

    if(DirectDrawCreate(NULL, &lpDD, NULL) != DD_OK)
        return FALSE;

    if(lpDD->lpVtbl->SetCooperativeLevel(lpDD, hWnd, DDSCL_NORMAL) != DD_OK)
    {
        lpDD->lpVtbl->Release(lpDD);
        return FALSE;
    }

    /* create our primary surface */
    ZeroMemory(&DDSurfaceDesc, sizeof(DDSurfaceDesc));
    DDSurfaceDesc.dwSize = sizeof(DDSurfaceDesc);
    DDSurfaceDesc.dwFlags = DDSD_CAPS;
    DDSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    DDSurfaceDesc.dwBackBufferCount = 0;

    if(lpDD->lpVtbl->CreateSurface(lpDD, &DDSurfaceDesc, &lpDDSurface, NULL) != DD_OK)
    {
        lpDD->lpVtbl->Release(lpDD);
        return FALSE;
    }

    TimerID = SetTimer(hWnd, -1, (UINT)TEST_DURATION, NULL);

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_TIMER && TimerID == msg.wParam)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_PAINT)
                DDRedrawFrame(lpDDSurface);
        }
    }
    KillTimer(hWnd, TimerID);
    lpDDSurface->lpVtbl->Release(lpDDSurface);
    lpDD->lpVtbl->Release(lpDD);

return TRUE;
}

VOID DDRedrawFrame(LPDIRECTDRAWSURFACE lpDDSurface)
{
    HDC hdc;

    if (lpDDSurface->lpVtbl->GetDC(lpDDSurface, &hdc) == DD_OK)
    {
        RECT rct;
        HBRUSH BlackBrush, WhiteBrush;
        BOOL Colour = FALSE;

        rct.left = (GetSystemMetrics(SM_CXSCREEN) - DD_TEST_WIDTH)/2;
        rct.right = (GetSystemMetrics(SM_CXSCREEN) - DD_TEST_WIDTH)/2 + DD_TEST_WIDTH;
        rct.top = (GetSystemMetrics(SM_CYSCREEN) - DD_TEST_HEIGHT)/2;
        rct.bottom = (GetSystemMetrics(SM_CYSCREEN) - DD_TEST_HEIGHT)/2 + DD_TEST_HEIGHT;

        BlackBrush = CreateSolidBrush(RGB(0,0,0));
        WhiteBrush = CreateSolidBrush(RGB(255,255,255));

        while((rct.bottom - rct.top) > DD_TEST_STEP){
            (Colour)? FillRect(hdc, &rct, WhiteBrush) : FillRect(hdc, &rct, BlackBrush);
            rct.top += DD_TEST_STEP;
            rct.bottom -= DD_TEST_STEP;
            rct.left += DD_TEST_STEP;
            rct.right -= DD_TEST_STEP;
            Colour = !Colour;
        }
        DeleteObject((HGDIOBJ)BlackBrush);
        DeleteObject((HGDIOBJ)WhiteBrush);
        lpDDSurface->lpVtbl->ReleaseDC(lpDDSurface, hdc);
    }
}


BOOL DDOffscreenBufferTest(HWND hWnd, BOOL Fullscreen){
    UINT_PTR TimerID, TimerIDUpdate;
    LPDIRECTDRAW lpDD;
    LPDIRECTDRAWSURFACE lpDDPrimarySurface;
    LPDIRECTDRAWSURFACE lpDDBackBuffer;
    DDSURFACEDESC DDSurfaceDesc;
    DDSURFACEDESC DDBBSurfaceDesc;
    DDSCAPS DDSCaps;
    MSG msg;
    RECT rectDD;
    POINT wndPoint;
    INT posX = 0, posY = 10, xGain = DD_SQUARE_STEP, yGain = DD_SQUARE_STEP;

    if(DirectDrawCreate(NULL, &lpDD, NULL) != DD_OK)
        return FALSE;

    ZeroMemory(&DDSurfaceDesc, sizeof(DDSurfaceDesc));
    DDSurfaceDesc.dwSize = sizeof(DDSurfaceDesc);

    if(Fullscreen)
    {
        if(lpDD->lpVtbl->SetCooperativeLevel(lpDD, hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) != DD_OK)
        {
            lpDD->lpVtbl->Release(lpDD);
            return FALSE;
        }
        if(lpDD->lpVtbl->SetDisplayMode(lpDD, DD_TEST_WIDTH, DD_TEST_HEIGHT, 32) != DD_OK)
        {
            lpDD->lpVtbl->Release(lpDD);
            return FALSE;
        }
        DDSurfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        DDSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE  | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        DDSurfaceDesc.dwBackBufferCount = 1;
    }
    else
    {
        if(lpDD->lpVtbl->SetCooperativeLevel(lpDD, hWnd, DDSCL_NORMAL) != DD_OK)
        {
            lpDD->lpVtbl->Release(lpDD);
            return FALSE;
        }
        DDSurfaceDesc.dwFlags = DDSD_CAPS;
        DDSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    }


    if(lpDD->lpVtbl->CreateSurface(lpDD, &DDSurfaceDesc, &lpDDPrimarySurface, NULL) != DD_OK)
    {
        lpDD->lpVtbl->Release(lpDD);
        return FALSE;
    }

    if(Fullscreen)
    {
        DDSCaps.dwCaps = DDSCAPS_BACKBUFFER;
        if (lpDDPrimarySurface->lpVtbl->GetAttachedSurface(lpDDPrimarySurface, &DDSCaps,&lpDDBackBuffer) != DD_OK)
        {
            lpDDPrimarySurface->lpVtbl->Release(lpDDPrimarySurface);
            lpDD->lpVtbl->Release(lpDD);
            return FALSE;
        }
    }
    else
    {
        /* create our offscreen back buffer */
        ZeroMemory(&DDBBSurfaceDesc,sizeof(DDBBSurfaceDesc));
        DDBBSurfaceDesc.dwSize = sizeof(DDBBSurfaceDesc);
        DDBBSurfaceDesc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        DDBBSurfaceDesc.dwHeight = DD_TEST_HEIGHT;
        DDBBSurfaceDesc.dwWidth = DD_TEST_WIDTH;
        DDBBSurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if(lpDD->lpVtbl->CreateSurface(lpDD, &DDBBSurfaceDesc, &lpDDBackBuffer, NULL) != DD_OK)
        {
            lpDD->lpVtbl->Release(lpDD);
            lpDDPrimarySurface->lpVtbl->Release(lpDDPrimarySurface);
            return FALSE;
        }
        wndPoint.x = 0;
        wndPoint.y = 0;
        ClientToScreen(hWnd, &wndPoint);
        GetClientRect(hWnd, &rectDD);
        OffsetRect(&rectDD, wndPoint.x, wndPoint.y);
    }

    /* set our timers, TimerID - for test timeout, TimerIDUpdate - for frame updating */
    TimerID = SetTimer(hWnd, -1, (UINT)TEST_DURATION, NULL);
    TimerIDUpdate = SetTimer(hWnd, 2, (UINT)10, NULL);
    (void)TimerIDUpdate;

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_TIMER)
            {
                if(msg.wParam == TimerID)
                {
                    break;
                }
                else
                {
                    DDUpdateFrame(lpDDPrimarySurface,lpDDBackBuffer, Fullscreen,&posX, &posY, &xGain, &yGain, &rectDD);
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    lpDDPrimarySurface->lpVtbl->Release(lpDDPrimarySurface);
    /* backbuffer is released automatically when in fullscreen */
    if(!Fullscreen)
        lpDDBackBuffer->lpVtbl->Release(lpDDBackBuffer);
    lpDD->lpVtbl->Release(lpDD);

return TRUE;
}


VOID DDUpdateFrame(LPDIRECTDRAWSURFACE lpDDPrimarySurface ,LPDIRECTDRAWSURFACE lpDDBackBuffer, BOOL Fullscreen, INT *posX, INT *posY, INT *gainX, INT *gainY, RECT *rectDD)
{
    HDC hdc;
    DDBLTFX DDBlitFx;

    /* clear back buffer and paint it black */
    ZeroMemory(&DDBlitFx, sizeof(DDBlitFx));
    DDBlitFx.dwSize = sizeof(DDBlitFx);
    DDBlitFx.dwFillColor = 0;

    lpDDBackBuffer->lpVtbl->Blt(lpDDBackBuffer, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &DDBlitFx);

    if (lpDDBackBuffer->lpVtbl->GetDC(lpDDBackBuffer, &hdc) == DD_OK)
    {
        RECT rct;
        HBRUSH WhiteBrush;

        rct.left = *posX;
        rct.right = *posX+DD_SQUARE_SIZE;
        rct.top = *posY;
        rct.bottom = *posY+DD_SQUARE_SIZE;

        WhiteBrush = CreateSolidBrush(RGB(255,255,255));
        FillRect(hdc, &rct, WhiteBrush);

        if(*posX >= (DD_TEST_WIDTH - DD_SQUARE_SIZE)) *gainX = -(*gainX);
        if(*posY >= (DD_TEST_HEIGHT - DD_SQUARE_SIZE)) *gainY = -(*gainY);
        if(*posX < 0) *gainX = -1*(*gainX);
        if(*posY < 0) *gainY = -1*(*gainY);

        *posX += *gainX;
        *posY += *gainY;

        lpDDBackBuffer->lpVtbl->ReleaseDC(lpDDBackBuffer, hdc);

        if(Fullscreen)
        {
            lpDDPrimarySurface->lpVtbl->Flip(lpDDPrimarySurface, NULL, DDFLIP_WAIT);
        }
        else
        {
            lpDDPrimarySurface->lpVtbl->Blt(lpDDPrimarySurface, rectDD, lpDDBackBuffer, NULL, DDBLT_WAIT, NULL);
        }
    }
}
