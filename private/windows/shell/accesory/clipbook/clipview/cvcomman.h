
/******************************************************************************

                    C V C O M M A N D   H E A D E R

    Name:       cvcomman.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for cvcomman.c

******************************************************************************/



LRESULT OnIDMDelete(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wparam,
    LPARAM  lparam);


static void CreateClipboardWindow (void);


static void CreateLocalWindow (void);


void UnsharePage (void);


LRESULT OnIdmUnshare (DWORD dwItem);


LRESULT ClipBookCommand(
    HWND        hwnd,
    UINT        msg,
    WPARAM      wParam,
    LPARAM      lParam);


BOOL SetListboxEntryToPageWindow(
    HWND        hwndc,
    PMDIINFO    pMDIc,
    int         lbindex);
