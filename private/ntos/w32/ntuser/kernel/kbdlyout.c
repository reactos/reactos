/**************************** Module Header ********************************\
* Module Name: kbdlyout.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Keyboard Layout API
*
* History:
* 04-14-92 IanJa      Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Workers (forward declarations)
 */
BOOL xxxInternalUnloadKeyboardLayout(PWINDOWSTATION, PKL, UINT);
VOID ReorderKeyboardLayouts(PWINDOWSTATION, PKL);

/*
 * Note that this only works for sections < 64K
 */
#define FIXUP_PTR(p, pBase) ((p) ? (p) = (PVOID)((PBYTE)pBase + (WORD)(ULONG_PTR)(p)) : 0)


/****************************************************************************\
* HKLtoPKL
*
* pti   - thread to look in
* hkl   - HKL_NEXT or HKL_PREV
*         Finds the the next/prev LOADED layout, NULL if none.
*         (Starts from the pti's active layout, may return pklActive itself)
*       - a real HKL (Keyboard Layout handle):
*         Finds the kbd layout struct (loaded or not), NULL if no match found.
*
* History:
* 1997-02-05 IanJa     added pti parameter
\****************************************************************************/
PKL HKLtoPKL(
    PTHREADINFO pti,
    HKL hkl)
{
    PKL pklActive;
    PKL pkl;

    UserAssert(pti != NULL);
    if ((pklActive = pti->spklActive) == NULL) {
        return NULL;
    }

    pkl = pklActive;

    if (hkl == (HKL)HKL_PREV) {
        do {
            pkl = pkl->pklPrev;
            if (!(pkl->dwKL_Flags & KL_UNLOADED)) {
                return pkl;
            }
        } while (pkl != pklActive);
        return NULL;
    } else if (hkl == (HKL)HKL_NEXT) {
        do {
            pkl = pkl->pklNext;
            if (!(pkl->dwKL_Flags & KL_UNLOADED)) {
                return pkl;
            }
        } while (pkl != pklActive);
        return NULL;
    }

    /*
     * Find the pkl for this hkl.
     * If the kbd layout isn't specified (in the HIWORD), ignore it and look
     * for a Locale match only.  (Mohamed Hamid's fix for Word bug)
     */
    if (HandleToUlong(hkl) & 0xffff0000) {
        do {
            if (pkl->hkl == hkl) {
                return pkl;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklActive);
    } else {
        do {
            if (LOWORD(HandleToUlong(pkl->hkl)) == LOWORD(HandleToUlong(hkl))) {
                return pkl;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklActive);
    }

    return NULL;
}


/***************************************************************************\
* ReadLayoutFile
*
* Maps layout file into memory and initializes layout table.
*
* History:
* 01-10-95 JimA         Created.
\***************************************************************************/

PKBDTABLES ReadLayoutFile(
    PKBDFILE pkf,
    HANDLE hFile,
    UINT offTable,
    PKBDNLSTABLES *ppNlsTables)
{
    HANDLE hmap;
    SIZE_T ulViewSize = 0;
    NTSTATUS Status;
    PIMAGE_DOS_HEADER DosHdr;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    ULONG NumberOfSubsections;
    ULONG OffsetToSectionTable;
    PBYTE pBaseDst, pBaseVirt;
    PKBDTABLES pktNew = NULL;
    DWORD dwDataSize;
    PKBDNLSTABLES pknlstNew = NULL;
    UINT  offNlsTable = HIWORD(offTable);
    /*
     * Mask off hiword.
     */
    offTable &= 0x0000FFFF;

    /*
     * Initialize KbdNlsTables with NULL.
     */
    *ppNlsTables = NULL;

    /*
     * Map the layout file into memory
     */
    DosHdr = NULL;
    Status = ZwCreateSection(&hmap, SECTION_ALL_ACCESS, NULL,
                             NULL, PAGE_READONLY, SEC_COMMIT, hFile);
    if (!NT_SUCCESS(Status))
        return NULL;

    Status = ZwMapViewOfSection(hmap, NtCurrentProcess(), &DosHdr, 0, 0, NULL,
                                &ulViewSize, ViewShare, 0, PAGE_READONLY);
    if (!NT_SUCCESS(Status)) {
        goto exitread;
    }

    /*
     * HACK Part 2!  We find the .data section in the file header
     * and by subtracting the virtual address from offTable find
     * the offset in the section of the layout table.
     */
    NtHeader = (PIMAGE_NT_HEADERS)((PBYTE)DosHdr + (ULONG)DosHdr->e_lfanew);

    /*
     * Build the next subsections.
     */
    NumberOfSubsections = NtHeader->FileHeader.NumberOfSections;

    /*
     * At this point the object table is read in (if it was not
     * already read in) and may displace the image header.
     */
    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              NtHeader->FileHeader.SizeOfOptionalHeader;
    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PBYTE)NtHeader +
                                OffsetToSectionTable);

    while (NumberOfSubsections > 0) {
        if (strcmp(SectionTableEntry->Name, ".data") == 0)
            break;

        SectionTableEntry++;
        NumberOfSubsections--;
    }
    if (NumberOfSubsections == 0) {
        goto exitread;
    }

    /*
     * We found the section, now compute starting offset and the table size.
     */
    offTable -= SectionTableEntry->VirtualAddress;
    dwDataSize = SectionTableEntry->Misc.VirtualSize;

    /*
     * Allocate layout table and copy from file.
     */
    pBaseDst = UserAllocPool(dwDataSize, TAG_KBDTABLE);
    if (pBaseDst != NULL) {
        VK_TO_WCHAR_TABLE *pVkToWcharTable;
        VSC_LPWSTR *pKeyName;
        LPWSTR *lpDeadKey;

        pkf->hBase = (HANDLE)pBaseDst;
        RtlMoveMemory(pBaseDst, (PBYTE)DosHdr +
                SectionTableEntry->PointerToRawData, dwDataSize);

        if (ISTS()) {
            pkf->Size = dwDataSize; // For shadow hotkey processing
        }

        /*
         * Compute table address and fixup pointers in table.
         */
        pktNew = (PKBDTABLES)(pBaseDst + offTable);

        /*
         * The address in the data section has the virtual address
         * added in, so we need to adjust the fixup pointer to
         * compensate.
         */
        pBaseVirt = pBaseDst - SectionTableEntry->VirtualAddress;

        FIXUP_PTR(pktNew->pCharModifiers, pBaseVirt);
        FIXUP_PTR(pktNew->pCharModifiers->pVkToBit, pBaseVirt);
        if (FIXUP_PTR(pktNew->pVkToWcharTable, pBaseVirt)) {
            for (pVkToWcharTable = pktNew->pVkToWcharTable;
                    pVkToWcharTable->pVkToWchars != NULL; pVkToWcharTable++)
                FIXUP_PTR(pVkToWcharTable->pVkToWchars, pBaseVirt);
        }
        FIXUP_PTR(pktNew->pDeadKey, pBaseVirt);
        /*
         * Version 1 layouts support ligatures.
         */
        if (GET_KBD_VERSION(pktNew)) {
            FIXUP_PTR(pktNew->pLigature, pBaseVirt);
        }
        if (FIXUP_PTR(pktNew->pKeyNames, pBaseVirt)) {
            for (pKeyName = pktNew->pKeyNames; pKeyName->vsc != 0; pKeyName++)
                FIXUP_PTR(pKeyName->pwsz, pBaseVirt);
        }
        if (FIXUP_PTR(pktNew->pKeyNamesExt, pBaseVirt)) {
            for (pKeyName = pktNew->pKeyNamesExt; pKeyName->vsc != 0; pKeyName++)
                FIXUP_PTR(pKeyName->pwsz, pBaseVirt);
        }
        if (FIXUP_PTR(pktNew->pKeyNamesDead, pBaseVirt)) {
            for (lpDeadKey = pktNew->pKeyNamesDead; *lpDeadKey != NULL;
                    lpDeadKey++)
                FIXUP_PTR(*lpDeadKey, pBaseVirt);
        }
        FIXUP_PTR(pktNew->pusVSCtoVK, pBaseVirt);
        FIXUP_PTR(pktNew->pVSCtoVK_E0, pBaseVirt);
        FIXUP_PTR(pktNew->pVSCtoVK_E1, pBaseVirt);

        if (offNlsTable) {
            /*
             * Compute table address and fixup pointers in table.
             */
            offNlsTable -= SectionTableEntry->VirtualAddress;
            pknlstNew = (PKBDNLSTABLES)(pBaseDst + offNlsTable);

            /*
             * Fixup the address.
             */
            FIXUP_PTR(pknlstNew->pVkToF, pBaseVirt);
            FIXUP_PTR(pknlstNew->pusMouseVKey, pBaseVirt);

            /*
             * Save the pointer.
             */
            *ppNlsTables = pknlstNew;

        #if DBG_FE
            {
                UINT NumOfVkToF = pknlstNew->NumOfVkToF;

                DbgPrint("NumOfVkToF - %d\n",NumOfVkToF);

                while(NumOfVkToF) {
                    DbgPrint("VK = %x\n",pknlstNew->pVkToF[NumOfVkToF-1].Vk);
                    NumOfVkToF--;
                }
            }
        #endif  // DBG_FE
        }
    }

exitread:

    /*
     * Unmap and release the mapped section.
     */
    ZwUnmapViewOfSection(NtCurrentProcess(), DosHdr);
    ZwClose(hmap);

    return pktNew;
}

