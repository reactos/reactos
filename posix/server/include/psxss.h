/* $Id: psxss.h,v 1.1 2002/04/10 21:30:21 ea Exp $ */
#ifndef _PSX_PSXSS_H
#define _PSX_PSXSS_H

#define NTOS_MODE_USER
#include <ntos.h>
#include <ntdll/rtl.h>
#include <psx/lpcproto.h>

#ifdef __PSXSS_ON_W32__
#include <windows.h>
#endif

#define NAME_BUFFER_SIZE 64

/* PSXSS GLOBAL DATA */

typedef struct _SERVER_DIRECTORY
{
    HANDLE         hObject;
    LPWSTR         wsName;
    UNICODE_STRING usName;

} SERVER_DIRECTORY;

#define PSXSS_THREADS_PER_PORT 2

typedef struct _SERVER_PORT
{
    HANDLE                 hObject;
    LPWSTR                 wsName;
    UNICODE_STRING         usName;
    PTHREAD_START_ROUTINE  EntryPoint;
    struct {
        HANDLE hObject;
        DWORD  Id;
    } ThreadInfo [PSXSS_THREADS_PER_PORT];

} SERVER_PORT, * PSERVER_PORT;

#define SERVER_DIRECTORY_COUNT 3
#define SERVER_PORT_COUNT      3

typedef struct _SERVER
{
    HANDLE           Heap;
    SERVER_DIRECTORY Directory [SERVER_DIRECTORY_COUNT];
    SERVER_PORT      Port [SERVER_PORT_COUNT];

} SERVER;

#define PSX_PORT_API           0
#define PSX_PORT_SBAPI         1
#define PSX_PORT_SESSION       2

#define PSX_DIRECTORY_POSIX    0
#define PSX_DIRECTORY_SESSIONS 1
#define PSX_DIRECTORY_SYSTEM   2

extern SERVER Server; /* server/misc/init.c */

/* System call type */
typedef NTSTATUS (STDCALL *PSX_SYSTEM_CALL)(PPSX_MAX_MESSAGE);

/* System calls table */

extern PSX_SYSTEM_CALL SystemCall []; /* server/call/syscall.c */

/* Listener's Threads */
VOID STDCALL ApiPortListener (PVOID);
VOID STDCALL SbApiPortListener (PVOID);
VOID STDCALL SessionPortListener (PVOID);

/* TERMINAL OBJECT */

typedef struct _PSX_TERMINAL
{
    DWORD Id;
    /* TODO */
} PSX_TERMINAL, * PPSX_TERMINAL;

/* PROCESS OBJECT */

typedef enum 
{
    PROCESS_STATUS_INITIALIZATION,
    PROCESS_STATUS_READY,
    PROCESS_STATUS_WAITING,
    PROCESS_STATUS_DEAD,
    PROCESS_STATUS_ZOMBIE

} PROCESS_STATUS;

typedef DWORD PSX_SECURITY; /* TODO */

struct _PSX_SESSION;

typedef struct _PSX_PROCESS
{
    INT                 Id;         /* pid */
    HANDLE              hProcess;   /* hProcess */
    PROCESS_STATUS      Status;
    struct _PSX_PROCESS    * Parent;
    struct _PSX_SESSION * Session;
    PPSX_TERMINAL       ControlTty;
    PSX_SECURITY        Security;

} PSX_PROCESS, * PPSX_PROCESS;

/* SESSION OBJECT */

typedef enum 
{
    SESSION_STATUS_INITIALIZATION,
    SESSION_STATUS_READY,
    SESSION_STATUS_SHUTDOWN,
    SESSION_STATUS_ERROR

} SESSION_STATUS;

typedef struct _PSX_SESSION
{
    DWORD           Id;
    ULONG           ReferenceCount;
    SESSION_STATUS  Status;
    HANDLE          Heap;
    struct {
        HANDLE  hPort;
        ULONG   ulPortIdentifier; 
    } SessionChannel;
    struct {
        ULONG   Id;   /* csrterm pid */
        HANDLE  hPort; /* LPC port owned by csrterm: \POSIX+\Sessions\P{Id} */
        struct {
            HANDLE  Handle; /* Section owned by csrterm: \POSIX+\Sessions\D{Id} */
            PVOID   BaseAddress;
            ULONG   ViewSize;
        } Section;
    } TerminalChannel;
    PPSX_PROCESS        Leader;
    CRITICAL_SECTION    Lock;
    struct _PSX_SESSION * Previous;
    struct _PSX_SESSION * Next;

} PSX_SESSION, * PPSX_SESSION;

/* prototypes */

VOID STDCALL debug_print (LPWSTR Template, ...);
NTSTATUS STDCALL PsxInitializeSessions (VOID); /* ob/session.c */
NTSTATUS STDCALL PsxCreateSession (PLPC_MAX_MESSAGE,HANDLE,ULONG);
NTSTATUS STDCALL PsxInitializeProcesses (VOID); /* ob/process.c */
NTSTATUS STDCALL PsxCreateProcess (PLPC_MAX_MESSAGE,HANDLE,ULONG);

#endif /* ndef _PSX_PSXSS_H */
