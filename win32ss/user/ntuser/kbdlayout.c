/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/user/ntuser/kbdlayout.c
 * PURPOSE:         Keyboard layout management
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 *                  Copyright 2008 Colin Finck
 *                  Copyright 2011 Rafal Harabien
 *                  Copyright 2022-2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

static BOOL IntIsValidLayoutFileName(PCWSTR pszPath)
{
    if (!*pszPath)
        return FALSE;
    SIZE_T cch = wcscspn(pszPath, L"\\/:");
#define MAX_VALID_LAYOUT_FILENAME 32
    return (cch < MAX_VALID_LAYOUT_FILENAME && !pszPath[cch]);
}

/*------------------------------------------------------------------------------
 * Raw keyboard layout image loader.
 *
 * Loads a keyboard layout stub DLL (e.g. kbdjpn.dll / kbdkor.dll) that has user-mode
 * imports (ntdll) and therefore cannot be loaded by EngLoadImage. The file is read
 * into pool memory (pkf->pRawImage), sections are copied, base relocations are applied,
 * and imports are NOT resolved. Code in the image is NEVER executed; the tables are
 * obtained only through DecodeConstStub.
 */

#define USERTAG_KBDRAW 'KbdR'
#define IFN_KbdLayerDescriptor    1
#define IFN_KbdNlsLayerDescriptor 2
#define KBDRAW_MAX_FILE_SIZE (1024 * 1024)

#ifndef IMAGE_REL_BASED_ABSOLUTE
    #define IMAGE_REL_BASED_ABSOLUTE 0
    #define IMAGE_REL_BASED_HIGHLOW  3
    #define IMAGE_REL_BASED_DIR64    10
#endif

#if defined(_M_IX86)
    #define KBDRAW_MACHINE IMAGE_FILE_MACHINE_I386
    #define KBDRAW_RELTYPE IMAGE_REL_BASED_HIGHLOW
#elif defined(_M_AMD64)
    #define KBDRAW_MACHINE IMAGE_FILE_MACHINE_AMD64
    #define KBDRAW_RELTYPE IMAGE_REL_BASED_DIR64
#else
    #error Unsupported architecture
#endif

/* Decodes the constant return value from a procedure (the code is never executed) */
static PVOID
DecodeConstStub(
    _In_ PUCHAR pbBase,
    _In_ ULONG cbImage,
    _In_opt_ PUCHAR pb)
{
    PUCHAR pbEnd = pbBase + cbImage;
    PVOID pvRet = NULL;

    if (pb < pbBase || pb + 8 > pbEnd)
        return NULL;

#if defined(_M_IX86)
    /* mov eax, imm32; ret */
    if (pb[0] == 0xB8 && pb[5] == 0xC3)
        pvRet = (PVOID)(ULONG_PTR)*(UNALIGNED ULONG *)(pb + 1);
#elif defined(_M_AMD64)
    /* lea rax,[rip+rel]; ret */
    if (pb[0] == 0x48 && pb[1] == 0x8D && pb[2] == 0x05 && pb[7] == 0xC3)
        pvRet = pb + 7 + *(UNALIGNED LONG *)(pb + 3);
#else
    #error Unsupported architecture
#endif

    return ((PUCHAR)pvRet >= pbBase && (PUCHAR)pvRet + 4 <= pbEnd) ? pvRet : NULL;
}