PKBDTABLES PrepareFallbackKeyboardFile(PKBDFILE pkf)
{
    PBYTE pBaseDst;

    pBaseDst = UserAllocPool(sizeof(KBDTABLES), TAG_KBDTABLE);
    if (pBaseDst != NULL) {
        RtlCopyMemory(pBaseDst, &KbdTablesFallback, sizeof KbdTablesFallback);
        // Note: Unlike ReadLayoutFile(),
        // we don't need to fix up pointers in struct KBDFILE.
    }
    pkf->hBase = (HANDLE)pBaseDst;
    pkf->pKbdNlsTbl = NULL;
    return (PKBDTABLES)pBaseDst;
}


/***************************************************************************\
* LoadKeyboardLayoutFile
*
* History:
* 10-29-95 GregoryW         Created.
\***************************************************************************/

PKBDFILE LoadKeyboardLayoutFile(
    HANDLE hFile,
    UINT offTable,
    LPCWSTR pwszKLID)
{
    PKBDFILE pkf = gpkfList;

    if (pkf) {
        int iCmp;

        do {
            iCmp = wcscmp(pkf->awchKF, pwszKLID);
            if (iCmp == 0) {

                /*
                 * The layout is already loaded.
                 */
                return pkf;
            }
            pkf = pkf->pkfNext;
        } while (pkf);
    }

    /*
     * Allocate a new Keyboard File structure.
     */
    pkf = (PKBDFILE)HMAllocObject(NULL, NULL, TYPE_KBDFILE, sizeof(KBDFILE));
    if (!pkf) {
        RIPMSG0(RIP_WARNING, "Keyboard Layout File: out of memory");
        return (PKBDFILE)NULL;
    }

    /*
     * Load layout table.
     */
    if (hFile != NULL) {
        /*
         * Load NLS layout table also...
         */
        pkf->pKbdTbl = ReadLayoutFile(pkf, hFile, offTable, &(pkf->pKbdNlsTbl));
    } else {
        /*
         * We failed to open the keyboard layout file in client side
         * because the dll was missing.
         * If this ever happens, we used to fail creating
         * a window station, but we should allow a user
         * at least to boot the system.
         */
        pkf->pKbdTbl = PrepareFallbackKeyboardFile(pkf);
        // Note: pkf->pKbdNlsTbl has been NULL'ed in PrepareFallbackKeyboardFile()
    }

    if (pkf->pKbdTbl == NULL) {
        HMFreeObject(pkf);
        return (PKBDFILE)NULL;
    }

    wcsncpycch(pkf->awchKF, pwszKLID, sizeof(pkf->awchKF) / sizeof(WCHAR));

    /*
     * Put keyboard layout file at front of list.
     */
    pkf->pkfNext = gpkfList;
    gpkfList = pkf;

    return pkf;
}

/***************************************************************************\
* RemoveKeyboardLayoutFile
*
* History:
* 10-29-95 GregoryW         Created.
\***************************************************************************/
VOID RemoveKeyboardLayoutFile(
    PKBDFILE pkf)
{
    PKBDFILE pkfPrev, pkfCur;

    // FE: NT4 SP4 #107809
    if (gpKbdTbl == pkf->pKbdTbl) {
        gpKbdTbl = &KbdTablesFallback;
    }
    if (gpKbdNlsTbl == pkf->pKbdNlsTbl) {
        gpKbdNlsTbl = NULL;
    }

    /*
     * Good old linked list management 101
     */
    if (pkf == gpkfList) {
        /*
         * Head of the list.
         */
        gpkfList = pkf->pkfNext;
        return;
    }
    pkfPrev = gpkfList;
    pkfCur = gpkfList->pkfNext;
    while (pkf != pkfCur) {
        pkfPrev = pkfCur;
        pkfCur = pkfCur->pkfNext;
    }
    /*
     * Found it!
     */
    pkfPrev->pkfNext = pkfCur->pkfNext;
}

/***************************************************************************\
* DestroyKF
*
* Called when a keyboard layout file is destroyed due to an unlock.
*
* History:
* 24-Feb-1997 adams     Created.
\***************************************************************************/

void
DestroyKF(PKBDFILE pkf)
{
    if (!HMMarkObjectDestroy(pkf))
        return;

    RemoveKeyboardLayoutFile(pkf);
    UserFreePool(pkf->hBase);
    HMFreeObject(pkf);
}

