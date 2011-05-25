#pragma once

#include <windows.h>
#include "resource.h"

typedef struct _INFO
{
    HINSTANCE hInstance;
    INT cx;
    INT cy;

} INFO, *PINFO;

extern PINFO g_pInfo;



HDC NT5_DrawBaseBackground(HDC hdcDesktop);
VOID NT5_CreateLogoffScreen(LPWSTR lpText, HDC hdcMem);
VOID NT5_RefreshLogoffScreenText(LPWSTR lpText, HDC hdcMem);
