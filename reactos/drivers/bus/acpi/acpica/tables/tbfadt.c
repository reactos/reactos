/******************************************************************************
 *
 * Module Name: tbfadt   - FADT table utilities
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbfadt")

/* Local prototypes */

static void
AcpiTbInitGenericAddress (
    ACPI_GENERIC_ADDRESS    *GenericAddress,
    UINT8                   SpaceId,
    UINT8                   ByteWidth,
    UINT64                  Address,
    const char              *RegisterName,
    UINT8                   Flags);

static void
AcpiTbConvertFadt (
    void);

static void
AcpiTbSetupFadtRegisters (
    void);

static UINT64
AcpiTbSelectAddress (
    char                    *RegisterName,
    UINT32                  Address32,
    UINT64                  Address64);


/* Table for conversion of FADT to common internal format and FADT validation */

typedef struct acpi_fadt_info
{
    const char              *Name;
    UINT16                  Address64;
    UINT16                  Address32;
    UINT16                  Length;
    UINT8                   DefaultLength;
    UINT8                   Flags;

} ACPI_FADT_INFO;

#define ACPI_FADT_OPTIONAL          0
#define ACPI_FADT_REQUIRED          1
#define ACPI_FADT_SEPARATE_LENGTH   2
#define ACPI_FADT_GPE_REGISTER      4

static ACPI_FADT_INFO     FadtInfoTable[] =
{
    {"Pm1aEventBlock",
        ACPI_FADT_OFFSET (XPm1aEventBlock),
        ACPI_FADT_OFFSET (Pm1aEventBlock),
        ACPI_FADT_OFFSET (Pm1EventLength),
        ACPI_PM1_REGISTER_WIDTH * 2,        /* Enable + Status register */
        ACPI_FADT_REQUIRED},

    {"Pm1bEventBlock",
        ACPI_FADT_OFFSET (XPm1bEventBlock),
        ACPI_FADT_OFFSET (Pm1bEventBlock),
        ACPI_FADT_OFFSET (Pm1EventLength),
        ACPI_PM1_REGISTER_WIDTH * 2,        /* Enable + Status register */
        ACPI_FADT_OPTIONAL},

    {"Pm1aControlBlock",
        ACPI_FADT_OFFSET (XPm1aControlBlock),
        ACPI_FADT_OFFSET (Pm1aControlBlock),
        ACPI_FADT_OFFSET (Pm1ControlLength),
        ACPI_PM1_REGISTER_WIDTH,
        ACPI_FADT_REQUIRED},

    {"Pm1bControlBlock",
        ACPI_FADT_OFFSET (XPm1bControlBlock),
        ACPI_FADT_OFFSET (Pm1bControlBlock),
        ACPI_FADT_OFFSET (Pm1ControlLength),
        ACPI_PM1_REGISTER_WIDTH,
        ACPI_FADT_OPTIONAL},

    {"Pm2ControlBlock",
        ACPI_FADT_OFFSET (XPm2ControlBlock),
        ACPI_FADT_OFFSET (Pm2ControlBlock),
        ACPI_FADT_OFFSET (Pm2ControlLength),
        ACPI_PM2_REGISTER_WIDTH,
        ACPI_FADT_SEPARATE_LENGTH},

    {"PmTimerBlock",
        ACPI_FADT_OFFSET (XPmTimerBlock),
        ACPI_FADT_OFFSET (PmTimerBlock),
        ACPI_FADT_OFFSET (PmTimerLength),
        ACPI_PM_TIMER_WIDTH,
        ACPI_FADT_SEPARATE_LENGTH},         /* ACPI 5.0A: Timer is optional */

    {"Gpe0Block",
        ACPI_FADT_OFFSET (XGpe0Block),
        ACPI_FADT_OFFSET (Gpe0Block),
        ACPI_FADT_OFFSET (Gpe0BlockLength),
        0,
        ACPI_FADT_SEPARATE_LENGTH | ACPI_FADT_GPE_REGISTER},

    {"Gpe1Block",
        ACPI_FADT_OFFSET (XGpe1Block),
        ACPI_FADT_OFFSET (Gpe1Block),
        ACPI_FADT_OFFSET (Gpe1BlockLength),
        0,
        ACPI_FADT_SEPARATE_LENGTH | ACPI_FADT_GPE_REGISTER}
};

