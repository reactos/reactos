#pragma once

// Win: ValidateHmenu
FORCEINLINE PMENU UserGetMenuObject(HMENU hMenu)
{
   PMENU pMenu = UserGetObject(gHandleTable, hMenu, TYPE_MENU);
   if (!pMenu)
   {
      EngSetLastError(ERROR_INVALID_MENU_HANDLE);
   }
   return pMenu;
}

#define ASSERT_REFS_CO(_obj_) \
{ \
   LONG ref = ((PHEAD)_obj_)->cLockObj;\
   if (!(ref >= 1)){ \
      ERR_CH(UserObj, "ASSERT: obj 0x%p, refs %ld\n", _obj_, ref); \
      ASSERT(FALSE); \
   } \
}

#if 0
#define ASSERT_REFS_CO(_obj_) \
{ \
   PSINGLE_LIST_ENTRY e; \
   BOOL gotit=FALSE; \
   LONG ref = ((PHEAD)_obj_)->cLockObj;\
   if (!(ref >= 1)){ \
      ERR_CH(UserObj, "obj 0x%p, refs %i\n", _obj_, ref); \
      ASSERT(FALSE); \
   } \
   \
   e = PsGetCurrentThreadWin32Thread()->ReferencesList.Next; \
   while (e) \
   { \
      PUSER_REFERENCE_ENTRY ref = CONTAINING_RECORD(e, USER_REFERENCE_ENTRY, Entry); \
      if (ref->obj == _obj_){ gotit=TRUE; break; } \
      e = e->Next; \
   } \
   ASSERT(gotit); \
}
#endif

#define DUMP_REFS(obj) TRACE_CH(UserObj,"obj 0x%x, refs %i\n",obj, ((PHEAD)obj)->cLockObj)

PWND FASTCALL IntGetWindowObject(HWND hWnd);

/*************** DDE.C ****************/

BOOL FASTCALL IntDdeSendMessageHook(PWND,UINT,WPARAM,LPARAM);
BOOL APIENTRY IntDdePostMessageHook(IN PWND,IN UINT,IN WPARAM,IN OUT LPARAM*,IN OUT LONG_PTR*);
BOOL APIENTRY IntDdeGetMessageHook(PMSG,LONG_PTR);

/*************** MAIN.C ***************/

NTSTATUS NTAPI InitThreadCallback(PETHREAD Thread);

/*************** WINSTA.C ***************/

HWINSTA FASTCALL UserGetProcessWindowStation(VOID);

/*************** FOCUS.C ***************/

HWND FASTCALL UserGetActiveWindow(VOID);

HWND FASTCALL UserGetForegroundWindow(VOID);

HWND FASTCALL co_UserSetFocus(PWND Window);

/*************** WINDC.C ***************/

INT FASTCALL UserReleaseDC(PWND Window, HDC hDc, BOOL EndPaint);
HDC FASTCALL UserGetDCEx(PWND Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);
HDC FASTCALL UserGetWindowDC(PWND Wnd);

/*************** SESSION.C ***************/

extern PRTL_ATOM_TABLE gAtomTable;
NTSTATUS FASTCALL InitSessionImpl(VOID);

/*************** METRIC.C ***************/

BOOL NTAPI InitMetrics(VOID);
LONG NTAPI UserGetSystemMetrics(ULONG Index);
BOOL FASTCALL UserIsDBCSEnabled(VOID);
BOOL FASTCALL UserIsIMMEnabled(VOID);
BOOL FASTCALL UserIsCiceroEnabled(VOID);

/*************** KEYBOARD.C ***************/

DWORD FASTCALL UserGetKeyState(DWORD key);
DWORD FASTCALL UserGetKeyboardType(DWORD TypeFlag);
HKL FASTCALL UserGetKeyboardLayout(DWORD dwThreadId);


/*************** MISC.C ***************/

int
__cdecl
_scwprintf(
    const wchar_t *format,
    ...);

BOOL FASTCALL
UserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);

