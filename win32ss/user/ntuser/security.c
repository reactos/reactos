/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security infrastructure of NTUSER component of Win32k
 * COPYRIGHT:       Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserSecurity);

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Opens an access token that represents the effective security
 * context of the caller. The purpose of this function is to query
 * the authenticated user that is associated with the security
 * context.
 *
 * @return
 * Returns a handle to an opened access token that represents the
 * security context of the authenticated user, otherwise NULL.
 */
HANDLE
IntGetCurrentAccessToken(VOID)
{
    NTSTATUS Status;
    HANDLE TokenHandle;

    /*
     * Try acquiring the security context by opening
     * the current thread (or so called impersonation)
     * token. Such token represents the effective caller.
     * Otherwise if the current thread does not have a
     * token (hence no impersonation occurs) then open
     * the token of main calling process instead.
     */
    Status = ZwOpenThreadToken(ZwCurrentThread(),
                               TOKEN_QUERY,
                               FALSE,
                               &TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        /*
         * We might likely fail to open the thread
         * token if the process isn't impersonating
         * a client. In scenarios where the server
         * isn't impersonating, open the main process
         * token.
         */
        if (Status == STATUS_NO_TOKEN)
        {
            TRACE("IntGetCurrentAccessToken(): The thread doesn't have a token, trying to open the process one...\n");
            Status = ZwOpenProcessToken(ZwCurrentProcess(),
                                        TOKEN_QUERY,
                                        &TokenHandle);
            if (!NT_SUCCESS(Status))
            {
                /* We failed opening process token as well, bail out... */
                ERR("IntGetCurrentAccessToken(): Failed to capture security context, couldn't open the process token (Status 0x%08lx)\n", Status);
                return NULL;
            }

            /* Return the opened token handle */
            return TokenHandle;
        }

        /* There's a thread token but we couldn't open it so bail out */
        ERR("IntGetCurrentAccessToken(): Failed to capture security context, couldn't open the thread token (Status 0x%08lx)\n", Status);
        return NULL;
    }

    /* Return the opened token handle */
    return TokenHandle;
}

/**
 * @brief
 * Allocates a buffer within UM (user mode) address
 * space area. Such buffer is reserved for security
 * purposes, such as allocating a buffer for a DACL
 * or a security descriptor.
 *
 * @param[in] Length
 * The length of the buffer that has to be allocated,
 * in bytes.
 *
 * @return
 * Returns a pointer to an allocated buffer whose
 * contents are arbitrary. If the function fails,
 * it means no pages are available to reserve for
 * memory allocation for this buffer.
 */
PVOID
IntAllocateSecurityBuffer(
    _In_ SIZE_T Length)
{
    NTSTATUS Status;
    PVOID Buffer = NULL;

    /* Allocate the buffer in UM memory space */
    Status = ZwAllocateVirtualMemory(ZwCurrentProcess(),
                                     &Buffer,
                                     0,
                                     &Length,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntAllocateSecurityBuffer(): Failed to allocate the buffer (Status 0x%08lx)\n", Status);
        return NULL;
    }

    return Buffer;
}

/**
 * @brief
 * Frees an allocated security buffer from UM
 * memory that is been previously allocated by
 * IntAllocateSecurityBuffer function.
 *
 * @param[in] Buffer
 * A pointer to a buffer whose contents are
 * arbitrary, to be freed from UM memory space.
 *
 * @return
 * Nothing.
 */
VOID
IntFreeSecurityBuffer(
    _In_ PVOID Buffer)
{
    SIZE_T Size = 0;

    ZwFreeVirtualMemory(ZwCurrentProcess(),
                        &Buffer,
                        &Size,
                        MEM_RELEASE);
}

/**
 * @brief
 * Queries the authenticated user security identifier
 * (SID) that is associated with the security context
 * of the access token that is being opened.
 *
 * @param[out] User
 * A pointer to the token user that contains the security
 * identifier of the authenticated user.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully
 * queried the token user. STATUS_UNSUCCESSFUL is returned
 * if the effective token of the caller couldn't be opened.
 * STATUS_NO_MEMORY is returned if memory allocation for
 * token user buffer has failed because of lack of necessary
 * pages reserved for such allocation. A failure NTSTATUS
 * code is returned otherwise.
 *
 * @remarks
 * !!!WARNING!!! -- THE CALLER WHO QUERIES THE TOKEN USER IS
 * RESPONSIBLE TO FREE THE ALLOCATED TOKEN USER BUFFER THAT IS
 * BEING GIVEN.
 */
