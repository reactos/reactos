/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Routines for x86 / x64 CPU information gathering
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <x86x64/Cpuid.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*!
 * \brief Get the CPUID information for a given CPU
 *
 * \param[out] VendorString - Pointer to a buffer to receive the vendor string
 */
VOID
NTAPI
KiGetCpuVendorString(
    _Out_writes_z_(CPU_VENDOR_STR_LEN) CHAR VendorString[CPU_VENDOR_STR_LEN])
{
    CPUID_SIGNATURE_REGS Signature;

    /* Get the Vendor string */
    __cpuid(&Signature.AsInt32[0], CPUID_SIGNATURE);

    /* Copy and null-terminate it */
    *(ULONG*)&VendorString[0] = *(ULONG*)&Signature.SignatureScrambled[0];
    *(ULONG*)&VendorString[4] = *(ULONG*)&Signature.SignatureScrambled[8];
    *(ULONG*)&VendorString[8] = *(ULONG*)&Signature.SignatureScrambled[4];
    VendorString[12] = 0;
}

/*!
 * \brief Identify the CPU vendor by the vendor string
 *
 * \param[in] VendorString - Pointer to a buffer containing the vendor string
 *
 * \return The CPU_VENDOR enum value that corresponds to the vendor string
 *
 * \see
 * - https://en.wikipedia.org/wiki/CPUID
 * - https://github.com/InstLatx64/InstLatx64
 *
 * \remark This function is called very early during the boot process, before
 *         the kernel debugger is initialized, so it must not do any debug prints.
 */
CPU_VENDORS
NTAPI
KiIdentifyCpuVendor(
    _In_reads_z_(CPU_VENDOR_STR_LEN) const CHAR VendorString[CPU_VENDOR_STR_LEN])
{
    /* Identify the CPU vendor */
    if (!strcmp(VendorString, "GenuineIntel") ||
        !strcmp(VendorString, "GenuineIotel")) // Intel Xeon E3-1231 v3
    {
        return CPU_INTEL;
    }
    else if (!strcmp(VendorString, "AuthenticAMD") ||
             !strcmp(VendorString, "HygonGenuine"))
    {
        return CPU_AMD;
    }
    else if (!strcmp(VendorString, "CentaurHauls") ||
             !strcmp(VendorString, "  Shanghai  "))
    {
        return CPU_VIA; // == CPU_CENTAUR
    }
#ifdef _M_I386
    else if (!strcmp(VendorString, "CyrixInstead"))
    {
        return CPU_CYRIX;
    }
    else if (!strcmp(VendorString, "GenuineTMx86"))
    {
        return CPU_TRANSMETA;
    }
    else if (!strcmp(VendorString, "RiseRiseRise"))
    {
        return CPU_RISE;
    }
#endif // _M_I386

    return CPU_UNKNOWN;
}

/*!
 * \brief Get the CPU signature
 *
 * \param[out] Family - Pointer to a USHORT to receive the family
 * \param[out] Model - Pointer to a USHORT to receive the model
 * \param[out] Stepping - Pointer to a USHORT to receive the stepping
 * 
 * \See
 * - https://www.sandpile.org/x86/cpuid.htm#level_0000_0001h
 * - https://github.com/InstLatx64/InstLatx64
 * 
 * - Intel:
 *   - https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2a-manual.pdf#G5.876260
 *   - https://www.scss.tcd.ie/Jeremy.Jones/CS4021/processor-identification-cpuid-instruction-note.pdf
 *   - https://web.archive.org/web/20250907014024/https://en.wikichip.org/wiki/intel/cpuid
 * - AMD:
 *   - https://docs.amd.com/v/u/en-US/24594_3.37 (Appendix E)
 *   - https://ia803100.us.archive.org/29/items/advancedmicrodevices_24594_3.28/24594.pdf
 *   - https://kib.kiev.ua/x86docs/AMD/AMD-CPUID-Spec/20734-r3.13.pdf
 *   - https://web.archive.org/web/20251208232456/https://en.wikichip.org/wiki/amd/cpuid
 * - Cyrix:
 *   - http://bitsavers.computerhistory.org/components/cyrix/appnotes/Cyrix_CPU_Detection_Guide_1997.pdf
 *
 */
VOID
NTAPI
KiGetCpuSignature(
    _Out_ PUSHORT Family,
    _Out_ PUSHORT Model,
    _Out_ PUSHORT Stepping)
{
    CPUID_VERSION_INFO_REGS VersionInfo;

    /* Get the CPUID */
    __cpuid(VersionInfo.AsInt32, 1);

    /* Get the family */
    *Family = VersionInfo.Eax.Bits.FamilyId;
    if (VersionInfo.Eax.Bits.FamilyId == 15)
    {
        *Family += VersionInfo.Eax.Bits.ExtendedFamilyId;
    }

    /* Get the model */
    *Model = VersionInfo.Eax.Bits.Model;
    if ((VersionInfo.Eax.Bits.FamilyId == 6) ||
        (VersionInfo.Eax.Bits.FamilyId == 15))
    {
        /* For Family < 15, AMD documents extended model as RAZ (reads as zero) */
        *Model |= VersionInfo.Eax.Bits.ExtendedModelId << 4;
    }

    /* Get the stepping */
    *Stepping = VersionInfo.Eax.Bits.SteppingId;
}
