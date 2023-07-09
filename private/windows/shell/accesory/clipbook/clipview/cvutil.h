
/*****************************************************************************

                    C L I P B O O K   U T I L I T I E S

    Name:       cvutil.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for cvutil.c

*****************************************************************************/




extern  DWORD   gXERR_Type;
extern  DWORD   gXERR_Err;

extern  HSZ     hszErrorRequest;




VOID AdjustControlSizes(
    HWND    hwnd);


VOID ShowHideControls(
    HWND    hwnd);


BOOL AssertConnection(
    HWND    hwnd);


HCONV InitSysConv(
    HWND    hwnd,
    HSZ     hszApp,
    HSZ     hszTopic,
    BOOL    fLocal);


BOOL UpdateListBox(
    HWND    hwnd,
    HCONV   hConv);


BOOL GetPreviewBitmap(
    HWND    hwnd,
    LPTSTR  szName,
    UINT    index);


VOID SetBitmapToListboxEntry(
    HDDEDATA    hbmp,
    HWND        hwndList,
    UINT        index);


BOOL InitListBox(
    HWND        hwnd,
    HDDEDATA    hData);


UINT MyGetFormat(
    LPTSTR  szFmt,
    int     mode);


VOID HandleOwnerDraw(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);


HWND CreateNewListBox(
    HWND    hwnd,
    DWORD   style);


BOOL SetClipboardFormatFromDDE(
    HWND     hwnd,
    UINT     uiFmt,
    HDDEDATA hDDE);


HWND  NewWindow(void);


VOID AdjustMDIClientSize(void);


HDDEDATA GetConvDataItem(
    HWND    hwnd,
    LPTSTR  szTopic,
    LPTSTR  szItem,
    UINT    uiFmt);


LRESULT  PASCAL MyMsgFilterProc(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam);


HDDEDATA MySyncXact(
    LPBYTE  lpbData,
    DWORD   cbDataLen,
    HCONV   hConv,
    HSZ     hszItem,
    UINT    wFmt,
    UINT    wType,
    DWORD   dwTimeout,
    LPDWORD lpdwResult);


void    RequestXactError(
    HCONV   hConv);


VOID ResetScrollInfo(
    HWND    hwnd);


BOOL IsShared(
    LPLISTENTRY lpLE);


BOOL SetShared(
    LPLISTENTRY lpLE,
    BOOL        fShared);


BOOL LockApp(
    BOOL    fLock,
    LPTSTR  lpszComment);


BOOL ForceRenderAll(
    HWND        hwnd,
    PVCLPBRD    pVclp);


BOOL UpdateNofMStatus(
    HWND    hwnd);


BOOL RestoreAllSavedConnections(void);


BOOL CreateNewRemoteWindow(
    LPTSTR  szMachineName,
    BOOL    fReconnect);


int MessageBoxID(
    HANDLE  hInstance,
    HWND    hwndParent,
    UINT    TextID,
    UINT    TitleID,
    UINT    fuStyle);


int NDdeMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    errCode,
    UINT    TitleID,
    UINT    fuStyle);


int SysMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    DWORD   dwErr,
    UINT    TitleID,
    UINT    fuStyle);


int XactMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    TitleID,
    UINT    fuStyle);


int DdeMessageBox(
    HANDLE  hInstance,
    HWND    hwnd,
    UINT    errCode,
    UINT    TitleID,
    UINT    fuStyle);


void    ClearInput (HWND hWnd);


PDATAREQ CreateNewDataReq(void);


BOOL DeleteDataReq(
    PDATAREQ    pDataReq);


BOOL ProcessDataReq(
    HDDEDATA    hData,
    PDATAREQ    pDataReq);
