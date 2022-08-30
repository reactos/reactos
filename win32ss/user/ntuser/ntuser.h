#pragma once

#define DECLARE_RETURN(type) type _ret_
#define RETURN(value) { _ret_ = value; goto _cleanup_; }
#define CLEANUP /*unreachable*/ ASSERT(FALSE); _cleanup_
#define END_CLEANUP return _ret_;
#define IS_IMM_MODE() (gpsi && (gpsi->dwSRVIFlags & SRVINFO_IMM32))

#define UserEnterCo UserEnterExclusive
#define UserLeaveCo UserLeave

typedef VOID (*TL_FN_FREE)(PVOID);

typedef struct _TL
{
    struct _TL* next;
    PVOID pobj;
    TL_FN_FREE pfnFree;
} TL, *PTL;

extern PSERVERINFO gpsi;
extern PTHREADINFO gptiCurrent;
extern PPROCESSINFO gppiList;
extern PPROCESSINFO ppiScrnSaver;
extern PPROCESSINFO gppiInputProvider;
extern BOOL g_AlwaysDisplayVersion;
extern ATOM gaGuiConsoleWndClass;
extern ATOM AtomDDETrack;
extern ATOM AtomQOS;
extern ATOM AtomImeLevel;
extern ERESOURCE UserLock;

CODE_SEG("INIT") NTSTATUS NTAPI InitUserImpl(VOID);
VOID FASTCALL CleanupUserImpl(VOID);
VOID FASTCALL UserEnterShared(VOID);
VOID FASTCALL UserEnterExclusive(VOID);
VOID FASTCALL UserLeave(VOID);
BOOL FASTCALL UserIsEntered(VOID);
BOOL FASTCALL UserIsEnteredExclusive(VOID);
DWORD FASTCALL UserGetLanguageToggle(VOID);

_Success_(return != FALSE)
BOOL
NTAPI
RegReadUserSetting(
    _In_z_ PCWSTR pwszKeyName,
    _In_z_ PCWSTR pwszValueName,
    _In_ ULONG ulType,
    _Out_writes_bytes_(cjDataSize) _When_(ulType == REG_SZ, _Post_z_) PVOID pvData,
    _In_ ULONG cjDataSize);

_Success_(return != FALSE)
BOOL
NTAPI
RegWriteUserSetting(
    _In_z_ PCWSTR pwszKeyName,
    _In_z_ PCWSTR pwszValueName,
    _In_ ULONG ulType,
    _In_reads_bytes_(cjDataSize) const VOID *pvData,
    _In_ ULONG cjDataSize);

PGRAPHICS_DEVICE
NTAPI
InitDisplayDriver(
    IN PWSTR pwszDeviceName,
    IN PWSTR pwszRegKey);

inline VOID APIENTRY PushW32ThreadLock(PVOID pobj, PTL ptl, PVOID pfnFree)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    ptl->next = pti->ptlW32;
    pti->ptlW32 = ptl;
    ptl->pobj = pobj;
    ptl->pfnFree = pfnFree;
}

inline VOID APIENTRY PopW32ThreadLock(PTL ptl)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    pti->ptlW32 = ptl->next;
}

inline VOID APIENTRY PopAndFreeAlwaysW32ThreadLock(PTL ptl)
{
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
    pti->ptlW32 = ptl->next;
    ptl->pfnFree(ptl->pobj);
}

/* EOF */
