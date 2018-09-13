/****************************** Module Header ******************************\
* Module Name: profile.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains code to emulate ini file mapping.
*
* History:
* 30-Nov-1993 SanfordS  Created.
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* aFastRegMap[]
*
* This array maps section ids (PMAP_) to cached registry keys and section
* addresses within the registry.  IF INI FILE MAPPING CHANGES ARE MADE,
* THIS TABLE MUST BE UPDATED.
*
* The first character of the szSection field indicates what root the
* section is in. (or locked open status)
*      M = LocalMachine
*      U = CurrentUser
*      L = Locked open - used only on M mappings.
*
* History:
\***************************************************************************/
#define PROOT_CPANEL     0
#define PROOT_ACCESS     1
#define PROOT_CURRENTM   2
#define PROOT_CURRENTU   3
#define PROOT_CONTROL    4
#define PROOT_SERVICES   5
#define PROOT_KEYBOARD   6

typedef struct tagFASTREGMAP {
    UINT idRoot;
    PCWSTR szSection;
} FASTREGMAP, *PFASTREGMAP;

CONST PCWSTR aFastRegRoot[] = {
    L"UControl Panel\\",                                    // PROOT_CPANEL
    L"UControl Panel\\Accessibility\\",                     // PROOT_ACCESS
    L"MSoftware\\Microsoft\\Windows NT\\CurrentVersion\\",  // PROOT_CURRENTM
    L"USoftware\\Microsoft\\Windows NT\\CurrentVersion\\",  // PROOT_CURRENTU
    L"MSystem\\CurrentControlSet\\Control\\",               // PROOT_CONTROL
    L"MSystem\\CurrentControlSet\\Services\\",              // PROOT_SERVICES
    L"UKeyboard Layout\\",                                  // PROOT_KEYBOARD
};

CONST FASTREGMAP aFastRegMap[PMAP_LAST + 1] = {
    { PROOT_CPANEL,   L"Colors" },                          // PMAP_COLORS
    { PROOT_CPANEL,   L"Cursors" },                         // PMAP_CURSORS
    { PROOT_CURRENTM, L"Windows" },                         // PMAP_WINDOWSM
    { PROOT_CURRENTU, L"Windows" },                         // PMAP_WINDOWSU
    { PROOT_CPANEL,   L"Desktop" },                         // PMAP_DESKTOP
    { PROOT_CPANEL,   L"Icons" },                           // PMAP_ICONS
    { PROOT_CURRENTM, L"Fonts" },                           // PMAP_FONTS
    { PROOT_CURRENTU, L"TrueType" },                        // PMAP_TRUETYPE
    { PROOT_CONTROL,  L"Keyboard Layout" },                 // PMAP_KBDLAYOUT
    { PROOT_SERVICES, L"RIT" },                             // PMAP_INPUT
    { PROOT_CURRENTM, L"Compatibility" },                   // PMAP_COMPAT
    { PROOT_CONTROL,  L"Session Manager\\SubSystems" },     // PMAP_SUBSYSTEMS
    { PROOT_CPANEL,   L"Sound" },                           // PMAP_BEEP
    { PROOT_CPANEL,   L"Mouse" },                           // PMAP_MOUSE
    { PROOT_CPANEL,   L"Keyboard" },                        // PMAP_KEYBOARD
    { PROOT_ACCESS,   L"StickyKeys" },                      // PMAP_STICKYKEYS
    { PROOT_ACCESS,   L"Keyboard Response" },               // PMAP_KEYBOARDRESPONSE
    { PROOT_ACCESS,   L"MouseKeys" },                       // PMAP_MOUSEKEYS
    { PROOT_ACCESS,   L"ToggleKeys" },                      // PMAP_TOGGLEKEYS
    { PROOT_ACCESS,   L"TimeOut" },                         // PMAP_TIMEOUT
    { PROOT_ACCESS,   L"SoundSentry" },                     // PMAP_SOUNDSENTRY
    { PROOT_ACCESS,   L"ShowSounds" },                      // PMAP_SHOWSOUNDS
    { PROOT_CURRENTM, L"AeDebug" },                         // PMAP_AEDEBUG
    { PROOT_CONTROL,  L"NetworkProvider" },                 // PMAP_NETWORK
    { PROOT_CPANEL,   L"Desktop\\WindowMetrics" },          // PMAP_METRICS
    { PROOT_KEYBOARD, L"" },                                // PMAP_UKBDLAYOUT
    { PROOT_KEYBOARD, L"Toggle" },                          // PMAP_UKBDLAYOUTTOGGLE
    { PROOT_CURRENTM, L"Winlogon" },                        // PMAP_WINLOGON
    { PROOT_ACCESS,   L"Keyboard Preference" },             // PMAP_KEYBOARDPREF
    { PROOT_ACCESS,   L"Blind Access" },                    // PMAP_SCREENREADER
    { PROOT_ACCESS,   L"HighContrast" },                    // PMAP_HIGHCONTRAST
    { PROOT_CURRENTM, L"IME Compatibility" },               // PMAP_IMECOMPAT
    { PROOT_CURRENTM, L"IMM" },                             // PMAP_IMM
    { PROOT_CONTROL,  L"Session Manager\\SubSystems\\Pool" },// PMAP_POOLLIMITS
    { PROOT_CURRENTM, L"Compatibility32" },                  // PMAP_COMPAT32
    { PROOT_CURRENTM, L"WOW\\SetupPrograms" },               // PMAP_SETUPPROGRAMNAMES
    { PROOT_CPANEL,   L"Input Method" },                     // PMAP_INPUTMETHOD
    { PROOT_CURRENTM, L"Compatibility2" },                   // PMAP_COMPAT2
    { PROOT_SERVICES, L"Mouclass\\Parameters" },             // PMAP_MOUCLASS_PARAMS
    { PROOT_SERVICES, L"Kbdclass\\Parameters" },             // PMAP_KBDCLASS_PARAMS
};

