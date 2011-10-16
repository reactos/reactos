/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/kbdlayout.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 *                  Copyright 2008 Colin Finck
 *                  Copyright 2011 Rafal Harabien
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserKbdLayout);

PKL gpklFirst = NULL; // Keyboard layout list.

typedef PVOID (*PFNKBDLAYERDESCRIPTOR)(VOID);


/* PRIVATE FUNCTIONS ******************************************************/

static BOOL
UserLoadKbdDll(CONST WCHAR *wszKLID,
               HANDLE *phModule,
               PKBDTABLES *pKbdTables)
{
    NTSTATUS Status;
    HKEY hKey;
    ULONG cbSize;
    PFNKBDLAYERDESCRIPTOR pfnKbdLayerDescriptor;
    WCHAR wszLayoutRegKey[256] = L"\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\"
                                 L"Control\\Keyboard Layouts\\";
    WCHAR wszLayoutPath[MAX_PATH] = L"\\SystemRoot\\System32\\";

    /* Open layout registry key */
    RtlStringCbCatW(wszLayoutRegKey, sizeof(wszLayoutRegKey), wszKLID);
    Status = RegOpenKey(wszLayoutRegKey, &hKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open keyboard layouts registry key %ws (%lx)\n", wszKLID, Status);
        return FALSE;
    }

    /* Read filename of layout DLL and close the key */
    cbSize = sizeof(wszLayoutPath) - (wcslen(wszLayoutPath) + 1)*sizeof(WCHAR);
    Status = RegQueryValue(hKey,
                           L"Layout File",
                           REG_SZ,
                           wszLayoutPath + wcslen(wszLayoutPath),
                           &cbSize);
    ZwClose(hKey);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Can't get layout filename for %ws (%lx)\n", wszKLID, Status);
        return FALSE;
    }

    /* Load keyboard layout DLL */
    TRACE("Loading Keyboard DLL %ws\n", wszLayoutPath);
    *phModule = EngLoadImage(wszLayoutPath);
    if (!(*phModule))
    {
        ERR("Failed to load dll %ws\n", wszLayoutPath);
        return FALSE;
    }

    /* Find KbdLayerDescriptor function and get layout tables */
    TRACE("Loaded %ws\n", wszLayoutPath);
    pfnKbdLayerDescriptor = EngFindImageProcAddress(*phModule, "KbdLayerDescriptor");

    if (pfnKbdLayerDescriptor)
        *pKbdTables = pfnKbdLayerDescriptor();
    else
        ERR("Error: %ws has no KbdLayerDescriptor()\n", wszLayoutPath);

    if (!pfnKbdLayerDescriptor || !*pKbdTables)
    {
        ERR("Failed to load the keyboard layout.\n");
        EngUnloadImage(*phModule);
        return FALSE;
    }

#if 0 // Dump keyboard layout
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

static PKL
UserLoadDllAndCreateKbl(DWORD LocaleId)
{
    PKL pNewKbl;
    ULONG hKl;
    LANGID langid;

    pNewKbl = ExAllocatePoolWithTag(PagedPool, sizeof(KL), USERTAG_KBDLAYOUT);

    if (!pNewKbl)
    {
        ERR("Can't allocate memory!\n");
        return NULL;
    }

    swprintf(pNewKbl->Name, L"%08lx", LocaleId);

    if (!UserLoadKbdDll(pNewKbl->Name, &pNewKbl->hModule, &pNewKbl->KBTables))
    {
        ERR("Failed to load %x dll!\n", LocaleId);
        ExFreePoolWithTag(pNewKbl, USERTAG_KBDLAYOUT);
        return NULL;
    }

    /* Microsoft Office expects this value to be something specific
     * for Japanese and Korean Windows with an IME the value is 0xe001
     * We should probably check to see if an IME exists and if so then
     * set this word properly.
     */
    langid = PRIMARYLANGID(LANGIDFROMLCID(LocaleId));
    hKl = LocaleId;

    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
        hKl |= 0xe001 << 16; /* FIXME */
    else hKl |= hKl << 16;

    pNewKbl->hkl = (HKL)(ULONG_PTR) hKl;
    pNewKbl->klid = LocaleId;
    pNewKbl->Flags = 0;
    pNewKbl->RefCount = 0;

    return pNewKbl;
}

