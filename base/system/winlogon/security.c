/*
 * PROJECT:         ReactOS Winlogon
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security utility infrastructure implementation of Winlogon
 * COPYRIGHT:       Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

/* DEFINES ******************************************************************/

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
    DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
    DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
    DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define DESKTOP_ADMINS_LIMITED (DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | \
    DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_ENUMERATE)

#define DESKTOP_INTERACTIVE_LIMITED (STANDARD_RIGHTS_READ | DESKTOP_ENUMERATE | \
    DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW)

#define DESKTOP_WINLOGON_ADMINS_LIMITED (STANDARD_RIGHTS_REQUIRED | DESKTOP_ENUMERATE)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
    WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
    WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
    WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
    STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ADMINS_LIMITED (WINSTA_READATTRIBUTES | WINSTA_ENUMERATE)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | \
    GENERIC_EXECUTE | GENERIC_ALL)

/* GLOBALS ******************************************************************/

static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

/* FUNCTIONS ****************************************************************/

/**
 * @brief
 * Converts an absolute security descriptor to a self-relative
 * format.
 *
 * @param[in] AbsoluteSd
 * A pointer to an absolute security descriptor to be
 * converted.
 *
 * @return
 * Returns a pointer to a converted security descriptor in
 * self-relative format. If the function fails, NULL is returned
 * otherwise.
 *
 * @remarks
 * The function allocates the security descriptor buffer in memory
 * heap, the caller is entirely responsible for freeing such buffer
 * from when it's no longer needed.
 */
PSECURITY_DESCRIPTOR
ConvertToSelfRelative(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSd)
{
    PSECURITY_DESCRIPTOR RelativeSd;
    DWORD DescriptorLength = 0;

    /* Determine the size for our buffer to allocate */
    if (!MakeSelfRelativeSD(AbsoluteSd, NULL, &DescriptorLength) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        ERR("ConvertToSelfRelative(): Unexpected error code (error code %lu -- must be ERROR_INSUFFICIENT_BUFFER)\n", GetLastError());
        return NULL;
    }

    /* Allocate the buffer now */
    RelativeSd = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 DescriptorLength);
    if (RelativeSd == NULL)
    {
        ERR("ConvertToSelfRelative(): Failed to allocate buffer for relative SD!\n");
        return NULL;
    }

    /* Convert the security descriptor now */
    if (!MakeSelfRelativeSD(AbsoluteSd, RelativeSd, &DescriptorLength))
    {
        ERR("ConvertToSelfRelative(): Failed to convert the security descriptor to a self relative format (error code %lu)\n", GetLastError());
        RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
        return NULL;
    }

    return RelativeSd;
}

/**
 * @brief
 * Creates a security descriptor for the default
 * window station upon its creation.
 *
 * @param[out] WinstaSd
 * A pointer to a created security descriptor for
 * the window station.
 *
 * @return
 * Returns TRUE if the function has successfully
 * created the security descriptor, FALSE otherwise.
 */
