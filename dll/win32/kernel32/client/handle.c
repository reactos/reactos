/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/handle.c
 * PURPOSE:         Object Handle Functions
 * PROGRAMMERS:     Ariadne ( ariadne@xs4all.nl)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

#include <winsock2.h> // for winsock handle duplication - dependent on PR-5482

/* PRIVATE FUNCTIONS **********************************************************/

// private items for winsock socket handle duplication
#define SystemHandleInformation 0x10
#define SystemHandleInformationSize 3427780 * 2 // VALUE FROM WIN10 headers doubled, seems to rise every call in ROS

ULONG_PTR hGetParentProcessId() // derived form Napalm @ NetCore2K from https://stackoverflow.com/questions/185254/how-can-a-win32-process-get-the-pid-of-its-parent
{
	ULONG_PTR pbi[6];
	ULONG ulSize = 0;
	if(NtQueryInformationProcess(GetCurrentProcess(), 0,
				&pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
		return pbi[5];

    return (ULONG_PTR)-1;
}
// end of private functions for winsock handle duplication

HANDLE
TranslateStdHandle(IN HANDLE hHandle)
{
    PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;

    switch ((ULONG_PTR)hHandle)
    {
        case STD_INPUT_HANDLE:  return Ppb->StandardInput;
        case STD_OUTPUT_HANDLE: return Ppb->StandardOutput;
        case STD_ERROR_HANDLE:  return Ppb->StandardError;
    }

    return hHandle;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetHandleInformation(IN HANDLE hObject,
                     OUT LPDWORD lpdwFlags)
{
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
    ULONG BytesWritten;
    NTSTATUS Status;
    DWORD Flags;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject))
    {
        return GetConsoleHandleInformation(hObject, lpdwFlags);
    }

    Status = NtQueryObject(hObject,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
                           &BytesWritten);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    Flags = 0;
    if (HandleInfo.Inherit) Flags |= HANDLE_FLAG_INHERIT;
    if (HandleInfo.ProtectFromClose) Flags |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
    *lpdwFlags = Flags;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetHandleInformation(IN HANDLE hObject,
                     IN DWORD dwMask,
                     IN DWORD dwFlags)
{
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleInfo;
    ULONG BytesWritten;
    NTSTATUS Status;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject))
    {
        return SetConsoleHandleInformation(hObject, dwMask, dwFlags);
    }

    Status = NtQueryObject(hObject,
                           ObjectHandleFlagInformation,
                           &HandleInfo,
                           sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
                           &BytesWritten);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (dwMask & HANDLE_FLAG_INHERIT)
    {
        HandleInfo.Inherit = (dwFlags & HANDLE_FLAG_INHERIT) != 0;
    }

    if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE)
    {
        HandleInfo.ProtectFromClose = (dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) != 0;
    }

    Status = NtSetInformationObject(hObject,
                                    ObjectHandleFlagInformation,
                                    &HandleInfo,
                                    sizeof(HandleInfo));
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CloseHandle(IN HANDLE hObject)
{
    NTSTATUS Status;

    hObject = TranslateStdHandle(hObject);

    if (IsConsoleHandle(hObject)) return CloseConsoleHandle(hObject);

    Status = NtClose(hObject);
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DuplicateHandle(IN HANDLE hSourceProcessHandle,
                IN HANDLE hSourceHandle,
                IN HANDLE hTargetProcessHandle,
                OUT LPHANDLE lpTargetHandle,
                IN DWORD dwDesiredAccess,
                IN BOOL bInheritHandle,
                IN DWORD dwOptions)
{
    NTSTATUS Status;
    HANDLE hTargetHandle;

    // added for winsock
    HMODULE winsock_module = NULL;
    typedef int (WINAPI *wsstcall)(DWORD, LPWSADATA);
    typedef int (WINAPI *wsskcall)(int, int, int, LPWSAPROTOCOL_INFOW, GROUP, DWORD);
    typedef int (WINAPI *wsgsocall)(SOCKET, int, int, char*, int*);
    typedef int (WINAPI *wsdskcall)(SOCKET, DWORD, LPWSAPROTOCOL_INFOW);	
    wsgsocall getsockopt;
    wsdskcall dupwinsock;
    typedef int (WINAPI *wsskcall)(int, int, int, LPWSAPROTOCOL_INFOW, GROUP, DWORD);
    wsskcall DynWSASocket;
    int optVal;
    int optLen = sizeof(int);
    WSAPROTOCOL_INFOW SharedSocketInfo; // our buffer for collecting shared socket information
    DWORD handleFlags;
    // end of winsock code items
    
    hSourceHandle = TranslateStdHandle(hSourceHandle);

    if ((IsConsoleHandle(hSourceHandle)) &&
        ((hSourceHandle != NtCurrentProcess()) &&
         (hSourceHandle != NtCurrentThread())))
    {
        /*
         * We can duplicate console handles only if both the source
         * and the target processes are in fact the current process.
         */
        if ((hSourceProcessHandle != NtCurrentProcess()) ||
            (hTargetProcessHandle != NtCurrentProcess()))
        {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
        }

        hTargetHandle = DuplicateConsoleHandle(hSourceHandle,
                                               dwDesiredAccess,
                                               bInheritHandle,
                                               dwOptions);
        if (hTargetHandle != INVALID_HANDLE_VALUE)
        {
            if (lpTargetHandle) *lpTargetHandle = hTargetHandle;
            return TRUE;
        }

        return FALSE;
    }

    // winsock socket handles code currently only dupes from self or inherited sockets from parent are implemented
    // TODO: add socket duplication from arbitrary source process (works in Windows 2003)
    WCHAR nameFull[80];
    ULONG returnedLength;
    BOOLEAN isSocket = FALSE;
    BOOLEAN canDupe = FALSE;
    _SEH2_TRY
    {
        // we rely on a side effect of a failed call.. we get the name even though call failed
        NtQueryObject(hSourceHandle, ObjectNameInformation, nameFull, sizeof(nameFull), &returnedLength);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        int excd = _SEH2_GetExceptionCode();
        DPRINT1("DuplicateHandle: NtQueryObject generated an exception (%lx) for handle(%lx)\n", excd, hSourceHandle);			
    }
    _SEH2_END;
    if (wcscmp((LPWSTR)(nameFull + (2 * sizeof(WCHAR))), L"\\Device\\Afd") == 0)
    { // check if this belongs to AFD, if so it's a winsock socket
        DPRINT("DuplicateHandle: NtQueryObject winsock socket device(AFD) detected for handle(%lx)\n", hSourceHandle);				
        // DynLink the winsock routines we need for this work.
        DPRINT("DuplicateHandle: calling GetModuleHandleA(\"ws2_32\").\n");
        winsock_module = GetModuleHandleA("ws2_32");
        if (winsock_module)
        {
            DynWSASocket = (wsskcall)GetProcAddress(winsock_module, "WSASocketW");
            getsockopt = (wsgsocall)GetProcAddress(winsock_module, "getsockopt");
            if (getsockopt) 
            {
                // getsockopt is our gatekeeper. if it fails winsock access is not in a reliable state
                int result = getsockopt((SOCKET)hSourceHandle, SOL_SOCKET, SO_TYPE, (char*)&optVal, &optLen);
                if (result == 0) 
                {
                    if(optVal == SOCK_STREAM)
                        DPRINT("(PID %lx) DuplicateHandle: socket is a TCP socket.\n");
                    else if(optVal == SOCK_DGRAM)
                        DPRINT("DuplicateHandle: socket is a UDP socket.\n");
                    dupwinsock = (wsdskcall)GetProcAddress(winsock_module, "WSADuplicateSocketW");
                    isSocket = TRUE; // we set this here which makes sockets fall back to normal handle duplication if we cannot dynlink
                } else DPRINT1("Error calling getsockopt for inheritable socket.\n");
            } else DPRINT1("Error GetProcAddressA() could not Dynlink getsockopt!\n");			
        } else DPRINT1("Error GetModuleHandleA() could not find WS2_32 module handle.\n");
        // NOTE: currently this code duplicates the handle/socket but DOES NOT CHAGE THE HANDLE ACCESS OR OPTIONS IN ANY WAY (ignores dwOption and access rights changes)
        // TODO: revise code to respect access changes requested in parameters
        if (isSocket) {
            if (GetCurrentProcessId() == GetProcessId(hSourceProcessHandle)) {
                canDupe = TRUE; // this is the only case that is fairly certain to work
            }
            else if (hGetParentProcessId() == GetProcessId(hSourceProcessHandle)) { // handle is from parent process, it can be directly duplicated if inheritable
                // check if handle has inheritance set
                if (GetHandleInformation(hSourceHandle, &handleFlags)) { // get flags for handle
                    if (handleFlags & HANDLE_FLAG_INHERIT) { // need to check if it's winsock.. or not.. also if access is changing or inheritance is changing, thos would require dupe
                        if (dwOptions & DUPLICATE_SAME_ACCESS) {
                            DPRINT("DuplicateHandle: winsock inherited handle with same access, duplicating handle.\n");			
                        } else {
                            DPRINT("DuplicateHandle: winsock inherited handle with differing access, duplicating handle and ignoring access/dwOptions (PART IMPL FIXME).\n");										
                        }
                        canDupe = TRUE;
                    }
                } else {
                    DPRINT1("DuplicateHandle: GetHandleInformation failed for parent owned handle(%lx) getlasterror is (%lx).\n", hSourceHandle, GetLastError());			
                }
            }
            if(canDupe) 
            { 
                SOCKET targetSocket;
                *lpTargetHandle = hSourceHandle; // if we fail we just return the handle we were given HACK TODO: Fix this
                if (dupwinsock) 
                { // if we didn't dynlink this gets skipped
                    _SEH2_TRY 
                    {			
                        int result = dupwinsock((SOCKET)hSourceHandle, GetCurrentProcessId(), &SharedSocketInfo); // was GetProcessId(ProcessHandle)
                        if (result != 0) {
                            DPRINT1("DuplicateHandle: IN Process call to WSADuplicateSocketW failed, returned:%x.\n", result);
                        } else {
                            // now call wsasocket to finish the process and create new socket
                            targetSocket = DynWSASocket(SharedSocketInfo.iAddressFamily, SharedSocketInfo.iSocketType, SharedSocketInfo.iProtocol, &SharedSocketInfo, 0, WSA_FLAG_OVERLAPPED); 
                            // 	not sure we want overlapped set, it's the default for winsock 1.x but winsock 2 stopped setting it by default	
                            if (targetSocket != INVALID_SOCKET) {
                                DPRINT("DuplicateHandle: winsock socket handle succesfully duplicated, returning handle(%lx).\n", targetSocket);
                                *lpTargetHandle = (HANDLE)targetSocket;							
                            } else DPRINT1("DuplicateHandle: WSASocketW on cross process call returned INVALID_SOCKET, returning input handle(%lx) (BADIMPL FIXME).\n", hSourceHandle);						
                        }
                    }			
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DPRINT1("DuplicateHandle: SOCKET DUPLICATION exception (%lx) for duplicating handle(%lx) returning source handle (BADIMPL FIXME).\n", _SEH2_GetExceptionCode(), hSourceHandle);			
                    }
                    _SEH2_END;		
                }
                DPRINT("DuplicateHandle: SOCKET DUPLICATION completed, returning true.\n");									
            } else { // !canDupe - winsock handle, but not from self or parent process, THIS WILL PROBABLY FAIL (FIXME)
                DPRINT("DuplicateHandle: canDupe is false, attempting CROSS-PROCESS socket duplication.\n");			
                SOCKET targetSocket;
                *lpTargetHandle = hSourceHandle; // if we fail we just return the handle we were given which will work but may result in closing twice or prematurely
                // Try to duplicate the socket
                if (dupwinsock) 
                { // we didn't dynlink this gets skipped		
                    _SEH2_TRY 
                    {			
                        int result = dupwinsock((SOCKET)hSourceHandle, GetProcessId(hSourceProcessHandle), &SharedSocketInfo); // was GetProcessId(ProcessHandle)
                        if (result != 0) 
                        {
                            DPRINT1("DuplicateHandle: CROSS Process call to WSADuplicateSocketW failed, returned:%x.\n", result);
                        } else {
                            // now call wsasocket to finish the process and create new socket for us
                            targetSocket = DynWSASocket(SharedSocketInfo.iAddressFamily, SharedSocketInfo.iSocketType, SharedSocketInfo.iProtocol, &SharedSocketInfo, 0, WSA_FLAG_OVERLAPPED); 
                            // 	not sure we want overlapped set, it's the default for winsock 1.x but winsock 2 stopped setting it by default	
                            if (targetSocket != INVALID_SOCKET) 
                            {
                                DPRINT("DuplicateHandle: winsock socket handle succesfully duplicated, returning handle(%lx).\n", targetSocket);
                                *lpTargetHandle = (HANDLE)targetSocket;							
                            } else DPRINT1("DuplicateHandle: WSASocketW on cross process call returned INVALID_SOCKET, returning input handle(%lx) (BADIMPL FIXME).\n", hSourceHandle);						
                        }
                    }			
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DPRINT1("DuplicateHandle: CROSS PROCESS SOCKET DUPLICATION exception (%lx) for duplicating handle(%lx) reutrning source handle.\n", _SEH2_GetExceptionCode(), hSourceHandle);			
                    }
                    _SEH2_END;	
                }
                DPRINT("DuplicateHandle: SOCKET DUPLICATION completed, returning true.\n");									
            }
            return TRUE;
        }
    }
    DPRINT("DuplicateHandle: handle is not a socket, proceeding with NtDuplicateObject\n");
    
    Status = NtDuplicateObject(hSourceProcessHandle,
                               hSourceHandle,
                               hTargetProcessHandle,
                               lpTargetHandle,
                               dwDesiredAccess,
                               bInheritHandle ? OBJ_INHERIT : 0,
                               dwOptions);
    if (NT_SUCCESS(Status)) return TRUE;

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
UINT
WINAPI
SetHandleCount(IN UINT nCount)
{
    return nCount;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetHandleContext(IN HANDLE Handle)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!GetHandleContext() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateSocketHandle(VOID)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!CreateSocketHandle() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetHandleContext(IN HANDLE Handle,
                 IN DWORD Context)
{
    /* This is Windows behavior, not a ReactOS Stub */
    DbgPrintEx(0, 0, "Unsupported API - kernel32!SetHandleContext() called\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/* EOF */
