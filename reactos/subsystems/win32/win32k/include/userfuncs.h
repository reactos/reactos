#ifndef _WIN32K_USERFUNCS_H
#define _WIN32K_USERFUNCS_H





PMENU_OBJECT FASTCALL UserGetMenuObject(HMENU hMenu);


#if 0
#define ObmDereferenceObject(_obj_) \
{ \
   DPRINT1("obj 0x%x dereffed to %i refs\n",_obj_, USER_BODY_TO_HEADER(_obj_)->RefCount-1); \
   ObmDereferenceObject2(_obj_); \
}

#endif

#define ObmDereferenceObject(_obj_) ObmDereferenceObject2(_obj_)





#define ASSERT_REFS_CO(_obj_) \
{ \
   LONG ref = USER_BODY_TO_HEADER(_obj_)->RefCount;\
   if (!(ref >= 1)){ \
      DPRINT1("ASSERT: obj 0x%x, refs %i\n", _obj_, ref); \
      ASSERT(FALSE); \
   } \
}

#if 0
#define ASSERT_REFS_CO(_obj_) \
{ \
   PSINGLE_LIST_ENTRY e; \
   BOOL gotit=FALSE; \
   LONG ref = USER_BODY_TO_HEADER(_obj_)->RefCount;\
   if (!(ref >= 1)){ \
      DPRINT1("obj 0x%x, refs %i\n", _obj_, ref); \
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

#define DUMP_REFS(obj) DPRINT1("obj 0x%x, refs %i\n",obj, USER_BODY_TO_HEADER(obj)->RefCount)




VOID FASTCALL ObmReferenceObject(PVOID obj);
BOOL FASTCALL ObmDereferenceObject2(PVOID obj);

PWINDOW_OBJECT FASTCALL IntGetWindowObject(HWND hWnd);
PVOID FASTCALL
ObmCreateObject(PUSER_HANDLE_TABLE ht, HANDLE* h,USER_OBJECT_TYPE type , ULONG size);

BOOL FASTCALL 
ObmDeleteObject(HANDLE h, USER_OBJECT_TYPE type );

#define UserRefObject(o) ObmReferenceObject(o)
#define UserDerefObject(o) ObmDereferenceObject(o)
BOOL FASTCALL ObmCreateHandleTable();

/******************** HANDLE.C ***************/

extern USER_HANDLE_TABLE gHandleTable;

PUSER_HANDLE_ENTRY handle_to_entry(PUSER_HANDLE_TABLE ht, HANDLE handle );
VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes);
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
PVOID UserFreeHandle(PUSER_HANDLE_TABLE ht, HANDLE handle );
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type );

/*************** WINSTA.C ***************/

HWINSTA FASTCALL UserGetProcessWindowStation(VOID);

/*************** INPUT.C ***************/

NTSTATUS FASTCALL
UserAcquireOrReleaseInputOwnership(BOOLEAN Release);

/*************** WINPOS.C ***************/

BOOL FASTCALL
UserGetClientOrigin(PWINDOW_OBJECT Window, LPPOINT Point);

/*************** FOCUS.C ***************/

HWND FASTCALL UserGetActiveWindow();

HWND FASTCALL UserGetForegroundWindow(VOID);

HWND FASTCALL co_UserSetFocus(PWINDOW_OBJECT Window);

/*************** WINDC.C ***************/

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Window, HDC hDc, BOOL EndPaint);

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd);


/*************** SESSION.C ***************/

extern PRTL_ATOM_TABLE gAtomTable;

NTSTATUS FASTCALL InitSessionImpl(VOID);

/*************** METRIC.C ***************/

ULONG FASTCALL
UserGetSystemMetrics(ULONG Index);

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
  
/*************** MESSAGE.C ***************/
  
BOOL FASTCALL
UserPostMessage(HWND Wnd,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam); 



/*************** PAINTING.C ***************/

BOOL FASTCALL co_UserValidateRgn(PWINDOW_OBJECT Window, HRGN hRgn);


/*************** WINDOW.C ***************/

PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd);

VOID FASTCALL
co_DestroyThreadWindows(struct _ETHREAD *Thread);

HWND FASTCALL UserGetShellWindow();

HWND FASTCALL UserGetWindow(HWND hWnd, UINT Relationship);

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

BOOLEAN FASTCALL co_UserDestroyWindow(PWINDOW_OBJECT Wnd);

LONG FASTCALL UserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi);

PWINDOW_OBJECT FASTCALL UserGetAncestor(PWINDOW_OBJECT Wnd, UINT Type);

/*************** MENU.C ***************/

HMENU FASTCALL UserCreateMenu(BOOL PopupMenu);

BOOL FASTCALL
UserSetMenuDefaultItem(
  PMENU_OBJECT Menu,
  UINT uItem,
  UINT fByPos);

BOOL FASTCALL UserDestroyMenu(HMENU hMenu);



 
 
/*************** SCROLLBAR.C ***************/
 
DWORD FASTCALL
co_UserShowScrollBar(PWINDOW_OBJECT Window, int wBar, DWORD bShow);

 
#endif /* _WIN32K_USERFUNCS_H */

/* EOF */