BOOL
CreateWinstaSecurity(
    _Out_ PSECURITY_DESCRIPTOR *WinstaSd)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL, NetworkServiceSid = NULL; /* NetworkServiceSid is a HACK, see the comment below for information */
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("CreateWinstaSecurity(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("CreateWinstaSecurity(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Create the network service SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_NETWORK_SERVICE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &NetworkServiceSid))
    {
        ERR("CreateWinstaSecurity(): Failed to create the network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of four ACEs of two different SIDs. The first two
     * ACEs give both window station and generic access
     * to Winlogon, the last two give limited window station
     * and desktop access to admins.
     *
     * ===================== !!!MUST READ!!! =====================
     *
     * HACK -- Include in the DACL two more ACEs for network
     * service SID. Network services will be granted full
     * access to the default window station. Whilst technically
     * services that are either network or local ones are part
     * and act on behalf of the system, what we are doing here
     * is a hack because of two reasons:
     *
     * 1) Winlogon does not allow default window station (Winsta0)
     * access to network services on Windows. As a matter of fact,
     * network services must access their own service window station
     * (aka Service-0x0-3e4$) which never gets created. Why it never
     * gets created is explained on the second point.
     *
     * 2) Our LSASS terribly lacks in code that handles special logon
     * service types, NetworkService and LocalService. For this reason
     * whenever an access token is created for a network service process
     * for example, its authentication ID (aka LogonId represented as a LUID)
     * is a uniquely generated ID by LSASS for this process. This is wrong
     * on so many levels, partly because a network service is not a regular
     * service and network services have their own special authentication logon
     * ID (with its respective LUID as {0x3e4, 0x0}). On top of that, a network
     * service process must have an impersonation token but for whatever reason
     * we are creating a primary access token instead.
     *
     * FOR ANYONE WHO'S INTERESTED ON FIXING THIS, DO NOT FORGET TO REMOVE THIS
     * HACK!!!
     *
     * =========================== !!!END!!! ================================
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid);

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("CreateWinstaSecurity(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("CreateWinstaSecurity(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- give full winsta access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               WinlogonSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- give full generic access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               WinlogonSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Third ACE -- give limited winsta access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Fourth ACE -- give limited desktop access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               DESKTOP_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Fifth ACE -- give full access to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               NetworkServiceSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for network service (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Sixth ACE -- give full generic access to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               NetworkServiceSid))
    {
        ERR("CreateWinstaSecurity(): Failed to set ACE for network service (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("CreateWinstaSecurity(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("CreateWinstaSecurity(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Convert it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("CreateWinstaSecurity(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Give the descriptor to the caller */
    *WinstaSd = RelativeSd;
    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    /* HACK */
    if (NetworkServiceSid != NULL)
    {
        FreeSid(NetworkServiceSid);
    }
    /* END HACK */

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Success == FALSE)
    {
        if (RelativeSd != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
        }
    }

    return Success;
}

/**
 * @brief
 * Creates a security descriptor for the default
 * application desktop upon its creation.
 *
 * @param[out] ApplicationDesktopSd
 * A pointer to a created security descriptor for
 * the application desktop.
 *
 * @return
 * Returns TRUE if the function has successfully
 * created the security descriptor, FALSE otherwise.
 */
BOOL
CreateApplicationDesktopSecurity(
    _Out_ PSECURITY_DESCRIPTOR *ApplicationDesktopSd)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL, NetworkServiceSid = NULL; /* NetworkServiceSid is a HACK, see the comment in CreateWinstaSecurity for information */
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Create the network service SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_NETWORK_SERVICE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &NetworkServiceSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to create the network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of two ACEs of two different SIDs. The first ACE
     * gives full access to Winlogon, the last one gives
     * limited desktop access to admins.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid); /* HACK */

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- Give full desktop power to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               WinlogonSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- Give limited desktop power to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Third ACE -- Give full desktop power to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               NetworkServiceSid))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to set ACE for network services (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Conver it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("CreateApplicationDesktopSecurity(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Give the descriptor to the caller */
    *ApplicationDesktopSd = RelativeSd;
    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    /* HACK */
    if (NetworkServiceSid != NULL)
    {
        FreeSid(NetworkServiceSid);
    }
    /* END HACK */

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Success == FALSE)
    {
        if (RelativeSd != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
        }
    }

    return Success;
}

/**
 * @brief
 * Creates a security descriptor for the default
 * Winlogon desktop. This descriptor serves as a
 * security measure for the winlogon desktop so
 * that only Winlogon itself (and admins) can
 * interact with it.
 *
 * @param[out] WinlogonDesktopSd
 * A pointer to a created security descriptor for
 * the Winlogon desktop.
 *
 * @return
 * Returns TRUE if the function has successfully
 * created the security descriptor, FALSE otherwise.
 */
BOOL
CreateWinlogonDesktopSecurity(
    _Out_ PSECURITY_DESCRIPTOR *WinlogonDesktopSd)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL;
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of two ACEs of two different SIDs. The first ACE
     * gives full access to Winlogon, the last one gives
     * limited desktop access to admins.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid);

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- Give full desktop access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               WinlogonSid))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- Give limited desktop access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_WINLOGON_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Conver it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("CreateWinlogonDesktopSecurity(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Give the descriptor to the caller */
    *WinlogonDesktopSd = RelativeSd;
    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Success == FALSE)
    {
        if (RelativeSd != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
        }
    }

    return Success;
}