DWORD gdwPolicyFlags = POLICY_ALL;
WCHAR PreviousUserStringBuf[256];
UNICODE_STRING PreviousUserString = {0, sizeof PreviousUserStringBuf, PreviousUserStringBuf};
LUID luidPrevious;

CONST WCHAR wszDefaultUser[] = L"\\Registry\\User\\.Default";
UNICODE_STRING DefaultUserString = {sizeof wszDefaultUser - sizeof(WCHAR), sizeof wszDefaultUser, (WCHAR *)wszDefaultUser};

void InitPreviousUserString(void) {
    UNICODE_STRING UserString;
    LUID           luidCaller;

    CheckCritIn();

    /*
     * Speed hack, check if luid of this process == system or previous to
     * save work.
     */
    if (NT_SUCCESS(GetProcessLuid(NULL, &luidCaller))) {

        if (RtlEqualLuid(&luidCaller, &luidPrevious)) {
            return;   // same as last time - no work.
        }
        luidPrevious = luidCaller;

        if (RtlEqualLuid(&luidCaller, &luidSystem))
            goto DefaultUser;

    } else {
        luidPrevious = RtlConvertLongToLuid(0);
    }

    /*
     * Set up current user registry base string.
     */
    if (!NT_SUCCESS(RtlFormatCurrentUserKeyPath(&UserString))) {

DefaultUser:

        RtlCopyUnicodeString(&PreviousUserString, &DefaultUserString);

    } else {
        UserAssert(sizeof(PreviousUserStringBuf) >= UserString.Length + 4);
        RtlCopyUnicodeString(&PreviousUserString, &UserString);
        RtlFreeUnicodeString(&UserString);
    }

    RtlAppendUnicodeToString(&PreviousUserString, L"\\");

}

typedef struct tagPROFILEUSERNAME {
    WCHAR awcName[MAXPROFILEBUF];
    UNICODE_STRING NameString;
} PROFILEUSERNAME, *PPROFILEUSERNAME;

PUNICODE_STRING CreateProfileUserName(TL *ptl)
{
    PPROFILEUSERNAME pMapName;

    CheckCritIn();

    pMapName = UserAllocPoolWithQuota(sizeof (PROFILEUSERNAME), TAG_PROFILEUSERNAME);
    if (!pMapName) {
        RIPMSG0(RIP_WARNING, "CreateProfileUserName: Allocation failed");
        return NULL;
    }

    ThreadLockPool(PtiCurrent(), pMapName, ptl);
    pMapName->NameString.Length = 0;
    pMapName->NameString.MaximumLength = sizeof (pMapName->awcName);
    pMapName->NameString.Buffer = pMapName->awcName;

    InitPreviousUserString();

    RtlCopyUnicodeString(&pMapName->NameString, &PreviousUserString);
    return &(pMapName->NameString);
}

void FreeProfileUserName(PUNICODE_STRING pProfileUserName,TL *ptl) {
    UNREFERENCED_PARAMETER(ptl);
    CheckCritIn();
    if (pProfileUserName) {
        ThreadUnlockAndFreePool(PtiCurrent(), ptl);
    }
}

/*****************************************************************************\
* RemoteOpenCacheKeyEx
*
* History:
* 21-Jan-1998 CLupu  Ported from Citrix.
\*****************************************************************************/
HANDLE RemoteOpenCacheKeyEx(
    UINT        idSection,
    ACCESS_MASK amRequest)
{
    OBJECT_ATTRIBUTES OA;
    WCHAR             UnicodeStringBuf[512];
    UNICODE_STRING    UnicodeString;
    LONG              Status;
    HANDLE            hKey;

    CheckCritIn();

    UserAssert(gbRemoteSession == TRUE);

    if (aFastRegRoot[aFastRegMap[idSection].idRoot][0] == L'M')
        return NULL;

    if (gstrBaseWinStationName[0] == 0)
        return NULL;

    if (amRequest != KEY_READ)
        return NULL;

    UserAssert(idSection <= PMAP_LAST);
    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = sizeof(UnicodeStringBuf);
    UnicodeString.Buffer        = UnicodeStringBuf;

    RtlAppendUnicodeToString(&UnicodeString, L"\\Registry\\Machine\\");
    RtlAppendUnicodeToString(&UnicodeString, WINSTATION_REG_NAME);

    RtlAppendUnicodeToString(&UnicodeString, L"\\");
    RtlAppendUnicodeToString(&UnicodeString, gstrBaseWinStationName);
    RtlAppendUnicodeToString(&UnicodeString, L"\\");
    RtlAppendUnicodeToString(&UnicodeString, WIN_USEROVERRIDE);
    RtlAppendUnicodeToString(&UnicodeString, L"\\");

    Status = RtlAppendUnicodeToString(&UnicodeString,
                                      (PWSTR)(&aFastRegRoot[aFastRegMap[idSection].idRoot][1]));
    UserAssert(NT_SUCCESS(Status));

    Status = RtlAppendUnicodeToString(&UnicodeString,
                                      (PWSTR)(aFastRegMap[idSection].szSection));
    UserAssert(NT_SUCCESS(Status));

    /*
     * Open the key for kernel mode access
     */
    InitializeObjectAttributes(&OA,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&hKey, amRequest, &OA);

    return (NT_SUCCESS(Status) ? hKey : NULL);
}

