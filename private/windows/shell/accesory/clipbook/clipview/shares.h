
/*****************************************************************************

                            S H A R E S   H E A D E R

    Name:       shares.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for shares.c

*****************************************************************************/


#if DEBUG

void DumpDdeInfo(
    PNDDESHAREINFO  pDdeI,
    LPTSTR          lpszServer);

#else
#define DumpDdeInfo(x,y)
#endif


LRESULT EditPermissions(
    BOOL    fSacl);


BOOL WINAPI EditPermissions2(
    HWND        hWnd,
    LPTSTR      pShareName,
    BOOL        fSacl);;


LRESULT EditOwner(void);


LRESULT Properties(
    HWND        hwnd,
    PLISTENTRY  lpLE);
