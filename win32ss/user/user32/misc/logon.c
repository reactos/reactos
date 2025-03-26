/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/misc/logon.c
 * PURPOSE:         Logon functions
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

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
 * Helper function used by SetWindowStationUser (see winsta.c)
 */
VOID FASTCALL
Logon(BOOL IsLogon)
{
    USER_API_MESSAGE ApiMessage;
    PUSER_LOGON LogonRequest = &ApiMessage.Data.LogonRequest;

    LogonRequest->IsLogon = IsLogon;
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                         NULL,
                         CSR_CREATE_API_NUMBER(USERSRV_SERVERDLL_INDEX, UserpLogon),
                         sizeof(*LogonRequest));
}

/*
 * @implemented
 */
BOOL
WINAPI
SetLogonNotifyWindow(HWND Wnd)
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
    // Update Imm support and load Imm32.dll.
    UpdatePerUserImmEnabling();

    /* Initialize the IME hotkeys */
    CliImmInitializeHotKeys(SETIMEHOTKEY_INITIALIZE, NULL);

    /* Load Preload keyboard layouts */
    IntLoadPreloadKeyboardLayouts();

    return NtUserUpdatePerUserSystemParameters(dwReserved, bEnable);
}
