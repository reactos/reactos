/**************************** Module Header ********************************\
* Module Name: service.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Service Support Routines
*
* History:
* 12-22-93 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxConnectService
*
* Open the windowstation assigned to the service logon session.  If
* no windowstation exists, create the windowstation and a default desktop.
*
* History:
* 12-23-93 JimA         Created.
\***************************************************************************/

HWINSTA xxxConnectService(
    PUNICODE_STRING pstrWinSta,
    HDESK *phdesk)
{
    NTSTATUS Status;
    HANDLE hToken;
    ULONG ulLength;
    PTOKEN_USER ptuService;
    PSECURITY_DESCRIPTOR psdService;
    PSID psid;
    PACCESS_ALLOWED_ACE paceService = NULL, pace;
    OBJECT_ATTRIBUTES ObjService;
    HWINSTA hwinsta;
    UNICODE_STRING strDesktop;
    TL tlPoolSdService, tlPoolAceService, tlPoolToken;

    /*
     * Open the token of the service.
     */
    Status = OpenEffectiveToken(&hToken);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "ConnectService: Could not open process/thread token (0x%X)", Status);
        return NULL;
    }

    /*
     * Get the user SID assigned to the service.
     */
    ptuService = NULL;
    paceService = NULL;
    psdService = NULL;
    hwinsta = NULL;
    ZwQueryInformationToken(hToken, TokenUser, NULL, 0, &ulLength);
    ptuService = (PTOKEN_USER)UserAllocPool(ulLength, TAG_TOKEN);
    if (ptuService == NULL) {
        RIPMSG1(RIP_WARNING, "ConnectService: Can't alloc buffer (size=%d) for token info", ulLength);
        goto sd_error;
    }
    Status = ZwQueryInformationToken(hToken, TokenUser, ptuService,
            ulLength, &ulLength);
    ZwClose(hToken);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "ConnectService: QueryInformationToken failed (0x%X)", Status);
        goto sd_error;
    }
    psid = ptuService->User.Sid;

    /*
     * Create ACE list.
     */
    paceService = AllocAce(NULL, ACCESS_ALLOWED_ACE_TYPE, 0,
            WINSTA_CREATEDESKTOP | WINSTA_READATTRIBUTES |
                WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS |
                WINSTA_ACCESSCLIPBOARD | STANDARD_RIGHTS_REQUIRED,
            psid, &ulLength);
    if (paceService == NULL) {
        RIPMSG0(RIP_WARNING, "ConnectService: AllocAce for WindowStation attributes failed");
        goto sd_error;
    }
    pace = AllocAce(paceService, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE |
            INHERIT_ONLY_ACE | NO_PROPAGATE_INHERIT_ACE,
            DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                STANDARD_RIGHTS_REQUIRED,
            psid, &ulLength);
    if (pace == NULL) {
        RIPMSG0(RIP_WARNING, "ConnectService: AllocAce for Desktop Attributes failed");
        goto sd_error;
    }
    paceService = pace;
    pace = AllocAce(pace, ACCESS_ALLOWED_ACE_TYPE, 0,
            WINSTA_ENUMERATE,
            SeExports->SeAliasAdminsSid, &ulLength);
    if (pace == NULL) {
        RIPMSG0(RIP_WARNING, "ConnectService: AllocAce for admin WinSta enumerate failed");
        goto sd_error;
    }
    paceService = pace;
    pace = AllocAce(pace, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE |
            INHERIT_ONLY_ACE | NO_PROPAGATE_INHERIT_ACE,
            DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE,
            SeExports->SeAliasAdminsSid, &ulLength);
    if (pace == NULL) {
        RIPMSG0(RIP_WARNING, "ConnectService: AllocAce for admin Desktop access failed");
        goto sd_error;
    }
    paceService = pace;

    /*
     * Initialize the SD
     */
    psdService = CreateSecurityDescriptor(paceService, ulLength, FALSE);
    if (psdService == NULL) {
        RIPMSG0(RIP_WARNING, "ConnectService: CreateSecurityDescriptor failed");
        goto sd_error;
    }

    ThreadLockPool(PtiCurrent(), ptuService,  &tlPoolToken);
    ThreadLockPool(PtiCurrent(), paceService, &tlPoolAceService);
    ThreadLockPool(PtiCurrent(), psdService,  &tlPoolSdService);

    /*
     * The windowstation does not exist and must be created.
     */
    InitializeObjectAttributes(&ObjService, pstrWinSta,
            OBJ_OPENIF, NULL, psdService);
    hwinsta = xxxCreateWindowStation(&ObjService,
                                     KernelMode,
                                     MAXIMUM_ALLOWED,
                                     NULL, 0, NULL, 0);
    if (hwinsta != NULL) {

        TRACE_INIT(("Service windowstation created\n"));

        /*
         * We have the windowstation, now create the desktop.  The security
         * descriptor will be inherited from the windowstation.  Save the
         * winsta handle because the access struct may be moved by the
         * desktop creation.
         */
        RtlInitUnicodeString(&strDesktop, TEXT("Default"));
        InitializeObjectAttributes(&ObjService, &strDesktop,
                OBJ_OPENIF | OBJ_CASE_INSENSITIVE, hwinsta, NULL);

        *phdesk = xxxCreateDesktop(&ObjService, KernelMode,
                NULL, NULL, 0, MAXIMUM_ALLOWED);

        if (*phdesk == NULL) {

            /*
             * The creation failed, wake the desktop thread, close the
             * windowstation and leave.
             */
            RIPMSG0(RIP_WARNING, "ConnectService: CreateDesktop('Default') failed.");

            ZwClose(hwinsta);
            hwinsta = NULL;
        } else {
            TRACE_INIT(("Default desktop in Service windowstation created\n"));
        }
    } else {
        *phdesk = NULL;
    }

    ThreadUnlockPool(PtiCurrent(), &tlPoolSdService);
    ThreadUnlockPool(PtiCurrent(), &tlPoolAceService);
    ThreadUnlockPool(PtiCurrent(), &tlPoolToken);

sd_error:
    if (ptuService != NULL)
        UserFreePool(ptuService);
    if (paceService != NULL)
        UserFreePool(paceService);
    if (psdService != NULL)
        UserFreePool(psdService);

    return hwinsta;
}

