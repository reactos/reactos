#ifndef _WIN32K_CLASS_H
#define _WIN32K_CLASS_H

#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/ntapi.h>
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
  UNICODE_STRING lpszMenuName;
  RTL_ATOM Atom;
  HICON   hIconSm;
  BOOL Unicode;
  BOOL Global;
  LIST_ENTRY ListEntry;
  PCHAR   ExtraData;
} WNDCLASS_OBJECT, *PWNDCLASS_OBJECT;

NTSTATUS FASTCALL
InitClassImpl(VOID);

NTSTATUS FASTCALL
CleanupClassImpl(VOID);

#define IntLockProcessClasses(W32Process) \
  ExAcquireFastMutex(&W32Process->ClassListLock)

#define IntUnLockProcessClasses(W32Process) \
  ExReleaseFastMutex(&W32Process->ClassListLock)

BOOL FASTCALL
ClassReferenceClassByAtom(
   PWNDCLASS_OBJECT* Class,
   RTL_ATOM Atom,
   HINSTANCE hInstance);

BOOL FASTCALL
ClassReferenceClassByName(
   PWNDCLASS_OBJECT *Class,
   LPCWSTR ClassName,
   HINSTANCE hInstance);

BOOL FASTCALL
ClassReferenceClassByNameOrAtom(
   PWNDCLASS_OBJECT *Class,
   LPCWSTR ClassNameOrAtom,
   HINSTANCE hInstance);

struct _WINDOW_OBJECT;
ULONG FASTCALL
IntGetClassLong(struct _WINDOW_OBJECT *WindowObject, ULONG Offset, BOOL Ansi);

#endif /* _WIN32K_CLASS_H */

/* EOF */
