/**************************************************************************\
* Module Name: ntsend.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* client-side macros for kernel-mode
*
* 03-21-95 JimA             Created.
\**************************************************************************/

/*
 * The BEGINCALLCONNECT macro ensures that the thread is set up correctly.
 */
#define BEGINCALLCONNECT()                              \
    {                                                   \
    ULONG_PTR retval;                                    \
    {                                                   \
        if (NtCurrentTeb()->Win32ThreadInfo == NULL) {  \
            if (!NtUserGetThreadState(UserThreadConnect)) { \
                MSGERROR();                             \
            }                                           \
        }

/*
 * Use this macro if you don't need to access shared memory.
 */
#define BEGINCALL()       \
    {                     \
    ULONG_PTR retval;      \
    {

#define BEGINCALLVOID()   \
    {

#define ERRORTRAP(error) \
       goto cleanup;        \
    }                       \
    goto errorexit;         \
errorexit:                  \
    retval = (ULONG_PTR)error; \
cleanup:

#define ERRORTRAPVOID()     \
    goto errorexit;         \
errorexit:

#define ENDCALL(type)     \
    return (type)retval;  \
    }

#define ENDCALLVOID() \
    return;           \
    }

#define MSGERROR() goto errorexit
#define MSGERRORCODE(code) { \
    RIPERR0(code, RIP_WARNING, "Unspecified error"); \
    goto errorexit; }

#define MSGNTERRORCODE(code) { \
    RIPNTERR0(code, RIP_WARNING, "Unspecified error"); \
    goto errorexit; }

#define MESSAGECALL(api) \
LRESULT api(             \
    HWND hwnd,           \
    UINT msg,            \
    WPARAM wParam,       \
    LPARAM lParam,       \
    ULONG_PTR xParam,     \
    DWORD xpfnProc,      \
    BOOL bAnsi)

/*
 * Copy optional string/Ordinal where if hiword is FF/FFFF then new WORD is a
 * resource oridinal ID
 * Sources is Unicode
 */
#define OrdinalLPSTR(src)   (MAKELONG(0xFFFF,((*(DWORD UNALIGNED *)src) >> 8)))
#define OrdinalLPSTRW(src)  (MAKELONG(0xFFFF,((*(DWORD UNALIGNED *)src) >> 8)))
#define OrdinalLPWSTR(src)  (*(DWORD UNALIGNED *)src)
#define OrdinalLPWSTRA(src) (*(DWORD UNALIGNED *)((PBYTE)src + 1))

/*
 * Ansi->Unicode macros
 */
#define COPYLPSTRW(pinstr, psz) \
    if (!RtlCaptureAnsiString((pinstr), (LPCSTR)(psz), TRUE))     \
        MSGERROR();

#define COPYLPSTRIDW(pinstr, psz) \
    if (IS_PTR(psz)) {                                      \
        if (!RtlCaptureAnsiString((pinstr), (LPCSTR)(psz), TRUE))   \
            MSGERROR();                                     \
    }                                                       \
    else {                                                  \
        (pinstr)->fAllocated = FALSE;                       \
        (pinstr)->pstr = &(pinstr)->strCapture;             \
        (pinstr)->strCapture.Length =                       \
                (pinstr)->strCapture.MaximumLength = 0;     \
        (pinstr)->strCapture.Buffer = (LPWSTR)(psz);        \
    }

#define COPYLPSTRIDOPTW     COPYLPSTRIDW
#define COPYLPSTROPTW       COPYLPSTRW

#define LARGECOPYLPSTRW(pinstr, psz) \
    if(!RtlCaptureLargeAnsiString((pinstr), (LPCSTR)(psz), TRUE)) \
        MSGERROR();

#define LARGECOPYLPSTROPTW  LARGECOPYLPSTRW

