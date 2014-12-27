/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/kbdlayout.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 *                  Copyright 2008 Colin Finck
 *                  Copyright 2011 Rafal Harabien
 */

#include <win32k.h>

#include <winnls.h>

DBG_DEFAULT_CHANNEL(UserKbdLayout);

PKL gspklBaseLayout = NULL;
PKBDFILE gpkfList = NULL;
DWORD gSystemFS = 0;
UINT gSystemCPCharSet = 0;

typedef PVOID (*PFN_KBDLAYERDESCRIPTOR)(VOID);


/* PRIVATE FUNCTIONS ******************************************************/

/*
 * UserLoadKbdDll
 *
 * Loads keyboard layout DLL and gets address to KbdTables
 */
static BOOL
UserLoadKbdDll(WCHAR *pwszLayoutPath,
               HANDLE *phModule,
               PKBDTABLES *pKbdTables)
{
    PFN_KBDLAYERDESCRIPTOR pfnKbdLayerDescriptor;

    /* Load keyboard layout DLL */
    TRACE("Loading Keyboard DLL %ws\n", pwszLayoutPath);
    *phModule = EngLoadImage(pwszLayoutPath);
    if (!(*phModule))
    {
        ERR("Failed to load dll %ws\n", pwszLayoutPath);
        return FALSE;
    }

    /* Find KbdLayerDescriptor function and get layout tables */
    TRACE("Loaded %ws\n", pwszLayoutPath);
    pfnKbdLayerDescriptor = EngFindImageProcAddress(*phModule, "KbdLayerDescriptor");

    /* FIXME: Windows reads file instead of executing!
              It's not safe to kbdlayout DLL in kernel mode! */

    if (pfnKbdLayerDescriptor)
        *pKbdTables = pfnKbdLayerDescriptor();
    else
        ERR("Error: %ws has no KbdLayerDescriptor()\n", pwszLayoutPath);

    if (!pfnKbdLayerDescriptor || !*pKbdTables)
    {
        ERR("Failed to load the keyboard layout.\n");
        EngUnloadImage(*phModule);
        return FALSE;
    }

#if 0 /* Dump keyboard layout */
    {
        unsigned i;
        PVK_TO_BIT pVkToBit = (*pKbdTables)->pCharModifiers->pVkToBit;
        PVK_TO_WCHAR_TABLE pVkToWchTbl = (*pKbdTables)->pVkToWcharTable;
        PVSC_VK pVscVk = (*pKbdTables)->pVSCtoVK_E0;
        DbgPrint("Kbd layout: fLocaleFlags %x bMaxVSCtoVK %x\n", (*pKbdTables)->fLocaleFlags, (*pKbdTables)->bMaxVSCtoVK);
        DbgPrint("wMaxModBits %x\n", (*pKbdTables)->pCharModifiers->wMaxModBits);
        while (pVkToBit->Vk)
        {
            DbgPrint("VkToBit %x -> %x\n", pVkToBit->Vk, pVkToBit->ModBits);
            ++pVkToBit;
        }
        for (i = 0; i <= (*pKbdTables)->pCharModifiers->wMaxModBits; ++i)
            DbgPrint("ModNumber %x -> %x\n", i, (*pKbdTables)->pCharModifiers->ModNumber[i]);
        while (pVkToWchTbl->pVkToWchars)
        {
            PVK_TO_WCHARS1 pVkToWch = pVkToWchTbl->pVkToWchars;
            DbgPrint("pVkToWchTbl nModifications %x cbSize %x\n", pVkToWchTbl->nModifications, pVkToWchTbl->cbSize);
            while (pVkToWch->VirtualKey)
            {
                DbgPrint("pVkToWch VirtualKey %x Attributes %x wc { ", pVkToWch->VirtualKey, pVkToWch->Attributes);
                for (i = 0; i < pVkToWchTbl->nModifications; ++i)
                    DbgPrint("%x ", pVkToWch->wch[i]);
                DbgPrint("}\n");
                pVkToWch = (PVK_TO_WCHARS1)(((PBYTE)pVkToWch) + pVkToWchTbl->cbSize);
            }
            ++pVkToWchTbl;
        }
        DbgPrint("pusVSCtoVK: { ");
        for (i = 0; i < (*pKbdTables)->bMaxVSCtoVK; ++i)
        DbgPrint("%x -> %x, ", i, (*pKbdTables)->pusVSCtoVK[i]);
        DbgPrint("}\n");
        DbgPrint("pVSCtoVK_E0: { ");
        while (pVscVk->Vsc)
        {
            DbgPrint("%x -> %x, ", pVscVk->Vsc, pVscVk->Vk);
            ++pVscVk;
        }
        DbgPrint("}\n");
        pVscVk = (*pKbdTables)->pVSCtoVK_E1;
        DbgPrint("pVSCtoVK_E1: { ");
        while (pVscVk->Vsc)
        {
            DbgPrint("%x -> %x, ", pVscVk->Vsc, pVscVk->Vk);
            ++pVscVk;
        }
        DbgPrint("}\n");
        DbgBreakPoint();
    }
#endif

    return TRUE;
}

