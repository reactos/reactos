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

#define KBDIMG_TAG 'ibKU'
#define KBDIMG_MAXSIZE (1 * 1024 * 1024)

#ifndef RVA
    #define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))
#endif

/* Describes the .data section taken by KbdBuildImage. The buffer holds the .data section
 * only, so the byte at offset 0 of the buffer is the byte at RVA offData of the file. */
typedef struct _KBDIMAGE_INFO
{
    ULONG_PTR ImageBase; /* The base address the pointers in the layout tables are based on */
    ULONG offData;       /* RVA of the .data section in the file (not an offset in the buffer) */
    ULONG cbData;        /* Size of the .data section (= size of the buffer) */
} KBDIMAGE_INFO, *PKBDIMAGE_INFO;

/**
 * @brief
 * Build the image of the keyboard layout file without executing it,
 * due to security reasons.
 *
 * For a valid keyboard layout file, the tables (and any data they point to)
 * are all stored in the .data section, so this is the only section we take.
 * The headers and any other sections are neither considered nor retained
 * (the returned buffer is exactly the size of the .data section), and neither
 * is the base-relocation data: the pointers inside the tables are adjusted
 * later by KbdRebaseTables(), using the RVA macro.
 *
 * @note
 * Offset 0 of the returned buffer corresponds to RVA pInfo->offData.
 **/