BOOL
UserInitDefaultKeyboardLayout()
{
    LCID LocaleId;
    NTSTATUS Status;

    /* Load keyboard layout for default locale */
    Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
    if (NT_SUCCESS(Status))
    {
        TRACE("DefaultLocale = %08lx\n", LocaleId);
        gpklFirst = UserLoadDllAndCreateKbl(LocaleId);
    }
    else
        ERR("Could not get default locale (%08lx).\n", Status);

    if (!NT_SUCCESS(Status) || !gpklFirst)
    {
        /* If failed load US keyboard layout */
        ERR("Trying to load US Keyboard Layout.\n");
        LocaleId = 0x409;

        if (!(gpklFirst = UserLoadDllAndCreateKbl(LocaleId)))
        {
            ERR("Failed to load any Keyboard Layout\n");
            return FALSE;
        }
    }

    /* Add layout to the list */
    gpklFirst->Flags |= KBL_PRELOAD;
    InitializeListHead(&gpklFirst->List);

    return TRUE;
}

PKL
W32kGetDefaultKeyLayout(VOID)
{
    CONST WCHAR wszDefaultUserPath[] = L"\\REGISTRY\\USER\\.DEFAULT";
    CONST WCHAR wszKeyboardLayoutPath[] = L"\\Keyboard Layout\\Preload";
    WCHAR wszKbdLayoutKey[256], *pwsz;
    size_t cbRemaining;
    HKEY hKey;
    ULONG cbValue;
    LCID LayoutLocaleId = 0;
    NTSTATUS Status;
    PKL pKbl;
    UNICODE_STRING CurrentUserPath;
    WCHAR wszBuffer[MAX_PATH];

    /* Try to get default alayout from HKCU\Keyboard Layout\Preload first */
    Status = RtlFormatCurrentUserKeyPath(&CurrentUserPath);
    if (NT_SUCCESS(Status))
    {
        /* FIXME: We're called very early, so HKEY_CURRENT_USER might not be
                  available yet. Check this first. */
        RtlStringCbCopyNExW(wszKbdLayoutKey, sizeof(wszKbdLayoutKey),
                            CurrentUserPath.Buffer, CurrentUserPath.Length,
                            &pwsz, &cbRemaining, 0);
        RtlStringCbCopyW(pwsz, cbRemaining, wszKeyboardLayoutPath);
        Status = RegOpenKey(wszKbdLayoutKey, &hKey);

        /* Free CurrentUserPath - we dont need it anymore */
        RtlFreeUnicodeString(&CurrentUserPath);
    }

    /* If failed try HKU\.DEFAULT\Keyboard Layout\Preload */
    if (!NT_SUCCESS(Status))
    {
        RtlStringCbCopyNExW(wszKbdLayoutKey, sizeof(wszKbdLayoutKey),
                            wszDefaultUserPath, sizeof(wszDefaultUserPath),
                            &pwsz, &cbRemaining, 0);
        RtlStringCbCopyW(pwsz, cbRemaining, wszKeyboardLayoutPath);
        Status = RegOpenKey(wszKbdLayoutKey, &hKey);
    }

    if (NT_SUCCESS(Status))
    {
        /* Return the first keyboard layout listed there */
        cbValue = sizeof(wszBuffer);
        Status = RegQueryValue(hKey, L"1", REG_SZ, wszBuffer, &cbValue);
        if (NT_SUCCESS(Status))
            LayoutLocaleId = (LCID)wcstol(wszBuffer, NULL, 16);
        else
            ERR("RegQueryValue failed (%08lx)\n", Status);

        /* Close the key */
        ZwClose(hKey);
    }
    else
        ERR("Failed to open keyboard layout preload key (%08lx)\n", Status);

    /* If we failed loading settings from registry use US layout */
    if (!LayoutLocaleId)
    {
        ERR("Assuming default locale for the keyboard layout (0x409 - US)\n");
        LayoutLocaleId = 0x409;
    }

    /* Check if layout is already loaded */
    pKbl = gpklFirst;
    do
    {
        if (pKbl->klid == LayoutLocaleId)
            return pKbl;

        pKbl = CONTAINING_RECORD(pKbl->List.Flink, KL, List);
    } while (pKbl != gpklFirst);

    /* Load the keyboard layout */
    TRACE("Loading new default keyboard layout.\n");
    pKbl = UserLoadDllAndCreateKbl(LayoutLocaleId);
    if (!pKbl)
    {
        ERR("Failed to load %x!!! Returning any available KL.\n", LayoutLocaleId);
        return gpklFirst;
    }

    /* Add loaded layout to the list */
    InsertTailList(&gpklFirst->List, &pKbl->List);
    return pKbl;
}