VOID FASTCALL IntSetWindowState(PWND, UINT);
VOID FASTCALL IntClearWindowState(PWND, UINT);
PTHREADINFO FASTCALL IntTID2PTI(HANDLE);
HBRUSH FASTCALL GetControlBrush(PWND,HDC,UINT);
HBRUSH FASTCALL GetControlColor(PWND,PWND,HDC,UINT);

NTSTATUS
GetProcessLuid(
    IN PETHREAD Thread OPTIONAL,
    IN PEPROCESS Process OPTIONAL,
    OUT PLUID Luid);

/*************** MESSAGE.C ***************/

BOOL FASTCALL UserPostMessage(HWND Wnd,UINT Msg, WPARAM wParam, LPARAM lParam);

/*************** WINDOW.C ***************/

PWND FASTCALL UserGetWindowObject(HWND hWnd);
VOID FASTCALL co_DestroyThreadWindows(struct _ETHREAD *Thread);
HWND FASTCALL UserGetShellWindow(VOID);
HDC FASTCALL UserGetDCEx(PWND Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);
BOOLEAN co_UserDestroyWindow(PVOID Object);
PWND FASTCALL UserGetAncestor(PWND Wnd, UINT Type);
BOOL APIENTRY DefSetText(PWND Wnd, PCWSTR WindowText);
DWORD FASTCALL IntGetWindowContextHelpId( PWND pWnd );

/*************** MENU.C ***************/

HMENU FASTCALL UserCreateMenu(PDESKTOP Desktop, BOOL PopupMenu);
BOOL FASTCALL UserSetMenuDefaultItem(PMENU Menu, UINT uItem, UINT fByPos);
BOOL FASTCALL UserDestroyMenu(HMENU hMenu);

/************** NONCLIENT **************/

VOID FASTCALL DefWndDoSizeMove(PWND pwnd, WORD wParam);
LRESULT NC_DoNCPaint(PWND,HDC,INT);
void FASTCALL NC_GetSysPopupPos(PWND, RECT *);
LRESULT NC_HandleNCActivate( PWND Wnd, WPARAM wParam, LPARAM lParam );
LRESULT NC_HandleNCCalcSize( PWND wnd, WPARAM wparam, RECTL *winRect, BOOL Suspended );
VOID NC_DrawFrame( HDC hDC, RECT *CurrentRect, BOOL Active, DWORD Style, DWORD ExStyle);
VOID UserDrawCaptionBar( PWND pWnd, HDC hDC, INT Flags);
void FASTCALL NC_GetInsideRect(PWND Wnd, RECT *rect);
LRESULT NC_HandleNCLButtonDown(PWND Wnd, WPARAM wParam, LPARAM lParam);
LRESULT NC_HandleNCLButtonDblClk(PWND Wnd, WPARAM wParam, LPARAM lParam);
LRESULT NC_HandleNCRButtonDown( PWND wnd, WPARAM wParam, LPARAM lParam );

/************** DEFWND **************/

HBRUSH FASTCALL DefWndControlColor(HDC hDC,UINT ctlType);
BOOL UserDrawSysMenuButton(PWND pWnd, HDC hDC, LPRECT Rect, BOOL Down);
BOOL UserPaintCaption(PWND pWnd, INT Flags);
PWND FASTCALL DWP_GetEnabledPopup(PWND pWnd);

/************** LAYERED **************/

BOOL FASTCALL SetLayeredStatus(PWND pWnd, BYTE set);
BOOL FASTCALL GetLayeredStatus(PWND pWnd);

/************** INPUT CONTEXT **************/

PIMC FASTCALL UserCreateInputContext(ULONG_PTR dwClientImcData);
VOID UserFreeInputContext(PVOID Object);
BOOLEAN UserDestroyInputContext(PVOID Object);
PVOID AllocInputContextObject(PDESKTOP pDesk, PTHREADINFO pti, SIZE_T Size, PVOID* HandleOwner);

/* EOF */