INT GetThreadsWithPKL(
    PTHREADINFO **ppptiList,
    PKL pkl)
{
    PTHREADINFO     ptiT, *pptiT, *pptiListAllocated;
    INT             cThreads, cThreadsAllocated;
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdesk;
    PLIST_ENTRY     pHead, pEntry;

    if (ppptiList != NULL)
        *ppptiList = NULL;

    cThreads = 0;

    /*
     * allocate a first list for 128 entries
     */
    cThreadsAllocated = 128;
    pptiListAllocated = UserAllocPool(cThreadsAllocated * sizeof(PTHREADINFO),
                            TAG_SYSTEM);

    if (pptiListAllocated == NULL) {
        RIPMSG0(RIP_WARNING, "GetPKLinThreads: out of memory");
        return 0;
    }

    // for all the winstations
    for (pwinsta = grpWinStaList; pwinsta != NULL ; pwinsta = pwinsta->rpwinstaNext) {

        // for all the desktops in that winstation
        for (pdesk = pwinsta->rpdeskList; pdesk != NULL ; pdesk = pdesk->rpdeskNext) {

            pHead = &pdesk->PtiList;

            // for all the threads in that desktop
            for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

                ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

                if (ptiT == NULL) {
                    continue;
                }

                if (pkl && (pkl != ptiT->spklActive)) { // #99321 cmp pkls, not hkls?
                    continue;
                }

                if (cThreads == cThreadsAllocated) {

                    cThreadsAllocated += 128;

                    pptiT = UserReAllocPool(pptiListAllocated,
                                    cThreads * sizeof(PTHREADINFO),
                                    cThreadsAllocated * sizeof(PTHREADINFO),
                                    TAG_SYSTEM);

                    if (pptiT == NULL) {
                        RIPMSG0(RIP_ERROR, "GetPKLinThreads: Out of memory");
                        UserFreePool(pptiListAllocated);
                        return 0;
                    }

                    pptiListAllocated = pptiT;

                }

                pptiListAllocated[cThreads++] = ptiT;
            }
        }
    }

    /*
     * add CSRSS threads
     */
    for (ptiT = PpiFromProcess(gpepCSRSS)->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {

        if (pkl && (pkl != ptiT->spklActive)) { // #99321 cmp pkls, not hkls?
            continue;
        }

        if (cThreads == cThreadsAllocated) {

            cThreadsAllocated += 128;

            pptiT = UserReAllocPool(pptiListAllocated,
                            cThreads * sizeof(PTHREADINFO),
                            cThreadsAllocated * sizeof(PTHREADINFO),
                            TAG_SYSTEM);

            if (pptiT == NULL) {
                RIPMSG0(RIP_ERROR, "GetPKLinThreads: Out of memory");
                UserFreePool(pptiListAllocated);
                return 0;
            }

            pptiListAllocated = pptiT;

        }

        pptiListAllocated[cThreads++] = ptiT;
    }

    if (cThreads == 0) {
        UserFreePool(pptiListAllocated);
    } else if (ppptiList != NULL) {
        *ppptiList = pptiListAllocated;
    } else {
        UserFreePool(pptiListAllocated);
    }

    return cThreads;
}


VOID xxxSetPKLinThreads(
    PKL pklNew,
    PKL pklToBeReplaced)
{
    PTHREADINFO *pptiList;
    INT cThreads, i;

    UserAssert(pklNew != pklToBeReplaced);

    CheckLock(pklNew);
    CheckLock(pklToBeReplaced);

    cThreads = GetThreadsWithPKL(&pptiList, pklToBeReplaced);

    /*
     * Will the foreground thread's keyboard layout change?
     */
    if (pklNew && gptiForeground && gptiForeground->spklActive == pklToBeReplaced) {
        ChangeForegroundKeyboardTable(pklToBeReplaced, pklNew);
    }

    if (pptiList != NULL) {
        if (pklToBeReplaced == NULL) {
            for (i = 0; i < cThreads; i++) {
                Lock(&pptiList[i]->spklActive, pklNew);
            }
        } else {
            /*
             * This is a replace. First, deactivate the *replaced* IME by
             * activating the pklNew. Second, unload the *replaced* IME.
             */
            xxxImmActivateAndUnloadThreadsLayout(pptiList, cThreads, NULL,
                                                 pklNew, HandleToUlong(pklToBeReplaced->hkl));
        }
        UserFreePool(pptiList);
    }

    /*
     * If this is a replace, link the new layout immediately after the
     * layout being replaced.  This maintains ordering of layouts when
     * the *replaced* layout is unloaded.  The input locale panel in the
     * regional settings applet depends on this.
     */
    if (pklToBeReplaced) {
        if (pklToBeReplaced->pklNext == pklNew) {
            /*
             * Ordering already correct.  Nothing to do.
             */
            return;
        }
        /*
         * Move new layout immediately after layout being replaced.
         *   1. Remove new layout from current position.
         *   2. Update links in new layout.
         *   3. Link new layout into desired position.
         */
        pklNew->pklPrev->pklNext = pklNew->pklNext;
        pklNew->pklNext->pklPrev = pklNew->pklPrev;

        pklNew->pklNext = pklToBeReplaced->pklNext;
        pklNew->pklPrev = pklToBeReplaced;

        pklToBeReplaced->pklNext->pklPrev = pklNew;
        pklToBeReplaced->pklNext = pklNew;
    }
}

VOID xxxFreeImeKeyboardLayouts(
    PWINDOWSTATION pwinsta)
{
    PTHREADINFO *pptiList;
    INT cThreads;

    if (pwinsta->dwWSF_Flags & WSF_NOIO)
        return;

    /*
     * should make GetThreadsWithPKL aware of pwinsta?
     */
    cThreads = GetThreadsWithPKL(&pptiList, NULL);
    if (pptiList != NULL) {
        xxxImmUnloadThreadsLayout(pptiList, cThreads, NULL, IFL_UNLOADIME);
        UserFreePool(pptiList);
    }

    return;
}

/***************************************************************************\
* xxxLoadKeyboardLayoutEx
*
* History:
\***************************************************************************/

