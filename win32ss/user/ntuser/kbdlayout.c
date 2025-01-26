/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/user/ntuser/kbdlayout.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 *                  Copyright 2008 Colin Finck
 *                  Copyright 2011 Rafal Harabien
 *                  Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <win32k.h>
#include <immdev.h>

// Was included only because of CP_ACP and required  the
// definition of SYSTEMTIME in ndk\rtltypes.h
//#include <winnls.h>
#define CP_ACP 0

DBG_DEFAULT_CHANNEL(UserKbdLayout);

PKL gspklBaseLayout = NULL; /* FIXME: Please move this to pWinSta->spklList */
PKBDFILE gpkfList = NULL;
DWORD gSystemFS = 0;
UINT gSystemCPCharSet = 0;
HKL ghKLSentToShell = NULL;

typedef PVOID (*PFN_KBDLAYERDESCRIPTOR)(VOID);

/* PRIVATE FUNCTIONS ******************************************************/

/*
 * Retrieves a PKL by an input locale identifier (HKL).
 * @implemented
 */
PKL FASTCALL IntHKLtoPKL(_Inout_ PTHREADINFO pti, _In_ HKL hKL)
{
    PKL pFirstKL, pKL;

    pFirstKL = pti->KeyboardLayout;
    if (!pFirstKL)
        return NULL;

    pKL = pFirstKL;

    /* hKL can have special value HKL_NEXT or HKL_PREV */
    if (hKL == UlongToHandle(HKL_NEXT)) /* Looking forward */
    {
        do
        {
            pKL = pKL->pklNext;
            if (!(pKL->dwKL_Flags & KL_UNLOAD))
                return pKL;
        } while (pKL != pFirstKL);
    }
    else if (hKL == UlongToHandle(HKL_PREV)) /* Looking backward */
    {
        do
        {
            pKL = pKL->pklPrev;
            if (!(pKL->dwKL_Flags & KL_UNLOAD))
                return pKL;
        } while (pKL != pFirstKL);
    }
    else if (HIWORD(hKL)) /* hKL is a full input locale identifier */
    {
        /* No KL_UNLOAD check */
        do
        {
            if (pKL->hkl == hKL)
                return pKL;

            pKL = pKL->pklNext;
        } while (pKL != pFirstKL);
    }
    else  /* Language only specified */
    {
        /* No KL_UNLOAD check */
        do
        {
            if (LOWORD(pKL->hkl) == LOWORD(hKL)) /* Low word is language ID */
                return pKL;

            pKL = pKL->pklNext;
        } while (pKL != pFirstKL);
    }

    return NULL;
}

/*
 * A helper function for NtUserGetKeyboardLayoutList.
 * @implemented
 */
static UINT APIENTRY
IntGetKeyboardLayoutList(
    _Inout_ PWINSTATION_OBJECT pWinSta,
    _In_ ULONG nBuff,
    _Out_ HKL *pHklBuff)
{
    UINT ret = 0;
    PKL pKL, pFirstKL;

    pFirstKL = gspklBaseLayout; /* FIXME: Use pWinSta->spklList instead */
    if (!pWinSta || !pFirstKL)
        return 0;

    pKL = pFirstKL;

    if (nBuff == 0)
    {
        /* Count the effective PKLs */
        do
        {
            if (!(pKL->dwKL_Flags & KL_UNLOAD))
                ++ret;
            pKL = pKL->pklNext;
        } while (pKL != pFirstKL);
    }
    else
    {
        /* Copy the effective HKLs to pHklBuff */
        do
        {
            if (!(pKL->dwKL_Flags & KL_UNLOAD))
            {
                *pHklBuff = pKL->hkl;
                ++pHklBuff;
                ++ret;
                --nBuff;

                if (nBuff == 0)
                    break;
            }
            pKL = pKL->pklNext;
        } while (pKL != pFirstKL);
    }

    return ret;
}

#if 0 && DBG

