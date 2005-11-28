#ifndef _WIN32K_CLASS_H
#define _WIN32K_CLASS_H

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

typedef struct _WNDCLASS_OBJECT
{
  UINT    cbSize;
  LONG    refs;                  /* windows using this class (is 0 after class creation) */
  UINT    style;
  WNDPROC lpfnWndProcA;
  WNDPROC lpfnWndProcW;
  int     cbClsExtra;
  int     cbWndExtra;
  HANDLE  hInstance;
  HICON   hIcon;
  HCURSOR hCursor;
  HBRUSH  hbrBackground;
  HMENU   hMenu;
  UNICODE_STRING lpszMenuName;
  RTL_ATOM Atom;
  HICON   hIconSm;
  BOOL Unicode;
  BOOL Global;
  LIST_ENTRY ListEntry;          /* linked into owning process */
  PCHAR   ExtraData;
} WNDCLASS_OBJECT, *PWNDCLASS_OBJECT;

NTSTATUS FASTCALL
InitClassImpl(VOID);

NTSTATUS FASTCALL
CleanupClassImpl(VOID);

void FASTCALL DestroyProcessClasses(PW32PROCESS Process );

__inline VOID FASTCALL 
ClassDerefObject(PWNDCLASS_OBJECT Class);

__inline VOID FASTCALL 
ClassRefObject(PWNDCLASS_OBJECT Class);

PWNDCLASS_OBJECT FASTCALL
ClassGetClassByAtom(
   RTL_ATOM Atom,
   HINSTANCE hInstance);

PWNDCLASS_OBJECT FASTCALL
ClassGetClassByName(
   LPCWSTR ClassName,
   HINSTANCE hInstance);

PWNDCLASS_OBJECT FASTCALL
ClassGetClassByNameOrAtom(
   LPCWSTR ClassNameOrAtom,
   HINSTANCE hInstance);

struct _WINDOW_OBJECT;
ULONG FASTCALL
IntGetClassLong(struct _WINDOW_OBJECT *WindowObject, ULONG Offset, BOOL Ansi);

#endif /* _WIN32K_CLASS_H */

/* EOF */