/* Applies base relocations so that the copy works at its pool address */
static BOOL
KbdRawRelocate(
    _Inout_ PUCHAR pbImage,
    _In_ ULONG cbImage,
    _In_ PIMAGE_NT_HEADERS pNt)
{
    PIMAGE_OPTIONAL_HEADER pOpt = &pNt->OptionalHeader;
    PIMAGE_DATA_DIRECTORY pRelocDir = &pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    ULONG_PTR Delta = (ULONG_PTR)pbImage - (ULONG_PTR)pOpt->ImageBase;
    ULONG cbBlock, nEntries, ibOffset, cbDir = pRelocDir->Size;

    if (!cbDir)
        return !Delta; /* No relocations */

    if (pRelocDir->VirtualAddress > cbImage || cbDir > cbImage - pRelocDir->VirtualAddress)
        return FALSE;

    for (ibOffset = 0; ibOffset + sizeof(IMAGE_BASE_RELOCATION) <= cbDir; ibOffset += cbBlock)
    {
        PIMAGE_BASE_RELOCATION pBlock = (PVOID)(pbImage + pRelocDir->VirtualAddress + ibOffset);
        PUSHORT pwEntry = (PUSHORT)(pBlock + 1);
        cbBlock = pBlock->SizeOfBlock;

        if (cbBlock < sizeof(*pBlock) || cbBlock > cbDir - ibOffset ||
            pBlock->VirtualAddress >= cbImage)
        {
            return FALSE;
        }

        for (nEntries = (cbBlock - sizeof(*pBlock)) / sizeof(USHORT); nEntries;
             --nEntries, ++pwEntry)
        {
            ULONG offsets = pBlock->VirtualAddress + (*pwEntry & 0x0FFF);
            switch (*pwEntry >> 12)
            {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;
                case KBDRAW_RELTYPE:
                    if (offsets > cbImage - sizeof(ULONG_PTR))
                        return FALSE;
                    *(UNALIGNED ULONG_PTR *)(pbImage + offsets) += Delta;
                    break;
                default:
                    return FALSE;
            }
        }
    }
    return TRUE;
}

/* Looks up an export by ordinal. Forwarders are rejected. */
static PVOID
KbdRawGetProc(
    _In_ PUCHAR pbImage,
    _In_ PIMAGE_NT_HEADERS pNt,
    _In_ ULONG ordinal)
{
    PIMAGE_DATA_DIRECTORY pDir = &pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    ULONG cbImage = pNt->OptionalHeader.SizeOfImage, rva = pDir->VirtualAddress, cb = pDir->Size;
    PIMAGE_EXPORT_DIRECTORY pExp;
    ULONG funcRva;

    if (rva > cbImage || cb < sizeof(*pExp) || cb > cbImage - rva)
        return NULL;

    pExp = (PIMAGE_EXPORT_DIRECTORY)(pbImage + rva);
    ordinal -= pExp->Base; /* Wraps around (and is rejected below) if ordinal < Base */
    if (ordinal >= pExp->NumberOfFunctions || pExp->AddressOfFunctions > cbImage ||
        pExp->NumberOfFunctions > (cbImage - pExp->AddressOfFunctions) / sizeof(ULONG))
    {
        return NULL;
    }

    funcRva = ((PULONG)(pbImage + pExp->AddressOfFunctions))[ordinal];
    if (!funcRva || funcRva >= cbImage || (funcRva >= rva && funcRva - rva < cb)) /* Forwarder */
        return NULL;

    return pbImage + funcRva;
}