static VOID
DumpKbdLayout(
    IN PKBDTABLES pKbdTbl)
{
    PVK_TO_BIT pVkToBit;
    PVK_TO_WCHAR_TABLE pVkToWchTbl;
    PVSC_VK pVscVk;
    ULONG i;

    DbgPrint("Kbd layout: fLocaleFlags %x bMaxVSCtoVK %x\n",
             pKbdTbl->fLocaleFlags, pKbdTbl->bMaxVSCtoVK);
    DbgPrint("wMaxModBits %x\n",
             pKbdTbl->pCharModifiers ? pKbdTbl->pCharModifiers->wMaxModBits
                                     : 0);

    if (pKbdTbl->pCharModifiers)
    {
        pVkToBit = pKbdTbl->pCharModifiers->pVkToBit;
        if (pVkToBit)
        {
            for (; pVkToBit->Vk; ++pVkToBit)
            {
                DbgPrint("VkToBit %x -> %x\n", pVkToBit->Vk, pVkToBit->ModBits);
            }
        }

        for (i = 0; i <= pKbdTbl->pCharModifiers->wMaxModBits; ++i)
        {
            DbgPrint("ModNumber %x -> %x\n", i, pKbdTbl->pCharModifiers->ModNumber[i]);
        }
    }

    pVkToWchTbl = pKbdTbl->pVkToWcharTable;
    if (pVkToWchTbl)
    {
        for (; pVkToWchTbl->pVkToWchars; ++pVkToWchTbl)
        {
            PVK_TO_WCHARS1 pVkToWch = pVkToWchTbl->pVkToWchars;

            DbgPrint("pVkToWchTbl nModifications %x cbSize %x\n",
                     pVkToWchTbl->nModifications, pVkToWchTbl->cbSize);
            if (pVkToWch)
            {
                while (pVkToWch->VirtualKey)
                {
                    DbgPrint("pVkToWch VirtualKey %x Attributes %x wc { ",
                             pVkToWch->VirtualKey, pVkToWch->Attributes);
                    for (i = 0; i < pVkToWchTbl->nModifications; ++i)
                    {
                        DbgPrint("%x ", pVkToWch->wch[i]);
                    }
                    DbgPrint("}\n");
                    pVkToWch = (PVK_TO_WCHARS1)(((PBYTE)pVkToWch) + pVkToWchTbl->cbSize);
                }
            }
        }
    }

// TODO: DeadKeys, KeyNames, KeyNamesExt, KeyNamesDead

    DbgPrint("pusVSCtoVK: { ");
    if (pKbdTbl->pusVSCtoVK)
    {
        for (i = 0; i < pKbdTbl->bMaxVSCtoVK; ++i)
        {
            DbgPrint("%x -> %x, ", i, pKbdTbl->pusVSCtoVK[i]);
        }
    }
    DbgPrint("}\n");

    DbgPrint("pVSCtoVK_E0: { ");
    pVscVk = pKbdTbl->pVSCtoVK_E0;
    if (pVscVk)
    {
        for (; pVscVk->Vsc; ++pVscVk)
        {
            DbgPrint("%x -> %x, ", pVscVk->Vsc, pVscVk->Vk);
        }
    }
    DbgPrint("}\n");

    DbgPrint("pVSCtoVK_E1: { ");
    pVscVk = pKbdTbl->pVSCtoVK_E1;
    if (pVscVk)
    {
        for (; pVscVk->Vsc; ++pVscVk)
        {
            DbgPrint("%x -> %x, ", pVscVk->Vsc, pVscVk->Vk);
        }
    }
    DbgPrint("}\n");

// TODO: Ligatures
}

#endif // DBG


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

#if 0 && DBG
    /* Dump keyboard layout */
    DumpKbdLayout(*pKbdTables);
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
    cbSize = (ULONG)(sizeof(wszLayoutPath) - wcslen(wszLayoutPath)*sizeof(WCHAR));
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
            UserDeleteObject(UserHMGetHandle(pkf), TYPE_KBDFILE);
    }

    return pRet;
}