/*****************************************************************************\
* OpenCacheKeyEx
*
* Attempts to open a cached key for a given section.  If we are calling
* for a client thread, we must check the access rights for the key after
* opening it.
*
* Returns fSuccess.
*
* Note -- param 1 can be NULL.  If the section name is a per-user registry
*         section, we ill use the first parameter if available or set up
*         and use the cached one if the first parameter is NULL.
*
* History:
* 03-Dec-1993 SanfordS  Created.
\*****************************************************************************/
HANDLE OpenCacheKeyEx(
    PUNICODE_STRING pMapName OPTIONAL,
    UINT        idSection,
    ACCESS_MASK amRequest,
    PDWORD      pdwPolicyFlags
    )
{
    OBJECT_ATTRIBUTES OA;
    WCHAR             UnicodeStringBuf[256];
    UNICODE_STRING    UnicodeString;
    LONG              Status;
    HANDLE            hKey = NULL;
    PEPROCESS         peCurrent = PsGetCurrentProcess();
    DWORD             dwPolicyFlags;

    CheckCritIn();

    UserAssert(idSection <= PMAP_LAST);

    /*
     * If we're opening the desktop key for read access, we should be checking
     * for relevant policy.
     */
    if (idSection == PMAP_DESKTOP && amRequest == KEY_READ && pdwPolicyFlags) {
        UserAssert(!(*pdwPolicyFlags & ~POLICY_ALL));
        dwPolicyFlags = *pdwPolicyFlags;
    } else {
        dwPolicyFlags = POLICY_NONE;
    }

TryAgain:

    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = sizeof(UnicodeStringBuf);
    UnicodeString.Buffer        = UnicodeStringBuf;


    if (dwPolicyFlags & POLICY_MACHINE) {
        dwPolicyFlags &= ~POLICY_MACHINE;
        RtlAppendUnicodeToString(&UnicodeString,
                                 L"\\Registry\\Machine\\");
        RtlAppendUnicodeToString(&UnicodeString,
                                 L"Software\\Policies\\Microsoft\\Windows\\");
    } else {
        if (aFastRegRoot[aFastRegMap[idSection].idRoot][0] == L'M') {
            RtlAppendUnicodeToString(&UnicodeString, L"\\Registry\\Machine\\");
        } else {
            if (!pMapName) {
                InitPreviousUserString();
                RtlAppendUnicodeStringToString(
                    &UnicodeString,
                    &PreviousUserString);
            } else {
                RtlAppendUnicodeStringToString(
                    &UnicodeString,
                    pMapName);
            }
        }
        if (dwPolicyFlags & POLICY_USER) {
            dwPolicyFlags &= ~POLICY_USER;
            RtlAppendUnicodeToString(&UnicodeString,
                                     L"Software\\Policies\\Microsoft\\Windows\\");
        } else {
            dwPolicyFlags &= ~POLICY_NONE;
        }

    }

    RtlAppendUnicodeToString(&UnicodeString,
                             (PWSTR)&aFastRegRoot[aFastRegMap[idSection].idRoot][1]);

    RtlAppendUnicodeToString(&UnicodeString,
                             (PWSTR)aFastRegMap[idSection].szSection);


    /*
     * Open the key for kernel mode access
     */
    InitializeObjectAttributes(&OA,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&hKey, amRequest, &OA);

    if (
        (amRequest == KEY_READ)   ||     /*
                                          * We must be able to read
                                          * our registry settings.
                                          */
        (peCurrent == gpepCSRSS)  ||
        (peCurrent == gpepInit)
             ) {

    } else {
        /*
         * Now check if the user has access to the key
         */

        if (NT_SUCCESS(Status)) {
            PVOID pKey;
            NTSTATUS Status2;
            Status2 = ObReferenceObjectByHandle(hKey,
                                        amRequest,
                                        NULL,
                                        KernelMode,
                                        &pKey,
                                        NULL);

            if (NT_SUCCESS(Status2)) {
                if (!AccessCheckObject(pKey, amRequest, UserMode, &KeyMapping)) {
                    ZwClose(hKey);
                    Status = STATUS_ACCESS_DENIED;
                }
                ObDereferenceObject(pKey);
            } else {
                ZwClose(hKey);
                Status = STATUS_ACCESS_DENIED;
            }
        }

    }

#if DBG
    if (!NT_SUCCESS(Status)) {
        UnicodeStringBuf[UnicodeString.Length / 2] = 0;

        if (PsGetCurrentProcess()->UniqueProcessId != gpidLogon) {
            RIPMSG1(RIP_WARNING | RIP_THERESMORE, "OpenCacheKeyEx failed with Status = %lx key:", Status);
            RIPMSG1(RIP_WARNING | RIP_THERESMORE | RIP_NONAME | RIP_NONEWLINE, " %ws\\", UnicodeStringBuf);
        }
    }
#endif DBG

    /*
     * If we didn't succeed and we're not down to bottom of policy chain, try again.
     */
    if (!NT_SUCCESS(Status) && dwPolicyFlags) {
        goto TryAgain;
    }

    /*
     * Update policy level
     */
    if (pdwPolicyFlags) {
        *pdwPolicyFlags = dwPolicyFlags;
    }

    return (NT_SUCCESS(Status) ? hKey : NULL);
}