/*
 * UserLoadKbdFile
 *
 * Loads keyboard layout DLL and creates KBDFILE object
 */
static PKBDFILE
UserLoadKbdFile(PUNICODE_STRING pwszKLID)
{
    PKBDFILE pkf, pRet = NULL;
    NTSTATUS Status;
    ULONG cbSize;
    HKEY hKey = NULL;
    WCHAR wszLayoutPath[MAX_PATH] = L"\\SystemRoot\\System32\\";
    WCHAR wszLayoutRegKey[256] = L"\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\"
                                 L"Control\\Keyboard Layouts\\";

    /* Create keyboard layout file object */
    pkf = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDFILE, sizeof(KBDFILE));
    if (!pkf)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    /* Set keyboard layout name */
    swprintf(pkf->awchKF, L"%wZ", pwszKLID);

    /* Open layout registry key */
    RtlStringCbCatW(wszLayoutRegKey, sizeof(wszLayoutRegKey), pkf->awchKF);
    Status = RegOpenKey(wszLayoutRegKey, &hKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open keyboard layouts registry key %ws (%lx)\n", wszLayoutRegKey, Status);
        goto cleanup;
    }

    /* Read filename of layout DLL */
    cbSize = sizeof(wszLayoutPath) - wcslen(wszLayoutPath)*sizeof(WCHAR);
    Status = RegQueryValue(hKey,
                           L"Layout File",
                           REG_SZ,
                           wszLayoutPath + wcslen(wszLayoutPath),
                           &cbSize);

    if (!NT_SUCCESS(Status))
    {
        ERR("Can't get layout filename for %wZ (%lx)\n", pwszKLID, Status);
        goto cleanup;
    }

    /* Load keyboard file now */
    if (!UserLoadKbdDll(wszLayoutPath, &pkf->hBase, &pkf->pKbdTbl))
    {
        ERR("Failed to load %ws dll!\n", wszLayoutPath);
        goto cleanup;
    }

    /* Update next field */
    pkf->pkfNext = gpkfList;
    gpkfList = pkf;

    /* Return keyboard file */
    pRet = pkf;

cleanup:
    if (hKey)
        ZwClose(hKey);
    if (pkf)
        UserDereferenceObject(pkf); // we dont need ptr anymore
    if (!pRet)
    {
        /* We have failed - destroy created object */
        if (pkf)
            UserDeleteObject(pkf->head.h, TYPE_KBDFILE);
    }

    return pRet;
}

/*
 * UserLoadKbdLayout
 *
 * Loads keyboard layout and creates KL object
 */
static PKL
UserLoadKbdLayout(PUNICODE_STRING pwszKLID, HKL hKL)
{
    LCID lCid;
    CHARSETINFO cs;
    PKL pKl;

    /* Create keyboard layout object */
    pKl = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDLAYOUT, sizeof(KL));
    if (!pKl)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    pKl->hkl = hKL;
    pKl->spkf = UserLoadKbdFile(pwszKLID);

    /* Dereference keyboard layout */
    UserDereferenceObject(pKl);

    /* If we failed, remove KL object */
    if (!pKl->spkf)
    {
        ERR("UserLoadKbdFile(%wZ) failed!\n", pwszKLID);
        UserDeleteObject(pKl->head.h, TYPE_KBDLAYOUT);
        return NULL;
    }

    // Up to Language Identifiers..
    RtlUnicodeStringToInteger(pwszKLID, (ULONG)16, (PULONG)&lCid);
    TRACE("Language Identifiers %wZ LCID 0x%x\n", pwszKLID, lCid);
    if (co_IntGetCharsetInfo(lCid, &cs))
    {
       pKl->iBaseCharset = cs.ciCharset;
       pKl->dwFontSigs = cs.fs.fsCsb[0];
       pKl->CodePage = (USHORT)cs.ciACP;
       TRACE("Charset %u Font Sig %lu CodePage %u\n", pKl->iBaseCharset, pKl->dwFontSigs, pKl->CodePage);
    }
    else
    {
       pKl->iBaseCharset = ANSI_CHARSET;
       pKl->dwFontSigs = FS_LATIN1;
       pKl->CodePage = CP_ACP;
    }

    // Set initial system character set and font signature.
    if (gSystemFS == 0)
    {
       gSystemCPCharSet = pKl->iBaseCharset;
       gSystemFS = pKl->dwFontSigs;
    }

    return pKl;
}