NTSTATUS
IntQueryUserSecurityIdentification(
    _Out_ PTOKEN_USER *User)
{
    NTSTATUS Status;
    PTOKEN_USER UserToken;
    HANDLE Token;
    ULONG BufferLength;

    /* Initialize the parameter */
    *User = NULL;

    /* Open the current token of the caller */
    Token = IntGetCurrentAccessToken();
    if (!Token)
    {
        ERR("IntQueryUserSecurityIdentification(): Couldn't capture the token!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Since we do not know what the length
     * of the buffer size should be exactly to
     * hold the user data, let the function
     * tell us the size.
     */
    Status = ZwQueryInformationToken(Token,
                                     TokenUser,
                                     NULL,
                                     0,
                                     &BufferLength);
    if (!NT_SUCCESS(Status) && Status == STATUS_BUFFER_TOO_SMALL)
    {
        /*
         * Allocate some memory for the buffer
         * based on the size that the function
         * gave us.
         */
        UserToken = IntAllocateSecurityBuffer(BufferLength);
        if (!UserToken)
        {
            /* Bail out if we failed */
            ERR("IntQueryUserSecurityIdentification(): Couldn't allocate memory for the token user!\n");
            ZwClose(Token);
            return STATUS_NO_MEMORY;
        }
    }

    /* Query the user now as we have plenty of space to hold it */
    Status = ZwQueryInformationToken(Token,
                                     TokenUser,
                                     UserToken,
                                     BufferLength,
                                     &BufferLength);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, bail out */
        ERR("IntQueryUserSecurityIdentification(): Failed to query token user (Status 0x%08lx)\n", Status);
        IntFreeSecurityBuffer(UserToken);
        ZwClose(Token);
        return Status;
    }

    /* All good, give the buffer to the caller and close the captured token */
    *User = UserToken;
    ZwClose(Token);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Assigns a security descriptor to the desktop
 * object during a desktop object parse procedure.
 *
 * @param[in] WinSta
 * A pointer to a window station object, of which
 * such object contains its own security descriptor
 * that will be captured.
 *
 * @param[in] Desktop
 * A pointer to a desktop object that is created
 * during a parse procedure.
 *
 * @param[in] AccessState
 * A pointer to an access state structure that
 * describes the progress state of an access in
 * action.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully
 * assigned new security descriptor to the desktop object.
 * A NTSTATUS failure code is returned otherwise.
 */
NTSTATUS
NTAPI
IntAssignDesktopSecurityOnParse(
    _In_ PWINSTATION_OBJECT WinSta,
    _In_ PDESKTOP Desktop,
    _In_ PACCESS_STATE AccessState)
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR CapturedDescriptor;
    BOOLEAN MemoryAllocated;

    /*
     * Capture the security descriptor from
     * the window station. The window station
     * in question has a descriptor that is
     * inheritable and contains desktop access
     * rights as well.
     */
    Status = ObGetObjectSecurity(WinSta,
                                 &CapturedDescriptor,
                                 &MemoryAllocated);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntAssignDesktopSecurityOnParse(): Failed to capture the security descriptor from window station (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Assign new security to the desktop */
    Status = ObAssignSecurity(AccessState,
                              CapturedDescriptor,
                              Desktop,
                              ExDesktopObjectType);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntAssignDesktopSecurityOnParse(): Failed to assign security information to the desktop object (Status 0x%08lx)\n", Status);
    }

    /* Release the descriptor that we have captured */
    ObReleaseObjectSecurity(CapturedDescriptor, MemoryAllocated);
    return Status;
}

/**
 * @brief
 * Creates a security descriptor for the service.
 *
 * @param[out] ServiceSd
 * A pointer to a newly allocated and created security
 * descriptor for the service.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully
 * queried created the security descriptor. STATUS_NO_MEMORY
 * is returned if memory allocation for security buffers because
 * of a lack of needed pages to reserve for such allocation. A
 * failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
IntCreateServiceSecurity(
    _Out_ PSECURITY_DESCRIPTOR *ServiceSd)
{
    NTSTATUS Status;
    PACL ServiceDacl;
    ULONG DaclSize;
    ULONG RelSDSize;
    SECURITY_DESCRIPTOR AbsSD;
    PSECURITY_DESCRIPTOR RelSD;
    PTOKEN_USER TokenUser;

    /* Initialize our local variables */
    RelSDSize = 0;
    TokenUser = NULL;
    RelSD = NULL;
    ServiceDacl = NULL;

    /* Query the logged in user of the current security context (aka token) */
    Status = IntQueryUserSecurityIdentification(&TokenUser);
    if (!TokenUser)
    {
        ERR("IntCreateServiceSecurity(): Failed to query the token user (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Initialize the absolute security descriptor */
    Status = RtlCreateSecurityDescriptor(&AbsSD, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to initialize absolute SD (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * Build up the size of access control
     * list (the DACL) necessary to initialize
     * our ACL. The first two entry members
     * of ACL field are the authenticated user
     * that is associated with the security
     * context of the token. Then here come
     * the last two entries which are admins.
     * Why the ACL contains two ACEs of the
     * same SID is because of service access
     * rights and ACE inheritance.
     *
     * A service is composed of a default
     * desktop and window station upon
     * booting the system. On Windows connection
     * to such service is being made if no
     * default window station and desktop handles
     * were created before. The desktop and winsta
     * objects grant access on a separate type basis.
     * The user is granted full access to the window
     * station first and then full access to the desktop.
     * After that admins are granted specific rights
     * separately, just like the user. Ultimately the
     * ACEs that handle desktop rights management are
     * inherited to the default desktop object so
     * that there's no need to have a separate security
     * descriptor for the desktop object alone.
     */
    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(TokenUser->User.Sid) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(TokenUser->User.Sid) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeExports->SeAliasAdminsSid) +
               sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(SeExports->SeAliasAdminsSid);

    /* Allocate memory for service DACL */
    ServiceDacl = IntAllocateSecurityBuffer(DaclSize);
    if (!ServiceDacl)
    {
        ERR("IntCreateServiceSecurity(): Failed to allocate memory for service DACL!\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    /* Now create the DACL */
    Status = RtlCreateAcl(ServiceDacl,
                          DaclSize,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to create service DACL (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * The authenticated user is the ultimate and absolute
     * king in charge of the created (or opened, whatever that is)
     * window station object.
     */
    Status = RtlAddAccessAllowedAceEx(ServiceDacl,
                                      ACL_REVISION,
                                      0,
                                      WINSTA_ACCESS_ALL,
                                      TokenUser->User.Sid);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to set up window station ACE for authenticated user (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * The authenticated user also has the ultimate power
     * over the desktop object as well. This ACE cannot
     * be propagated but inherited. See the comment
     * above regarding ACL size for further explanation.
     */
    Status = RtlAddAccessAllowedAceEx(ServiceDacl,
                                      ACL_REVISION,
                                      INHERIT_ONLY_ACE | NO_PROPAGATE_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                      DESKTOP_ALL_ACCESS,
                                      TokenUser->User.Sid);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to set up desktop ACE for authenticated user (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * Administrators can only enumerate window
     * stations within a desktop.
     */
    Status = RtlAddAccessAllowedAceEx(ServiceDacl,
                                      ACL_REVISION,
                                      0,
                                      WINSTA_ENUMERATE,
                                      SeExports->SeAliasAdminsSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to set up window station ACE for admins (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * Administrators have some share of power over
     * the desktop object. They can enumerate desktops,
     * write and read upon the object itself.
     */
    Status = RtlAddAccessAllowedAceEx(ServiceDacl,
                                      ACL_REVISION,
                                      INHERIT_ONLY_ACE | NO_PROPAGATE_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                      DESKTOP_ENUMERATE | DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS,
                                      SeExports->SeAliasAdminsSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to set up desktop ACE for admins (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Set the DACL to absolute SD */
    Status = RtlSetDaclSecurityDescriptor(&AbsSD,
                                          TRUE,
                                          ServiceDacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to set up service DACL to absolute SD (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* This descriptor is ownerless */
    Status = RtlSetOwnerSecurityDescriptor(&AbsSD,
                                           NULL,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to make the absolute SD as ownerless (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* This descriptor has no primary group */
    Status = RtlSetGroupSecurityDescriptor(&AbsSD,
                                           NULL,
                                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to make the absolute SD as having no primary group (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /*
     * Determine how much size is needed to allocate
     * memory space for our relative security descriptor.
     */
    Status = RtlAbsoluteToSelfRelativeSD(&AbsSD,
                                         NULL,
                                         &RelSDSize);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        ERR("IntCreateServiceSecurity(): Unexpected status code, must be STATUS_BUFFER_TOO_SMALL (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Allocate memory for this */
    RelSD = IntAllocateSecurityBuffer(RelSDSize);
    if (!RelSD)
    {
        ERR("IntCreateServiceSecurity(): Failed to allocate memory pool for relative SD!\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    /* Convert the absolute SD into a relative one now */
    Status = RtlAbsoluteToSelfRelativeSD(&AbsSD,
                                         RelSD,
                                         &RelSDSize);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntCreateServiceSecurity(): Failed to convert absolute SD to a relative one (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* All good, give the SD to the caller */
    *ServiceSd = RelSD;

Quit:
    if (ServiceDacl)
    {
        IntFreeSecurityBuffer(ServiceDacl);
    }

    if (TokenUser)
    {
        IntFreeSecurityBuffer(TokenUser);
    }

    if (!NT_SUCCESS(Status))
    {
        if (RelSD)
        {
            IntFreeSecurityBuffer(RelSD);
        }
    }

    return Status;
}

/* EOF */