HKL xxxLoadKeyboardLayoutEx(
    PWINDOWSTATION pwinsta,
    HANDLE hFile,
    HKL hklToBeReplaced,
    UINT offTable,
    LPCWSTR pwszKLID,
    UINT KbdInputLocale,
    UINT Flags)
{
    PKL pkl, pklFirst, pklToBeReplaced;
    PKBDFILE pkf;
    CHARSETINFO cs;
    TL tlpkl;
    PTHREADINFO ptiCurrent;
    UNICODE_STRING strLcidKF;
    LCID lcidKF;
    BOOL bCharSet;
    PIMEINFOEX piiex;

    /*
     * If the windowstation does not do I/O, don't load the
     * layout.  Also check KdbInputLocale for #307132
     */
    if ((KbdInputLocale == 0) || (pwinsta->dwWSF_Flags & WSF_NOIO)) {
        return NULL;
    }

    /*
     * If hklToBeReplaced is non-NULL make sure it's valid.
     *    NOTE: may want to verify they're not passing HKL_NEXT or HKL_PREV.
     */
    ptiCurrent = PtiCurrent();
    if (hklToBeReplaced && !(pklToBeReplaced = HKLtoPKL(ptiCurrent, hklToBeReplaced))) {
        return NULL;
    }
    if (KbdInputLocale == HandleToUlong(hklToBeReplaced)) {
        /*
         * Replacing a layout/lang pair with itself.  Nothing to do.
         */
        return pklToBeReplaced->hkl;
    }

    if (Flags & KLF_RESET) {
        /*
         * Only WinLogon can use this flag
         */
        if (ptiCurrent->pEThread->Cid.UniqueProcess != gpidLogon) {
             RIPERR0(ERROR_INVALID_FLAGS, RIP_WARNING,
                     "Invalid flag passed to LoadKeyboardLayout" );
             return NULL;
        }
        xxxFreeImeKeyboardLayouts(pwinsta);
        /*
         * Make sure we don't lose track of the left-over layouts
         * They have been unloaded, but are still in use by some threads).
         * The FALSE will prevent xxxFreeKeyboardLayouts from unlocking the
         * unloaded layouts.
         */
        xxxFreeKeyboardLayouts(pwinsta, FALSE);
    }

    /*
     * Does this hkl already exist?
     */
    pkl = pklFirst = pwinsta->spklList;

    if (pkl) {
        do {
            if (pkl->hkl == (HKL)IntToPtr( KbdInputLocale )) {
               /*
                * The hkl already exists.
                */

               /*
                * If it is unloaded (but not yet destroyed because it is
                * still is use), recover it.
                */
               if (pkl->dwKL_Flags & KL_UNLOADED) {
                   // stop it from being destroyed if not is use.
                   PHE phe = HMPheFromObject(pkl);
                   // An unloaded layout must be marked for destroy.
                   UserAssert(phe->bFlags & HANDLEF_DESTROY);
                   phe->bFlags &= ~HANDLEF_DESTROY;
#if DBG
                   phe->bFlags &= ~HANDLEF_MARKED_OK;
#endif
                   pkl->dwKL_Flags &= ~KL_UNLOADED;
               } else if (!(Flags & KLF_RESET)) {
                   /*
                    * If it was already loaded and we didn't change all layouts
                    * with KLF_RESET, there is nothing to tell the shell about
                    */
                   Flags &= ~KLF_NOTELLSHELL;
               }

               goto AllPresentAndCorrectSir;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklFirst);
    }

    if (IS_IME_KBDLAYOUT((HKL)IntToPtr( KbdInputLocale ))) {
        /*
         * This is an IME keyboard layout, do a callback
         * to read the extended IME information structure.
         * Note: We can't fail the call so easily if
         *       KLF_RESET is specified.
         */
        piiex = xxxImmLoadLayout((HKL)IntToPtr( KbdInputLocale ));
        if (piiex == NULL && !(Flags & KLF_RESET)) {
            RIPMSG1(RIP_WARNING,
                  "Keyboard Layout: xxxImmLoadLayout(%lx) failed", KbdInputLocale);
            return NULL;
        }
    } else {
        piiex = NULL;
    }

    /*
     * Get the system font's font signature.  These are 64-bit FS_xxx values,
     * but we are only asking for an  ANSI ones, so gSystemFS is just a DWORD.
     * gSystemFS is consulted when posting WM_INPUTLANGCHANGEREQUEST (input.c)
     */
    if (gSystemFS == 0) {
        LCID lcid;

        ZwQueryDefaultLocale(FALSE, &lcid);
        if (xxxClientGetCharsetInfo(lcid, &cs)) {
            gSystemFS = cs.fs.fsCsb[0];
            gSystemCPCharSet = (BYTE)cs.ciCharset;
        } else {
            gSystemFS = 0xFFFF;
            gSystemCPCharSet = ANSI_CHARSET;
        }
    }

    /*
     * Use the Keyboard Layout's LCID to calculate the charset, codepage etc,
     * so that characters from that layout don't just becomes ?s if the input
     * locale doesn't match.  This allows "dumb" applications to display the
     * text if the user chooses the right font.
     * We can't just use the HIWORD of KbdInputLocale because if a variant
     * keyboard layout was chosen, this will be something like F008 - have to
     * look inside the KF to get the real LCID of the kbdfile: this will be
     * something like L"00010419", and we want the last 4 digits.
     */
    RtlInitUnicodeString(&strLcidKF, pwszKLID + 4);
    RtlUnicodeStringToInteger(&strLcidKF, 16, (PULONG)&lcidKF);
    bCharSet = xxxClientGetCharsetInfo(lcidKF, &cs);

    /*
     * Keyboard Layout Handle object does not exist.  Load keyboard layout file,
     * if not already loaded.
     */
    if (!(pkf = LoadKeyboardLayoutFile(hFile, offTable, pwszKLID))) {
        goto freePiiex;
    }
    /*
     * Allocate a new Keyboard Layout structure (hkl)
     */
    pkl = (PKL)HMAllocObject(NULL, NULL, TYPE_KBDLAYOUT, sizeof(KL));
    if (!pkl) {
        RIPMSG0(RIP_WARNING, "Keyboard Layout: out of memory");
        UserFreePool(pkf->hBase);
        HMMarkObjectDestroy(pkf);
        HMUnlockObject(pkf);
freePiiex:
        if (piiex) {
            UserFreePool(piiex);
        }
        return NULL;
    }

    /*
     * Link to itself in case we have to DestroyKL
     */
    pkl->pklNext = pkl;
    pkl->pklPrev = pkl;

    /*
     * Init KL
     */
    pkl->dwKL_Flags = 0;
    pkl->wchDiacritic = 0;
    pkl->hkl = (HKL)IntToPtr( KbdInputLocale );
    Lock(&pkl->spkf, pkf);

    pkl->spkf->pKbdTbl->fLocaleFlags |= KLL_LAYOUT_ATTR_FROM_KLF(Flags);

    pkl->piiex = piiex;

    if (bCharSet) {
        pkl->CodePage = (WORD)cs.ciACP;
        pkl->dwFontSigs = cs.fs.fsCsb[1];   // font signature mask (FS_xxx values)
        pkl->iBaseCharset = cs.ciCharset;   // charset value
    } else {
        pkl->CodePage = CP_ACP;
        pkl->dwFontSigs = FS_LATIN1;
        pkl->iBaseCharset = ANSI_CHARSET;
    }

    /*
     * Insert KL in the double-linked circular list, at the end.
     */
    pklFirst = pwinsta->spklList;
    if (pklFirst == NULL) {
        Lock(&pwinsta->spklList, pkl);
    } else {
        pkl->pklNext = pklFirst;
        pkl->pklPrev = pklFirst->pklPrev;
        pklFirst->pklPrev->pklNext = pkl;
        pklFirst->pklPrev = pkl;
    }

AllPresentAndCorrectSir:

    // FE_IME
    ThreadLockAlwaysWithPti(ptiCurrent, pkl, &tlpkl);

    if (hklToBeReplaced) {
        TL tlPKLToBeReplaced;
        ThreadLockAlwaysWithPti(ptiCurrent, pklToBeReplaced, &tlPKLToBeReplaced);
        xxxSetPKLinThreads(pkl, pklToBeReplaced);
        xxxInternalUnloadKeyboardLayout(pwinsta, pklToBeReplaced, KLF_INITTIME);
        ThreadUnlock(&tlPKLToBeReplaced);
    }

    if (Flags & KLF_REORDER) {
        ReorderKeyboardLayouts(pwinsta, pkl);
    }

    if (!(Flags & KLF_NOTELLSHELL) && IsHooked(PtiCurrent(), WHF_SHELL)) {
        xxxCallHook(HSHELL_LANGUAGE, (WPARAM)NULL, (LPARAM)0, WH_SHELL);
        gLCIDSentToShell = 0;
    }

    if (Flags & KLF_ACTIVATE) {
        TL tlPKL;
        ThreadLockAlwaysWithPti(ptiCurrent, pkl, &tlPKL);
        xxxInternalActivateKeyboardLayout(pkl, Flags, NULL);
        ThreadUnlock(&tlPKL);
    }

    if (Flags & KLF_RESET) {
        RIPMSG2(RIP_VERBOSE, "Flag & KLF_RESET, locking gspklBaseLayout(%08x) with new kl(%08x)",
                gspklBaseLayout ? gspklBaseLayout->hkl : 0,
                pkl->hkl);
        Lock(&gspklBaseLayout, pkl);
        xxxSetPKLinThreads(pkl, NULL);
    }

    /*
     * Use the hkl as the layout handle
     * If the KL is freed somehow, return NULL for safety. -- ianja --
     */
    pkl = ThreadUnlock(&tlpkl);
    if (pkl == NULL) {
        return NULL;
    }
    return pkl->hkl;
}

HKL xxxActivateKeyboardLayout(
    PWINDOWSTATION pwinsta,
    HKL hkl,
    UINT Flags,
    PWND pwnd)
{
    PKL pkl;
    TL tlPKL;
    HKL hklRet;
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckLock(pwnd);

    pkl = HKLtoPKL(ptiCurrent, hkl);
    if (pkl == NULL) {
        return 0;
    }

    if (Flags & KLF_REORDER) {
        ReorderKeyboardLayouts(pwinsta, pkl);
    }

    ThreadLockAlwaysWithPti(ptiCurrent, pkl, &tlPKL);
    hklRet = xxxInternalActivateKeyboardLayout(pkl, Flags, pwnd);
    ThreadUnlock(&tlPKL);
    return hklRet;
}

VOID ReorderKeyboardLayouts(
    PWINDOWSTATION pwinsta,
    PKL pkl)
{
    PKL pklFirst = pwinsta->spklList;

    if (pwinsta->dwWSF_Flags & WSF_NOIO) {
        RIPMSG1(RIP_ERROR, "ReorderKeyboardLayouts called for non-interactive windowstation %#p",
                pwinsta);
        return;
    }

    UserAssert(pklFirst != NULL);

    /*
     * If the layout is already at the front of the list there's nothing to do.
     */
    if (pkl == pklFirst) {
        return;
    }
    /*
     * Cut pkl from circular list:
     */
    pkl->pklPrev->pklNext = pkl->pklNext;
    pkl->pklNext->pklPrev = pkl->pklPrev;

    /*
     * Insert pkl at front of list
     */
    pkl->pklNext = pklFirst;
    pkl->pklPrev = pklFirst->pklPrev;

    pklFirst->pklPrev->pklNext = pkl;
    pklFirst->pklPrev = pkl;

    Lock(&pwinsta->spklList, pkl);
}

VOID ChangeForegroundKeyboardTable(PKL pklOld, PKL pklNew)
{
    CheckCritIn();
    UserAssert(pklNew != NULL);

    if (pklOld == pklNew || (pklOld != NULL && pklOld->spkf == pklNew->spkf)) {
        return;
    }

    // Manage the VK_KANA toggle key for Japanese KL.
    // Since VK_HANGUL and VK_KANA share the same VK value and
    // VK_KANA is a toggle key, when keyboard layouts are switched,
    // VK_KANA toggle status should be restored.

    //
    // If:
    // 1) Old and New keyboard layouts are both Japanese, do nothing.
    // 2) Old and New keyboard layouts are not Japanese, do nothing.
    // 3) Old keyboard is Japanese and new one is not, clear the KANA toggle.
    // 4) New keyboard is Japanese and old one is not, restore the KANA toggle.
    //

    if (pklOld && JAPANESE_KBD_LAYOUT(pklOld->hkl)) {
        if (!JAPANESE_KBD_LAYOUT(pklNew->hkl)) {
            // Old keyboard layout is Japanese and the new one is not Japanese,
            // so we save the current KANA toggle status and clear it.
            gfKanaToggle = (TestAsyncKeyStateToggle(VK_KANA) != 0);
            RIPMSG0(RIP_VERBOSE, "Old kbd is JPN. VK_KANA toggle is being cleared.\n");
            ClearAsyncKeyStateToggle(VK_KANA);
            ClearRawKeyToggle(VK_KANA);
            UpdateKeyLights(TRUE);
        }
    } else if (JAPANESE_KBD_LAYOUT(pklNew->hkl)) {
        // Previous keyboard layout does not exist or is not Japanese,
        // and the new one is Japanese.
        // Have to restore the KANA toggle status.
        RIPMSG0(RIP_VERBOSE, "New kbd is JPN. ");
        if (gfKanaToggle) {
            RIPMSG0(RIP_VERBOSE, "VK_KANA is being set.\n");
            SetAsyncKeyStateToggle(VK_KANA);
            SetRawKeyToggle(VK_KANA);
            if (gptiForeground && gptiForeground->pq) {
                SetKeyStateToggle(gptiForeground->pq, VK_KANA);
            }
        } else {
            RIPMSG0(RIP_VERBOSE, "VK_KANA is beging cleared.\n");
            ClearAsyncKeyStateToggle(VK_KANA);
            ClearRawKeyToggle(VK_KANA);
            if (gptiForeground && gptiForeground->pq) {
                ClearKeyStateToggle(gptiForeground->pq, VK_KANA);
            }
        }
        UpdateKeyLights(TRUE);
    }

    /*
     * Set gpKbdTbl so foreground thread processes AltGr appropriately
     */
    gpKbdTbl = pklNew->spkf->pKbdTbl;
    if (ISTS()) {
        ghKbdTblBase = pklNew->spkf->hBase;
        guKbdTblSize = pklNew->spkf->Size;
    }

    UserAssert(pklNew);
    TAGMSG2(DBGTAG_IMM, "ChangeForegroundKeyboardTable:Changing KL NLS Table: prev HKL=%x to new HKL=%x\n", pklOld ? pklOld->hkl : 0, pklNew->hkl);
    TAGMSG1(DBGTAG_IMM, "ChangeForegroundKeyboardTable: new gpKbdNlsTbl=%x\n", pklNew->spkf->pKbdNlsTbl);
    gpKbdNlsTbl = pklNew->spkf->pKbdNlsTbl;
}


//
// Toggle and push state adjusters:
//
// ResetPushState, AdjustPushState, AdjustPushStateForKL
//

void ResetPushState(PTHREADINFO pti, UINT uVk)
{
    TAGMSG1(DBGTAG_IMM, "ResetPushState: has to reset the push state of vk=%x\n", uVk);
    if (uVk != 0) {
        ClearAsyncKeyStateDown(uVk);
        ClearAsyncKeyStateDown(uVk);
        ClearRawKeyDown(uVk);
        ClearRawKeyToggle(uVk);
        ClearKeyStateDown(pti->pq, uVk);
        ClearKeyStateToggle(pti->pq, uVk);
    }
}

//
// AdjustPushState:
//
//  Left            Right           Clear?
// Down    Vanish  Down    Vanish
// (x)     (y)     (z)     (w)     (f)
//  0       0       0       0       0
//  0       0       0       1       0
//  0       0       1       0       0
//  0       0       1       1       1*
//
//  0       1       0       0       0
//  0       1       0       1       0
//  0       1       1       0       0
//  0       1       1       1       1*
//
//  1       0       0       0       0
//  1       0       0       1       0
//  1       0       1       0       0
//  1       0       1       1       0
//
//  1       1       0       0       1*
//  1       1       0       1       1*
//  1       1       1       0       0
//  1       1       1       1       1*
//
// f = ~x~yzw + ~xyzw + xy~z~w + xy~zw + xyzw
//   = zw~x(y + ~y) + xy~z(~w +w) + xyzw
//   = zw~x + xy~z + xyzw
//   = xy~z + ~xzw + xyzw.
//   = xy(~z + zw) + ~xzw
//
void AdjustPushState(PTHREADINFO ptiCurrent, BYTE bBaseVk, BYTE bVkL, BYTE bVkR, PKL pklPrev, PKL pklNew)
{
    BOOLEAN fDownL = FALSE, fDownR = FALSE;
    BOOLEAN fVanishL = FALSE, fVanishR = FALSE;

    UINT uScanCode1, uScanCode2;

    if (bVkL) {
        fDownL = TestRawKeyDown(bVkL) || TestAsyncKeyStateDown(bVkL) || TestKeyStateDown(ptiCurrent->pq, bVkL);
        uScanCode1 = InternalMapVirtualKeyEx(bVkL, 0, pklPrev->spkf->pKbdTbl);
        uScanCode2 = InternalMapVirtualKeyEx(bVkL, 0, pklNew->spkf->pKbdTbl);
        fVanishL = (uScanCode1 && uScanCode2 == 0);
        if (fVanishL) {
            ResetPushState(ptiCurrent, bVkL);
        }
    }

    if (bVkR) {
        fDownR = TestRawKeyDown(bVkR) || TestAsyncKeyStateDown(bVkR) || TestKeyStateDown(ptiCurrent->pq, bVkR);
        uScanCode1 = InternalMapVirtualKeyEx(bVkR, 0, pklPrev->spkf->pKbdTbl);
        uScanCode2 = InternalMapVirtualKeyEx(bVkR, 0, pklNew->spkf->pKbdTbl);
        fVanishR = (uScanCode1 && uScanCode2 == 0);
        if (fVanishR) {
            ResetPushState(ptiCurrent, bVkR);
        }
    }

    if (bBaseVk) {
        TAGMSG4(DBGTAG_IMM, "fDL(%d) fVL(%d) fDR(%d) fVR(%d)\n", fDownL, fVanishL, fDownR, fVanishR);
        if (((fDownL & fVanishL & ((BOOLEAN)~fDownR | (fDownR & fVanishR))) | ((BOOLEAN)~fDownL & fDownR & fVanishR)) & 1) {
            TAGMSG1(DBGTAG_IMM, "AdjustPushState(): Going to reset %x\n", bBaseVk);
            ResetPushState(ptiCurrent, bBaseVk);
        }
    }
}

VOID AdjustPushStateForKL(PTHREADINFO ptiCurrent, PBYTE pbDone, PKL pklTarget, PKL pklPrev, PKL pklNew)
{
    CONST VK_TO_BIT* pVkToBits;

    UserAssert(pklPrev);
    CheckLock(pklPrev);
    UserAssert(pklNew);
    CheckLock(pklNew);

    if (pklTarget->spkf == NULL || pklPrev->spkf == NULL) {
        return;
    }

    pVkToBits = pklTarget->spkf->pKbdTbl->pCharModifiers->pVkToBit;

    for (; pVkToBits->Vk; ++pVkToBits) {
        BYTE bVkVar1 = 0, bVkVar2 = 0;

        //
        // Is it already processed ?
        //
        UserAssert(pVkToBits->Vk < 0x100);
        if (pbDone[pVkToBits->Vk >> 3] & (1 << (pVkToBits->Vk & 7))) {
            continue;
        }

        switch (pVkToBits->Vk) {
        case VK_SHIFT:
            bVkVar1 = VK_LSHIFT;
            bVkVar2 = VK_RSHIFT;
            break;
        case VK_CONTROL:
            bVkVar1 = VK_LCONTROL;
            bVkVar2 = VK_RCONTROL;
            break;
        case VK_MENU:
            bVkVar1 = VK_LMENU;
            bVkVar2 = VK_RMENU;
            break;
        }

        TAGMSG3(DBGTAG_IMM, "Adjusting VK=%x var1=%x var2=%x\n", pVkToBits->Vk, bVkVar1, bVkVar2);

        AdjustPushState(ptiCurrent, pVkToBits->Vk, bVkVar1, bVkVar2, pklPrev, pklNew);

        pbDone[pVkToBits->Vk >> 3] |= (1 << (pVkToBits->Vk & 7));
    }
}

/*****************************************************************************\
* xxxInternalActivateKeyboardLayout
*
* pkl   - pointer to keyboard layout to switch the current thread to
* Flags - KLF_RESET
*         KLF_SETFORPROCESS
*         KLLF_SHIFTLOCK (any of KLLF_GLOBAL_ATTRS)
*         others are ignored
* pwnd  - If the current thread has no focus or active window, send the
*         WM_INPUTLANGCHANGE message to this window (unless it is NULL too)
*
* History:
* 1998-10-14 IanJa    Added pwnd parameter
\*****************************************************************************/
HKL xxxInternalActivateKeyboardLayout(
    PKL pkl,
    UINT Flags,
    PWND pwnd)
{
    HKL hklPrev;
    PKL pklPrev;
    TL  tlpklPrev;
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckLock(pkl);
    CheckLock(pwnd);

    /*
     * Remember what is about to become the "previously" active hkl
     * for the return value.
     */
    if (ptiCurrent->spklActive != (PKL)NULL) {
        pklPrev = ptiCurrent->spklActive;
        hklPrev = ptiCurrent->spklActive->hkl;
    } else {
        pklPrev = NULL;
        hklPrev = (HKL)0;
    }

    /*
     * ShiftLock/CapsLock is a global feature applying to all layouts
     * Only Winlogon and the Input Locales cpanel applet set KLF_RESET.
     */
    if (Flags & KLF_RESET) {
        gdwKeyboardAttributes = KLL_GLOBAL_ATTR_FROM_KLF(Flags);
    }

    /*
     * Early out
     */
    if (!(Flags & KLF_SETFORPROCESS) && (pkl == ptiCurrent->spklActive)) {
        return hklPrev;
    }

    /*
     * Clear out diacritics when switching kbd layouts #102838
     */
    pkl->wchDiacritic = 0;

    /*
     * Update the active layout in the pti.  KLF_SETFORPROCESS will always be set
     * when the keyboard layout switch is initiated by the keyboard hotkey.
     */

    /*
     * Lock the previous keyboard layout for it's used later.
     */
    ThreadLockWithPti(ptiCurrent, pklPrev, &tlpklPrev);

    /*
     * Is this is a console thread, apply this change to any process in it's
     * window.  This can really help character-mode apps!  (#58025)
     */
    if (ptiCurrent->TIF_flags & TIF_CSRSSTHREAD) {
        Lock(&ptiCurrent->spklActive, pkl);
        ptiCurrent->pClientInfo->CodePage = pkl->CodePage;
#if 0   // see Raid #58025 and #78586
        ptiCurrent->pClientInfo->hKL = pkl->hkl;
        PLIST_ENTRY pHead;
        PLIST_ENTRY pEntry;
        PQ pqCurrent = ptiCurrent->pq;
        PTHREADINFO ptiT;

        pHead = &(grpdeskRitInput->PtiList);
        for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
            ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
            if (ptiT->pq == pqCurrent) {
                Lock(&ptiT->spklActive, pkl);
                UserAssert(ptiT->pClientInfo != NULL);
                ptiT->pClientInfo->CodePage = pkl->CodePage;
                ptiT->pClientInfo->hKL = pkl->hkl;
            }
        }
#endif
    } else if ((Flags & KLF_SETFORPROCESS) && !(ptiCurrent->TIF_flags & TIF_16BIT)) {
        /*
         * For 16 bit app., only the calling thread will have its active layout updated.
         */
       PTHREADINFO ptiT;

       if (IS_IME_ENABLED()) {
           /*
            * Only allow *NOT* CSRSS to make this call
            */
           UserAssert(PsGetCurrentProcess() != gpepCSRSS);
           // pti->pClientInfo is updated in xxxImmActivateThreadsLayout()
           if (!xxxImmActivateThreadsLayout(ptiCurrent->ppi->ptiList, NULL, pkl)) {
               RIPMSG1(RIP_WARNING, "no layout change necessary via xxxImmActivateThreadLayout() for process %lx", ptiCurrent->ppi);
               goto UnlockAndGo;
           }
       } else {
           BOOL fKLChanged = FALSE;

           for (ptiT = ptiCurrent->ppi->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
               if (ptiT->spklActive != pkl && (ptiT->TIF_flags & TIF_INCLEANUP) == 0) {
                   Lock(&ptiT->spklActive, pkl);
                   UserAssert(ptiT->pClientInfo != NULL);
                   ptiT->pClientInfo->CodePage = pkl->CodePage;
                   ptiT->pClientInfo->hKL = pkl->hkl;
                   fKLChanged = TRUE;
               }
           }
           if (!fKLChanged) {
              RIPMSG1(RIP_WARNING, "no layout change necessary for process %lx ?", ptiCurrent->ppi);
              goto UnlockAndGo;
           }
       }

    } else {
        if (IS_IME_ENABLED()) {
            xxxImmActivateLayout(ptiCurrent, pkl);
        } else {
            Lock(&ptiCurrent->spklActive, pkl);
        }
        UserAssert(ptiCurrent->pClientInfo != NULL);
        if ((ptiCurrent->TIF_flags & TIF_INCLEANUP) == 0) {
            ptiCurrent->pClientInfo->CodePage = pkl->CodePage;
            ptiCurrent->pClientInfo->hKL = pkl->hkl;
        }
    }

    /*
     * Some keys (pressed to switch layout) may still be down.  When these come
     * back up, they may have different VK values due to the new layout, so the
     * original key will be left stuck down. (eg: an ISV layout from Attachmate
     * and the CAN/CSA layout, both of which redefine the right-hand Ctrl key's
     * VK so switching to that layout with right Ctrl+Shift will leave the Ctrl
     * stuck down).
     * The solution is to clear all the keydown bits whenever we switch layouts
     * (leaving the toggle bits alone to preserve CapsLock, NumLock etc.). This
     * also solves the AltGr problem, where the simulated Ctrl key doesn't come
     * back up if we switch to a non-AltGr layout before releasing AltGr - IanJa
     *
     * Clear down bits only if necessary --- i.e. if the VK value differs between
     * old and new keyboard layout. We have to take complex path for some of the
     * keys, like Ctrl or Alt, may have left and right equivalents. - HiroYama
     */
    if (ptiCurrent->pq) {
        if (pklPrev) {
            BYTE baDone[256 / 8];

            RtlZeroMemory(baDone, sizeof baDone);

            /*
             * Clear the toggle state if needed. First check the modifier keys
             * of pklPrev. Next check the modifier keys of pklNew.
             */
            TAGMSG2(DBGTAG_IMM, "Changing KL from %08lx to %08lx", pklPrev->hkl, pkl->hkl);
            AdjustPushStateForKL(ptiCurrent, baDone, pklPrev, pklPrev, pkl);
            AdjustPushStateForKL(ptiCurrent, baDone, pkl, pklPrev, pkl);

            if (pklPrev->spkf && (pklPrev->spkf->pKbdTbl->fLocaleFlags & KLLF_ALTGR)) {
                /*
                 * If the previous keyboard has AltGr, clear the left control anyway.
                 * See xxxAltGr().
                 */
                TAGMSG0(DBGTAG_IMM, "Clearing VK_LCONTROL for AltGr\n");
                ResetPushState(ptiCurrent, VK_LCONTROL);
            }
        }
        else {
            /*
             * If the current keyboard is unkown, clear all the push state.
             */
            int i;
            for (i = 0; i < CBKEYSTATE; i++) {
                ptiCurrent->pq->afKeyState[i] &= KEYSTATE_TOGGLE_BYTEMASK;
                gafAsyncKeyState[i] &= KEYSTATE_TOGGLE_BYTEMASK;
                gafRawKeyState[i] &= KEYSTATE_TOGGLE_BYTEMASK;
            }
        }
    }

    /*
     * Call the Shell hook with the new language.
     */
    if (gptiForeground && (gptiForeground->ppi == ptiCurrent->ppi)) {
        ChangeForegroundKeyboardTable(pklPrev, pkl);

        /*
         * Only call the hook if we are the foreground process, to prevent
         * background apps from changing the indicator.  (All console apps
         * are part of the same process, but I have never seen a cmd window
         * app change the layout, let alone in the background)
         */
        if (gLCIDSentToShell != pkl->hkl && (ptiCurrent != gptiRit)) {
           if (IsHooked(ptiCurrent, WHF_SHELL)) {
               gLCIDSentToShell = pkl->hkl;
               xxxCallHook(HSHELL_LANGUAGE, (WPARAM)NULL, (LPARAM)pkl->hkl, WH_SHELL);
           }
        }
    }

    /*
     * Tell the app what happened
     */
    if (ptiCurrent->pq) {
        PWND pwndT;
        TL tlpwndT;

        /*
         * If we have no Focus window, use the Active window.
         * eg: Console full-screen has NULL focus window.
         */
        pwndT = ptiCurrent->pq->spwndFocus;
        if (pwndT == NULL) {
            pwndT = ptiCurrent->pq->spwndActive;
            if (pwndT == NULL) {
                pwndT = pwnd;
            }
        }

        if (pwndT != NULL) {
            ThreadLockAlwaysWithPti( ptiCurrent, pwndT, &tlpwndT);
            xxxSendMessage(pwndT, WM_INPUTLANGCHANGE, (WPARAM)pkl->iBaseCharset, (LPARAM)pkl->hkl);
            ThreadUnlock(&tlpwndT);
        }
    }

    /*
     * Tell IME to send mode update notification
     */
    if (ptiCurrent && ptiCurrent->spwndDefaultIme &&
            (ptiCurrent->TIF_flags & TIF_CSRSSTHREAD) == 0) {
        if (IS_IME_KBDLAYOUT(pkl->hkl)) {
            BOOL fForProcess = (ptiCurrent->TIF_flags & KLF_SETFORPROCESS) && !(ptiCurrent->TIF_flags & TIF_16BIT);
            TL tlpwndIme;

            TAGMSG1(DBGTAG_IMM, "Sending IMS_SENDNOTIFICATION to pwnd=%p", ptiCurrent->spwndDefaultIme);

            ThreadLockAlwaysWithPti(ptiCurrent, ptiCurrent->spwndDefaultIme, &tlpwndIme);
            xxxSendMessage(ptiCurrent->spwndDefaultIme, WM_IME_SYSTEM, IMS_SENDNOTIFICATION, fForProcess);
            ThreadUnlock(&tlpwndIme);
        }
    }

UnlockAndGo:
    ThreadUnlock(&tlpklPrev);

    return hklPrev;
}