/*****************************************************************************\
* CheckDesktopPolicy
*
* Check if a desktop value has an associated policy.
*
* Returns TRUE if there is a policy, FALSE otherwise.
*
* History:
* 07-Feb-2000 JerrySh   Created.
\*****************************************************************************/
BOOL CheckDesktopPolicy(
    PUNICODE_STRING pProfileUserName OPTIONAL,
    PCWSTR      lpKeyName
    )
{
    WCHAR          szKey[80];
    HANDLE         hKey;
    DWORD          cbSize;
    NTSTATUS       Status;
    UNICODE_STRING UnicodeString;
    KEY_VALUE_BASIC_INFORMATION  KeyInfo;
    DWORD          dwPolicyFlags = gdwPolicyFlags & (POLICY_MACHINE | POLICY_USER);

    /*
     * If there is no policy or the caller is winlogon, let it go.
     */
    if (!dwPolicyFlags || GetCurrentProcessId() == gpidLogon) {
        return FALSE;
    }

    /*
     * Convert the ID to a string if we need to.
     */
    if (!IS_PTR(lpKeyName)) {
        ServerLoadString(hModuleWin, PTR_TO_ID(lpKeyName), szKey, ARRAY_SIZE(szKey));
        lpKeyName = szKey;
    }

TryAgain:

    /*
     * Try to open a key.
     */
    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               PMAP_DESKTOP,
                               KEY_READ,
                               &dwPolicyFlags)) == NULL) {
        return FALSE;
    }

    /*
     * See if the value exists.
     */
    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = ZwQueryValueKey(hKey,
                             &UnicodeString,
                             KeyValueBasicInformation,
                             &KeyInfo,
                             sizeof(KeyInfo),
                             &cbSize);

    ZwClose(hKey);

    if (!NT_ERROR(Status)) {
        return TRUE;
    } else if (dwPolicyFlags) {
        goto TryAgain;
    } else {
        return FALSE;
    }
}


/*****************************************************************************\
* CheckDesktopPolicyChange
*
* Check if policy has changed since last time we checked.
*
* Returns TRUE if policy changed, FASLE otherwise.
*
* History:
* 07-Feb-2000 JerrySh   Created.
\*****************************************************************************/
BOOL CheckDesktopPolicyChange(
    PUNICODE_STRING pProfileUserName OPTIONAL
    )
{
    static LARGE_INTEGER  LastMachineWriteTime;
    static LARGE_INTEGER  LastUserWriteTime;
    KEY_BASIC_INFORMATION KeyInfo;
    BOOL                  bPolicyChanged = FALSE;
    HANDLE                hKey;
    DWORD                 cbSize;
    DWORD                 dwPolicyFlags;

    /*
     * Check if machine policy has changed since last time we checked.
     */
    dwPolicyFlags = POLICY_MACHINE;
    KeyInfo.LastWriteTime.QuadPart = 0;
    hKey = OpenCacheKeyEx(pProfileUserName,
                          PMAP_DESKTOP,
                          KEY_READ,
                          &dwPolicyFlags);
    if (hKey) {
        ZwQueryKey(hKey,
                   KeyValueBasicInformation,
                   &KeyInfo,
                   sizeof(KeyInfo),
                   &cbSize);
        ZwClose(hKey);
        gdwPolicyFlags |= POLICY_MACHINE;
    } else {
        gdwPolicyFlags &= ~POLICY_MACHINE;
    }
    if (LastMachineWriteTime.QuadPart != KeyInfo.LastWriteTime.QuadPart) {
        LastMachineWriteTime.QuadPart = KeyInfo.LastWriteTime.QuadPart;
        bPolicyChanged = TRUE;
    }

    /*
     * Check if user policy has changed since last time we checked.
     */
    dwPolicyFlags = POLICY_USER;
    KeyInfo.LastWriteTime.QuadPart = 0;
    hKey = OpenCacheKeyEx(pProfileUserName,
                          PMAP_DESKTOP,
                          KEY_READ,
                          &dwPolicyFlags);
    if (hKey) {
        ZwQueryKey(hKey,
                   KeyValueBasicInformation,
                   &KeyInfo,
                   sizeof(KeyInfo),
                   &cbSize);
        ZwClose(hKey);
        gdwPolicyFlags |= POLICY_USER;
    } else {
        gdwPolicyFlags &= ~POLICY_USER;
    }
    if (LastUserWriteTime.QuadPart != KeyInfo.LastWriteTime.QuadPart) {
        LastUserWriteTime.QuadPart = KeyInfo.LastWriteTime.QuadPart;
        bPolicyChanged = TRUE;
    }

    return bPolicyChanged;
}