static PVOID
KbdBuildImage(
    _In_ PVOID pvRaw,
    _In_ SIZE_T cbRaw,
    _Out_ PULONG pcbImage,
    _Out_ PKBDIMAGE_INFO pInfo)
{
    static const UCHAR s_DataName[IMAGE_SIZEOF_SHORT_NAME] = ".data";
    PIMAGE_DOS_HEADER pDos = pvRaw;
    PIMAGE_NT_HEADERS pNt;
    PIMAGE_SECTION_HEADER pSec, pData = NULL;
    PBYTE pbRaw = pvRaw;
    _SEH2_VOLATILE PBYTE pbImage = NULL;
    ULONG cbImage, cbVirt, cbCopy, i;
    SIZE_T offSec;
    ULONG_PTR imageBase;

    *pcbImage = 0;
    RtlZeroMemory(pInfo, sizeof(*pInfo));

    _SEH2_TRY
    {
        if (cbRaw < sizeof(IMAGE_DOS_HEADER) || pDos->e_magic != IMAGE_DOS_SIGNATURE ||
            pDos->e_lfanew <= 0 || cbRaw < sizeof(IMAGE_NT_HEADERS) ||
            (SIZE_T)pDos->e_lfanew > cbRaw - sizeof(IMAGE_NT_HEADERS) || (pDos->e_lfanew & 3))
        {
            _SEH2_YIELD(return NULL);
        }

        pNt = (PIMAGE_NT_HEADERS)(pbRaw + pDos->e_lfanew);
        if (pNt->Signature != IMAGE_NT_SIGNATURE)
            _SEH2_YIELD(return NULL);

#ifdef _M_AMD64
        if (pNt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
            pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        {
            imageBase = (ULONG_PTR)((PIMAGE_NT_HEADERS64)pNt)->OptionalHeader.ImageBase;
        }
#elif defined(_M_ARM64)
        if (pNt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
            pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64)
        {
            imageBase = (ULONG_PTR)((PIMAGE_NT_HEADERS64)pNt)->OptionalHeader.ImageBase;
        }
#elif defined(_M_ARM)
        if (pNt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_ARMNT)
        {
            imageBase = ((PIMAGE_NT_HEADERS32)pNt)->OptionalHeader.ImageBase;
        }
#elif defined(_M_IX86)
        if (pNt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
            pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
        {
            imageBase = ((PIMAGE_NT_HEADERS32)pNt)->OptionalHeader.ImageBase;
        }
#else
    #error Unsupported architecture.
#endif
        else
        {
            _SEH2_YIELD(return NULL);
        }

        cbImage = pNt->OptionalHeader.SizeOfImage;
        if (!cbImage || cbImage > KBDIMG_MAXSIZE)
            _SEH2_YIELD(return NULL);

        /* Keep pSec aligned: IMAGE_FIRST_SECTION depends on SizeOfOptionalHeader */
        if (pNt->FileHeader.SizeOfOptionalHeader & 3)
            _SEH2_YIELD(return NULL);

        /* The section headers must lie within the file (compare offsets, not pointers) */
        pSec = IMAGE_FIRST_SECTION(pNt);
        offSec = (SIZE_T)((PBYTE)pSec - pbRaw);
        if (offSec > cbRaw ||
            (cbRaw - offSec) / sizeof(IMAGE_SECTION_HEADER) < pNt->FileHeader.NumberOfSections)
        {
            _SEH2_YIELD(return NULL);
        }

        /* Find the .data section. Any other section is ignored. */
        for (i = 0; i < pNt->FileHeader.NumberOfSections; ++i)
        {
            if (RtlEqualMemory(pSec[i].Name, s_DataName, sizeof(s_DataName)))
            {
                pData = &pSec[i];
                break;
            }
        }
        if (!pData)
        {
            ERR("No .data section\n");
            _SEH2_YIELD(return NULL);
        }

        /* The part beyond SizeOfRawData (if any) is zero-filled, as the loader does */
        cbVirt = pData->Misc.VirtualSize ? pData->Misc.VirtualSize : pData->SizeOfRawData;
        cbCopy = min(pData->SizeOfRawData, cbVirt);
        if (!cbVirt || pData->VirtualAddress > cbImage ||
            cbVirt > cbImage - pData->VirtualAddress || pData->PointerToRawData > cbRaw ||
            cbCopy > cbRaw - pData->PointerToRawData)
        {
            ERR("Invalid .data section\n");
            _SEH2_YIELD(return NULL);
        }

        /* Retain the .data section only. The buffer starts at the beginning of .data,
         * so an RVA must be converted to an offset by subtracting offData. */
        pbImage = ExAllocatePoolZero(PagedPool, cbVirt, KBDIMG_TAG);
        if (!pbImage)
            _SEH2_YIELD(return NULL);
        RtlCopyMemory(pbImage, pbRaw + pData->PointerToRawData, cbCopy);

        pInfo->ImageBase = imageBase;
        pInfo->offData = pData->VirtualAddress;
        pInfo->cbData = cbVirt;
        *pcbImage = cbVirt;
        _SEH2_YIELD(return pbImage);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception in KbdBuildImage! Status: 0x%lx\n", _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (pbImage)
        ExFreePoolWithTag(pbImage, KBDIMG_TAG);
    return NULL;
}

/* Loads the image of keyboard layout file. See also KbdUnloadImage below. */
static PVOID
KbdLoadImage(
    _In_ HANDLE hKernelFile,
    _Out_ PULONG pcbImage,
    _Out_ PKBDIMAGE_INFO pInfo)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileInfo;
    LARGE_INTEGER Offset;
    PVOID pvRaw, pvImage;
    ULONG cbRaw;

    *pcbImage = 0;
    RtlZeroMemory(pInfo, sizeof(*pInfo));

    if (!hKernelFile)
        return NULL;

    /* Get the file size */
    Status = ZwQueryInformationFile(hKernelFile, &IoStatusBlock, &FileInfo,
                                    sizeof(FileInfo), FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        ERR("ZwQueryInformationFile failed: 0x%08X\n", Status);
        return NULL;
    }
    if (FileInfo.EndOfFile.HighPart != 0 || FileInfo.EndOfFile.LowPart > KBDIMG_MAXSIZE)
    {
        ERR("Invalid file size (%I64u bytes)\n", FileInfo.EndOfFile.QuadPart);
        return NULL;
    }
    cbRaw = FileInfo.EndOfFile.LowPart;
    if (cbRaw < sizeof(IMAGE_DOS_HEADER)) /* Too small to be a PE file? */
    {
        ERR("Invalid file size (%lu bytes)\n", cbRaw);
        return NULL;
    }

    /* Allocate raw image buffer */
    pvRaw = ExAllocatePoolWithTag(PagedPool, cbRaw, KBDIMG_TAG);
    if (!pvRaw)
        return NULL;

    /* Load the file content */
    Offset.QuadPart = 0;
    Status = ZwReadFile(hKernelFile, NULL, NULL, NULL, &IoStatusBlock,
                        pvRaw, cbRaw, &Offset, NULL);
    if (!NT_SUCCESS(Status) || IoStatusBlock.Information != cbRaw)
    {
        ERR("ZwReadFile failed: 0x%08X (read %Iu of %lu)\n",
            Status, IoStatusBlock.Information, cbRaw);
        ExFreePoolWithTag(pvRaw, KBDIMG_TAG);
        return NULL;
    }

    /* Take the .data section only (no relocation; see KbdRebaseTables) */
    pvImage = KbdBuildImage(pvRaw, cbRaw, pcbImage, pInfo);
    if (!pvImage)
        ERR("KbdBuildImage failed\n");

    ExFreePoolWithTag(pvRaw, KBDIMG_TAG);
    return pvImage;
}

static __inline VOID
KbdUnloadImage(
    _In_opt_ PVOID pvImageBase)
{
    if (pvImageBase)
        ExFreePoolWithTag(pvImageBase, KBDIMG_TAG);
}

/* Tells whether the RVA range [off, off + cb) lies within the .data section
 * (without overflowing) */
static __inline BOOL
KbdIsRangeInData(_In_ const KBDIMAGE_INFO *pInfo, _In_ ULONG_PTR off, _In_ SIZE_T cb)
{
    return (off >= pInfo->offData) && (cb <= pInfo->cbData) &&
           ((off - pInfo->offData) <= (pInfo->cbData - cb));
}

/* Limits on the number of elements walked in each table. Tables and strings of a crafted layout
 * may overlap each other, so these bound the work it can cause. They are far beyond what any
 * real layout needs. */
#define KBD_MAX_VKTOBIT         0x100
#define KBD_MAX_WCHTABLES       0x20
#define KBD_MAX_VKTOWCH         0x200
#define KBD_MAX_KEYNAMES        0x200
#define KBD_MAX_DEADNAMES       0x400
#define KBD_MAX_DEADKEYS        0x10000
#define KBD_MAX_VSCVK           0x100
#define KBD_MAX_LIGATURES       0x10000
#define KBD_MAX_NLS_ENTRIES     0x100
#define KBD_MAX_NAME_CCH        128     /* including the terminating NUL */
#define KBD_MAX_MOD_COLUMNS     10      /* the widest VK_TO_WCHARS<n> */
#define KBD_MAX_LIGATURE_CCH    5       /* the widest LIGATURE<n> */

/* The state of the validation. All the checks are done on the copy in our buffer. */
typedef struct _KBDVALIDATE
{
    PBYTE     pbImage;   /* The buffer (the .data section only) */
    ULONG     cbData;    /* Size of the buffer */
    ULONG_PTR ImageBase; /* The base address the pointers in the layout tables are based on */
    ULONG     offData;   /* RVA of the .data section in the file */
    PBYTE     pbSlots;   /* 1 bit per pointer-sized slot of the buffer that is already rebased */
} KBDVALIDATE, *PKBDVALIDATE;

/* Tells whether [p, p + cb) lies within the buffer (without overflowing) and p is aligned */
static BOOL
KbdIsBufRange(
    _In_ const KBDVALIDATE *pv,
    _In_ const VOID *p,
    _In_ SIZE_T cb,
    _In_ SIZE_T cbAlign)
{
    ULONG_PTR off;

    if ((ULONG_PTR)p < (ULONG_PTR)pv->pbImage)
        return FALSE;

    off = (ULONG_PTR)p - (ULONG_PTR)pv->pbImage;
    if ((off & (cbAlign - 1)) || cb > pv->cbData || off > pv->cbData - cb)
        return FALSE;

    return TRUE;
}

/**
 * @brief
 * Converts a pointer slot of the layout tables from a virtual address (relative to ImageBase)
 * to a pointer into our buffer, and verifies that the target has room for cbMin bytes.
 *
 * Every slot can be converted only once: if the tables overlap, the slot would hold an
 * already-converted pointer (a kernel address), which must never be treated as a virtual
 * address again.
 *
 * @param[in] ppSlot    The slot. It must lie within the buffer.
 * @param[in] cbMin     The number of bytes that must be available at the target.
 * @param[in] cbAlign   The alignment of the target.
 * @param[in] bRequired Whether a NULL pointer is rejected (otherwise, it is left as NULL).
 **/
static BOOL
KbdRebaseSlot(
    _In_ PKBDVALIDATE pv,
    _Inout_ PVOID *ppSlot,
    _In_ SIZE_T cbMin,
    _In_ SIZE_T cbAlign,
    _In_ BOOL bRequired,
    _In_ PCSTR pszName)
{
    ULONG_PTR va, rva, idx;

    if (!KbdIsBufRange(pv, ppSlot, sizeof(PVOID), sizeof(PVOID)))
    {
        ERR("Invalid slot of %s: 0x%p\n", pszName, ppSlot);
        return FALSE;
    }

    idx = ((ULONG_PTR)ppSlot - (ULONG_PTR)pv->pbImage) / sizeof(PVOID);
    if (pv->pbSlots[idx >> 3] & (1 << (idx & 7)))
    {
        ERR("%s overlaps another pointer\n", pszName);
        return FALSE;
    }
    pv->pbSlots[idx >> 3] |= (1 << (idx & 7));

    va = (ULONG_PTR)*ppSlot;
    if (!va)
    {
        if (bRequired)
        {
            ERR("%s is NULL\n", pszName);
            return FALSE;
        }
        return TRUE;
    }

    rva = va - pv->ImageBase;
    if (va < pv->ImageBase || rva < pv->offData || (rva - pv->offData) >= pv->cbData)
    {
        ERR("Invalid %s: 0x%p\n", pszName, (PVOID)va);
        return FALSE;
    }

    *ppSlot = pv->pbImage + (rva - pv->offData);
    if (!KbdIsBufRange(pv, *ppSlot, cbMin, cbAlign))
    {
        ERR("%s: No room for %Iu bytes at 0x%p\n", pszName, cbMin, (PVOID)va);
        *ppSlot = NULL;
        return FALSE;
    }
    return TRUE;
}

#define KBD_REBASE(pv, field, cbMin, cbAlign, bRequired) \
    KbdRebaseSlot((pv), (PVOID*)&(field), (cbMin), (cbAlign), (bRequired), #field)

/**
 * @brief
 * Measures a table that ends with a terminating element, verifying every element
 * (the terminator included) in full, that is, with the stride of the element.
 * The terminator is an element whose first cbKey bytes are all zero.
 **/
static BOOL
KbdMeasureTable(
    _In_ const KBDVALIDATE *pv,
    _In_ const VOID *pTable,
    _In_ SIZE_T cbElem,
    _In_ SIZE_T cbKey,
    _In_ SIZE_T cbAlign,
    _In_ ULONG cMax,
    _Out_ PULONG pcElems,
    _In_ PCSTR pszName)
{
    const BYTE *pb;
    SIZE_T k;
    ULONG i;

    *pcElems = 0;

    for (i = 0; i < cMax; ++i)
    {
        pb = (const BYTE *)pTable + i * cbElem;
        if (!KbdIsBufRange(pv, pb, cbElem, cbAlign))
        {
            ERR("%s: Element %lu is out of range\n", pszName, i);
            return FALSE;
        }

        for (k = 0; k < cbKey; ++k)
        {
            if (pb[k])
                break;
        }
        if (k == cbKey) /* The terminator */
        {
            *pcElems = i;
            return TRUE;
        }
    }

    ERR("%s: No terminator within %lu elements\n", pszName, cMax);
    return FALSE;
}

/* Verifies that the string is terminated within the buffer (and is not unreasonably long) */
static BOOL
KbdCheckWideString(_In_ const KBDVALIDATE *pv, _In_ const WCHAR *pwsz)
{
    SIZE_T i, cchMax;

    if (!KbdIsBufRange(pv, pwsz, sizeof(WCHAR), TYPE_ALIGNMENT(WCHAR)))
        return FALSE;

    cchMax = (pv->pbImage + pv->cbData - (const BYTE *)pwsz) / sizeof(WCHAR);
    cchMax = min(cchMax, KBD_MAX_NAME_CCH);
    for (i = 0; i < cchMax; ++i)
    {
        if (!pwsz[i])
            return TRUE;
    }

    ERR("Unterminated string at 0x%p\n", pwsz);
    return FALSE;
}

static BOOL
KbdCheckModifiers(_In_ PKBDVALIDATE pv, _Inout_ PKBDTABLES pTbl)
{
    PMODIFIERS pMod;
    ULONG i, cVkToBit;
    SIZE_T cbMod;

    if (!KBD_REBASE(pv, pTbl->pCharModifiers, sizeof(MODIFIERS), TYPE_ALIGNMENT(MODIFIERS), TRUE))
        return FALSE;
    pMod = pTbl->pCharModifiers;

    /* ModNumber[] is indexed by the modifier bits, up to wMaxModBits (inclusive) */
    cbMod = FIELD_OFFSET(MODIFIERS, ModNumber) + ((SIZE_T)pMod->wMaxModBits + 1) * sizeof(BYTE);
    if (pMod->wMaxModBits > 0xFF || !KbdIsBufRange(pv, pMod, cbMod, TYPE_ALIGNMENT(MODIFIERS)))
    {
        ERR("Invalid wMaxModBits: 0x%X\n", pMod->wMaxModBits);
        return FALSE;
    }
    for (i = 0; i <= pMod->wMaxModBits; ++i)
    {
        /* A column number must not exceed the widest VK_TO_WCHARS<n> */
        if (pMod->ModNumber[i] != SHFT_INVALID && pMod->ModNumber[i] >= KBD_MAX_MOD_COLUMNS)
        {
            ERR("Invalid ModNumber[%lu]: 0x%X\n", i, pMod->ModNumber[i]);
            return FALSE;
        }
    }

    if (!KBD_REBASE(pv, pMod->pVkToBit, sizeof(VK_TO_BIT), TYPE_ALIGNMENT(VK_TO_BIT), TRUE) ||
        !KbdMeasureTable(pv, pMod->pVkToBit, sizeof(VK_TO_BIT), sizeof(BYTE),
                         TYPE_ALIGNMENT(VK_TO_BIT), KBD_MAX_VKTOBIT, &cVkToBit, "pVkToBit"))
    {
        return FALSE;
    }
    return TRUE;
}

static BOOL
KbdCheckVkToWchars(
    _In_ PKBDVALIDATE pv,
    _Inout_ PKBDTABLES pTbl,
    _Out_ PBOOL pbLigature,
    _Out_ PBOOL pbDead)
{
    PVK_TO_WCHAR_TABLE pWchTbl;
    const WCHAR *pwch;
    SIZE_T cbEntry;
    ULONG iTbl, cTbls, iVk, cVks, iMod;

    *pbLigature = *pbDead = FALSE;

    /* This table ends with an entry that has NULL pVkToWchars */
    if (!KBD_REBASE(pv, pTbl->pVkToWcharTable, sizeof(VK_TO_WCHAR_TABLE),
                    TYPE_ALIGNMENT(VK_TO_WCHAR_TABLE), TRUE) ||
        !KbdMeasureTable(pv, pTbl->pVkToWcharTable, sizeof(VK_TO_WCHAR_TABLE), sizeof(PVOID),
                         TYPE_ALIGNMENT(VK_TO_WCHAR_TABLE), KBD_MAX_WCHTABLES, &cTbls,
                         "pVkToWcharTable"))
    {
        return FALSE;
    }
    if (cTbls == 0)
    {
        ERR("pVkToWcharTable is empty\n");
        return FALSE;
    }

    for (iTbl = 0; iTbl < cTbls; ++iTbl)
    {
        pWchTbl = &pTbl->pVkToWcharTable[iTbl];

        /* The stride must match the number of columns, or we would walk (and later
         * index) the entries wrongly */
        cbEntry = FIELD_OFFSET(VK_TO_WCHARS1, wch) + pWchTbl->nModifications * sizeof(WCHAR);
        if (pWchTbl->nModifications < 1 || pWchTbl->nModifications > KBD_MAX_MOD_COLUMNS ||
            pWchTbl->cbSize != cbEntry)
        {
            ERR("Invalid table: nModifications %u, cbSize %u\n",
                pWchTbl->nModifications, pWchTbl->cbSize);
            return FALSE;
        }

        /* The entries end with an entry that has VirtualKey == 0 */
        if (!KBD_REBASE(pv, pWchTbl->pVkToWchars, cbEntry, TYPE_ALIGNMENT(VK_TO_WCHARS1), TRUE) ||
            !KbdMeasureTable(pv, pWchTbl->pVkToWchars, cbEntry, sizeof(BYTE),
                             TYPE_ALIGNMENT(VK_TO_WCHARS1), KBD_MAX_VKTOWCH, &cVks,
                             "pVkToWchars"))
        {
            return FALSE;
        }

        for (iVk = 0; iVk < cVks; ++iVk)
        {
            pwch = (const WCHAR *)((PBYTE)pWchTbl->pVkToWchars + iVk * cbEntry +
                                   FIELD_OFFSET(VK_TO_WCHARS1, wch));
            for (iMod = 0; iMod < pWchTbl->nModifications; ++iMod)
            {
                if (pwch[iMod] == WCH_LGTR)
                    *pbLigature = TRUE;
                else if (pwch[iMod] == WCH_DEAD)
                    *pbDead = TRUE;
            }
        }
    }
    return TRUE;
}

/* Key names. This table ends with an entry that has vsc == 0. */
static BOOL
KbdCheckKeyNames(
    _In_ PKBDVALIDATE pv,
    _Inout_ VSC_LPWSTR **ppNames,
    _In_ PCSTR pszName)
{
    VSC_LPWSTR *pNames;
    ULONG i, cNames;

    if (!KbdRebaseSlot(pv, (PVOID*)ppNames, sizeof(VSC_LPWSTR), TYPE_ALIGNMENT(VSC_LPWSTR),
                       TRUE, pszName) ||
        !KbdMeasureTable(pv, *ppNames, sizeof(VSC_LPWSTR), sizeof(BYTE),
                         TYPE_ALIGNMENT(VSC_LPWSTR), KBD_MAX_KEYNAMES, &cNames, pszName))
    {
        return FALSE;
    }

    pNames = *ppNames;
    for (i = 0; i < cNames; ++i)
    {
        if (!KbdRebaseSlot(pv, (PVOID*)&pNames[i].pwsz, sizeof(WCHAR), TYPE_ALIGNMENT(WCHAR),
                           TRUE, pszName) ||
            !KbdCheckWideString(pv, pNames[i].pwsz))
        {
            ERR("%s[%lu]: Invalid name\n", pszName, i);
            return FALSE;
        }
    }
    return TRUE;
}

/* Dead key names (optional). This array of strings ends with NULL. */
static BOOL
KbdCheckDeadKeyNames(_In_ PKBDVALIDATE pv, _Inout_ PKBDTABLES pTbl)
{
    LPWSTR *ppwsz;
    ULONG i, cNames;

    if (!KBD_REBASE(pv, pTbl->pKeyNamesDead, sizeof(LPWSTR), TYPE_ALIGNMENT(LPWSTR), FALSE))
        return FALSE;
    if (!pTbl->pKeyNamesDead)
        return TRUE;

    if (!KbdMeasureTable(pv, pTbl->pKeyNamesDead, sizeof(LPWSTR), sizeof(LPWSTR),
                         TYPE_ALIGNMENT(LPWSTR), KBD_MAX_DEADNAMES, &cNames, "pKeyNamesDead"))
    {
        return FALSE;
    }

    ppwsz = pTbl->pKeyNamesDead;
    for (i = 0; i < cNames; ++i)
    {
        if (!KbdRebaseSlot(pv, (PVOID*)&ppwsz[i], sizeof(WCHAR), TYPE_ALIGNMENT(WCHAR),
                           TRUE, "pKeyNamesDead[]") ||
            !KbdCheckWideString(pv, ppwsz[i]))
        {
            ERR("pKeyNamesDead[%lu]: Invalid name\n", i);
            return FALSE;
        }
    }
    return TRUE;
}

/* The VSC -> VK tables for the E0/E1 prefixes. They end with an entry that has Vsc == 0. */
static BOOL
KbdCheckVscToVk(
    _In_ PKBDVALIDATE pv,
    _Inout_ PVSC_VK *ppVscVk,
    _In_ PCSTR pszName)
{
    ULONG cEntries;

    return KbdRebaseSlot(pv, (PVOID*)ppVscVk, sizeof(VSC_VK), TYPE_ALIGNMENT(VSC_VK), TRUE,
                         pszName) &&
           KbdMeasureTable(pv, *ppVscVk, sizeof(VSC_VK), sizeof(BYTE), TYPE_ALIGNMENT(VSC_VK),
                           KBD_MAX_VSCVK, &cEntries, pszName);
}

/* Dead keys (optional). This table ends with an entry that has dwBoth == 0. */
static BOOL
KbdCheckDeadKeys(_In_ PKBDVALIDATE pv, _Inout_ PKBDTABLES pTbl)
{
    ULONG cEntries;

    if (!KBD_REBASE(pv, pTbl->pDeadKey, sizeof(DEADKEY), TYPE_ALIGNMENT(DEADKEY), FALSE))
        return FALSE;
    if (!pTbl->pDeadKey)
        return TRUE;

    return KbdMeasureTable(pv, pTbl->pDeadKey, sizeof(DEADKEY), sizeof(DWORD),
                           TYPE_ALIGNMENT(DEADKEY), KBD_MAX_DEADKEYS, &cEntries, "pDeadKey");
}

/* Ligatures (optional). This table ends with an entry that has VirtualKey == 0. */
static BOOL
KbdCheckLigatures(_In_ PKBDVALIDATE pv, _Inout_ PKBDTABLES pTbl)
{
    ULONG cEntries;
    SIZE_T cbEntry;

    if (!pTbl->pLigature) /* Not yet rebased, but we only test it for NULL here */
        return KBD_REBASE(pv, pTbl->pLigature, 0, 1, FALSE);

    cbEntry = FIELD_OFFSET(LIGATURE1, wch) + pTbl->nLgMaxd * sizeof(WCHAR);
    if (pTbl->nLgMaxd < 1 || pTbl->nLgMaxd > KBD_MAX_LIGATURE_CCH || pTbl->cbLgEntry != cbEntry)
    {
        ERR("Invalid ligature stride: nLgMaxd %u, cbLgEntry %u\n",
            pTbl->nLgMaxd, pTbl->cbLgEntry);
        return FALSE;
    }

    return KBD_REBASE(pv, pTbl->pLigature, cbEntry, TYPE_ALIGNMENT(LIGATURE1), TRUE) &&
           KbdMeasureTable(pv, pTbl->pLigature, cbEntry, sizeof(BYTE), TYPE_ALIGNMENT(LIGATURE1),
                           KBD_MAX_LIGATURES, &cEntries, "pLigature");
}

/* The NLS tables of the Far-East layouts. These tables are counted, not terminated. */
static BOOL
KbdCheckNls(_In_ PKBDVALIDATE pv, _Inout_ PKBDNLSTABLES pNls)
{
    PVK_F pVkToF;
    ULONG i, j;

    if (pNls->NumOfVkToF > KBD_MAX_NLS_ENTRIES ||
        pNls->NumOfMouseVKey < 0 || pNls->NumOfMouseVKey > KBD_MAX_NLS_ENTRIES)
    {
        ERR("Invalid NLS counts: %lu, %d\n", pNls->NumOfVkToF, pNls->NumOfMouseVKey);
        return FALSE;
    }

    /* A pointer is required only if its count is not zero */
    if (!KBD_REBASE(pv, pNls->pVkToF, pNls->NumOfVkToF * sizeof(VK_F), TYPE_ALIGNMENT(VK_F),
                    pNls->NumOfVkToF != 0) ||
        !KBD_REBASE(pv, pNls->pusMouseVKey, pNls->NumOfMouseVKey * sizeof(USHORT),
                    TYPE_ALIGNMENT(USHORT), pNls->NumOfMouseVKey != 0))
    {
        return FALSE;
    }

    /* The indexes in these entries select the functions and the arrays of 8 elements */
    pVkToF = pNls->pVkToF;
    for (i = 0; i < pNls->NumOfVkToF; ++i)
    {
        if (pVkToF[i].NLSFEProcType > KBDNLS_TYPE_TOGGLE ||
            pVkToF[i].NLSFEProcCurrent >= RTL_NUMBER_OF(pVkToF[i].NLSFEProc))
        {
            ERR("Invalid VK_F[%lu]\n", i);
            return FALSE;
        }
        for (j = 0; j < RTL_NUMBER_OF(pVkToF[i].NLSFEProc); ++j)
        {
            if (pVkToF[i].NLSFEProc[j].NLSFEProcIndex > KBDNLS_CONV_OR_NONCONV ||
                pVkToF[i].NLSFEProcAlt[j].NLSFEProcIndex > KBDNLS_CONV_OR_NONCONV)
            {
                ERR("Invalid VK_F[%lu] procedure\n", i);
                return FALSE;
            }
        }
    }
    return TRUE;
}

/**
 * @brief
 * Adjusts the pointers inside the layout tables, and validates the whole layout.
 *
 * In the file, these pointers are virtual addresses relative to ImageBase,
 * so the RVA of the target is (VA - ImageBase), and the adjusted pointer
 * is the offset (RVA - offData) applied to our buffer, which holds the .data
 * section only. The tables are modified in place (in our own copy).
 *
 * The layout comes from user mode, so nothing in it is trusted:
 *  - The pointers that the keyboard code uses unconditionally must not be NULL.
 *  - Every counted table, and every table that ends with a terminator, must lie in
 *    the .data section in full: each element with its stride (including the terminator
 *    element), and the string with its terminating NUL.
 *  - No pointer slot can be taken as a virtual address twice (overlapping tables).
 *
 * @return
 * Returns TRUE if the layout is valid, or FALSE if anything is invalid. The layout must
 * not be used (or installed) unless this returns TRUE.
 **/
static BOOL
KbdRebaseTables(
    _In_ PBYTE pbImage,
    _In_ const KBDIMAGE_INFO *pInfo,
    _Inout_ PKBDTABLES pTbl,
    _Inout_opt_ PKBDNLSTABLES pNls)
{
    KBDVALIDATE v;
    BOOL bLigature, bDead, ret = FALSE;

    v.pbImage = pbImage;
    v.cbData = pInfo->cbData;
    v.ImageBase = pInfo->ImageBase;
    v.offData = pInfo->offData;
    v.pbSlots = ExAllocatePoolZero(PagedPool, v.cbData / (sizeof(PVOID) * 8) + 1, KBDIMG_TAG);
    if (!v.pbSlots)
        return FALSE;

    /* The root tables themselves */
    if (!KbdIsBufRange(&v, pTbl, sizeof(*pTbl), TYPE_ALIGNMENT(KBDTABLES)) ||
        (pNls && !KbdIsBufRange(&v, pNls, sizeof(*pNls), TYPE_ALIGNMENT(KBDNLSTABLES))))
    {
        ERR("Misplaced root tables\n");
        goto Quit;
    }

    /* pusVSCtoVK is counted by bMaxVSCtoVK */
    if (pTbl->bMaxVSCtoVK == 0)
    {
        ERR("bMaxVSCtoVK is zero\n");
        goto Quit;
    }

    if (!KbdCheckModifiers(&v, pTbl) ||
        !KbdCheckVkToWchars(&v, pTbl, &bLigature, &bDead) ||
        !KbdCheckKeyNames(&v, &pTbl->pKeyNames, "pKeyNames") ||
        !KbdCheckKeyNames(&v, &pTbl->pKeyNamesExt, "pKeyNamesExt") ||
        !KbdCheckDeadKeyNames(&v, pTbl) ||
        !KbdCheckDeadKeys(&v, pTbl) ||
        !KBD_REBASE(&v, pTbl->pusVSCtoVK, pTbl->bMaxVSCtoVK * sizeof(USHORT),
                    TYPE_ALIGNMENT(USHORT), TRUE) ||
        !KbdCheckVscToVk(&v, &pTbl->pVSCtoVK_E0, "pVSCtoVK_E0") ||
        !KbdCheckVscToVk(&v, &pTbl->pVSCtoVK_E1, "pVSCtoVK_E1") ||
        !KbdCheckLigatures(&v, pTbl))
    {
        goto Quit;
    }

    /* WCH_LGTR and WCH_DEAD refer to these tables */
    if ((bLigature && !pTbl->pLigature) || (bDead && !pTbl->pDeadKey))
    {
        ERR("The layout uses WCH_LGTR/WCH_DEAD without the table\n");
        goto Quit;
    }

    if (pNls && !KbdCheckNls(&v, pNls))
        goto Quit;

    ret = TRUE;

Quit:
    ExFreePoolWithTag(v.pbSlots, KBDIMG_TAG);
    return ret;
}

/* Loads keyboard layout DLL and gets address to KbdTables */
static BOOL
UserLoadKbdDll(
    _In_ HANDLE hKernelFile,
    _In_ ULONG offTable,
    _Out_ HANDLE *phModule,
    _Out_ PULONG pcbImage,
    _Out_ PKBDTABLES *ppKbdTables,
    _Out_ PKBDNLSTABLES *ppKbdNlsTables)
{
    PBYTE pbImage;
    ULONG cbImage, wMainOffset = LOWORD(offTable), wNlsOffset = HIWORD(offTable);
    KBDIMAGE_INFO info;
    HANDLE hModule;
    PKBDTABLES pKbdTables;
    PKBDNLSTABLES pKbdNlsTables;

    *phModule = NULL;
    *ppKbdTables = NULL;
    *ppKbdNlsTables = NULL;

    if (!wMainOffset)
        return FALSE;

    /* Load keyboard layout DLL */
    hModule = KbdLoadImage(hKernelFile, &cbImage, &info);
    if (!hModule)
        return FALSE;

    /* offTable comes from user mode: the tables must lie entirely within the .data section */
    if (!KbdIsRangeInData(&info, wMainOffset, sizeof(**ppKbdTables)) ||
        (wNlsOffset && !KbdIsRangeInData(&info, wNlsOffset, sizeof(**ppKbdNlsTables))))
    {
        ERR("Table offset out of range: offTable 0x%08X, .data 0x%08lX+0x%08lX\n",
            offTable, info.offData, info.cbData);
        KbdUnloadImage(hModule);
        return FALSE;
    }

    /* The buffer holds the .data section only: convert the RVAs to the offsets in the buffer */
    pbImage = (PBYTE)hModule;
    pKbdTables = (PKBDTABLES)RVA(wMainOffset - info.offData, pbImage);
    pKbdNlsTables = wNlsOffset ? (PKBDNLSTABLES)RVA(wNlsOffset - info.offData, pbImage) : NULL;

    _SEH2_TRY
    {
        /* Adjust the pointers inside the tables using RVA (instead of the base relocations),
           and validate their targets */
        if (!KbdRebaseTables(pbImage, &info, pKbdTables, pKbdNlsTables))
        {
            ERR("Invalid pointer in the layout tables\n");
            KbdUnloadImage(hModule);
            _SEH2_YIELD(return FALSE);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Exception in UserLoadKbdDll! Status: 0x%lx\n", _SEH2_GetExceptionCode());
        KbdUnloadImage(hModule);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    *phModule = hModule;
    *pcbImage = cbImage;
    *ppKbdTables = pKbdTables;
    *ppKbdNlsTables = pKbdNlsTables;

#if 0 && DBG
    /* Dump keyboard layout */
    DumpKbdLayout(*ppKbdTables);
#endif

    return TRUE;
}

/* Loads keyboard layout DLL and creates KBDFILE object */
static PKBDFILE
UserLoadKbdFile(
    _In_ HANDLE hKernelFile,
    _In_ ULONG offTable,
    _In_ PCUNICODE_STRING pustrKLID)
{
    PKBDFILE pkf, pRet = NULL;

    /* Create keyboard layout file object */
    pkf = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDFILE, sizeof(KBDFILE));
    if (!pkf)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    _swprintf(pkf->awchKF, L"%wZ", pustrKLID); /* Set keyboard layout name */

    /* Load keyboard file now */
    if (!UserLoadKbdDll(hKernelFile, offTable, &pkf->hBase, &pkf->Size,
                        &pkf->pKbdTbl, &pkf->pKbdNlsTbl))
    {
        ERR("Failed to load layout file!\n");
        goto cleanup;
    }

    /* Update next field */
    pkf->pkfNext = gpkfList;
    gpkfList = pkf;

    /* Return keyboard file */
    pRet = pkf;

cleanup:
    if (pkf)
        UserDereferenceObject(pkf); // we dont need ptr anymore
    if (!pRet && pkf) /* If we have failed, destroy the created object */
        UserDeleteObject(UserHMGetHandle(pkf), TYPE_KBDFILE);
    return pRet;
}

/* Loads keyboard layout and creates KL object */
static PKL
co_UserLoadKbdLayout(
    _In_ HANDLE hKernelFile,
    _In_ ULONG offTable,
    _In_ const UNICODE_STRING* pustrKLID,
    _In_ HKL hKL)
{
    LCID lCid;
    CHARSETINFO cs;
    PKL pKl;

    if (!hKernelFile || !offTable)
    {
        ERR("hKernelFile %p, offTable 0x%08X\n", hKernelFile, offTable);
        return NULL;
    }

    // Retrieve the language identifier.
    // NOTE: Do this before creating any object, so that a failure here leaks nothing.
    if (!NT_SUCCESS(RtlUnicodeStringToInteger(pustrKLID, 16, (PULONG)&lCid)))
    {
        ERR("RtlUnicodeStringToInteger failed for '%wZ'\n", pustrKLID);
        return NULL;
    }

    TRACE("Language Identifiers %wZ LCID 0x%x\n", pustrKLID, lCid);

    /* Create keyboard layout object */
    pKl = UserCreateObject(gHandleTable, NULL, NULL, NULL, TYPE_KBDLAYOUT, sizeof(KL));
    if (!pKl)
    {
        ERR("Failed to create object!\n");
        return NULL;
    }

    pKl->hkl = hKL;
    pKl->spkf = UserLoadKbdFile(hKernelFile, offTable, pustrKLID);

    /* Dereference keyboard layout */
    UserDereferenceObject(pKl);

    /* If we failed, remove KL object */
    if (!pKl->spkf)
    {
        ERR("UserLoadKbdFile(%wZ) failed!\n", pustrKLID);
        UserDeleteObject(UserHMGetHandle(pKl), TYPE_KBDLAYOUT);
        return NULL;
    }

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

    KbdUnloadImage(pkf->hBase);
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
        if (pKl->pklNext == pKl)
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
    _Inout_ PWINSTATION_OBJECT pWinSta,
    _In_ HANDLE hKernelFile,
    _In_opt_ HKL hOldKL,
    _In_ ULONG offTable,
    _In_ PUNICODE_STRING puszSafeKLID,
    _In_ HKL hNewKL,
    _In_ UINT Flags)
{
    PKL pOldKL, pNewKL;

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
        pNewKL = co_UserLoadKbdLayout(hKernelFile, offTable, puszSafeKLID, hNewKL);
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

typedef struct tagKBDFILE_NAME
{
    OBJECT_NAME_INFORMATION Info;
    WCHAR Buffer[MAX_PATH];
} KBDFILE_NAME, *PKBDFILE_NAME;

/* Gets the full NT path of a file or directory handle. */
static NTSTATUS IntQueryHandleName(HANDLE hObject, PKBDFILE_NAME pName)
{
    ULONG cbReturned;
    return ZwQueryObject(hObject, ObjectNameInformation, pName, sizeof(*pName), &cbReturned);
}

static BOOL FASTCALL
IntEqualDir(
    _In_ PCUNICODE_STRING pustrParent,
    _In_ PCUNICODE_STRING pustrDir)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE hDir;
    KBDFILE_NAME Name;

    /* Get the canonical name of the directory */
    InitializeObjectAttributes(&ObjectAttributes, (PUNICODE_STRING)pustrDir,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwOpenFile(&hDir, FILE_LIST_DIRECTORY | SYNCHRONIZE, &ObjectAttributes,
                        &IoStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        ERR("ZwOpenFile(%wZ) failed, Status 0x%08lx\n", pustrDir, Status);
        return FALSE;
    }

    Status = IntQueryHandleName(hDir, &Name);
    ZwClose(hDir);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntQueryHandleName(%p) failed, Status 0x%08lx\n", hDir, Status);
        return FALSE;
    }

    if (!RtlEqualUnicodeString(pustrParent, &Name.Info.Name, TRUE))
        return FALSE;

    return TRUE;
}

/**
 * @brief
 * Validates that the given user-mode handle corresponds to an opened file in
 * SystemRoot\system32.
 *
 * If so, duplicates it into a kernel-mode handle.
 *
 * @param[in]   hFile
 * User-mode handle to a file. Accept it only if the file is in system32.
 *
 * @return
 * On success, returns a NEW kernel handle (OBJ_KERNEL_HANDLE) to the same file object.
 * The caller must release it with ZwClose and must use it, not hFile, for all further I/O:
 * the user-mode handle value can be closed and reused by another thread right after the
 * check. The caller's own handle is never closed here (user32 owns and closes it).
 *
 * @note
 * Only synchronous file objects are accepted, because KbdLoadImage calls ZwReadFile without
 * an event and relies on the read being complete when it returns.
 **/
HANDLE FASTCALL IntVerifyKeyboardFileHandle(HANDLE hFile)
{
    static const UNICODE_STRING ustrDir = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32");
    KBDFILE_NAME Name;
    UNICODE_STRING ustrParent;
    PFILE_OBJECT pFileObject = NULL;
    HANDLE hKernelFile = NULL;
    NTSTATUS Status;

    if (!hFile)
        return NULL;

    /* Resolve the user handle exactly once and pin the file object */
    Status = ObReferenceObjectByHandle(hFile, FILE_READ_DATA, *IoFileObjectType, UserMode,
                                       (PVOID*)&pFileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("ObReferenceObjectByHandle(%p) failed, Status 0x%08lx\n", hFile, Status);
        return NULL;
    }

    if (!(pFileObject->Flags & FO_SYNCHRONOUS_IO))
    {
        ERR("Not a synchronous file: %p\n", hFile);
        ObDereferenceObject(pFileObject);
        return NULL;
    }

    /* Duplicate the handle for kernel-mode, which user mode cannot close or swap */
    Status = ObOpenObjectByPointer(pFileObject, OBJ_KERNEL_HANDLE, NULL,
                                   FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                   *IoFileObjectType, KernelMode, &hKernelFile);
    ObDereferenceObject(pFileObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("ObOpenObjectByPointer(%p) failed, Status 0x%08lx\n", hFile, Status);
        return NULL;
    }

    /* Get the canonical name of the file */
    Status = IntQueryHandleName(hKernelFile, &Name);
    if (!NT_SUCCESS(Status))
    {
        ERR("IntQueryHandleName(%p) failed, Status 0x%08lx\n", hFile, Status);
        goto Failure;
    }

    /* Strip the filename of the file */
    ustrParent = Name.Info.Name;
    while (ustrParent.Length > 0)
    {
        ustrParent.Length -= sizeof(WCHAR);
        if (ustrParent.Buffer[ustrParent.Length / sizeof(WCHAR)] == L'\\')
            break;
    }

    if (!IntEqualDir(&ustrParent, &ustrDir))
    {
        ERR("Not in system32: %wZ\n", &Name.Info.Name);
        goto Failure;
    }

    return hKernelFile; /* Success */

Failure:
    ZwClose(hKernelFile);
    return NULL;
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
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto Quit;
    }

    _SEH2_TRY
    {
        ProbeForWrite(pHklBuff, nBuff * sizeof(HKL), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(goto Quit);
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
        _SEH2_YIELD(goto Quit);
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
                _SEH2_YIELD(goto cleanup);
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
 */
HKL
NTAPI
NtUserLoadKeyboardLayoutEx(
    _In_ HANDLE hFile,
    _In_ ULONG offTable,
    _In_opt_ PKBDTABLE_MULTI_EX pMulti,
    _In_opt_ HKL hOldKL,
    _In_ PUNICODE_STRING puszKLID,
    _In_ ULONG dwNewKL,
    _In_ UINT Flags)
{
    HKL hRetKL = NULL;
    WCHAR Buffer[KL_NAMELENGTH];
    UNICODE_STRING uszSafeKLID;
    PWINSTATION_OBJECT pWinSta;
    HANDLE hKernelFile = NULL; /* Local kernel handle duplicated from hFile */
    HANDLE ahKernelAuxFile[KBDTABLE_MULTI_MAX] = { NULL }; /* Ditto, for the aux files */
    KBDTABLE_MULTI_EX multi;
    ULONG iTable;

    if (Flags & ~(KLF_ACTIVATE|KLF_NOTELLSHELL|KLF_REORDER|KLF_REPLACELANG|
                  KLF_SUBSTITUTE_OK|KLF_SETFORPROCESS|KLF_UNLOADPREVIOUS|
                  KLF_RESET|KLF_SHIFTLOCK))
    {
        ERR("Invalid flags: %x\n", Flags);
        EngSetLastError(ERROR_INVALID_FLAGS);
        return NULL;
    }

    RtlInitEmptyUnicodeString(&uszSafeKLID, Buffer, sizeof(Buffer));
    RtlZeroMemory(&multi, sizeof(multi));

    /* Read from paramters */
    _SEH2_TRY
    {
        ProbeForRead(puszKLID, sizeof(*puszKLID), 1);
        ProbeForRead(puszKLID->Buffer, sizeof(puszKLID->Length), 1); /* FIXME */
        RtlCopyUnicodeString(&uszSafeKLID, puszKLID);
        if (pMulti)
        {
            ProbeForRead(pMulti, sizeof(*pMulti), 1);
            multi = *pMulti;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;

    UserEnterExclusive();

    /* Validate data */
    if (!hFile || !offTable || multi.Multi.nTables > KBDTABLE_MULTI_MAX)
    {
        ERR("hFile:%p, nTables:%u, offTable:0x%08X\n", hFile, multi.Multi.nTables, offTable);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        goto Quit;
    }

    /* Verify file handles. The handles passed in belong to user32, which closes them itself.
     * We only work with (and close) our own kernel handles made by the verification. */
    hKernelFile = IntVerifyKeyboardFileHandle(hFile);
    if (!hKernelFile)
        goto Quit;
    for (iTable = 0; iTable < multi.Multi.nTables; ++iTable) /* nTables < MAX is checked above */
    {
        HANDLE hAuxFile = multi.AuxEntries[iTable].hFile;
        /* user32 tolerates a failed aux file by storing NULL */
        if (!hAuxFile)
            continue;

        ahKernelAuxFile[iTable] = IntVerifyKeyboardFileHandle(hAuxFile);
        if (!ahKernelAuxFile[iTable])
            goto Quit;
    }

    /* Load it now */
    pWinSta = IntGetProcessWindowStation(NULL);
    hRetKL = co_IntLoadKeyboardLayoutEx(pWinSta, hKernelFile, hOldKL, offTable, &uszSafeKLID,
                                        UlongToHandle(dwNewKL), Flags);
Quit:
    /* Close our kernel handles. Never use the (unvalidated) user-supplied nTables here. */
    if (hKernelFile)
        ZwClose(hKernelFile);
    for (iTable = 0; iTable < RTL_NUMBER_OF(ahKernelAuxFile); ++iTable)
    {
        if (ahKernelAuxFile[iTable])
            ZwClose(ahKernelAuxFile[iTable]);
    }

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
