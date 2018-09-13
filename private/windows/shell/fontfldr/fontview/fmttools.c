#include <windows.h>
#include <fontdefs.h>
#include <fvmsg.h>

TCHAR gpszUnknownError[MAX_PATH] = TEXT("Error");
TCHAR gszDots[] = TEXT("...");
/***************************************************************************\
*
*     FUNCTION: FmtMessageBox( HWND hwnd, DWORD dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ... );
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*
*
*
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int FmtMessageBox( HWND hwnd, DWORD dwTitleID, LPTSTR pszTitleStr,
    UINT fuStyle, BOOL fSound, DWORD dwTextID, ... ) {
    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start( marker, dwTextID );

    if(!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK, hInst,
            dwTextID, 0, (LPTSTR)&pszMsg, 1, &marker))
        pszMsg = gpszUnknownError;

    va_end( marker );

    GetLastError();

    if (dwTitleID != FMB_TTL_ERROR ||
            !FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK |
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                hInst, dwTitleID, 0, (LPTSTR)&pszTitle, 1, (va_list *)&pszTitleStr)) {
        pszTitle = NULL;
    }

    GetLastError();

    if (fSound) {
        MessageBeep( fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK) );
    }

    if (hwnd == NULL)
        hwnd = GetDesktopWindow();

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        FmtFree( pszTitle );

    if (pszMsg != gpszUnknownError)
        FmtFree( pszMsg );

    return idRet;
}

/***************************************************************************\
*
*     FUNCTION: FmtSprintf( DWORD id, ... );
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
LPTSTR FmtSprintf( DWORD id, ... ) {
    LPTSTR pszMsg;
    va_list marker;

    va_start( marker, id );

    if(!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK, hInst,
            id, 0, (LPTSTR)&pszMsg, 1, &marker)) {
        GetLastError();
        pszMsg = gszDots;
    }
    va_end( marker );

    return pszMsg;
}

/***************************************************************************\
*
*     FUNCTION: PVOID AllocMem( DWORD cb );
*
*     PURPOSE:  allocates memory, checking for errors
*
*   Do not call this function until after LoadFontFile() has been called
*   since this function will try and remove the font.
*
* History:
*   22-Apr-1993 JonPa   Wrote it.
\***************************************************************************/
PVOID AllocMem( DWORD cb ) {
    PVOID pv = (PVOID)LocalAlloc(LPTR, cb);

    if (pv == NULL) {
        FmtMessageBox( ghwndFrame, FMB_TTL_ERROR, NULL, MB_OK | MB_ICONSTOP,
                TRUE, MSG_OUTOFMEM );
        RemoveFontResource( gszFontPath );
        ExitProcess(2);
    }

    return pv;
}

#ifdef FV_DEBUG
/***************************************************************************\
*
*     FUNCTION: FmtSprintf( DWORD id, ... );
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
void Dprintf( LPTSTR pszFmt, ... ) {
    TCHAR szBuffer[256];
    va_list marker;

    va_start( marker, pszFmt );

    wvsprintf( szBuffer, pszFmt, marker );
    OutputDebugString(szBuffer);

    va_end( marker );

}
#endif
