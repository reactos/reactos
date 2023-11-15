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
