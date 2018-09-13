
//
// Unicode Common Control stubs for Win95
//

#include "ctlspriv.h"
#include <prsht.h>


void WINAPI DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR pszText, UINT uFlags)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return;
}

HWND WINAPI CreateStatusWindowW(LONG style, LPCWSTR pszText, HWND hwndParent, UINT uID)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


int WINAPI PropertySheetW(LPCPROPSHEETHEADER ppsh)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
}

HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGE psp)
{
    SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}

HIMAGELIST WINAPI ImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
   SetLastErrorEx(SLE_WARNING, ERROR_CALL_NOT_IMPLEMENTED);
   return NULL;
}