BOOL xxxUnloadKeyboardLayout(
    PWINDOWSTATION pwinsta,
    HKL hkl)
{
    PKL pkl;

    /*
     * Validate HKL and check to make sure an app isn't attempting to unload a system
     * preloaded layout.
     */
    pkl = HKLtoPKL(PtiCurrent(), hkl);
    if (pkl == NULL) {
        return FALSE;
    }

    return xxxInternalUnloadKeyboardLayout(pwinsta, pkl, 0);
}

HKL _GetKeyboardLayout(
    DWORD idThread)
{
    PTHREADINFO ptiT;
    PLIST_ENTRY pHead, pEntry;

    CheckCritIn();

    /*
     * If idThread is NULL return hkl of the current thread
     */
    if (idThread == 0) {
        PKL pklActive = PtiCurrentShared()->spklActive;

        if (pklActive == NULL) {
            return (HKL)0;
        }
        return pklActive->hkl;
    }
    /*
     * Look for idThread
     */
    pHead = &PtiCurrent()->rpdesk->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
        ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
        if (ptiT->pEThread->Cid.UniqueThread == (HANDLE)LongToHandle( idThread )) {
            if (ptiT->spklActive == NULL) {
                return (HKL)0;
            }
            return ptiT->spklActive->hkl;
        }
    }
    /*
     * idThread doesn't exist
     */
    return (HKL)0;
}