/*
 * co_UserLoadKbdLayout
 *
 * Loads keyboard layout and creates KL object
 */
static PKL
co_UserLoadKbdLayout(PUNICODE_STRING pustrKLID, HKL hKL)
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
    pKl->spkf = UserLoadKbdFile(pustrKLID);

    /* Dereference keyboard layout */
    UserDereferenceObject(pKl);

    /* If we failed, remove KL object */
    if (!pKl->spkf)
    {
        ERR("UserLoadKbdFile(%wZ) failed!\n", pustrKLID);
        UserDeleteObject(UserHMGetHandle(pKl), TYPE_KBDLAYOUT);
        return NULL;
    }

    // Up to Language Identifiers..
    if (!NT_SUCCESS(RtlUnicodeStringToInteger(pustrKLID, 16, (PULONG)&lCid)))
    {
        ERR("RtlUnicodeStringToInteger failed for '%wZ'\n", pustrKLID);
        UserDeleteObject(UserHMGetHandle(pKl), TYPE_KBDLAYOUT);
        return NULL;
    }

    TRACE("Language Identifiers %wZ LCID 0x%x\n", pustrKLID, lCid);
    if (co_IntGetCharsetInfo(lCid, &cs))
    {
       pKl->iBaseCharset = cs.ciCharset;
       pKl->dwFontSigs = cs.fs.fsCsb[0];
       pKl->CodePage = (USHORT)cs.ciACP;
       TRACE("Charset %u Font Sig %lu CodePage %u\n",
             pKl->iBaseCharset, pKl->dwFontSigs, pKl->CodePage);
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
    UserDeleteObject(UserHMGetHandle(pkf), TYPE_KBDFILE);
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
        pKl->dwKL_Flags |= KL_UNLOAD;
        return FALSE;
    }

    /* Unload the layout */
    pKl->pklPrev->pklNext = pKl->pklNext;
    pKl->pklNext->pklPrev = pKl->pklPrev;
    UnloadKbdFile(pKl->spkf);
    if (pKl->piiex)
    {
        ExFreePoolWithTag(pKl->piiex, USERTAG_IME);
    }
    UserDeleteObject(UserHMGetHandle(pKl), TYPE_KBDLAYOUT);
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
        if (!(pKl->dwKL_Flags & KL_UNLOAD))
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

VOID FASTCALL
IntReorderKeyboardLayouts(
    _Inout_ PWINSTATION_OBJECT pWinSta,
    _Inout_ PKL pNewKL)
{
    PKL pOldKL = gspklBaseLayout;

    if ((pWinSta->Flags & WSS_NOIO) || pNewKL == pOldKL)
        return;

    pNewKL->pklPrev->pklNext = pNewKL->pklNext;
    pNewKL->pklNext->pklPrev = pNewKL->pklPrev;
    pNewKL->pklNext = pOldKL;
    pNewKL->pklPrev = pOldKL->pklPrev;
    pOldKL->pklPrev->pklNext = pNewKL;
    pOldKL->pklPrev = pNewKL;
    gspklBaseLayout = pNewKL; /* Should we use UserAssignmentLock? */
}

/*
 * UserSetDefaultInputLang
 *
 * Sets default keyboard layout for system. Called from UserSystemParametersInfo.
 */
BOOL
NTAPI
UserSetDefaultInputLang(HKL hKl)
{
    PKL pKl;

    pKl = UserHklToKbl(hKl);
    if (!pKl)
        return FALSE;

    IntReorderKeyboardLayouts(IntGetProcessWindowStation(NULL), pKl);
    return TRUE;
}