PKL
UserHklToKbl(HKL hKl)
{
    PKL pKbl = gpklFirst;
    do
    {
        if (pKbl->hkl == hKl)
            return pKbl;

        pKbl = CONTAINING_RECORD(pKbl->List.Flink, KL, List);
    } while (pKbl != gpklFirst);

    return NULL;
}

BOOL
UserUnloadKbl(PKL pKbl)
{
    /* According to msdn, UnloadKeyboardLayout can fail
       if the keyboard layout identifier was preloaded. */

    if (pKbl->Flags & KBL_PRELOAD)
    {
        ERR("Attempted to unload preloaded keyboard layout.\n");
        return FALSE;
    }

    if (pKbl->RefCount > 0)
    {
        /* Layout is used by other threads.
           Mark it as unloaded and don't do anything else. */
        pKbl->Flags |= KBL_UNLOAD;
    }
    else
    {
        //Unload the layout
        EngUnloadImage(pKbl->hModule);
        RemoveEntryList(&pKbl->List);
        ExFreePoolWithTag(pKbl, USERTAG_KBDLAYOUT);
    }

    return TRUE;
}

static PKL
co_UserActivateKbl(PTHREADINFO pti, PKL pKbl, UINT Flags)
{
    PKL pklPrev;

    pklPrev = pti->KeyboardLayout;
    pklPrev->RefCount--;
    pti->KeyboardLayout = pKbl;
    pKbl->RefCount++;

    if (Flags & KLF_SETFORPROCESS)
    {
        //FIXME

    }

    if (pklPrev->Flags & KBL_UNLOAD && pklPrev->RefCount == 0)
    {
        UserUnloadKbl(pklPrev);
    }

    // Send WM_INPUTLANGCHANGE to thread's focus window
    co_IntSendMessage(pti->MessageQueue->FocusWindow,
                      WM_INPUTLANGCHANGE,
                      0, // FIXME: put charset here (what is this?)
                      (LPARAM)pKbl->hkl); //klid

    return pklPrev;
}

/* EXPORTS *******************************************************************/

HKL FASTCALL
UserGetKeyboardLayout(
    DWORD dwThreadId)
{
    NTSTATUS Status;
    PETHREAD Thread;
    PTHREADINFO pti;
    HKL Ret;

    if (!dwThreadId)
    {
        pti = PsGetCurrentThreadWin32Thread();
        return pti->KeyboardLayout->hkl;
    }

    Status = PsLookupThreadByThreadId((HANDLE)(DWORD_PTR)dwThreadId, &Thread);
    if (!NT_SUCCESS(Status))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    pti = PsGetThreadWin32Thread(Thread);
    Ret = pti->KeyboardLayout->hkl;
    ObDereferenceObject(Thread);
    return Ret;
}

