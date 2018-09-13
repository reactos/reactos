/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1996  Microsoft Corporation

Module Name:

    vars.h

Abstract:

    Header file for the winsock browser util globals

Author:

    Dan Knudson (DanKn)    29-Jul-1996

Revision History:

--*/


#ifdef WIN32
#define my_far
#else
#define my_far _far
#endif

extern FILE        *hLogFile;
extern HANDLE      ghInst;
extern HWND        ghwndMain, ghwndEdit, ghwndList1, ghwndList2, ghwndList3;
extern BOOL        bShowParams;
extern BOOL        gbDisableHandleChecking;
extern LPBYTE      pBigBuf;
extern DWORD       dwBigBufSize;
extern BOOL        bDumpParams;
extern BOOL        gbWideStringParams;
extern BOOL        bTimeStamp;
extern DWORD       dwDumpStructsFlags;

extern BOOL        gbWideStringParams;

extern DWORD       aUserButtonFuncs[MAX_USER_BUTTONS];
extern char        aUserButtonsText[MAX_USER_BUTTONS][MAX_USER_BUTTON_TEXT_SIZE];

extern char my_far szTab[];

extern char        aAscii[];
extern BYTE        aHex[];

extern LOOKUP      aAddressFamilies[];
extern LOOKUP      aIoctlCmds[];
extern LOOKUP      aJLFlags[];
extern LOOKUP      aNameSpaces[];
extern LOOKUP      aNetworkByteOrders[];
extern LOOKUP      aNetworkEvents[];
extern LOOKUP      aProperties[];
extern LOOKUP      aProtocols[];
extern LOOKUP      aProviderFlags[];
extern LOOKUP      aQOSServiceTypes[];
extern LOOKUP      aRecvFlags[];
extern LOOKUP      aResDisplayTypes[];
extern LOOKUP      aResFlags[];
extern LOOKUP      aSendFlags[];
extern LOOKUP      aWSASendAndRecvFlags[];
extern LOOKUP      aShutdownOps[];
extern LOOKUP      aSocketTypes[];
extern LOOKUP      aSockOptLevels[];
extern LOOKUP      aSockOpts[];
extern LOOKUP      aServiceFlags[];
extern LOOKUP      aServiceOps[];
extern LOOKUP      aSvcFlags[];
extern LOOKUP      aWSAFlags[];
extern LOOKUP      aWSAIoctlCmds[];
extern LOOKUP      aWSARecvFlags[];
extern LOOKUP      aWSASendFlags[];
extern LOOKUP      aWSAErrors[];
extern char        *aFuncNames[];
