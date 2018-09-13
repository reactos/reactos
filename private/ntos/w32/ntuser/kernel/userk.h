/*++ BUILD Version: 0015    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: userk.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used exclusively by the User
* kernel-mode code.
*
* History:
* 04-28-91 DarrinM      Created from PROTO.H, MACRO.H, and STRTABLE.H
* 01-25-95 JimA         Prepped for kernel-mode
\***************************************************************************/

#ifndef _USERK_
#define _USERK_

#ifndef _WINBASE_
#include "wbasek.h"
#endif // _WINBASE_

#include "csrmsg.h"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
//BlColor flags
#define BC_INVERT             0x00000001
#define BC_NOMIRROR           0x00000002
#ifdef USE_MIRRORING
#define MIRRORED_HDC(hdc)     (GreGetLayout(hdc) & LAYOUT_RTL)
#endif
#define HEBREW_UI_LANGID()    (gpsi->UILangID == MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT))

#define OEMRESOURCE 1

#define CCACHEDCAPTIONS 5

#define RIT_PROCESSINPUT  0x01
#define RIT_STOPINPUT     0x02

#include <winnls.h>
#include <wincon.h>

#include <winuser.h>
#include <winuserp.h>
#include <wowuserp.h>

#include <user.h>

PTHREADINFO _ptiCrit(VOID);
PTHREADINFO _ptiCritShared(VOID);

#if DBG
    #define PtiCurrent()  _ptiCrit()
    #define PtiCurrentShared() _ptiCritShared()
#else // DBG
    #define PtiCurrent()  (gptiCurrent)
    #define PtiCurrentShared() ((PTHREADINFO)(W32GetCurrentThread()))
#endif // DBG

typedef struct tagEVENTHOOK *PEVENTHOOK;
typedef struct tagNOTIFY *PNOTIFY;

#if DEBUGTAGS
    void CheckPtiSysPeek(int where, PQ pq, ULONG_PTR newIdSysPeek);
    void CheckSysLock(int where, PQ pq, PTHREADINFO pti);
#else // DEBUGTAGS
    #define CheckPtiSysPeek(where, pq, newIdSysPeek)
    #define CheckSysLock(where, pq, pti)
#endif // DEBUGTAGS

/*
 * ShutdownProcessRoutine return values
 */
#define SHUTDOWN_KNOWN_PROCESS   1
#define SHUTDOWN_UNKNOWN_PROCESS 2
#define SHUTDOWN_CANCEL          3

/*
 * Macros to get address of current thread and process information.
 */

#define PpiCurrent() \
    ((PPROCESSINFO)(W32GetCurrentProcess()))

#define PtiFromThread(Thread) ((PTHREADINFO)((Thread)->Tcb.Win32Thread))

#define PpiFromProcess(Process)                                           \
        ((PPROCESSINFO)((PW32PROCESS)(Process)->Win32Process))

#define GetCurrentProcessId() \
        (PsGetCurrentThread()->Cid.UniqueProcess)

#define ISCSRSS() (PsGetCurrentProcess() == gpepCSRSS)

#define CheckForClientDeath()


NTSTATUS OpenEffectiveToken(
    PHANDLE phToken);

NTSTATUS GetProcessLuid(
    PETHREAD Thread OPTIONAL,
    PLUID LuidProcess);

BOOLEAN IsRestricted(
    PETHREAD Thread);

NTSTATUS CreateSystemThread(
    PKSTART_ROUTINE lpThreadAddress,
    PVOID pvContext,
    PHANDLE phThread);

NTSTATUS InitSystemThread(
    PUNICODE_STRING pstrThreadName);

PKEVENT CreateKernelEvent(
    IN EVENT_TYPE Type,
    IN BOOLEAN State);

NTSTATUS ProtectHandle(
    IN HANDLE Handle,
    IN BOOLEAN Protect);

VOID __inline FreeKernelEvent(PVOID* pp)
{
    UserFreePool(*pp);
    *pp = NULL;
}

/*
 * Object types exported from the kernel.
 */
extern POBJECT_TYPE *ExWindowStationObjectType;
extern POBJECT_TYPE *ExDesktopObjectType;
extern POBJECT_TYPE *ExEventObjectType;

/*
 * Private probing macros
 */

#if defined(_X86_)
#define DATAALIGN sizeof(BYTE)
#define CHARALIGN sizeof(BYTE)
#else
#define DATAALIGN sizeof(DWORD)
#define CHARALIGN sizeof(WCHAR)
#endif

#define ProbeForReadBuffer(Address, Count, Alignment) {                     \
    if ((ULONG)(Count) > (ULONG)(MAXULONG / sizeof(*(Address)))) {          \
        ExRaiseAccessViolation();                                           \
    }                                                                       \
    ProbeForRead(Address, (ULONG)(Count) * sizeof(*(Address)), Alignment);  \
}

#define ProbeForWriteBuffer(Address, Count, Alignment) {                    \
    if ((ULONG)(Count) > (ULONG)(MAXULONG / sizeof(*(Address)))) {          \
        ExRaiseAccessViolation();                                           \
    }                                                                       \
    ProbeForWrite(Address, (ULONG)(Count) * sizeof(*(Address)), Alignment); \
}

#define ProbeAndReadSize(Address)                         \
    (((Address) >= (SIZE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SIZE * const)MM_USER_PROBE_ADDRESS) : (*(volatile SIZE *)(Address)))