/**
 * @brief
 * Creates a security descriptor for the screen
 * saver desktop.
 *
 * @param[out] ScreenSaverDesktopSd
 * A pointer to a created security descriptor for
 * the screen-saver desktop.
 *
 * @return
 * Returns TRUE if the function has successfully
 * created the security descriptor, FALSE otherwise.
 */
BOOL
CreateScreenSaverSecurity(
    _Out_ PSECURITY_DESCRIPTOR *ScreenSaverDesktopSd)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL, InteractiveSid = NULL;
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Create the interactive logon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &InteractiveSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to create the interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of three ACEs of three different SIDs. The first ACE
     * gives full access to Winlogon, the second one gives
     * limited desktop access to admins and the last one
     * gives full desktop access to users who have logged in
     * interactively.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(InteractiveSid);

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("CreateScreenSaverSecurity(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("CreateScreenSaverSecurity(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- Give full desktop access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               WinlogonSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- Give limited desktop access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               DESKTOP_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Third ACE -- Give full desktop access to interactive logon users */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               DESKTOP_INTERACTIVE_LIMITED,
                               InteractiveSid))
    {
        ERR("CreateScreenSaverSecurity(): Failed to set ACE for interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("CreateScreenSaverSecurity(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("CreateScreenSaverSecurity(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Conver it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("CreateScreenSaverSecurity(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Give the descriptor to the caller */
    *ScreenSaverDesktopSd = RelativeSd;
    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    if (InteractiveSid != NULL)
    {
        FreeSid(InteractiveSid);
    }

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (Success == FALSE)
    {
        if (RelativeSd != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
        }
    }

    return Success;
}

/**
 * @brief
 * Assigns access to the specific logon user to
 * the default window station. Such access is
 * given to the user when it has logged in.
 *
 * @param[in] WinSta
 * A handle to a window station where the
 * user is given access to it.
 *
 * @param[in] LogonSid
 * A pointer to a logon SID that represents
 * the logged in user in question.
 *
 * @return
 * Returns TRUE if the function has successfully
 * assigned access to the user, FALSE otherwise.
 */
BOOL
AllowWinstaAccessToUser(
    _In_ HWINSTA WinSta,
    _In_ PSID LogonSid)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL, InteractiveSid = NULL, NetworkServiceSid = NULL; /* NetworkServiceSid is a HACK, see the comment in CreateWinstaSecurity for information */
    SECURITY_INFORMATION SecurityInformation;
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Create the interactive logon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &InteractiveSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to create the interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Create the network service SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_NETWORK_SERVICE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &NetworkServiceSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to create the network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of eight ACEs of four different SIDs. The first ACE
     * gives full winsta access to Winlogon, the second one gives
     * generic access to Winlogon. Such approach is the same
     * for both interactive logon users and logon user as well.
     * Only admins are given limited powers.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(InteractiveSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(InteractiveSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(LogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(LogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid) + /* HACK */
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid);

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("AllowWinstaAccessToUser(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("AllowWinstaAccessToUser(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- Give full winsta access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               WinlogonSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- Give generic access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               WinlogonSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Third ACE -- Give limited winsta access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Fourth ACE -- Give limited desktop access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               DESKTOP_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Fifth ACE -- Give full winsta access to interactive logon users */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               InteractiveSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Sixth ACE -- Give generic access to interactive logon users */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               InteractiveSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Seventh ACE -- Give full winsta access to logon user */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               LogonSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for logon user SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Eighth ACE -- Give generic access to logon user */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               LogonSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for logon user SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK : Ninenth ACE -- Give full winsta access to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               NO_PROPAGATE_INHERIT_ACE,
                               WINSTA_ALL,
                               NetworkServiceSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for logon network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Tenth ACE -- Give generic access to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                               GENERIC_ACCESS,
                               NetworkServiceSid))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set ACE for network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("AllowWinstaAccessToUser(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Convert it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("AllowWinstaAccessToUser(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Set new winsta security based on this descriptor */
    SecurityInformation = DACL_SECURITY_INFORMATION;
    if (!SetUserObjectSecurity(WinSta, &SecurityInformation, RelativeSd))
    {
        ERR("AllowWinstaAccessToUser(): Failed to set window station security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    if (InteractiveSid != NULL)
    {
        FreeSid(InteractiveSid);
    }

    /* HACK */
    if (NetworkServiceSid != NULL)
    {
        FreeSid(NetworkServiceSid);
    }
    /* END HACK */

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (RelativeSd != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
    }

    return Success;
}

/**
 * @brief
 * Assigns access to the specific logon user to
 * the default desktop. Such access is given to
 * the user when it has logged in.
 *
 * @param[in] Desktop
 * A handle to a desktop where the user
 * is given access to it.
 *
 * @param[in] LogonSid
 * A pointer to a logon SID that represents
 * the logged in user in question.
 *
 * @return
 * Returns TRUE if the function has successfully
 * assigned access to the user, FALSE otherwise.
 */
BOOL
AllowDesktopAccessToUser(
    _In_ HDESK Desktop,
    _In_ PSID LogonSid)
{
    BOOL Success = FALSE;
    SECURITY_DESCRIPTOR AbsoluteSd;
    PSECURITY_DESCRIPTOR RelativeSd = NULL;
    PSID WinlogonSid = NULL, AdminsSid = NULL, InteractiveSid = NULL, NetworkServiceSid = NULL; /* NetworkServiceSid is a HACK, see the comment in CreateWinstaSecurity for information */
    SECURITY_INFORMATION SecurityInformation;
    DWORD DaclSize;
    PACL Dacl;

    /* Create the Winlogon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &WinlogonSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to create the Winlogon SID (error code %lu)\n", GetLastError());
        return FALSE;
    }

    /* Create the admins SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to create the admins SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Create the interactive logon SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_INTERACTIVE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &InteractiveSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to create the interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Create the network service SID */
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_NETWORK_SERVICE_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &NetworkServiceSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to create the network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /*
     * Build up the DACL size. This includes a number
     * of four ACEs of four different SIDs. The first ACE
     * gives full desktop access to Winlogon, the second one gives
     * generic limited desktop access to admins. The last two give
     * full power to both interactive logon users and logon user as
     * well.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(WinlogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(AdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(InteractiveSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(LogonSid) +
               sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(NetworkServiceSid); /* HACK */

    /* Allocate the DACL now */
    Dacl = RtlAllocateHeap(RtlGetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           DaclSize);
    if (Dacl == NULL)
    {
        ERR("AllowDesktopAccessToUser(): Failed to allocate memory buffer for DACL!\n");
        goto Quit;
    }

    /* Initialize it */
    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        ERR("AllowDesktopAccessToUser(): Failed to initialize DACL (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* First ACE -- Give full desktop access to Winlogon */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               WinlogonSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set ACE for Winlogon (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Second ACE -- Give limited desktop access to admins */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ADMINS_LIMITED,
                               AdminsSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set ACE for admins (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Third ACE -- Give full desktop access to interactive logon users */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               InteractiveSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set ACE for interactive SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Fourth ACE -- Give full desktop access to logon user */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               LogonSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set ACE for logon user SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* HACK: Fifth ACE -- Give full desktop to network services */
    if (!AddAccessAllowedAceEx(Dacl,
                               ACL_REVISION,
                               0,
                               DESKTOP_ALL,
                               NetworkServiceSid))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set ACE for network service SID (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsoluteSd, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("AllowDesktopAccessToUser(): Failed to initialize absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Set the DACL to the descriptor */
    if (!SetSecurityDescriptorDacl(&AbsoluteSd, TRUE, Dacl, FALSE))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set up DACL to absolute security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Conver it to self-relative format */
    RelativeSd = ConvertToSelfRelative(&AbsoluteSd);
    if (RelativeSd == NULL)
    {
        ERR("AllowDesktopAccessToUser(): Failed to convert security descriptor to self relative format!\n");
        goto Quit;
    }

    /* Assign new security to desktop based on this descriptor */
    SecurityInformation = DACL_SECURITY_INFORMATION;
    if (!SetUserObjectSecurity(Desktop, &SecurityInformation, RelativeSd))
    {
        ERR("AllowDesktopAccessToUser(): Failed to set desktop security descriptor (error code %lu)\n", GetLastError());
        goto Quit;
    }

    Success = TRUE;

Quit:
    if (WinlogonSid != NULL)
    {
        FreeSid(WinlogonSid);
    }

    if (AdminsSid != NULL)
    {
        FreeSid(AdminsSid);
    }

    if (InteractiveSid != NULL)
    {
        FreeSid(InteractiveSid);
    }

    /* HACK */
    if (NetworkServiceSid != NULL)
    {
        FreeSid(NetworkServiceSid);
    }
    /* END HACK */

    if (Dacl != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Dacl);
    }

    if (RelativeSd != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, RelativeSd);
    }

    return Success;
}

/**
 * @brief
 * Assigns both window station and desktop access
 * to the specific session currently active on the
 * system.
 *
 * @param[in] Session
 * A pointer to an active session.
 *
 * @return
 * Returns TRUE if the function has successfully
 * assigned access to the current session, FALSE otherwise.
 */
BOOL
AllowAccessOnSession(
    _In_ PWLSESSION Session)
{
    BOOL Success = FALSE;
    DWORD Index, SidLength, GroupsLength = 0;
    PTOKEN_GROUPS TokenGroup = NULL;
    PSID LogonSid;

    /* Get required buffer size and allocate the TOKEN_GROUPS buffer */
    if (!GetTokenInformation(Session->UserToken,
                             TokenGroups,
                             TokenGroup,
                             0,
                             &GroupsLength))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ERR("AllowAccessOnSession(): Unexpected error code returned, must be ERROR_INSUFFICIENT_BUFFER (error code %lu)\n", GetLastError());
            return FALSE;
        }

        TokenGroup = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, GroupsLength);
        if (TokenGroup == NULL)
        {
            ERR("AllowAccessOnSession(): Failed to allocate memory buffer for token group!\n");
            return FALSE;
        }
    }

    /* Get the token group information from the access token */
    if (!GetTokenInformation(Session->UserToken,
                             TokenGroups,
                             TokenGroup,
                             GroupsLength,
                             &GroupsLength))
    {
        ERR("AllowAccessOnSession(): Failed to retrieve the token group information (error code %lu)\n", GetLastError());
        goto Quit;
    }

    /* Loop through the groups to find the logon SID */
    for (Index = 0; Index < TokenGroup->GroupCount; Index++)
    {
        if ((TokenGroup->Groups[Index].Attributes & SE_GROUP_LOGON_ID)
            == SE_GROUP_LOGON_ID)
        {
            LogonSid = TokenGroup->Groups[Index].Sid;
            break;
        }
    }

    /* Allow window station access to this user within this session */
    if (!AllowWinstaAccessToUser(Session->InteractiveWindowStation, LogonSid))
    {
        ERR("AllowAccessOnSession(): Failed to allow winsta access to the logon user!\n");
        goto Quit;
    }

    /* Allow application desktop access to this user within this session */
    if (!AllowDesktopAccessToUser(Session->ApplicationDesktop, LogonSid))
    {
        ERR("AllowAccessOnSession(): Failed to allow application desktop access to the logon user!\n");
        goto Quit;
    }

    /* Get the length of this logon SID */
    SidLength = GetLengthSid(LogonSid);

    /* Assign the window station to this logged in user */
    if (!SetWindowStationUser(Session->InteractiveWindowStation,
                              &Session->LogonId,
                              LogonSid,
                              SidLength))
    {
        ERR("AllowAccessOnSession(): Failed to assign the window station to the logon user!\n");
        goto Quit;
    }

    Success = TRUE;

Quit:
    if (TokenGroup != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, TokenGroup);
    }

    return Success;
}

/* EOF */
