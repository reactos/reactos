/****************************** Module Header ******************************\
* Module Name: usrpro.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis in usrpro.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

//
// Prototypes
//

BOOL
SaveUserProfile(
    PWINDOWSTATION pWS
    );

NTSTATUS
RestoreUserProfile(
    PTERMINAL pTerm
    );

NTSTATUS
MergeProfiles (
    PTERMINAL pTerm
    );

BOOL
IsUserAGuest(
    PWINDOWSTATION pWS
    );

DWORD
StartMachineGPOProcessing(
    PWLX_NOTIFICATION_INFO pNotifyInfo
    );

DWORD
StopMachineGPOProcessing(
    PWLX_NOTIFICATION_INFO pNotifyInfo
    );

DWORD
StartUserGPOProcessing(
    PWLX_NOTIFICATION_INFO pNotifyInfo
    );

DWORD
StopUserGPOProcessing(
    PWLX_NOTIFICATION_INFO pNotifyInfo
    );

DWORD
RunGPOLogonScripts(
    PWLX_NOTIFICATION_INFO pNotifyInfo
    );

VOID
InitializeGPOSupport(
    PTERMINAL pTerm
    );