/*
 * UnloadKbdFile
 *
 * Destroys specified Keyboard File object
 */
static
VOID
UnloadKbdFile(_In_ PKBDFILE pkf)
{
    PKBDFILE *ppkfLink = &gpkfList;
    NT_ASSERT(pkf != NULL);

    /* Find previous object */
    while (*ppkfLink)
    {
        if (*ppkfLink == pkf)
            break;

        ppkfLink = &(*ppkfLink)->pkfNext;
    }

    if (*ppkfLink == pkf)
        *ppkfLink = pkf->pkfNext;

    EngUnloadImage(pkf->hBase);
    UserDeleteObject(pkf->head.h, TYPE_KBDFILE);
}

/*
 * UserUnloadKbl
 *
 * Unloads specified Keyboard Layout if possible
 */
BOOL
UserUnloadKbl(PKL pKl)
{
    /* According to msdn, UnloadKeyboardLayout can fail
       if the keyboard layout identifier was preloaded. */
    if (pKl == gspklBaseLayout)
    {
        if (pKl->pklNext == pKl->pklPrev)
        {
            /* There is only one layout */
            return FALSE;
        }

        /* Set next layout as default */
        gspklBaseLayout = pKl->pklNext;
    }

    if (pKl->head.cLockObj > 1)
    {
        /* Layout is used by other threads */
        pKl->dwKL_Flags |= KLF_UNLOAD;
        return FALSE;
    }

    /* Unload the layout */
    pKl->pklPrev->pklNext = pKl->pklNext;
    pKl->pklNext->pklPrev = pKl->pklPrev;
    UnloadKbdFile(pKl->spkf);
    UserDeleteObject(pKl->head.h, TYPE_KBDLAYOUT);
    return TRUE;
}

/*
 * W32kGetDefaultKeyLayout
 *
 * Returns default layout for new threads
 */
PKL
W32kGetDefaultKeyLayout(VOID)
{
    PKL pKl = gspklBaseLayout;

    if (!pKl)
        return NULL;

    /* Return not unloaded layout */
    do
    {
        if (!(pKl->dwKL_Flags & KLF_UNLOAD))
            return pKl;

        pKl = pKl->pklPrev; /* Confirmed on Win2k */
    } while(pKl != gspklBaseLayout);

    /* We have not found proper KL */
    return NULL;
}

/*
 * UserHklToKbl
 *
 * Gets KL object from hkl value
 */
PKL
NTAPI
UserHklToKbl(HKL hKl)
{
    PKL pKl = gspklBaseLayout;

    if (!gspklBaseLayout)
        return NULL;

    do
    {
        if (pKl->hkl == hKl)
            return pKl;

        pKl = pKl->pklNext;
    } while (pKl != gspklBaseLayout);

    return NULL;
}

/*
 * UserSetDefaultInputLang
 *
 * Sets default kyboard layout for system. Called from UserSystemParametersInfo.
 */
BOOL
NTAPI
UserSetDefaultInputLang(HKL hKl)
{
    PKL pKl;

    pKl = UserHklToKbl(hKl);
    if (!pKl)
        return FALSE;

    gspklBaseLayout = pKl;
    return TRUE;
}

/*
 * co_UserActivateKbl
 *
 * Activates given layout in specified thread
 */
static PKL
co_UserActivateKbl(PTHREADINFO pti, PKL pKl, UINT Flags)
{
    PKL pklPrev;

    pklPrev = pti->KeyboardLayout;
    if (pklPrev)
        UserDereferenceObject(pklPrev);

    pti->KeyboardLayout = pKl;
    pti->pClientInfo->hKL = pKl->hkl;
    UserReferenceObject(pKl);

    if (Flags & KLF_SETFORPROCESS)
    {
        // FIXME
    }

    // Send WM_INPUTLANGCHANGE to thread's focus window
    co_IntSendMessage(pti->MessageQueue->spwndFocus ? UserHMGetHandle(pti->MessageQueue->spwndFocus) : 0,
                      WM_INPUTLANGCHANGE,
                      (WPARAM)pKl->iBaseCharset, // FIXME: How to set it?
                      (LPARAM)pKl->hkl); // hkl

    return pklPrev;
}

