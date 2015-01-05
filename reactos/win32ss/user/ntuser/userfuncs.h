#pragma once

FORCEINLINE PMENU UserGetMenuObject(HMENU hMenu)
{
   return UserGetObject(gHandleTable, hMenu, TYPE_MENU);
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
BOOL FASTCALL IntDdePostMessageHook(PWND,UINT,WPARAM,LPARAM);
VOID FASTCALL IntDdeGetMessageHook(PMSG);

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

/*************** KEYBOARD.C ***************/

DWORD FASTCALL UserGetKeyState(DWORD key);
DWORD FASTCALL UserGetKeyboardType(DWORD TypeFlag);
HKL FASTCALL UserGetKeyboardLayout(DWORD dwThreadId);


/*************** MISC.C ***************/

BOOL FASTCALL
UserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);

VOID FASTCALL IntSetWindowState(PWND, UINT);
VOID FASTCALL IntClearWindowState(PWND, UINT);
PTHREADINFO FASTCALL IntTID2PTI(HANDLE);

/*************** MESSAGE.C ***************/

BOOL FASTCALL
UserPostMessage(HWND Wnd,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam);

/*************** WINDOW.C ***************/

PWND FASTCALL UserGetWindowObject(HWND hWnd);
VOID FASTCALL co_DestroyThreadWindows(struct _ETHREAD *Thread);
HWND FASTCALL UserGetShellWindow(VOID);
HDC FASTCALL UserGetDCEx(PWND Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);
BOOLEAN co_UserDestroyWindow(PVOID Object);
PWND FASTCALL UserGetAncestor(PWND Wnd, UINT Type);

/*************** MENU.C ***************/

HMENU FASTCALL UserCreateMenu(PDESKTOP Desktop, BOOL PopupMenu);
BOOL FASTCALL UserSetMenuDefaultItem(PMENU Menu, UINT uItem, UINT fByPos);
BOOL FASTCALL UserDestroyMenu(HMENU hMenu);

/*************** SCROLLBAR.C ***************/

DWORD FASTCALL
co_UserShowScrollBar(PWND Wnd, int nBar, BOOL fShowH, BOOL fShowV);

/* EOF */
