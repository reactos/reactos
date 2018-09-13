/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDDEML.H
 *  WOW32 16-bit DDEML API support
 *
 *  History:
 *  Created Jan-23-1993 by Chandan Chauhan (ChandanC)
--*/

#define MAX_CONVS   3200

typedef struct _BIND1632 {
    DWORD  x16;
    DWORD  x32;
} BIND1632;

ULONG FASTCALL WD32DdeInitialize(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeUninitialize(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeConnectList(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeQueryNextServer(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeDisconnectList(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeConnect(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeDisconnect(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeQueryConvInfo(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeSetUserHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeClientTransaction(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeAbandonTransaction(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdePostAdvise(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeCreateDataHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeAddData(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeGetData(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeAccessData(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeUnaccessData(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeFreeDataHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeGetLastError(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeCreateStringHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeFreeStringHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeQueryString(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeKeepStringHandle(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeEnableCallback(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeNameService(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeCmpStringHandles(PVDMFRAME pFrame);
ULONG FASTCALL WD32DdeReconnect(PVDMFRAME pFrame);
HDDEDATA W32DdemlCallBack(UINT type, UINT fmt, HCONV hconv, HSZ hsz1,
                        HSZ hsz2, HDDEDATA hData, DWORD dwData1,
                        DWORD dwData2);
VOID WOWDdemlBind (DWORD x16, DWORD x32, BIND1632 aBind[]);
VOID WOWDdemlUnBind (DWORD x32, BIND1632 aBind[]);
DWORD WOWDdemlGetBind16 (DWORD x32, BIND1632 aBind[]);
DWORD WOWDdemlGetBind32 (DWORD x16, BIND1632 aBind[]);
BOOL DdeDataBuf16to32(PVOID p16DdeData, LPBYTE *pp32DdeData, PDWORD pcbData,
        PDWORD pcbOffset, UINT format);
BOOL DdeDataBuf32to16(PVOID p16DdeData, PVOID p32DdeData, DWORD cbData,
        DWORD cbOffset, UINT format);
VOID DdeDataSize16to32(DWORD *pcbData, DWORD *pcbOff, UINT format);
VOID DdeDataSize32to16(DWORD *pcbData, DWORD *pcbOff, UINT format);
VOID W32GetConvContext (VPVOID vp, PCONVCONTEXT pCC32);
VOID W32PutConvContext (VPVOID vp, PCONVCONTEXT pCC32);

/*
 * Imports from user32.dll - need to eventually be moved to winuserp.h
 * or ddemlp.h
 */
BOOL DdeIsDataHandleInitialized(HDDEDATA hData);