UINT _GetKeyboardLayoutList(
    PWINDOWSTATION pwinsta,
    UINT nItems,
    HKL *ccxlpBuff)
{
    UINT nHKL = 0;
    PKL pkl, pklFirst;

    if (!pwinsta) {
        return 0;
    }

    pkl = pwinsta->spklList;

    /*
     * Windowstations that do not take input could have no layouts
     */
    if (pkl == NULL) {
        // SetLastError() ????
        return 0;
    }

    /*
     * The client/server thunk sets nItems to 0 if ccxlpBuff == NULL
     */
    UserAssert(ccxlpBuff || (nItems == 0));

    pklFirst = pkl;
    if (nItems) {
        try {
            do {
               if (!(pkl->dwKL_Flags & KL_UNLOADED)) {
                   if (nItems-- == 0) {
                       break;
                   }
                   nHKL++;
                   *ccxlpBuff++ = pkl->hkl;
               }
               pkl = pkl->pklNext;
            } while (pkl != pklFirst);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_ERROR,
                    "_GetKeyBoardLayoutList: exception writing ccxlpBuff %lx", ccxlpBuff);
            return 0;
        }
    } else do {
        if (!(pkl->dwKL_Flags & KL_UNLOADED)) {
            nHKL++;
        }
        pkl = pkl->pklNext;
    } while (pkl != pklFirst);

    return nHKL;
}