UINT
APIENTRY
NtUserGetKeyboardLayoutList(
    INT nBuff,
    HKL* pHklBuff)
{
    UINT uRet = 0;
    PKL pKbl;

    UserEnterShared();
    pKbl = gpklFirst;

    if (!pHklBuff)
        nBuff = 0;

    if (nBuff == 0)
    {
        do
        {
            uRet++;
            pKbl = CONTAINING_RECORD(pKbl->List.Flink, KL, List);
        } while (pKbl != gpklFirst);
    }
    else
    {
        _SEH2_TRY
        {
            ProbeForWrite(pHklBuff, nBuff*sizeof(HKL), 4);

            while (uRet < nBuff)
            {
                if (!(pKbl->Flags & KBL_UNLOAD))
                {
                    pHklBuff[uRet] = pKbl->hkl;
                    uRet++;
                    pKbl = CONTAINING_RECORD(pKbl->List.Flink, KL, List);
                    if (pKbl == gpklFirst)
                        break;
                }
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

BOOL
APIENTRY
NtUserGetKeyboardLayoutName(
    LPWSTR lpszName)
{
    BOOL bRet = FALSE;
    PKL pKbl;
    PTHREADINFO pti;

    UserEnterShared();

    _SEH2_TRY
    {
        ProbeForWrite(lpszName, KL_NAMELENGTH*sizeof(WCHAR), 1);
        pti = PsGetCurrentThreadWin32Thread();
        pKbl = pti->KeyboardLayout;
        RtlCopyMemory(lpszName, pKbl->Name, KL_NAMELENGTH*sizeof(WCHAR));
        bRet = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
    }
    _SEH2_END;

    UserLeave();
    return bRet;
}

HKL
APIENTRY
NtUserLoadKeyboardLayoutEx(
    IN HANDLE Handle,
    IN DWORD offTable,
    IN PUNICODE_STRING puszKeyboardName,
    IN HKL hKL,
    IN PUNICODE_STRING puszKLID,
    IN DWORD dwKLID,
    IN UINT Flags)
{
    HKL hklRet = NULL;
    PKL pKbl = NULL, pklCur;

    if (Flags & ~(KLF_ACTIVATE|KLF_NOTELLSHELL|KLF_REORDER|KLF_REPLACELANG|
                  KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS|KLF_UNLOADPREVIOUS))
    {
        ERR("Invalid flags: %x\n", Flags);
        EngSetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    UserEnterExclusive();

    //Let's see if layout was already loaded.
    pklCur = gpklFirst;
    do
    {
        if (pklCur->klid == dwKLID)
        {
            pKbl = pklCur;
            pKbl->Flags &= ~KBL_UNLOAD;
            break;
        }

        pklCur = CONTAINING_RECORD(pKbl->List.Flink, KL, List);
    } while (pklCur != gpklFirst);

    //It wasn't, so load it.
    if (!pKbl)
    {
        pKbl = UserLoadDllAndCreateKbl(dwKLID);

        if (!pKbl)
        {
            goto cleanup;
        }

        InsertTailList(&gpklFirst->List, &pKbl->List);
    }

    if (Flags & KLF_REORDER) gpklFirst = pKbl;

    if (Flags & KLF_ACTIVATE)
    {
        co_UserActivateKbl(PsGetCurrentThreadWin32Thread(), pKbl, Flags);
    }

    hklRet = pKbl->hkl;

    //FIXME: KLF_NOTELLSHELL
    //       KLF_REPLACELANG
    //       KLF_SUBSTITUTE_OK

cleanup:
    UserLeave();
    return hklRet;
}

HKL
APIENTRY
NtUserActivateKeyboardLayout(
    HKL hKl,
    ULONG Flags)
{
    PKL pKbl;
    HKL hklRet = NULL;
    PTHREADINFO pti;

    UserEnterExclusive();

    pti = PsGetCurrentThreadWin32Thread();

    if (pti->KeyboardLayout->hkl == hKl)
    {
        hklRet = hKl;
        goto cleanup;
    }

    if (hKl == (HKL)HKL_NEXT)
    {
        pKbl = CONTAINING_RECORD(pti->KeyboardLayout->List.Flink, KL, List);
    }
    else if (hKl == (HKL)HKL_PREV)
    {
        pKbl = CONTAINING_RECORD(pti->KeyboardLayout->List.Blink, KL, List);
    }
    else pKbl = UserHklToKbl(hKl);

    //FIXME:  KLF_RESET, KLF_SHIFTLOCK

    if (pKbl)
    {
        if (Flags & KLF_REORDER)
            gpklFirst = pKbl;

        if (pKbl == pti->KeyboardLayout)
        {
            hklRet = pKbl->hkl;
        }
        else
        {
            pKbl = co_UserActivateKbl(pti, pKbl, Flags);
            hklRet = pKbl->hkl;
        }
    }
    else
    {
        ERR("Invalid HKL %x!\n", hKl);
    }

cleanup:
    UserLeave();
    return hklRet;
}

BOOL
APIENTRY
NtUserUnloadKeyboardLayout(
    HKL hKl)
{
    PKL pKbl;
    BOOL bRet = FALSE;

    UserEnterExclusive();

    pKbl = UserHklToKbl(hKl);
    if (pKbl)
    {
        bRet = UserUnloadKbl(pKbl);
    }
    else
    {
        ERR("Invalid HKL %x!\n", hKl);
    }

    UserLeave();
    return bRet;
}

/* EOF */
