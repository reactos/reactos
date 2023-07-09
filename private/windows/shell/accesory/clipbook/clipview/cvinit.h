

/*****************************************************************************

                    C V I N I T   H E A D E R

    Name:       cvinit.h
    Date:       20-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for cvinit.c

*****************************************************************************/



extern  HWND        hwndToolbar;
extern  HWND        hwndStatus;
extern  HBITMAP     hbmStatus;

extern  TCHAR       szWindows[];

extern  DWORD       nIDs[];
extern  TBBUTTON    tbButtons[];












VOID LoadIntlStrings (void);


VOID SaveWindowPlacement (
    PWINDOWPLACEMENT    pwp);


BOOL ReadWindowPlacement(
    LPTSTR              szKey,
    PWINDOWPLACEMENT    pwp);


BOOL CreateTools(
    HWND    hwnd);


VOID DeleteTools(
    HWND    hwnd);