/*****************************************************************************\
* FastGetProfileDwordW
*
* Reads a REG_DWORD type key from the registry.
*
* returns value read or default value on failure.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
DWORD FastGetProfileDwordW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    DWORD   dwDefault
    )
{
    HANDLE         hKey;
    DWORD          cbSize;
    DWORD          dwRet;
    LONG           Status;
    UNICODE_STRING UnicodeString;
    BYTE           Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];

    UserAssert(idSection <= PMAP_LAST);

    if (gbRemoteSession) {

        hKey = RemoteOpenCacheKeyEx(idSection, KEY_READ);
        if (hKey != NULL) {
            goto Override;
        }
    }

    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_READ,
                               NULL)) == NULL) {
        RIPMSG1(RIP_WARNING | RIP_NONAME, "%ws", lpKeyName);
        return dwDefault;
    }

Override:

    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = ZwQueryValueKey(hKey,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
                             sizeof(Buf),
                             &cbSize);

    dwRet = dwDefault;

    if (NT_SUCCESS(Status)) {

        dwRet = *((PDWORD)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);

    } else if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

        RIPMSG1(RIP_WARNING,
                "FastGetProfileDwordW: ObjectName not found: %ws",
                lpKeyName);
    }

    ZwClose(hKey);

    return dwRet;
}

/*****************************************************************************\
* FastGetProfileKeysW()
*
* Reads all key names in the given section.
*
* History:
* 15-Dec-1994 JimA      Created.
\*****************************************************************************/
DWORD FastGetProfileKeysW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpDefault,
    LPWSTR  *lpReturnedString
    )
{
    HANDLE                       hKey;
    DWORD                        cchSize;
    DWORD                        cchKey;
    LONG                         Status;
    WCHAR                        Buffer[256 + 6];
    PKEY_VALUE_BASIC_INFORMATION pKeyInfo;
    ULONG                        iValue;
    LPWSTR                       lpTmp;
    LPWSTR                       lpKeys = NULL;
    DWORD                        dwPoolSize;

    UserAssert(idSection <= PMAP_LAST);

    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_READ,
                               NULL)) == NULL) {
        RIPMSG0(RIP_WARNING | RIP_NONAME, "");
        goto DefExit;
    }

    pKeyInfo          = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
    cchSize           = 0;
    *lpReturnedString = NULL;
    iValue            = 0;

    while (TRUE) {

#if DBG
        wcscpy(Buffer + 256, L"DON'T");
#endif
        Status = ZwEnumerateValueKey(hKey,
                                     iValue,
                                     KeyValueBasicInformation,
                                     pKeyInfo,
                                     sizeof(Buffer),
                                     &cchKey);

        UserAssert(_wcsicmp(Buffer + 256, L"DON'T") == 0);

        if (Status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(Status)) {

            if (lpKeys) {
                UserFreePool(lpKeys);
                lpKeys = NULL;
            }
            goto DefExit;
        }

        UserAssert(pKeyInfo->NameLength * sizeof(WCHAR) <=
                   sizeof(Buffer) - sizeof(KEY_VALUE_BASIC_INFORMATION));

        UserAssert(cchKey <= sizeof(Buffer));

        /*
         * A key was found.  Allocate space for it.  Note that
         * NameLength is in bytes.
         */
        cchKey   = cchSize;
        cchSize += pKeyInfo->NameLength + sizeof(WCHAR);

        if (lpKeys == NULL) {

            dwPoolSize = cchSize + sizeof(WCHAR);
            lpKeys = UserAllocPoolWithQuota(dwPoolSize, TAG_PROFILE);

        } else {

            lpTmp = lpKeys;
            lpKeys = UserReAllocPoolWithQuota(lpTmp,
                                              dwPoolSize,
                                              cchSize + sizeof(WCHAR),
                                              TAG_PROFILE);

            /*
             * Free the original buffer if the allocation fails
             */
            if (lpKeys == NULL) {
                UserFreePool(lpTmp);
            }
            dwPoolSize = cchSize + sizeof(WCHAR);
        }

        /*
         * Check for out of memory.
         */
        if (lpKeys == NULL)
            goto DefExit;

        /*
         * NULL terminate the string and append it to
         * the key list.
         */
        UserAssert(pKeyInfo->NameLength < sizeof(Buffer) - sizeof(KEY_VALUE_BASIC_INFORMATION));

        RtlCopyMemory(&lpKeys[cchKey / sizeof(WCHAR)], pKeyInfo->Name, pKeyInfo->NameLength);
        lpKeys[(cchKey + pKeyInfo->NameLength) / sizeof(WCHAR)] = 0;

        iValue++;
    }

    /*
     * If no keys were found, return the default.
     */
    if (iValue == 0) {

DefExit:

        cchSize = wcslen(lpDefault)+1;
        lpKeys  = UserAllocPoolWithQuota((cchSize+1) * sizeof(WCHAR), TAG_PROFILE);

        if (lpKeys)
            wcscpy(lpKeys, lpDefault);
        else
            cchSize = 0;

    } else {

        /*
         * Turn the byte count into a char count.
         */
        cchSize /= sizeof(WCHAR);
    }

    /*
     * Make sure hKey is closed.
     */
    if (hKey)
        ZwClose(hKey);

    /*
     * Append the ending NULL.
     */
    if (lpKeys)
        lpKeys[cchSize] = 0;

    *lpReturnedString = lpKeys;

    return cchSize;
}