VOID APIENTRY
IntImmActivateLayout(
    _Inout_ PTHREADINFO pti,
    _Inout_ PKL pKL)
{
    PWND pImeWnd;
    HWND hImeWnd;
    USER_REFERENCE_ENTRY Ref;

    if (pti->KeyboardLayout == pKL)
        return;

    pImeWnd = pti->spwndDefaultIme;
    if (pImeWnd)
    {
        UserRefObjectCo(pImeWnd, &Ref);
        hImeWnd = UserHMGetHandle(pImeWnd);
        co_IntSendMessage(hImeWnd, WM_IME_SYSTEM, IMS_ACTIVATELAYOUT, (LPARAM)pKL->hkl);
        UserDerefObjectCo(pImeWnd);
    }
    else
    {
        /* Remember old keyboard layout to switch back for Chinese IMEs */
        pti->hklPrev = pti->KeyboardLayout->hkl;

        if (pti->spDefaultImc)
        {
            /* IME Activation is needed */
            pti->pClientInfo->CI_flags |= CI_IMMACTIVATE;
        }
    }

    UserAssignmentLock((PVOID*)&(pti->KeyboardLayout), pKL);
    pti->pClientInfo->hKL = pKL->hkl;
    pti->pClientInfo->CodePage = pKL->CodePage;
}

static VOID co_IntSetKeyboardLayoutForProcess(PPROCESSINFO ppi, PKL pKL)
{
    PTHREADINFO ptiNode, ptiNext;
    PCLIENTINFO pClientInfo;
    BOOL bImmMode = IS_IMM_MODE();

    for (ptiNode = ppi->ptiList; ptiNode; ptiNode = ptiNext)
    {
        IntReferenceThreadInfo(ptiNode);
        ptiNext = ptiNode->ptiSibling;

        /* Skip this thread if its keyboard layout is already the correct one, or if it's dying */
        if (ptiNode->KeyboardLayout == pKL || (ptiNode->TIF_flags & TIF_INCLEANUP))
        {
            IntDereferenceThreadInfo(ptiNode);
            continue;
        }

        if (bImmMode)
        {
            IntImmActivateLayout(ptiNode, pKL);
        }
        else
        {
            UserAssignmentLock((PVOID*)&ptiNode->KeyboardLayout, pKL);
            pClientInfo = ptiNode->pClientInfo;
            pClientInfo->CodePage = pKL->CodePage;
            pClientInfo->hKL = pKL->hkl;
        }

        IntDereferenceThreadInfo(ptiNode);
    }
}

