
/*****************************************************************************

                        D E B U G   H E A D E R

    Name:       debug.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for the debug.c

*****************************************************************************/




LRESULT EditPermissions(
    BOOL    fSacl);


DWORD CALLBACK SedCallback(
    HWND                 hwndParent,
    HANDLE               hInstance,
    ULONG                penvstr,
    PSECURITY_DESCRIPTOR SecDesc,
    PSECURITY_DESCRIPTOR SecDescNewObjects,
    BOOLEAN              ApplyToSubContainers,
    BOOLEAN              ApplyToSubObjects,
    LPDWORD              StatusReturn);


LRESULT EditOwner(void);


LRESULT OnIDMKeep (
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);