#define ACPI_FADT_INFO_ENTRIES \
            (sizeof (FadtInfoTable) / sizeof (ACPI_FADT_INFO))


/* Table used to split Event Blocks into separate status/enable registers */

typedef struct acpi_fadt_pm_info
{
    ACPI_GENERIC_ADDRESS    *Target;
    UINT16                  Source;
    UINT8                   RegisterNum;

} ACPI_FADT_PM_INFO;

static ACPI_FADT_PM_INFO    FadtPmInfoTable[] =
{
    {&AcpiGbl_XPm1aStatus,
        ACPI_FADT_OFFSET (XPm1aEventBlock),
        0},

    {&AcpiGbl_XPm1aEnable,
        ACPI_FADT_OFFSET (XPm1aEventBlock),
        1},

    {&AcpiGbl_XPm1bStatus,
        ACPI_FADT_OFFSET (XPm1bEventBlock),
        0},

    {&AcpiGbl_XPm1bEnable,
        ACPI_FADT_OFFSET (XPm1bEventBlock),
        1}
};

#define ACPI_FADT_PM_INFO_ENTRIES \
            (sizeof (FadtPmInfoTable) / sizeof (ACPI_FADT_PM_INFO))


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInitGenericAddress
 *
 * PARAMETERS:  GenericAddress      - GAS struct to be initialized
 *              SpaceId             - ACPI Space ID for this register
 *              ByteWidth           - Width of this register
 *              Address             - Address of the register
 *              RegisterName        - ASCII name of the ACPI register
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a Generic Address Structure (GAS)
 *              See the ACPI specification for a full description and
 *              definition of this structure.
 *
 ******************************************************************************/