HKL APIENTRY
co_UserActivateKeyboardLayout(
    _Inout_ PKL     pKL,
    _In_    ULONG   uFlags,
    _In_opt_ PWND pWnd)
{
    HKL hOldKL = NULL;
    PKL pOldKL;
    PTHREADINFO pti = GetW32ThreadInfo();
    PWND pTargetWnd, pImeWnd;
    HWND hTargetWnd, hImeWnd;
    USER_REFERENCE_ENTRY Ref1, Ref2;
    PCLIENTINFO ClientInfo;
    BOOL bSetForProcess = !!(uFlags & KLF_SETFORPROCESS);

    IntReferenceThreadInfo(pti);
    ClientInfo = pti->pClientInfo;

    pOldKL = pti->KeyboardLayout;
    if (pOldKL)
        hOldKL = pOldKL->hkl;

    if (uFlags & KLF_RESET)
    {
        FIXME("KLF_RESET\n");
    }

    if (!bSetForProcess && pKL == pti->KeyboardLayout)
    {
        IntDereferenceThreadInfo(pti);
        return hOldKL;
    }

    pKL->wchDiacritic = UNICODE_NULL;

    if (pOldKL)
        UserRefObjectCo(pOldKL, &Ref1);

    if (pti->TIF_flags & TIF_CSRSSTHREAD)
    {
        UserAssignmentLock((PVOID*)&pti->KeyboardLayout, pKL);
        ClientInfo->CodePage = pKL->CodePage;
        ClientInfo->hKL = pKL->hkl;
    }
    else if (bSetForProcess)
    {
        co_IntSetKeyboardLayoutForProcess(pti->ppi, pKL);
    }
    else
    {
        if (IS_IMM_MODE())
            IntImmActivateLayout(pti, pKL);
        else
            UserAssignmentLock((PVOID*)&pti->KeyboardLayout, pKL);

        ClientInfo->CodePage = pKL->CodePage;
        ClientInfo->hKL = pKL->hkl;
    }

    /* Send shell message if necessary */
    if (gptiForeground && (gptiForeground->ppi == pti->ppi) && ISITHOOKED(WH_SHELL))
    {
        /* Send the HKL if needed and remember it */
        if (ghKLSentToShell != pKL->hkl)
        {
            co_IntShellHookNotify(HSHELL_LANGUAGE, 0, (LPARAM)pKL->hkl);
            ghKLSentToShell = pKL->hkl;
        }
    }

    if (pti->MessageQueue)
    {
        /* Determine the target window */
        pTargetWnd = pti->MessageQueue->spwndFocus;
        if (!pTargetWnd)
        {
            pTargetWnd = pti->MessageQueue->spwndActive;
            if (!pTargetWnd)
                pTargetWnd = pWnd;
        }

        /* Send WM_INPUTLANGCHANGE message */
        if (pTargetWnd)
        {
            UserRefObjectCo(pTargetWnd, &Ref2);
            hTargetWnd = UserHMGetHandle(pTargetWnd);
            co_IntSendMessage(hTargetWnd, WM_INPUTLANGCHANGE, pKL->iBaseCharset, (LPARAM)pKL->hkl);
            UserDerefObjectCo(pTargetWnd);
        }
    }

    // Refresh IME UI via WM_IME_SYSTEM:IMS_SENDNOTIFICATION messaging
    if (!(pti->TIF_flags & TIF_CSRSSTHREAD))
    {
        if (IS_IME_HKL(pKL->hkl) || (IS_CICERO_MODE() && !IS_16BIT_MODE()))
        {
            pImeWnd = pti->spwndDefaultIme;
            if (pImeWnd)
            {
                bSetForProcess &= !IS_16BIT_MODE();
                UserRefObjectCo(pImeWnd, &Ref2);
                hImeWnd = UserHMGetHandle(pImeWnd);
                co_IntSendMessage(hImeWnd, WM_IME_SYSTEM, IMS_SENDNOTIFICATION, bSetForProcess);
                UserDerefObjectCo(pImeWnd);
            }
        }
    }

    if (pOldKL)
        UserDerefObjectCo(pOldKL);

    IntDereferenceThreadInfo(pti);
    return hOldKL;
}

HKL APIENTRY
co_IntActivateKeyboardLayout(
    _Inout_ PWINSTATION_OBJECT pWinSta,
    _In_ HKL hKL,
    _In_ ULONG uFlags,
    _In_opt_ PWND pWnd)
{
    PKL pKL;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    pKL = IntHKLtoPKL(pti, hKL);
    if (!pKL)
    {
        ERR("Invalid HKL %p!\n", hKL);
        return NULL;
    }

    if (uFlags & KLF_REORDER)
        IntReorderKeyboardLayouts(pWinSta, pKL);

    return co_UserActivateKeyboardLayout(pKL, uFlags, pWnd);
}

