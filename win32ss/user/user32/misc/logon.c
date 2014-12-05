/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/logon.c
 * PURPOSE:         Logon functions
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
RegisterServicesProcess(DWORD ServicesProcessId)
{
    USER_API_MESSAGE ApiMessage;
    PUSER_REGISTER_SERVICES_PROCESS RegisterServicesProcessRequest = &ApiMessage.Data.RegisterServicesProcessRequest;

    RegisterServicesProcessRequest->ProcessId = ServicesProcessId;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpRegisterServicesProcess),
                        sizeof(*RegisterServicesProcessRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        UserSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
RegisterLogonProcess(DWORD dwProcessId,
                     BOOL bRegister)
{
    gfLogonProcess = NtUserxRegisterLogonProcess(dwProcessId, bRegister);

    if (gfLogonProcess)
    {
        USER_API_MESSAGE ApiMessage;
        PUSER_REGISTER_LOGON_PROCESS RegisterLogonProcessRequest = &ApiMessage.Data.RegisterLogonProcessRequest;

        RegisterLogonProcessRequest->ProcessId = dwProcessId;
        RegisterLogonProcessRequest->Register  = bRegister;

        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpRegisterLogonProcess),
                            sizeof(*RegisterLogonProcessRequest));
        if (!NT_SUCCESS(ApiMessage.Status))
        {
            ERR("Failed to register logon process with CSRSS\n");
            UserSetLastNTError(ApiMessage.Status);
        }
    }

    return gfLogonProcess;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetLogonNotifyWindow(HWND Wnd, HWINSTA WinSta)
{
    return NtUserSetLogonNotifyWindow(Wnd);
}

/*
 * @implemented
 */
BOOL
WINAPI
UpdatePerUserSystemParameters(DWORD dwReserved,
                              BOOL bEnable)
{
    return NtUserUpdatePerUserSystemParameters(dwReserved, bEnable);
}