/*
 * Layouts are locked by each thread using them and possibly by:
 *    - pwinsta->spklList (head of windowstation's list)
 *    - gspklBaseLayout   (default layout for new threads)
 * The layout is marked for destruction when gets unloaded, so that it will be
 * unlinked and freed as soon as an Unlock causes the lock count to go to 0.
 * If it is reloaded before that time, it is unmarked for destruction. This
 * ensures that laoded layouts stay around even when they go out of use.
 */
BOOL xxxInternalUnloadKeyboardLayout(
    PWINDOWSTATION pwinsta,
    PKL pkl,
    UINT Flags)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    TL tlpkl;

    UserAssert(pkl);

    /*
     * Never unload the default layout, unless we are destroying the current
     * windowstation or replacing one user's layouts with another's.
     */
    if ((pkl == gspklBaseLayout) && !(Flags & KLF_INITTIME)) {
        return FALSE;
    }

    /*
     * Keeps pkl good, but also allows destruction when unlocked later
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pkl, &tlpkl);

    /*
     * Mark it for destruction so it gets removed when the lock count reaches 0
     * Mark it KL_UNLOADED so that it appears to be gone from the toggle list
     */
    HMMarkObjectDestroy(pkl);
    pkl->dwKL_Flags |= KL_UNLOADED;

    /*
     * If unloading this thread's active layout, helpfully activate the next one
     * (Don't bother if KLF_INITTIME - unloading all previous user's layouts)
     */
    if (!(Flags & KLF_INITTIME)) {
        UserAssert(ptiCurrent->spklActive != NULL);
        if (ptiCurrent->spklActive == pkl) {
            PKL pklNext;
            pklNext = HKLtoPKL(ptiCurrent, (HKL)HKL_NEXT);
            if (pklNext != NULL) {
                TL tlPKL;
                ThreadLockAlwaysWithPti(ptiCurrent, pklNext, &tlPKL);
                xxxInternalActivateKeyboardLayout(pklNext, Flags, NULL);
                ThreadUnlock(&tlPKL);
            }
        }
    }

    /*
     * If this pkl == pwinsta->spklList, give it a chance to be destroyed by
     * unlocking it from pwinsta->spklList.
     */
    if (pwinsta->spklList == pkl) {
        UserAssert(pkl != NULL);
        if (pkl != pkl->pklNext) {
            pkl = Lock(&pwinsta->spklList, pkl->pklNext);
            UserAssert(pkl != NULL); // gspklBaseLayout and ThreadLocked pkl
        }
    }

    /*
     * This finally destroys the unloaded layout if it is not in use anywhere
     */
    ThreadUnlock(&tlpkl);

    /*
     * Update keyboard list.
     */
    if (IsHooked(ptiCurrent, WHF_SHELL)) {
        xxxCallHook(HSHELL_LANGUAGE, (WPARAM)NULL, (LPARAM)0, WH_SHELL);
        gLCIDSentToShell = 0;
    }

    return TRUE;
}

