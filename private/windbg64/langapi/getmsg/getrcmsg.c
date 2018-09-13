// getrcmsg.c

#include "windows.h"

static HINSTANCE    hInstModule = NULL;

HINSTANCE
SetHInstance(HINSTANCE h)
{
    HINSTANCE   hRet = hInstModule;
    hInstModule = h;
    return hRet;
}

char  *
get_err(int msg_num)
{

    static char rgchErr[1024];

#if !defined(HARD_LINK)
    static HMODULE hmodUser32;
    static int (WINAPI *pfnLoadStringA)(HINSTANCE, UINT, LPSTR, int);

    if (hmodUser32 == NULL) {
        hmodUser32  = LoadLibrary("USER32.DLL");

        if (hmodUser32 == NULL) {
            return NULL;
        }
    }

    if (pfnLoadStringA == NULL) {
        pfnLoadStringA = (int (WINAPI *)(HINSTANCE, UINT, LPSTR, int))
                             GetProcAddress(hmodUser32, "LoadStringA");

        if (pfnLoadStringA == NULL) {
            return NULL;
        }
    }

    if ((*pfnLoadStringA)(hInstModule, msg_num, rgchErr, sizeof(rgchErr)) == 0) {
        rgchErr[0] = '\0';
    }
#else
#pragma comment(lib, "user32")

    if (LoadString(hInstModule, msg_num, rgchErr, sizeof(rgchErr)) == 0) {
        rgchErr[0] = '\0';
    }
#endif
    return rgchErr;
}

int
SetErrorFile(char *pFilename, char *pExeName, int fSearchExePath)
{
    return 1;
}
