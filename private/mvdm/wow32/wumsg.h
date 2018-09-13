/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUMSG.H
 *  WOW32 16-bit User Message API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/

#define WOWDDE_POSTMESSAGE TRUE

extern BOOL fWhoCalled;

ULONG FASTCALL   WU32CallMsgFilter(PVDMFRAME pFrame);
ULONG FASTCALL   WU32CallWindowProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DefDlgProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DefFrameProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DefMDIChildProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DefWindowProc(PVDMFRAME pFrame);
ULONG FASTCALL   WU32DispatchMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetMessagePos(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetMessageTime(PVDMFRAME pFrame);
ULONG FASTCALL   WU32InSendMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32PeekMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32PostAppMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32PostMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32PostQuitMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32RegisterWindowMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32ReplyMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SendDlgItemMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SendMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32TranslateAccelerator(PVDMFRAME pFrame);
ULONG FASTCALL   WU32TranslateMDISysAccel(PVDMFRAME pFrame);
ULONG FASTCALL   WU32TranslateMessage(PVDMFRAME pFrame);
ULONG FASTCALL   WU32WaitMessage(PVDMFRAME pFrame);

VOID    SetFakeDialogClass(HWND hwnd);
