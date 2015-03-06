#pragma once

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
    /* FIXME: Check for 64-bit architectures... */
    return ((ULONG_PTR)lpWndProc & 0xFFFF0000) == 0xFFFF0000;
}

BOOLEAN
DestroyCallProc(_Inout_ PVOID Object);

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

VOID
IntDereferenceClass(IN OUT PCLS Class,
                    IN PDESKTOPINFO Desktop,
                    IN PPROCESSINFO pi);

PCLS
IntGetAndReferenceClass(PUNICODE_STRING ClassName, HINSTANCE hInstance, BOOL bDesktopThread);

BOOL FASTCALL UserRegisterSystemClasses(VOID);

VOID
UserAddCallProcToClass(IN OUT PCLS Class,
                       IN PCALLPROCDATA CallProc);

BOOL
NTAPI
IntGetAtomFromStringOrAtom(
    _In_ PUNICODE_STRING ClassName,
    _Out_ RTL_ATOM *Atom);

BOOL
IntCheckProcessDesktopClasses(IN PDESKTOP Desktop,
                              IN BOOL FreeOnFailure);

ULONG_PTR FASTCALL UserGetCPD(PVOID,GETCPD,ULONG_PTR);

_Must_inspect_result_
NTSTATUS
NTAPI
ProbeAndCaptureUnicodeStringOrAtom(
    _Out_ _When_(return>=0, _At_(pustrOut->Buffer, _Post_ _Notnull_)) PUNICODE_STRING pustrOut,
    __in_data_source(USER_MODE) _In_ PUNICODE_STRING pustrUnsafe);

/* EOF */
