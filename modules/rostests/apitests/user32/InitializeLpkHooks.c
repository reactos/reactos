/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for InitializeLpkHooks
 * PROGRAMMERS:     Magnus Olsen
 */

#include "precomp.h"

typedef struct _LPK_LPEDITCONTROL_LIST
{
    PVOID EditCreate;
    PVOID EditIchToXY;
    PVOID EditMouseToIch;
    PVOID EditCchInWidth;
    PVOID EditGetLineWidth;
    PVOID EditDrawText;
    PVOID EditHScroll;
    PVOID EditMoveSelection;
    PVOID EditVerifyText;
    PVOID EditNextWord;
    PVOID EditSetMenu;
    PVOID EditProcessMenu;
    PVOID EditCreateCaret;
    PVOID EditAdjustCaret;
} LPK_LPEDITCONTROL_LIST, *PLPK_LPEDITCONTROL_LIST;


static DWORD (APIENTRY *fpLpkTabbedTextOut) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID);
static DWORD (APIENTRY *fpLpkPSMTextOut) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID);
static DWORD (APIENTRY *fpLpkDrawTextEx) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID);
static PLPK_LPEDITCONTROL_LIST (APIENTRY *fpLpkEditControl) ();

static int Count_myLpkTabbedTextOut = 0;
static int Count_myLpkPSMTextOut = 0;
static int Count_myLpkDrawTextEx = 0;

DWORD WINAPI myLpkTabbedTextOut (LPVOID x1,LPVOID x2,LPVOID x3, LPVOID x4, LPVOID x5, LPVOID x6, LPVOID x7, LPVOID x8,
                                   LPVOID x9, LPVOID x10, LPVOID x11, LPVOID x12)
{
    Count_myLpkTabbedTextOut++;
    return fpLpkTabbedTextOut(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12);
}

DWORD myLpkPSMTextOut (LPVOID x1,LPVOID x2,LPVOID x3,LPVOID x4,LPVOID x5,LPVOID x6)
{
    Count_myLpkPSMTextOut++;
    return fpLpkPSMTextOut ( x1,  x2,  x3,  x4,  x5,  x6);
}

DWORD myLpkDrawTextEx(LPVOID x1,LPVOID x2,LPVOID x3,LPVOID x4,LPVOID x5, LPVOID x6, LPVOID x7, LPVOID x8, LPVOID x9,LPVOID x10)
{
    Count_myLpkDrawTextEx++;
    return fpLpkDrawTextEx(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10);
}


typedef struct _USER32_INTERN_INITIALIZEHOOKS
{
    PVOID fpLpkTabbedTextOut;
    PVOID fpLpkPSMTextOut;
    PVOID fpLpkDrawTextEx;
    PLPK_LPEDITCONTROL_LIST fpListLpkEditControl;
} USER32_INTERN_INITIALIZEHOOKS, *PUSER32_INTERN_INITIALIZEHOOKS;

VOID WINAPI InitializeLpkHooks (PUSER32_INTERN_INITIALIZEHOOKS);

void Test_InitializeLpkHooks()
{
    USER32_INTERN_INITIALIZEHOOKS setup;
    HMODULE lib = LoadLibrary("LPK.DLL");

    ok(lib != NULL, "lib = 0\n");
    if (lib != NULL)
    {
        fpLpkTabbedTextOut = (DWORD (APIENTRY *) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID, LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID)) GetProcAddress(lib, "LpkTabbedTextOut");
        fpLpkPSMTextOut = (DWORD (APIENTRY *) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID)) GetProcAddress(lib, "fpLpkPSMTextOut");
        fpLpkDrawTextEx = (DWORD (APIENTRY *) (LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID,LPVOID)) GetProcAddress(lib, "LpkDrawTextEx");
        fpLpkEditControl = (PLPK_LPEDITCONTROL_LIST (APIENTRY *) (VOID)) GetProcAddress(lib, "LpkEditControl");

        setup.fpLpkTabbedTextOut = myLpkTabbedTextOut;
        setup.fpLpkPSMTextOut = myLpkPSMTextOut;
        setup.fpLpkDrawTextEx = myLpkDrawTextEx;

        /* we have not add any test to this api */
        setup.fpListLpkEditControl = (PLPK_LPEDITCONTROL_LIST)fpLpkEditControl;

        /* use our own api that we just made */
        InitializeLpkHooks(&setup);

        /* FIXME add test now */

        /* restore */
        setup.fpLpkTabbedTextOut = fpLpkTabbedTextOut;
        setup.fpLpkPSMTextOut = fpLpkPSMTextOut;
        setup.fpLpkDrawTextEx = fpLpkDrawTextEx;
        setup.fpListLpkEditControl = (PLPK_LPEDITCONTROL_LIST)fpLpkEditControl;
        InitializeLpkHooks(&setup);
    }

}

START_TEST(InitializeLpkHooks)
{
    Test_InitializeLpkHooks();
}