static BOOL APIENTRY
co_IntUnloadKeyboardLayoutEx(
    _Inout_ PWINSTATION_OBJECT pWinSta,
    _Inout_ PKL pKL,
    _In_ DWORD dwFlags)
{
    PKL pNextKL;
    USER_REFERENCE_ENTRY Ref1, Ref2;
    PTHREADINFO pti = gptiCurrent;

    if (pKL == gspklBaseLayout && !(dwFlags & UKL_NOACTIVATENEXT))
        return FALSE;

    UserRefObjectCo(pKL, &Ref1); /* Add reference */

    /* Regard as unloaded */
    UserMarkObjectDestroy(pKL);
    pKL->dwKL_Flags |= KL_UNLOAD;

    if (!(dwFlags & UKL_NOACTIVATENEXT) && pti->KeyboardLayout == pKL)
    {
        pNextKL = IntHKLtoPKL(pti, UlongToHandle(HKL_NEXT));
        if (pNextKL)
        {
            UserRefObjectCo(pNextKL, &Ref2); /* Add reference */
            co_UserActivateKeyboardLayout(pNextKL, dwFlags, NULL);
            UserDerefObjectCo(pNextKL); /* Release reference */
        }
    }

    if (gspklBaseLayout == pKL && pKL != pKL->pklNext)
    {
        /* Set next layout as default (FIXME: Use UserAssignmentLock?) */
        gspklBaseLayout = pKL->pklNext;
    }

    UserDerefObjectCo(pKL); /* Release reference */

    if (ISITHOOKED(WH_SHELL))
    {
        co_IntShellHookNotify(HSHELL_LANGUAGE, 0, 0);
        ghKLSentToShell = NULL;
    }

    return TRUE;
}

static BOOL APIENTRY
IntUnloadKeyboardLayout(_Inout_ PWINSTATION_OBJECT pWinSta, _In_ HKL hKL)
{
    PKL pKL = IntHKLtoPKL(gptiCurrent, hKL);
    if (!pKL)
    {
        ERR("Invalid HKL %p!\n", hKL);
        return FALSE;
    }
    return co_IntUnloadKeyboardLayoutEx(pWinSta, pKL, 0);
}

/// Invokes imm32!ImmLoadLayout and returns PIMEINFOEX
PIMEINFOEX FASTCALL co_UserImmLoadLayout(_In_ HKL hKL)
{
    PIMEINFOEX piiex;

    if (!IS_IME_HKL(hKL) && !IS_CICERO_MODE())
        return NULL;

    piiex = ExAllocatePoolWithTag(PagedPool, sizeof(IMEINFOEX), USERTAG_IME);
    if (!piiex)
        return NULL;

    if (!co_ClientImmLoadLayout(hKL, piiex))
    {
        ExFreePoolWithTag(piiex, USERTAG_IME);
        return NULL;
    }

    return piiex;
}

HKL APIENTRY
co_IntLoadKeyboardLayoutEx(
    IN OUT PWINSTATION_OBJECT pWinSta,
    IN HANDLE hSafeFile,
    IN HKL hOldKL,
    IN PUNICODE_STRING puszSafeKLID,
    IN HKL hNewKL,
    IN UINT Flags)
{
    PKL pOldKL, pNewKL;

    UNREFERENCED_PARAMETER(hSafeFile);

    if (hNewKL == NULL || (pWinSta->Flags & WSS_NOIO))
        return NULL;

    /* If hOldKL is specified, unload it and load new layput as default */
    if (hOldKL && hOldKL != hNewKL)
    {
        pOldKL = UserHklToKbl(hOldKL);
        if (pOldKL)
            UserUnloadKbl(pOldKL);
    }

    /* FIXME: It seems KLF_RESET is only supported for WINLOGON */

    /* Let's see if layout was already loaded. */
    pNewKL = UserHklToKbl(hNewKL);
    if (!pNewKL)
    {
        /* It wasn't, so load it. */
        pNewKL = co_UserLoadKbdLayout(puszSafeKLID, hNewKL);
        if (!pNewKL)
            return NULL;

        if (gspklBaseLayout)
        {
            /* Find last not unloaded layout */
            PKL pLastKL = gspklBaseLayout->pklPrev;
            while (pLastKL != gspklBaseLayout && (pLastKL->dwKL_Flags & KL_UNLOAD))
                pLastKL = pLastKL->pklPrev;

            /* Add new layout to the list */
            pNewKL->pklNext = pLastKL->pklNext;
            pNewKL->pklPrev = pLastKL;
            pNewKL->pklNext->pklPrev = pNewKL;
            pNewKL->pklPrev->pklNext = pNewKL;
        }
        else
        {
            /* This is the first layout */
            pNewKL->pklNext = pNewKL;
            pNewKL->pklPrev = pNewKL;
            gspklBaseLayout = pNewKL;
        }

        pNewKL->piiex = co_UserImmLoadLayout(hNewKL);
    }

    /* If this layout was prepared to unload, undo it */
    pNewKL->dwKL_Flags &= ~KL_UNLOAD;

    /* Reorder if necessary */
    if (Flags & KLF_REORDER)
        IntReorderKeyboardLayouts(pWinSta, pNewKL);

    /* Activate this layout in current thread */
    if (Flags & KLF_ACTIVATE)
        co_UserActivateKeyboardLayout(pNewKL, Flags, NULL);

    /* Send shell message */
    if (!(Flags & KLF_NOTELLSHELL))
        co_IntShellHookNotify(HSHELL_LANGUAGE, 0, (LPARAM)hNewKL);

    /* FIXME: KLF_REPLACELANG */

    return hNewKL;
}