/*****************************************************************************\
* FastGetProfileStringW()
*
* Implements a fast version of the standard API using predefined registry
* section indecies (PMAP_) that reference lazy-opened, cached registry
* handles.  FastCloseProfileUserMapping() should be called to clean up
* cached entries when fast profile calls are completed.
*
* This api does NOT implement the NULL lpKeyName feature of the real API.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
DWORD FastGetProfileStringW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   cchBuf
    )
{
    HANDLE                         hKey = NULL;
    DWORD                          cbSize;
    LONG                           Status;
    UNICODE_STRING                 UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    BOOL                           bRemoteOverride = FALSE;
    DWORD                          dwPolicyFlags = gdwPolicyFlags;


    UserAssert(idSection <= PMAP_LAST);
    UserAssert(lpKeyName != NULL);

    if (gbRemoteSession) {
        hKey = RemoteOpenCacheKeyEx(idSection, KEY_READ);
        if (hKey != NULL) {
            bRemoteOverride = TRUE;
            goto Override;
        }
    }

TryAgain:
    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_READ,
                               &dwPolicyFlags)) == NULL) {

#if DBG
        if (PsGetCurrentProcess()->UniqueProcessId != gpidLogon) {
            RIPMSG1(RIP_WARNING | RIP_NONAME, "%ws", lpKeyName);
        }
#endif
        goto DefExit;
    }

Override:

    cbSize = (cchBuf * sizeof(WCHAR)) +
            FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    if ((pKeyInfo = UserAllocPoolWithQuota(cbSize, TAG_PROFILE)) == NULL)
        goto DefExit;

    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = ZwQueryValueKey(hKey,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             pKeyInfo,
                             cbSize,
                             &cbSize);

    if (Status == STATUS_BUFFER_OVERFLOW) {
        RIPMSG0(RIP_WARNING, "FastGetProfileStringW: Buffer overflow");
        Status = STATUS_SUCCESS;
    }

    UserAssert(NT_SUCCESS(Status) || (Status == STATUS_OBJECT_NAME_NOT_FOUND));

    if (NT_SUCCESS(Status)) {

        if (pKeyInfo->DataLength >= sizeof(WCHAR)) {

            ((LPWSTR)(pKeyInfo->Data))[cchBuf - 1] = L'\0';
            wcscpy(lpReturnedString, (LPWSTR)pKeyInfo->Data);

        } else {
            /*
             * Appears to be a bug with empty strings - only first
             * byte is set to NULL. (SAS)
             */
            lpReturnedString[0] = TEXT('\0');
        }

        cchBuf = pKeyInfo->DataLength;

        UserFreePool(pKeyInfo);

        ZwClose(hKey);

        /*
         * data length includes terminating zero [bodind]
         */
        return (cchBuf / sizeof(WCHAR));

    } else if (bRemoteOverride) {
        bRemoteOverride = FALSE;
        UserFreePool(pKeyInfo);
        ZwClose(hKey);
        hKey = NULL;
        goto TryAgain;

    } else if (dwPolicyFlags) {
        UserFreePool(pKeyInfo);
        ZwClose(hKey);
        goto TryAgain;
    }

    UserFreePool(pKeyInfo);

DefExit:

    /*
     * Make sure the key is closed.
     */
    if (hKey)
        ZwClose(hKey);

    /*
     * wcscopy copies terminating zero, but the length returned by
     * wcslen does not, so add 1 to be consistent with success
     * return [bodind]
     */
    if (lpDefault != NULL) {
        cchBuf = wcslen(lpDefault) + 1;
        RtlCopyMemory(lpReturnedString, lpDefault, cchBuf * sizeof(WCHAR));
        return cchBuf;
    }

    return 0;
}

/*****************************************************************************\
* FastGetProfileIntW()
*
* Implements a fast version of the standard API using predefined registry
* section indecies (PMAP_) that reference lazy-opened, cached registry
* handles.  FastCloseProfileUserMapping() should be called to clean up
* cached entries when fast profile calls are completed.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
UINT FastGetProfileIntW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    UINT    nDefault
    )
{
    WCHAR          ValueBuf[40];
    UNICODE_STRING Value;
    UINT           ReturnValue;

    UserAssert(idSection <= PMAP_LAST);

    if (!FastGetProfileStringW(pProfileUserName,
                               idSection,
                               lpKeyName,
                               NULL,
                               ValueBuf,
                               sizeof(ValueBuf) / sizeof(WCHAR)
                               )) {

        return nDefault;
    }

    /*
     * Convert string to int.
     */
    RtlInitUnicodeString(&Value, ValueBuf);
    RtlUnicodeStringToInteger(&Value, 10, &ReturnValue);

    return ReturnValue;
}