#define LARGECOPYLPSTRORDINALOPTW(pinstr, psz) \
    (pinstr)->pstr = &(pinstr)->strCapture;                                         \
    (pinstr)->fAllocated = FALSE;                                                   \
    if (psz) {                                                                      \
        if (*(LPBYTE)(psz) != 0xff) {                                               \
            if (!RtlCaptureLargeAnsiString((pinstr), (LPCSTR)(psz), TRUE))          \
                MSGERROR();                                                         \
        } else {                                                                    \
            (pinstr)->strCapture.Length =                                           \
                    (pinstr)->strCapture.MaximumLength = sizeof(DWORD);             \
            dwOrdinal = OrdinalLPSTRW(psz);                                         \
            (pinstr)->strCapture.Buffer = (LPWSTR)&dwOrdinal;                       \
        }                                                                           \
    } else {                                                                        \
        (pinstr)->strCapture.Length =                                               \
                (pinstr)->strCapture.MaximumLength = 0;                             \
        (pinstr)->strCapture.Buffer = NULL;                                         \
    }

#define FIRSTCOPYLPSTRW(pinstr, psz) \
    if (!RtlCaptureAnsiString((pinstr), (LPCSTR)(psz), FALSE))    \
        MSGERROR();

#define FIRSTCOPYLPSTRIDW(pinstr, psz) \
    if (IS_PTR(psz)) {                                      \
        if (!RtlCaptureAnsiString((pinstr), (LPCSTR)(psz), FALSE))  \
            MSGERROR();                                     \
    } else {                                                \
        (pinstr)->fAllocated = FALSE;                       \
        (pinstr)->pstr = &(pinstr)->strCapture;             \
        (pinstr)->strCapture.Length =                       \
                (pinstr)->strCapture.MaximumLength = 0;     \
        (pinstr)->strCapture.Buffer = (LPWSTR)(psz);        \
    }

#define FIRSTCOPYLPSTRIDOPTW     FIRSTCOPYLPSTRIDW
#define FIRSTCOPYLPSTROPTW       FIRSTCOPYLPSTRW

#define FIRSTLARGECOPYLPSTRW(pinstr, psz) \
    if (!RtlCaptureLargeAnsiString((pinstr), (LPCSTR)(psz), FALSE))   \
        MSGERROR();

#define FIRSTLARGECOPYLPSTROPTW  FIRSTLARGECOPYLPSTRW

#define FIRSTLARGECOPYLPSTRORDINALOPTW(pinstr, psz) \
    (pinstr)->pstr = &(pinstr)->strCapture;                                             \
    (pinstr)->fAllocated = FALSE;                                                       \
    if (psz) {                                                                          \
        if (*(LPBYTE)(psz) != 0xff) {                                                   \
            if (!RtlCaptureLargeAnsiString((pinstr), (LPCSTR)(psz), FALSE))             \
                MSGERROR();                                                             \
        } else {                                                                        \
            (pinstr)->strCapture.Length =                                               \
                    (pinstr)->strCapture.MaximumLength = sizeof(DWORD);                 \
            dwOrdinal = OrdinalLPSTRW(psz);                                             \
            (pinstr)->strCapture.Buffer = (LPWSTR)&dwOrdinal;                           \
        }                                                                               \
    } else {                                                                            \
        (pinstr)->strCapture.Length =                                                   \
                (pinstr)->strCapture.MaximumLength = 0;                                 \
        (pinstr)->strCapture.Buffer = NULL;                                             \
    }

#define CLEANUPLPSTRW(instr) \
    if (instr.fAllocated)                     \
        RtlFreeHeap(pUserHeap, 0, instr.strCapture.Buffer);

/*
 * Unicode->Unicode macros
 */
#define COPYLPWSTR(pinstr, psz) \
    (pinstr)->fAllocated = FALSE;                           \
    (pinstr)->pstr = &(pinstr)->strCapture;                 \
    RtlInitUnicodeString(&(pinstr)->strCapture, (psz));

#define COPYLPWSTRID(pinstr, psz) \
    (pinstr)->fAllocated = FALSE;                           \
    (pinstr)->pstr = &(pinstr)->strCapture;                 \
    if (IS_PTR(psz))                                        \
        RtlInitUnicodeString(&(pinstr)->strCapture, (psz)); \
    else {                                                  \
        (pinstr)->strCapture.Length =                       \
                (pinstr)->strCapture.MaximumLength = 0;     \
        (pinstr)->strCapture.Buffer = (LPWSTR)(psz);        \
    }

