/*
 * PROJECT:     ReactOS HTML Application Host
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Forwards HTA application information to mshtml
 * COPYRIGHT:   Copyright 2017-2018 Jared Smudde(computerwhiz02@hotmail.com)
 */

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <mshtml.h>

extern DWORD WINAPI RunHTMLApplication(HINSTANCE hinst, HINSTANCE hPrevInst, LPSTR szCmdLine, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{
    return RunHTMLApplication(hInst, hPrevInst, cmdline, cmdshow);
}
