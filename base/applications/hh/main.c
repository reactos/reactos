#include <stdarg.h>
#include <windef.h>
#include <winbase.h>

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
    if (hModule)
    {
        doWinMain = (DOWINMAIN*)GetProcAddress(hModule, "doWinMain");
        if (doWinMain)
            ret = doWinMain(hInst, cmdline);

        FreeLibrary(hModule);
    }

    return ret;
}
