#include "shprv.h"

//
//  simple form of Shell message box, does not handle param replacment
//  just calls LoadString and MessageBox
//
int WINAPI ShellMessageBox(HINSTANCE hAppInst, HWND hWnd, LPCSTR lpcText, LPCSTR lpcTitle, UINT fuStyle)
{
    char    achText[256];
    char    achTitle[80];

    if (HIWORD(lpcText) == 0)
    {
        LoadString(hAppInst, LOWORD(lpcText), achText, sizeof(achText));
        lpcText = (LPCSTR)achText;
    }

    if (HIWORD(lpcTitle) == 0)
    {
        if (LOWORD(lpcTitle) == 0)
            GetWindowText(hWnd, achTitle, sizeof(achTitle));
        else
            LoadString(hAppInst, LOWORD(lpcTitle), achTitle, sizeof(achTitle));

        lpcTitle = (LPCSTR)achTitle;
    }

    return MessageBox(hWnd, lpcText, lpcTitle, fuStyle);
}
