/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security infrastructure of NTUSER component of Win32k
 * COPYRIGHT:       Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

//
// USER objects security rights
//

/* Desktop access rights */
#define DESKTOP_READ (STANDARD_RIGHTS_READ      | \
                      DESKTOP_ENUMERATE         | \
                      DESKTOP_READOBJECTS)

#define DESKTOP_WRITE (STANDARD_RIGHTS_WRITE    | \
                       DESKTOP_CREATEMENU       | \
                       DESKTOP_CREATEWINDOW     | \
                       DESKTOP_HOOKCONTROL      | \
                       DESKTOP_JOURNALPLAYBACK  | \
                       DESKTOP_JOURNALRECORD    | \
                       DESKTOP_WRITEOBJECTS)

#define DESKTOP_EXECUTE (STANDARD_RIGHTS_EXECUTE  | \
                         DESKTOP_SWITCHDESKTOP)

#define DESKTOP_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
                            DESKTOP_CREATEMENU       | \
                            DESKTOP_CREATEWINDOW     | \
                            DESKTOP_ENUMERATE        | \
                            DESKTOP_HOOKCONTROL      | \
                            DESKTOP_JOURNALPLAYBACK  | \
                            DESKTOP_JOURNALRECORD    | \
                            DESKTOP_READOBJECTS      | \
                            DESKTOP_SWITCHDESKTOP    | \
                            DESKTOP_WRITEOBJECTS)

/* Window Station access rights */
#define WINSTA_READ (STANDARD_RIGHTS_READ     | \
                     WINSTA_ENUMDESKTOPS      | \
                     WINSTA_ENUMERATE         | \
                     WINSTA_READATTRIBUTES    | \
                     WINSTA_READSCREEN)

#define WINSTA_WRITE (STANDARD_RIGHTS_WRITE    | \
                      WINSTA_ACCESSCLIPBOARD   | \
                      WINSTA_CREATEDESKTOP     | \
                      WINSTA_WRITEATTRIBUTES)

#define WINSTA_EXECUTE (STANDARD_RIGHTS_EXECUTE  | \
                        WINSTA_ACCESSGLOBALATOMS | \
                        WINSTA_EXITWINDOWS)

#define WINSTA_ACCESS_ALL (STANDARD_RIGHTS_REQUIRED | \
                           WINSTA_ACCESSCLIPBOARD   | \
                           WINSTA_ACCESSGLOBALATOMS | \
                           WINSTA_CREATEDESKTOP     | \
                           WINSTA_ENUMDESKTOPS      | \
                           WINSTA_ENUMERATE         | \
                           WINSTA_EXITWINDOWS       | \
                           WINSTA_READATTRIBUTES    | \
                           WINSTA_READSCREEN        | \
                           WINSTA_WRITEATTRIBUTES)

//
// Function prototypes
//

HANDLE
IntCaptureCurrentAccessToken(VOID);

PVOID
IntAllocateSecurityBuffer(
    _In_ SIZE_T Length);

VOID
IntFreeSecurityBuffer(
    _In_ PVOID Buffer);

NTSTATUS
IntQueryUserSecurityIdentification(
    _Out_ PTOKEN_USER *User);

NTSTATUS
NTAPI
IntAssignDesktopSecurityOnParse(
    _In_ PWINSTATION_OBJECT WinSta,
    _In_ PDESKTOP Desktop,
    _In_ PACCESS_STATE AccessState);

NTSTATUS
NTAPI
IntCreateServiceSecurity(
    _Out_ PSECURITY_DESCRIPTOR *ServiceSd);

/* EOF */