/* Reads the whole file and builds an import-less, not-yet-relocated copy in pool memory */
static PVOID
KbdRawLoadImage(
    _In_ PCWSTR pwszPath)
{
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    FILE_STANDARD_INFORMATION fsi;
    HANDLE hFile;
    PUCHAR pbFile = NULL, pbImage = NULL, pbRet = NULL;
    PIMAGE_DOS_HEADER pDos;
    PIMAGE_NT_HEADERS pNt;
    PIMAGE_SECTION_HEADER pSec;
    ULONG cbFile, cbImage, cbHeaders, secOff, iSection;
    NTSTATUS Status;

    RtlInitUnicodeString(&Name, pwszPath);
    InitializeObjectAttributes(&oa, &Name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwOpenFile(&hFile, GENERIC_READ | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
        return NULL;

    Status = ZwQueryInformationFile(hFile, &iosb, &fsi, sizeof(fsi), FileStandardInformation);
    if (!NT_SUCCESS(Status) ||
        fsi.EndOfFile.QuadPart < (LONGLONG)(sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) ||
        fsi.EndOfFile.QuadPart > KBDRAW_MAX_FILE_SIZE)
    {
        goto Quit;
    }
    cbFile = fsi.EndOfFile.LowPart;

    pbFile = ExAllocatePoolWithTag(PagedPool, cbFile, USERTAG_KBDRAW);
    if (!pbFile ||
        !NT_SUCCESS(ZwReadFile(hFile, NULL, NULL, NULL, &iosb, pbFile, cbFile, NULL, NULL)) ||
        iosb.Information != cbFile)
    {
        goto Quit;
    }

    /* Validate headers (all offsets are checked against the file size) */
    pDos = (PIMAGE_DOS_HEADER)pbFile;
    if (pDos->e_magic != IMAGE_DOS_SIGNATURE || pDos->e_lfanew <= 0 ||
        (ULONG)pDos->e_lfanew > cbFile - sizeof(IMAGE_NT_HEADERS))
    {
        goto Quit;
    }

    pNt = (PIMAGE_NT_HEADERS)(pbFile + pDos->e_lfanew);
    cbImage = pNt->OptionalHeader.SizeOfImage;
    cbHeaders = pNt->OptionalHeader.SizeOfHeaders;
    secOff = (ULONG)((PUCHAR)IMAGE_FIRST_SECTION(pNt) - pbFile);
    if (pNt->Signature != IMAGE_NT_SIGNATURE ||
        pNt->FileHeader.Machine != KBDRAW_MACHINE ||
        pNt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
        cbImage == 0 || cbImage > KBDRAW_MAX_FILE_SIZE ||
        cbHeaders > cbFile || cbHeaders > cbImage ||
        (ULONG)pDos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > cbHeaders ||
        secOff > cbFile ||
        pNt->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER) > cbFile - secOff)
    {
        goto Quit;
    }

    pbImage = ExAllocatePoolWithTag(PagedPool, cbImage, USERTAG_KBDRAW);
    if (!pbImage)
        goto Quit;

    RtlZeroMemory(pbImage, cbImage);
    RtlCopyMemory(pbImage, pbFile, cbHeaders);

    /* Copy sections */
    pSec = (PIMAGE_SECTION_HEADER)(pbFile + secOff);
    for (iSection = 0; iSection < pNt->FileHeader.NumberOfSections; ++iSection, ++pSec)
    {
        ULONG cbCopy = pSec->SizeOfRawData;
        if (pSec->Misc.VirtualSize && pSec->Misc.VirtualSize < cbCopy)
            cbCopy = pSec->Misc.VirtualSize;
        if (!cbCopy)
            continue;

        if (pSec->VirtualAddress > cbImage || cbCopy > cbImage - pSec->VirtualAddress ||
            pSec->PointerToRawData > cbFile || cbCopy > cbFile - pSec->PointerToRawData)
        {
            goto Quit;
        }
        RtlCopyMemory(pbImage + pSec->VirtualAddress, pbFile + pSec->PointerToRawData, cbCopy);
    }

    pbRet = pbImage;
    pbImage = NULL; /* Ownership moves to the caller */

Quit:
    if (pbImage)
        ExFreePoolWithTag(pbImage, USERTAG_KBDRAW);
    if (pbFile)
        ExFreePoolWithTag(pbFile, USERTAG_KBDRAW);
    ZwClose(hFile);
    return pbRet;
}

/* Loads keyboard layout DLL as a raw image (*ppRawImage) and gets the tables from it */
static BOOL
UserLoadKbdDll(
    _In_ PCWSTR pwszLayoutPath,
    _Out_ PVOID *ppRawImage,
    _Out_ PKBDTABLES *ppKbdTables,
    _Out_ PKBDNLSTABLES *ppKbdNlsTables)
{
    PUCHAR pbImage;
    PIMAGE_NT_HEADERS pNt;
    PVOID pfnMain, pfnNls;
    BOOL bOK = FALSE;

    /* Stub DLLs with user-mode imports (kbdjpn/kbdkor) cannot be loaded by EngLoadImage */
    pbImage = *ppRawImage = KbdRawLoadImage(pwszLayoutPath);
    if (!pbImage)
        goto Failed;

    _SEH2_TRY
    {
        pNt = RtlImageNtHeader(pbImage);

        /* Relocations are mandatory: the tables contain absolute pointers */
        if (KbdRawRelocate(pbImage, pNt->OptionalHeader.SizeOfImage, pNt))
        {
            pfnMain = KbdRawGetProc(pbImage, pNt, IFN_KbdLayerDescriptor);
            if (pfnMain)
            {
                const ULONG dwSizeOfImage = pNt->OptionalHeader.SizeOfImage;
                /* Decode the main table */
                *ppKbdTables = DecodeConstStub(pbImage, dwSizeOfImage, pfnMain);
                /* Decode the NLS table (optional) */
                pfnNls = KbdRawGetProc(pbImage, pNt, IFN_KbdNlsLayerDescriptor);
                if (pfnNls)
                    *ppKbdNlsTables = DecodeConstStub(pbImage, dwSizeOfImage, pfnNls);

                bOK = (*ppKbdTables && (!pfnNls || *ppKbdNlsTables));
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception in UserLoadKbdDll!\n");
    }
    _SEH2_END;

    if (!bOK)
        goto Failed;

    TRACE("Loaded %ws\n", pwszLayoutPath);

#if 0 && DBG
    DumpKbdLayout(*ppKbdTables);
#endif
    return TRUE;

Failed:
    ERR("Failed to load dll %ws\n", pwszLayoutPath);
    if (*ppRawImage)
    {
        ExFreePoolWithTag(*ppRawImage, USERTAG_KBDRAW);
        *ppRawImage = NULL;
    }
    return FALSE;
}

/* Loads keyboard layout DLL and creates KBDFILE object */
static PKBDFILE
UserLoadKbdFile(
    _In_ HKL hKL,
    _In_ PCUNICODE_STRING pwszKLID,
    _In_opt_z_ PCWSTR pszLayoutFile,
    _In_opt_ PKBDFILE *ppkfReal)
{
    PKBDFILE pkf, pRet = NULL;
    NTSTATUS Status;
    WCHAR wszLayoutFile[80], wszLayoutPath[MAX_PATH];
    WCHAR wszLayoutRegKey[MAX_PATH], wszRealDllName[80];

    if (ppkfReal)
        *ppkfReal = NULL;

    /* Create keyboard layout file object */
    pkf = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDFILE, sizeof(*pkf));
    if (!pkf)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    _swprintf(pkf->awchKF, L"%wZ", pwszKLID); /* Set keyboard layout name */

    if (!pszLayoutFile)
    {
        ULONG cbSize;
        HKEY hKey;

        /* Open layout registry key */
        RtlStringCbPrintfW(wszLayoutRegKey, sizeof(wszLayoutRegKey),
            L"\\REGISTRY\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s",
            pkf->awchKF);
        Status = RegOpenKey(wszLayoutRegKey, &hKey);
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to open keyboard layouts registry %ws (%lx)\n", wszLayoutRegKey, Status);
            goto cleanup;
        }

        /* Read filename of layout DLL */
        cbSize = sizeof(wszLayoutFile);
        Status = RegQueryValue(hKey, L"Layout File", REG_SZ, wszLayoutFile, &cbSize);
        ZwClose(hKey);
        if (!NT_SUCCESS(Status))
        {
            ERR("Can't get layout filename for %wZ (%lx)\n", pwszKLID, Status);
            goto cleanup;
        }

        pszLayoutFile = wszLayoutFile;
    }

    /* Validate the filename */
    if (!IntIsValidLayoutFileName(pszLayoutFile))
    {
        ERR("Invalid layout filename: %S\n", pszLayoutFile);
        goto cleanup;
    }

    /* Build the full path */
    Status = RtlStringCbPrintfW(wszLayoutPath, sizeof(wszLayoutPath),
                                L"\\SystemRoot\\System32\\%s", pszLayoutFile);
    if (!NT_SUCCESS(Status))
    {
        ERR("Can't build pathname for %wZ (%lx)\n", pwszKLID, Status);
        goto cleanup;
    }

    /* Load keyboard file now */
    if (!UserLoadKbdDll(wszLayoutPath, &pkf->hBase, &pkf->pKbdTbl, &pkf->pKbdNlsTbl))
    {
        ERR("Failed to load %ws dll!\n", wszLayoutPath);
        goto cleanup;
    }

    if (ppkfReal) /* Root layout? */
    {
        /* Load the keyboard multi-table in user-mode */
        KBDTABLE_MULTI kbdTableMulti;
        RtlZeroMemory(&kbdTableMulti, sizeof(kbdTableMulti));
        if (co_GetKeyboardMultiTable(pszLayoutFile, hKL, &kbdTableMulti,
                                     wszRealDllName, _countof(wszRealDllName)))
        {
            /* Choose the best table and load it as *ppkfReal */
            ULONG iTable, nTables = kbdTableMulti.nTables;
            PKBDTABLE_DESC pKbdTables = kbdTableMulti.aKbdTables;
            for (iTable = 0; iTable < nTables; ++iTable)
            {
                if (_wcsicmp(wszRealDllName, pKbdTables[iTable].wszDllName) == 0)
                {
                    /* Recurse with new filename */
                    *ppkfReal = UserLoadKbdFile(hKL, pwszKLID, wszRealDllName, NULL);
                    break;
                }
            }
            if (!*ppkfReal)
                *ppkfReal = UserLoadKbdFile(hKL, pwszKLID, pKbdTables[0].wszDllName, NULL);
        }
    }

    /* Append pkf to gpkfList */
    pkf->pkfNext = gpkfList;
    gpkfList = pkf;

    pRet = pkf; /* Return keyboard file */

cleanup:
    if (pkf)
        UserDereferenceObject(pkf); // we dont need ptr anymore
    if (!pRet && pkf) /* We have failed? - destroy created object */
        UserDeleteObject(UserHMGetHandle(pkf), TYPE_KBDFILE);
    return pRet;
}

/* Loads keyboard layout and creates KL object */
static PKL
co_UserLoadKbdLayout(
    _In_ PCUNICODE_STRING pustrKLID,
    _In_ HKL hKL)
{
    LCID lCid;
    CHARSETINFO cs;
    PKL pKl;
    PKBDFILE spkf, spkfReal;

    /* Create keyboard layout object */
    pKl = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDLAYOUT, sizeof(KL));
    if (!pKl)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    pKl->hkl = hKL;
    spkf = UserLoadKbdFile(hKL, pustrKLID, NULL, &spkfReal);
    if (spkfReal)
    {
        pKl->spkf = spkfReal;
        pKl->spkfSub = spkf;
    }
    else
    {
        pKl->spkf = spkf;
        pKl->spkfSub = NULL;
    }

    /* Dereference keyboard layout */
    UserDereferenceObject(pKl);

    /* If we failed, remove KL object */
    if (!pKl->spkf)
    {
        ERR("UserLoadKbdFile(%wZ) failed!\n", pustrKLID);
        if (pKl->spkfSub)
        {
            UnloadKbdFile(pKl->spkfSub);
            pKl->spkfSub = NULL;
        }
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
VOID FASTCALL
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

    ExFreePoolWithTag(pkf->hBase, USERTAG_KBDRAW); /* raw image */
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
    if (pKl->spkfSub)
        UnloadKbdFile(pKl->spkfSub);
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
        if (pti->KeyboardLayout)
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
 *       We don't use some parameters of NtUserLoadKeyboardLayoutEx.
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

    UNREFERENCED_PARAMETER(hFile);
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

    pWinSta = IntGetProcessWindowStation(NULL);
    hRetKL = co_IntLoadKeyboardLayoutEx(pWinSta,
                                        NULL,
                                        hOldKL,
                                        &uszSafeKLID,
                                        UlongToHandle(dwNewKL),
                                        Flags);

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
