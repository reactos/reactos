/*
 * PROJECT:         HTML Application Viewer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/mshta/main.c
 * PURPOSE:         HTML Application Host
 * PROGRAMMERS:     Jared Smudde (computerwhiz02@hotmail.com)
 */

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <mshtml.h>

extern DWORD WINAPI RunHTMLApplication(HINSTANCE hinst, HINSTANCE hPrevInst, LPSTR szCmdLine, int nCmdShow);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{
    return RunHTMLApplication(hInst, hPrevInst, cmdline, cmdshow);
}
