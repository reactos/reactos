/* desktop.cpp */

#include <desktopp.h>

#define WM_CHANGENOTIFY            (WM_USER + 42)

#define DTM_BANDFILE               (WM_USER + 200)

#define PROGMAN             TEXT("Program Manager")
#define DESKTOPCLASS        TEXT(STR_DESKTOPCLASS)
#define DESKTOPPROXYCLASS   TEXT("Proxy Desktop")

void FireEventSz(LPCTSTR pszEvent);

// #define BETA_WARNING
#ifdef BETA_WARNING
void    DoTimebomb(HWND hwnd)
#else
#define DoTimebomb(hwnd)
#endif


