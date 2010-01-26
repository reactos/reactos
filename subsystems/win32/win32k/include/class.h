#ifndef _WIN32K_CLASS_H
#define _WIN32K_CLASS_H

#include <include/win32.h>
#include <include/desktop.h>

#define IS_ATOM(x) \
  (((ULONG_PTR)(x) > 0x0) && ((ULONG_PTR)(x) < 0x10000))

typedef struct _WNDPROC_INFO
{
    WNDPROC WindowProc;
    BOOL IsUnicode;
} WNDPROC_INFO, *PWNDPROC_INFO;

static __inline BOOL
IsCallProcHandle(IN WNDPROC lpWndProc)
{
    /* FIXME - check for 64 bit architectures... */
    return ((ULONG_PTR)lpWndProc & 0xFFFF0000) == 0xFFFF0000;
}

WNDPROC
GetCallProcHandle(IN PCALLPROCDATA CallProc);

VOID
DestroyCallProc(IN PDESKTOPINFO Desktop,
                IN OUT PCALLPROCDATA CallProc);

PCALLPROCDATA
CloneCallProc(IN PDESKTOP Desktop,
              IN PCALLPROCDATA CallProc);

PCALLPROCDATA
CreateCallProc(IN PDESKTOP Desktop,
               IN WNDPROC WndProc,
               IN BOOL Unicode,
               IN PPROCESSINFO pi);

BOOL
UserGetCallProcInfo(IN HANDLE hCallProc,
                    OUT PWNDPROC_INFO wpInfo);

void FASTCALL
DestroyProcessClasses(PPROCESSINFO Process );

PCLS
IntReferenceClass(IN OUT PCLS BaseClass,
                  IN OUT PCLS *ClassLink,
                  IN PDESKTOP Desktop);

VOID
IntDereferenceClass(IN OUT PCLS Class,
                    IN PDESKTOPINFO Desktop,
                    IN PPROCESSINFO pi);

RTL_ATOM
UserRegisterClass(IN CONST WNDCLASSEXW* lpwcx,
                  IN PUNICODE_STRING ClassName,
                  IN PUNICODE_STRING MenuName,
                  IN DWORD fnID,
                  IN DWORD dwFlags);

BOOL
UserUnregisterClass(IN PUNICODE_STRING ClassName,
                    IN HINSTANCE hInstance,
                    OUT PCLSMENUNAME pClassMenuName);

ULONG_PTR
UserGetClassLongPtr(IN PCLS Class,
                    IN INT Index,
                    IN BOOL Ansi);

RTL_ATOM
IntGetClassAtom(IN PUNICODE_STRING ClassName,
                IN HINSTANCE hInstance  OPTIONAL,
                IN PPROCESSINFO pi  OPTIONAL,
                OUT PCLS *BaseClass  OPTIONAL,
                OUT PCLS **Link  OPTIONAL);

PCLS
FASTCALL
IntCreateClass(IN CONST WNDCLASSEXW* lpwcx,
               IN PUNICODE_STRING ClassName,
               IN PUNICODE_STRING MenuName,
               IN DWORD fnID,
               IN DWORD dwFlags,
               IN PDESKTOP Desktop,
               IN PPROCESSINFO pi);

BOOL FASTCALL UserRegisterSystemClasses(VOID);

VOID
UserAddCallProcToClass(IN OUT PCLS Class,
                       IN PCALLPROCDATA CallProc);

BOOL
IntGetAtomFromStringOrAtom(IN PUNICODE_STRING ClassName,
                           OUT RTL_ATOM *Atom);

BOOL
IntCheckProcessDesktopClasses(IN PDESKTOP Desktop,
                              IN BOOL FreeOnFailure);

BOOL FASTCALL LookupFnIdToiCls(int, int * );
WNDPROC FASTCALL IntGetClassWndProc(PCLS, BOOL);
ULONG_PTR FASTCALL UserGetCPD(PVOID,GETCPD,ULONG_PTR);

#endif /* _WIN32K_CLASS_H */

/* EOF */