#define COPYLPWSTRIDOPT     COPYLPWSTRID
#define COPYLPWSTROPT       COPYLPWSTR

#define LARGECOPYLPWSTR(pinstr, psz) \
    (pinstr)->fAllocated = FALSE;                           \
    (pinstr)->pstr = &(pinstr)->strCapture;                         \
    RtlInitLargeUnicodeString(&(pinstr)->strCapture, (psz), (UINT)-1);

#define LARGECOPYLPWSTROPT  LARGECOPYLPWSTR

#define LARGECOPYLPWSTRORDINALOPT(pinstr, psz) \
    (pinstr)->fAllocated = FALSE;                           \
    (pinstr)->pstr = &(pinstr)->strCapture;                                     \
    if (psz) {                                                                  \
        if (*(LPWORD)(psz) != 0xffff)                                           \
            RtlInitLargeUnicodeString(&(pinstr)->strCapture, (psz), (UINT)-1);  \
        else {                                                                  \
            (pinstr)->strCapture.Length =                                       \
                    (pinstr)->strCapture.MaximumLength = sizeof(DWORD);         \
            dwOrdinal = OrdinalLPWSTR(psz);                                     \
            (pinstr)->strCapture.Buffer = (LPWSTR)&dwOrdinal;                   \
        }                                                                       \
    } else {                                                                    \
        (pinstr)->strCapture.Length =                                           \
                (pinstr)->strCapture.MaximumLength = 0;                         \
        (pinstr)->strCapture.Buffer = NULL;                                     \
    }

#define FIRSTCOPYLPWSTR                 COPYLPWSTR
#define FIRSTCOPYLPWSTRID               COPYLPWSTRID
#define FIRSTCOPYLPWSTRIDOPT            COPYLPWSTRIDOPT
#define FIRSTCOPYLPWSTROPT              COPYLPWSTROPT
#define FIRSTLARGECOPYLPWSTR            LARGECOPYLPWSTR
#define FIRSTLARGECOPYLPWSTROPT         LARGECOPYLPWSTROPT
#define FIRSTLARGECOPYLPWSTRORDINALOPT  LARGECOPYLPWSTRORDINALOPT

#define CLEANUPLPWSTR(instr)

/*
 * Type-neutral macros
 */
#ifdef UNICODE

#define COPYLPTSTR                  COPYLPWSTR
#define COPYLPTSTRID                COPYLPWSTRID
#define COPYLPTSTRIDOPT             COPYLPWSTRIDOPT
#define COPYLPTSTROPT               COPYLPWSTROPT
#define FIRSTCOPYLPTSTR             COPYLPWSTR
#define FIRSTCOPYLPTSTRID           COPYLPWSTRID
#define FIRSTCOPYLPTSTRIDOPT        COPYLPWSTRIDOPT
#define LARGECOPYLPTSTR             LARGECOPYLPWSTR
#define LARGECOPYLPTSTROPT          LARGECOPYLPWSTROPT
#define FIRSTLARGECOPYLPTSTROPT     LARGECOPYLPWSTROPT
#define CLEANUPLPTSTR               CLEANUPLPWSTR

#else

#define COPYLPTSTR                  COPYLPSTRW
#define COPYLPTSTRID                COPYLPSTRIDW
#define COPYLPTSTRIDOPT             COPYLPSTRIDOPTW
#define COPYLPTSTROPT               COPYLPSTROPTW
#define FIRSTCOPYLPTSTR             COPYLPSTRW
#define FIRSTCOPYLPTSTRID           COPYLPSTRIDW
#define FIRSTCOPYLPTSTRIDOPT        COPYLPSTRIDOPTW
#define LARGECOPYLPTSTR             LARGECOPYLPSTRW
#define LARGECOPYLPTSTROPT          LARGECOPYLPSTROPTW
#define FIRSTLARGECOPYLPTSTROPT     LARGECOPYLPSTROPTW
#define CLEANUPLPTSTR               CLEANUPLPSTRW

#endif
