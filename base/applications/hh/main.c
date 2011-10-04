#include <windows.h>

typedef int WINAPI DOWINMAIN(HMODULE hMod, LPSTR cmdline);

int WINAPI
WinMain(HINSTANCE hInst,
        HINSTANCE hPrevInst,
        LPSTR cmdline,
        int cmdshow)
{
    HMODULE hModule;
    DOWINMAIN *doWinMain;
    int ret = -1;

    hModule = LoadLibraryA("hhctrl.ocx");
    doWinMain = (DOWINMAIN*) GetProcAddress(hModule, "doWinMain");

    ret = doWinMain(hInst, cmdline);

    FreeLibrary(hModule);

    return ret;
}