/*****************************************************************************\
* FastWriteProfileStringW
*
* Implements a fast version of the standard API using predefined registry
* section indecies (PMAP_) that reference lazy-opened, cached registry
* handles.  FastCloseProfileUserMapping() should be called to clean up
* cached entries when fast profile calls are completed.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
BOOL FastWriteProfileStringW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    LPCWSTR lpString
    )
{
    HANDLE         hKey;
    LONG           Status;
    UNICODE_STRING UnicodeString;

    UserAssert(idSection <= PMAP_LAST);

    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_WRITE,
                               NULL)) == NULL) {
        RIPMSG1(RIP_WARNING | RIP_NONAME, "%ws", lpKeyName);
        return FALSE;
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);
    Status = ZwSetValueKey(hKey,
                           &UnicodeString,
                           0,
                           REG_SZ,
                           (PVOID)lpString,
                           (wcslen(lpString) + 1) * sizeof(WCHAR));

    ZwClose(hKey);

    return (NT_SUCCESS(Status));
}

/*****************************************************************************\
* FastGetProfileIntFromID
*
* Just like FastGetProfileIntW except it reads the USER string table for the
* key name.
*
* History:
* 02-Dec-1993 SanfordS  Created.
* 25-Feb-1995 BradG     Added TWIPS -> Pixel conversion.
\*****************************************************************************/
int FastGetProfileIntFromID(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT idSection,
    UINT idKey,
    int  def
    )
{
    int   result;
    WCHAR szKey[80];


    UserAssert(idSection <= PMAP_LAST);

    ServerLoadString(hModuleWin, idKey, szKey, ARRAY_SIZE(szKey));

    result = FastGetProfileIntW(pProfileUserName,idSection, szKey, def);

    /*
     * If you change the below list of STR_* make sure you make a
     * corresponding change in SetWindowMetricInt (rare.c)
     */
    switch (idKey) {
    case STR_BORDERWIDTH:
    case STR_SCROLLWIDTH:
    case STR_SCROLLHEIGHT:
    case STR_CAPTIONWIDTH:
    case STR_CAPTIONHEIGHT:
    case STR_SMCAPTIONWIDTH:
    case STR_SMCAPTIONHEIGHT:
    case STR_MENUWIDTH:
    case STR_MENUHEIGHT:
    case STR_ICONHORZSPACING:
    case STR_ICONVERTSPACING:
    case STR_MINWIDTH:
    case STR_MINHORZGAP:
    case STR_MINVERTGAP:
        /*
         * Convert any registry values stored in TWIPS back to pixels
         */
        if (result < 0)
            result = MultDiv(-result, gpsi->dmLogPixels, 72 * 20);
        break;
    }

    return result;
}

/*****************************************************************************\
* FastGetProfileIntFromID
*
* Just like FastGetProfileStringW except it reads the USER string table for
* the key name.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
DWORD FastGetProfileStringFromIDW(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    UINT    idKey,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   cch
    )
{
    WCHAR szKey[80];

    UserAssert(idSection <= PMAP_LAST);

    ServerLoadString(hModuleWin, idKey, szKey, ARRAY_SIZE(szKey));

    return FastGetProfileStringW(pProfileUserName,
                                 idSection,
                                 szKey,
                                 lpDefault,
                                 lpReturnedString,
                                 cch
                                 );
}

/*****************************************************************************\
* FastWriteProfileValue
*
* History:
* 06/10/96 GerardoB Renamed and added uType parameter
\*****************************************************************************/
BOOL FastWriteProfileValue(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    UINT    uType,
    LPBYTE  lpStruct,
    UINT    cbSizeStruct
    )
{
    HANDLE         hKey;
    LONG           Status;
    UNICODE_STRING UnicodeString;
    WCHAR          szKey[SERVERSTRINGMAXSIZE];

    UserAssert(idSection <= PMAP_LAST);

    if (!IS_PTR(lpKeyName)) {
        *szKey = (WCHAR)0;
        ServerLoadString(hModuleWin, PTR_TO_ID(lpKeyName), szKey, ARRAY_SIZE(szKey));
        UserAssert(*szKey != (WCHAR)0);
        lpKeyName = szKey;
    }

    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_WRITE,
                               NULL)) == NULL) {
        RIPMSG1(RIP_WARNING, "FastWriteProfileValue: Failed to open cache-key (%ws)", lpKeyName);
        return FALSE;
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);

    Status = ZwSetValueKey(hKey,
                           &UnicodeString,
                           0,
                           uType,
                           lpStruct,
                           cbSizeStruct);
    ZwClose(hKey);

#if DBG
    if (!NT_SUCCESS(Status)) {
        RIPMSG3 (RIP_WARNING, "FastWriteProfileValue: ZwSetValueKey Failed. Status:%#lx idSection:%#lx KeyName:%s",
                 Status, idSection, UnicodeString.Buffer);
    }
#endif

    return (NT_SUCCESS(Status));
}

