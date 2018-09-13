/*****************************************************************************
 *
 *    ftpinet.cpp - Interfacing to WinINet
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpinet.h"

#define SHInterlockedCompareExchangePointer SHInterlockedCompareExchange


/*****************************************************************************
 *
 *    Const strings for our Wininet stuff.
 *
 *****************************************************************************/

HINSTANCE g_hinstWininet = NULL;    /* The DLL handle */
HINTERNET g_hint = NULL;        /* Shared internet anchor handle */

#define SZ_WININET_AGENT TEXT("Microsoft(r) Windows(tm) FTP Folder")


/*****************************************************************************\
    FUNCTION: InitWininet
\*****************************************************************************/
void InitWininet(void)
{
    // You can't use a critical section around LoadLibrary().
    ASSERTNONCRITICAL;

    if (!g_hinstWininet)
    {
        HINSTANCE hinstTemp = LoadLibrary(TEXT("WININET.DLL"));

        if (EVAL(hinstTemp))
        {
            // Can we successfully put it here?
            if (SHInterlockedCompareExchangePointer((void **)&g_hinstWininet, hinstTemp, NULL))
            {
                // No, someone else beat us there.
                ASSERT(g_hinstWininet);
                FreeLibrary(hinstTemp);
            }
        }
    }

    if (EVAL(g_hinstWininet))
    {
        if (!g_hint)
        {
            HINTERNET hinternetTemp;

            EVAL(SUCCEEDED(InternetOpenWrap(TRUE, SZ_WININET_AGENT, PRE_CONFIG_INTERNET_ACCESS, 0, 0, 0, &hinternetTemp)));
            if (EVAL(hinternetTemp))
            {
                // Can we successfully put it here?
                if (SHInterlockedCompareExchangePointer((void **)&g_hint, hinternetTemp, NULL))
                {
                    // No, someone else beat us there.
                    ASSERT(g_hint);
                    InternetCloseHandle(hinternetTemp);
                }
            }
        }
    }
}


/*****************************************************************************\
    FUNCTION: UnloadWininet
\*****************************************************************************/
void UnloadWininet(void)
{
    // You can't use a critical section around FreeLibrary() (I think).
    ASSERTNONCRITICAL;

    if (g_hint)
    {
        HINTERNET hinternetTemp = InterlockedExchangePointer(&g_hint, NULL);

        if (hinternetTemp)
        {
            InternetCloseHandle(hinternetTemp);
        }
    }

/************************
//  BUGBUG: I want to unload wininet, I really do.  But this function is called
//          during process un-attach and it's better to leak wininet than to
//          call FreeLibrary() during process unattach.

    if (g_hinstWininet)
    {
        HINSTANCE hinstTemp = (HINSTANCE)InterlockedExchangePointer((void **) &g_hinstWininet, NULL);

        if (hinstTemp)
        {
            FreeLibrary(hinstTemp);
        }
    }
*********************/
}

/*****************************************************************************\
 *    hintShared
 *
 *    Obtain the shared internet handle that we use for all our stuff.
 *    We load WinINet only on demand, so that quick things will be quick.
 *    If this procedure fails, the reason can be obtained via GetLastError().
 *    (Note that this assumes that we always try to InitWininet().)
\*****************************************************************************/
HINTERNET GetWininetSessionHandle(void)
{
    //    Avoid taking the critical section unless you really need to.
    if (!g_hint)
    {
        InitWininet();
        ASSERT(g_hint);
    }
    return g_hint;
}

