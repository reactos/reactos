#ifndef _CSRTERM_H
#define _CSRTERM_H

/* PSX session: CSR terminal emulator side */

#define NAME_BUFFER_SIZE 64

typedef struct _PSXSS_PORT
{
    UNICODE_STRING    Name;
    WCHAR             NameBuffer [NAME_BUFFER_SIZE];
    HANDLE            Handle;

} PSXSS_PORT, * PPSXSS_PORT;

typedef struct _CSRTERM_SESSION_PORT
{
    UNICODE_STRING  Name;
    WCHAR           NameBuffer [NAME_BUFFER_SIZE];
    HANDLE          Handle;
    struct {
        HANDLE  Handle;
        DWORD   Id;
    } Thread;

} CSRTERM_SESSION_PORT;

typedef struct _CSRTERM_SESSION_SECTION
{
    UNICODE_STRING  Name;
    WCHAR           NameBuffer [NAME_BUFFER_SIZE];
    HANDLE          Handle;
    ULONG           Size;
    PVOID           BaseAddress;
    ULONG           ViewSize;

} CSRTERM_SESSION_SECTION;

typedef struct _CSRTERM_SESSION
{
    ULONG                    Identifier; /* PortID for ServerPort in PSXSS */
    PSXSS_PORT               ServerPort; /* \POSIX+\SessionPort */
    CSRTERM_SESSION_PORT     Port;       /* \POSIX+\Sessions\P<pid> */
    CSRTERM_SESSION_SECTION  Section;    /* \POSIX+\Sessions\D<pid> */
    CLIENT_ID                Client;
    CRITICAL_SECTION         Lock;
    BOOL                     SsLinkIsActive;

} CSRTERM_SESSION, * PCSRTERM_SESSION;

#define   LOCK_SESSION RtlEnterCriticalSection(& Session.Lock)
#define UNLOCK_SESSION RtlLeaveCriticalSection(& Session.Lock)
#endif /* ndef _CSRTERM_H */