static void
AcpiTbInitGenericAddress (
    ACPI_GENERIC_ADDRESS    *GenericAddress,
    UINT8                   SpaceId,
    UINT8                   ByteWidth,
    UINT64                  Address,
    const char              *RegisterName,
    UINT8                   Flags)
{
    UINT8                   BitWidth;


    /*
     * Bit width field in the GAS is only one byte long, 255 max.
     * Check for BitWidth overflow in GAS.
     */
    BitWidth = (UINT8) (ByteWidth * 8);
    if (ByteWidth > 31)     /* (31*8)=248, (32*8)=256 */
    {
        /*
         * No error for GPE blocks, because we do not use the BitWidth
         * for GPEs, the legacy length (ByteWidth) is used instead to
         * allow for a large number of GPEs.
         */
        if (!(Flags & ACPI_FADT_GPE_REGISTER))
        {
            ACPI_ERROR ((AE_INFO,
                "%s - 32-bit FADT register is too long (%u bytes, %u bits) "
                "to convert to GAS struct - 255 bits max, truncating",
                RegisterName, ByteWidth, (ByteWidth * 8)));
        }

        BitWidth = 255;
    }

    /*
     * The 64-bit Address field is non-aligned in the byte packed
     * GAS struct.
     */
    ACPI_MOVE_64_TO_64 (&GenericAddress->Address, &Address);

    /* All other fields are byte-wide */

    GenericAddress->SpaceId = SpaceId;
    GenericAddress->BitWidth = BitWidth;
    GenericAddress->BitOffset = 0;
    GenericAddress->AccessWidth = 0; /* Access width ANY */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbSelectAddress
 *
 * PARAMETERS:  RegisterName        - ASCII name of the ACPI register
 *              Address32           - 32-bit address of the register
 *              Address64           - 64-bit address of the register
 *
 * RETURN:      The resolved 64-bit address
 *
 * DESCRIPTION: Select between 32-bit and 64-bit versions of addresses within
 *              the FADT. Used for the FACS and DSDT addresses.
 *
 * NOTES:
 *
 * Check for FACS and DSDT address mismatches. An address mismatch between
 * the 32-bit and 64-bit address fields (FIRMWARE_CTRL/X_FIRMWARE_CTRL and
 * DSDT/X_DSDT) could be a corrupted address field or it might indicate
 * the presence of two FACS or two DSDT tables.
 *
 * November 2013:
 * By default, as per the ACPICA specification, a valid 64-bit address is
 * used regardless of the value of the 32-bit address. However, this
 * behavior can be overridden via the AcpiGbl_Use32BitFadtAddresses flag.
 *
 ******************************************************************************/

static UINT64
AcpiTbSelectAddress (
    char                    *RegisterName,
    UINT32                  Address32,
    UINT64                  Address64)
{

    if (!Address64)
    {
        /* 64-bit address is zero, use 32-bit address */

        return ((UINT64) Address32);
    }

    if (Address32 &&
       (Address64 != (UINT64) Address32))
    {
        /* Address mismatch between 32-bit and 64-bit versions */

        ACPI_BIOS_WARNING ((AE_INFO,
            "32/64X %s address mismatch in FADT: "
            "0x%8.8X/0x%8.8X%8.8X, using %u-bit address",
            RegisterName, Address32, ACPI_FORMAT_UINT64 (Address64),
            AcpiGbl_Use32BitFadtAddresses ? 32 : 64));

        /* 32-bit address override */

        if (AcpiGbl_Use32BitFadtAddresses)
        {
            return ((UINT64) Address32);
        }
    }

    /* Default is to use the 64-bit address */

    return (Address64);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbParseFadt
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize the FADT, DSDT and FACS tables
 *              (FADT contains the addresses of the DSDT and FACS)
 *
 ******************************************************************************/

void
AcpiTbParseFadt (
    void)
{
    UINT32                  Length;
    ACPI_TABLE_HEADER       *Table;


    /*
     * The FADT has multiple versions with different lengths,
     * and it contains pointers to both the DSDT and FACS tables.
     *
     * Get a local copy of the FADT and convert it to a common format
     * Map entire FADT, assumed to be smaller than one page.
     */
    Length = AcpiGbl_RootTableList.Tables[AcpiGbl_FadtIndex].Length;

    Table = AcpiOsMapMemory (
        AcpiGbl_RootTableList.Tables[AcpiGbl_FadtIndex].Address, Length);
    if (!Table)
    {
        return;
    }

    /*
     * Validate the FADT checksum before we copy the table. Ignore
     * checksum error as we want to try to get the DSDT and FACS.
     */
    (void) AcpiTbVerifyChecksum (Table, Length);

    /* Create a local copy of the FADT in common ACPI 2.0+ format */

    AcpiTbCreateLocalFadt (Table, Length);

    /* All done with the real FADT, unmap it */

    AcpiOsUnmapMemory (Table, Length);

    /* Obtain the DSDT and FACS tables via their addresses within the FADT */

    AcpiTbInstallFixedTable ((ACPI_PHYSICAL_ADDRESS) AcpiGbl_FADT.XDsdt,
        ACPI_SIG_DSDT, &AcpiGbl_DsdtIndex);

    /* If Hardware Reduced flag is set, there is no FACS */

    if (!AcpiGbl_ReducedHardware)
    {
        if (AcpiGbl_FADT.Facs)
        {
            AcpiTbInstallFixedTable ((ACPI_PHYSICAL_ADDRESS) AcpiGbl_FADT.Facs,
                ACPI_SIG_FACS, &AcpiGbl_FacsIndex);
        }
        if (AcpiGbl_FADT.XFacs)
        {
            AcpiTbInstallFixedTable ((ACPI_PHYSICAL_ADDRESS) AcpiGbl_FADT.XFacs,
                ACPI_SIG_FACS, &AcpiGbl_XFacsIndex);
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbCreateLocalFadt
 *
 * PARAMETERS:  Table               - Pointer to BIOS FADT
 *              Length              - Length of the table
 *
 * RETURN:      None
 *
 * DESCRIPTION: Get a local copy of the FADT and convert it to a common format.
 *              Performs validation on some important FADT fields.
 *
 * NOTE:        We create a local copy of the FADT regardless of the version.
 *
 ******************************************************************************/

void
AcpiTbCreateLocalFadt (
    ACPI_TABLE_HEADER       *Table,
    UINT32                  Length)
{

    /*
     * Check if the FADT is larger than the largest table that we expect
     * (typically the current ACPI specification version). If so, truncate
     * the table, and issue a warning.
     */
    if (Length > sizeof (ACPI_TABLE_FADT))
    {
        ACPI_BIOS_WARNING ((AE_INFO,
            "FADT (revision %u) is longer than %s length, "
            "truncating length %u to %u",
            Table->Revision, ACPI_FADT_CONFORMANCE, Length,
            (UINT32) sizeof (ACPI_TABLE_FADT)));
    }

    /* Clear the entire local FADT */

    memset (&AcpiGbl_FADT, 0, sizeof (ACPI_TABLE_FADT));

    /* Copy the original FADT, up to sizeof (ACPI_TABLE_FADT) */

    memcpy (&AcpiGbl_FADT, Table,
        ACPI_MIN (Length, sizeof (ACPI_TABLE_FADT)));

    /* Take a copy of the Hardware Reduced flag */

    AcpiGbl_ReducedHardware = FALSE;
    if (AcpiGbl_FADT.Flags & ACPI_FADT_HW_REDUCED)
    {
        AcpiGbl_ReducedHardware = TRUE;
    }

    /* Convert the local copy of the FADT to the common internal format */

    AcpiTbConvertFadt ();

    /* Initialize the global ACPI register structures */

    AcpiTbSetupFadtRegisters ();
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbConvertFadt
 *
 * PARAMETERS:  None - AcpiGbl_FADT is used.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Converts all versions of the FADT to a common internal format.
 *              Expand 32-bit addresses to 64-bit as necessary. Also validate
 *              important fields within the FADT.
 *
 * NOTE:        AcpiGbl_FADT must be of size (ACPI_TABLE_FADT), and must
 *              contain a copy of the actual BIOS-provided FADT.
 *
 * Notes on 64-bit register addresses:
 *
 * After this FADT conversion, later ACPICA code will only use the 64-bit "X"
 * fields of the FADT for all ACPI register addresses.
 *
 * The 64-bit X fields are optional extensions to the original 32-bit FADT
 * V1.0 fields. Even if they are present in the FADT, they are optional and
 * are unused if the BIOS sets them to zero. Therefore, we must copy/expand
 * 32-bit V1.0 fields to the 64-bit X fields if the the 64-bit X field is
 * originally zero.
 *
 * For ACPI 1.0 FADTs (that contain no 64-bit addresses), all 32-bit address
 * fields are expanded to the corresponding 64-bit X fields in the internal
 * common FADT.
 *
 * For ACPI 2.0+ FADTs, all valid (non-zero) 32-bit address fields are expanded
 * to the corresponding 64-bit X fields, if the 64-bit field is originally
 * zero. Adhering to the ACPI specification, we completely ignore the 32-bit
 * field if the 64-bit field is valid, regardless of whether the host OS is
 * 32-bit or 64-bit.
 *
 * Possible additional checks:
 *  (AcpiGbl_FADT.Pm1EventLength >= 4)
 *  (AcpiGbl_FADT.Pm1ControlLength >= 2)
 *  (AcpiGbl_FADT.PmTimerLength >= 4)
 *  Gpe block lengths must be multiple of 2
 *
 ******************************************************************************/

static void
AcpiTbConvertFadt (
    void)
{
    const char              *Name;
    ACPI_GENERIC_ADDRESS    *Address64;
    UINT32                  Address32;
    UINT8                   Length;
    UINT8                   Flags;
    UINT32                  i;


    /*
     * For ACPI 1.0 FADTs (revision 1 or 2), ensure that reserved fields which
     * should be zero are indeed zero. This will workaround BIOSs that
     * inadvertently place values in these fields.
     *
     * The ACPI 1.0 reserved fields that will be zeroed are the bytes located
     * at offset 45, 55, 95, and the word located at offset 109, 110.
     *
     * Note: The FADT revision value is unreliable. Only the length can be
     * trusted.
     */
    if (AcpiGbl_FADT.Header.Length <= ACPI_FADT_V2_SIZE)
    {
        AcpiGbl_FADT.PreferredProfile = 0;
        AcpiGbl_FADT.PstateControl = 0;
        AcpiGbl_FADT.CstControl = 0;
        AcpiGbl_FADT.BootFlags = 0;
    }

    /*
     * Now we can update the local FADT length to the length of the
     * current FADT version as defined by the ACPI specification.
     * Thus, we will have a common FADT internally.
     */
    AcpiGbl_FADT.Header.Length = sizeof (ACPI_TABLE_FADT);

    /*
     * Expand the 32-bit DSDT addresses to 64-bit as necessary.
     * Later ACPICA code will always use the X 64-bit field.
     */
    AcpiGbl_FADT.XDsdt = AcpiTbSelectAddress ("DSDT",
        AcpiGbl_FADT.Dsdt, AcpiGbl_FADT.XDsdt);

    /* If Hardware Reduced flag is set, we are all done */

    if (AcpiGbl_ReducedHardware)
    {
        return;
    }

    /* Examine all of the 64-bit extended address fields (X fields) */

    for (i = 0; i < ACPI_FADT_INFO_ENTRIES; i++)
    {
        /*
         * Get the 32-bit and 64-bit addresses, as well as the register
         * length and register name.
         */
        Address32 = *ACPI_ADD_PTR (UINT32,
            &AcpiGbl_FADT, FadtInfoTable[i].Address32);

        Address64 = ACPI_ADD_PTR (ACPI_GENERIC_ADDRESS,
            &AcpiGbl_FADT, FadtInfoTable[i].Address64);

        Length = *ACPI_ADD_PTR (UINT8,
            &AcpiGbl_FADT, FadtInfoTable[i].Length);

        Name = FadtInfoTable[i].Name;
        Flags = FadtInfoTable[i].Flags;

        /*
         * Expand the ACPI 1.0 32-bit addresses to the ACPI 2.0 64-bit "X"
         * generic address structures as necessary. Later code will always use
         * the 64-bit address structures.
         *
         * November 2013:
         * Now always use the 64-bit address if it is valid (non-zero), in
         * accordance with the ACPI specification which states that a 64-bit
         * address supersedes the 32-bit version. This behavior can be
         * overridden by the AcpiGbl_Use32BitFadtAddresses flag.
         *
         * During 64-bit address construction and verification,
         * these cases are handled:
         *
         * Address32 zero, Address64 [don't care]   - Use Address64
         *
         * Address32 non-zero, Address64 zero       - Copy/use Address32
         * Address32 non-zero == Address64 non-zero - Use Address64
         * Address32 non-zero != Address64 non-zero - Warning, use Address64
         *
         * Override: if AcpiGbl_Use32BitFadtAddresses is TRUE, and:
         * Address32 non-zero != Address64 non-zero - Warning, copy/use Address32
         *
         * Note: SpaceId is always I/O for 32-bit legacy address fields
         */
        if (Address32)
        {
            if (!Address64->Address)
            {
                /* 64-bit address is zero, use 32-bit address */

                AcpiTbInitGenericAddress (Address64,
                    ACPI_ADR_SPACE_SYSTEM_IO,
                    *ACPI_ADD_PTR (UINT8, &AcpiGbl_FADT,
                        FadtInfoTable[i].Length),
                    (UINT64) Address32, Name, Flags);
            }
            else if (Address64->Address != (UINT64) Address32)
            {
                /* Address mismatch */

                ACPI_BIOS_WARNING ((AE_INFO,
                    "32/64X address mismatch in FADT/%s: "
                    "0x%8.8X/0x%8.8X%8.8X, using %u-bit address",
                    Name, Address32,
                    ACPI_FORMAT_UINT64 (Address64->Address),
                    AcpiGbl_Use32BitFadtAddresses ? 32 : 64));

                if (AcpiGbl_Use32BitFadtAddresses)
                {
                    /* 32-bit address override */

                    AcpiTbInitGenericAddress (Address64,
                        ACPI_ADR_SPACE_SYSTEM_IO,
                        *ACPI_ADD_PTR (UINT8, &AcpiGbl_FADT,
                            FadtInfoTable[i].Length),
                        (UINT64) Address32, Name, Flags);
                }
            }
        }

        /*
         * For each extended field, check for length mismatch between the
         * legacy length field and the corresponding 64-bit X length field.
         * Note: If the legacy length field is > 0xFF bits, ignore this
         * check. (GPE registers can be larger than the 64-bit GAS structure
         * can accomodate, 0xFF bits).
         */
        if (Address64->Address &&
           (ACPI_MUL_8 (Length) <= ACPI_UINT8_MAX) &&
           (Address64->BitWidth != ACPI_MUL_8 (Length)))
        {
            ACPI_BIOS_WARNING ((AE_INFO,
                "32/64X length mismatch in FADT/%s: %u/%u",
                Name, ACPI_MUL_8 (Length), Address64->BitWidth));
        }

        if (FadtInfoTable[i].Flags & ACPI_FADT_REQUIRED)
        {
            /*
             * Field is required (PM1aEvent, PM1aControl).
             * Both the address and length must be non-zero.
             */
            if (!Address64->Address || !Length)
            {
                ACPI_BIOS_ERROR ((AE_INFO,
                    "Required FADT field %s has zero address and/or length: "
                    "0x%8.8X%8.8X/0x%X",
                    Name, ACPI_FORMAT_UINT64 (Address64->Address), Length));
            }
        }
        else if (FadtInfoTable[i].Flags & ACPI_FADT_SEPARATE_LENGTH)
        {
            /*
             * Field is optional (PM2Control, GPE0, GPE1) AND has its own
             * length field. If present, both the address and length must
             * be valid.
             */
            if ((Address64->Address && !Length) ||
                (!Address64->Address && Length))
            {
                ACPI_BIOS_WARNING ((AE_INFO,
                    "Optional FADT field %s has valid %s but zero %s: "
                    "0x%8.8X%8.8X/0x%X", Name,
                    (Length ? "Length" : "Address"),
                    (Length ? "Address": "Length"),
                    ACPI_FORMAT_UINT64 (Address64->Address), Length));
            }
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbSetupFadtRegisters
 *
 * PARAMETERS:  None, uses AcpiGbl_FADT.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize global ACPI PM1 register definitions. Optionally,
 *              force FADT register definitions to their default lengths.
 *
 ******************************************************************************/

static void
AcpiTbSetupFadtRegisters (
    void)
{
    ACPI_GENERIC_ADDRESS    *Target64;
    ACPI_GENERIC_ADDRESS    *Source64;
    UINT8                   Pm1RegisterByteWidth;
    UINT32                  i;


    /*
     * Optionally check all register lengths against the default values and
     * update them if they are incorrect.
     */
    if (AcpiGbl_UseDefaultRegisterWidths)
    {
        for (i = 0; i < ACPI_FADT_INFO_ENTRIES; i++)
        {
            Target64 = ACPI_ADD_PTR (ACPI_GENERIC_ADDRESS, &AcpiGbl_FADT,
                FadtInfoTable[i].Address64);

            /*
             * If a valid register (Address != 0) and the (DefaultLength > 0)
             * (Not a GPE register), then check the width against the default.
             */
            if ((Target64->Address) &&
                (FadtInfoTable[i].DefaultLength > 0) &&
                (FadtInfoTable[i].DefaultLength != Target64->BitWidth))
            {
                ACPI_BIOS_WARNING ((AE_INFO,
                    "Invalid length for FADT/%s: %u, using default %u",
                    FadtInfoTable[i].Name, Target64->BitWidth,
                    FadtInfoTable[i].DefaultLength));

                /* Incorrect size, set width to the default */

                Target64->BitWidth = FadtInfoTable[i].DefaultLength;
            }
        }
    }

    /*
     * Get the length of the individual PM1 registers (enable and status).
     * Each register is defined to be (event block length / 2). Extra divide
     * by 8 converts bits to bytes.
     */
    Pm1RegisterByteWidth = (UINT8)
        ACPI_DIV_16 (AcpiGbl_FADT.XPm1aEventBlock.BitWidth);

    /*
     * Calculate separate GAS structs for the PM1x (A/B) Status and Enable
     * registers. These addresses do not appear (directly) in the FADT, so it
     * is useful to pre-calculate them from the PM1 Event Block definitions.
     *
     * The PM event blocks are split into two register blocks, first is the
     * PM Status Register block, followed immediately by the PM Enable
     * Register block. Each is of length (Pm1EventLength/2)
     *
     * Note: The PM1A event block is required by the ACPI specification.
     * However, the PM1B event block is optional and is rarely, if ever,
     * used.
     */

    for (i = 0; i < ACPI_FADT_PM_INFO_ENTRIES; i++)
    {
        Source64 = ACPI_ADD_PTR (ACPI_GENERIC_ADDRESS, &AcpiGbl_FADT,
            FadtPmInfoTable[i].Source);

        if (Source64->Address)
        {
            AcpiTbInitGenericAddress (FadtPmInfoTable[i].Target,
                Source64->SpaceId, Pm1RegisterByteWidth,
                Source64->Address +
                    (FadtPmInfoTable[i].RegisterNum * Pm1RegisterByteWidth),
                "PmRegisters", 0);
        }
    }
}