/*****************************************************************************\
* FastGetProfileValue
*
* If cbSizeReturn is 0, just return the size of the data
*
* History:
* 06/10/96 GerardoB Renamed
\*****************************************************************************/
DWORD FastGetProfileValue(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT    idSection,
    LPCWSTR lpKeyName,
    LPBYTE  lpDefault,
    LPBYTE  lpReturn,
    UINT    cbSizeReturn
    )
{
    HANDLE                         hKey;
    UINT                           cbSize;
    LONG                           Status;
    UNICODE_STRING                 UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    WCHAR                          szKey[SERVERSTRINGMAXSIZE];
    KEY_VALUE_PARTIAL_INFORMATION  KeyInfo;

    UserAssert(idSection <= PMAP_LAST);

    if (!IS_PTR(lpKeyName)) {
        *szKey = (WCHAR)0;
        ServerLoadString(hModuleWin, PTR_TO_ID(lpKeyName), szKey, ARRAY_SIZE(szKey));
        UserAssert(*szKey != (WCHAR)0);
        lpKeyName = szKey;
    }

    if (gbRemoteSession) {
        hKey = RemoteOpenCacheKeyEx(idSection, KEY_READ);
        if (hKey != NULL) {
            goto Override;
        }
    }

    if ((hKey = OpenCacheKeyEx(pProfileUserName,
                               idSection,
                               KEY_READ,
                               NULL)) == NULL) {
        // if hi-word of lpKeName is 0, it is a resource number not a string

        if (!IS_PTR(lpKeyName))
            RIPMSG1(RIP_WARNING, "FastGetProfileValue: Failed to open cache-key (%08x)", lpKeyName);
        else
            RIPMSG1(RIP_WARNING | RIP_NONAME, "%ws", lpKeyName);

        goto DefExit;
    }

Override:

    if (cbSizeReturn == 0) {
        cbSize = sizeof(KeyInfo);
        pKeyInfo = &KeyInfo;
    } else {
        cbSize = cbSizeReturn + FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
        if ((pKeyInfo = UserAllocPoolWithQuota(cbSize, TAG_PROFILE)) == NULL) {
            goto DefExit;
        }
    }

    RtlInitUnicodeString(&UnicodeString, lpKeyName);

    Status = ZwQueryValueKey(hKey,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             pKeyInfo,
                             cbSize,
                             &cbSize);

    if (NT_SUCCESS(Status)) {

        UserAssert(cbSizeReturn >= pKeyInfo->DataLength);

        cbSize = pKeyInfo->DataLength;
        RtlCopyMemory(lpReturn, pKeyInfo->Data, cbSize);

        if (cbSizeReturn != 0) {
            UserFreePool(pKeyInfo);
        }
        ZwClose(hKey);

        return cbSize;
    } else if ((Status == STATUS_BUFFER_OVERFLOW) && (cbSizeReturn == 0)) {
        ZwClose(hKey);
        return pKeyInfo->DataLength;
    }

#if DBG
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        RIPMSG3 (RIP_WARNING, "FastGetProfileValue: ZwQueryValueKey Failed. Status:%#lx idSection:%#lx KeyName:%s",
                Status, idSection, UnicodeString.Buffer);
    }
#endif

    if (cbSizeReturn != 0) {
        UserFreePool(pKeyInfo);
    }

DefExit:

    if (hKey)
        ZwClose(hKey);

    if (lpDefault) {
        RtlMoveMemory(lpReturn, lpDefault, cbSizeReturn);
        return cbSizeReturn;
    }

    return 0;
}

/*****************************************************************************\
* UT_FastGetProfileIntsW
*
* Repeatedly calls FastGetProfileIntW on the given table.
*
* History:
* 02-Dec-1993 SanfordS  Created.
\*****************************************************************************/
BOOL FastGetProfileIntsW(PUNICODE_STRING pProfileUserName OPTIONAL,
    PPROFINTINFO ppii
    )
{
    WCHAR szKey[40];

    while (ppii->idSection != 0) {

        ServerLoadString(hModuleWin,
                             PTR_TO_ID(ppii->lpKeyName),
                             szKey,
                             ARRAY_SIZE(szKey));

        *ppii->puResult = FastGetProfileIntW(pProfileUserName,
                                                 ppii->idSection,
                                                 szKey,
                                                 ppii->nDefault
                                                 );
        ppii++;
    }

    return TRUE;
}

/***************************************************************************\
* UpdateWinIni
*
* Handles impersonation stuff and writes the given value to the registry.
*
* History:
* 28-Jun-1991 MikeHar       Ported.
* 03-Dec-1993 SanfordS      Used FastProfile calls, moved to profile.c
\***************************************************************************/
BOOL FastUpdateWinIni(PUNICODE_STRING pProfileUserName OPTIONAL,
    UINT         idSection,
    UINT         wKeyNameId,
    LPWSTR       lpszValue
    )
{
    WCHAR            szKeyName[40];
    BOOL             bResult = FALSE;

    UserAssert(idSection <= PMAP_LAST);

    ServerLoadString(hModuleWin,
                         wKeyNameId,
                         szKeyName,
                         ARRAY_SIZE(szKeyName));

    bResult = FastWriteProfileStringW(pProfileUserName,
                                          idSection, szKeyName, lpszValue);

    return bResult;
}