#define ProbeAndReadBlendfunction(Address)                         \
    (((Address) >= (BLENDFUNCTION * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BLENDFUNCTION * const)MM_USER_PROBE_ADDRESS) : (*(volatile BLENDFUNCTION *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadPoint(
//     IN PPOINT Address
//     )
//
//--

#define ProbePoint(Address)                                \
    (((Address) >= (POINT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DWORD * const)MM_USER_PROBE_ADDRESS) : (*(volatile DWORD *)(Address)))

#define ProbeAndReadPoint(Address)                         \
    (((Address) >= (POINT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile POINT * const)MM_USER_PROBE_ADDRESS) : (*(volatile POINT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadRect(
//     IN PRECT Address
//     )
//
//--

#define ProbeRect(Address)                                \
    (((Address) >= (RECT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DWORD * const)MM_USER_PROBE_ADDRESS) : (*(volatile DWORD *)(Address)))

#define ProbeAndReadRect(Address)                         \
    (((Address) >= (RECT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile RECT * const)MM_USER_PROBE_ADDRESS) : (*(volatile RECT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadMessage(
//     IN PMSG Address
//     )
//
//--

#define ProbeMessage(Address)                            \
    (((Address) >= (MSG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DWORD * const)MM_USER_PROBE_ADDRESS) : (*(volatile DWORD *)(Address)))

#define ProbeAndReadMessage(Address)                     \
    (((Address) >= (MSG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MSG * const)MM_USER_PROBE_ADDRESS) : (*(volatile MSG *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadLargeString(
//     IN PLARGE_STRING Address
//     )
//
//--

#define ProbeAndReadLargeString(Address)                          \
    (((Address) >= (LARGE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LARGE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile LARGE_STRING *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadWindowPlacement(
//     IN PWINDOWPLACEMENT Address
//     )
//
//--

#define ProbeAndReadWindowPlacement(Address)                         \
    (((Address) >= (WINDOWPLACEMENT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile WINDOWPLACEMENT * const)MM_USER_PROBE_ADDRESS) : (*(volatile WINDOWPLACEMENT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadMenuItem(
//     IN PMENUITEMINFO Address
//     )
//
//--

#define ProbeAndReadMenuItem(Address)                             \
    (((Address) >= (MENUITEMINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MENUITEMINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile MENUITEMINFO *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadMenuInfo(
//     IN PMENUINFO Address
//     )
//
//--

#define ProbeAndReadMenuInfo(Address)                             \
    (((Address) >= (MENUINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MENUINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile MENUINFO *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadScrollInfo(
//     IN PSCROLLINFO Address
//     )
//
//--

#define ProbeAndReadScrollInfo(Address)                         \
    (((Address) >= (SCROLLINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SCROLLINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile SCROLLINFO *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadPopupParams(
//     IN PTPMPARAMS Address
//     )
//
//--

#define ProbeAndReadPopupParams(Address)                       \
    (((Address) >= (TPMPARAMS * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile TPMPARAMS * const)MM_USER_PROBE_ADDRESS) : (*(volatile TPMPARAMS *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadPaintStruct(
//     IN PPAINTSTRUCT Address
//     )
//
//--

#define ProbeAndReadPaintStruct(Address)                         \
    (((Address) >= (PAINTSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile PAINTSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile PAINTSTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCreateStruct(
//     IN PCREATESTRUCTW Address
//     )
//
//--

#define ProbeAndReadCreateStruct(Address)                          \
    (((Address) >= (CREATESTRUCTW * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CREATESTRUCTW * const)MM_USER_PROBE_ADDRESS) : (*(volatile CREATESTRUCTW *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadMDICreateStruct(
//     IN PMDICREATESTRUCT Address
//     )
//
//--

#define ProbeAndReadMDICreateStruct(Address)                         \
    (((Address) >= (MDICREATESTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MDICREATESTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile MDICREATESTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCopyDataStruct(
//     IN PCOPYDATASTRUCT Address
//     )
//
//--

#define ProbeAndReadCopyDataStruct(Address)                         \
    (((Address) >= (COPYDATASTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile COPYDATASTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile COPYDATASTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCompareItemStruct(
//     IN PCOMPAREITEMSTRUCT Address
//     )
//
//--

#define ProbeAndReadCompareItemStruct(Address)                         \
    (((Address) >= (COMPAREITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile COMPAREITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile COMPAREITEMSTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadDeleteItemStruct(
//     IN PDELETEITEMSTRUCT Address
//     )
//
//--

#define ProbeAndReadDeleteItemStruct(Address)                         \
    (((Address) >= (DELETEITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DELETEITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile DELETEITEMSTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadHelp(
//     IN PHLP Address
//     )
//
//--

#define ProbeAndReadHelp(Address)                        \
    (((Address) >= (HLP * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HLP * const)MM_USER_PROBE_ADDRESS) : (*(volatile HLP *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadHelpInfo(
//     IN PHELPINFO Address
//     )
//
//--

#define ProbeAndReadHelpInfo(Address)                         \
    (((Address) >= (HELPINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HELPINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile HELPINFO *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadDrawItemStruct(
//     IN PDRAWITEMSTRUCT Address
//     )
//
//--

#define ProbeAndReadDrawItemStruct(Address)                         \
    (((Address) >= (DRAWITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DRAWITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile DRAWITEMSTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadHookInfo(
//     IN PDEBUGHOOKINFO Address
//     )
//
//--

#define ProbeAndReadHookInfo(Address)                              \
    (((Address) >= (DEBUGHOOKINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile DEBUGHOOKINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile DEBUGHOOKINFO *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCBTActivateStruct(
//     IN PCBTACTIVATESTRUCT Address
//     )
//
//--

#define ProbeAndReadCBTActivateStruct(Address)                         \
    (((Address) >= (CBTACTIVATESTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CBTACTIVATESTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile CBTACTIVATESTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadKbdHook(
//     IN PKBDHOOKSTRUCT Address
//     )
//
//--

#define ProbeAndReadKbdHook(Address)                               \
    (((Address) >= (KBDLLHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile KBDLLHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile KBDLLHOOKSTRUCT *)(Address)))
//++
//
// BOOLEAN
// ProbeAndReadMsllHook(
//     IN PMSLLHOOKSTRUCT Address
//     )
//
//--

#define ProbeAndReadMsllHook(Address)                               \
    (((Address) >= (MSLLHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MSLLHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile MSLLHOOKSTRUCT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadMouseHook(
//     IN PMOUSEHOOKSTRUCTEX Address
//     )
//
//--

#define ProbeAndReadMouseHook(Address)                               \
    (((Address) >= (MOUSEHOOKSTRUCTEX * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MOUSEHOOKSTRUCTEX * const)MM_USER_PROBE_ADDRESS) : (*(volatile MOUSEHOOKSTRUCTEX *)(Address)))


#ifdef REDIRECTION

//++
//
// BOOLEAN
// ProbeAndReadHTHook(
//     IN PHTHOOKSTRUCT Address
//     )
//
//--

#define ProbeAndReadHTHook(Address)                               \
    (((Address) >= (HTHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HTHOOKSTRUCT * const)MM_USER_PROBE_ADDRESS) : (*(volatile HTHOOKSTRUCT *)(Address)))

#endif // REDIRECTION

//++
//
// BOOLEAN
// ProbeAndReadCBTCreateStruct(
//     IN PCBT_CREATEWND Address
//     )
//
//--

#define ProbeAndReadCBTCreateStruct(Address)                       \
    (((Address) >= (CBT_CREATEWND * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CBT_CREATEWND * const)MM_USER_PROBE_ADDRESS) : (*(volatile CBT_CREATEWND *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadTrackMouseEvent(
//     IN LPTRACKMOUSEEVENT Address
//     )
//
//--

#define ProbeAndReadTrackMouseEvent(Address) \
    (((Address) >= (TRACKMOUSEEVENT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile TRACKMOUSEEVENT * const)MM_USER_PROBE_ADDRESS) : (*(volatile TRACKMOUSEEVENT *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadWindowPos(
//     IN PWINDOWPOS Address
//     )
//
//--

#define ProbeAndReadWindowPos(Address) \
    (((Address) >= (WINDOWPOS * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile WINDOWPOS * const)MM_USER_PROBE_ADDRESS) : (*(volatile WINDOWPOS *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCursorFind(
//     IN PCURSORFIND Address
//     )
//
//--

#define ProbeAndReadCursorFind(Address) \
    (((Address) >= (CURSORFIND * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CURSORFIND * const)MM_USER_PROBE_ADDRESS) : (*(volatile CURSORFIND *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadSetClipBData(
//     IN PSETCLIPBDATA Address
//     )
//
//--

#define ProbeAndReadSetClipBData(Address) \
    (((Address) >= (SETCLIPBDATA * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SETCLIPBDATA * const)MM_USER_PROBE_ADDRESS) : (*(volatile SETCLIPBDATA *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadBroadcastSystemMsgParams(
//     IN LPBROADCASTSYSTEMMSGPARAMS Address
//     )
//
//--

#define ProbeAndReadBroadcastSystemMsgParams(Address) \
    (((Address) >= (BROADCASTSYSTEMMSGPARAMS * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BROADCASTSYSTEMMSGPARAMS * const)MM_USER_PROBE_ADDRESS) : (*(volatile BROADCASTSYSTEMMSGPARAMS *)(Address)))

//++
//
// BOOLEAN
// ProbeAndReadCursorData(
//     IN PCURSORDATA Address
//     )
//
//--

#define ProbeAndReadCursorData(Address) \
    (((Address) >= (CURSORDATA * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CURSORDATA * const)MM_USER_PROBE_ADDRESS) : (*(volatile CURSORDATA *)(Address)))

//++
//
// BOOLEAN
// ProbeForReadUnicodeStringBuffer(
//     IN UNICODE_STRING String
//     )
//
//--

#if defined(_X86_)
#define ProbeForReadUnicodeStringBuffer(String)                                                          \
    if (((ULONG_PTR)((String).Buffer) & (sizeof(BYTE) - 1)) != 0) {                                   \
        ExRaiseDatatypeMisalignment();                                                            \
    } else if ((((ULONG_PTR)((String).Buffer) + ((String).Length) + sizeof(UNICODE_NULL)) < (ULONG_PTR)((String).Buffer)) ||     \
               (((ULONG_PTR)((String).Buffer) + ((String).Length) + sizeof(UNICODE_NULL)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
        ExRaiseAccessViolation();                                                                 \
    } else if (((String).Length) > ((String).MaximumLength)) {                                    \
        ExRaiseAccessViolation();                                                                 \
    }
#else
#define ProbeForReadUnicodeStringBuffer(String)                                                          \
    if (((ULONG_PTR)((String).Buffer) & (sizeof(WCHAR) - 1)) != 0) {                                  \
        ExRaiseDatatypeMisalignment();                                                            \
    } else if ((((ULONG_PTR)((String).Buffer) + ((String).Length) + sizeof(UNICODE_NULL)) < (ULONG_PTR)((String).Buffer)) ||     \
               (((ULONG_PTR)((String).Buffer) + ((String).Length) + sizeof(UNICODE_NULL)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
        ExRaiseAccessViolation();                                                                 \
    } else if (((String).Length) > ((String).MaximumLength)) {                                    \
        ExRaiseAccessViolation();                                                                 \
    }
#endif

#if defined(_X86_)
#define ProbeForReadUnicodeStringFullBuffer(String)                                                          \
    if (((ULONG_PTR)((String).Buffer) & (sizeof(BYTE) - 1)) != 0) {                                   \
        ExRaiseDatatypeMisalignment();                                                            \
    } else if ((((ULONG_PTR)((String).Buffer) + ((String).MaximumLength)) < (ULONG_PTR)((String).Buffer)) ||     \
               (((ULONG_PTR)((String).Buffer) + ((String).MaximumLength)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
        ExRaiseAccessViolation();                                                                 \
    } else if (((String).Length) > ((String).MaximumLength)) {                                    \
        ExRaiseAccessViolation();                                                                 \
    }
#else
#define ProbeForReadUnicodeStringFullBuffer(String)                                                          \
    if (((ULONG_PTR)((String).Buffer) & (sizeof(WCHAR) - 1)) != 0) {                                  \
        ExRaiseDatatypeMisalignment();                                                            \
    } else if ((((ULONG_PTR)((String).Buffer) + ((String).MaximumLength)) < (ULONG_PTR)((String).Buffer)) ||     \
               (((ULONG_PTR)((String).Buffer) + ((String).MaximumLength)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
        ExRaiseAccessViolation();                                                                 \
    } else if (((String).Length) > ((String).MaximumLength)) {                                    \
        ExRaiseAccessViolation();                                                                 \
    }
#endif

//++
//
// BOOLEAN
// ProbeForReadUnicodeStringBufferOrId(
//     IN UNICODE_STRING String
//     )
//
//--

#define ProbeForReadUnicodeStringBufferOrId(String) \
    if (IS_PTR((String).Buffer)) {           \
        ProbeForReadUnicodeStringBuffer(String);    \
    }

    //++
    //
    // BOOLEAN
    // ProbeAndReadCandidateForm(
    //     IN PCANDIDATEFORM Address
    //     )
    //
    //--

    #define ProbeAndReadCandidateForm(Address) \
        (((Address) >= (CANDIDATEFORM * const)MM_USER_PROBE_ADDRESS) ? \
            (*(volatile CANDIDATEFORM * const)MM_USER_PROBE_ADDRESS) : (*(volatile CANDIDATEFORM *)(Address)))

    //++
    //
    // BOOLEAN
    // ProbeAndReadCompositionForm(
    //     IN PCANDIDATEFORM Address
    //     )
    //
    //--

    #define ProbeAndReadCompositionForm(Address) \
        (((Address) >= (COMPOSITIONFORM * const)MM_USER_PROBE_ADDRESS) ? \
            (*(volatile COMPOSITIONFORM * const)MM_USER_PROBE_ADDRESS) : (*(volatile COMPOSITIONFORM *)(Address)))

    //++
    //
    // BOOLEAN
    // ProbeAndReadLogFontW(
    //     IN PLOGFONTA Address
    //     )
    //
    //--

    #define ProbeAndReadLogFontW(Address) \
        (((Address) >= (LOGFONTW * const)MM_USER_PROBE_ADDRESS) ? \
            (*(volatile LOGFONTW * const)MM_USER_PROBE_ADDRESS) : (*(volatile LOGFONTW *)(Address)))


//++
//
// VOID
// ProbeForWritePoint(
//     IN PPOINT Address
//     )
//
//--

#define ProbeForWritePoint(Address) {                                        \
    if ((Address) >= (POINT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile POINT *)(Address) = *(volatile POINT *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteRect(
//     IN PRECT Address
//     )
//
//--

#define ProbeForWriteRect(Address) {                                         \
    if ((Address) >= (RECT * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile RECT *)(Address) = *(volatile RECT *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteMessage(
//     IN PMSG Address
//     )
//
//--

#define ProbeForWriteMessage(Address) {                                      \
    if ((Address) >= (MSG * const)MM_USER_PROBE_ADDRESS) {                   \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile MSG *)(Address) = *(volatile MSG *)(Address);                 \
}

//++
//
// VOID
// ProbeForWritePaintStruct(
//     IN PPAINTSTRUCT Address
//     )
//
//--

#define ProbeForWritePaintStruct(Address) {                                  \
    if ((Address) >= (PAINTSTRUCT * const)MM_USER_PROBE_ADDRESS) {           \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile PAINTSTRUCT *)(Address) = *(volatile PAINTSTRUCT *)(Address); \
}

//++
//
// VOID
// ProbeForWriteDropStruct(
//     IN PDROPSTRUCT Address
//     )
//
//--

#define ProbeForWriteDropStruct(Address) {                                   \
    if ((Address) >= (DROPSTRUCT * const)MM_USER_PROBE_ADDRESS) {            \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile DROPSTRUCT *)(Address) = *(volatile DROPSTRUCT *)(Address);   \
}

//++
//
// VOID
// ProbeForWriteScrollInfo(
//     IN PSCROLLINFO Address
//     )
//
//--

#define ProbeForWriteScrollInfo(Address) {                                   \
    if ((Address) >= (SCROLLINFO * const)MM_USER_PROBE_ADDRESS) {            \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile SCROLLINFO *)(Address) = *(volatile SCROLLINFO *)(Address);   \
}

//++
//
// VOID
// ProbeForWriteStyleStruct(
//     IN PSTYLESTRUCT Address
//     )
//
//--

#define ProbeForWriteStyleStruct(Address) {                                  \
    if ((Address) >= (STYLESTRUCT * const)MM_USER_PROBE_ADDRESS) {           \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile STYLESTRUCT *)(Address) = *(volatile STYLESTRUCT *)(Address); \
}

//++
//
// VOID
// ProbeForWriteMeasureItemStruct(
//     IN PMEASUREITEMSTRUCT Address
//     )
//
//--

#define ProbeForWriteMeasureItemStruct(Address) {                                       \
    if ((Address) >= (MEASUREITEMSTRUCT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                             \
    }                                                                                   \
                                                                                        \
    *(volatile MEASUREITEMSTRUCT *)(Address) = *(volatile MEASUREITEMSTRUCT *)(Address);\
}

//++
//
// VOID
// ProbeForWriteCreateStruct(
//     IN PCREATESTRUCTW Address
//     )
//
//--

#define ProbeForWriteCreateStruct(Address) {                                    \
    if ((Address) >= (CREATESTRUCTW * const)MM_USER_PROBE_ADDRESS) {            \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                     \
    }                                                                           \
                                                                                \
    *(volatile CREATESTRUCTW *)(Address) = *(volatile CREATESTRUCTW *)(Address);\
}

//++
//
// VOID
// ProbeForWriteEvent(
//     IN PEVENTMSGMSG Address
//     )
//
//--

#define ProbeForWriteEvent(Address) {                                        \
    if ((Address) >= (EVENTMSG * const)MM_USER_PROBE_ADDRESS) {              \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile EVENTMSG *)(Address) = *(volatile EVENTMSG *)(Address);       \
}

//++
//
// VOID
// ProbeForWriteWindowPlacement(
//     IN PWINDOWPLACEMENT Address
//     )
//
//--

#define ProbeForWriteWindowPlacement(Address) {                                     \
    if ((Address) >= (WINDOWPLACEMENT * const)MM_USER_PROBE_ADDRESS) {              \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                         \
    }                                                                               \
                                                                                    \
    *(volatile WINDOWPLACEMENT *)(Address) = *(volatile WINDOWPLACEMENT *)(Address);\
}

//++
//
// VOID
// ProbeForWriteGetClipData(
//     IN PGETCLIPBDATA Address
//     )
//
//--

#define ProbeForWriteGetClipData(Address) {                                   \
    if ((Address) >= (GETCLIPBDATA * const)MM_USER_PROBE_ADDRESS) {           \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                         \
                                                                              \
    *(volatile GETCLIPBDATA *)(Address) = *(volatile GETCLIPBDATA *)(Address);\
}

//++
//
// VOID
// ProbeForWriteMDINextMenu(
//     IN PMDINEXTMENU Address
//     )
//
//--

#define ProbeForWriteMDINextMenu(Address) {                                  \
    if ((Address) >= (MDINEXTMENU * const)MM_USER_PROBE_ADDRESS) {           \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile MDINEXTMENU *)(Address) = *(volatile MDINEXTMENU *)(Address); \
}

//++
//
// VOID
// ProbeForWritePoint5(
//     IN PPOINT5 Address
//     )
//
//--

#define ProbeForWritePoint5(Address) {                                     \
    if ((Address) >= (POINT5 * const)MM_USER_PROBE_ADDRESS) {              \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                         \
    }                                                                               \
                                                                                    \
    *(volatile POINT5 *)(Address) = *(volatile POINT5 *)(Address);\
}

//++
//
// VOID
// ProbeForWriteNCCalcSize(
//     IN PNCCALCSIZE_PARAMS Address
//     )
//
//--

#define ProbeForWriteNCCalcSize(Address) {                                     \
    if ((Address) >= (NCCALCSIZE_PARAMS * const)MM_USER_PROBE_ADDRESS) {              \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                         \
    }                                                                               \
                                                                                    \
    *(volatile NCCALCSIZE_PARAMS *)(Address) = *(volatile NCCALCSIZE_PARAMS *)(Address);\
}

//++
//
// VOID
// ProbeForWriteWindowPos(
//     IN PWINDOWPOS Address
//     )
//
//--

#define ProbeForWriteWindowPos(Address) {                                     \
    if ((Address) >= (WINDOWPOS * const)MM_USER_PROBE_ADDRESS) {              \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                         \
    }                                                                               \
                                                                                    \
    *(volatile WINDOWPOS *)(Address) = *(volatile WINDOWPOS *)(Address);\
}


    //++
    //
    // VOID
    // ProbeForWriteCandidateForm(
    //     IN PCANDIDATEFORM Address
    //     )
    //
    //--

    #define ProbeForWriteCandidateForm(Address) {                                     \
        if ((Address) >= (CANDIDATEFORM * const)MM_USER_PROBE_ADDRESS) {              \
            *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                       \
        }                                                                             \
                                                                                      \
        *(volatile CANDIDATEFORM *)(Address) = *(volatile CANDIDATEFORM *)(Address);  \
    }

    //++
    //
    // VOID
    // ProbeForWriteCompositionForm(
    //     IN PCOMPOSITIONFORM Address
    //     )
    //
    //--

    #define ProbeForWriteCompositionForm(Address) {                                     \
        if ((Address) >= (COMPOSITIONFORM * const)MM_USER_PROBE_ADDRESS) {              \
            *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                         \
        }                                                                               \
                                                                                        \
        *(volatile COMPOSITIONFORM *)(Address) = *(volatile COMPOSITIONFORM *)(Address);\
    }

    //++
    //
    // VOID
    // ProbeForWriteLogFontW(
    //     IN PLOGFONTW Address
    //     )
    //
    //--

    #define ProbeForWriteLogFontW(Address) {                                   \
        if ((Address) >= (LOGFONTW * const)MM_USER_PROBE_ADDRESS) {            \
            *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                \
        }                                                                      \
                                                                               \
        *(volatile LOGFONTW *)(Address) = *(volatile LOGFONTW *)(Address);     \
    }

//++
//
// VOID
// ProbeForWriteReconvertString(IN PRECONVERTSTRING Address)
//
//--

#define ProbeForWriteReconvertString(Address) { \
    if ((Address) >= (RECONVERTSTRING* const)MM_USER_PROBE_ADDRESS) {           \
        *(volatile ULONG* const)MM_USER_PROBE_ADDRESS = 0;                      \
    }                                                                           \
                                                                                \
    *(volatile RECONVERTSTRING*)(Address) = *(volatile RECONVERTSTRING*)(Address); \
    *((volatile BYTE*)(Address) + (Address)->dwSize) = *((volatile BYTE*)(Address) + (Address)->dwSize); \
}

#define ProbeForReadReconvertString(pReconv) \
    ProbeForRead((pReconv), (pReconv)->dwSize, 1)


//++
//
// VOID
// ProbeForWriteImeCharPosition(IN LPPrivateIMECHARPOSITION Address)
//
//--

#define ProbeForWriteImeCharPosition(Address) { \
    if ((Address) >= (PrivateIMECHARPOSITION* const)MM_USER_PROBE_ADDRESS) {    \
        *(volatile ULONG* const)MM_USER_PROBE_ADDRESS = 0;                      \
    }                                                                           \
                                                                                \
    *(volatile PrivateIMECHARPOSITION*)(Address) = *(volatile PrivateIMECHARPOSITION*)(Address); \
}



//++
//
// VOID
// ProbeAndReadMenuGetObjectInfo(
//     IN PMENUGETOBJECTINFO Address
//     )
//
//--

#define ProbeAndReadMenuGetObjectInfo(Address) \
    (((Address) >= (MENUGETOBJECTINFO * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile MENUGETOBJECTINFO * const)MM_USER_PROBE_ADDRESS) : (*(volatile MENUGETOBJECTINFO *)(Address)))


/*
 * This macro makes sure an object is thread locked. DEBUG only.
 */
#if DBG
    VOID CheckLock(PVOID pobj);
#else // DBG
    #define CheckLock(p)
#endif // DBG

/*
 * Debug macros
 */
#if DBG

    #define TRACE_INIT(str)    { if (TraceInitialization > 0) {  KdPrint(str); }}
    #define TRACE_SWITCH(str)  { if (TraceFullscreenSwitch > 0)  {  KdPrint(str); }}

    extern PCSZ apszSimpleCallNames[];

    #define TRACE(s)            TAGMSG2(DBGTAG_StubReturn, "%s, retval = %x", (s), retval)
    #define TRACEVOID(s)        TAGMSG1(DBGTAG_StubReturn, "%s", (s))

    #define TRACETHUNK(t)                                                       \
                TAGMSG3(DBGTAG_StubThunk,                                       \
                        "Thunk %s, %s(%s)",                                     \
                        (t),                                                    \
                        (xpfnProc >= FNID_START && xpfnProc <= FNID_END ?       \
                                gapszFNID[xpfnProc - FNID_START] : "Unknown"),  \
                        (msg >= WM_USER ? "WM_USER" : gapszMessage[msg]))

    #define TRACECALLBACK(s)    TAGMSG2(DBGTAG_StubCallback, "%s, retval = %x", (s), retval)

    #define TRACECALLBACKMSG(s)                                                         \
                TAGMSG4(DBGTAG_StubCallback,                                            \
                        "Callback %s, %s(%s), retval = %x",                             \
                        (s),                                                            \
                        (xpfnProc >= (PROC)FNID_START && xpfnProc <= (PROC)FNID_END ?   \
                            gapszFNID[(ULONG_PTR)xpfnProc - FNID_START] : "Unknown"),    \
                        (msg >= WM_USER ? "WM_USER" : gapszMessage[msg]),               \
                        retval)
#else // DBG

    #define TRACE_INIT(str) {}
    #define TRACE_SWITCH(str) {}
    #define TRACE(s)
    #define TRACEVOID(s)
    #define TRACETHUNK(t)
    #define TRACECALLBACK(t)
    #define TRACECALLBACKMSG(t)

#endif // DBG

/*
 * Statistics for performance counter
 */

typedef struct tagPERFINFO {
    LONG               lCount;
    LONG               lMaxCount;
    LONG               lTotalCount;
    SIZE_T             lSize;
} PERFHANDLEINFO, *PPERFHANDLEINFO;

typedef struct _HANDLEPAGE {
    ULONG_PTR iheLimit;    /* first handle index past the end of the page */
    ULONG_PTR iheFreeEven; /* first even free handle in the page -- window objects */
    ULONG_PTR iheFreeOdd;  /* first even odd handle in the page */
} HANDLEPAGE, *PHANDLEPAGE;


#if DBG
VOID  HMCleanUpHandleTable(VOID);
DWORD DbgDumpHandleTable(VOID);
#endif

BOOL     HMInitHandleTable(PVOID pBase);
PVOID    HMAllocObject(PTHREADINFO pti, PDESKTOP pdesk, BYTE btype, DWORD size);
BOOL     HMFreeObject(PVOID pobj);
BOOL     HMMarkObjectDestroy(PVOID pobj);
BOOL     HMDestroyObject(PVOID pobj);
PVOID FASTCALL HMAssignmentLock(PVOID *ppobj, PVOID pobj);
PVOID FASTCALL HMAssignmentUnlock(PVOID *ppobj);
NTSTATUS HMGetStats(HANDLE hProcess, int iPidType, PVOID pResults, UINT cjResultSize);
HANDLE   KernelPtoH(PVOID pObj);
void     HMDestroyUnlockedObject(PHE phe);

void     HMCleanupGrantedHandle(HANDLE h);

/*
 * Validation, handle mapping, etc.
 */
#define RevalidateHwnd(hwnd)   HMValidateHandleNoSecure(hwnd, TYPE_WINDOW)
#define RevalidateCatHwnd(hwnd)   HMValidateCatHandleNoSecure(hwnd, TYPE_WINDOW)

#define HtoPq(h)    ((PVOID)HMObjectFromHandle(h))
#define HtoPqCat(h)    ((PVOID)HMCatObjectFromHandle(h))
#define HtoP(h)     ((PVOID)HMObjectFromHandle(h))
#define HtoPCat(h)     ((PVOID)HMCatObjectFromHandle(h))
#define PW(hwnd)    ((PWND)HtoP(hwnd))
#define PWCat(hwnd)    ((PWND)HtoPCat(hwnd))
#define TID(pti)    HandleToUlong((pti) == NULL ? NULL : (pti)->pEThread->Cid.UniqueThread)
#define TIDq(pti)   HandleToUlong((pti)->pEThread->Cid.UniqueThread)

/*
 * Assignment lock macro -> used for locking objects embedded in structures
 * and globals. Threadlocks used for locking objects across callbacks.
 */
#define Lock(ppobj, pobj) HMAssignmentLock((PVOID *)ppobj, (PVOID)pobj)
#define Unlock(ppobj)     HMAssignmentUnlock((PVOID *)ppobj)

PVOID HMUnlockObjectInternal(PVOID pobj);

#define HMUnlockObject(pobj) \
    ( (--((PHEAD)pobj)->cLockObj == 0) ? HMUnlockObjectInternal(pobj) : pobj )

VOID HMChangeOwnerThread(PVOID pobj, PTHREADINFO pti);
VOID HMChangeOwnerPheProcess(PHE phe, PTHREADINFO pti);
#define HMChangeOwnerProcess(pobj, pti) HMChangeOwnerPheProcess(HMPheFromObject(pobj), pti)

#if DBG
    VOID  HMLockObject(PVOID pobj);
    BOOL  HMRelocateLockRecord(PVOID ppobjOld, LONG_PTR cbDelta);
#else // DBG
    #define HMLockObject(p)     (((PHEAD)p)->cLockObj++)
#endif // DBG

#if DBG
    VOID ThreadLock(PVOID pobj, PTL ptl);
#else // DBG
    #define ThreadLock(_pobj_, _ptl_)          \
    {                                          \
                                               \
        PTHREADINFO _pti_;                     \
        PVOID __pobj_ = (_pobj_);              \
                                               \
        _pti_ = PtiCurrent();                  \
        (_ptl_)->next = _pti_->ptl;            \
        _pti_->ptl = (_ptl_);                  \
        (_ptl_)->pobj = __pobj_;              \
        if (__pobj_ != NULL) {                \
            HMLockObject(__pobj_);            \
        }                                      \
    }
#endif // DBG

#if DBG
    #define ThreadLockAlways(_pobj_, _ptl_)    \
    {                                          \
        PVOID __pobj_ = (_pobj_);              \
        UserAssert(__pobj_ != NULL);          \
        ThreadLock(__pobj_, _ptl_);             \
    }
#else // DBG
    #define ThreadLockAlways(_pobj_, _ptl_)    \
    {                                          \
                                               \
        PTHREADINFO _pti_;                     \
        PVOID __pobj_ = (_pobj_);              \
                                               \
        _pti_ = PtiCurrent();                  \
        (_ptl_)->next = _pti_->ptl;            \
        _pti_->ptl = (_ptl_);                  \
        (_ptl_)->pobj = __pobj_;              \
        HMLockObject(__pobj_);                \
    }
#endif // DBG

#if DBG
    #define ThreadLockNever(_ptl_)             \
    {                                          \
        ThreadLock(NULL, _ptl_);               \
    }
#else // DBG
    #define ThreadLockNever(_ptl_)             \
    {                                          \
                                               \
        PTHREADINFO _pti_;                     \
                                               \
        _pti_ = PtiCurrent();                  \
        (_ptl_)->next = _pti_->ptl;            \
        _pti_->ptl = (_ptl_);                  \
        (_ptl_)->pobj = NULL;                  \
    }
#endif // DBG

#if DBG
    #define ThreadLockAlwaysWithPti(_pti_, _pobj_, _ptl_)  \
    {                                          \
        PVOID __pobj_ = (_pobj_);              \
        UserAssert(_pti_ == PtiCurrentShared());     \
        UserAssert(__pobj_ != NULL);          \
        ThreadLock(__pobj_, _ptl_);             \
    }
#else // DBG
    #define ThreadLockAlwaysWithPti(_pti_, _pobj_, _ptl_)  \
    {                                          \
        PVOID __pobj_ = (_pobj_);              \
        (_ptl_)->next = _pti_->ptl;            \
        _pti_->ptl = (_ptl_);                  \
        (_ptl_)->pobj = __pobj_;              \
        HMLockObject(__pobj_);                \
    }
#endif // DBG

#if DBG
    #define ThreadLockNeverWithPti(_pti_, _ptl_)    \
    {                                               \
        UserAssert(_pti_ == PtiCurrentShared());    \
        ThreadLock(NULL, _ptl_);                    \
    }
#else // DBG
    #define ThreadLockNeverWithPti(_pti_, _ptl_)    \
    {                                               \
        (_ptl_)->next = _pti_->ptl;                 \
        _pti_->ptl = (_ptl_);                       \
        (_ptl_)->pobj = NULL;                       \
    }
#endif // DBG

#if DBG
    #define ThreadLockWithPti(_pti_, _pobj_, _ptl_) \
    {                                               \
        PVOID __pobj_ = (_pobj_);              \
        UserAssert(_pti_ == PtiCurrentShared());    \
        ThreadLock(__pobj_, _ptl_);                  \
    }
#else // DBG
    #define ThreadLockWithPti(_pti_, _pobj_, _ptl_) \
    {                                               \
        PVOID __pobj_ = (_pobj_);              \
        (_ptl_)->next = _pti_->ptl;                 \
        _pti_->ptl = (_ptl_);                       \
        (_ptl_)->pobj = __pobj_;                   \
        if (__pobj_ != NULL) {                     \
            HMLockObject(__pobj_);                 \
        }                                           \
    }
#endif // DBG

#if DBG
    PVOID ThreadLockExchange(PVOID pobj, PTL ptl);
#else // DBG
    PVOID __inline ThreadLockExchange(PVOID pobj, PTL ptl)
    {
        PVOID   pobjOld;

        pobjOld = ptl->pobj;
        ptl->pobj = pobj;
        if (pobj) {
            HMLockObject(pobj);
        }

        if (pobjOld) {
            pobjOld = HMUnlockObject((PHEAD)pobjOld);
        }

        return pobjOld;
    }
#endif // DBG

#if DBG
    #define ThreadLockExchangeAlways(_pobj_, _ptl_)    \
    {                                                  \
        PVOID __pobj_ = (_pobj_);              \
        UserAssert(__pobj_ != NULL);                  \
        ThreadLockExchange(__pobj_, _ptl_);             \
    }
#else // DBG
    PVOID __inline ThreadLockExchangeAlways(PVOID pobj, PTL ptl)
    {
        PVOID   pobjOld;

        pobjOld = ptl->pobj;
        ptl->pobj = pobj;
        HMLockObject(pobj);
        if (pobjOld) {
            pobjOld = HMUnlockObject((PHEAD)pobjOld);
        }

        return pobjOld;
    }
#endif // DBG

#if DBG
    PVOID ThreadUnlock1(PTL ptl);
    #define ThreadUnlock(ptl) ThreadUnlock1(ptl)
#else // DBG
    PVOID ThreadUnlock1(VOID);
    #define ThreadUnlock(ptl) ThreadUnlock1()
#endif // DBG

/*
 * Define this only if you want to track down lock/unlock mismatches
 * for desktop objects
 */
#if DBG
// #define LOGDESKTOPLOCKS
#endif // DBG

#ifdef LOGDESKTOPLOCKS

/*
 * this is the structure used by the desktop logging stuff
 */
typedef struct tagLogD {
    WORD   tag;         // tag
    WORD   type;        // lock | unlock
    ULONG_PTR extra;    // extra information to identify the lock/unlock
    PVOID  trace[6];    // stack trace
} LogD, *PLogD;

/*
 * Tags for LOCK/UNLOCK REFERENCE/DEREFERENCE calls for
 * desktop objects
 */

#define LDU_CLS_DESKPARENT1                 1
#define LDL_CLS_DESKPARENT1                 2

#define LDU_CLS_DESKPARENT2                 3
#define LDL_CLS_DESKPARENT2                 5

#define LDU_FN_DESTROYCLASS                 6
#define LDL_FN_DESTROYCLASS                 7

#define LDU_FN_DESTROYMENU                  8
#define LDL_FN_DESTROYMENU                  9

#define LDU_FN_DESTROYTHREADINFO            10
#define LDL_FN_DESTROYTHREADINFO            11

#define LDU_FN_DESTROYWINDOWSTATION         12
#define LDL_FN_DESTROYWINDOWSTATION         13

#define LDU_DESKDISCONNECT                  14
#define LDL_DESKDISCONNECT                  15

#define LDU_DESK_DESKNEXT                   16
#define LDL_DESK_DESKNEXT1                  17

#define LDU_OBJ_DESK                        18
#define LDL_OBJ_DESK                        19
#define LDL_MOTHERDESK_DESK1                20

#define LDL_PTI_DESK                        21
#define LDL_DT_DESK                         23

#define LDU_PTI_DESK                        24

#define LDU_PPI_DESKSTARTUP1                26
#define LDU_PPI_DESKSTARTUP2                27
#define LDU_PPI_DESKSTARTUP3                28
#define LDL_PPI_DESKSTARTUP1                29
#define LDL_PPI_DESKSTARTUP2                30

#define LDU_DESKLOGON                       31
#define LDL_DESKLOGON                       32

#define LDUT_FN_FREEWINDOW                  33
#define LDLT_FN_FREEWINDOW                  34

#define LDUT_FN_DESKTOPTHREAD_DESK          35
#define LDLT_FN_DESKTOPTHREAD_DESK          36

#define LDUT_FN_DESKTOPTHREAD_DESKTEMP      37
#define LDLT_FN_DESKTOPTHREAD_DESKTEMP      38

#define LDUT_FN_SETDESKTOP                  39
#define LDLT_FN_SETDESKTOP                  40

#define LDUT_FN_NTUSERSWITCHDESKTOP         41
#define LDLT_FN_NTUSERSWITCHDESKTOP         42

#define LDUT_FN_SENDMESSAGEBSM1             43
#define LDUT_FN_SENDMESSAGEBSM2             44
#define LDLT_FN_SENDMESSAGEBSM              45

#define LDUT_FN_SYSTEMBROADCASTMESSAGE      46
#define LDLT_FN_SYSTEMBROADCASTMESSAGE      47

#define LDUT_FN_CTXREDRAWSCREEN             48
#define LDLT_FN_CTXREDRAWSCREEN             49

#define LDUT_FN_CTXDISABLESCREEN            50
#define LDLT_FN_CTXDISABLESCREEN            51

#define LD_DEREF_FN_CREATEDESKTOP1          52
#define LD_DEREF_FN_CREATEDESKTOP2          53
#define LD_DEREF_FN_CREATEDESKTOP3          54
#define LD_REF_FN_CREATEDESKTOP             55

#define LD_DEREF_FN_OPENDESKTOP             56
#define LD_REF_FN_OPENDESKTOP               57

#define LD_DEREF_FN_SETDESKTOP              58
#define LD_REF_FN_SETDESKTOP                59

#define LD_DEREF_FN_GETTHREADDESKTOP        60
#define LD_REF_FN_GETTHREADDESKTOP          61

#define LD_DEREF_FN_CLOSEDESKTOP1           62
#define LD_DEREF_FN_CLOSEDESKTOP2           63
#define LD_REF_FN_CLOSEDESKTOP              64

#define LD_DEREF_FN_RESOLVEDESKTOP          65
#define LD_REF_FN_RESOLVEDESKTOP            66

#define LD_DEREF_VALIDATE_HDESK1            67
#define LD_DEREF_VALIDATE_HDESK2            68
#define LD_DEREF_VALIDATE_HDESK3            69
#define LD_DEREF_VALIDATE_HDESK4            70
#define LDL_VALIDATE_HDESK                  71

#define LDUT_FN_CREATETHREADINFO1           72
#define LDUT_FN_CREATETHREADINFO2           73
#define LDLT_FN_CREATETHREADINFO            74

#define LD_DEREF_FN_SETCSRSSTHREADDESKTOP1  75
#define LD_DEREF_FN_SETCSRSSTHREADDESKTOP2  76
#define LD_REF_FN_SETCSRSSTHREADDESKTOP     77

#define LD_DEREF_FN_CONSOLECONTROL1         78
#define LD_REF_FN_CONSOLECONTROL1           79

#define LD_DEREF_FN_CONSOLECONTROL2         80
#define LD_REF_FN_CONSOLECONTROL2           81

#define LD_DEREF_FN_GETUSEROBJECTINFORMATION 82
#define LD_REF_FN_GETUSEROBJECTINFORMATION   83

#define LD_DEREF_FN_SETUSEROBJECTINFORMATION 84
#define LD_REF_FN_SETUSEROBJECTINFORMATION   85

#define LD_DEREF_FN_CREATEWINDOWSTATION     86
#define LD_REF_FN_CREATEWINDOWSTATION       87

#define LDL_TERM_DESKDESTROY1               88
#define LDL_TERM_DESKDESTROY2               89

#define LDL_MOTHERDESK_DESK2                92

#define LDL_WINSTA_DESKLIST2                93
#define LDL_WINSTA_DESKLIST1                94

#define LDL_DESKRITINPUT                    95
#define LDU_DESKRITINPUT                    96

#define LD_DEREF_FN_2CREATEDESKTOP          97

#define LDL_DESK_DESKNEXT2                  98

#define LDL_DESKSHOULDBEFOREGROUND1         99
#define LDL_DESKSHOULDBEFOREGROUND2         100
#define LDL_DESKSHOULDBEFOREGROUND3         101

#define LDL_HOOK_DESK                       102
#define LDU_HOOK_DESK                       103

#define LDU_DESKSHOULDBEFOREGROUND          105

#define LDU_MOTHERDESK_DESK                 106

void LogDesktop(PDESKTOP pdesk, DWORD tag, BOOL bLock, ULONG_PTR extra);

#else
    #define LogDesktop(pdesk, tag, bLock, extra)
#endif // LOGDESKTOPLOCKS

/*
 * Routines for referencing and assigning kernel objects.
 */
#ifdef LOGDESKTOPLOCKS
    VOID LockObjectAssignment(PVOID*, PVOID, DWORD, ULONG_PTR);
    VOID UnlockObjectAssignment(PVOID*, DWORD, ULONG_PTR);
#else
    VOID LockObjectAssignment(PVOID*, PVOID);
    VOID UnlockObjectAssignment(PVOID*);
#endif

VOID UserDereferenceObject(PVOID pobj);

#define ThreadLockObject(pobj, ptl)                                                 \
{                                                                                   \
    UserAssert(!(PpiCurrent()->W32PF_Flags & W32PF_TERMINATED));                    \
    UserAssert(pobj == NULL || OBJECT_TO_OBJECT_HEADER(pobj)->PointerCount != 0);   \
    PushW32ThreadLock(pobj, ptl, UserDereferenceObject);                            \
    if (pobj != NULL) {                                                             \
        ObReferenceObject(pobj);                                                    \
    }                                                                               \
}

#define ThreadLockExchangeObject(pobj, ptl)                                         \
{                                                                                   \
    UserAssert(!(PpiCurrent()->W32PF_Flags & W32PF_TERMINATED));                    \
    UserAssert(pobj == NULL || OBJECT_TO_OBJECT_HEADER(pobj)->PointerCount != 0);   \
    if (pobj != NULL) {                                                             \
        ObReferenceObject(pobj);                                                    \
    }                                                                               \
    ExchangeW32ThreadLock(pobj, ptl);                                               \
}

#define ThreadUnlockObject(ptl)                                                     \
{                                                                                   \
    PopAndFreeW32ThreadLock(ptl);                                                   \
}                                                                                   \

#ifdef LOGDESKTOPLOCKS

    #define UnlockWinSta(ppwinsta) \
            UnlockObjectAssignment(ppwinsta, 0, 0)

    #define LockWinSta(ppwinsta, pwinsta) \
    {                                                                                           \
        if (pwinsta != NULL)                                                                    \
        {                                                                                       \
            UserAssert(OBJECT_TO_OBJECT_HEADER(pwinsta)->Type == *ExWindowStationObjectType);   \
        }                                                                                       \
        LockObjectAssignment(ppwinsta, pwinsta, 0, 0);                                                \
    }

    #define LockDesktop(ppdesk, pdesk, tag, extra) \
    {                                                                                           \
        if (pdesk != NULL)                                                                      \
        {                                                                                       \
            UserAssert(OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);           \
        }                                                                                       \
        LockObjectAssignment(ppdesk, pdesk, tag, extra);                                        \
    }

    #define UnlockDesktop(ppdesk, tag, extra) \
            UnlockObjectAssignment(ppdesk, tag, extra)

    #define ThreadLockDesktop(pti, pdesk, ptl, tag) \
    {                                                                                           \
        UserAssert(pdesk == NULL || OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);\
        ThreadLockObject(pdesk, ptl);                                                           \
        LogDesktop(pdesk, tag, TRUE, (ULONG_PTR)PtiCurrent());                                      \
    }

    #define ThreadLockExchangeDesktop(pti, pdesk, ptl, tag) \
    {                                                                                           \
        UserAssert(pdesk == NULL || OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);\
        ThreadLockExchangeObject(pdesk, ptl);                                                   \
        LogDesktop(pdesk, tag, TRUE, (ULONG_PTR)PtiCurrent());                                  \
    }

    #define ThreadUnlockDesktop(pti, ptl, tag)                                                  \
    {                                                                                           \
        LogDesktop((PDESKTOP)(((PTL)ptl)->pobj), tag, FALSE, (ULONG_PTR)PtiCurrent());                \
        ThreadUnlockObject(ptl);                                                                \
    }

#else

    #define UnlockWinSta(ppwinsta) \
            UnlockObjectAssignment(ppwinsta)

    #define LockWinSta(ppwinsta, pwinsta) \
    {                                                                                           \
        if (pwinsta != NULL)                                                                    \
        {                                                                                       \
            UserAssert(OBJECT_TO_OBJECT_HEADER(pwinsta)->Type == *ExWindowStationObjectType);   \
        }                                                                                       \
        LockObjectAssignment(ppwinsta, pwinsta);                                                \
    }

    #define LockDesktop(ppdesk, pdesk, tag, extra) \
    {                                                                                           \
        if (pdesk != NULL)                                                                      \
        {                                                                                       \
            UserAssert(OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);           \
        }                                                                                       \
        LockObjectAssignment(ppdesk, pdesk);                                                    \
    }

    #define UnlockDesktop(ppdesk, tag, extra) \
            UnlockObjectAssignment(ppdesk)

    #define ThreadLockDesktop(pti, pdesk, ptl, tag) \
    {                                                                                           \
        UserAssert(pdesk == NULL || OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);\
        ThreadLockObject(pdesk, ptl);                                                           \
    }

    #define ThreadLockExchangeDesktop(pti, pdesk, ptl, tag) \
    {                                                                                           \
        UserAssert(pdesk == NULL || OBJECT_TO_OBJECT_HEADER(pdesk)->Type == *ExDesktopObjectType);\
        ThreadLockExchangeObject(pdesk, ptl);                                                   \
    }

    #define ThreadUnlockDesktop(pti, ptl, tag) ThreadUnlockObject(ptl)

#endif // LOGDESKTOPLOCKS

#define ThreadLockWinSta(pti, pwinsta, ptl) \
{                                                                                           \
    UserAssert(pwinsta == NULL || OBJECT_TO_OBJECT_HEADER(pwinsta)->Type == *ExWindowStationObjectType);\
    ThreadLockObject(pwinsta, ptl);                                                         \
}

#define ThreadLockExchangeWinSta(pti, pwinsta, ptl) \
{                                                                                           \
    UserAssert(pwinsta == NULL || OBJECT_TO_OBJECT_HEADER(pwinsta)->Type == *ExWindowStationObjectType);\
    ThreadLockExchangeObject(pwinsta, ptl);                                                 \
}

#define _ThreadLockPti(pti, pobj, ptl) LockW32Thread((PW32THREAD)pobj, ptl)
#if DBG
#define ThreadLockPti(pti, pobj, ptl) \
{ \
    if ((pobj != NULL) \
            && (pobj->TIF_flags & TIF_INCLEANUP) \
            && (pobj != PtiCurrent())) { \
        RIPMSG1(RIP_ERROR, "ThreadLockPti: dead thread %#p", pobj); \
    } \
    _ThreadLockPti(pti, pobj, ptl); \
}
#else
#define ThreadLockPti(pti, pobj, ptl) \
{ \
    _ThreadLockPti(pti, pobj, ptl); \
}
#endif

#define ThreadUnlockWinSta(pti, ptl) ThreadUnlockObject(ptl)
#define ThreadUnlockPti(pti, ptl) UnlockW32Thread(ptl)

/*
 * Macros for locking pool allocations
 */
#define ThreadLockPool(_pti_, _ppool_, _ptl_)  \
        PushW32ThreadLock(_ppool_, _ptl_, UserFreePool)

#define ThreadUnlockPool(_pti_, _ptl_)  \
        PopW32ThreadLock(_ptl_)

#define ThreadUnlockAndFreePool(_pti_, _ptl_)  \
        PopAndFreeAlwaysW32ThreadLock(_ptl_)

#define ThreadLockPoolCleanup(_pti_, _ppool_, _ptl_, _pfn_)  \
        PushW32ThreadLock(_ppool_, _ptl_, _pfn_)

#define ThreadUnlockPoolCleanup(_pti_, _ptl_)  \
        PopW32ThreadLock(_ptl_)

#define ThreadUnlockAndCleanupPool(_pti_, _ptl_)  \
        PopAndFreeAlwaysW32ThreadLock(_ptl_)

#define ThreadLockDesktopHandle(_pti, _ptl_, _hdesk_)   \
        PushW32ThreadLock(_hdesk_, _ptl_, CloseProtectedHandle)

#define ThreadUnlockDesktopHandle(_ptl_)   \
        PopAndFreeAlwaysW32ThreadLock(_ptl_)

void CleanupDecSFWLockCount(PVOID pIgnore);
#define ThreadLockSFWLockCount(_ptl_) \
        { \
            IncSFWLockCount(); \
            /* Pass a fake pObj or the cleanup function won't be called */ \
            PushW32ThreadLock(&guSFWLockCount, _ptl_, CleanupDecSFWLockCount); \
        }

#define ThreadUnlockSFWLockCount(_ptl_) \
    { \
        DecSFWLockCount(); \
        PopW32ThreadLock(_ptl_); \
    }

/*
 * special handle that signifies we have a rle bitmap for the wallpaper
 */
#define HBITMAP_RLE ((HBITMAP)0xffffffff)

typedef struct tagWPINFO {
    int xsize, ysize;
    PBITMAPINFO pbmi;
    PBYTE pdata;
    PBYTE pbmfh;
} WPINFO;

/*
 * Defines used by GetMouseMovePointsEx API
 */

#define MAX_MOUSEPOINTS 64

#define PREVPOINT(i)    \
    ((i == 0) ? (MAX_MOUSEPOINTS - 1) : ((i - 1) % MAX_MOUSEPOINTS))

#define NEXTPOINT(i)    \
    ((i + 1) % MAX_MOUSEPOINTS)

#define NEXTPOINTCOUNT(c)           \
    {                               \
        if (c < MAX_MOUSEPOINTS)    \
            c++;                    \
    }

#define SAVEPOINT(xc, yc, resX, resY, t, e)                         \
{                                                                   \
    /*                                                              \
     * (xc, yc) is the point and (resX, resY) is the resolution     \
     */                                                             \
    gaptMouse[gptInd].x = MAKELONG(LOWORD(xc), LOWORD(resX));       \
    gaptMouse[gptInd].y = MAKELONG(LOWORD(yc), LOWORD(resY));       \
    gaptMouse[gptInd].time        = t;                              \
    gaptMouse[gptInd].dwExtraInfo = e;                              \
                                                                    \
    gptInd = NEXTPOINT(gptInd);                                     \
}


/*
 * Structure used for getting the stack traces for user critical section
 */

#define MAX_STACK_CALLS 8

typedef struct tagCRITSTACK {
struct tagCRITSTACK* pNext;
    PETHREAD         thread;
    int              nFrames;
    PVOID            trace[ MAX_STACK_CALLS ];
} CRITSTACK, *PCRITSTACK;


/*
 * Macros for User Server and Raw Input Thread critical sections.
 */
#if DBG
    #define KeUserModeCallback(api, pIn, cb, pOut, pcb)    _KeUserModeCallback(api, pIn, cb, pOut, pcb);
    #define CheckCritIn()                _AssertCritIn()
    #define CheckDeviceInfoListCritIn()  _AssertDeviceInfoListCritIn()
    #define CheckCritInShared()          _AssertCritInShared()
    #define CheckCritOut()               _AssertCritOut()
    #define CheckDeviceInfoListCritOut() _AssertDeviceInfoListCritOut()

    void    BeginAtomicCheck();
    void    BeginAtomicDeviceInfoListCheck();
    void    EndAtomicCheck();
    void    EndAtomicDeviceInfoListCheck();
    #define BEGINATOMICCHECK()     BeginAtomicCheck();                              \
                                    { DWORD dwCritSecUseSave = gdwCritSecUseCount;

    #define ENDATOMICCHECK()        UserAssert(dwCritSecUseSave == gdwCritSecUseCount);  \
                                    } EndAtomicCheck();
    #define BEGINATOMICDEVICEINFOLISTCHECK() \
            BeginAtomicDeviceInfoListCheck(); \
            { DWORD dwDeviceInfoListCritSecUseSave = gdwDeviceInfoListCritSecUseCount;

    #define ENDATOMICDEVICEINFOLISTCHECK() \
            UserAssert(dwDeviceInfoListCritSecUseSave == gdwDeviceInfoListCritSecUseCount);  \
            } EndAtomicDeviceInfoListCheck();

    // Use this to jump/return out of scope of dwCritSecUseSave (eg: error handling)
    #define EXITATOMICCHECK()       UserAssert(dwCritSecUseSave == gdwCritSecUseCount);  \
                                    EndAtomicCheck();
    #define ISATOMICCHECK()         (gdwInAtomicOperation != 0)
    #define ISATOMICDEVICEINFOLISTCHECK() (gdwInAtomicDeviceInfoListOperation != 0)

#else // DBG
    #define CheckCritIn()
    #define CheckDeviceInfoListCritIn()
    #define CheckCritInShared()
    #define CheckCritOut()
    #define CheckDeviceInfoListCritOut()
    #define BEGINATOMICCHECK()
    #define BEGINATOMICDEVICEINFOLISTCHECK()
    #define BeginAtomicCheck()
    #define BeginAtomicDeviceInfoListCheck()
    #define ENDATOMICCHECK()
    #define ENDATOMICDEVICEINFOLISTCHECK()
    #define EndAtomicCheck()
    #define EndAtomicDeviceInfoListCheck()
    #define EXITATOMICCHECK()
    #define ISATOMICCHECK()
    #define ISATOMICDEVICEINFOLISTCHECK()
#endif // DBG


#define DIAGNOSE_IO 1
#ifdef DIAGNOSE_IO
ULONG MonotonicTick();
#define LOGTIME(gt) gt = MonotonicTick();
#else
#define LOGTIME(gt)
#endif

/*
 * #defines used for mouse/keyboard read buffer
 */
#define MAXIMUM_ITEMS_READ 10
#define NELEM_BUTTONQUEUE 16

/*
 * Number of times to retry reading a device after a read attempt fails
 */
#define MAXIMUM_READ_RETRIES 5

typedef struct tagGENERIC_DEVICE_INFO {
    struct tagDEVICEINFO *pNext;
    BYTE                 type;
    BYTE                 bFlags;
    USHORT               usActions;
    BYTE                 nRetryRead;
    UNICODE_STRING       ustrName;
    HANDLE               handle;
    PVOID                NotificationEntry;
    PKEVENT              pkeHidChangeCompleted; // wake RequestDeviceChange()
    IO_STATUS_BLOCK      iosb;
    NTSTATUS             ReadStatus;
#ifdef DIAGNOSE_IO
    HANDLE               OpenerProcess;
    NTSTATUS             OpenStatus;
    NTSTATUS             AttrStatus;
    ULONG                timeStartRead;     // tick before ZwReadFile
    ULONG                timeEndRead;       // tick after ZwReadFile
    int                  nReadsOutstanding; // ZwReadFile ++, consume data --
#endif
} GENERIC_DEVICE_INFO, *PGENERIC_DEVICE_INFO;

// valuse for GENERIC_DEVICE_INFO.type
#define DEVICE_TYPE_MOUSE    0
#define DEVICE_TYPE_KEYBOARD 1
#define DEVICE_TYPE_MAX      1

// values for GENERIC_DEVICE_INFO.usActions and SignalDeviceChange()
#define GDIAF_ARRIVED         (USHORT)0x0001 // open & start reading
#define GDIAF_QUERYREMOVE     (USHORT)0x0002 // close the device
#define GDIAF_REMOVECANCELLED (USHORT)0x0004 // reopen the device
#define GDIAF_DEPARTED        (USHORT)0x0008 // close and free the device
#define GDIAF_IME_STATUS      (USHORT)0x0010 // ???
#define GDIAF_REFRESH_MOUSE   (USHORT)0x0020 // ???
#define GDIAF_FREEME          (USHORT)0x0080 // Request to Free the DeviceInfo
#define GDIAF_PNPWAITING      (USHORT)0x0100 // a PnP thread is waiting
#define GDIAF_RETRYREAD       (USHORT)0x0200 // Retry the read
#define GDIAF_RECONNECT       (USHORT)0x0400 // The session reconnected

// values for GENERIC_DEVICE_INFO.bFlags;
#define GDIF_NOTPNP         0x01 // Not a PnP device (eg: PS/2)
#define GDIF_READING        0x02 // Read may be pending (don't free DeviceInfo).
#if DIAGNOSE_IO
#define GDIF_READERMUSTFREE 0x04 // "Free Device" while read pending
#define GDIF_PNPMUSTFREE    0x08 // "Free Device" while PnP notification pending
#endif
#define GDIF_DBGREAD        0x10 // Verbose dbg output about this device

typedef struct tagMOUSE_DEVICE_INFO {    // DEVICE_TYPE_MOUSE
    MOUSE_ATTRIBUTES     Attr;
    MOUSE_INPUT_DATA     Data[MAXIMUM_ITEMS_READ];
} MOUSE_DEVICE_INFO, *PMOUSE_DEVICE_INFO;

typedef struct tagKEYBOARD_DEVICE_INFO { // DEVICE_TYPE_KEYBOARD
    KEYBOARD_ATTRIBUTES  Attr;
    KEYBOARD_INPUT_DATA  Data[MAXIMUM_ITEMS_READ];
} KEYBOARD_DEVICE_INFO, *PKEYBOARD_DEVICE_INFO;

typedef struct tagDEVICEINFO {
    GENERIC_DEVICE_INFO;
    union {
        MOUSE_DEVICE_INFO    mouse;
        KEYBOARD_DEVICE_INFO keyboard;
    };
} DEVICEINFO, *PDEVICEINFO;

typedef struct tagDEVICE_TEMPLATE {
    SIZE_T cbDeviceInfo;        // bytes to allocate for DEVICEINFO
    const GUID *pClassGUID;     // GUID of the class
    UINT   uiRegistrySection;   // Parameters for class (HKLM\SYSTEM\CurrentControlSet\Services\*\Parameters)
    LPWSTR pwszClassName;       // Class name (eg: L"mouclass")
    LPWSTR pwszDefDevName;      // Default Device Name
    LPWSTR pwszLegacyDevName;   // Legacy Device Name (eg: "PointerClassLegacy0")
    ULONG  IOCTL_Attr;          // IOCTL_*_QUERY_ATTRIBUTES
    UINT   offAttr;             // offset of *_ATTRIBUTES struct within DEVICEINFO
    ULONG  cbAttr;              // sizeof *_ATTRIBUTES struct
    UINT   offData;             // offset of *_INPUT_DATA buffer within DEVICEINFO
    ULONG  cbData;              // sizeof *_INPUT_DATA buffer
    VOID   (*DeviceRead)(PDEVICEINFO); // routine to read the device
    PKEVENT pkeHidChange;       // event to signal changes to this sort of device
} DEVICE_TEMPLATE, *PDEVICE_TEMPLATE;

extern DEVICE_TEMPLATE aDeviceTemplate[]; // in pnp.c

typedef struct tagMOUSEEVENT {
    USHORT  ButtonFlags;
    USHORT  ButtonData;
    ULONG_PTR ExtraInfo;
    POINT   ptPointer;
    LONG    time;
    BOOL    bInjected;
} MOUSEEVENT, *PMOUSEEVENT;


VOID ProcessKeyboardInput(PDEVICEINFO pDeviceInfo);
VOID ProcessMouseInput(PDEVICEINFO pDeviceInfo);
VOID RequestDeviceChange(
    PDEVICEINFO pDeviceInfo,
    USHORT usAction,
    BOOL fInDeviceInfoListCrit);

VOID RetryReadInput();

VOID NTAPI InputApc(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved);

ULONG GetDeviceChangeInfo(VOID);
VOID InitializeMediaChange(HANDLE);

/*
 * Hard error information
 */
typedef struct tagHARDERRORHANDLER {
    PTHREADINFO pti;
    PQ pqAttach;
} HARDERRORHANDLER, *PHARDERRORHANDLER;

/*
 * Terminal Structure.
 *
 *   This structure is only viewable from the kernel.
 */

#define TEST_GTERMF(f)               TEST_FLAG(gdwGTERMFlags, f)
#define TEST_BOOL_GTERMF(f)          TEST_BOOL_FLAG(gdwGTERMFlags, f)
#define SET_GTERMF(f)                SET_FLAG(gdwGTERMFlags, f)
#define CLEAR_GTERMF(f)              CLEAR_FLAG(gdwGTERMFlags, f)
#define SET_OR_CLEAR_GTERMF(f, fSet) SET_OR_CLEAR_FLAG(gdwGTERMFlags, f, fSet)
#define TOGGLE_GTERMF(f)             TOGGLE_FLAG(gdwGTERMFlags, f)

#define GTERMF_MOUSE        0x00000001


#define TERMF_INITIALIZED       0x00000001
#define TERMF_NOIO              0x00000002
#define TERMF_STOPINPUT         0x00000004
#define TERMF_DTINITSUCCESS     0x00000008
#define TERMF_DTINITFAILED      0x00000010
#define TERMF_DTDESTROYED       0x00000020

typedef struct tagTERMINAL {

    DWORD               dwTERMF_Flags;      // terminal flags

    /*
     * System Information
     */
    PWND                spwndDesktopOwner;  // mother desktop


    PTHREADINFO         ptiDesktop;
    PQ                  pqDesktop;

    PKEVENT             pEventTermInit;
    PKEVENT             pEventDestroyDesktop;   // Used for destroying desktops

    PDESKTOP            rpdeskDestroy;          // Desktop destroy list.

    PKEVENT             pEventInputReady;   // input ready event. This is created in
                                            // CreateTerminal. RIT and the desktop thread
                                            // will wait for it. It will be set when the
                                            // first desktop in that terminal will be created.
    PKEVENT             pEventDTExit;

} TERMINAL, *PTERMINAL;

/*
 * Pool allocation tags and macros
 */

/*
 * Define tags. To add tags, add them to ntuser\kernel\ptag.lst
 */
#define DEFINE_POOLTAG(value, index) value

#define DECLARE_POOLTAG(name, value, index)

#include "ptag.h"

NTSTATUS UserCommitDesktopMemory(
    PVOID  pBase,
    PVOID *ppCommit,
    PSIZE_T pCommitSize);

NTSTATUS UserCommitSharedMemory(
    PVOID  pBase,
    PVOID *ppCommit,
    PSIZE_T pCommitSize);

PWIN32HEAP UserCreateHeap(
    HANDLE                   hSection,
    ULONG                    ulViewOffset,
    PVOID                    pvBaseAddress,
    DWORD                    dwSize,
    PRTL_HEAP_COMMIT_ROUTINE pfnCommit);

#define IsValidTag(p, tag)      TRUE

#define RECORD_STACK_TRACE_SIZE 6

/*
 * Pool allocation flags
 */

#define POOL_HEAVY_ALLOCS       0x00000001  // use HeavyAllocPool
#define POOL_CAPTURE_STACK      0x00000002  // stack traces are captured
#define POOL_FAIL_ALLOCS        0x00000004  // fail pool allocations
#define POOL_FAIL_BY_INDEX      0x00000008  // fail allocations by index
#define POOL_TAIL_CHECK         0x00000010  // append tail string
#define POOL_KEEP_FREE_RECORD   0x00000020  // keep a list with last x frees
#define POOL_KEEP_FAIL_RECORD   0x00000040  // keep a list with last x failed allocations
#define POOL_BREAK_FOR_LEAKS    0x00000080  // break on pool leaks (remote sessions)

typedef struct tagWin32AllocStats {
    SIZE_T dwMaxMem;             // max pool memory allocated
    SIZE_T dwCrtMem;             // current pool memory used
    DWORD  dwMaxAlloc;           // max number of pool allocations made
    DWORD  dwCrtAlloc;           // current pool allocations

    PWin32PoolHead pHead;        // pointer to the link list with the allocations

} Win32AllocStats, *PWin32AllocStats;

typedef struct tagPOOLRECORD {
    PVOID   ExtraData;           // the tag
    SIZE_T  size;
    PVOID   trace[RECORD_STACK_TRACE_SIZE];
} POOLRECORD, *PPOOLRECORD;

#ifdef POOL_INSTR_API

    BOOL _Win32PoolAllocationStats(
        LPDWORD  parrTags,
        SIZE_T   tagsCount,
        SIZE_T*  lpdwMaxMem,
        SIZE_T*  lpdwCrtMem,
        LPDWORD  lpdwMaxAlloc,
        LPDWORD  lpdwCrtAlloc);

#endif // POOL_INSTR_API

#ifdef POOL_INSTR

    void CleanupPoolAllocations(void);
    void InitPoolLimitations();
    void CleanUpPoolLimitations();
#else
    #define CleanupPoolAllocations()
    #define InitPoolLimitations()
    #define CleanUpPoolLimitations()

#endif // POOL_INSTR

#ifndef TRACE_MAP_VIEWS
    #define InitSectionTrace()
    #define CleanUpSections()
#else
    VOID InitSectionTrace(VOID);
    void CleanUpSections(void);
#endif // TRACE_MAP_VIEWS

extern PWIN32HEAP gpvSharedAlloc;

PVOID __inline
SharedAlloc(ULONG cb)
{
    return Win32HeapAlloc(gpvSharedAlloc, cb, 0, 0);
}

BOOL __inline
SharedFree(PVOID pv)
{
    return Win32HeapFree(gpvSharedAlloc, pv);
}

NTSTATUS CommitReadOnlyMemory(HANDLE hSection, PSIZE_T pulCommit,
                              DWORD dwCommitOffset, int* pdCommit);

/*
 * Height and Width of the desktop pattern bitmap.
 */
#define CXYDESKPATTERN      8

/*
 * LATER: these things are not defined yet
 */
#define CheckHwnd(x)        TRUE
#define CheckHwndNull(x)    TRUE

/***************************************************************************\
* Typedefs and Macros
*
* Here are defined all types and macros that are shared across the User's
* server-side code modules.  Types and macros that are unique to a single
* module should be defined at the head of that module, not in this file.
*
\***************************************************************************/


// Window Proc Window Validation macro

#define VALIDATECLASSANDSIZE(pwnd, message, wParam, lParam, inFNID, initmessage)          \
    if ((pwnd)->fnid != (inFNID)) {                                                       \
        switch ((pwnd)->fnid) {                                                           \
        DWORD cb;                                                                         \
        case 0:                                                                           \
                                                                                          \
            if ((cb = pwnd->cbwndExtra + sizeof(WND)) < (DWORD)(CBFNID(inFNID))) {        \
                RIPMSG3(RIP_WARNING,                                                      \
                       "(%#p %lX) needs at least (%ld) window words for this proc",       \
                        pwnd, cb - sizeof(WND),                                           \
                        (DWORD)(CBFNID(inFNID)) - sizeof(WND));                           \
                return 0;                                                                 \
            }                                                                             \
            /*                                                                            \
             * If this is not the initialization message, we cannot set the fnid;         \
             *  otherwise, we'll probably fault working on this pwnd's private            \
             *  uninitialized data                                                        \
             */                                                                           \
            if ((message) != (initmessage)) {                                             \
                if (((message) != WM_NCCREATE) && ((message) != WM_NCCALCSIZE)  && ((message) != WM_GETMINMAXINFO)) {         \
                    RIPMSG3(RIP_WARNING,                                                  \
                        "Default processing message %#lx for pwnd %#p. fnid %#lx not set",\
                        (message), (pwnd), (DWORD)(inFNID));                              \
                }                                                                         \
                return xxxDefWindowProc((pwnd), (message), (wParam), (lParam));           \
            }                                                                             \
                                                                                          \
            /*                                                                            \
             * Remember what window class this window belongs to.  Can't use              \
             * the real class because any app can call CallWindowProc()                   \
             * directly no matter what the class is!                                      \
             */                                                                           \
            (pwnd)->fnid = (WORD)(inFNID);                                                \
            break;                                                                        \
                                                                                          \
        default:                                                                          \
            RIPMSG3(RIP_WARNING, "Window (%#p) not of correct class; fnid = %lX not %lX", \
                    (pwnd), (DWORD)((pwnd)->fnid), (DWORD)(inFNID));                      \
                                                                                          \
            /* Fall through */                                                            \
                                                                                          \
        case (inFNID | FNID_CLEANEDUP_BIT):                                               \
        case (inFNID | FNID_DELETED_BIT):                                                 \
        case (inFNID | FNID_STATUS_BITS):                                                 \
            return 0;                                                                     \
        }                                                                                 \
    }

/*
 * Handy Region helper macros
 */
#define CopyRgn(hrgnDst, hrgnSrc) \
            GreCombineRgn(hrgnDst, hrgnSrc, NULL, RGN_COPY)
#define IntersectRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_AND)
#define SubtractRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_DIFF)
#define UnionRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_OR)
#define XorRgn(hrgnResult, hrgnA, hrgnB) \
            GreCombineRgn(hrgnResult, hrgnA, hrgnB, RGN_XOR)

void DeleteMaybeSpecialRgn(HRGN hrgn);

BOOL zzzInvalidateDCCache(PWND pwndInvalid, DWORD flags);

#define IDC_DEFAULT         0x0001
#define IDC_CHILDRENONLY    0x0002
#define IDC_CLIENTONLY      0x0004
#define IDC_MOVEBLT         0x0008
#define IDC_NOMOUSE         0x0010

/*
 * RestoreSpb return Flags
 */

#define RSPB_NO_INVALIDATE      0   // nothing invalidated by restore
#define RSPB_INVALIDATE         1   // restore invalidate some area
#define RSPB_INVALIDATE_SSB     2   // restore called SaveScreenBits which invalidated

// Calls Proc directly without doing any messages translation

#define SCMS_FLAGS_ANSI         0x0001
#define SCMS_FLAGS_INONLY       0x0002      // Message should be one way (hooks)

#define CallClientProcA(pwnd, msg, wParam, lParam, xpfn) \
            SfnDWORD(pwnd, msg, wParam, lParam, xpfn,          \
                ((PROC)(gpsi->apfnClientW.pfnDispatchMessage)), TRUE, NULL)
#define CallClientProcW(pwnd, msg, wParam, lParam, xpfn) \
            SfnDWORD(pwnd, msg, wParam, lParam, xpfn,          \
                ((PROC)(gpsi->apfnClientW.pfnDispatchMessage)), TRUE, NULL)
#define CallClientWorkerProc(pwnd, msg, wParam, lParam, xpfn) \
            SfnDWORD(pwnd, msg, wParam, lParam, 0, xpfn, TRUE, NULL)
#define ScSendMessageSMS(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms) \
        (((msg) & ~MSGFLAG_MASK) >= WM_USER) ? \
        SfnDWORD(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms) : \
        gapfnScSendMessage[MessageTable[msg & 0xffff].iFunction](pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms)
#define ScSendMessage(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags) \
        ScSendMessageSMS(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, NULL)

/*
 * Server-side routines for loading cursors/icons/strings/menus from server.
 */
#define SERVERSTRINGMAXSIZE  40
void RtlInitUnicodeStringOrId(PUNICODE_STRING pstrName, LPWSTR lpstrName);
int RtlLoadStringOrError(UINT, LPTSTR, int, WORD);
#define ServerLoadString(hmod, id, p, cch)\
        RtlLoadStringOrError(id, p, cch, 0)
#define ServerLoadStringEx(hmod, id, p, cch, wLang)\
        RtlLoadStringOrError(id, p, cch, wLang)

/*
 * Callback routines for loading resources from client.
 */
HANDLE xxxClientLoadImage(
    PUNICODE_STRING pstrName,
    ATOM atomModName,
    WORD wImageType,
    int cxSize,
    int cySize,
    UINT LR_flags,
    BOOL fWallpaper);

HANDLE xxxClientCopyImage(
    IN HANDLE          hImage,
    IN UINT            uImageType,
    IN int             cxDesired,
    IN int             cyDesired,
    IN UINT            LR_flags);

PMENU xxxClientLoadMenu(
    HANDLE hmod,
    PUNICODE_STRING pstrName);

int xxxClientAddFontResourceW(PUNICODE_STRING, DWORD, DESIGNVECTOR*);

VOID ClientFontSweep(VOID);
VOID ClientLoadLocalT1Fonts();
VOID ClientLoadRemoteT1Fonts();

/*
 * Server-side routine for thread initialization.
 */
NTSTATUS InitializeClientPfnArrays(
    CONST PFNCLIENT *ppfnClientA,
    CONST PFNCLIENT *ppfnClientW,
    CONST PFNCLIENTWORKER *ppfnClientWorker,
    HANDLE hModUser);

//VOID _SetDebugErrorLevel(DWORD);
VOID _SetRipFlags(DWORD, DWORD);
VOID _SetDbgTag(int, DWORD);


/*
 * xxxActivateWindow() commands
 */
#define AW_USE       1
#define AW_TRY       2
#define AW_SKIP      3
#define AW_TRY2      4
#define AW_SKIP2     5      /* used internally in xxxActivateWindow() */
#define AW_USE2      6      /* nc mouse activation added by craigc */

/*
 * Structure for WM_ACTIVATEAPP EnumWindows() callback.
 */
typedef struct tagAAS {
    PTHREADINFO ptiNotify;
    DWORD tidActDeact;
    UINT fActivating  : 1;
    UINT fQueueNotify : 1;
} AAS;

/*
 * Declaration for EnumWindows() callback function.
 */
BOOL xxxActivateApp(PWND pwnd, AAS *paas);

#define GETDESKINFO(pti)  ((pti)->pDeskInfo)

#define SET_TIME_LAST_READ(pti)     ((pti)->pcti->timeLastRead = NtGetTickCount())
#define GET_TIME_LAST_READ(pti)     ((pti)->pcti->timeLastRead)


/*
 * General purpose helper macros
 */
#define abs(A)      (((A) < 0)? -(A) : (A))

#define N_ELEM(a)     (sizeof(a)/sizeof(a[0]))
#define LAST_ELEM(a)  ( (a) [ N_ELEM(a) - 1 ] )
#define PLAST_ELEM(a) (&LAST_ELEM(a))


/*
 * General purpose access check macro
 */
#define RETURN_IF_ACCESS_DENIED(amGranted, amRequested, r) \
        if (!CheckGrantedAccess((amGranted), (amRequested))) return r

/*
 * Lock record structure for tracking locks (debug only)
 */

#define LOCKRECORD_STACK    8
#define LOCKRECORD_MARKDESTROY  IntToPtr( 0xFFFFFFFF )

typedef struct _LOCKRECORD {
    PLR    plrNext;
    DWORD  cLockObj;
    PVOID  ppobj;
    PVOID  trace[LOCKRECORD_STACK];
} LOCKRECORD;

/*
 * We limit recursion until if we have only this much stack left.
 * We have to leave room for kernel interupts
 */
#define KERNEL_STACK_MINIMUM_RESERVE  (4*1024)

/*
 * The following is a LOCK structure. This structure is recorded for
 * each threadlock so unlocks can occur at cleanup time.
 */
typedef struct _LOCK {
    PTHREADINFO pti;
    PVOID pobj;
    PTL ptl;
#if DBG
    PVOID pfn;                      // for debugging purposes only
    int ilNext;                     // for debugging purposes only
    int iilPrev;                    // for debugging purposes only
#endif // DBG
} LOCK, *PLOCK;

#define NEEDSSYNCPAINT(pwnd) TestWF(pwnd, WFSENDERASEBKGND | WFSENDNCPAINT)

typedef struct tagCVR       // cvr
{
    WINDOWPOS   pos;        // MUST be first field of CVR!
    int         xClientNew; // New client rectangle
    int         yClientNew;
    int         cxClientNew;
    int         cyClientNew;
    RECT        rcBlt;
    int         dxBlt;      // Distance blt rectangle is moving
    int         dyBlt;
    UINT        fsRE;       // RE_ flags: whether hrgnVisOld is empty or not
    HRGN        hrgnVisOld; // Previous visrgn
    PTHREADINFO pti;        // The thread this SWP should be processed on
    HRGN        hrgnClip;   // Window clipping region
    HRGN        hrgnInterMonitor;  // multimon support
} CVR, *PCVR;

/*
 * CalcValidRects() "Region Empty" flag values
 * A set bit indicates the corresponding region is empty.
 */
#define RE_VISNEW       0x0001  // CVR "Region Empty" flag values
#define RE_VISOLD       0x0002  // A set bit indicates the
#define RE_VALID        0x0004  // corresponding region is empty.
#define RE_INVALID      0x0008
#define RE_SPB          0x0010
#define RE_VALIDSUM     0x0020
#define RE_INVALIDSUM   0x0040

typedef struct tagSMWP {    // smwp
    HEAD           head;
    UINT           bShellNotify:1; // The acvr list contains shell notify flags
    UINT           bHandle:1;   // This is an HM object allocation -- See -BeginDeferWindowPos
    /*
     * All fields AFTER ccvr are preserved when reusing the global SMWP structure.
     */
    int            ccvr;        // Number of CVRs in the SWMP
    int            ccvrAlloc;   // Number of actual CVRs allocated in the SMWP
    PCVR           acvr;        // Pointer to array of CVR structures
} SMWP, *PSMWP;

void DestroySMWP(PSMWP psmwp);

/*
 * Clipboard data object definition
 */
typedef struct tagCLIPDATA {
    HEAD    head;
    DWORD   cbData;
    BYTE    abData[0];
} CLIPDATA, *PCLIPDATA;

/*
 * Private User Startupinfo
 */
typedef struct tagUSERSTARTUPINFO {
    DWORD   cb;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
} USERSTARTUPINFO, *PUSERSTARTUPINFO;

/*
 * TLBLOCK structure for multiple threads locking.
 */
#define THREADS_PER_TLBLOCK 16

typedef struct tagTLBLOCK {
    struct      tagTLBLOCK *ptlBlockPrev;
    struct {
        PTHREADINFO pti;
        TL          tlpti;
        DWORD       dwFlags;
#if DBG
        DWORD       dwUnlockedCount;
#endif
    } list[THREADS_PER_TLBLOCK];
} TLBLOCK, *PTLBLOCK;

/*
 * Keyboard File object
 */
typedef struct tagKBDFILE {
    HEAD               head;
    struct tagKBDFILE *pkfNext;   // next keyboard file
    WCHAR              awchKF[9]; // Name of Layout eg: L"00000409"
    HANDLE             hBase;     // base address of data
    PKBDTABLES         pKbdTbl;   // pointer to kbd layout data.
    ULONG              Size;      // Size of pKbdTbl
    PKBDNLSTABLES      pKbdNlsTbl; // pointer to kbd nls layout data.
} KBDFILE, *PKBDFILE;

/*
 * Keyboard Layout object
 */
typedef struct tagKL {   /* kl */
    HEAD          head;
    struct tagKL *pklNext;     // next in layout cycle
    struct tagKL *pklPrev;     // prev in layout cycle
    DWORD         dwKL_Flags;  // KL_* flags
    HKL           hkl;         // (Layout ID | Base Language ID)
    KBDFILE      *spkf;        // Keyboard Layout File
    DWORD         dwFontSigs;  // mask of FS_xxx bits - fonts that layout is good for
    UINT          iBaseCharset;// Charset value (Win95 compat) eg: ANSI_CHARSET
    WORD          CodePage;    // Windows Codepage of kbd layout, eg: 1252, 1250
    WCHAR         wchDiacritic;// Dead key saved here until next keystroke
    PIMEINFOEX    piiex;       // Extended information for IME based layout
} KL, *PKL;

/*
 * Flag values for KL dwFlags
 */
#define KL_UNLOADED 0x20000000
#define KL_RESET    0x40000000


PKL HKLtoPKL(PTHREADINFO pti, HKL hkl);

typedef struct tagKBDLANGTOGGLE
{
    BYTE bVkey;
    BYTE bScan;
    int  iBitPosition;
} KBDLANGTOGGLE;

/*
 * These constants are derived from combinations of
 * iBitPosition (refer to the LangToggle array defined
 * in globals.c).
 */

/*
 * This bit is used for both control and alt keys
 */
#define KLT_ALT              1

/*
 * This bit is used for the left shift key
 */
#define KLT_LEFTSHIFT        2

/*
 * This combination denotes ctrl/alt and the left shift key
 */
#define KLT_ALTLEFTSHIFT     3

/*
 * This bit is used for the right shift key
 */
#define KLT_RIGHTSHIFT       4

/*
 * This combination denotes ctrl/alt and the right shift key
 */
#define KLT_ALTRIGHTSHIFT    5

/*
 * This combination denotes ctrl/alt and both the shift keys
 */
#define KLT_ALTBOTHSHIFTS    7

/*
 * This value is used to mark invalid toggle key sequence
 */
#define KLT_NONE             8


/*
 * Desktop flags
 */
#define DF_DYING            0x80000000
#define DF_DESKWNDDESTROYED 0x40000000
#define DF_DESTROYED        0x20000000
#define DF_HOTTRACKING      0x10000000
#define DF_TOOLTIPSHOWING   0x08000000
#define DF_TOOLTIPACTIVE    0x04000000
#define DF_TOOLTIP          (DF_TOOLTIPACTIVE | DF_TOOLTIPSHOWING)
#define DF_TRACKMOUSELEAVE  0x02000000
#define DF_TRACKMOUSEHOVER  0x01000000
#define DF_TRACKMOUSEEVENT  (DF_TRACKMOUSELEAVE | DF_TRACKMOUSEHOVER)
#define DF_MOUSEMOVETRK     (DF_HOTTRACKING | DF_TOOLTIPACTIVE | DF_TRACKMOUSELEAVE | DF_TRACKMOUSEHOVER)
#define DF_MENUINUSE        0x00800000
#define DF_NEWDISPLAYSETTINGS 0x00400000
#define DF_DESKCREATED      0x00200000



#define CAPTIONTOOLTIPLEN   100

/*
 * Used to identify desktops uniquely for GDI
 */

#define GW_DESKTOP_ID 1

// #define DESKTOP_ALLOC_TRACE
#define DESKTOP_ALLOC_TRACE_SIZE    6

#if DBG
    typedef struct tagDbgAllocHead {
        DWORD    mark;
        DWORD    tag;
        PDESKTOP pdesk;
        SIZE_T   size;                  // the size of the allocation (doesn't include
                                        // this structure

        struct tagDbgAllocHead* pPrev;  // pointer to the previous allocation of this tag
        struct tagDbgAllocHead* pNext;  // pointer to the next allocation of this tag

#ifdef DESKTOP_ALLOC_TRACE
        PVOID  trace[DESKTOP_ALLOC_TRACE_SIZE];
#endif // DESKTOP_ALLOC_TRACE

    } DbgAllocHead, *PDbgAllocHead;
#endif // DBG

#define DTAG_CLASS              0x0001
#define DTAG_DESKTOPINFO        0x0002
#define DTAG_CLIENTTHREADINFO   0x0003
#define DTAG_TEXT               0x0004
#define DTAG_HANDTABL           0x0005
#define DTAG_SBINFO             0x0006
#define DTAG_MENUITEM           0x0007
#define DTAG_MENUTEXT           0x0008
#define DTAG_IMETEXT            0x0009
#define DTAG_PROPLIST           0x000A

/*
 * Desktop Structure.
 *
 *   This structure is only viewable from the kernel.  If any desktop
 *   information is needed in the client, then they should reference off
 *   the pDeskInfo field (i.e. pti->pDeskInfo).
 */
typedef struct tagDESKTOP {

    PDESKTOPINFO            pDeskInfo;         // Desktop information
    PDISPLAYINFO            pDispInfo;         //

    PDESKTOP                 rpdeskNext;       // Next desktop in list
    PWINDOWSTATION           rpwinstaParent;   // Windowstation owner

    DWORD                   dwDTFlags;         // Desktop flags
    ULONG                   dwDesktopId;       // Needed by GDI to tag display devices

    PWND                     spwndMenu;        //
    PMENU                    spmenuSys;        //
    PMENU                    spmenuDialogSys;  //
    PMENU                    spmenuHScroll;
    PMENU                    spmenuVScroll;
    PWND                     spwndForeground;  //
    PWND                     spwndTray;
    PWND                     spwndMessage;
    PWND                     spwndTooltip;

    HANDLE                  hsectionDesktop;   //
    PWIN32HEAP              pheapDesktop;      //
    DWORD                   dwConsoleThreadId; //
    DWORD                   dwConsoleIMEThreadId;
    CONSOLE_CARET_INFO      cciConsole;
    LIST_ENTRY              PtiList;           //

    PWND                    spwndTrack;        // xxxTrackMouseMove data
    int                     htEx;
    RECT                    rcMouseHover;
    DWORD                   dwMouseHoverTime;

    DWORD                   dwSessionId;

#ifdef LOGDESKTOPLOCKS
    int                     nLockCount;
    int                     nLogMax;
    int                     nLogCrt;
    PLogD                   pLog;
#endif // LOGDESKTOPLOCKS

} DESKTOP;

typedef struct tagDESKWND {
    WND   wnd;
    DWORD idProcess;
    DWORD idThread;
} DESKWND, *PDESKWND;

PVOID DesktopAlloc(PDESKTOP pdesk, UINT uSize,DWORD tag);

#define DesktopAllocAlways(pdesk, uSize, tag)   \
            Win32HeapAlloc(pdesk->pheapDesktop, uSize, tag, 0)

#define DesktopFree(pdesk, p)   Win32HeapFree(pdesk->pheapDesktop, p)

/*
 * Windowstation structure
 */
#define WSF_SWITCHLOCK          0x0001
#define WSF_OPENLOCK            0x0002
#define WSF_NOIO                0x0004
#define WSF_SHUTDOWN            0x0008
#define WSF_DYING               0x0010

#define WSF_REALSHUTDOWN        0x0020

typedef struct tagWINDOWSTATION {
    PWINDOWSTATION       rpwinstaNext;
    PDESKTOP             rpdeskList;

    PTERMINAL            pTerm;
    /*
     * Pointer to the currently active desktop for the window station.
     */
    DWORD                dwWSF_Flags;
    struct tagKL         *spklList;

    /*
     * Clipboard variables
     */
    PTHREADINFO          ptiClipLock;
    PTHREADINFO          ptiDrawingClipboard;
    PWND                 spwndClipOpen;
    PWND                 spwndClipViewer;
    PWND                 spwndClipOwner;
    struct tagCLIP       *pClipBase;
    int                  cNumClipFormats;
    UINT                 iClipSerialNumber;
    UINT                 iClipSequenceNumber;
    UINT                 fClipboardChanged : 1;
    UINT                 fInDelayedRendering : 1;

    /*
     * Global Atom table
     */
    PVOID                pGlobalAtomTable;

    LUID                 luidEndSession;
    LUID                 luidUser;
    PSID                 psidUser;
    PQ                   pqDesktop;

    DWORD                dwSessionId;

#if DBG
    PDESKTOP             pdeskCurrent;
#endif // DBG

} WINDOWSTATION;

typedef struct tagCAPTIONCACHE {
    PCURSOR         spcursor;
    POEMBITMAPINFO  pOem;
#if DBG
    HICON           hico;
#endif // DBG
} CAPTIONCACHE;

/*
 * Configurable icon and cursor stuff
 */
    typedef struct tagSYSCFGICO
    {
        WORD    Id;     // configurable id (OIC_ or OCR_ value)
        WORD    StrId;  // String ID for registry key name
        PCURSOR spcur;  // perminant cursor/icon pointer
    } SYSCFGICO;

    #define SYSICO(name) (gasysico[OIC_##name##_DEFAULT - OIC_FIRST_DEFAULT].spcur)
    #define SYSCUR(name) (gasyscur[OCR_##name##_DEFAULT - OCR_FIRST_DEFAULT].spcur)


/*
 * Accelerator Table structure
 */
typedef struct tagACCELTABLE {
    PROCOBJHEAD head;
    UINT        cAccel;
    ACCEL       accel[1];
} ACCELTABLE, *LPACCELTABLE;

/*
 * Besides the desktop window used by the current thread, we also
 * need to get the desktop window of a window and the input desktop
 * window.
 */
#define PWNDDESKTOP(p)      ((p)->head.rpdesk->pDeskInfo->spwnd)
#define PWNDMESSAGE(p)      ((p)->head.rpdesk->spwndMessage)
#define PWNDTOOLTIP(p)      ((p)->head.rpdesk->spwndTooltip)

/*
 * During window destruction, even a locked window can have a
 * NULL parent so use this macro where a NULL parent is a problem.
 */
#define PWNDPARENT(p) (p->spwndParent ? p->spwndParent : PWNDDESKTOP(p))

#define ISAMENU(pwwnd)       \
        (GETFNID(pwnd) == FNID_MENU)


/* NEW MENU STUFF */
typedef struct tagPOPUPMENU
{

  DWORD  fIsMenuBar:1;       /* This is a hacked struct which refers to the
                              * menu bar associated with a app. Only true if
                              * in the root ppopupMenuStruct.
                              */
  DWORD  fHasMenuBar:1;      /* This popup is part of a series which has a
                              * menu bar (either a sys menu or top level menu
                              * bar)
                              */
  DWORD  fIsSysMenu:1;    /* The system menu is here. */
  DWORD  fIsTrackPopup:1;    /* Is TrackPopup popup menu */
  DWORD  fDroppedLeft:1;
  DWORD  fHierarchyDropped:1;/* If true, a submenu has been dropped off this popup */
  DWORD  fRightButton:1;     /* Allow right button in menu */
  DWORD  fToggle:1;          /* If TRUE, button up cancels the popup */
  DWORD  fSynchronous:1;     /* For synchronous return value of cmd chosen */
  DWORD  fFirstClick:1;      /* Keep track if this was the first click on the
                              * top level menu bar item.  If the user down/up
                              * clicks on a top level menu bar item twice, we
                              * want to cancel menu mode.
                              */
  DWORD  fDropNextPopup:1;   /* Should we drop hierarchy of next menu item w/ popup? */
  DWORD  fNoNotify:1;        /* Don't send WM_ msgs to owner, except WM_COMMAND  */
  DWORD  fAboutToHide:1;     // Same purpose as fHideTimer?
  DWORD  fShowTimer:1;       // TRUE if the IDSYS_MNSHOW timer is set
  DWORD  fHideTimer:1;       // TRUE if the IDSYS_MNHIDE timer is set

  DWORD  fDestroyed:1;       /* Set when the owner menu window has been destroyed
                              *  so the popup can be freed once it's no longer needed
                              * Also set in root popupmenu when menu mode must end
                              */

  DWORD  fDelayedFree:1;    /* Avoid freeing the popup when the owner menu
                             *  window is destroyed.
                             * If set, it must be a root popupmenu or must
                             *  be linked in ppmDelayedFree
                             * This is eventually set for all hierarchical popups
                             */

  DWORD  fFlushDelayedFree:1; /* Used in root popupmenus only.
                               * Set when a hierarchical popup marked as fDelayedFree
                               *  has been destroyed.
                               */


  DWORD  fFreed:1;           /* Popup has been freed. Used for debug only */

  DWORD  fInCancel:1;        /* Popup has been passed to xxxMNCancel */

  DWORD  fTrackMouseEvent:1; /* TrackMouseEvent has been called */
  DWORD  fSendUninit:1;      /* Send WM_UNINITMENUPOPUP */
  DWORD  fRtoL:1;            /* For going backwards with the keys */
  DWORD  fDesktopMenu:1;     /* Set if this popup belons to the pdesk->spwndMenu window */
  DWORD  iDropDir:5;         /* Animation direction */


  PWND           spwndNotify;
                        /* Window who gets the notification messages. If this
                         * is a window with a menu bar, then this is the same
                         * as hwndPopupMenu.
                         */
  PWND           spwndPopupMenu;
                        /* The window associated with this ppopupMenu struct.
                         * If this is a top level menubar, then hwndPopupMenu
                         * is the window the menu bar. ie. it isn't really a
                         * popup menu window.
                         */
  PWND           spwndNextPopup;
                        /* The next popup in the hierarchy. Null if the last
                         * in chain
                         */
  PWND           spwndPrevPopup;
                        /* The previous popup in the hierarchy. NULL if at top
                         */
  PMENU          spmenu;/* The PMENU displayed in this window
                         */
  PMENU          spmenuAlternate;
                        /* Alternate PMENU. If the system menu is displayed,
                         * and a menubar menu exists, this will contain the
                         * menubar menu. If menubar menu is displayed, this
                         * will contain the system menu. Use only on top level
                         * ppopupMenu structs so that we can handle windows
                         * with both a system menu and a menu bar menu.  Only
                         * used in the root ppopupMenuStruct.
                         */

  PWND           spwndActivePopup; /* This is the popup the mouse/"keyboard focus" is on */

  PPOPUPMENU     ppopupmenuRoot;

  PPOPUPMENU     ppmDelayedFree;       /* List of hierarchical popups marked
                                        *  as fDelayedFree.
                                        */

  UINT   posSelectedItem;  /* Position of the selected item in this menu
                            */
  UINT   posDropped;

} POPUPMENU;

typedef struct tagMENUWND {
    WND wnd;
    PPOPUPMENU ppopupmenu;
} MENUWND, *PMENUWND;

/*
 * CheckPoint structure
 */
typedef struct tagCHECKPOINT {
    RECT rcNormal;
    POINT ptMin;
    POINT ptMax;
    DWORD fDragged:1;
    DWORD fWasMaximizedBeforeMinimized:1;
    DWORD fWasMinimizedBeforeMaximized:1;
    DWORD fMinInitialized:1;
    DWORD fMaxInitialized:1;
} CHECKPOINT, *PCHECKPOINT;


#define dpHorzRes           HORZRES
#define dpVertRes           VERTRES

/*
 * If the handle for CF_TEXT/CF_OEMTEXT is a dummy handle then this implies
 * that data is available in the other format (as CF_OEMTEXT/CF_TEXT)
 */
#define DUMMY_TEXT_HANDLE       0xFFFF
#define DATA_NOT_BANKED         0xFFFF

typedef struct tagCLIP {
    UINT    fmt;
    HANDLE  hData;
    BOOL    fGlobalHandle;
} CLIP, *PCLIP;

/*
 * DDEML instance structure
 */
typedef struct tagSVR_INSTANCE_INFO {
    THROBJHEAD head;
    struct tagSVR_INSTANCE_INFO *next;
    struct tagSVR_INSTANCE_INFO *nextInThisThread;
    DWORD afCmd;
    PWND spwndEvent;
    PVOID pcii;
} SVR_INSTANCE_INFO, *PSVR_INSTANCE_INFO;

typedef struct tagPUBOBJ {
    struct tagPUBOBJ *next;
    HANDLE hObj;
    int count;
    //
    // BUGBUG, MAKE FIELD USHORT!!!
    //
    W32PID pid;
} PUBOBJ, *PPUBOBJ;

/*
 * Defines for Menu focus
 */
#define FREEHOLD    0
#define MOUSEHOLD  -1 /* Mouse button held down and dragging */
#define KEYBDHOLD   1

/*
 * Structure definition for messages as they exist on a Q.  Same as MSG
 * structure except for the link-pointer and flags at the end.
 */
typedef struct tagQMSG {
    PQMSG           pqmsgNext;
    PQMSG           pqmsgPrev;
    MSG             msg;
    LONG_PTR        ExtraInfo;
    DWORD           dwQEvent;
    PTHREADINFO     pti;
} QMSG;

/*
 * dwQEvent values for QMSG structure.
 */
#define QEVENT_SHOWWINDOW           0x0001
#define QEVENT_CANCELMODE           0x0002
#define QEVENT_SETWINDOWPOS         0x0003
#define QEVENT_UPDATEKEYSTATE       0x0004
#define QEVENT_DEACTIVATE           0x0005
#define QEVENT_ACTIVATE             0x0006
#define QEVENT_POSTMESSAGE          0x0007  // Chicago
#define QEVENT_EXECSHELL            0x0008  // Chicago
#define QEVENT_CANCELMENU           0x0009  // Chicago
#define QEVENT_DESTROYWINDOW        0x000A
#define QEVENT_ASYNCSENDMSG         0x000B
#define QEVENT_HUNGTHREAD           0x000C
#define QEVENT_CANCELMOUSEMOVETRK   0x000D
#define QEVENT_NOTIFYWINEVENT       0x000E
#define QEVENT_RITACCESSIBILITY     0x000F
#define QEVENT_RITSOUND             0x0010
// QEvent for WM_APPCOMMAND messages - bug 339877
#define QEVENT_APPCOMMAND           0x0011

#define   RITSOUND_UPSIREN          0x0000
#define   RITSOUND_DOWNSIREN        0x0001
#define   RITSOUND_LOWBEEP          0x0002
#define   RITSOUND_HIGHBEEP         0x0003
#define   RITSOUND_KEYCLICK         0x0004
#define   RITSOUND_DOBEEP           0x0005

/*
 * xxxProcessEventMessage flags
 */
#define PEM_ACTIVATE_RESTORE        0x0001
#define PEM_ACTIVATE_NOZORDER       0x0002

typedef struct _MOVESIZEDATA {
    PWND            spwnd;
    RECT            rcDrag;
    RECT            rcDragCursor;
    RECT            rcParent;
    POINT           ptMinTrack;
    POINT           ptMaxTrack;
    RECT            rcWindow;
    int             dxMouse;
    int             dyMouse;
    int             cmd;
    int             impx;
    int             impy;
    POINT           ptRestore;
    UINT            fInitSize         : 1;    // should we initialize cursor pos
    UINT            fmsKbd            : 1;    // who knows
    UINT            fLockWindowUpdate : 1;    // whether screen was locked ok
    UINT            fTrackCancelled   : 1;    // Set if tracking ended by other thread.
    UINT            fForeground       : 1;    // whether the tracking thread is foreground
                                              //  and if we should draw the drag-rect
    UINT            fDragFullWindows  : 1;
    UINT            fOffScreen        : 1;
} MOVESIZEDATA, *PMOVESIZEDATA;

/*
 * DrawDragRect styles.
 */
#define DDR_START     0     // - start drag.
#define DDR_ENDACCEPT 1     // - end and accept
#define DDR_ENDCANCEL 2     // - end and cancel.


/*
 * Pseudo Event stuff.  (fManualReset := TRUE, fInitState := FALSE)
 */

DWORD WaitOnPseudoEvent(HANDLE *phE, DWORD dwMilliseconds);

#define PSEUDO_EVENT_ON     ((HANDLE)IntToPtr( 0xFFFFFFFF ))
#define PSEUDO_EVENT_OFF    ((HANDLE)IntToPtr( 0x00000000 ))
#define INIT_PSEUDO_EVENT(ph) *ph = PSEUDO_EVENT_OFF;

#define SET_PSEUDO_EVENT(phE)                                   \
    CheckCritIn();                                              \
    if (*(phE) == PSEUDO_EVENT_OFF) *(phE) = PSEUDO_EVENT_ON;   \
    else if (*(phE) != PSEUDO_EVENT_ON) {                       \
        KeSetEvent(*(phE), EVENT_INCREMENT, FALSE);             \
        ObDereferenceObject(*(phE));                            \
        *(phE) = PSEUDO_EVENT_ON;                               \
    }

#define RESET_PSEUDO_EVENT(phE)                                 \
    CheckCritIn();                                              \
    if (*(phE) == PSEUDO_EVENT_ON) *(phE) = PSEUDO_EVENT_OFF;   \
    else if (*(phE) != PSEUDO_EVENT_OFF) {                      \
        KeClearEvent(*(phE));                                   \
    }

#define CLOSE_PSEUDO_EVENT(phE)                                 \
    CheckCritIn();                                              \
    if (*(phE) == PSEUDO_EVENT_ON) *(phE) = PSEUDO_EVENT_OFF;   \
    else if (*(phE) != PSEUDO_EVENT_OFF) {                      \
        KeSetEvent(*(phE), EVENT_INCREMENT, FALSE);             \
        ObDereferenceObject(*(phE));                            \
        *(phE) = PSEUDO_EVENT_OFF;                              \
    }

typedef struct tagMLIST {
    PQMSG pqmsgRead;
    PQMSG pqmsgWriteLast;
    DWORD cMsgs;
} MLIST, *PMLIST;

/*
 * Message Queue structure.
 *
 * Note, if you need to add a WORD sized value,
 * do so after xbtnDblClk.
 */
typedef struct tagQ {
    MLIST       mlInput;            // raw mouse and key message list.

    PTHREADINFO ptiSysLock;         // Thread currently allowed to process input
    ULONG_PTR    idSysLock;          // Last message removed
    ULONG_PTR    idSysPeek;          // Last message peeked

    PTHREADINFO ptiMouse;           // Last thread to get mouse msg.
    PTHREADINFO ptiKeyboard;

    PWND        spwndCapture;
    PWND        spwndFocus;
    PWND        spwndActive;
    PWND        spwndActivePrev;

    UINT        codeCapture;
    UINT        msgDblClk;
    WORD        xbtnDblClk;
    DWORD       timeDblClk;
    HWND        hwndDblClk;
    POINT       ptDblClk;

    BYTE        afKeyRecentDown[CBKEYSTATERECENTDOWN];
    BYTE        afKeyState[CBKEYSTATE];

    CARET       caret;

    PCURSOR     spcurCurrent;
    int         iCursorLevel;

    DWORD       QF_flags;            // QF_ flags go here

    USHORT      cThreads;            // Count of threads using this queue
    USHORT      cLockCount;          // Count of threads that don't want this queue freed

    UINT        msgJournal;
    LONG_PTR    ExtraInfo;
} Q;

/*
 * Used for zzzAttachThreadInput()
 */
typedef struct tagATTACHINFO {
    struct tagATTACHINFO *paiNext;
    PTHREADINFO pti1;
    PTHREADINFO pti2;
} ATTACHINFO, *PATTACHINFO;

#define POLL_EVENT_CNT 5

#define IEV_IDLE    0
#define IEV_INPUT   1
#define IEV_EXEC    2
#define IEV_TASK    3
#define IEV_WOWEXEC 4


typedef struct tagWOWTHREADINFO {
    struct tagWOWTHREADINFO *pwtiNext;
    DWORD    idTask;                // WOW task id
    ULONG_PTR idWaitObject;          // pseudo handle returned to parent
    DWORD    idParentProcess;       // process that called CreateProcess
    PKEVENT  pIdleEvent;            // event that WaitForInputIdle will wait on
} WOWTHREADINFO, *PWOWTHREADINFO;

/*
 * Task Data Block structure.
 */
typedef struct tagTDB {
    PTDB            ptdbNext;
    int             nEvents;
    int             nPriority;
    PTHREADINFO     pti;
    PWOWTHREADINFO  pwti;               // per thread info for shared Wow
    WORD            hTaskWow;           // Wow cookie to find apps during shutdown
    WORD            TDB_Flags;             //  bit 0 means setup app
} TDB;

#define TDBF_SETUP 1


/*
 * Hack message for shell to tell them a setup app is exiting.
 * This message is defined in \nt\private\shell\inc, but I really
 * don't want to introduce that dependency in the build.  DavidDS
 * has put a check in that file to make sure that the value does not
 * change and refers to this usage.  FritzS
 */
#define DTM_SETUPAPPRAN (WM_USER+90)

/*
 * Menu animation GDI objects.
 */
typedef struct tagMENUANIDC
{
    HDC     hdcAni;         // Scratch dc for animation
} MENUANIDC;

/*
 * Menu Control Structure
 */
typedef struct tagMENUSTATE {
    PPOPUPMENU pGlobalPopupMenu;
    DWORD   fMenuStarted : 1;
    DWORD   fIsSysMenu : 1;
    DWORD   fInsideMenuLoop : 1;
    DWORD   fButtonDown:1;
    DWORD   fInEndMenu:1;
    DWORD   fUnderline:1;               /* Shorcut key underlines are shown */
    DWORD   fButtonAlwaysDown:1;        /* The mouse has always been down since the menu started */
    DWORD   fDragging:1;                /* Dragging (in DoDragDrop) or about to */
    DWORD   fModelessMenu:1;            /* No modal loop */
    DWORD   fInCallHandleMenuMessages:1;/* processing a msg from CallHandleMM */
    DWORD   fDragAndDrop:1;             /* This menu can do drag and drop */
    DWORD   fAutoDismiss:1;             /* This menu goes away on its own if mouse is off for certain time */
    DWORD   fAboutToAutoDismiss:1;      /* Autodismiss will take place when timer goes off */
    DWORD   fIgnoreButtonUp:1;          /* Eat next button up, i.e, cancel dragging */
    DWORD   fMouseOffMenu:1;            /* Mouse is off the menu - modeless menus only */
    DWORD   fInDoDragDrop:1;            /* in a WM_MENUDODRAGDROP callback */
    DWORD   fActiveNoForeground:1;      /* A menu window is active but we're not in the foreground */
    DWORD   fNotifyByPos:1;             /* Use WM_MENUCOMMAND */
    DWORD   fSetCapture:1;              /* True if the menu mode set capture */
    DWORD   iAniDropDir:5;              /* direction of animation */

    POINT   ptMouseLast;
    int     mnFocus;
    int     cmdLast;
    PTHREADINFO ptiMenuStateOwner;

    DWORD   dwLockCount;

    struct tagMENUSTATE *pmnsPrev;      /* Previous menustate for nested/context menus */

    POINT   ptButtonDown;               /* Mouse down position (begin drag position) */
    ULONG_PTR uButtonDownHitArea;        /* Return from xxxMNFindWindowFromPoint on button down */
    UINT    uButtonDownIndex;           /* Index of the item being dragged */

    int     vkButtonDown;               /* Mouse button being dragged */

    ULONG_PTR uDraggingHitArea;          /* Last hit area while InDoDragDrop */
    UINT    uDraggingIndex;             /* Last index  */
    UINT    uDraggingFlags;             /* Gap flags */

    HDC     hdcWndAni;      // window DC while animating
    DWORD   dwAniStartTime; // starting time of animation
    int     ixAni;          // current x-step in animation
    int     iyAni;          // current y-step in animation
    int     cxAni;          // total x in animation
    int     cyAni;          // total y in animation
    HBITMAP hbmAni;         // Scratch bmp for animation.

    /*
     * Important: The following structure must be the last
     *  thing in tagMENUSTATE. MNAllocMenuState doesn't NULL out
     *  this structure
     */
    MENUANIDC;

} MENUSTATE, *PMENUSTATE;

typedef struct tagLASTINPUT {  /* linp */
    DWORD timeLastInputMessage;
    DWORD dwFlags;
    PTHREADINFO ptiLastWoken;  /* Last thread woken by key or click  */
                               /* It can be NULL                     */
    POINT ptLastClick;         /* point of the last mouse click      */
} LASTINPUT, PLASTINPUT;

#define LINP_KEYBOARD       0x00000001
#define LINP_SCREENSAVER    0x00000002
#define LINP_LOWPOWER       0x00000004
#define LINP_POWEROFF       0x00000008
#define LINP_POWERTIMEOUTS  (LINP_LOWPOWER | LINP_POWEROFF)
#define LINP_INPUTTIMEOUTS  (LINP_SCREENSAVER | LINP_LOWPOWER | LINP_POWEROFF)

/*
 * Menu data to be passed to xxxRealDrawMenuItem from xxxDrawState
 */
typedef struct {
    PMENU pMenu;
    PITEM pItem;
} GRAYMENU;
typedef GRAYMENU *PGRAYMENU;


#define IS_THREAD_RESTRICTED(pti, r)                            \
    ((pti->TIF_flags & TIF_RESTRICTED) ?                        \
        (pti->ppi->pW32Job->restrictions & (r)) :               \
        FALSE)

#define IS_CURRENT_THREAD_RESTRICTED(r)                         \
    ((PtiCurrent()->TIF_flags & TIF_RESTRICTED) ?               \
        (PtiCurrent()->ppi->pW32Job->restrictions & (r)) :      \
        FALSE)

/*
 * These types are needed before they are fully defined.
 */
typedef struct tagSMS               * KPTR_MODIFIER PSMS;

/*
 * Make sure this structure matches up with W32THREAD, since they're
 * really the same thing.
 */

/*
 * NOTE -- this structure has been sorted (roughly) in order of use
 * of the fields.   The x86 code set allows cheaper access to fields
 * that are in the first 0x80 bytes of a structure.  Please attempt
 * to ensure that frequently-used fields are below this boundary.
 *          FritzS
 */

typedef struct tagTHREADINFO {
    W32THREAD;

//***************************************** begin: USER specific fields

    PTL             ptl;                // Listhead for thread lock list

    PPROCESSINFO    ppi;                // process info struct for this thread

    PQ              pq;                 // keyboard and mouse input queue

    PKL             spklActive;         // active keyboard layout for this thread

    PCLIENTTHREADINFO pcti;             // Info that must be visible from client

    PDESKTOP        rpdesk;
    PDESKTOPINFO    pDeskInfo;          // Desktop info visible to client
    PCLIENTINFO     pClientInfo;        // Client info stored in TEB

    DWORD           TIF_flags;          // TIF_ flags go here.

    PUNICODE_STRING pstrAppName;        // Application module name.

    PSMS            psmsSent;           // Most recent SMS this thread has sent
    PSMS            psmsCurrent;        // Received SMS this thread is currently processing
    PSMS            psmsReceiveList;    // SMSs to be processed

    LONG            timeLast;           // Time, position, and ID of last message
    ULONG_PTR        idLast;

    int             cQuit;
    int             exitCode;

    HDESK           hdesk;              // Desktop handle
    int             cPaintsReady;
    UINT            cTimersReady;

    PMENUSTATE      pMenuState;

    union {
        PTDB            ptdb;           // Win16Task Schedule data for WOW thread
        PWINDOWSTATION  pwinsta;        // Window station for SYSTEM thread
    };

    PSVR_INSTANCE_INFO psiiList;        // thread DDEML instance list
    DWORD           dwExpWinVer;
    DWORD           dwCompatFlags;      // The Win 3.1 Compat flags
    DWORD           dwCompatFlags2;     // new DWORD to extend compat flags for NT5+ features

    PQ              pqAttach;           // calculation variabled used in
                                        // zzzAttachThreadInput()

    PTHREADINFO     ptiSibling;         // pointer to sibling thread info

    PMOVESIZEDATA   pmsd;

    DWORD           fsHooks;                // WHF_ Flags for which hooks are installed
    PHOOK           sphkCurrent;            // Hook this thread is currently processing

    PSBTRACK        pSBTrack;

    HANDLE          hEventQueueClient;
    PKEVENT         pEventQueueServer;
    LIST_ENTRY      PtiLink;            // Link to other threads on desktop
    int             iCursorLevel;       // keep track of each thread's level
    POINT           ptLast;

    PWND            spwndDefaultIme;    // Default IME Window for this thread
    PIMC            spDefaultImc;       // Default input context for this thread
    HKL             hklPrev;            // Previous active keyboard layout
    int             cEnterCount;
    MLIST           mlPost;             // posted message list.
    USHORT          fsChangeBitsRemoved;// Bits removed during PeekMessage
    WCHAR           wchInjected;        // character from last VK_PACKET
    DWORD           fsReserveKeys;      // Keys that must be sent to the active
                                        // active console window.
    PKEVENT        *apEvent;            // Wait array for xxxPollAndWaitForSingleObject
    ACCESS_MASK     amdesk;             // Granted desktop access
    UINT            cWindows;           // Number of windows owned by this thread
    UINT            cVisWindows;        // Number of visible windows on this thread

    PHOOK           aphkStart[CWINHOOKS];   // Hooks registered for this thread
    CLIENTTHREADINFO  cti;              // Use this when no desktop is available
} THREADINFO;

#define PWNDTOPSBTRACK(pwnd) (((GETPTI(pwnd)->pSBTrack)))

/*
 * The number of library module handles we can store in the dependency
 * tables.  If this exceeds 32, the load mask implementation must be
 * changed.
 */
#define CLIBS           32

/*
 * Process Info structure.
 */
typedef struct tagWOWPROCESSINFO {
    struct tagWOWPROCESSINFO *pwpiNext; // List of WOW ppi's, gppiFirstWow is head
    PTHREADINFO ptiScheduled;           // current thread in nonpreemptive scheduler
    DWORD       nTaskLock;              // nonpreemptive scheduler task lock count
    PTDB        ptdbHead;               // list of this process's WOW tasks
    PVOID       lpfnWowExitTask;        // func addr for wow exittask callback
    PKEVENT     pEventWowExec;          // WowExec Virt HWint scheduler event
    HANDLE      hEventWowExecClient;    // client handle value for wowexec
    DWORD       nSendLock;              // Send Scheduler inter process Send count
    DWORD       nRecvLock;              // Send Scheduler inter process Receive count
    PTHREADINFO CSOwningThread;         // Pseudo Wow CritSect ClientThreadId
    LONG        CSLockCount;            // Pseudo Wow CritSect LockCount
}WOWPROCESSINFO, *PWOWPROCESSINFO;

typedef struct tagDESKTOPVIEW {
    struct tagDESKTOPVIEW *pdvNext;
    PDESKTOP              pdesk;
    ULONG_PTR              ulClientDelta;
} DESKTOPVIEW, *PDESKTOPVIEW;


/*
 * number of DWORDs in ppi->pgh
 */
#define GH_SIZE  8

/*
 * the delta allocation for ppiTable array in W32JOB structure.
 */
#define JP_DELTA  4

/*
 * W32JOB structure
 */
typedef struct tagW32JOB {
    struct tagW32JOB*  pNext;           // next W32JOB structure
    PEJOB              Job;             // pointer to the EJOB structure
    PVOID              pAtomTable;      // the atom table for the job object
    DWORD              restrictions;    // UI restrictions
    UINT               uProcessCount;   // number of processes in ppiTable
    UINT               uMaxProcesses;   // how much room is in ppiTable
    PPROCESSINFO*      ppiTable;        // the array of processes contained in the job
    UINT               ughCrt;          // crt number of handles in pgh
    UINT               ughMax;          // number of handles pgh can store
    PULONG_PTR          pgh;             // the granted handles table
} W32JOB, *PW32JOB;


/*
 * Make sure this structure matches up with               W32PROCESS, since they're
 * really the same thing.
 */

/*
 * NOTE -- this structure has been sorted (roughly) in order of use
 * of the fields.   The x86 code set allows cheaper access to fields
 * that are in the first 0x80 bytes of a structure.  Please attempt
 * to ensure that frequently-used fields are below this boundary.
 *          FritzS
 */


typedef struct tagPROCESSINFO {
    W32PROCESS;
//***************************************** begin: USER specific fields
    PTHREADINFO     ptiList;                    // threads in this process
    PTHREADINFO     ptiMainThread;              // pti of "main thread"
    PDESKTOP        rpdeskStartup;              // initial desktop
    PCLS            pclsPrivateList;            // this processes' private classes
    PCLS            pclsPublicList;             // this processes' public classes
    PWOWPROCESSINFO pwpi;                       // Wow PerProcess Info

    PPROCESSINFO    ppiNext;                    // next ppi structure in start list
    PPROCESSINFO    ppiNextRunning;
    int             cThreads;                   // count of threads using this process info
    HDESK           hdeskStartup;               // initial desktop handle
    UINT            cSysExpunge;                // sys expunge counter
    DWORD           dwhmodLibLoadedMask;        // bits describing loaded hook dlls
    HANDLE          ahmodLibLoaded[CLIBS];      // process unique hmod array for hook dlls
    struct          tagWINDOWSTATION *rpwinsta; // process windowstation
    HWINSTA         hwinsta;                    // windowstation handle
    ACCESS_MASK     amwinsta;                   // windowstation accesses

    DWORD           dwHotkey;                   // hot key from progman
    HMONITOR        hMonitor;                   // monitor handle from CreateProcess
    PDESKTOPVIEW    pdvList;                    // list of desktop views
    UINT            iClipSerialNumber;          // clipboard serial number
    RTL_BITMAP      bmHandleFlags;              // per handle flags
    PCURSOR         pCursorCache;               // process cursor/icon cache
    PVOID           pClientBase;                // LEAVE THIS FOR HYDRA; offset to the shared section
    DWORD           dwLpkEntryPoints;           // user mode language pack installed

    PW32JOB         pW32Job;                    // pointer to the W32JOB structure

    DWORD           dwImeCompatFlags;           // per-process Ime Compatibility flags
    LUID            luidSession;                // logon session id
    USERSTARTUPINFO usi;                        // process startup info

#ifdef VALIDATEHANDLEQUOTA
    LONG lHandles;
#endif

#ifdef USE_MIRRORING
    DWORD           dwLayout;                   // the default Window orientation for this process
#endif

} PROCESSINFO;

/*
 * Bit definitions for dwLpkEntryPoints in the processinfo structure.
 * These are passed from the client side when an lpk is registered.
 * The kernel determines when to perform callbacks based on which
 * entry points an lpk supports.
 */
#define LPK_TABBEDTEXTOUT 0x01
#define LPK_PSMTEXTOUT    0x02
#define LPK_DRAWTEXTEX    0x04
#define LPK_EDITCONTROL   0x08
#define LPK_INSTALLED     0x0f

#define CALL_LPK(ptiCurrent)  ((PpiCurrent()->dwLpkEntryPoints & LPK_INSTALLED) && \
                               !((ptiCurrent)->TIF_flags & (TIF_INCLEANUP | TIF_SYSTEMTHREAD)))

/*
 * This is used to send cool switch windows information
 * to the lpk
 */
typedef struct _LPKDRAWSWITCHWND {
    RECT rcRect;
    LARGE_UNICODE_STRING strName;
} LPKDRAWSWITCHWND;

/*
 * DC cache entry structure (DCE)
 *
 *   This structure identifies an entry in the DCE cache.  It is
 *   usually initialized at GetDCEx() and cleanded during RelaseCacheDC
 *   calls.
 *
 *   Field
 *   -----
 *
 *   pdceNext       - Pointer to the next DCE entry.
 *
 *
 *   hdc            - GDI DC handle for the dce entry.  This will have
 *                    the necessary clipping regions selected into it.
 *
 *   pwndOrg        - Identifies the window in the GetDCEx() call which owns
 *                    the DCE Entry.
 *
 *   pwndClip       - Identifies the window by which the DC is clipped to.
 *                    This is usually done for PARENTDC windows.
 *
 *   hrgnClip       - This region is set if the caller to GetDCEx() passes a
 *                    clipping region in which to intersect with the visrgn.
 *                    This is used when we need to recalc the visrgn for the
 *                    DCE entry.  This will be freed at ReleaseCacheDC()
 *                    time if the flag doesn't have DCX_NODELETERGN set.
 *
 *   hrgnClipPublic - This is a copy of the (hrgnClip) passed in above.  We
 *                    make a copy and set it as PUBLIC ownership so that
 *                    we can use it in computations during the UserSetDCVisRgn
 *                    call.  This is necessary for Full-Hung-Draw where we
 *                    are drawing from a different process then the one
 *                    who created the (hrgnClip).  This is always deleted
 *                    in the ReleaseCacheDC() call.
 *
 *   hrgnSavedVis   - This is a copy of the saved visrgn for the DCE entry.
 *
 *   flags          - DCX_ flags.
 *
 *   ptiOwner       - Thread owner of the DCE entry.
 *
 */
typedef struct tagDCE {
    PDCE                 pdceNext;
    HDC                  hdc;
    PWND                 pwndOrg;
    PWND                 pwndClip;
    HRGN                 hrgnClip;
    HRGN                 hrgnClipPublic;
    HRGN                 hrgnSavedVis;
    DWORD                DCX_flags;
    PTHREADINFO          ptiOwner;
    PMONITOR             pMonitor;
} DCE;

#define DCE_SIZE_CACHEINIT        5    // Initial number of DCEs in the cache.
#define DCE_SIZE_CACHETHRESHOLD  32    // Number of dce's as a threshold.

#define DCE_RELEASED              0    // ReleaseDC released
#define DCE_FREED                 1    // ReleaseDC freed
#define DCE_NORELEASE             2    // ReleaseDC in-use.

/*
 * CalcVisRgn DC type bits
 */
#define DCUNUSED        0x00        /* Unused cache entry */
#define DCC             0x01        /* Client area */
#define DCW             0x02        /* Window area */
#define DCSAVEDVISRGN   0x04
#define DCCLIPRGN       0x08
#define DCNOCHILDCLIP   0x10        /* Nochildern clip */
#define DCSAVEVIS       0x20        /* Save visrgn before calculating */
#define DCCACHE         0x40

/*
 * Window List Structure
 */
typedef struct tagBWL {
    struct tagBWL *pbwlNext;
    HWND          *phwndNext;
    HWND          *phwndMax;
    PTHREADINFO   ptiOwner;
    HWND          rghwnd[1];
} BWL, *PBWL;

/*
 * Numbers of HWND slots to to start with and to increase by.
 */
#define BWL_CHWNDINIT      32     /* initial # slots pre-allocated */
#define BWL_CHWNDMORE       8     /* # slots to obtain when required */

#define BWL_ENUMCHILDREN    1
#define BWL_ENUMLIST        2
#define BWL_ENUMOWNERLIST   4

#define BWL_ENUMIMELAST     0x08
#define BWL_REMOVEIMECHILD  0x10

/*
 * Saved Popup Bits structure
 */
typedef struct tagSPB {
    struct tagSPB *pspbNext;
    PWND          spwnd;
    HBITMAP       hbm;
    RECT          rc;
    HRGN          hrgn;
    DWORD         flags;
    ULONG_PTR     ulSaveId;
} SPB;

#define SPB_SAVESCREENBITS  0x0001  // GreSaveScreenBits() was called
#define SPB_LOCKUPDATE      0x0002  // LockWindowUpdate() SPB
#define SPB_DRAWBUFFER      0x0004  // BeginDrawBuffer() SPB

#define AnySpbs()   (gpDispInfo->pspbFirst != NULL)     // TRUE if there are any SPBs

/*
 * Macro to check if the journal playback hook is installed.
 */
#define FJOURNALRECORD()    (GETDESKINFO(PtiCurrent())->aphkStart[WH_JOURNALRECORD + 1] != NULL)
#define FJOURNALPLAYBACK()  (GETDESKINFO(PtiCurrent())->aphkStart[WH_JOURNALPLAYBACK + 1] != NULL)

#define TESTHMODLOADED(pti, x)       ((pti)->ppi->dwhmodLibLoadedMask & (1 << (x)))
#define SETHMODLOADED(pti, x, hmod)  ((pti)->ppi->ahmodLibLoaded[x] = hmod, \
                                      (pti)->ppi->dwhmodLibLoadedMask |= (1 << (x)))
#define CLEARHMODLOADED(pti, x)      ((pti)->ppi->ahmodLibLoaded[x] = NULL, \
                                      (pti)->ppi->dwhmodLibLoadedMask &= ~(1 << (x)))
#define PFNHOOK(phk) (phk->ihmod == -1 ? (PROC)phk->offPfn : \
        (PROC)(((ULONG_PTR)(PtiCurrent()->ppi->ahmodLibLoaded[phk->ihmod])) + \
        ((ULONG_PTR)(phk->offPfn))))

/*
 * Extended structures for message thunking.
 */
typedef struct _CREATESTRUCTEX {
    CREATESTRUCT cs;
    LARGE_STRING strName;
    LARGE_STRING strClass;
} CREATESTRUCTEX, *PCREATESTRUCTEX;

typedef struct _MDICREATESTRUCTEX {
    MDICREATESTRUCT mdics;
    LARGE_STRING strTitle;
    LARGE_STRING strClass;
} MDICREATESTRUCTEX, *PMDICREATESTRUCTEX;

typedef struct _CWPSTRUCTEX {
    struct tagCWPSTRUCT;
    PSMS            psmsSender;
} CWPSTRUCTEX, *PCWPSTRUCTEX;

typedef struct _CWPRETSTRUCTEX {
    LRESULT         lResult;
    struct tagCWPSTRUCT;
    PSMS            psmsSender;
} CWPRETSTRUCTEX, *PCWPRETSTRUCTEX;

/*
 * SendMessage structure and defines.
 */
typedef struct tagSMS {   /* sms */
    PSMS            psmsNext;          // link in global psmsList
#if DBG
    PSMS            psmsSendList;      // head of queue's SendMessage chain
    PSMS            psmsSendNext;      // link in queue's SendMessage chain
#endif // DBG
    PSMS            psmsReceiveNext;   // link in queue's ReceiveList
    DWORD           tSent;              // time message was sent
    PTHREADINFO     ptiSender;          // sending thread
    PTHREADINFO     ptiReceiver;        // receiving thread

    SENDASYNCPROC   lpResultCallBack;   // function to receive the SendMessageCallback return value
    ULONG_PTR        dwData;             // value to be passed back to the lpResultCallBack function
    PTHREADINFO     ptiCallBackSender;  // sending thread

    LRESULT         lRet;               // message return value
    UINT            flags;              // SMF_ flags
    WPARAM          wParam;             // message fields...
    LPARAM          lParam;
    UINT            message;
    PWND            spwnd;
    PVOID           pvCapture;          // captured argument data
} SMS;

#define SMF_REPLY                   0x0001      // message has been replied to
#define SMF_RECEIVERDIED            0x0002      // receiver has died
#define SMF_SENDERDIED              0x0004      // sender has died
#define SMF_RECEIVERFREE            0x0008      // receiver should free sms when done
#define SMF_RECEIVEDMESSAGE         0x0010      // sms has been received
#define SMF_CB_REQUEST              0x0100      // SendMessageCallback requested
#define SMF_CB_REPLY                0x0200      // SendMessageCallback reply
#define SMF_CB_CLIENT               0x0400      // Client process request
#define SMF_CB_SERVER               0x0800      // Server process request
#define SMF_WOWRECEIVE              0x1000      // wow sched has incr recv count
#define SMF_WOWSEND                 0x2000      // wow sched has incr send count
#define SMF_RECEIVERBUSY            0x4000      // reciver is processing this msg

/*
 * InterSendMsgEx parameter used for SendMessageCallback and TimeOut
 */
typedef struct tagINTERSENDMSGEX {   /* ism */
    UINT   fuCall;                      // callback or timeout call

    SENDASYNCPROC lpResultCallBack;     // function to receive the send message value
    ULONG_PTR dwData;                    // Value to be passed back to the SendResult call back function
    LRESULT lRet;                       // return value from the send message

    UINT fuSend;                        // how to send the message, SMTO_BLOCK, SMTO_ABORTIFHUNG
    UINT uTimeout;                      // time-out duration
    PULONG_PTR lpdwResult;               // the return value for a syncornis call
} INTRSENDMSGEX, *PINTRSENDMSGEX;

#define ISM_CALLBACK        0x0001      // callback function request
#define ISM_TIMEOUT         0x0002      // timeout function request
#define ISM_REQUEST         0x0010      // callback function request message
#define ISM_REPLY           0x0020      // callback function reply message
#define ISM_CB_CLIENT       0x0100      // client process callback function

/*
 * Event structure to handle broadcasts of notification messages.
 */
typedef struct tagASYNCSENDMSG {
    WPARAM  wParam;
    LPARAM  lParam;
    UINT    message;
    HWND    hwnd;
} ASYNCSENDMSG, *PASYNCSENDMSG;

/*
 * HkCallHook() structure
 */
#define IsHooked(pti, fsHook) \
    ((fsHook & (pti->fsHooks | pti->pDeskInfo->fsHooks)) != 0)

#define IsGlobalHooked(pti, fsHook) \
    ((fsHook & pti->pDeskInfo->fsHooks) != 0)

typedef struct tagHOOKMSGSTRUCT { /* hch */
    PHOOK   phk;
    int     nCode;
    LPARAM  lParam;
} HOOKMSGSTRUCT, *PHOOKMSGSTRUCT;

/*
 * BroadcastMessage() commands.
 */
#define BMSG_SENDMSG                0x0000
#define BMSG_SENDNOTIFYMSG          0x0001
#define BMSG_POSTMSG                0x0002
#define BMSG_SENDMSGCALLBACK        0x0003
#define BMSG_SENDMSGTIMEOUT         0x0004
#define BMSG_SENDNOTIFYMSGPROCESS   0x0005

/*
 * xxxBroadcastMessage parameter used for SendMessageCallback and TimeOut
 */
typedef union tagBROADCASTMSG {   /* bcm */
     struct {                               // for callback broadcast
         SENDASYNCPROC lpResultCallBack;    // function to receive the send message value
         ULONG_PTR dwData;                   // Value to be passed back to the SendResult call back function
         BOOL bClientRequest;               // if a cliet or server callback request
     } cb;
     struct {                               // for timeout broadcast
         UINT fuFlags;                      // timeout type flags
         UINT uTimeout;                     // timeout length
         PULONG_PTR lpdwResult;              // where to put the return value
     } to;
} BROADCASTMSG, *PBROADCASTMSG;

/*
 * Internal hotkey structures and defines.
 */
typedef struct tagHOTKEY {
    PTHREADINFO pti;
    PWND    spwnd;
    WORD    fsModifiers; // MOD_SHIFT, MOD_ALT, MOD_CONTROL, MOD_WIN
    WORD    wFlags;      // MOD_SAS
    UINT    vk;
    int     id;
    struct tagHOTKEY *phkNext;
} HOTKEY, *PHOTKEY;

#define PWND_INPUTOWNER (PWND)1    // Means send WM_HOTKEY to input owner.
#define PWND_FOCUS      (PWND)NULL // Means send WM_HOTKEY to queue's pwndFocus.
#define PWND_ERROR      (PWND)0x10  // Means HWND validation returned an error
#define PWND_TOP        (PWND)0
#define PWND_BOTTOM     (PWND)1
#define PWND_GROUPTOTOP ((PWND)-1)
#define PWND_TOPMOST    ((PWND)-1)
#define PWND_NOTOPMOST  ((PWND)-2)
#define PWND_BROADCAST  ((PWND)-1)

#define IDHOT_DEBUG         (-5)
#define IDHOT_DEBUGSERVER   (-6)
#define IDHOT_WINDOWS       (-7)

/*
 * xPos, yPos for WM_CONTEXTMENU from keyboard
 */
#define KEYBOARD_MENU   ((LPARAM)-1)    // Keyboard generated menu

/*
 * Capture codes
 */
#define NO_CAP_CLIENT           0   /* no capture; in client area */
#define NO_CAP_SYS              1   /* no capture; in sys area */
#define CLIENT_CAPTURE          2   /* client-relative capture */
#define WINDOW_CAPTURE          3   /* window-relative capture */
#define SCREEN_CAPTURE          4   /* screen-relative capture */
#define FULLSCREEN_CAPTURE      5   /* capture entire machine */
#define CLIENT_CAPTURE_INTERNAL 6   /* client-relative capture (Win 3.1 style; won't release) */

#define CH_HELPPREFIX   0x08

#ifdef KANJI
    #define CH_KANJI1       0x1D
    #define CH_KANJI2       0x1E
    #define CH_KANJI3       0x1F
#endif // KANJI

#define xxxRedrawScreen() \
        xxxInternalInvalidate(PtiCurrent()->rpdesk->pDeskInfo->spwnd, \
        HRGN_FULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN)

/*
 * Preallocated buffer for use during SetWindowPos to prevent memory
 * allocation failures.
 */
#define CCVR_WORKSPACE      4

/*
 * DrawIconCallBack data, global only for state data in tmswitch.c
 */
typedef struct tagDRAWICONCB {   /* dicb */
    PWND   pwndTop;                     // Window being drawn
    UINT   cx;                          // x offset for icon
    UINT   cy;                          // y offset for icon
} DRAWICONCB, *PDRAWICONCB;

/*
 * The following defines the components of nKeyboardSpeed
 */
#define KSPEED_MASK     0x001F          // Defines the key repeat speed.
#define KDELAY_MASK     0x0060          // Defines the keyboard delay.
#define KDELAY_SHIFT    5

/*
 * Property list checkpoint int
 */
#define PROP_CHECKPOINT     MAKEINTATOM(atomCheckpointProp)
#define PROP_DDETRACK       MAKEINTATOM(atomDDETrack)
#define PROP_QOS            MAKEINTATOM(atomQOS)
#define PROP_DDEIMP         MAKEINTATOM(atomDDEImp)
#define PROP_WNDOBJ         MAKEINTATOM(atomWndObj)
#define PROP_IMELEVEL       MAKEINTATOM(atomImeLevel)
#define PROP_LAYER          MAKEINTATOM(atomLayer)

#define WinFlags    ((WORD)(&__WinFlags))

/*
 * ntinput.c
 */
VOID xxxInternalKeyEventDirect(
    BYTE  bVk,
    WORD  wScan,
    DWORD dwFlags,
    DWORD dwTime,
    ULONG_PTR dwExtraInfo);

UINT xxxSendInput(
    UINT    nInputs,
    LPINPUT pInputs);

BOOL _BlockInput(
    BOOL    fBlockIt);

int _GetMouseMovePointsEx(
    CONST MOUSEMOVEPOINT* ppt,
    MOUSEMOVEPOINT*       pptBuf,
    UINT                  nPoints,
    DWORD                 resolution);


VOID xxxProcessKeyEvent(
   PKE       pke,
   ULONG_PTR ExtraInformation,
   BOOL      bInjected);

VOID xxxButtonEvent(
    DWORD ButtonNumber,
    POINT ptPointer,
    BOOL  fBreak,
    DWORD time,
    ULONG_PTR ExtraInfo,
    BOOL  bInjected,
    BOOL  fDblClk);

VOID xxxMoveEvent(
    LONG         dx,
    LONG         dy,
    DWORD        dwFlags,
    ULONG_PTR    dwExtraInfo,
    DWORD        time,
    BOOL         bInjected
    );

PDEVICEINFO StartDeviceRead(PDEVICEINFO pDeviceInfo);

NTSTATUS DeviceNotify(
    IN PPLUGPLAY_NOTIFY_HDR pNotification,
    IN PDEVICEINFO pDeviceInfo);

#define MOUSE_SENSITIVITY_MIN     1
#define MOUSE_SENSITIVITY_DEFAULT 10
#define MOUSE_SENSITIVITY_MAX     20
LONG CalculateMouseSensitivity(LONG lSens);

PDEVICEINFO FreeDeviceInfo(PDEVICEINFO pMouseInfo);

VOID QueueMouseEvent(
    USHORT       ButtonFlags,
    USHORT       ButtonData,
    ULONG_PTR    ExtraInfo,
    POINT        ptMouse,
    LONG         time,
    BOOL         bInjected,
    BOOL         bWakeRIT
    );

typedef struct {
    DWORD dwVersion;
    DWORD dwFlags;
    DWORD dwMapCount;
    DWORD dwMap[0];
} SCANCODEMAP, *PSCANCODEMAP;


#ifndef SCANCODE_NUMPAD_PLUS
#define SCANCODE_NUMPAD_PLUS    (0x4e)
#endif
#ifndef SCANCODE_NUMPAD_DOT
#define SCANCODE_NUMPAD_DOT     (0x53)
#endif

/*
 * Flag (LowLevel and HighLevel) for
 * hex Alt+Numpad mode.
 * If you need to add a new flag for gfInNumpadHexInput,
 * note the variable is BYTE.
 */
#define NUMPAD_HEXMODE_LL       (1)
#define NUMPAD_HEXMODE_HL       (2)

#define MODIFIER_FOR_ALT_NUMPAD(wModBit) \
    (((wModBits) == KBDALT) || ((wModBits) == (KBDALT | KBDSHIFT)) || \
     ((wModBits) == (KBDKANA | KBDALT)) || ((wModBits) == (KBDKANA | KBDALT | KBDSHIFT)))


BOOL UnqueueMouseEvent(PMOUSEEVENT pme);

BYTE    VKFromVSC(PKE pke, BYTE bPrefix, LPBYTE afKeyState);
BOOL    KEOEMProcs(PKE pke);
BOOL    xxxKELocaleProcs(PKE pke);
BOOL    xxxKENLSProcs(PKE pke, ULONG_PTR dwExtraInformation);

VOID    xxxKeyEvent(USHORT usVk, WORD wScanCode, DWORD time, ULONG_PTR ExtraInfo, BOOL bInjected);

typedef BITMAPINFOHEADER *PBMPHEADER, *LPBMPHEADER;

/*
 * Defines for WinOldAppHackoMatic flags which win386 oldapp can send to us.
 * These are kept in user's global variable winOldAppHackoMaticFlags
 */
#define WOAHACK_CHECKALTKEYSTATE 1
#define WOAHACK_IGNOREALTKEYDOWN 2

void xxxSimpleDoSyncPaint(PWND pwnd);
VOID xxxDoSyncPaint(PWND pwnd, DWORD flags);
void xxxInternalDoSyncPaint(PWND pwnd, DWORD flags);

/*
 * NOTE: the first 4 values must be as defined for backward compatibility
 * reasons.  They are sent as parameters to the WM_SYNCPAINT message.
 * They used to be hard-coded constants.
 *
 * Only ENUMCLIPPEDCHILDREN, ALLCHILDREN, and NOCHECKPARENTS are passed on
 * during recursion.  The other bits reflect the current window only.
 */
#define DSP_ERASE               0x0001  // Send WM_ERASEBKGND
#define DSP_FRAME               0x0002  // Send WM_NCPAINT
#define DSP_ENUMCLIPPEDCHILDREN 0x0004  // Enum children if WS_CLIPCHILDREN
#define DSP_WM_SYNCPAINT        0x0008  // Called from WM_SYNCPAINT handler
#define DSP_NOCHECKPARENTS      0x0010  // Don't check parents for update region
#define DSP_ALLCHILDREN         0x0020  // Enumerate all children.

BOOL xxxDrawAnimatedRects(
    PWND pwndClip,
    int idAnimation,
    LPRECT lprcStart,
    LPRECT lprcEnd);

typedef struct tagTIMER {           // tmr
    HEAD            head;
    struct tagTIMER *ptmrNext;
    struct tagTIMER *ptmrPrev;
    PTHREADINFO     pti;
    struct tagWND * spwnd;
    UINT_PTR        nID;
    INT             cmsCountdown;
    INT             cmsRate;
    UINT            flags;
    TIMERPROC_PWND  pfn;
    PTHREADINFO     ptiOptCreator;     // used for journal playback
                                       // will be NULL if timer was created by non-GUI thread
} TIMER, *PTIMER;

UINT_PTR InternalSetTimer(PWND pwnd, UINT_PTR nIDEvent, UINT dwElapse,
        TIMERPROC_PWND pTimerFunc, UINT flags);

VOID FreeTimer(PTIMER ptmr);

/*
 * Call FindTimer() with fKill == TRUE and TMRF_RIT.  This will basically
 * delete the timer.
 */
#define KILLRITTIMER(pwnd, nID) FindTimer(pwnd, nID, TMRF_RIT, TRUE)

/*
 * Raster Ops
 */
#define DPO           0x00FA0089  /* destination, pattern, or */

/*
 * Message thunks.
 */
typedef LRESULT (APIENTRY *SFNSCSENDMESSAGE)(PWND, UINT, WPARAM, LPARAM,
        ULONG_PTR, PROC, DWORD, PSMS);

#define SMESSAGEPROTO(func) \
     LRESULT CALLBACK Sfn ## func(                              \
        PWND pwnd, UINT msg, WPARAM wParam, LPARAM lParam,      \
        ULONG_PTR xParam, PROC xpfnWndProc, DWORD dwSCMSFlags, PSMS psms)

SMESSAGEPROTO(SENTDDEMSG);
SMESSAGEPROTO(DDEINIT);
SMESSAGEPROTO(DWORD);
SMESSAGEPROTO(INWPARAMCHAR);
SMESSAGEPROTO(INWPARAMDBCSCHAR);

SMESSAGEPROTO(GETTEXTLENGTHS);

SMESSAGEPROTO(GETDBCSTEXTLENGTHS);
SMESSAGEPROTO(INLPCREATESTRUCT);
SMESSAGEPROTO(INLPDROPSTRUCT);
SMESSAGEPROTO(INOUTLPPOINT5);
SMESSAGEPROTO(INOUTLPSCROLLINFO);
SMESSAGEPROTO(INOUTLPRECT);
SMESSAGEPROTO(INOUTNCCALCSIZE);
SMESSAGEPROTO(OUTLPRECT);
SMESSAGEPROTO(INLPMDICREATESTRUCT);
SMESSAGEPROTO(INLPCOMPAREITEMSTRUCT);
SMESSAGEPROTO(INLPDELETEITEMSTRUCT);
SMESSAGEPROTO(INLPHLPSTRUCT);
SMESSAGEPROTO(INLPHELPINFOSTRUCT);      // WINHELP4
SMESSAGEPROTO(INLPDRAWITEMSTRUCT);
SMESSAGEPROTO(INOUTLPMEASUREITEMSTRUCT);
SMESSAGEPROTO(INSTRING);
SMESSAGEPROTO(INPOSTEDSTRING);
SMESSAGEPROTO(INSTRINGNULL);
SMESSAGEPROTO(OUTSTRING);
SMESSAGEPROTO(INCNTOUTSTRING);
SMESSAGEPROTO(POUTLPINT);
SMESSAGEPROTO(POPTINLPUINT);
SMESSAGEPROTO(INOUTLPWINDOWPOS);
SMESSAGEPROTO(INLPWINDOWPOS);
SMESSAGEPROTO(INLBOXSTRING);
SMESSAGEPROTO(OUTLBOXSTRING);
SMESSAGEPROTO(INCBOXSTRING);
SMESSAGEPROTO(OUTCBOXSTRING);
SMESSAGEPROTO(INCNTOUTSTRINGNULL);
SMESSAGEPROTO(INOUTDRAG);
SMESSAGEPROTO(FULLSCREEN);
SMESSAGEPROTO(INPAINTCLIPBRD);
SMESSAGEPROTO(INSIZECLIPBRD);
SMESSAGEPROTO(OUTDWORDDWORD);
SMESSAGEPROTO(OUTDWORDINDWORD);
SMESSAGEPROTO(OPTOUTLPDWORDOPTOUTLPDWORD);
SMESSAGEPROTO(DWORDOPTINLPMSG);
SMESSAGEPROTO(COPYGLOBALDATA);
SMESSAGEPROTO(COPYDATA);
SMESSAGEPROTO(INDESTROYCLIPBRD);
SMESSAGEPROTO(INOUTNEXTMENU);
SMESSAGEPROTO(INOUTSTYLECHANGE);
SMESSAGEPROTO(IMAGEIN);
SMESSAGEPROTO(IMAGEOUT);
SMESSAGEPROTO(INDEVICECHANGE);
SMESSAGEPROTO(INOUTMENUGETOBJECT);
SMESSAGEPROTO(POWERBROADCAST);
SMESSAGEPROTO(LOGONNOTIFY);
SMESSAGEPROTO(IMECONTROL);
SMESSAGEPROTO(IMEREQUEST);
SMESSAGEPROTO(INLPKDRAWSWITCHWND);

/***************************************************************************\
* Function Prototypes
*
* NOTE: Only prototypes for GLOBAL (across module) functions should be put
* here.  Prototypes for functions that are global to a single module should
* be put at the head of that module.
*
* LATER: There's still lots of bogus trash in here to be cleaned out.
*
\***************************************************************************/

/*
 * Random prototypes.
 */
DWORD _GetWindowContextHelpId(
    PWND pwnd);

BOOL _SetWindowContextHelpId(
    PWND pwnd,
    DWORD dwContextId);

void xxxSendHelpMessage(
    PWND   pwnd,
    int    iType,
    int    iCtrlId,
    HANDLE hItemHandle,
    DWORD  dwContextId);

HPALETTE _SelectPalette(
    HDC hdc,
    HPALETTE hpalette,
    BOOL fForceBackground);

int xxxRealizePalette(
    HDC hdc);

VOID xxxFlushPalette(
    PWND pwnd);

VOID xxxBroadcastPaletteChanged(
    PWND pwnd,
    BOOL fForceDesktop);

PCURSOR SearchIconCache(
    PCURSOR         pCursorCache,
    ATOM            atomModName,
    PUNICODE_STRING pstrResName,
    PCURSOR         pCursorSrc,
    PCURSORFIND     pcfSearch);

VOID ZombieCursor(PCURSOR pcur);

BOOL IsSmallerThanScreen(PWND pwnd);

BOOL zzzSetSystemCursor(
    PCURSOR pcur,
    DWORD   id);

BOOL zzzSetSystemImage(
    PCURSOR pcur,
    PCURSOR pcurOld);

BOOL _InternalGetIconInfo(
    IN  PCURSOR                  pcur,
    OUT PICONINFO                piconinfo,
    OUT OPTIONAL PUNICODE_STRING pstrModName,
    OUT OPTIONAL PUNICODE_STRING pstrResName,
    OUT OPTIONAL LPDWORD         pbpp,
    IN  BOOL                     fInternalCursor);

VOID LinkCursor(
    PCURSOR pcur);

BOOL _SetCursorIconData(
    PCURSOR         pcur,
    PUNICODE_STRING pstrModName,
    PUNICODE_STRING pstrResName,
    PCURSORDATA     pData,
    DWORD           cbData);

PCURSOR _GetCursorFrameInfo(
    PCURSOR pcur,
    int     iFrame,
    PJIF    pjifRate,
    LPINT   pccur);

BOOL zzzSetSystemCursor(
    PCURSOR pcur,
    DWORD id);

PCURSOR _FindExistingCursorIcon(
    ATOM            atomModName,
    PUNICODE_STRING pstrResName,
    PCURSOR         pcurSrc,
    PCURSORFIND     pcfSearch);

HCURSOR _CreateEmptyCursorObject(
    BOOL fPublic);

BOOL _GetUserObjectInformation(HANDLE h,
    int nIndex, PVOID pvInfo, DWORD nLength, LPDWORD lpnLengthNeeded);
BOOL _SetUserObjectInformation(HANDLE h,
    int nIndex, PVOID pvInfo, DWORD nLength);
DWORD xxxWaitForInputIdle(ULONG_PTR idProcess, DWORD dwMilliseconds,
        BOOL fSharedWow);
VOID StartScreenSaver(BOOL bOnlyIfSecure);
UINT InternalMapVirtualKeyEx(UINT wCode, UINT wType, PKBDTABLES pKbdTbl);
SHORT InternalVkKeyScanEx(WCHAR cChar, PKBDTABLES pKbdTbl);



PWND ParentNeedsPaint(PWND pwnd);
VOID SetHungFlag(PWND pwnd, WORD wFlag);
VOID ClearHungFlag(PWND pwnd, WORD wFlag);

BOOL _DdeSetQualityOfService(PWND pwndClient,
        CONST PSECURITY_QUALITY_OF_SERVICE pqosNew,
        PSECURITY_QUALITY_OF_SERVICE pqosOld);
BOOL _DdeGetQualityOfService(PWND pwndClient,
        PWND pwndServer, PSECURITY_QUALITY_OF_SERVICE pqos);

BOOL QueryTrackMouseEvent(LPTRACKMOUSEEVENT lpTME);
void CancelMouseHover(PQ pq);
void ResetMouseTracking(PQ pq, PWND pwnd);

void _SetIMEShowStatus(BOOL fShow);
BOOL _GetIMEShowStatus(VOID);

/*
 * Prototypes for internal version of APIs.
 */
PWND _FindWindowEx(PWND pwndParent, PWND pwndChild,
                              LPCWSTR pszClass, LPCWSTR pszName, DWORD dwType);
UINT APIENTRY GreSetTextAlign(HDC, UINT);
UINT APIENTRY GreGetTextAlign(HDC);

/*
 * Prototypes for validation, RIP, error handling, etc functions.
 */
PWND FASTCALL   ValidateHwnd(HWND hwnd);

NTSTATUS ValidateHwinsta(HWINSTA, KPROCESSOR_MODE, ACCESS_MASK, PWINDOWSTATION*);
NTSTATUS ValidateHdesk(HDESK, KPROCESSOR_MODE, ACCESS_MASK, PDESKTOP*);

PMENU           ValidateHmenu(HMENU hmenu);
PMONITOR        ValidateHmonitor(HMONITOR hmonitor);
HRGN            UserValidateCopyRgn(HRGN);

BOOL    ValidateHandleSecure(HANDLE h);

NTSTATUS UserJobCallout(PKWIN32_JOBCALLOUT_PARAMETERS Parm);

BOOL RemoveProcessFromJob(PPROCESSINFO ppi);


BOOL    xxxActivateDebugger(UINT fsModifiers);

void ClientDied(void);

VOID    SendMsgCleanup(PTHREADINFO ptiCurrent);
VOID    ReceiverDied(PSMS psms, PSMS *ppsmsUnlink);
LRESULT xxxInterSendMsgEx(PWND, UINT, WPARAM, LPARAM, PTHREADINFO, PTHREADINFO, PINTRSENDMSGEX );
VOID    ClearSendMessages(PWND pwnd);
PPCLS   GetClassPtr(ATOM atom, PPROCESSINFO ppi, HANDLE hModule);
BOOL    ReferenceClass(PCLS pcls, PWND pwnd);
VOID    DereferenceClass(PWND pwnd);
ULONG_PTR MapClientToServerPfn(ULONG_PTR dw);


VOID xxxReceiveMessage(PTHREADINFO);
#define xxxReceiveMessages(pti) \
    while ((pti)->pcti->fsWakeBits & QS_SENDMESSAGE) { xxxReceiveMessage((pti)); }

PBWL     BuildHwndList(PWND pwnd, UINT flags, PTHREADINFO ptiOwner);
VOID     FreeHwndList(PBWL pbwl);

#define  MINMAX_KEEPHIDDEN 0x1
#define  MINMAX_ANIMATE    0x10000

PWND     xxxMinMaximize(PWND pwnd, UINT cmd, DWORD dwFlags);
void     xxxMinimizeHungWindow(PWND pwnd);
VOID     xxxInitSendValidateMinMaxInfo(PWND pwnd, LPMINMAXINFO lpmmi);
HRGN     CreateEmptyRgn(void);
HRGN     CreateEmptyRgnPublic(void);
HRGN     SetOrCreateRectRgnIndirectPublic(HRGN * phrgn, LPCRECT lprc);
BOOL     SetEmptyRgn(HRGN hrgn);
BOOL     SetRectRgnIndirect(HRGN hrgn, LPCRECT lprc);
NTSTATUS xxxRegisterForDeviceClassNotifications();
BOOL     xxxInitInput(PTERMINAL);
VOID     InitMice();
void     UpdateMouseInfo(void);
BOOL     OpenMouse(PDEVICEINFO pMouseInfo);
void     ProcessDeviceChanges(DWORD DeviceType);
PDEVICEINFO CreateDeviceInfo(DWORD DeviceType, PUNICODE_STRING SymbolicLinkName, BYTE bFlags);
void     InitKeyboard(void);
UINT     xxxHardErrorControl(DWORD, HANDLE, PDESKRESTOREDATA);

VOID     SetKeyboardRate(UINT nKeySpeed);
VOID     RecolorDeskPattern();
BOOL     xxxInitWindowStation(PWINDOWSTATION);
VOID     zzzInternalSetCursorPos(int x, int y);
VOID     UpdateKeyLights(BOOL bInjected);
VOID     SetDebugHotKeys();
VOID     BoundCursor(LPPOINT lppt);

void     DestroyKF(PKBDFILE pkf);
VOID     DestroyKL(PKL pkl);
BOOL     xxxSetDeskPattern(PUNICODE_STRING pProfileUserName,LPWSTR lpPat, BOOL fCreation);
BOOL     xxxSetDeskWallpaper(PUNICODE_STRING pProfileUserName,LPWSTR lpszFile);
HPALETTE CreateDIBPalette(LPBITMAPINFOHEADER pbmih, UINT colors);
BOOL     CalcVisRgn(HRGN* hrgn, PWND pwndOrg, PWND pwndClip, DWORD flags);

NTSTATUS xxxCreateThreadInfo(PETHREAD, BOOL);

BOOL     DestroyProcessInfo(PW32PROCESS);

#if DBG
PWCHAR GetDesktopName(PDESKTOP pdesk);
#endif // DBG

VOID     xxxDesktopThread(PTERMINAL pTerm);

VOID     ForceEmptyClipboard(PWINDOWSTATION);

NTSTATUS zzzInitTask(UINT dwExpWinVer, DWORD dwAppCompatFlags,
                PUNICODE_STRING pstrModName, PUNICODE_STRING pstrBaseFileName,
                DWORD hTaskWow, DWORD dwHotkey, DWORD idTask,
                DWORD dwX, DWORD dwY, DWORD dwXSize, DWORD dwYSize);
VOID    DestroyTask(PPROCESSINFO ppi, PTHREADINFO ptiToRemove);
void    PostInputMessage(PQ pq, PWND pwnd, UINT message, WPARAM wParam,
                LPARAM lParam, DWORD time, ULONG_PTR dwExtraInfo);
PWND    PwndForegroundCapture(VOID);
BOOL    xxxSleepThread(UINT fsWakeMask, DWORD Timeout, BOOL fForegroundIdle);
VOID    SetWakeBit(PTHREADINFO pti, UINT wWakeBit);
VOID    WakeSomeone(PQ pq, UINT message, PQMSG pqmsg);
VOID    ClearWakeBit(PTHREADINFO pti, UINT wWakeBit, BOOL fSysCheck);
NTSTATUS xxxInitProcessInfo(PW32PROCESS);

PTHREADINFO PtiFromThreadId(DWORD idThread);
BOOL    zzzAttachThreadInput(PTHREADINFO ptiAttach, PTHREADINFO ptiAttachTo, BOOL fAttach);
BOOL    zzzReattachThreads(BOOL fJournalAttach);
PQ      AllocQueue(PTHREADINFO, PQ);
VOID    FreeQueue(PQ pq);


VOID    FreeCachedQueues(VOID);
VOID    CleanupGDI(VOID);
VOID    CleanupResources(VOID);

void    zzzDestroyQueue(PQ pq, PTHREADINFO pti);
PQMSG   AllocQEntry(PMLIST pml);
__inline void FreeQEntry(PQMSG pqmsg)
{
    extern PPAGED_LOOKASIDE_LIST QEntryLookaside;
    ExFreeToPagedLookasideList(QEntryLookaside, pqmsg);
}

void    DelQEntry(PMLIST pml, PQMSG pqmsg);
void    zzzAttachToQueue(PTHREADINFO pti, PQ pqAttach, PQ pqJournal,
        BOOL fJoiningForeground);
VOID    xxxProcessEventMessage(PTHREADINFO ptiCurrent, PQMSG pqmsg);
VOID    xxxProcessSetWindowPosEvent(PSMWP psmwpT);
VOID    xxxProcessAsyncSendMessage(PASYNCSENDMSG pmsg);
BOOL    PostEventMessage(PTHREADINFO pti, PQ pq, DWORD dwQEvent, PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL    DoPaint(PWND pwndFilter, LPMSG lpMsg);
BOOL    DoTimer(PWND pwndFilter);
BOOL    CheckPwndFilter(PWND pwnd, PWND pwndFilter);

#define WHT_IGNOREDISABLED      0x00000001

#ifdef REDIRECTION

#define WHT_FAKESPEEDHITTEST    0x00000002

BOOL xxxGetCursorPos(LPPOINT lpPt);
PWND xxxCallSpeedHitTestHook(POINT* ppt);
VOID PushMouseMove(PQ pq, POINT pt);
VOID PopMouseMove(PQ pq, POINT* ppt);

#endif // REDIRECTION

HWND    xxxWindowHitTest(PWND pwnd,  POINT pt, int *pipos, DWORD dwHitTestFlags);
HWND    xxxWindowHitTest2(PWND pwnd, POINT pt, int *pipos, DWORD dwHitTestFlags);

PWND    SpeedHitTest(PWND pwndParent, POINT pt);
VOID    xxxDeactivate(PTHREADINFO pti, DWORD tidSetForeground);

#define SFW_STARTUP             0x0001
#define SFW_SWITCH              0x0002
#define SFW_NOZORDER            0x0004
#define SFW_SETFOCUS            0x0008
#define SFW_ACTIVATERESTORE     0x0010

BOOL    xxxSetForegroundWindow2(PWND pwnd, PTHREADINFO ptiCurrent, DWORD fFlags);
VOID    SetForegroundThread(PTHREADINFO pti);
VOID    xxxSendFocusMessages(PTHREADINFO pti, PWND pwndReceive);

#define ATW_MOUSE               0x0001
#define ATW_SETFOCUS            0x0002
#define ATW_ASYNC               0x0004
#define ATW_NOZORDER            0x0008

BOOL    FBadWindow(PWND pwnd);
BOOL    xxxActivateThisWindow(PWND pwnd, DWORD tidLoseForeground, DWORD fFlags);
BOOL    xxxActivateWindow(PWND pwnd, UINT cmd);

#define NTW_PREVIOUS         1
#define NTW_IGNORETOOLWINDOW 2
PWND    NextTopWindow(PTHREADINFO pti, PWND pwnd, PWND pwndSkip, DWORD flags);

int     xxxMouseActivate(PTHREADINFO pti, PWND pwnd, UINT message, WPARAM wParam, LPPOINT lppt, int ht);
int     UT_GetParentDCClipBox(PWND pwnd, HDC hdc, LPRECT lprc);
VOID    UpdateAsyncKeyState(PQ pq, UINT wVK, BOOL fBreak);
void    PostUpdateKeyStateEvent(PQ pq);
void    ProcessUpdateKeyStateEvent(PQ pq, CONST PBYTE pbKeyState, CONST PBYTE pbRecentDown);

BOOL    InternalSetProp(PWND pwnd, LPWSTR pszKey, HANDLE hData, DWORD dwFlags);
HANDLE  InternalRemoveProp(PWND pwnd, LPWSTR pszKey, BOOL fInternal);
VOID    DeleteProperties(PWND pwnd);
CHECKPOINT *CkptRestore(PWND pwnd, LPCRECT lprcWindow);
UINT_PTR _SetTimer(PWND pwnd, UINT_PTR nIDEvent, UINT dwElapse, TIMERPROC_PWND pTimerFunc);
BOOL    KillTimer2(PWND pwnd, UINT_PTR nIDEvent, BOOL fSystemTimer);
VOID    DestroyThreadsTimers(PTHREADINFO pti);
VOID    DecTimerCount(PTHREADINFO pti);
VOID    zzzInternalShowCaret();
VOID    zzzInternalHideCaret();
VOID    zzzInternalDestroyCaret();
VOID    ChangeAcquireResourceType(VOID);
VOID    EnterCrit(VOID);
VOID    EnterSharedCrit(VOID);
VOID    LeaveCrit(VOID);
VOID    _AssertCritIn(VOID);
VOID    _AssertDeviceInfoListCritIn(VOID);
VOID    _AssertCritInShared(VOID);
VOID    _AssertCritOut(VOID);
VOID    _AssertDeviceInfoListCritOut(VOID);
NTSTATUS _KeUserModeCallback (IN ULONG ApiNumber, IN PVOID InputBuffer,
    IN ULONG InputLength, OUT PVOID *OutputBuffer, OUT PULONG OutputLength);

#define UnlockProcess           ObDereferenceObject
#define UnlockThread            ObDereferenceObject

extern ULONG gSessionId;

#if DBG
    #define ValidateProcessSessionId(pEProcess)  \
        UserAssert((pEProcess)->SessionId == gSessionId)

    #define ValidateThreadSessionId(pEThread)  \
        UserAssert((pEThread)->ThreadsProcess->SessionId == gSessionId)
#else
    #define ValidateProcessSessionId(pEProcess)
    #define ValidateThreadSessionId(pEThread)
#endif


NTSTATUS __inline LockProcessByClientId(HANDLE dwProcessId, PEPROCESS* ppEProcess)
{
    NTSTATUS Status;

    CheckCritOut();

    Status = PsLookupProcessByProcessId(dwProcessId, ppEProcess);

    if (NT_SUCCESS(Status) && (*ppEProcess)->SessionId != gSessionId) {
        UnlockProcess(*ppEProcess);
        return STATUS_UNSUCCESSFUL;
    }
    return Status;
}

NTSTATUS __inline LockThreadByClientId(HANDLE dwThreadId, PETHREAD* ppEThread)
{
    NTSTATUS Status;

    Status = PsLookupThreadByThreadId(dwThreadId, ppEThread);

    if (NT_SUCCESS(Status) && (*ppEThread)->ThreadsProcess->SessionId != gSessionId) {
        UnlockThread(*ppEThread);
        return STATUS_UNSUCCESSFUL;
    }
    return Status;
}

BOOL    IsSAS(BYTE vk, UINT* pfsModifiers);
BOOL    xxxDoHotKeyStuff(UINT vk, BOOL fBreak, DWORD fsReserveKeys);
PHOTKEY IsHotKey(UINT fsModifiers, UINT vk);
/*
 * Server.c
 */
BOOL InitCreateUserCrit(VOID);
PMDEV InitVideo(
    BOOL bReenumerationNeeded);

/*
 * DRVSUP.C
 */
BOOL InitUserScreen();

VOID InitLoadResources();

typedef struct tagDISPLAYRESOURCE {
    WORD cyThunb;
    WORD cxThumb;
    WORD xCompressIcon;
    WORD yCompressIcon;
    WORD xCompressCursor;
    WORD yCompressCursor;
    WORD yKanji;
    WORD cxBorder;
    WORD cyBorder;
} DISPLAYRESOURCE, *PDISPLAYRESOURCE;



VOID xxxUserResetDisplayDevice();

/*
 * Object management and security
 */
#define DEFAULT_WINSTA  L"\\Windows\\WindowStations\\WinSta0"

#define POBJECT_NAME(pobj) (OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(pobj)) ? \
    &(OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(pobj))->Name) : NULL)

PSECURITY_DESCRIPTOR CreateSecurityDescriptor(PACCESS_ALLOWED_ACE paceList,
        DWORD cbAce, BOOLEAN fDaclDefaulted);
PACCESS_ALLOWED_ACE AllocAce(PACCESS_ALLOWED_ACE pace, BYTE bType,
        BYTE bFlags, ACCESS_MASK am, PSID psid, LPDWORD lpdwLength);
BOOL CheckGrantedAccess(ACCESS_MASK, ACCESS_MASK);
BOOL AccessCheckObject(PVOID, ACCESS_MASK, KPROCESSOR_MODE, CONST GENERIC_MAPPING *);
BOOL InitSecurity(VOID);
BOOL IsPrivileged(PPRIVILEGE_SET ppSet);
BOOL CheckWinstaWriteAttributesAccess(void);

NTSTATUS xxxUserDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle,
        HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess,
        ULONG HandleAttributes, ULONG Options);
HWINSTA xxxConnectService(PUNICODE_STRING, HDESK *);
NTSTATUS TestForInteractiveUser(PLUID pluidCaller);
NTSTATUS _UserTestForWinStaAccess( PUNICODE_STRING pstrWinSta, BOOL fInherit);
HDESK xxxResolveDesktop(HANDLE hProcess, PUNICODE_STRING pstrDesktop,
    HWINSTA *phwinsta, BOOL fInherit, BOOL* pbShutDown);

/* NEW CODE */
NTSTATUS xxxResolveDesktopForWOW (
    IN OUT PUNICODE_STRING pstrDesktop);

WORD xxxClientWOWGetProcModule(WNDPROC_PWND pfn);

PVOID _MapDesktopObject(HANDLE h);
PDESKTOPVIEW GetDesktopView(PPROCESSINFO ppi, PDESKTOP pdesk);
VOID TerminateConsole(PDESKTOP);


/*
 * Object manager callouts for windowstations
 */
VOID DestroyWindowStation(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount);

VOID FreeWindowStation(
    IN PWINDOWSTATION WindowStation);

NTSTATUS ParseWindowStation(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object);

BOOLEAN OkayToCloseWindowStation(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle
    );

/*
 * Object manager callouts for desktops
 */
VOID MapDesktop(
    IN OB_OPEN_REASON OpenReason,
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG HandleCount);

VOID UnmapDesktop(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount);

VOID FreeDesktop(
    IN PVOID Desktop);

NTSTATUS ParseDesktop(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object);

BOOLEAN OkayToCloseDesktop(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN HANDLE Handle
    );

/*
 * Routines pilfered from kernel32
 */
VOID UserSleep(DWORD dwMilliseconds);
BOOL UserBeep(DWORD dwFreq, DWORD dwDuration);
NTSTATUS UserRtlCreateAtomTable(ULONG NumberOfBuckets);
ATOM UserAddAtom(LPCWSTR lpAtom, BOOL bPin);
ATOM UserFindAtom(LPCWSTR lpAtom);
ATOM UserDeleteAtom(ATOM atom);
UINT UserGetAtomName(ATOM atom, LPWSTR lpch, int cchMax);

#define FindClassAtom(lpszClassName) \
    (IS_PTR(lpszClassName) ? UserFindAtom(lpszClassName) : PTR_TO_ID(lpszClassName))

/*
 * Keyboard Layouts
 */
VOID ChangeForegroundKeyboardTable(PKL pklOld, PKL pklNew);
HKL  xxxLoadKeyboardLayoutEx(PWINDOWSTATION, HANDLE, HKL, UINT, LPCWSTR, UINT, UINT);
HKL  xxxActivateKeyboardLayout(PWINDOWSTATION pwinsta, HKL hkl, UINT Flags, PWND pwnd);
HKL  xxxInternalActivateKeyboardLayout(PKL pkl, UINT Flags, PWND pwnd);
BOOL GetKbdLangSwitch(PUNICODE_STRING pProfileUserName);

BOOL xxxUnloadKeyboardLayout(PWINDOWSTATION, HKL);
VOID RemoveKeyboardLayoutFile(PKBDFILE pkf);
HKL  _GetKeyboardLayout(DWORD idThread);
UINT _GetKeyboardLayoutList(PWINDOWSTATION pwinsta, UINT nItems, HKL *lpBuff);
VOID xxxFreeKeyboardLayouts(PWINDOWSTATION, BOOL bUnlock);

DWORD xxxDragObject(PWND pwndParent, PWND xhwndFrom, UINT wFmt,
        ULONG_PTR dwData, PCURSOR xpcur);
BOOL xxxDragDetect(PWND pwnd, POINT pt);
BOOL xxxIsDragging(PWND pwnd, POINT ptScreen, UINT uMsg);

HKL GetActiveHKL();

#define DMI_INVERT         0x00000001
#define DMI_GRAYED         0x00000002

void xxxDrawMenuItem(HDC hdc, PMENU pMenu, PITEM pItem, DWORD dwFlags);

BOOL xxxRealDrawMenuItem(HDC hdc, PGRAYMENU lpGray, int cx, int cy);

void xxxDrawMenuBarUnderlines(PWND pwnd, BOOL fShow);

/*
 * Menu macros
 */
__inline BOOL IsRootPopupMenu(PPOPUPMENU ppopupmenu)
{
    return (ppopupmenu == ppopupmenu->ppopupmenuRoot);
}
__inline BOOL ExitMenuLoop (PMENUSTATE pMenuState, PPOPUPMENU ppopupmenu)
{
    return  (!pMenuState->fInsideMenuLoop || ppopupmenu->fDestroyed);
}
__inline PMENUSTATE GetpMenuState (PWND pwnd)
{
    return (GETPTI(pwnd)->pMenuState);
}
__inline PPOPUPMENU GetpGlobalPopupMenu (PWND pwnd)
{
    return (GetpMenuState(pwnd) ? GetpMenuState(pwnd)->pGlobalPopupMenu : NULL);
}
__inline BOOL IsInsideMenuLoop(PTHREADINFO pti)
{
    return ((pti->pMenuState != NULL) && pti->pMenuState->fInsideMenuLoop);
}
__inline BOOL IsMenuStarted(PTHREADINFO pti)
{
    return ((pti->pMenuState != NULL) && pti->pMenuState->fMenuStarted);
}
__inline PITEM MNGetToppItem(PMENU pMenu)
{
    return pMenu->rgItems + pMenu->iTop;
}
__inline BOOL MNIsItemSelected(PPOPUPMENU ppopupmenu)
{
    return ((int)ppopupmenu->posSelectedItem >= 0);
}
__inline PITEM MNGetSelectedpitem(PPOPUPMENU ppopupmenu)
{
    return ppopupmenu->spmenu->rgItems + ppopupmenu->posSelectedItem;
}
__inline BOOL MNIsScrollArrowSelected(PPOPUPMENU ppopupmenu)
{
    return ((ppopupmenu->posSelectedItem == MFMWFP_UPARROW)
            || (ppopupmenu->posSelectedItem == MFMWFP_DOWNARROW));
}
__inline BOOL IsModelessMenuNotificationWindow (PWND pwnd)
{
    PMENUSTATE pMenuState;
    return (((pMenuState = GetpMenuState(pwnd)) != NULL)
                && pMenuState->fModelessMenu
                && (pMenuState->pGlobalPopupMenu->spwndNotify == pwnd));
}
__inline BOOL IsRecursedMenuState(PMENUSTATE pMenuState, PPOPUPMENU ppopupmenu)
{
    return (pMenuState->pGlobalPopupMenu != ppopupmenu->ppopupmenuRoot);
}

__inline BOOL IsMDIItem (PITEM pitem)
{
   return (TestMFS(pitem, MFS_CACHEDBMP)
      && (pitem->hbmp != NULL)
      && (pitem->hbmp <= HBMMENU_MBARLAST));
}


#define MNXBORDER (SYSMET(CXBORDER) + SYSMET(CXEDGE))
#define MNYBORDER (SYSMET(CYBORDER) + SYSMET(CYEDGE))
#define MNXSPACE  (SYSMET(CXEDGE))
#define MNLEFTMARGIN (SYSMET(CXEDGE))

/*
 * xxxMNUpdateShownMenu flags
 */
#define MNUS_DEFAULT      0x00000001
#define MNUS_DELETE       0x00000002
#define MNUS_DRAWFRAME    0x00000004

/* This tells xxxMNItemSize that the bitamp size is not avilable */
#define MNIS_MEASUREBMP -1


/*
 * MN_SIZEWINDOW wParam flag. xxxMNUpdateShownMenu sends this
 *  message, so keep MNSW_ and MNUS_ in sync.
 */
#define MNSW_RETURNSIZE  0
#define MNSW_SIZE        MNUS_DEFAULT
#define MNSW_DRAWFRAME   MNUS_DRAWFRAME

/*
 * Animation flags (pMenuState->iAniDropDir)
 */
#define PAS_RIGHT       (TPM_HORPOSANIMATION >> TPM_FIRSTANIBITPOS)
#define PAS_LEFT        (TPM_HORNEGANIMATION >> TPM_FIRSTANIBITPOS)
#define PAS_DOWN        (TPM_VERPOSANIMATION >> TPM_FIRSTANIBITPOS)
#define PAS_UP          (TPM_VERNEGANIMATION >> TPM_FIRSTANIBITPOS)
#define PAS_OUT         0x10
#define PAS_HORZ        (PAS_LEFT | PAS_RIGHT)
#define PAS_VERT        (PAS_UP | PAS_DOWN)

#if (PAS_HORZ + PAS_VERT >= PAS_OUT)
#error PAS_ & TPM_*ANIMATION conflict.
#endif

#define CXMENU3DEDGE 1
#define CYMENU3DEDGE 1

/*
 * Scrollbar initialization types
 */
#define SCROLL_NORMAL   0
#define SCROLL_DIRECT   1
#define SCROLL_MENU     2

/*
 * movesize.c
 */
void xxxDrawDragRect(PMOVESIZEDATA pmsd, LPRECT lprc, UINT flags);
void GetMonitorMaxArea(PWND pwnd, PMONITOR pMonitor, LPRECT * pprc);

/*
 * focusact.c
 */
VOID SetForegroundPriorityProcess(PPROCESSINFO ppi, PTHREADINFO pti, BOOL fSetForegound);
VOID SetForegroundPriority(PTHREADINFO pti, BOOL fSetForeground);
void xxxUpdateTray(PWND pwnd);


//
// mnchange.c
//
void xxxMNUpdateShownMenu(PPOPUPMENU ppopup, PITEM pItem, UINT uFlags);

//
// mnkey.c
//
UINT xxxMNFindChar(PMENU pMenu, UINT ch, INT idxC, INT *lpr);
UINT MNFindItemInColumn(PMENU pMenu, UINT idxB, int dir, BOOL fRoot);

//
// mndraw.c
//
void MNAnimate(PMENUSTATE pMenuState, BOOL fIterate);
void MNDrawFullNC(PWND pwnd, HDC hdcIn, PPOPUPMENU ppopup);
void MNDrawArrow(HDC hdcIn, PPOPUPMENU ppopup, UINT uArrow);
void MNEraseBackground (HDC hdc, PMENU pmenu, int x, int y, int cx, int cy);

//
// mnstate.c
//
PMENUSTATE xxxMNAllocMenuState(PTHREADINFO ptiCurrent, PTHREADINFO ptiNotify, PPOPUPMENU ppopupmenuRoot);
void xxxMNEndMenuState(BOOL fFreePopup);
BOOL MNEndMenuStateNotify (PMENUSTATE pMenuState);
void MNFlushDestroyedPopups (PPOPUPMENU ppopupmenu, BOOL fUnlock);
BOOL MNSetupAnimationDC (PMENUSTATE pMenuState);
BOOL MNCreateAnimationBitmap(PMENUSTATE pMenuState, UINT cx, UINT cy);
void MNDestroyAnimationBitmap(PMENUSTATE pMenuState);
PMENUSTATE xxxMNStartMenuState(PWND pwnd, DWORD cmd, LPARAM lParam);
__inline void LockMenuState (PMENUSTATE pMenuState)
{
    (pMenuState->dwLockCount)++;
}
BOOL xxxUnlockMenuState (PMENUSTATE pMenuState);

//
// menu.c
//
#if DBG
    void Validateppopupmenu (PPOPUPMENU ppopupmenu);
#else // DBG
    #define Validateppopupmenu(ppopupmenu)
#endif // DBG

#if DBG
    #define MNGetpItemIndex DBGMNGetpItemIndex
UINT DBGMNGetpItemIndex(PMENU pmenu, PITEM pitem);
#else // DBG
    #define MNGetpItemIndex _MNGetpItemIndex
#endif // DBG
__inline UINT _MNGetpItemIndex(PMENU pmenu, PITEM pitem)
    {return (UINT)(((ULONG_PTR)pitem - (ULONG_PTR)pmenu->rgItems) / sizeof(ITEM));}

void xxxMNDismiss (PMENUSTATE pMenuState);
PITEM MNGetpItem(PPOPUPMENU ppopup, UINT uIndex);
void xxxMNSetCapture (PPOPUPMENU ppopup);
void xxxMNReleaseCapture (void);
void MNCheckButtonDownState (PMENUSTATE pMenuState);
PWND GetMenuStateWindow (PMENUSTATE pMenuState);
PVOID LockPopupMenu(PPOPUPMENU ppopup, PMENU * pspmenu, PMENU pmenu);
PVOID UnlockPopupMenu(PPOPUPMENU ppopup, PMENU * pspmenu);
PVOID LockWndMenu(PWND pwnd, PMENU * pspmenu, PMENU pmenu);
PVOID UnlockWndMenu(PWND pwnd, PMENU * pspmenu);
UINT MNSetTimerToCloseHierarchy(PPOPUPMENU ppopup);
BOOL xxxMNSetTop(PPOPUPMENU ppopup, int iNewTop);
LRESULT xxxMenuWindowProc(PWND, UINT, WPARAM, LPARAM);
VOID xxxMNButtonUp(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT posItemHit, LPARAM lParam);
VOID xxxMNButtonDown(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT posItemHit, BOOL fClick);
PITEM xxxMNSelectItem(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT itemPos);
BOOL xxxMNSwitchToAlternateMenu(PPOPUPMENU ppopupMenu);
void xxxMNCancel(PMENUSTATE pMenuState, UINT uMsg, UINT cmd, LPARAM lParam);
VOID xxxMNKeyDown(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT key);
BOOL xxxMNDoubleClick(PMENUSTATE pMenuState, PPOPUPMENU ppopup, int idxItem);
VOID xxxMNCloseHierarchy(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState);
PWND xxxMNOpenHierarchy(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState);
void LockMFMWFPWindow (PULONG_PTR puHitArea, ULONG_PTR uNewHitArea);
void UnlockMFMWFPWindow (PULONG_PTR puHitArea);
BOOL IsMFMWFPWindow (ULONG_PTR uHitArea);
LONG_PTR xxxMNFindWindowFromPoint(PPOPUPMENU ppopupMenu, PUINT pIndex, POINTS screenPt);
VOID xxxMNMouseMove(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, POINTS screenPt);
int xxxMNCompute(PMENU pMenu, PWND pwndNotify, DWORD yMenuTop,
        DWORD xMenuLeft,DWORD cxMax, LPDWORD lpdwHeight);
VOID xxxMNRecomputeBarIfNeeded(PWND pwndNotify, PMENU pMenu);
VOID xxxMenuDraw(HDC hdc, PMENU pMenu);
UINT  MNFindNextValidItem(PMENU pMenu, int i, int dir, UINT flags);
VOID MNFreeItem(PMENU pMenu, PITEM pItem, BOOL fFreeItemPopup);
BOOL   xxxMNStartMenu(PPOPUPMENU ppopupMenu, int mn);
VOID MNPositionSysMenu(PWND pwnd, PMENU pSysMenu);

PITEM xxxMNInvertItem(PPOPUPMENU ppopupmenu, PMENU pMenu,int itemNumber,PWND pwndNotify, BOOL fOn);

VOID   xxxSendMenuSelect(PWND pwndNotify, PWND pwndMenu, PMENU pMenu, int idx);
#define SMS_NOMENU      (PMENU)(-1)


BOOL   xxxSetSystemMenu(PWND pwnd, PMENU pMenu);
BOOL   xxxSetDialogSystemMenu(PWND pwnd);

VOID xxxMNChar(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT character);
PPOPUPMENU MNAllocPopup(BOOL fForceAlloc);
VOID MNFreePopup(PPOPUPMENU ppopupmenu);

/*
 * Menu entry points used by the rest of USER
 */
VOID xxxMNKeyFilter(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, UINT ch);
int  xxxMenuBarCompute(PMENU pMenu, PWND pwndNotify, DWORD yMenuTop,
        DWORD xMenuLeft, int cxMax);
VOID xxxEndMenu(PMENUSTATE pMenuState);
BOOL xxxCallHandleMenuMessages(PMENUSTATE pMenuState, PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL xxxHandleMenuMessages(LPMSG lpmsg, PMENUSTATE pMenuState, PPOPUPMENU ppopupmenu);
void xxxEndMenuLoop (PMENUSTATE pMenuState, PPOPUPMENU ppopupmenu);
int  xxxMNLoop(PPOPUPMENU ppopupMenu, PMENUSTATE pMenuState, LPARAM lParam, BOOL fDblClk);
VOID xxxSetSysMenu(PWND pwnd);
PMENU xxxGetSysMenuHandle(PWND pwnd);
PMENU xxxGetSysMenu(PWND pwnd, BOOL fSubMenu);
PMENU MakeMenuRtoL(PMENU pMenu, BOOL bRtoL);
HDC CreateCompatiblePublicDC(HDC hdc, HBITMAP *pbmDCGray);
void xxxPSMTextOut(HDC hdc, int xLeft, int yTop, LPWSTR lpsz, int cch, DWORD dwFlags);
BOOL xxxPSMGetTextExtent(HDC hdc, LPWSTR lpstr, int cch, PSIZE psize);

/*
 * LPK callbacks
 */
void xxxClientPSMTextOut(HDC hdc, int xLeft, int yTop, PUNICODE_STRING lpsz, int cch, DWORD dwFlags);
int  xxxClientLpkDrawTextEx(HDC hdc, int xLeft, int yTop, LPCWSTR lpsz, int nCount,
        BOOL fDraw, UINT wFormat, LPDRAWTEXTDATA lpDrawInfo, UINT bAction, int iCharSet);
BOOL xxxClientExtTextOutW(HDC hdc, int x, int y, int flOpts, RECT *prcl,
        LPCWSTR pwsz, UINT cwc, INT *pdx);
BOOL xxxClientGetTextExtentPointW(HDC hdc, LPCWSTR lpstr, int cch, PSIZE psize);

/*
 * Menu Drag and Drop
 */
NTSTATUS xxxClientRegisterDragDrop (HWND hwnd);
NTSTATUS xxxClientRevokeDragDrop (HWND hwnd);
NTSTATUS xxxClientLoadOLE(VOID);
void xxxMNSetGapState (ULONG_PTR uHitArea, UINT uIndex, UINT uFlags, BOOL fSet);
BOOL xxxMNDragOver(POINT * ppt, PMNDRAGOVERINFO pmndoi);
BOOL xxxMNDragLeave(VOID);
void xxxMNUpdateDraggingInfo (PMENUSTATE pMenuState, ULONG_PTR uHitArea, UINT uIndex);

/*
 * Scroll bar entry points
 */
VOID xxxSBTrackInit(PWND pwnd, LPARAM lParam, int curArea, UINT uType);
VOID SBCtlSetup(PSBWND psbwnd);
void CalcSBStuff(PWND pwnd, PSBCALC pSBCalc, BOOL fVert);
void CalcSBStuff2(PSBCALC  pSBCalc, LPRECT lprc, CONST PSBDATA pw, BOOL fVert);
BOOL xxxEnableScrollBar(PWND pwnd, UINT wSBflags, UINT wArrows);
void DrawSize(PWND pwnd, HDC hdc, int cxFrame, int cyFrame);
int xxxScrollWindowEx(PWND pwnd, int dx, int dy, LPRECT prcScroll,
        LPRECT prcClip, HRGN hrgnUpdate, LPRECT prcUpdate, DWORD flags);
void xxxDoScrollMenu(PWND pwndNotify, PWND pwndSB, BOOL fVert, LPARAM lParam);

/*
 * ICONS.C
 */
BOOL xxxInternalEnumWindow(PWND pwndNext, WNDENUMPROC_PWND lpfn, LPARAM lParam, UINT fEnumChildren);
VOID ISV_InitMinMaxInfo(PWND pwnd, PPOINT aptMinMaxWnd);
VOID ISV_ValidateMinMaxInfo(PWND pwnd, PPOINT aptMinMaxWnd);
/*
 * GETSET.C
 */
WORD  _SetWindowWord(PWND pwnd, int index, WORD value);
DWORD xxxSetWindowLong(PWND pwnd, int index, DWORD value, BOOL bAnsi);
ULONG_PTR xxxSetWindowData(PWND pwnd, int index, ULONG_PTR dwData, BOOL bAnsi);
LONG  xxxSetWindowStyle(PWND pwnd, int gwl, DWORD styleNew);
BOOL FCallerOk(PWND pwnd);

int IntersectVisRect(HDC, int, int, int, int);  // Imported from GDI
PCURSOR xxxGetWindowSmIcon(PWND pwnd, BOOL fDontSendMsg);
VOID xxxDrawCaptionBar(PWND pwnd, HDC hdc, UINT fFlags);
VOID xxxDrawScrollBar(PWND pwnd, HDC hdc, BOOL fVert);
VOID xxxTrackBox(PWND, UINT, WPARAM, LPARAM, PSBCALC);
VOID xxxTrackThumb(PWND, UINT, WPARAM, LPARAM, PSBCALC);
VOID xxxEndScroll(PWND pwnd, BOOL fCancel);
VOID xxxDrawWindowFrame(PWND pwnd, HDC hdc,
        BOOL fHungRedraw, BOOL fActive);
BOOL xxxInternalPaintDesktop(PWND pwnd, HDC hdc, BOOL fPaint);
VOID xxxSysCommand(PWND pwnd, DWORD cmd, LPARAM lParam);
VOID xxxHandleNCMouseGuys(PWND pwnd, UINT message, int htArea, LPARAM lParam);
void xxxCreateClassSmIcon(PCLS pcls);
HICON xxxCreateWindowSmIcon(PWND pwnd, HICON hIconBig, BOOL fCopyFromRes);
BOOL DestroyWindowSmIcon(PWND pwnd);
BOOL DestroyClassSmIcon(PCLS pcls);
UINT DWP_GetHotKey(PWND);
UINT DWP_SetHotKey(PWND, DWORD);
PWND HotKeyToWindow(DWORD);

VOID xxxDWP_DoNCActivate(PWND pwnd, DWORD dwFlags, HRGN hrgnClip);
#define NCA_ACTIVE          0x00000001
#define NCA_FORCEFRAMEOFF   0x00000002

VOID xxxDWP_ProcessVirtKey(UINT key);
BOOL xxxDWP_EraseBkgnd(PWND pwnd, UINT msg, HDC hdc);
VOID SetTiledRect(PWND pwnd, LPRECT lprc, PMONITOR pMonitor);
VOID LinkWindow(PWND pwnd, PWND pwndInsert, PWND pwndParent);
VOID UnlinkWindow(PWND pwndUnlink, PWND pwndParent);
VOID xxxDW_DestroyOwnedWindows(PWND pwndParent);
VOID xxxDW_SendDestroyMessages(PWND pwnd);
VOID xxxFreeWindow(PWND pwnd, PTL ptlpwndFree);
VOID xxxFW_DestroyAllChildren(PWND pwnd);

PHOTKEY FindHotKey(PTHREADINFO pti, PWND pwnd, int id, UINT fsModifiers, UINT vk,
        BOOL fUnregister, PBOOL pfKeysExist);

NTSTATUS _BuildNameList(
        PWINDOWSTATION pwinsta,
        PNAMELIST pNameList,
        UINT cbNameList,
        PUINT pcbNeeded);

VOID xxxHelpLoop(PWND pwnd);

NTSTATUS _BuildPropList(PWND pwnd, PROPSET aPropSet[], UINT cPropMax, PUINT pcPropReturned);
BOOL xxxSendEraseBkgnd(PWND pwnd, HDC hdcBeginPaint, HRGN hrgnUpdate);
LONG xxxSetScrollBar(PWND pwnd, int code, LPSCROLLINFO lpsi, BOOL fRedraw);
VOID IncPaintCount(PWND pwnd);
VOID DecPaintCount(PWND pwnd);
PPROP CreateProp(PWND pwnd);

/*
 * METRICS.C
 */
VOID xxxRecreateSmallIcons(PWND pwnd);

VOID   TransferWakeBit(PTHREADINFO pti, UINT message);
BOOL   SysHasKanji(VOID);
LONG   xxxBroadcastMessage(PWND, UINT, WPARAM, LPARAM, UINT, PBROADCASTMSG );

VOID   zzzSetFMouseMoved();

VOID   TimersProc(VOID);

VOID   PostMove(PQ pq);
VOID   DestroyWindowsTimers(PWND pwnd);

UINT_PTR StartTimers(VOID);

/*==========================================================================*/
/*                                                                          */
/*  Internal Function Declarations                                          */
/*                                                                          */
/*==========================================================================*/

LRESULT xxxTooltipWndProc(PWND, UINT, WPARAM, LPARAM);
LRESULT xxxSwitchWndProc(PWND, UINT, WPARAM, LPARAM);
LRESULT xxxDesktopWndProc(PWND, UINT, WPARAM, LPARAM);

LRESULT xxxSBWndProc(PSBWND, UINT, WPARAM, LPARAM);

VOID   DrawThumb2(PWND, PSBCALC, HDC, HBRUSH, BOOL, UINT);
UINT   GetWndSBDisableFlags(PWND, BOOL);

HANDLE _ConvertMemHandle(LPBYTE lpData, int cbData);

VOID zzzRegisterSystemThread (DWORD flags, DWORD reserved);

VOID zzzUpdateCursorImage();
void zzzCalcStartCursorHide(PW32PROCESS Process, DWORD timeAdd);
BOOL FinalUserInit();
BOOL LW_RegisterWindows(BOOL fSystem);

BOOL xxxSystemParametersInfo(UINT wFlag, DWORD wParam, LPVOID lParam, UINT flags);

PWINDOWSTATION CheckClipboardAccess(void);
PCLIP FindClipFormat(PWINDOWSTATION pwinsta, UINT format);
BOOL InternalSetClipboardData(PWINDOWSTATION pwinsta, UINT format,
        HANDLE hData, BOOL fGlobalHandle, BOOL fIncSerialNumber);
VOID DisownClipboard(PWND pwndClipOwner);

VOID CaretBlinkProc(PWND pwnd, UINT message, UINT_PTR id, LPARAM lParam);
VOID xxxRedrawFrame(PWND pwnd);
VOID xxxRedrawFrameAndHook(PWND pwnd);
VOID BltColor(HDC, HBRUSH, HDC, int, int, int, int, int, int, UINT);
VOID StoreMessage(LPMSG pmsg, PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD time);
VOID StoreQMessage(PQMSG pqmsg, PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD time, DWORD dwQEvent, ULONG_PTR dwExtraInfo);

#ifdef REDIRECTION
void __inline StoreQMessagePti(PQMSG pqmsg, PTHREADINFO pti)
{
    if (pqmsg->msg.message >= WM_MOUSEFIRST && pqmsg->msg.message <= WM_MOUSELAST) {
        pqmsg->msg.pt.x = LOWORD(pqmsg->msg.lParam);
        pqmsg->msg.pt.y = HIWORD(pqmsg->msg.lParam);
    } else {
        if (pti != NULL)
            pqmsg->msg.pt = pti->ptLast;
    }
    pqmsg->pti = pti;
}
#else
void __inline StoreQMessagePti(PQMSG pqmsg, PTHREADINFO pti)
{
    pqmsg->pti = pti;
}
#endif // REDIRECTION

VOID xxxSendSizeMessage(PWND pwnd, UINT cmdSize);

VOID xxxCheckFocus(PWND pwnd);
VOID OffsetChildren(PWND pwnd, int dx, int dy, LPRECT prcHitTest);

VOID xxxMoveSize(PWND pwnd, UINT cmdMove, DWORD wptStart);
VOID xxxShowOwnedWindows(PWND pwndOwner, UINT cmdShow, HRGN hrgnHung);
VOID xxxAdjustSize(PWND pwnd, LPINT lpcx, LPINT lpcy);

VOID xxxNextWindow(PQ pq, DWORD wParam);
VOID xxxOldNextWindow(UINT flags);
VOID xxxCancelCoolSwitch(void);

VOID xxxCancelTracking(VOID);
VOID xxxCancelTrackingForThread(PTHREADINFO ptiCancel);
VOID xxxCapture(PTHREADINFO pti, PWND pwnd, UINT code);
UINT SystoChar(UINT message, LPARAM lParam);

PHOOK PhkFirstValid(PTHREADINFO pti, int nFilterType);
PHOOK PhkFirstGlobalValid(PTHREADINFO pti, int nFilterType);
VOID  FreeHook(PHOOK phk);
int   xxxCallHook(int, WPARAM, LPARAM, int);
LRESULT xxxCallHook2(PHOOK, int, WPARAM, LPARAM, LPBOOL);
BOOL  xxxCallMouseHook(UINT message, PMOUSEHOOKSTRUCTEX pmhs, BOOL fRemove);
VOID  xxxCallJournalRecordHook(PQMSG pqmsg);
DWORD xxxCallJournalPlaybackHook(PQMSG pqmsg);
VOID  SetJournalTimer(DWORD dt, UINT msgJournal);
VOID  FreeThreadsWindowHooks(VOID);

BOOL xxxSnapWindow(PWND pwnd);

BOOL    DefSetText(PWND pwnd, PLARGE_STRING pstrText);
PWND    DSW_GetTopLevelCreatorWindow(PWND pwnd);
VOID    xxxCalcClientRect(PWND pwnd, LPRECT lprc, BOOL fHungRedraw);
VOID    xxxUpdateClientRect(PWND pwnd);

BOOL   AllocateUnicodeString(PUNICODE_STRING pstrDst, PUNICODE_STRING pstrSrc);

HANDLE CreateDesktopHeap(PWIN32HEAP* ppheapRet, ULONG ulHeapSize);

BOOL xxxSetInternalWindowPos(PWND pwnd, UINT cmdShow, LPRECT lprcWin,
            LPPOINT lpptMin);
VOID xxxMetricsRecalc(UINT wFlags, int dx, int dy, int dyCaption, int dyMenu);

VOID xxxBroadcastDisplaySettingsChange(PDESKTOP, BOOL);


/*
 * This is for SPI_GET/SETUSERPREFERENCE.
 * Currently it's for DWORD values only. A type field will be added
 *  so all new settings will be mostly handled through common SystemParametersInfo
 *  code.
 */
typedef struct tagPROFILEVALUEINFO {
    DWORD       dwValue;
    UINT        uSection;
    LPCWSTR     pwszKeyName;
} PROFILEVALUEINFO, *PPROFILEVALUEINFO;

/*
 *  SystemParametersInfo UserPreferences manipulation macros.
 *  SPI_ values in the BOOL or DWORD ranges (see winuser.w) are stored in
 *   gpdwCPUserPreferencesMask (BOOL) and gpviCPUserPreferences (DOWRD) (see kernel\globals.c).
 *  The following macros use the actual SPI_ value to determine the
 *   location of a given bit (BOOL mask) or DWORD in those  globals.
 *
 *  Macros to access DWORDs stored in gpviCPUserPreferences.
 *
 */
#define UPIsDWORDRange(uSetting)    \
            ((uSetting) >= SPI_STARTDWORDRANGE && (uSetting) < SPI_MAXDWORDRANGE)

/*
 * The first entry in gpviCPUserPreferences is reserved for the bitmask, so add 1.
 * Each setting has SPI_GET and SPI_SET, so divide by 2 to get the index
 */
#define UPDWORDIndex(uSetting)    \
            (1 + (((uSetting) - SPI_STARTDWORDRANGE) / 2))

/*
 * Macros to access BOOLs stored in gpdwCPUserPreferencesMask.
 */
#define UPIsBOOLRange(uSetting) \
    ((uSetting) >= SPI_STARTBOOLRANGE && (uSetting) < SPI_MAXBOOLRANGE)

/*
 * Each setting has SPI_GET and SPI_SET, so divide by 2 to get the index
 */
#define UPBOOLIndex(uSetting) \
    (((uSetting) - SPI_STARTBOOLRANGE) / 2)

/*
 * Returns a pointer to the DWORD that contains the bit corresponding to uSetting
 */
#define UPBOOLPointer(pdw, uSetting)    \
    (pdw + (UPBOOLIndex(uSetting) / 32))

/*
 * Returns the DWORD mask needed to test/set/clear the bit corresponding to uSetting
 */
#define UPBOOLMask(uSetting)    \
    (1 << (UPBOOLIndex(uSetting) - ((UPBOOLIndex(uSetting) / 32) * 32)))

#define TestUPBOOL(pdw, uSetting)   \
    (*UPBOOLPointer(pdw, uSetting) & UPBOOLMask(uSetting))

#define SetUPBOOL(pdw, uSetting)    \
    (*UPBOOLPointer(pdw, uSetting) |= UPBOOLMask(uSetting))

#define ClearUPBOOL(pdw, uSetting)                              \
{                                                               \
    UserAssert(UPIsBOOLRange(uSetting));                        \
    *UPBOOLPointer(pdw, uSetting) &= ~UPBOOLMask(uSetting);     \
}

/*
 * Use these macros ONLY if UPIsBOOLRange(SPI_GET ## uSetting) is TRUE
 */
#define TestUP(uSetting)    TestUPBOOL(gpdwCPUserPreferencesMask, SPI_GET ## uSetting)
#define SetUP(uSetting)     SetUPBOOL(gpdwCPUserPreferencesMask, SPI_GET ## uSetting)
#define ClearUP(uSetting)   ClearUPBOOL(gpdwCPUserPreferencesMask, SPI_GET ## uSetting)

#define IndexUP(uSetting) \
    (1 << (((uSetting) - SPI_STARTBOOLRANGE) / 2))

/*
 * Some settings (ie, UI Effects) are disabled when TestUP(UISETTINGS) is FALSE.
 */
#define TestEffectUP(uSetting)                                          \
    ((*gpdwCPUserPreferencesMask &                                      \
     (IndexUP(SPI_GET ## uSetting) | IndexUP(SPI_GETUIEFFECTS))) ==     \
     (IndexUP(SPI_GET ## uSetting) | IndexUP(SPI_GETUIEFFECTS)))

/*
 * Some UI effects have an "inverted" disabled value (ie, disabled is TRUE)
 */
#define TestEffectInvertUP(uSetting) (TestUP(uSetting) || !TestUP(UIEFFECTS))

/*
 * Some of these BOOL values are needed in the client side. This macro propagates them to gpsi->PUSIFlags
 * Note that the SI_ value must match the UPBOOLMask value for this to work fine.
 */
#define PropagetUPBOOLTogpsi(uSetting) \
    UserAssert((DWORD)(PUSIF_ ## uSetting) == (DWORD)UPBOOLMask(SPI_GET ## uSetting)); \
    COPY_FLAG(gpsi->PUSIFlags, TestUP(## uSetting), PUSIF_ ## uSetting)

BOOL xxxUpdatePerUserSystemParameters(HANDLE hToken, BOOL bUserLoggedOn);
VOID SaveVolatileUserSettings(VOID);

void MenuRecalc(void);

#define UNDERLINE_RECALC    0x7FFFFFFF      // MAXINT; tells us to recalc underline position


/*
 * Library management routines.
 */
int GetHmodTableIndex(PUNICODE_STRING pstrName);
VOID AddHmodDependency(int iatom);
VOID RemoveHmodDependency(int iatom);
HANDLE xxxLoadHmodIndex(int iatom, BOOL bWx86KnownDll);
VOID xxxDoSysExpunge(PTHREADINFO pti);


#define UnrealizeObject(hbr)    /* NOP for NT */


VOID DestroyThreadsObjects(VOID);
VOID MarkThreadsObjects(PTHREADINFO pti);

VOID FreeMessageList(PMLIST pml);
VOID DestroyThreadsHotKeys(VOID);
VOID DestroyWindowsHotKeys(PWND pwnd);

VOID DestroyClass(PPCLS ppcls);
VOID PatchThreadWindows(PTHREADINFO);
VOID DestroyCacheDCEntries(PTHREADINFO);

VOID DestroyProcessesClasses(PPROCESSINFO);

/*
 *  Win16 Task Apis Taskman.c
 */

VOID InsertTask(PPROCESSINFO ppi, PTDB ptdbNew);

BOOL xxxSleepTask(BOOL fInputIdle, HANDLE);

BOOL xxxUserYield(PTHREADINFO pti);
VOID xxxDirectedYield(DWORD dwThreadId);
VOID DirectedScheduleTask(PTHREADINFO ptiOld, PTHREADINFO ptiNew, BOOL bSendMsg, PSMS psms);
VOID WakeWowTask(PTHREADINFO Pti);

/*
 *  WowScheduler assertion for multiple wow tasks running simultaneously
 */

_inline
VOID
EnterWowCritSect(
    PTHREADINFO pti,
    PWOWPROCESSINFO pwpi
)
{
   if (!++pwpi->CSLockCount) {
       pwpi->CSOwningThread = pti;
       return;
       }

   RIPMSG2(RIP_ERROR,
         "MultipleWowTasks running simultaneously %x %x\n",
         pwpi->CSOwningThread,
         pwpi->CSLockCount
     );

   return;
}

_inline
VOID
ExitWowCritSect(
    PTHREADINFO pti,
    PWOWPROCESSINFO pwpi
)
{
   if (pti == pwpi->CSOwningThread) {
       pwpi->CSOwningThread = NULL;
       pwpi->CSLockCount--;
       }

   return;
}


////////////////////////////////////////////////////////////////////////////
//
// These are internal USER functions called from inside and outside the
// critical section (from server & client side).  They are a private 'API'.
//
// The prototypes appear in pairs:
//    as called from outside the critsect (from client-side)
//    as called from inside the critsect (from server-side)
// there must be layer code for the 1st function of each pair which validates
// handles, enters the critsect, calls the 2nd of the pair of functions, and
// leaves the critsect again.
//
// Things may have to change when we go client server: InitPwSB() mustn't
// return a pointer to global (server) data! etc.
//
////////////////////////////////////////////////////////////////////////////

BOOL  xxxFillWindow(PWND pwndBrush, PWND pwndPaint, HDC hdc, HBRUSH hbr);
HBRUSH xxxGetControlBrush(PWND pwnd, HDC hdc, UINT msg);
HBRUSH xxxGetControlColor(PWND pwndParent, PWND pwndCtl, HDC hdc, UINT message);
PSBINFO  _InitPwSB(PWND);
BOOL  _KillSystemTimer(PWND pwnd, UINT_PTR nIDEvent);
BOOL  xxxPaintRect(PWND, PWND, HDC, HBRUSH, LPRECT);

////////////////////////////////////////////////////////////////////////////
//
// these are called from stubs.c in the client so will probably go away
//
////////////////////////////////////////////////////////////////////////////


/*
 * From CLASS.C
 */
PCLS InternalRegisterClassEx(
        LPWNDCLASSEX lpwndcls,
        WORD fnid,
        DWORD flags
        );

PCURSOR GetClassIcoCur(PWND pwnd, int index);
PCURSOR xxxSetClassIcon(PWND pwnd, PCLS pcls, PCURSOR pCursor, int gcw);
ULONG_PTR xxxSetClassData(PWND pwnd, int index, ULONG_PTR dwData, BOOL bAnsi);
ULONG_PTR SetClassCursor(PWND pwnd, PCLS pcls, DWORD index, ULONG_PTR dwData);

/*
 * CREATEW.C
 * LATER IanJa: LPSTR -> LPCREATESTRUCT pCreateParams
 */

PWND xxxCreateWindowEx(DWORD dwStyle, PLARGE_STRING pstrClass,
        PLARGE_STRING pstrName, DWORD style, int x, int y, int cx,
        int cy, PWND pwndParent, PMENU pmenu, HANDLE hModule,
        LPVOID pCreateParams, DWORD dwExpWinVerAndFlags);
BOOL xxxDestroyWindow(PWND pwnd);

/*
 * SENDMSG.C
 */
LRESULT xxxSendMessageFF(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam, ULONG_PTR xParam);
LONG xxxSendMessageBSM(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam,
        LPBROADCASTSYSTEMMSGPARAMS pbsmParams);
LRESULT xxxSendMessageEx(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam, ULONG_PTR xParam);
LRESULT xxxSendMessage(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT xxxSendMessageTimeout(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam,
        UINT fuFlags, UINT uTimeout, PLONG_PTR lpdwResult);
BOOL xxxSendNotifyMessage(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);
void QueueNotifyMessage(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL xxxSendMessageCallback(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam,
        SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData, BOOL bClientReqest );
BOOL _ReplyMessage(LRESULT lRet);

/*
 * MN*.C
 */
int xxxTranslateAccelerator(PWND pwnd, LPACCELTABLE pat, LPMSG lpMsg);
BOOL  xxxSetMenu(PWND pwnd, PMENU pmenu, BOOL fRedraw);
VOID  ChangeMenuOwner(PMENU pMenu, PPROCESSINFO ppi);
int   xxxMenuBarDraw(PWND pwnd, HDC hdc, int cxFrame, int cyFrame);
BOOL  xxxDrawMenuBar(PWND pwnd);

BOOL xxxSetMenuItemInfo(PMENU pMenu, UINT nPos, BOOL fByPosition,
    LPMENUITEMINFOW lpmii, PUNICODE_STRING pstrItem);
BOOL _SetMenuContextHelpId(PMENU pMenu, DWORD dwContextHelpId);
BOOL _SetMenuFlagRtoL(PMENU pMenu);
BOOL xxxInsertMenuItem(PMENU pMenu, UINT wIndex, BOOL fByPosition,
        LPMENUITEMINFOW lpmii, PUNICODE_STRING pstrItem);
BOOL  xxxRemoveMenu(PMENU pMenu, UINT nPos, UINT dwFlags);
BOOL  xxxDeleteMenu(PMENU pMenu, UINT nPos, UINT dwFlags);
BOOL  xxxSetMenuInfo(PMENU pMenu, LPCMENUINFO lpmi);
BOOL  xxxTrackPopupMenuEx(PMENU pmenu, UINT dwFlags, int x, int y,
        PWND pwnd, CONST TPMPARAMS *pparams);
LONG FindBestPos(int x, int y, int cx, int cy, LPRECT prcExclude,
                UINT wFlags, PPOPUPMENU ppopupmenu, PMONITOR pMonitor);
BOOL _SetMenuDefaultItem(PMENU pMenu, UINT wId, BOOL fByPosition);
int xxxMenuItemFromPoint(PWND pwnd, PMENU pMenu, POINT ptScreen);
BOOL xxxGetMenuItemRect(PWND pwnd, PMENU pMenu, UINT uIndex, LPRECT lprcScreen);
PPOPUPMENU MNGetPopupFromMenu(PMENU pMenu, PMENUSTATE *ppMenuState);
PVOID LockDesktopMenu(PMENU * ppmenu, PMENU pmenu);
PVOID UnlockDesktopMenu(PMENU * ppmenu);
PMENU xxxLoadSysDesktopMenu (PMENU * ppmenu, UINT uMenuId);
__inline PVOID UnlockDesktopSysMenu(PMENU * ppmenu)
{
    ClearMF(*ppmenu, MFSYSMENU);
    return UnlockDesktopMenu(ppmenu);
}

/*
 * SHOWWIN.C
 */
BOOL xxxShowWindow(PWND pwnd, DWORD cmdShowAnimate);
BOOL _ShowWindowAsync(PWND pwnd, int cmdShow, UINT uWPFlags);
BOOL xxxShowOwnedPopups(PWND pwndOwner, BOOL fShow);

#define RDW_HASWINDOWRGN        0x8000
BOOL xxxSetWindowRgn(PWND pwnd, HRGN hrgn, BOOL fRedraw);

/*
 * SWP.C
 */
void SelectWindowRgn(PWND pwnd, HRGN hrgnClip);
PWND GetTopMostInsertAfter (PWND pwnd);

#define GETTOPMOSTINSERTAFTER(pwnd) \
    (gHardErrorHandler.pti == NULL ? NULL : GetTopMostInsertAfter(pwnd))

__inline BOOL FSwpTopmost(PWND pwnd)
{
    return (!!TestWF(pwnd, WEFTOPMOST) ^ !!TestWF(pwnd, WFTOGGLETOPMOST));
}


PWND CalcForegroundInsertAfter(PWND pwnd);
BOOL xxxSetWindowPos(PWND pwnd, PWND pwndInsertAfter, int x, int y,
        int cx, int cy, UINT flags);
PSMWP InternalBeginDeferWindowPos(int cwndGuess);
BOOL AllocateCvr (PSMWP psmwp, int cwndHint);
PSMWP _BeginDeferWindowPos(int cwndGuess);
PSMWP _DeferWindowPos(PSMWP psmwp, PWND pwnd, PWND pwndInsertAfter,
        int x, int y, int cx, int cy, UINT rgf);
BOOL xxxEndDeferWindowPosEx(PSMWP psmwp, BOOL fAsync);
BOOL xxxMoveWindow(PWND pwnd, int x, int y, int cx, int cy, BOOL fRedraw);
PWND GetLastTopMostWindow(VOID);
VOID xxxHandleWindowPosChanged(PWND pwnd, PWINDOWPOS ppos);
VOID IncVisWindows(PWND pwnd);
VOID DecVisWindows(PWND pwnd);
BOOL FVisCountable(PWND pwnd);
VOID SetVisible(PWND pwnd, UINT flags);
VOID ClrFTrueVis(PWND pwnd);

VOID SetWindowState(PWND pwnd, DWORD flags);
VOID ClearWindowState(PWND pwnd, DWORD flags);

BOOL xxxUpdateWindows(PWND pwnd, HRGN hrgn);

VOID SetMinimize(PWND pwnd, UINT uFlags);
#define SMIN_CLEAR            0
#define SMIN_SET              1

/*
 * DWP.C
 */
LRESULT xxxDefWindowProc(PWND, UINT, WPARAM, LPARAM);
PWND DWP_GetEnabledPopup(PWND pwndStart);


/*
 * INPUT.C
 */
BOOL xxxWaitMessage(VOID);
VOID IdleTimerProc(VOID);
VOID zzzWakeInputIdle(PTHREADINFO pti);
VOID SleepInputIdle(PTHREADINFO pti);
BOOL xxxInternalGetMessage(LPMSG lpmsg, HWND hwnd, UINT wMsgFilterMin,
        UINT wMsgFilterMax, UINT wRemoveMsg, BOOL fGetMessage);
#define xxxPeekMessage(lpmsg, hwnd, wMsgMin, wMsgMax, wRemoveMsg) \
    xxxInternalGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax, wRemoveMsg, FALSE)
#define xxxGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax) \
    xxxInternalGetMessage(lpmsg, hwnd, wMsgMin, wMsgMax, PM_REMOVE, TRUE)
DWORD _GetMessagePos(VOID);
LRESULT xxxDispatchMessage(LPMSG lpmsg);
UINT GetMouseKeyFlags(PQ pq);
/*
 * OLE private message signature. From rpc\runtime\mtrt\wmsgpack.hxx
 */
#define WMSG_MAGIC_VALUE 0xBABE
BOOL _PostMessage(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IPostQuitMessage(PTHREADINFO pti, int nExitCode);
BOOL _PostQuitMessage(int nExitCode);
BOOL _PostThreadMessage(PTHREADINFO pti, UINT message, WPARAM wParam, LPARAM lParam);
BOOL xxxTranslateMessage(LPMSG pmsg, UINT flags);
BOOL _GetInputState(VOID);
DWORD _GetQueueStatus(UINT);
typedef VOID (CALLBACK* MSGWAITCALLBACK)(DWORD DeviceType);
DWORD xxxMsgWaitForMultipleObjects(DWORD nCount, PVOID *apObjects, MSGWAITCALLBACK pfnNonMsg, PKWAIT_BLOCK WaitBlockArray);

BOOL FHungApp(PTHREADINFO pti, DWORD dwTimeFromLastRead);
VOID xxxRedrawHungWindow(PWND pwnd, HRGN hrgnFullDrag);
VOID xxxRedrawHungWindowFrame(PWND pwnd, BOOL fActive);
void zzzActiveCursorTracking (PWND pwnd);
PWND GetActiveTrackPwnd(PWND pwnd, Q **ppq);
int xxxActiveWindowTracking(PWND pwnd, UINT uMsg, int iHitTest);
VOID xxxHungAppDemon(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam);

#ifdef HUNGAPP_GHOSTING

/*
 * GHOST.C
 */
LRESULT xxxGhostWndProc(PWND, UINT, WPARAM, LPARAM);
VOID SignalGhost(PWND pwnd);
BOOL xxxCreateGhost(PWND pwnd);
VOID RemoveGhost(PWND pwnd);
PWND FindGhost(PWND pwnd);

#define WM_HUNGTHREAD (WM_USER + 0)

__inline VOID SignalGhost(PWND pwnd)
{
    _PostMessage(PWNDDESKTOP(pwnd), WM_HUNGTHREAD, 0, (LPARAM)HWq(pwnd));
}

#endif // HUNGAPP_GHOSTING

/*
 * TMSWITCH.C
 */
VOID xxxSwitchToThisWindow(PWND pwnd, BOOL fAltTab);

/*
 * TOUNICOD.C
 */
int xxxToUnicodeEx(UINT wVirtKey, UINT wScanCode, CONST BYTE *lpKeyState,
      LPWSTR pwszBuff, int cchBuff, UINT wFlags, HKL hkl);
int xxxInternalToUnicode(UINT wVirtKey, UINT wScanCode, CONST IN PBYTE pfvk,
      OUT PWCHAR awchChars, INT cChar, UINT uiTMFlags, OUT PDWORD pdwFlags, HKL hkl);

/*
 * HOTKEYS.C
 */
BOOL _RegisterHotKey(PWND pwnd, int id, UINT fsModifiers, UINT vk);
BOOL _UnregisterHotKey(PWND pwnd, int id);

/*
 * FOCUSACT.C
 */
PWND xxxSetFocus(PWND pwnd);
BOOL CanForceForeground(PPROCESSINFO ppi);
BOOL xxxStubSetForegroundWindow(PWND pwnd);
BOOL xxxSetForegroundWindow(PWND pwnd, BOOL fFlash);
PWND xxxSetActiveWindow(PWND pwnd);
PWND _GetActiveWindow(VOID);
BOOL xxxAllowSetForegroundWindow(DWORD dwProcessId);
BOOL _LockSetForegroundWindow(UINT uLockCode);

/*
 * UPDATE.C
 */
BOOL xxxInvalidateRect(PWND pwnd, LPRECT lprc, BOOL fErase);
BOOL xxxValidateRect(PWND pwnd, LPRECT lprc);
BOOL xxxInvalidateRgn(PWND pwnd, HRGN hrgn, BOOL fErase);
BOOL xxxValidateRgn(PWND pwnd, HRGN hrgn);
BOOL xxxUpdateWindow(PWND pwnd);
BOOL xxxGetUpdateRect(PWND pwnd, LPRECT lprc, BOOL fErase);
int  xxxGetUpdateRgn(PWND pwnd, HRGN hrgn, BOOL fErase);
int  _ExcludeUpdateRgn(HDC hdc, PWND pwnd);
int  CalcWindowRgn(PWND pwnd, HRGN hrgn, BOOL fClient);
VOID DeleteUpdateRgn(PWND pwnd);
BOOL xxxRedrawWindow(PWND pwnd, LPRECT lprcUpdate, HRGN hrgnUpdate, DWORD flags);
BOOL IntersectWithParents(PWND pwnd, LPRECT lprc);
VOID xxxInternalInvalidate(PWND pwnd, HRGN hrgnUpdate, DWORD flags);

/*
 * WINMGR.C
 */
BOOL xxxEnableWindow(PWND pwnd, BOOL fEnable);
int xxxGetWindowText(PWND pwnd, LPWSTR psz, int cchMax);
PWND xxxSetParent(PWND pwnd, PWND pwndNewParent);
BOOL xxxFlashWindow(PWND pwnd, DWORD dwFlags, DWORD dwTimeout);
extern ATOM gaFlashWState;
__inline DWORD GetFlashWindowState(PWND pwnd)
{
    return (DWORD)(ULONG_PTR)_GetProp(pwnd, MAKEINTATOM(gaFlashWState), PROPF_INTERNAL);
}
__inline void SetFlashWindowState(PWND pwnd, DWORD dwState)
{
    InternalSetProp(pwnd, MAKEINTATOM(gaFlashWState),
                    (HANDLE)ULongToPtr(dwState), PROPF_INTERNAL | PROPF_NOPOOL);
}
__inline void RemoveFlashWindowState(PWND pwnd)
{
    InternalRemoveProp(pwnd, MAKEINTATOM(gaFlashWState), PROPF_INTERNAL);
}
BOOL _GetWindowPlacement(PWND pwnd, PWINDOWPLACEMENT pwp);
BOOL xxxSetWindowPlacement(PWND pwnd, PWINDOWPLACEMENT pwp);
BOOL ValidateParentDepth(PWND pwnd, PWND pwndParent);
BOOL ValidateOwnerDepth(PWND pwnd, PWND pwndOwner);
void WPUpdateCheckPointSettings (PWND pwnd, UINT uWPFlags);

/*
 * DC.C
 */
HDC  _GetDC(PWND pwnd);
HDC  _GetDCEx(PWND pwnd, HRGN hrgnClip, DWORD flags);
HDC  _GetWindowDC(PWND pwnd);
BOOL _ReleaseDC(HDC hdc);
UINT ReleaseCacheDC(HDC hdc, BOOL fEndPaint);
HDC  CreateCacheDC(PWND, DWORD, PMONITOR);
BOOL DestroyCacheDC(PDCE *, HDC);
VOID InvalidateDce(PDCE pdce);
void DeleteHrgnClip(PDCE pdce);
PWND WindowFromCacheDC(HDC hdc);
PWND FastWindowFromDC(HDC hdc);
VOID DelayedDestroyCacheDC(VOID);
PDCE LookupDC(HDC hdc);
HDC GetMonitorDC(PDCE pdceOrig, PMONITOR pMonitor);
BOOL GetDCOrgOnScreen(HDC hdc, LPPOINT ppt);
__inline VOID MarkDCEInvalid(PDCE pdce)
{
    /*
     * Clear all bits, but these.
     */
    pdce->DCX_flags &= (DCX_CACHE | DCX_LAYERED);

    /*
     * Mark this cache entry as invalid
     */
    pdce->DCX_flags |= DCX_INVALID;
}

#ifdef USE_MIRRORING
void MirrorRect(PWND pwnd, LPRECT lprc);
BOOL MirrorRegion(PWND pwnd, HRGN hrgn, BOOL bUseClient);
#endif

/*
 * PAINT.C
 */
HDC  xxxBeginPaint(PWND pwnd, PAINTSTRUCT *lpps);
BOOL xxxEndPaint(PWND pwnd, PAINTSTRUCT *lpps);

/*
 * CAPTURE.C
 */
PWND xxxSetCapture(PWND pwnd);
BOOL xxxReleaseCapture(VOID);

/*
 * KEYBOARD.C
 */
SHORT _GetAsyncKeyState(int vk);
BOOL _SetKeyboardState(CONST BYTE *pKeyboard);
int _GetKeyboardType(int nTypeFlag);
VOID RegisterPerUserKeyboardIndicators(PUNICODE_STRING pProfileUserName);
VOID UpdatePerUserKeyboardIndicators(PUNICODE_STRING pProfileUserName);

#define TestRawKeyDown(vk)     TestKeyDownBit(gafRawKeyState, vk)
#define SetRawKeyDown(vk)      SetKeyDownBit(gafRawKeyState, vk)
#define ClearRawKeyDown(vk)    ClearKeyDownBit(gafRawKeyState, vk)
#define TestRawKeyToggle(vk)   TestKeyToggleBit(gafRawKeyState, vk)
#define SetRawKeyToggle(vk)    SetKeyToggleBit(gafRawKeyState, vk)
#define ClearRawKeyToggle(vk)  ClearKeyToggleBit(gafRawKeyState, vk)
#define ToggleRawKeyToggle(vk) ToggleKeyToggleBit(gafRawKeyState, vk)

/*
 * XLATE.C
 */
int  _GetKeyNameText(LONG lParam, LPWSTR lpString, int nSize);

/*
 * TIMERS.C
 */
BOOL _KillTimer(PWND pwnd, UINT_PTR nIDEvent);
PTIMER FindTimer(PWND pwnd, UINT_PTR nID, UINT flags, BOOL fKill);
VOID xxxSystemTimerProc(PWND pwnd, UINT msg, UINT_PTR id, LPARAM lParam);


/*
 * CARET.C
 */
BOOL zzzDestroyCaret(VOID);
BOOL xxxCreateCaret(PWND, HBITMAP, int, int);
BOOL zzzShowCaret(PWND);
BOOL zzzHideCaret(PWND);
BOOL _SetCaretBlinkTime(UINT);
BOOL zzzSetCaretPos(int, int);

/*
 * MSGBEEP.C
 */
BOOL xxxOldMessageBeep(VOID);
BOOL xxxMessageBeep(UINT wType);
VOID PlayEventSound(UINT idSound);

/*
 * WINWHERE.C
 */
PWND _ChildWindowFromPointEx(PWND pwndParent, POINT pt, UINT i);
PWND xxxWindowFromPoint(POINT pt);
PWND FAR SizeBoxHwnd(PWND pwnd);

/*
 * GETSET.C
 */
WORD  _SetWindowWord(PWND pwnd, int index, WORD value);
DWORD xxxSetWindowLong(PWND pwnd, int index, DWORD value, BOOL bAnsi);
#ifdef _WIN64
ULONG_PTR xxxSetWindowLongPtr(PWND pwnd, int index, ULONG_PTR value, BOOL bAnsi);
#else
#define xxxSetWindowLongPtr xxxSetWindowLong
#endif
#define __GetWindowLong(pwnd, index) ((LONG)(*(DWORD UNALIGNED *)((BYTE *)((pwnd) + 1) + (index))))
#define __GetWindowLongPtr(pwnd, index) ((LONG_PTR)(*(ULONG_PTR UNALIGNED *)((BYTE *)((pwnd) + 1) + (index))))
#if DBG
ULONG DBGGetWindowLong(PWND pwnd, int index);
#define _GetWindowLong DBGGetWindowLong
ULONG_PTR DBGGetWindowLongPtr(PWND pwnd, int index);
#define _GetWindowLongPtr DBGGetWindowLongPtr
#else
#define _GetWindowLong __GetWindowLong
#define _GetWindowLongPtr __GetWindowLongPtr
#endif

/*
 * CLIPBRD.C
 */
BOOL xxxOpenClipboard(PWND pwnd, LPBOOL lpfEmptyClient);
BOOL xxxCloseClipboard(PWINDOWSTATION pwinsta);
UINT _EnumClipboardFormats(UINT fmt);
BOOL xxxEmptyClipboard(PWINDOWSTATION pwinsta);
HANDLE xxxGetClipboardData(PWINDOWSTATION pwinsta, UINT fmt, PGETCLIPBDATA gcd);
BOOL _IsClipboardFormatAvailable(UINT fmt);
int _GetPriorityClipboardFormat(UINT *lpPriorityList, int cfmts);
PWND xxxSetClipboardViewer(PWND pwndClipViewerNew);
BOOL xxxChangeClipboardChain(PWND pwndRemove, PWND pwndNewNext);

/*
 * miscutil.c
 */
VOID SetDialogPointer(PWND pwnd, LONG_PTR lPtr);
VOID ZapActiveAndFocus(VOID);
BOOL xxxSetShellWindow(PWND pwnd, PWND pwndBkGnd);
BOOL _SetProgmanWindow(PWND pwnd);
BOOL _SetTaskmanWindow(PWND pwnd);

#define STW_SAME    ((PWND) 1)
void xxxSetTrayWindow(PDESKTOP pdesk, PWND pwnd, PMONITOR pMonitor);
BOOL xxxAddFullScreen(PWND pwnd, PMONITOR pMonitor);
BOOL xxxRemoveFullScreen(PWND pwnd, PMONITOR pMonitor);
BOOL xxxCheckFullScreen(PWND pwnd, PSIZERECT psrc);
BOOL IsTrayWindow(PWND);

#define FDoTray()   (SYSMET(ARRANGE) & ARW_HIDE)
#define FCallHookTray() (IsHooked(PtiCurrent(), WHF_SHELL))
#define FPostTray(p) (p->pDeskInfo->spwndTaskman)
#define FCallTray(p) (FDoTray() && ( FCallHookTray()|| FPostTray(p) ))

// ----------------------------------------------------------------------------
//
//  FTopLevel() - TRUE if window is a top level window
//
//  FHas31TrayStyles() -  TRUE if window is either full screen or has
//                        both a system menu and a caption
//                        (NOTE:  minimized windows always have captions)
//
// ----------------------------------------------------------------------------
#define FTopLevel(pwnd)         (pwnd->spwndParent == PWNDDESKTOP(pwnd))
#define FHas31TrayStyles(pwnd)    (TestWF(pwnd, WFFULLSCREEN) || \
                                  (TestWF(pwnd, WFSYSMENU | WFMINBOX) && \
                                  (TestWF(pwnd, WFCAPTION) || TestWF(pwnd, WFMINIMIZED))))
BOOL IsVSlick(PWND pwnd);
BOOL Is31TrayWindow(PWND pwnd);

/*
 * fullscr.c
 */

#if DBG
#define  VerifyVisibleMonitorCount()    \
    {                                   \
        PMONITOR pMonitor = gpDispInfo->pMonitorFirst;  \
        ULONG cVisMon = 0;                              \
        while (pMonitor) {                              \
            if (pMonitor->dwMONFlags & MONF_VISIBLE) {  \
                cVisMon++;                              \
            }                                           \
            pMonitor = pMonitor->pMonitorNext;          \
        }                                               \
        UserAssert(cVisMon == gpDispInfo->cMonitors);   \
    }
#endif

BOOL xxxMakeWindowForegroundWithState(PWND, BYTE);
void FullScreenCleanup();
LONG xxxUserChangeDisplaySettings(PUNICODE_STRING pstrDeviceName, LPDEVMODEW pDevMode,
    HWND hwnd, PDESKTOP pdesk, DWORD dwFlags, PVOID lParam, MODE PreviousMode);
BOOL xxxbFullscreenSwitch(BOOL bFullscreenSwitch, HWND hwnd);


/*
 * SBAPI.C
 */
BOOL xxxShowScrollBar(PWND, UINT, BOOL);
#define xxxSetScrollInfo(a,b,c,d) xxxSetScrollBar((a),(b),(c),(d))

/*
 * mngray.c
 */
BOOL xxxDrawState(HDC hdcDraw, HBRUSH hbrFore,
        LPARAM lData, int x, int y, int cx, int cy, UINT uFlags);

/*
 * SCROLLW.C
 */
BOOL _ScrollDC(HDC, int, int, LPRECT, LPRECT, HRGN, LPRECT);

/*
 * SPB.C
 */
VOID SpbCheckRect(PWND pwnd, LPRECT lprc, DWORD flags);
VOID SpbCheck(VOID);
PSPB FindSpb(PWND pwnd);
VOID FreeSpb(PSPB pspb);
VOID FreeAllSpbs(void);
VOID CreateSpb(PWND pwnd, UINT flags, HDC hdcScreen);
UINT RestoreSpb(PWND pwnd, HRGN hrgnUncovered, HDC *phdcScreen);
VOID SpbCheckPwnd(PWND pwnd);
VOID SpbCheckDce(PDCE pdce);
BOOL LockWindowUpdate2(PWND pwndLock, BOOL fThreadOverride);

/*
 * DRAWFRM.C
 */
BOOL FAR BitBltSysBmp(HDC hdc, int x, int y, UINT i);

/*
 * SYSMET.c
 */
BOOL APIENTRY xxxSetSysColors(PUNICODE_STRING pProfileUserName,int count, PUINT pIndex, LPDWORD pClrVal, UINT uOptions);
VOID SetSysColor(UINT icol, DWORD rgb, UINT uOptions);

/*
 * ICONS.C
 */
UINT xxxArrangeIconicWindows(PWND pwnd);
BOOL  _SetSystemMenu(PWND pwnd, PMENU pMenu);

/*
 * RMCREATE.C
 */
PICON _CreateIconIndirect(PICONINFO piconinfo);
PCURSOR _CreateCursor(HANDLE hModule, int iXhotspot, int iYhotspot,
        int iWidth, int iHeight, LPBYTE lpANDplane, LPBYTE lpXORplane);
PICON _CreateIcon(HANDLE hModule, int iWidth, int iHeight,
        BYTE bPlanes, BYTE bBitsPixel, LPBYTE lpANDplane, LPBYTE lpXORplane);
void DestroyUnlockedCursor(void *);
BOOL _DestroyCursor(PCURSOR, DWORD);
HANDLE _CreateAcceleratorTable(LPACCEL, int);

/*
 * CURSOR.C
 */
#if DBG
    PCURSOR DbgLockQCursor(PQ pq, PCURSOR pcur);
    #define LockQCursor(pq, pcur)   DbgLockQCursor(pq, pcur)
#else
    #define LockQCursor(pq, pcur)   Lock(&pq->spcurCurrent, pcur)
#endif // DBG

BOOL    _GetCursorPos(LPPOINT);
PCURSOR zzzSetCursor(PCURSOR pcur);
BOOL    zzzSetCursorPos(int x, int y);
int     zzzShowCursor(BOOL fShow);
BOOL    zzzClipCursor(LPCRECT prcClip);
PCURSOR _GetCursor(VOID);
BOOL    _SetCursorContents(PCURSOR pcur, PCURSOR pcurNew);
void    SetPointer(BOOL fSet);
void    zzzHideCursorNoCapture(void);
#define GETPCI(pcur) ((PCURSINFO)&(pcur->CI_FIRST))

/*
 * WMICON.C
 */
BOOL _DrawIconEx(HDC hdc, int x, int y, PCURSOR pcur, int cx, int cy,
        UINT istepIfAniCur, HBRUSH hbrush, UINT diFlags) ;
BOOL BltIcon(HDC hdc, int x, int y, int cx, int cy,
        HDC hdcSrc, PCURSOR pcursor, BOOL fMask, LONG rop);

void DBGValidateQueueStates(PDESKTOP pdesk);
/*
 * DESKTOP.C
 */

HDESK xxxCreateDesktop(
        POBJECT_ATTRIBUTES,
        KPROCESSOR_MODE,
        PUNICODE_STRING,
        LPDEVMODEW,
        DWORD,
        DWORD);

HDESK xxxOpenDesktop(POBJECT_ATTRIBUTES, KPROCESSOR_MODE, DWORD, DWORD, BOOL*);
BOOL OpenDesktopCompletion(PDESKTOP pdesk, HDESK hdesk, DWORD dwFlags, BOOL*);
BOOL xxxSwitchDesktop(PWINDOWSTATION, PDESKTOP, BOOL);
VOID zzzSetDesktop(PTHREADINFO pti, PDESKTOP pdesk, HDESK hdesk);
HDESK xxxGetInputDesktop(VOID);
BOOL xxxSetThreadDesktop(HDESK, PDESKTOP);
HDESK xxxGetThreadDesktop(DWORD, HDESK, KPROCESSOR_MODE);
BOOL xxxCloseDesktop(HDESK, KPROCESSOR_MODE);
BOOL xxxEnumDesktops(FARPROC, LONG, BOOL);
DWORD _SetDesktopConsoleThread(PDESKTOP pdesk, DWORD dwThreadId);
VOID xxxRealizeDesktop(PWND pwnd);

/*
 * WINSTA.C
 */
NTSTATUS CreateGlobalAtomTable(PVOID* ppAtomTable);
HWINSTA xxxCreateWindowStation(POBJECT_ATTRIBUTES ObjA,
    KPROCESSOR_MODE OwnershipMode,
    DWORD amRequest,
    HANDLE hKbdLayoutFile,
    DWORD offTable,
    PCWSTR pwszKLID,
    UINT uKbdInputLocale);
HWINSTA _OpenWindowStation(POBJECT_ATTRIBUTES, DWORD, KPROCESSOR_MODE);
BOOL _CloseWindowStation(HWINSTA hwinsta);
BOOL xxxSetProcessWindowStation(HWINSTA, KPROCESSOR_MODE);

PWINDOWSTATION _GetProcessWindowStation(HWINSTA *);
BOOL _LockWorkStation(VOID);

NTSTATUS ReferenceWindowStation(PETHREAD Thread, HWINSTA hwinsta,
        ACCESS_MASK amDesiredAccess, PWINDOWSTATION *ppwinsta, BOOL fUseDesktop);

/*
 * HOOKS.C
 */
PROC zzzSetWindowsHookAW(int nFilterType, PROC pfnFilterProc, DWORD dwFlags);
BOOL zzzUnhookWindowsHookEx(PHOOK phk);
BOOL zzzUnhookWindowsHook(int nFilterType, PROC pfnFilterProc);
LRESULT xxxCallNextHookEx(int nCode, WPARAM wParam, LPARAM lParam);
BOOL _CallMsgFilter(LPMSG lpMsg, int nCode);
void zzzCancelJournalling(void);
#if DBG
void DbgValidateHooks(PHOOK phk, int iType);
#else
#define DbgValidateHooks(phk, iType)
#endif


/*
 * SRVHOOK.C
 */
LRESULT fnHkINLPCWPEXSTRUCT(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);
LRESULT fnHkINLPCWPRETEXSTRUCT(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);

/*
 * QUEUE.C
 */
__inline BOOL IsShellProcess (PPROCESSINFO ppi)
{
    return ((ppi->rpdeskStartup != NULL)
        && (ppi->rpdeskStartup->pDeskInfo->ppiShellProcess == ppi));
}
__inline DWORD GetAppCompatFlags2ForPti(PTHREADINFO pti, WORD wVer)
{
    if (wVer < pti->dwExpWinVer) {
        return 0;
    }
    return pti->dwCompatFlags2;
}

VOID ClearWakeMask(VOID);
ULONG GetTaskName(PTHREADINFO pti, PWSTR Buffer, ULONG BufferLength);
PQMSG FindQMsg(PTHREADINFO, PMLIST, PWND, UINT, UINT, BOOL);
void zzzShowStartGlass(DWORD dwTimeout);
DWORD _GetChangeBits(VOID);
NTSTATUS xxxSetCsrssThreadDesktop(PDESKTOP pdesk, PDESKRESTOREDATA pdrdRestore);
NTSTATUS xxxRestoreCsrssThreadDesktop(PDESKRESTOREDATA pdrdRestore);

PQ GetJournallingQueue(PTHREADINFO pti);
void ClearAppStarting (PPROCESSINFO ppi);
#ifdef USE_MIRRORING
BOOL _GetProcessDefaultLayout(DWORD *pdwDefaultLayout);
BOOL _SetProcessDefaultLayout(DWORD dwDefaultLayout);
#endif

/*
 * EXITWIN.C
 */
LONG xxxClientShutdown(PWND pwnd, WPARAM wParam);
BOOL xxxRegisterUserHungAppHandlers( PFNW32ET pfnW32EndTask, HANDLE hEventWowExec);

/*
 * INIT.C
 */
BOOL CreateTerminalInput(PTERMINAL);

VOID LW_LoadSomeStrings(VOID);
VOID LW_LoadProfileInitData();
VOID xxxODI_ColorInit(PUNICODE_STRING pProfileUserName);
HRGN InitCreateRgn(VOID);
VOID xxxUpdateSystemCursorsFromRegistry(PUNICODE_STRING pProfileUserName);
VOID xxxUpdateSystemIconsFromRegistry(PUNICODE_STRING pProfileUserName);
void RegisterLPK(DWORD);
HBITMAP CreateCaptionStrip();

BOOL LW_BrushInit(VOID);
VOID xxxLW_LoadFonts(BOOL bRemote);

VOID _LoadCursorsAndIcons(VOID);

void UnloadCursorsAndIcons(VOID);

void IncrMBox(void);
void DecrMBox(void);
void InitAnsiOem(PCHAR pOemToAnsi, PCHAR pAnsiToOem);
int  xxxAddFontResourceW(LPWSTR lpFile, FLONG flags, DESIGNVECTOR *pdv);
void EnforceColorDependentSettings(void);


/*
 * ACCESS.C
 */
VOID xxxUpdatePerUserAccessPackSettings(PUNICODE_STRING pProfileUserName);

/*
 * inctlpan.c
 */
VOID GetWindowNCMetrics(LPNONCLIENTMETRICS lpnc);

HFONT FAR PASCAL CreateFontFromWinIni(PUNICODE_STRING pProfileUserName,LPLOGFONT lplf, UINT idFont);
VOID SetMinMetrics(PUNICODE_STRING pProfileUserName,LPMINIMIZEDMETRICS lpmin);
BOOL xxxSetWindowNCMetrics(PUNICODE_STRING pProfileUserName,LPNONCLIENTMETRICS lpnc, BOOL fSizeChange, int clNewBorder);
BOOL SetIconMetrics(PUNICODE_STRING pProfileUserName,LPICONMETRICS lpicon);
BOOL xxxSetNCFonts(PUNICODE_STRING pProfileUserName, LPNONCLIENTMETRICS lpnc);
BOOL CreateBitmapStrip(VOID);
BOOL UpdateWinIniInt(PUNICODE_STRING pProfileUserName, UINT idSection, UINT wKeyNameId, int value);

/*
 * rare.c
 */
void FAR SetDesktopMetrics();

void SetMsgBox(PWND pwnd);

BOOL _RegisterShellHookWindow(PWND pwnd);
BOOL _DeregisterShellHookWindow(PWND pwnd);
BOOL xxxSendMinRectMessages(PWND pwnd, RECT *lpRect);
void PostShellHookMessages(UINT message, LPARAM lParam);
VOID _ResetDblClk(VOID);
VOID xxxSimulateShiftF10(VOID);
BOOL VWPLAdd(PVWPL *ppvwpl, PWND pwnd, DWORD dwThreshold);
BOOL VWPLRemove(PVWPL *ppvwpl, PWND pwnd);
PWND VWPLNext(PVWPL pvwpl, PWND pwndPrev, DWORD *pnPrev);

/*
 * DDETRACK STUFF
 */

typedef struct tagFREELIST {
    struct tagFREELIST *next;
    HANDLE h;                           // CSR client side GMEM_DDESHARE handle
    DWORD flags;                        // XS_ flags describing data
} FREELIST, *PFREELIST;

typedef struct tagDDEIMP {
    SECURITY_QUALITY_OF_SERVICE qos;
    SECURITY_CLIENT_CONTEXT ClientContext;
    short cRefInit;
    short cRefConv;
} DDEIMP, *PDDEIMP;

typedef struct tagDDECONV {
    THROBJHEAD          head;           // HM header
    struct tagDDECONV   *snext;
    struct tagDDECONV   *spartnerConv;  // siamese twin
    PWND                spwnd;          // associated pwnd
    PWND                spwndPartner;   // associated partner pwnd
    struct tagXSTATE    *spxsOut;       // transaction info queue - out point
    struct tagXSTATE    *spxsIn;        // transaction info queue - in point
    struct tagFREELIST  *pfl;           // free list
    DWORD               flags;          // CXF_ flags
    struct tagDDEIMP    *pddei;         // impersonation information
} DDECONV, *PDDECONV;

typedef DWORD (FNDDERESPONSE)(PDWORD pmsg, LPARAM *plParam, PDDECONV pDdeConv);
typedef FNDDERESPONSE *PFNDDERESPONSE;

typedef struct tagXSTATE {
    THROBJHEAD          head;           // HM header
    struct tagXSTATE    *snext;
    PFNDDERESPONSE      fnResponse;     // proc to handle next msg.
    HANDLE              hClient;        // GMEM_DDESAHRE handle on client side
    HANDLE              hServer;        // GMEM_DDESHARE handle on server side
    PINTDDEINFO         pIntDdeInfo;    // DDE data being transfered
    DWORD               flags;          // XS_ flags describing transaction/data
} XSTATE, *PXSTATE;

// values for flags field

#define CXF_IS_SERVER               0x0001
#define CXF_TERMINATE_POSTED        0x0002
#define CXF_PARTNER_WINDOW_DIED     0x0004
#define CXF_INTRA_PROCESS           0x8000

BOOL xxxDDETrackSendHook(PWND pwndTo, DWORD message, WPARAM wParam, LPARAM lParam);
DWORD xxxDDETrackPostHook(PUINT pmessage, PWND pwndTo, WPARAM wParam, LPARAM *plParam, BOOL fSent);
VOID FreeDdeXact(PXSTATE pxs);

VOID xxxDDETrackGetMessageHook(PMSG pmsg);
VOID xxxDDETrackWindowDying(PWND pwnd, PDDECONV pDdeConv);
VOID FreeDdeConv(PDDECONV pDdeConv);
BOOL _ImpersonateDdeClientWindow(PWND pwndClient, PWND pwndServer);

HBITMAP _ConvertBitmap(HBITMAP hBitmap);

typedef struct tagMONITORPOS
{
    RECT     rcMonitor;     /* where the monitor rect was */
    RECT     rcWork;        /* where the work rect was */
    PMONITOR pMonitor;      /* what new monitor gets its windows */
} MONITORPOS, *PMONITORPOS;

typedef struct tagMONITORRECTS
{
    int             cMonitor;   /* number of monitors */
    MONITORPOS      amp[1];     /* the monitor positions */
} MONITORRECTS, *PMONITORRECTS;


PMONITORRECTS SnapshotMonitorRects(void);
void xxxDesktopRecalc(PMONITORRECTS pmrOld);

BOOL _SetDoubleClickTime(UINT);
BOOL APIENTRY _SwapMouseButton(BOOL fSwapButtons);
VOID xxxDestroyThreadInfo(VOID);

BOOL _GetWindowPlacement(PWND pwnd, PWINDOWPLACEMENT pwp);

PMENU xxxGetSystemMenu(PWND pWnd, BOOL bRevert);
PMENU _CreateMenu(VOID);
PMENU _CreatePopupMenu(VOID);
BOOL  _DestroyMenu(PMENU pMenu);
DWORD _CheckMenuItem(PMENU pMenu, UINT wIDCheckItem, UINT wCheck);
DWORD xxxEnableMenuItem(PMENU pMenu, UINT wIDEnableItem, UINT wEnable);
WINUSERAPI UINT  _GetMenuItemID(PMENU pMenu, int nPos);
WINUSERAPI UINT  _GetMenuItemCount(PMENU pMenu);

PMENU _GetMenu(PWND pWnd);
BOOL _SetMenuContextHelpId(PMENU pMenu, DWORD dwContextHelpId);

PWND _GetNextQueueWindow(PWND pwnd, BOOL fDir, BOOL fAltEsc);

UINT_PTR _SetSystemTimer(PWND pwnd, UINT_PTR nIDEvent, DWORD dwElapse,
        TIMERPROC_PWND pTimerFunc);
BOOL   _SetClipboardData(UINT fmt, HANDLE hData, BOOL fGlobalHandle, BOOL fIncSerialNumber);
WORD   _SetClassWord(PWND pwnd, int index, WORD value);
DWORD  xxxSetClassLong(PWND pwnd, int index, DWORD value, BOOL bAnsi);
#ifdef _WIN64
ULONG_PTR xxxSetClassLongPtr(PWND pwnd, int index, ULONG_PTR value, BOOL bAnsi);
#else
#define xxxSetClassLongPtr  xxxSetClassLong
#endif
ATOM   xxxRegisterClassEx(LPWNDCLASSEX pwc, PCLSMENUNAME pcmn,
        WORD fnid, DWORD dwFlags, LPDWORD pdwWOW);
BOOL  xxxHiliteMenuItem(PWND pwnd, PMENU pmenu, UINT cmd, UINT flags);
HANDLE _CreateAcceleratorTable(LPACCEL paccel, int cbAccel);
HANDLE xxxGetInputEvent(DWORD dwWakeMask);
BOOL   _UnregisterClass(LPCWSTR lpszClassName, HANDLE hModule, PCLSMENUNAME pcmn);
ATOM   _GetClassInfoEx(HANDLE hModule, LPCWSTR lpszClassName, LPWNDCLASSEX pwc, LPWSTR *ppszMenuName, BOOL bAnsi);
PWND   _WindowFromDC(HDC hdc);
PCLS   _GetWOWClass(HANDLE hModule, LPCWSTR lpszClassName);
LRESULT xxxHkCallHook(PHOOK phk, int nCode, WPARAM wParam, LPARAM lParam);
PHOOK  zzzSetWindowsHookEx(HANDLE hmod, PUNICODE_STRING pstrLib,
        PTHREADINFO ptiThread, int nFilterType, PROC pfnFilterProc, DWORD dwFlags);
DWORD  GetDebugHookLParamSize(WPARAM wParam, PDEBUGHOOKINFO pdebughookstruct);
BOOL   _RegisterLogonProcess(DWORD dwProcessId, BOOL fSecure);
UINT   _LockWindowStation(PWINDOWSTATION pwinsta);
BOOL   _UnlockWindowStation(PWINDOWSTATION pwinsta);
UINT   _SetWindowStationUser(PWINDOWSTATION pwinsta, PLUID pluidUser,
        PSID psidUser, DWORD cbsidUser);
BOOL   _SetDesktopBitmap(PDESKTOP pdesk, HBITMAP hbitmap, DWORD dwStyle);

BOOL   _SetLogonNotifyWindow(PWND pwnd);


BOOL   _RegisterTasklist(PWND pwndTasklist);
LONG_PTR _SetMessageExtraInfo(LONG_PTR);
VOID   xxxRemoveEvents(PQ pq, int nQueue, DWORD flags);

PPCLS _InnerGetClassPtr(ATOM atom, PPCLS ppclsList, HANDLE hModule);

/*
 * ntcb.h funtions.
 */
DWORD ClientGetListboxString(PWND hwnd, UINT msg,
        WPARAM wParam, PVOID lParam,
        ULONG_PTR xParam, PROC xpfn, DWORD dwSCMSFlags, BOOL bNotString, PSMS psms);
HANDLE ClientLoadLibrary(PUNICODE_STRING pstrLib, BOOL bWx86KnownDll);
BOOL ClientFreeLibrary(HANDLE hmod);
BOOL xxxClientGetCharsetInfo(LCID lcid, PCHARSETINFO pcs);
BOOL ClientExitProcess(PFNW32ET pfn, DWORD dwExitCode);
BOOL ClientGrayString(GRAYSTRINGPROC pfnOutProc, HDC hdc,
        DWORD lpData, int nCount);
BOOL CopyFromClient(LPBYTE lpByte, LPBYTE lpByteClient, DWORD cch,
        BOOL fString, BOOL fAnsi);
BOOL CopyToClient(LPBYTE lpByte, LPBYTE lpByteClient,
        DWORD cchMax, BOOL fAnsi);
VOID ClientNoMemoryPopup(VOID);
NTSTATUS ClientThreadSetup(VOID);

VOID ClientDeliverUserApc(VOID);

BOOL ClientImmLoadLayout(HKL, PIMEINFOEX);
DWORD ClientImmProcessKey(HWND, HKL, UINT, LPARAM, DWORD);

NTSTATUS xxxUserModeCallback (ULONG uApi, PVOID pIn, ULONG cbIn, PVOID pOut, ULONG cbOut);


PCURSOR ClassSetSmallIcon(
    PCLS pcls,
    PCURSOR pcursor,
    BOOL fServerCreated);

BOOL _GetTextMetricsW(
    HDC hdc,
    LPTEXTMETRICW ptm);

int xxxDrawMenuBarTemp(
    PWND pwnd,
    HDC hdc,
    LPRECT lprc,
    PMENU pMenu,
    HFONT hFont);

BOOL xxxDrawCaptionTemp(
    PWND pwnd,
    HDC hdc,
    LPRECT lprc,
    HFONT hFont,
    PCURSOR pcursor,
    PUNICODE_STRING pstrText OPTIONAL,
    UINT flags);

WORD xxxTrackCaptionButton(
    PWND pwnd,
    UINT hit);

void GiveForegroundActivateRight(HANDLE hPid);
BOOL HasForegroundActivateRight(HANDLE hPid);
BOOL FRemoveForegroundActivate(PTHREADINFO pti);
void RestoreForegroundActivate();
void CancelForegroundActivate();

#define ACTIVATE_ARRAY_SIZE 5
extern HANDLE ghCanActivateForegroundPIDs[ACTIVATE_ARRAY_SIZE];

__inline void GiveForegroundActivateRight(HANDLE hPid)
{
    static int index = 0;

    TAGMSG1(DBGTAG_FOREGROUND, "Giving %x foreground activate right", hPid);
    ghCanActivateForegroundPIDs[index++] = hPid;
    if(index == ACTIVATE_ARRAY_SIZE) {
        index = 0;
    }
}

__inline BOOL HasForegroundActivateRight(HANDLE hPid)
{
    int i = 0;

    for(; i < ACTIVATE_ARRAY_SIZE; ++i) {
            if(ghCanActivateForegroundPIDs[i] == hPid) {
                TAGMSG1(DBGTAG_FOREGROUND, "HasForegroundActivateRight: Found %x", hPid);
                return TRUE;
            }
     }

     TAGMSG1(DBGTAG_FOREGROUND, "HasForegroundActivateRight: Did NOT find %x", hPid);
     return FALSE;
}


#define WHERE_NOONE_CAN_SEE_ME ((int) -32000)
BOOL MinToTray(PWND pwnd);

void xxxUpdateThreadsWindows(
    PTHREADINFO pti,
    PWND pwnd,
    HRGN hrgnFullDrag);

NTSTATUS xxxQueryInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL);

NTSTATUS xxxSetInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength);

#ifdef USE_MIRRORING
NTSTATUS GetProcessDefaultWindowOrientation(
    IN HANDLE hProcess,
    OUT DWORD *pdwDefaultOrientation);

NTSTATUS SetProcessDefaultWindowOrientation(
    IN HANDLE hProcess,
    IN DWORD dwDefaultOrientation);
#endif

NTSTATUS SetInformationProcess(
    IN HANDLE hProcess,
    IN USERPROCESSINFOCLASS ProcessInfoClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength);


NTSTATUS xxxConsoleControl(
    IN CONSOLECONTROL ConsoleControl,
    IN PVOID ConsoleInformation,
    IN ULONG ConsoleInformationLength);


/***************************************************************************\
* String Table Defines
*
* KERNEL\STRID.MC has a nice big table of strings that are meant to be
* localized.  Before use, the strings are pulled from the resource table
* with LoadString, passing it one of the following string ids.
*
* NOTE: Only strings that need to be localized should be added to the
*       string table.  Class name strings, etc are NOT localized.
*
* LATER: All string table entries should be reexamined to be sure they
*        conform to the note above.
*
\***************************************************************************/

#define OCR_APPSTARTING         32650

/*
 * Win Event Hook struct
 */
typedef struct tagEVENTHOOK {
    THROBJHEAD          head;                //
    struct tagEVENTHOOK *pehNext;            // 0x14 Next event hook
    UINT                eventMin;            // 0x18 Min event (>=) to hook
    UINT                eventMax;            // 0x1C Max event (<=) to hook
    UINT                fDestroyed:1;        // 0x20 If orphaned while in use
// IanJa - we don't need this bit               0x24
//  UINT                f32Bit:1;            // 0x28 If 32-bit client
    UINT                fIgnoreOwnThread:1;  // 0x2C Ignore events for installer thread
    UINT                fIgnoreOwnProcess:1; //      Ignore events for installer process
    UINT                fSync:1;             //      Sync event (inject DLL into each process)
    UINT                fWx86KnownDll:1;     //      x86 emulation? (IanJa - provide support)
    HANDLE              hEventProcess;       // 0x30 Process being hooked
    DWORD               idEventThread;       // 0x34 Thread being hooked
// (IanJa - have head.pti: don't need ppiInstaller, can use GETPTI(peh)->ppi)
//  DWORD               idInstallerProcess;  // 0x38 ID of process that installed hook
    ULONG_PTR            offPfn;              // 0x3C offset event proc
    int                 ihmod;               // 0x40 index of module containing event proc
    LPWSTR              pwszModulePath;      // 0x44 Path of module library for global sync.
} EVENTHOOK, *PEVENTHOOK;                    // 0x48 (size)

typedef struct tagNOTIFY {
    struct tagNOTIFY *pNotifyNext;         // 0x00 Next notification
    PEVENTHOOK        spEventHook;         // 0x04 Event this refers to
    DWORD             event;               // 0x08 Event
    HWND              hwnd;                // 0x0C hwnd to ask about it
    LONG              idObject;            // 0x10 object ID
    LONG              idChild;             // 0x14 child id
    DWORD             idSenderThread;      // 0x18 Thread generating event
    DWORD             dwEventTime;         // 0x1C Event time
    DWORD             dwWEFlags;           // 0x20 WEF_DEFERNOTIFY etc.
    PTHREADINFO       ptiReceiver;         // 0x24 Thread receiving event
} NOTIFY, *PNOTIFY;                        // 0x28

VOID xxxWindowEvent(DWORD event, PWND pwnd, LONG idObject, LONG idChild, DWORD dwFlags);
#define WEF_USEPWNDTHREAD 0x0001
#define WEF_DEFERNOTIFY   0x0002
#define WEF_ASYNC         0x0004
#define WEF_POSTED        0x0008

#define DeferWinEventNotify()      CheckCritIn();          \
                                   gdwDeferWinEvent++
#define IsWinEventNotifyDeferred() (gdwDeferWinEvent > 0)
#define IsWinEventNotifyDeferredOK() (!IsWinEventNotifyDeferred() || ISATOMICCHECK())
#define zzzEndDeferWinEventNotify()                        \
        UserAssert(IsWinEventNotifyDeferred());            \
        UserAssert(FWINABLE() || gnDeferredWinEvents == 0);  \
        CheckCritIn();                                     \
        if (--gdwDeferWinEvent == 0) {                     \
            if (FWINABLE() && (gpPendingNotifies != NULL)) { \
                xxxFlushDeferredWindowEvents();            \
            }                                              \
        }                                                  \
        UserAssert(FWINABLE() || gnDeferredWinEvents == 0)

/*
 * Only use this one for bookkeeping gdwDeferWinEvent,
 * which may be required without leaving the critical section.
 */
#define EndDeferWinEventNotifyWithoutProcessing()          \
        UserAssert(IsWinEventNotifyDeferred());            \
        UserAssert(FWINABLE() || gnDeferredWinEvents == 0);  \
        CheckCritIn();                                     \
        --gdwDeferWinEvent

#define zzzWindowEvent(event, pwnd, idObject, idChild, dwFlags) \
        xxxWindowEvent(event, pwnd, idObject, idChild,          \
            IsWinEventNotifyDeferred() ? (dwFlags) | WEF_DEFERNOTIFY : (dwFlags))

VOID xxxFlushDeferredWindowEvents();

BOOL xxxClientCallWinEventProc(WINEVENTPROC pfn, PEVENTHOOK pEventHook, PNOTIFY pNotify);
void DestroyEventHook(PEVENTHOOK);
VOID FreeThreadsWinEvents(PTHREADINFO pti);

BOOL       _UnhookWinEvent(PEVENTHOOK peh);
VOID       DestroyNotify(PNOTIFY pNotify);
PEVENTHOOK xxxProcessNotifyWinEvent(PNOTIFY pNotify);
PEVENTHOOK _SetWinEventHook(DWORD eventMin, DWORD eventMax,
        HMODULE hmodWinEventProc, PUNICODE_STRING pstrLib,
        WINEVENTPROC pfnWinEventProc, HANDLE hEventProcess,
        DWORD idEventThread, DWORD dwFlags);
BOOL _GetGUIThreadInfo(PTHREADINFO pti, PGUITHREADINFO pgui);
BOOL xxxGetTitleBarInfo(PWND pwnd, PTITLEBARINFO ptbi);
BOOL _GetComboBoxInfo(PWND pwnd, PCOMBOBOXINFO ptbi);
DWORD _GetListBoxInfo(PWND pwnd);
BOOL _GetScrollBarInfo(PWND pwnd, LONG idObject, PSCROLLBARINFO ptbi);
PWND _GetAncestor(PWND pwnd, UINT gaFlags);
PWND _RealChildWindowFromPoint(PWND pwndParent, POINT pt);
BOOL _GetAltTabInfo(int iItem, PALTTABINFO pati,
        LPWSTR lpszItemText, UINT cchItemText, BOOL bAnsi);
BOOL xxxGetMenuBarInfo(PWND pwnd, long idObject, long idItem, PMENUBARINFO pmbi);

typedef HWND *PHWND;

typedef struct tagSwitchWndInfo {

    PBWL    pbwl;               // Pointer to the window list built.
    PHWND   phwndLast;          // Pointer to the last window in the list.
    PHWND   phwndCurrent;       // pointer to the current window.

    INT     iTotalTasks;        // Total number of tasks.
    INT     iTasksShown;        // Total tasks shown.
    BOOL    fScroll;            // Is there a need to scroll?

    INT     iFirstTaskIndex;    // Index to the first task shown.

    INT     iNoOfColumns;       // Max Number of tasks per row.
    INT     iNoOfRows;          // Max Number of rows of icons in the switch window.
    INT     iIconsInLastRow;    // Icons in last row.
    INT     iCurCol;            // Current column where hilite lies.
    INT     iCurRow;            // Current row where hilite lies.
    INT     cxSwitch;           // Switch Window dimensions.
    INT     cySwitch;
    POINT   ptFirstRowStart;    // Top left corner of the first Icon Slot.
    RECT    rcTaskName;         // Rect where Task name is displayed.
    BOOL    fJournaling;        // Determins how we check the keyboard state
} SWITCHWNDINFO, *PSWINFO;

typedef struct tagSWITCHWND {
    WND;
    PSWINFO pswi;
} SWITCHWND, *PSWITCHWND;

typedef struct tagHOTKEYSTRUCT {
    PWND  spwnd;
    DWORD key;
} HOTKEYSTRUCT, *PHOTKEYSTRUCT;

#define LANGTOGGLEKEYS_SIZE 3

/*
 * ACCF_ and PUDF_ flags share the same field. ACCF fields
 * are so named because they may later move to a differnt
 * struct.
 */
#define ACCF_DEFAULTFILTERKEYSON        0x00000001
#define ACCF_DEFAULTSTICKYKEYSON        0x00000002
#define ACCF_DEFAULTMOUSEKEYSON         0x00000004
#define ACCF_DEFAULTTOGGLEKEYSON        0x00000008
#define ACCF_DEFAULTTIMEOUTON           0x00000010
#define ACCF_DEFAULTKEYBOARDPREF        0x00000020
#define ACCF_DEFAULTSCREENREADER        0x00000040
#define ACCF_DEFAULTHIGHCONTRASTON      0x00000080
#define ACCF_ACCESSENABLED              0x00000100
#define ACCF_IGNOREBREAKCODE            0x00000400
#define ACCF_FKMAKECODEPROCESSED        0x00000800
#define ACCF_MKVIRTUALMOUSE             0x00001000
#define ACCF_MKREPEATVK                 0x00002000
#define ACCF_FIRSTTICK                  0x00004000
#define ACCF_SHOWSOUNDSON               0x00008000

/*
 * NOTE: PUDF_ANIMATE must have the same value as MINMAX_ANIMATE.
 */
#define PUDF_ANIMATE                    0x00010000

#define ACCF_KEYBOARDPREF               0x00020000
#define ACCF_SCREENREADER               0x00040000
#define PUDF_BEEP                       0x00080000  /* Warning beeps allowed?                   */
#define PUDF_EXTENDEDSOUNDS             0x00100000  /* Extended sounds enabling                 */
#define PUDF_DRAGFULLWINDOWS            0x00200000  /* Drag xor rect or full windows            */
#define PUDF_ICONTITLEWRAP              0x00400000  /* Wrap icon titles or just use single line */
#define PUDF_FONTSARELOADED             0x00800000
#define PUDF_POPUPINUSE                 0x01000000
#define PUDF_MENUSTATEINUSE             0x02000000
#define PUDF_VDMBOUNDSACTIVE            0x04000000
#define PUDF_ALLOWFOREGROUNDACTIVATE    0x08000000
#define PUDF_DRAGGINGFULLWINDOW         0x10000000
#define PUDF_LOCKFULLSCREEN             0x20000000
#define PUDF_GSMWPINUSE                 0x40000000

#define TEST_ACCF(f)               TEST_FLAG(gdwPUDFlags, f)
#define TEST_BOOL_ACCF(f)          TEST_BOOL_FLAG(gdwPUDFlags, f)
#define SET_ACCF(f)                SET_FLAG(gdwPUDFlags, f)
#define CLEAR_ACCF(f)              CLEAR_FLAG(gdwPUDFlags, f)
#define SET_OR_CLEAR_ACCF(f, fSet) SET_OR_CLEAR_FLAG(gdwPUDFlags, f, fSet)
#define TOGGLE_ACCF(f)             TOGGLE_FLAG(gdwPUDFlags, f)

#define TEST_PUDF(f)               TEST_FLAG(gdwPUDFlags, f)
#define TEST_BOOL_PUDF(f)          TEST_BOOL_FLAG(gdwPUDFlags, f)
#define SET_PUDF(f)                SET_FLAG(gdwPUDFlags, f)
#define CLEAR_PUDF(f)              CLEAR_FLAG(gdwPUDFlags, f)
#define SET_OR_CLEAR_PUDF(f, fSet) SET_OR_CLEAR_FLAG(gdwPUDFlags, f, fSet)
#define TOGGLE_PUDF(f)             TOGGLE_FLAG(gdwPUDFlags, f)

/*
 * Power state stuff
 */
typedef struct tagPOWERSTATE {
    volatile ULONG           fInProgress:1;
    volatile ULONG           fCritical:1;
    volatile ULONG           fOverrideApps:1;
    volatile ULONG           fQueryAllowed:1;
    volatile ULONG           fUIAllowed:1;
    PKEVENT                  pEvent;
    BROADCASTSYSTEMMSGPARAMS bsmParams;
    POWERSTATEPARAMS         psParams;
} POWERSTATE, *PPOWERSTATE;

#define POWERON_PHASE  -1
#define LOWPOWER_PHASE  1
#define POWEROFF_PHASE  2

NTSTATUS InitializePowerRequestList(HANDLE hPowerRequestEvent);
VOID     CleanupPowerRequestList(VOID);
VOID     DeletePowerRequestList(VOID);
VOID     xxxUserPowerCalloutWorker(VOID);

/*
 * Fade-in / fade-out globals.
 */

typedef struct tagFADE {
    HANDLE hsprite;
    HDC hdc;
    HBITMAP hbm;
    POINT ptDst;
    SIZE size;
    DWORD dwTime;
    DWORD dwStart;
    DWORD dwFlags;
} FADE, *PFADE;

/*
 * Globals are included last because they may require some of the types
 * being defined above.
 */
#include "globals.h"
#include "ddemlsvr.h"
/*
 * If you make a change that requires including strid.h when building
 *  ntuser\rtl, then you need to change the sources/makefil* files so this
 *  file will be built in ntuser\inc; make sure that the output of
 *  mc.exe still goes to the kernel directory; this is because there are
 *  other places (like ntuser\server) where we use the same file name.
 */
#ifndef _USERRTL_
#include "strid.h"
#endif

#include "ntuser.h"

#define TestALPHA(uSetting) (!gbDisableAlpha && TestEffectUP(uSetting))

/*
 * tooltips/tracking prototypes from tooltips.c
 */

typedef struct tagTOOLTIP {
    DWORD dwFlags;
    UINT uTID;
    DWORD dwAnimStart;
    int iyAnim;
    LPWSTR pstr;
} TOOLTIP;

typedef struct tagTOOLTIPWND {
    WND;

    DWORD dwShowDelay;
    DWORD dwHideDelay;
    HDC hdcMem;
    HBITMAP hbmMem;

    TOOLTIP;  // this field must be last!
} TOOLTIPWND, *PTOOLTIPWND;

#define HTEXSCROLLFIRST     60
#define HTSCROLLUP          60
#define HTSCROLLDOWN        61
#define HTSCROLLUPPAGE      62
#define HTSCROLLDOWNPAGE    63
#define HTSCROLLTHUMB       64
#define HTEXSCROLLLAST      64
#define HTEXMENUFIRST       65
#define HTMDISYSMENU        65
#define HTMDIMAXBUTTON      66
#define HTMDIMINBUTTON      67
#define HTMDICLOSE          68
#define HTMENUITEM          69
#define HTEXMENULAST        69

int FindNCHitEx(PWND pwnd, int ht, POINT pt);
void xxxTrackMouseMove(PWND pwnd, int htEx, UINT message);
BOOL xxxHotTrack(PWND pwnd, int htEx, BOOL fDraw);
void xxxResetTooltip(PTOOLTIPWND pttwnd);
void xxxCancelMouseMoveTracking (DWORD dwDTFlags, PWND pwndTrack, int htEx, DWORD dwDTCancel);

__inline PVOID DesktopRebaseToClient(PTHREADINFO pti, PVOID p)
{
    UserAssert(pti->pClientInfo->ulClientDelta != 0);
    return (p) ? (PVOID)((PBYTE)p - pti->pClientInfo->ulClientDelta) : NULL;
}
__inline PVOID SharedRebaseToClient(PPROCESSINFO ppi, PVOID p)
{
    UserAssert(ppi->pClientBase != 0);
    return (p) ? (PVOID)((PBYTE)ppi->pClientBase + ((PBYTE)p -
            (PBYTE)gpvSharedBase)) : NULL;
}
#define SHRSTR(ppi, s) SharedRebaseToClient(ppi, gpsi->s)

/*
 * String range IDs.
 *
 * These are defined here to avoid duplicate entries in strid.mc
 */
#define STR_COLORSTART                   STR_SCROLLBAR
#define STR_COLOREND                     STR_GRADIENTINACTIVECAPTION

/*
 * Sprite and Fade related functions and defines.
 */
#define FADE_SHOW           0x00000001
#define FADE_COMPLETED      0x00000002
#define FADE_SHOWN          0x00000004
#define FADE_WINDOW         0x00000008
#define FADE_MENU           0x00000010
#define FADE_TOOLTIP        0x00000020

HDC CreateFade(PWND pwnd, RECT *prc, DWORD dwTime, DWORD dwFlags);
void StartFade(void);
void StopFade(void);
void ShowFade(void);
void AnimateFade(void);
__inline DWORD TestFadeFlags(DWORD dwFlags)
{
    return (gfade.dwFlags & dwFlags);
}
HANDLE xxxSetLayeredWindow(PWND pwnd, BOOL fRepaintBehind);
BOOL UnsetLayeredWindow(PWND pwnd);
void TrackLayeredZorder(PWND pwnd);
VOID UpdateLayeredSprite(PDCE pdce);
BOOL _UpdateLayeredWindow(PWND pwnd, HDC hdcDst, POINT *pptDst, SIZE *psize, HDC hdcSrc,
        POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags);
BOOL _SetLayeredWindowAttributes(PWND pwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
BOOL RecreateRedirectionBitmap(PWND pwnd);
PWND GetLayeredWindow(PWND pwnd);

#ifdef REDIRECTION
BOOL SetRedirectedWindow(PWND pwnd);
BOOL UnsetRedirectedWindow(PWND pwnd);
__inline BOOL FLayeredOrRedirected(PWND pwnd)
{
    return (TestWF(pwnd, WEFLAYERED) || TestWF(pwnd, WEFREDIRECTED));
}
#else
__inline BOOL FLayeredOrRedirected(PWND pwnd)
{
    return TestWF(pwnd, WEFLAYERED);
}
#endif // REDIRECTION

void InternalInvalidate3(
    PWND pwnd,
    HRGN hrgn,
    DWORD flags);

BOOL UserSetFont(PUNICODE_STRING pProfileUserName,
    LPLOGFONTW   lplf,
    UINT         idFont,
    HFONT*       phfont
    );

HICON DWP_GetIcon(
    PWND pwnd,
    UINT uType);

BOOL xxxRedrawTitle(
    PWND pwnd, UINT wFlags);

DWORD GetContextHelpId(
    PWND pwnd);

BOOL BltIcon(
    HDC hdc, int x, int y, int cx, int cy,
    HDC hdcSrc, PCURSOR pcursor, BOOL fMask, LONG rop);

HANDLE xxxClientCopyImage(
    HANDLE hImage,
    UINT type,
    int cxNew,
    int cyNew,
    UINT flags);

VOID _WOWCleanup(
    HANDLE hInstance,
    DWORD hTaskWow);

VOID _WOWModuleUnload(HANDLE hModule);

/*
 * FastProfile APIs
 */
typedef struct tagPROFINTINFO {
    UINT idSection;
    LPWSTR lpKeyName;
    DWORD  nDefault;
    PUINT puResult;
} PROFINTINFO, *PPROFINTINFO;

#define INITIAL_USER_HANDLE_QUOTA  10000
#define MINIMUM_USER_HANDLE_QUOTA    200

#define INITIAL_POSTMESSAGE_LIMIT   10000
#define MINIMUM_POSTMESSAGE_LIMIT    4000

/*
 * See aFastRegMap[] in ntuser\kernel\profile.c
 */
#define PMAP_COLORS             0
#define PMAP_CURSORS            1
#define PMAP_WINDOWSM           2
#define PMAP_WINDOWSU           3
#define PMAP_DESKTOP            4
#define PMAP_ICONS              5
#define PMAP_FONTS              6
#define PMAP_TRUETYPE           7
#define PMAP_KBDLAYOUT          8
#define PMAP_INPUT              9
#define PMAP_COMPAT            10
#define PMAP_SUBSYSTEMS        11
#define PMAP_BEEP              12
#define PMAP_MOUSE             13
#define PMAP_KEYBOARD          14
#define PMAP_STICKYKEYS        15
#define PMAP_KEYBOARDRESPONSE  16
#define PMAP_MOUSEKEYS         17
#define PMAP_TOGGLEKEYS        18
#define PMAP_TIMEOUT           19
#define PMAP_SOUNDSENTRY       20
#define PMAP_SHOWSOUNDS        21
#define PMAP_AEDEBUG           22
#define PMAP_NETWORK           23
#define PMAP_METRICS           24
#define PMAP_UKBDLAYOUT        25
#define PMAP_UKBDLAYOUTTOGGLE  26
#define PMAP_WINLOGON          27
#define PMAP_KEYBOARDPREF      28
#define PMAP_SCREENREADER      29
#define PMAP_HIGHCONTRAST      30
#define PMAP_IMECOMPAT         31
#define PMAP_IMM               32
#define PMAP_POOLLIMITS        33
#define PMAP_COMPAT32          34
#define PMAP_SETUPPROGRAMNAMES 35
#define PMAP_INPUTMETHOD       36
#define PMAP_COMPAT2           37
#define PMAP_MOUCLASS_PARAMS   38
#define PMAP_KBDCLASS_PARAMS   39
#define PMAP_LAST              39

#define MAXPROFILEBUF 256

#define POLICY_NONE     0x0001
#define POLICY_USER     0x0002
#define POLICY_MACHINE  0x0004
#define POLICY_ALL      (POLICY_NONE | POLICY_USER | POLICY_MACHINE)

PUNICODE_STRING CreateProfileUserName(TL *ptl);
void FreeProfileUserName(PUNICODE_STRING pProfileUserName,TL *ptl);
HANDLE  OpenCacheKeyEx(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, ACCESS_MASK amRequest, PDWORD pdwPolicyFlags);
BOOL    CheckDesktopPolicy(PUNICODE_STRING pProfileUserName OPTIONAL, PCWSTR lpKeyName);
BOOL    CheckDesktopPolicyChange(PUNICODE_STRING pProfileUserName OPTIONAL);
DWORD   FastGetProfileKeysW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR pszDefault, LPWSTR *ppszKeys);
DWORD   FastGetProfileDwordW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName, DWORD dwDefault);
DWORD   FastGetProfileStringW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize);
UINT    FastGetProfileIntW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName, UINT nDefault);
BOOL    FastWriteProfileStringW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName, LPCWSTR lpString);
int     FastGetProfileIntFromID(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, UINT idKey, int def);
DWORD   FastGetProfileStringFromIDW(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, UINT idKey, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD cch);
BOOL    FastWriteProfileValue(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName, UINT uType, LPBYTE lpStruct, UINT cbSizeStruct);
DWORD   FastGetProfileValue(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, LPCWSTR lpKeyName,LPBYTE lpDefault, LPBYTE lpReturn, UINT cbSizeReturn);
BOOL    FastGetProfileIntsW(PUNICODE_STRING pProfileUserName OPTIONAL, PPROFINTINFO ppii);
BOOL    FastUpdateWinIni(PUNICODE_STRING pProfileUserName OPTIONAL, UINT idSection, UINT wKeyNameId, LPWSTR lpszValue);

VOID RecreateSmallIcons(PWND pwnd);

/*
 *  # of pels added to border width.  When a user requests a border width of 1
 *  that user actualy gets a border width of BORDER_EXTRA + 1if the window
 *  has a sizing border.
 */

#define BORDER_EXTRA    3

/*
 * tmswitch.c stuff
 */

__inline int GetCaptionHeight(PWND pwnd)
{
    int height;


    if (!TestWF(pwnd, WFCPRESENT))
        return 0;

    height = TestWF(pwnd, WEFTOOLWINDOW) ? SYSMET(CYSMCAPTION) : SYSMET(CYCAPTION);

    return height;
}

__inline void InitTooltipDelay(PTOOLTIPWND pttwnd)
{
    if (pttwnd != NULL) {
        pttwnd->dwShowDelay = gdtDblClk * 3;
        pttwnd->dwHideDelay = gdtDblClk * 8;
    }
}

__inline PPROFILEVALUEINFO UPDWORDPointer (UINT uSetting)
{
    UserAssert(UPIsDWORDRange(uSetting));
    return gpviCPUserPreferences + UPDWORDIndex(uSetting);
}


/*
 * ComputeTickDelta
 *
 * ComputeTickDelta computes a time delta between two times.  The
 * delta is defined as a 31-bit, signed value.  It is best to think of time as
 * a clock that wraps around.  The delta is the minimum distance on this circle
 * between two different places on the circle.  If the delta goes
 * counter-clockwise, it is looking at a time in the PAST and is POSITIVE.  If
 * the delta goes clockwise, it is looking at a time in the FUTURE and is
 * negative.
 *
 * It is IMPORTANT to realize that the (dwCurTime >= dwLastTime) comparison does
 * not determine the delta's sign, but only determines the operation to compute
 * the delta without an overflow occuring.
 */
__inline
int ComputeTickDelta(
        IN DWORD dwCurTick,
        IN DWORD dwLastTick)
{
    return (int) dwCurTick - dwLastTick;
}


__inline
int ComputePastTickDelta(
        IN DWORD dwCurTick,
        IN DWORD dwLastTick)
{
    int nDelta = ComputeTickDelta(dwCurTick, dwLastTick);
    UserAssertMsg0(nDelta >= 0, "Ensure delta occurs in the past");
    return nDelta;
}


/*
 * SubtractTick() subtracts a delta from a given time and returns another time.
 * This is different than using ComputeTimeDelta() to compare two
 * times and get a delta because of the way wrap around works.
 */
__inline
DWORD SubtractTick(
        IN DWORD dwTime,
        IN int nDelta)
{
    UserAssertMsg0(nDelta >= 0, "Delta must be postive");
    return dwTime - (DWORD) nDelta;
}

__inline BOOL IsTimeFromLastInput (DWORD dwTimeout)
{
    return ((NtGetTickCount() - glinp.timeLastInputMessage) > dwTimeout);
}

__inline BOOL IsTimeFromLastRITEvent (DWORD dwTimeout)
{
    return ((NtGetTickCount() - gpsi->dwLastRITEventTickCount) > dwTimeout);
}

#if DBG
__inline void DBGIncModalMenuCount()
{
    guModalMenuStateCount++;
}

__inline void DBGDecModalMenuCount()
{
    UserAssert(guModalMenuStateCount != 0);
    guModalMenuStateCount--;
}
#else
#define DBGIncModalMenuCount()
#define DBGDecModalMenuCount()
#endif

__inline BOOL IsForegroundLocked()
{
    return ((guSFWLockCount != 0) || (gppiLockSFW != NULL));
}


/* Bug 247768 - joejo
 * Add compatibility hack for foreground activation problems.
 *
 */
__inline
BOOL GiveUpForeground()
{

    if (gptiForeground == NULL) {
        return FALSE;
    }

    if (GetAppCompatFlags2ForPti(gptiForeground , VER40) & GACF2_GIVEUPFOREGROUND){
        TAGMSG0(DBGTAG_FOREGROUND, "GiveUpForeground Hack Succeeded!");
        return TRUE;
    }

    return FALSE;
}

__inline void IncSFWLockCount()
{
    guSFWLockCount++;
}

__inline void DecSFWLockCount()
{
    UserAssert(guSFWLockCount != 0);
    guSFWLockCount--;
}

__inline DWORD UPDWORDValue (UINT uSetting)
{
    return UPDWORDPointer(uSetting)->dwValue;
}
/*
 * Use this macro ONLY if UPIsDWORDRange(SPI_GET ## uSetting) is TRUE.
 */
#define UP(uSetting) UPDWORDValue(SPI_GET ## uSetting)

/*
 * NTIMM.C
 */

#define IMESHOWSTATUS_NOTINITIALIZED    ((BOOL)0xffff)

PIMC CreateInputContext(
    IN ULONG_PTR dwClientImcData);

BOOL DestroyInputContext(
    IN PIMC pImc);

VOID FreeInputContext(
    IN PIMC pImc);

HIMC AssociateInputContext(
    IN PWND pWnd,
    IN PIMC pImc);

AIC_STATUS AssociateInputContextEx(
    IN PWND  pWnd,
    IN PIMC  pImc,
    IN DWORD dwFlag);

BOOL UpdateInputContext(
    IN PIMC pImc,
    IN UPDATEINPUTCONTEXTCLASS UpdateType,
    IN ULONG_PTR UpdateValue);

VOID xxxFocusSetInputContext(
    IN PWND pwnd,
    IN BOOL fActivate,
    IN BOOL fQueueMsg);

UINT BuildHimcList(
    PTHREADINFO pti,
    UINT cHimcMax,
    HIMC *phimcFirst);

PWND xxxCreateDefaultImeWindow(
    IN PWND pwnd,
    IN ATOM atomT,
    IN HANDLE hInst);

BOOL xxxImmActivateThreadsLayout(
    PTHREADINFO pti,
    PTLBLOCK    ptlBlockPrev,
    PKL         pkl);

VOID xxxImmActivateAndUnloadThreadsLayout(
    IN PTHREADINFO *ptiList,
    IN UINT         nEntries,
    IN PTLBLOCK     ptlBlockPrev,
    PKL             pklCurrent,
    DWORD           dwHklReplace);

VOID xxxImmActivateLayout(
    IN PTHREADINFO pti,
    IN PKL pkl);

VOID xxxImmUnloadThreadsLayout(
    IN PTHREADINFO *ptiList,
    IN UINT         nEntry,
    IN PTLBLOCK     ptlBlockPrev,
    IN DWORD        dwFlag);

VOID xxxImmUnloadLayout(
    IN PTHREADINFO pti,
    IN DWORD       dwFlag);

PIMEINFOEX xxxImmLoadLayout(
    IN HKL hKL);

VOID xxxImmActivateLayout(
    IN PTHREADINFO pti,
    IN PKL pkl);

BOOL GetImeInfoEx(
    IN PWINDOWSTATION pwinsta,
    IN PIMEINFOEX piiex,
    IN IMEINFOEXCLASS SearchType);

BOOL SetImeInfoEx(
    IN PWINDOWSTATION pwinsta,
    IN PIMEINFOEX piiex);

DWORD xxxImmProcessKey(
    IN PQ   pq,
    IN PWND pwnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam);

BOOL GetImeHotKey(
    DWORD dwHotKeyID,
    PUINT puModifiers,
    PUINT puVKey,
    HKL   *phKL );

BOOL  SetImeHotKey(
    DWORD  dwHotKeyID,
    UINT   uModifiers,
    UINT   uVKey,
    HKL    hKL,
    DWORD  dwAction );

PIMEHOTKEYOBJ CheckImeHotKey(
    PQ   pq,
    UINT uVKey,
    LPARAM lParam);

BOOL ImeCanDestroyDefIME(
    IN PWND pwndDefaultIme,
    IN PWND pwndDestroy);

BOOL IsChildSameThread(
    IN PWND pwndParent,
    IN PWND pwndChild);

BOOL ImeCanDestroyDefIMEforChild(
    IN PWND pwndDefaultIme,
    IN PWND pwndDestroy);

VOID ImeCheckTopmost(
    IN PWND pwnd);

VOID ImeSetFutureOwner(
    IN PWND pwndDefaultIme,
    IN PWND pwndOrgOwner);

VOID ImeSetTopmostChild(
    IN PWND pwndRoot,
    IN BOOL fFlag);

VOID ImeSetTopmost(
    IN PWND pwndRoot,
    IN BOOL fFlag,
    IN PWND pwndInsertBefore);

PSOFTKBDDATA ProbeAndCaptureSoftKbdData(
    PSOFTKBDDATA Source);

VOID xxxNotifyIMEStatus(
    IN PWND pwnd,
    IN DWORD dwOpen,
    IN DWORD dwConversion );

BOOL xxxSetIMEShowStatus(
    IN BOOL fShow);

VOID xxxBroadcastImeShowStatusChange(
    IN PWND pwndDefIme,
    IN BOOL fShow);

VOID xxxCheckImeShowStatusInThread(
    IN PWND pwndDefIme);


#define IsWndImeRelated(pwnd)   \
    (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME] || \
     TestCF(pwnd, CFIME))

/*
 * Critical section routines for processing mouse input
 */
__inline VOID EnterMouseCrit() {

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(gpresMouseEventQueue, TRUE);
}

__inline VOID LeaveMouseCrit() {

    ExReleaseResource(gpresMouseEventQueue);
    KeLeaveCriticalRegion();
}

#if DBG
#define EnterDeviceInfoListCrit _EnterDeviceInfoListCrit
#define LeaveDeviceInfoListCrit _LeaveDeviceInfoListCrit
VOID _EnterDeviceInfoListCrit();
VOID _LeaveDeviceInfoListCrit();
#else
/*
 * Critical section routines for accessing the Device List (gpDeviceInfoList)
 */
__inline VOID EnterDeviceInfoListCrit() {
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(gpresDeviceInfoList, TRUE);
}
__inline VOID LeaveDeviceInfoListCrit() {
    ExReleaseResource(gpresDeviceInfoList);
    KeLeaveCriticalRegion();
}
#endif

/*
 * Keep some capture state visible from user-mode for performance.
 */
__inline VOID LockCaptureWindow(PQ pq, PWND pwnd) {
    if (pq->spwndCapture) {
        UserAssert(gpsi->cCaptures > 0);
        gpsi->cCaptures--;
    }
    if (pwnd) {
        gpsi->cCaptures++;
    }
    Lock(&pq->spwndCapture, pwnd);
}

__inline VOID UnlockCaptureWindow(PQ pq) {
    if (pq->spwndCapture) {
        UserAssert(gpsi->cCaptures > 0);
        gpsi->cCaptures--;
        Unlock(&pq->spwndCapture);
    }
}

/*
 * Some routines for manipulating desktop and windowstation handles.
 */
#define HF_DESKTOPHOOK  0       // offset to desktop hook flag
#define HF_PROTECTED    1       // offset to protected flag
#define HF_LIMIT        2       // number of flags per handle

BOOL SetHandleFlag(HANDLE, DWORD, BOOL);
BOOL CheckHandleFlag(HANDLE, DWORD);
VOID SetHandleInUse(HANDLE);
BOOL CheckHandleInUse(HANDLE);

__inline NTSTATUS CloseProtectedHandle(HANDLE handle)
{
    if (handle != NULL) {
        SetHandleFlag(handle, HF_PROTECTED, FALSE);
        return ZwClose(handle);
    }
    return STATUS_SUCCESS;
}

__inline VOID EnterHandleFlagsCrit() {
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gpHandleFlagsMutex);
}

__inline VOID LeaveHandleFlagsCrit() {
    ExReleaseFastMutexUnsafe(gpHandleFlagsMutex);
    KeLeaveCriticalRegion();
}

// multimon apis

#define HMONITOR_PRIMARY ((HMONITOR)0x00010000)

BOOL     xxxEnumDisplayMonitors(
                HDC             hdc,
                LPRECT          lprcClip,
                MONITORENUMPROC lpfnEnum,
                LPARAM          dwData,
                BOOL            fInternal);

BOOL    xxxClientMonitorEnumProc(
                HMONITOR        hMonitor,
                HDC             hdcMonitor,
                LPRECT          lprc,
                LPARAM          dwData,
                MONITORENUMPROC xpfnProc);

void    ClipPointToDesktop(LPPOINT lppt);
void    DestroyMonitor(PMONITOR pMonitor);
BOOL    GetHDevName(HMONITOR hMon, PWCHAR pName);
ULONG   HdevFromMonitor(PMONITOR pMonitor);

/*
 * Rebasing functions for shared memory.
 */
#define REBASESHAREDPTR(p)       (p)
#define REBASESHAREDPTRALWAYS(p) (p)

/*
 * Multimonitor macros used in RTL. There are similar definitions
 * in client\userdll.h
 */
__inline PDISPLAYINFO
GetDispInfo(void)
{

    return gpDispInfo;
}

__inline PMONITOR
GetPrimaryMonitor(void)
{
    return REBASESHAREDPTRALWAYS(GetDispInfo()->pMonitorPrimary);
}

VOID _QueryUserHandles(
        IN  LPDWORD     lpIn,
        IN  DWORD       dwInLength,
        OUT DWORD       pdwResult[][TYPE_CTYPES]);



#define REMOVE_FROM_LIST(type, pstart, pitem, next) \
    {                                                           \
        type** pp;                                              \
                                                                \
        for (pp = &pstart; *pp != NULL; pp = &(*pp)->next) {    \
            if (*pp == pitem) {                                 \
                *pp = pitem->next;                              \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \


#define HH_DRIVERENTRY              0x00000001
#define HH_USERINITIALIZE           0x00000002
#define HH_INITVIDEO                0x00000004
#define HH_REMOTECONNECT            0x00000008
#define HH_REMOTEDISCONNECT         0x00000010
#define HH_REMOTERECONNECT          0x00000020
#define HH_REMOTELOGOFF             0x00000040
#define HH_DRIVERUNLOAD             0x00000080
#define HH_GRECLEANUP               0x00000100
#define HH_USERKCLEANUP             0x00000200
#define HH_INITIATEWIN32KCLEANUP    0x00000400
#define HH_ALLDTGONE                0x00000800
#define HH_RITGONE                  0x00001000
#define HH_RITCREATED               0x00002000
#define HH_LOADCURSORS              0x00004000
#define HH_KBDLYOUTGLOBALCLEANUP    0x00008000
#define HH_KBDLYOUTFREEWINSTA       0x00010000
#define HH_CLEANUPRESOURCES         0x00020000
#define HH_DISCONNECTDESKTOP        0x00040000
#define HH_DTQUITPOSTED             0x00080000

#define HYDRA_HINT(ev)  (gdwHydraHint |= ev)

#if DBG
    VOID TrackAddDesktop(PVOID pDesktop);
    VOID TrackRemoveDesktop(PVOID pDesktop);
    VOID DumpTrackedDesktops(BOOL bBreak);

    #define DbgTrackAddDesktop(pdesk) TrackAddDesktop(pdesk)
    #define DbgTrackRemoveDesktop(pdesk) TrackRemoveDesktop(pdesk)
    #define DbgDumpTrackedDesktops(b) DumpTrackedDesktops(b)
#else
    #define DbgTrackAddDesktop(pdesk)
    #define DbgTrackRemoveDesktop(pdesk)
    #define DbgDumpTrackedDesktops(b)
#endif

#if DBG
    #define TRACE_HYDAPI(m)                                     \
        if (gbTraceHydraApi) {                                  \
            KdPrint(("HYD-%d API: ", gSessionId));              \
            KdPrint(m);                                         \
        }
#else
    #define TRACE_HYDAPI(m)
#endif

#if DBG
    #define TRACE_DESKTOP(m)                                    \
        if (gbTraceDesktop) {                                   \
            KdPrint(("HYD-%d DT ", gSessionId));                \
            KdPrint(m);                                         \
        }

    #define TRACE_RIT(m)                                        \
        if (gbTraceRIT) {                                       \
            KdPrint(("HYD-%d RIT ", gSessionId));               \
            KdPrint(m);                                         \
        }
#else
    #define TRACE_DESKTOP(m)
    #define TRACE_RIT(m)
#endif

NTSTATUS
RemoteConnect(
    IN PDOCONNECTDATA pDoConnectData,
    IN ULONG DisplayDriverNameLength,
    IN PWCHAR DisplayDriverName);

NTSTATUS
xxxRemoteDisconnect(
    VOID);

NTSTATUS
xxxRemoteReconnect(
    IN PDORECONNECTDATA pDoReconnectData);

NTSTATUS
RemoteLogoff(
    VOID);

BOOL
PrepareForLogoff(
    UINT uFlags);

NTSTATUS
xxxRemoteStopScreenUpdates(
    BOOL fDisableGraphics);

VOID xxxPushKeyEvent(
    BYTE  bVk,
    BYTE  bScan,
    DWORD dwFlags,
    DWORD dwExtraInfo);

NTSTATUS
RemoteThinwireStats(
    OUT PVOID Stats);

NTSTATUS
RemoteNtSecurity(
    VOID);

NTSTATUS
xxxRemoteShadowSetup(
    VOID);

NTSTATUS
RemoteShadowStart(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength);

NTSTATUS
xxxRemoteShadowStop(
    VOID);

NTSTATUS
RemoteShadowCleanup(
    IN PVOID pThinwireData,
    ULONG ThinwireDataLength);

NTSTATUS
xxxRemotePassthruEnable(
    VOID);

NTSTATUS
RemotePassthruDisable(
    VOID);

NTSTATUS
CtxDisplayIOCtl(
    ULONG  DisplayIOCtlFlags,
    PUCHAR pDisplayIOCtlData,
    ULONG  cbDisplayIOCtlData);

DWORD
RemoteConnectState(
    VOID);

BOOL
_GetWinStationInfo(
    WSINFO* pWsInfo);

// from fullscr.c

NTSTATUS
RemoteRedrawRectangle(
    WORD Left,
    WORD Top,
    WORD Right,
    WORD Bottom);

NTSTATUS
RemoteRedrawScreen(
    VOID);

NTSTATUS
RemoteDisableScreen(
    VOID);

// from muclean.c

// from fekbd.c
VOID
NlsKbdSendIMEProc(
    DWORD dwImeOpen,
    DWORD dwImeConversion);

#endif  // !_USERK_
