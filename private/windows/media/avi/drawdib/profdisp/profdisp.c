#include <windows.h>
#include "ntavi.h"
#include <vfw.h>

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
#else
int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR szCmdLine, WORD sw)
#endif
{
    DrawDibProfileDisplay(NULL);

    return 0;
}