HANDLE FASTCALL IntVerifyKeyboardFileHandle(HANDLE hFile)
{
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    Status = ObReferenceObjectByHandle(hFile, FILE_READ_DATA, NULL, UserMode,
                                       (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("0x%08X\n", Status);
        return NULL;
    }

    /* FIXME: Is the file in the system directory? */

    if (FileObject)
        ObDereferenceObject(FileObject);

    return hFile;
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
    PTHREADINFO pti;
    PLIST_ENTRY ListEntry;
    PKL pKl;

    pti = PsGetCurrentThreadWin32Thread();

    if (!dwThreadId)
    {
        pKl = pti->KeyboardLayout;
        return pKl ? pKl->hkl : NULL;
    }

    ListEntry = pti->rpdesk->PtiList.Flink;

    //
    // Search the Desktop Thread list for related Desktop active Threads.
    //
    while(ListEntry != &pti->rpdesk->PtiList)
    {
        pti = CONTAINING_RECORD(ListEntry, THREADINFO, PtiLink);

        if (PsGetThreadId(pti->pEThread) == UlongToHandle(dwThreadId))
        {
           pKl = pti->KeyboardLayout;
           return pKl ? pKl->hkl : NULL;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
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
    UINT ret = 0;
    PWINSTATION_OBJECT pWinSta;

    if (!pHklBuff)
        nBuff = 0;

    UserEnterShared();

    if (nBuff > MAXULONG / sizeof(HKL))
    {
        SetLastNtError(ERROR_INVALID_PARAMETER);
        goto Quit;
    }

    _SEH2_TRY
    {
        ProbeForWrite(pHklBuff, nBuff * sizeof(HKL), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        goto Quit;
    }
    _SEH2_END;

    pWinSta = IntGetProcessWindowStation(NULL);

    _SEH2_TRY
    {
        ret = IntGetKeyboardLayoutList(pWinSta, nBuff, pHklBuff);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        goto Quit;
    }
    _SEH2_END;

Quit:
    UserLeave();
    return ret;
}

/*
 * NtUserGetKeyboardLayoutName
 *
 * Returns KLID of current thread keyboard layout
 */
BOOL
APIENTRY
NtUserGetKeyboardLayoutName(
    _Inout_ PUNICODE_STRING pustrName)
{
    BOOL bRet = FALSE;
    PKL pKl;
    PTHREADINFO pti;
    UNICODE_STRING ustrNameSafe;
    NTSTATUS Status;

    UserEnterShared();

    pti = PsGetCurrentThreadWin32Thread();
    pKl = pti->KeyboardLayout;

    if (!pKl)
        goto cleanup;

    _SEH2_TRY
    {
        ProbeForWriteUnicodeString(pustrName);
        ustrNameSafe = *pustrName;

        ProbeForWrite(ustrNameSafe.Buffer, ustrNameSafe.MaximumLength, 1);

        if (IS_IME_HKL(pKl->hkl))
        {
            Status = RtlIntegerToUnicodeString(HandleToUlong(pKl->hkl), 16, &ustrNameSafe);
        }
        else
        {
            if (ustrNameSafe.MaximumLength < KL_NAMELENGTH * sizeof(WCHAR))
            {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                goto cleanup;
            }

            /* FIXME: Do not use awchKF */
            ustrNameSafe.Length = 0;
            Status = RtlAppendUnicodeToString(&ustrNameSafe, pKl->spkf->awchKF);
        }

        if (NT_SUCCESS(Status))
        {
            *pustrName = ustrNameSafe;
            bRet = TRUE;
        }
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
 *
 * NOTE: We adopt a different design from Microsoft's one due to security reason.
 *       We don't use the 3rd parameter of NtUserLoadKeyboardLayoutEx.
 *       See https://seclists.org/fulldisclosure/2012/Jul/137
 */
HKL
NTAPI
NtUserLoadKeyboardLayoutEx(
    IN HANDLE hFile,
    IN DWORD offTable,
    IN PVOID pTables,
    IN HKL hOldKL,
    IN PUNICODE_STRING puszKLID,
    IN DWORD dwNewKL,
    IN UINT Flags)
{
    HKL hRetKL;
    WCHAR Buffer[KL_NAMELENGTH];
    UNICODE_STRING uszSafeKLID;
    PWINSTATION_OBJECT pWinSta;
    HANDLE hSafeFile;

    UNREFERENCED_PARAMETER(offTable);
    UNREFERENCED_PARAMETER(pTables);

    if (Flags & ~(KLF_ACTIVATE|KLF_NOTELLSHELL|KLF_REORDER|KLF_REPLACELANG|
                  KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS|KLF_UNLOADPREVIOUS|
                  KLF_RESET|KLF_SHIFTLOCK))
    {
        ERR("Invalid flags: %x\n", Flags);
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    RtlInitEmptyUnicodeString(&uszSafeKLID, Buffer, sizeof(Buffer));
    _SEH2_TRY
    {
        ProbeForRead(puszKLID, sizeof(*puszKLID), 1);
        ProbeForRead(puszKLID->Buffer, sizeof(puszKLID->Length), 1);
        RtlCopyUnicodeString(&uszSafeKLID, puszKLID);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;

    UserEnterExclusive();

    hSafeFile = (hFile ? IntVerifyKeyboardFileHandle(hFile) : NULL);
    pWinSta = IntGetProcessWindowStation(NULL);
    hRetKL = co_IntLoadKeyboardLayoutEx(pWinSta,
                                        hSafeFile,
                                        hOldKL,
                                        &uszSafeKLID,
                                        UlongToHandle(dwNewKL),
                                        Flags);
    if (hSafeFile)
        ZwClose(hSafeFile);

    UserLeave();
    return hRetKL;
}

/*
 * NtUserActivateKeyboardLayout
 *
 * Activates specified layout for thread or process
 */
HKL
NTAPI
NtUserActivateKeyboardLayout(
    HKL hKL,
    ULONG Flags)
{
    PWINSTATION_OBJECT pWinSta;
    HKL hOldKL;

    UserEnterExclusive();

    /* FIXME */

    pWinSta = IntGetProcessWindowStation(NULL);
    hOldKL = co_IntActivateKeyboardLayout(pWinSta, hKL, Flags, NULL);
    UserLeave();

    return hOldKL;
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
    BOOL ret;
    PWINSTATION_OBJECT pWinSta;

    UserEnterExclusive();

    pWinSta = IntGetProcessWindowStation(NULL);
    ret = IntUnloadKeyboardLayout(pWinSta, hKl);

    UserLeave();
    return ret;
}

/* EOF */
