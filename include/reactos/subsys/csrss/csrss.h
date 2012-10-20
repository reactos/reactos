/*****************************  CSRSS Data  ***********************************/

#ifndef __INCLUDE_CSRSS_CSRSS_H
#define __INCLUDE_CSRSS_CSRSS_H

#define CSR_NATIVE     0x0000   // CSRSRV
#define CSR_CONSOLE    0x0001   // WIN32CSR
#define CSR_GUI        0x0002   // WINSRV
#define CONSOLE_INPUT_MODE_VALID  (0x0f)
#define CONSOLE_OUTPUT_MODE_VALID (0x03)


#define CSR_CSRSS_SECTION_SIZE          (65536)

typedef VOID (CALLBACK *PCONTROLDISPATCHER)(DWORD);

typedef struct
{
    USHORT nMaxIds;
    PDWORD ProcessId;
    ULONG nProcessIdsTotal;
} CSRSS_GET_PROCESS_LIST, *PCSRSS_GET_PROCESS_LIST;

typedef struct
{
    HANDLE  UniqueThread;
    CLIENT_ID Cid;
} CSRSS_IDENTIFY_ALERTABLE_THREAD, *PCSRSS_IDENTIFY_ALERTABLE_THREAD;

typedef struct
{
    HANDLE ProcessId;
} CSRSS_REGISTER_SERVICES_PROCESS, *PCSRSS_REGISTER_SERVICES_PROCESS;

typedef struct
{
    UINT Flags;
    DWORD Reserved;
} CSRSS_EXIT_REACTOS, *PCSRSS_EXIT_REACTOS;

typedef struct
{
    HANDLE Handle;
} CSRSS_CLOSE_HANDLE, *PCSRSS_CLOSE_HANDLE;

typedef struct
{
    HANDLE Handle;
} CSRSS_VERIFY_HANDLE, *PCSRSS_VERIFY_HANDLE;

typedef struct
{
    HANDLE Handle;
    DWORD Access;
    BOOL Inheritable;
    DWORD Options;
} CSRSS_DUPLICATE_HANDLE, *PCSRSS_DUPLICATE_HANDLE;

typedef struct
{
    HDESK DesktopHandle;
} CSRSS_CREATE_DESKTOP, *PCSRSS_CREATE_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
    ULONG Width;
    ULONG Height;
} CSRSS_SHOW_DESKTOP, *PCSRSS_SHOW_DESKTOP;

typedef struct
{
    HWND DesktopWindow;
} CSRSS_HIDE_DESKTOP, *PCSRSS_HIDE_DESKTOP;

typedef struct
{
    HWND LogonNotifyWindow;
} CSRSS_SET_LOGON_NOTIFY_WINDOW, *PCSRSS_SET_LOGON_NOTIFY_WINDOW;

typedef struct
{
    HANDLE ProcessId;
    BOOL Register;
} CSRSS_REGISTER_LOGON_PROCESS, *PCSRSS_REGISTER_LOGON_PROCESS;

typedef struct
{
    HANDLE InputWaitHandle;
} CSRSS_GET_INPUT_WAIT_HANDLE, *PCSRSS_GET_INPUT_WAIT_HANDLE;

#define CSR_API_MESSAGE_HEADER_SIZE(Type)       (FIELD_OFFSET(CSR_API_MESSAGE, Data) + sizeof(Type))

#define REGISTER_SERVICES_PROCESS       (0x1D)
#define EXIT_REACTOS                    (0x1E)
#define CLOSE_HANDLE                    (0x26)
#define VERIFY_HANDLE                   (0x27)
#define DUPLICATE_HANDLE                (0x28)
#define CREATE_DESKTOP                  (0x2B)
#define SHOW_DESKTOP                    (0x2C)
#define HIDE_DESKTOP                    (0x2D)
#define SET_LOGON_NOTIFY_WINDOW         (0x2F)
#define REGISTER_LOGON_PROCESS          (0x30)
#define GET_INPUT_WAIT_HANDLE           (0x35)
#define GET_PROCESS_LIST                (0x36)
#define START_SCREEN_SAVER              (0x37)


#endif /* __INCLUDE_CSRSS_CSRSS_H */