VOID xxxFreeKeyboardLayouts(
    PWINDOWSTATION pwinsta, BOOL bUnlock)
{
    PKL pkl;

    /*
     * Unload all of the windowstation's layouts.
     * They may still be locked by some threads (eg: console), so this
     * may not destroy them all, but it will mark them all KL_UNLOADED.
     * Set KLF_INITTIME to ensure that the default layout (gspklBaseLayout)
     * gets unloaded too.
     * Note: it's much faster to unload non-active layouts, so start with
     * the next loaded layout, leaving the active layout till last.
     */
    while ((pkl = HKLtoPKL(PtiCurrent(), (HKL)HKL_NEXT)) != NULL) {
        xxxInternalUnloadKeyboardLayout(pwinsta, pkl, KLF_INITTIME);
    }

    /*
     * The WindowStation is being destroyed, or one user's layouts are being
     * replaced by another user's, so it's OK to Unlock spklList.
     * Any layout still in the double-linked circular KL list will still be
     * pointed to by gspklBaseLayout: this is important, since we don't want
     * to leak any KL or KBDFILE objects by losing pointers to them.
     * There are no layouts when we first come here (during bootup).
     */
    if (bUnlock) {
        Unlock(&pwinsta->spklList);
    }
}

/***************************************************************************\
* DestroyKL
*
* Destroys a keyboard layout. Note that this function does not
* follow normal destroy function semantics. See IanJa.
*
* History:
* 25-Feb-1997 adams     Created.
\***************************************************************************/

VOID DestroyKL(
    PKL pkl)
{
    PKBDFILE pkf;

    /*
     * Cut it out of the pwinsta->spklList circular bidirectional list.
     * We know pwinsta->spklList != pkl, since pkl is unlocked.
     */
    pkl->pklPrev->pklNext = pkl->pklNext;
    pkl->pklNext->pklPrev = pkl->pklPrev;

    /*
     * Unlock its pkf
     */
    pkf = Unlock(&pkl->spkf);
    if (pkf) {
        DestroyKF(pkf);
    }

    if (pkl->piiex != NULL) {
        UserFreePool(pkl->piiex);
    }

    /*
     * Free the pkl itself.
     */
    HMFreeObject(pkl);
}