/* EXPORTS *******************************************************************/

/*
 * UserGetKeyboardLayout
 *
 * Returns hkl of given thread keyboard layout
 */
HKL FASTCALL
UserGetKeyboardLayout(
    DWORD dwThreadId)
{
    NTSTATUS Status;
    PETHREAD pThread;
    PTHREADINFO pti;
    PKL pKl;
    HKL hKl;

    if (!dwThreadId)
    {
        pti = PsGetCurrentThreadWin32Thread();
        pKl = pti->KeyboardLayout;
        return pKl ? pKl->hkl : NULL;
    }

    Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)dwThreadId, &pThread);
    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    pti = PsGetThreadWin32Thread(pThread);
    pKl = pti->KeyboardLayout;
    hKl = pKl ? pKl->hkl : NULL;;
    ObDereferenceObject(pThread);
    return hKl;
}

/*
 * NtUserGetKeyboardLayoutList
 *
 * Returns list of loaded keyboard layouts in system
 */
UINT
APIENTRY
NtUserGetKeyboardLayoutList(
    ULONG nBuff,
    HKL *pHklBuff)
{
    UINT uRet = 0;
    PKL pKl;

    if (!pHklBuff)
        nBuff = 0;

    UserEnterShared();

    if (!gspklBaseLayout)
    {
        UserLeave();
        return 0;
    }
    pKl = gspklBaseLayout;

    if (nBuff == 0)
    {
        do
        {
            uRet++;
            pKl = pKl->pklNext;
        } while (pKl != gspklBaseLayout);
    }
    else
    {
        _SEH2_TRY
        {
            ProbeForWrite(pHklBuff, nBuff*sizeof(HKL), 4);

            while (uRet < nBuff)
            {
                pHklBuff[uRet] = pKl->hkl;
                uRet++;
                pKl = pKl->pklNext;
                if (pKl == gspklBaseLayout)
                    break;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            SetLastNtError(_SEH2_GetExceptionCode());
            uRet = 0;
        }
        _SEH2_END;
    }

    UserLeave();
    return uRet;
}

/*
 * NtUserGetKeyboardLayoutName
 *
 * Returns KLID of current thread keyboard layout
 */
BOOL
APIENTRY
NtUserGetKeyboardLayoutName(
    LPWSTR pwszName)
{
    BOOL bRet = FALSE;
    PKL pKl;
    PTHREADINFO pti;

    UserEnterShared();

    pti = PsGetCurrentThreadWin32Thread();
    pKl = pti->KeyboardLayout;

    if (!pKl)
        goto cleanup;

    _SEH2_TRY
    {
        ProbeForWrite(pwszName, KL_NAMELENGTH*sizeof(WCHAR), 1);
        wcscpy(pwszName, pKl->spkf->awchKF);
        bRet = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

cleanup:
    UserLeave();
    return bRet;
}

/*
 * NtUserLoadKeyboardLayoutEx
 *
 * Loads keyboard layout with given locale id
 */
HKL
APIENTRY
NtUserLoadKeyboardLayoutEx(
    IN HANDLE Handle, // hFile (See downloads.securityfocus.com/vulnerabilities/exploits/43774.c)
    IN DWORD offTable, // Offset to KbdTables
    IN PUNICODE_STRING puszKeyboardName, // Not used?
    IN HKL hklUnload,
    IN PUNICODE_STRING pustrKLID,
    IN DWORD hkl,
    IN UINT Flags)
{
    HKL hklRet = NULL;
    PKL pKl = NULL, pklLast;
    WCHAR Buffer[9];
    UNICODE_STRING ustrSafeKLID;

    if (Flags & ~(KLF_ACTIVATE|KLF_NOTELLSHELL|KLF_REORDER|KLF_REPLACELANG|
                  KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS|KLF_UNLOADPREVIOUS|
                  KLF_RESET|KLF_SETFORPROCESS|KLF_SHIFTLOCK))
    {
        ERR("Invalid flags: %x\n", Flags);
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    /* FIXME: It seems KLF_RESET is only supported for WINLOGON */

    RtlInitEmptyUnicodeString(&ustrSafeKLID, Buffer, sizeof(Buffer));
    _SEH2_TRY
    {
        ProbeForRead(pustrKLID, sizeof(*pustrKLID), 1);
        ProbeForRead(pustrKLID->Buffer, sizeof(pustrKLID->Length), 1);
        RtlCopyUnicodeString(&ustrSafeKLID, pustrKLID);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;

    UserEnterExclusive();

    /* If hklUnload is specified, unload it and load new layput as default */
    if (hklUnload && hklUnload != (HKL)hkl)
    {
        pKl = UserHklToKbl(hklUnload);
        if (pKl)
            UserUnloadKbl(pKl);
    }

    /* Let's see if layout was already loaded. */
    pKl = UserHklToKbl((HKL)hkl);
    if (!pKl)
    {
        /* It wasn't, so load it. */
        pKl = UserLoadKbdLayout(&ustrSafeKLID, (HKL)hkl);
        if (!pKl)
            goto cleanup;

        if (gspklBaseLayout)
        {
            /* Find last not unloaded layout */
            pklLast = gspklBaseLayout->pklPrev;
            while (pklLast != gspklBaseLayout && pklLast->dwKL_Flags & KLF_UNLOAD)
                pklLast = pklLast->pklPrev;

            /* Add new layout to the list */
            pKl->pklNext = pklLast->pklNext;
            pKl->pklPrev = pklLast;
            pKl->pklNext->pklPrev = pKl;
            pKl->pklPrev->pklNext = pKl;
        }
        else
        {
            /* This is the first layout */
            pKl->pklNext = pKl;
            pKl->pklPrev = pKl;
            gspklBaseLayout = pKl;
        }
    }

    /* If this layout was prepared to unload, undo it */
    pKl->dwKL_Flags &= ~KLF_UNLOAD;

    /* Activate this layout in current thread */
    if (Flags & KLF_ACTIVATE)
        co_UserActivateKbl(PsGetCurrentThreadWin32Thread(), pKl, Flags);

    /* Send shell message */
    if (!(Flags & KLF_NOTELLSHELL))
        co_IntShellHookNotify(HSHELL_LANGUAGE, 0, (LPARAM)hkl);

    /* Return hkl on success */
    hklRet = (HKL)hkl;

    /* FIXME: KLF_REPLACELANG
              KLF_REORDER */

cleanup:
    UserLeave();
    return hklRet;
}

/*
 * NtUserActivateKeyboardLayout
 *
 * Activates specified layout for thread or process
 */
HKL
APIENTRY
NtUserActivateKeyboardLayout(
    HKL hKl,
    ULONG Flags)
{
    PKL pKl = NULL;
    HKL hkl = NULL;
    PTHREADINFO pti;

    UserEnterExclusive();

    pti = PsGetCurrentThreadWin32Thread();

    /* hKl can have special value HKL_NEXT or HKL_PREV */
    if (hKl == (HKL)HKL_NEXT)
    {
        /* Get next keyboard layout starting with current */
        if (pti->KeyboardLayout)
            pKl = pti->KeyboardLayout->pklNext;
    }
    else if (hKl == (HKL)HKL_PREV)
    {
        /* Get previous keyboard layout starting with current */
        if (pti->KeyboardLayout)
            pKl = pti->KeyboardLayout->pklNext;
    }
    else
        pKl = UserHklToKbl(hKl);

    if (!pKl)
    {
        ERR("Invalid HKL %p!\n", hKl);
        goto cleanup;
    }

    hkl = pKl->hkl;

    /* FIXME: KLF_RESET
              KLF_SHIFTLOCK */

    if (Flags & KLF_REORDER)
        gspklBaseLayout = pKl;

    if (pKl != pti->KeyboardLayout)
    {
        /* Activate layout for current thread */
        pKl = co_UserActivateKbl(pti, pKl, Flags);

        /* Send shell message */
        if (!(Flags & KLF_NOTELLSHELL))
            co_IntShellHookNotify(HSHELL_LANGUAGE, 0, (LPARAM)hkl);
    }

cleanup:
    UserLeave();
    return hkl;
}

/*
 * NtUserUnloadKeyboardLayout
 *
 * Unloads keyboard layout with specified hkl value
 */
BOOL
APIENTRY
NtUserUnloadKeyboardLayout(
    HKL hKl)
{
    PKL pKl;
    BOOL bRet = FALSE;

    UserEnterExclusive();

    pKl = UserHklToKbl(hKl);
    if (pKl)
        bRet = UserUnloadKbl(pKl);
    else
        ERR("Invalid HKL %p!\n", hKl);

    UserLeave();
    return bRet;
}

/* EOF */
