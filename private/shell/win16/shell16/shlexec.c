#include "shprv.h"

#define IMECOMPAT_USEALTSTKFORSHLEXEC   0x00008000      // ;Internal

#define ALT_STACK 0x5000


typedef struct tagSHLEXECPARAM {
    HWND hwnd;
    LPSTR lpsz[4];
    int wShowCmd;
} SHLEXECPARAM, *PSHLEXECPARAM, FAR *LPSHLEXECPARAM;

typedef DWORD (WINAPI *LPFNIMMGETAPPCOMPATFLAGS)(DWORD);

HANDLE WINAPI ShellExecute32( HWND hwnd, LPSTR lpszOp, LPSTR lpszFile, LPSTR lpszParams, LPSTR lpszDir, int wShowCmd);


/*--------------------------------------------------------------------------
 *
 * ShellExecute
 *
 * ShellExecute of SHELL32.DLL eats lots of stack under 16 bit applciation.
 * Because ATOK9 does not have enough stack, we need to prepare altanative 
 * stack to call WIn32 ShellExecute.
 *
 * GACF_INCREASE does not work for ATOK9, because its _DATA is too big and
 * there is no rool to increase Stack.
 *
 *------------------------------------------------------------------------*/

HANDLE WINAPI ShellExecute( HWND hwnd, LPCSTR lpszOp, LPCSTR lpszFile, LPCSTR lpszParams, LPCSTR lpszDir, int wShowCmd)

{

    UINT os[4];
    UINT uLen = 0;
    int i;
    HANDLE hIMM;
    static HANDLE hRet;
    static LPSHLEXECPARAM lpsep;
    static LPFNIMMGETAPPCOMPATFLAGS lpfnImmGetAppCompatFlag = NULL;

    if (!lpfnImmGetAppCompatFlag)
    {
        if (hIMM = GetModuleHandle("IMM"))
            lpfnImmGetAppCompatFlag = (LPFNIMMGETAPPCOMPATFLAGS)GetProcAddress(hIMM,MAKEINTRESOURCE(146));
    }

    if (!lpfnImmGetAppCompatFlag ||
        !((*lpfnImmGetAppCompatFlag)(0) & IMECOMPAT_USEALTSTKFORSHLEXEC))
        return ShellExecute32(hwnd, (LPSTR)lpszOp, (LPSTR)lpszFile, 
                              (LPSTR)lpszParams, (LPSTR)lpszDir, wShowCmd);

    for (i = 0; i < 4; i ++)
        os[i] = 0xFFFF;

    if (lpszOp )
    {
        os[0] = 0;
        uLen += (lstrlen(lpszOp) + 5);
        uLen &= 0xFFFC;
    }

    if (lpszFile )
    {
        os[1] = uLen;
        uLen += (lstrlen(lpszFile) + 5);
        uLen &= 0xFFFC;
    }

    if (lpszParams )
    {
        os[2] = uLen;
        uLen += (lstrlen(lpszParams) + 5);
        uLen &= 0xFFFC;
    }

    if (lpszDir )
    {
        os[3] = uLen;
        uLen += (lstrlen(lpszDir) + 5);
        uLen &= 0xFFFC;
    }

    if (lpsep = MAKELP(GlobalAlloc(GPTR, ALT_STACK + uLen + sizeof(SHLEXECPARAM)),0))
    {
        LPSTR lpTemp;
        (LPSTR)lpsep += ALT_STACK;
        lpTemp = (LPSTR)lpsep + sizeof(SHLEXECPARAM);

        for (i = 0; i < 4; i ++)
        {
            if (os[i] != 0xFFFF)
                lpsep->lpsz[i] = (LPSTR)(lpTemp + os[i]);
            else
                lpsep->lpsz[i] = NULL;
        }

        lpsep->hwnd = hwnd;
        lpsep->wShowCmd = wShowCmd;

        if (lpsep->lpsz[0])
            lstrcpy(lpsep->lpsz[0], lpszOp);
        if (lpsep->lpsz[1])
            lstrcpy(lpsep->lpsz[1], lpszFile);
        if (lpsep->lpsz[2])
            lstrcpy(lpsep->lpsz[2], lpszParams);
        if (lpsep->lpsz[3])
            lstrcpy(lpsep->lpsz[3], lpszDir);

        SwitchStackTo(SELECTOROF(lpsep), ALT_STACK - 1, 0);

        hRet =  ShellExecute32(lpsep->hwnd, lpsep->lpsz[0], lpsep->lpsz[1], 
                               lpsep->lpsz[2], lpsep->lpsz[3], lpsep->wShowCmd);

        SwitchStackBack();
        GlobalFree(SELECTOROF(lpsep));
    }

    return hRet;
}
