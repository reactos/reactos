#ifndef __WIN32K_CLASS_H
#define __WIN32K_CLASS_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <napi/win32.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

typedef struct _WNDCLASS_OBJECT
{
  UINT    cbSize;
  UINT    style;
  WNDPROC lpfnWndProcA;
  WNDPROC lpfnWndProcW;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  PUNICODE_STRING lpszMenuName;
  PUNICODE_STRING lpszClassName;
  HICON   hIconSm;
  BOOL Unicode;
  LIST_ENTRY ListEntry;
} WNDCLASS_OBJECT, *PWNDCLASS_OBJECT;

NTSTATUS FASTCALL
InitClassImpl(VOID);

NTSTATUS FASTCALL
CleanupClassImpl(VOID);

NTSTATUS STDCALL
ClassReferenceClassByName(PWNDCLASS_OBJECT *Class,
			  LPWSTR ClassName);

NTSTATUS FASTCALL
ClassReferenceClassByAtom(PWNDCLASS_OBJECT *Class,
			  RTL_ATOM ClassAtom);

NTSTATUS FASTCALL
ClassReferenceClassByNameOrAtom(PWNDCLASS_OBJECT *Class,
				LPWSTR ClassNameOrAtom);
PWNDCLASS_OBJECT FASTCALL
W32kCreateClass(CONST WNDCLASSEXW *lpwcx,
		BOOL bUnicodeClass,
		RTL_ATOM Atom);
struct _WINDOW_OBJECT;
ULONG FASTCALL
W32kGetClassLong(struct _WINDOW_OBJECT *WindowObject, ULONG Offset, BOOL Ansi);

#endif /* __WIN32K_CLASS_H */

/* EOF */
