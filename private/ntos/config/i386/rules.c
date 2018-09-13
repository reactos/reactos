/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    rules.c

Abstract:

    This modules contains routines to implement rules to describe a machine.
    This is based on the detection code from W9x.

Author:

    Santosh Jodh (santoshj) 08-Aug-1998

Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"
#include "stdlib.h"
#include "parseini.h"
#include "geninst.h"
#include "rules.h"
#include "acpitabl.h"
#include "ntacpi.h"

#define TABLE_ENTRIES_FROM_RSDT_POINTER(p)  (((p)->Header.Length-min((p)->Header.Length, sizeof(DESCRIPTION_HEADER))) / 4)


//
// Size of the ROM BIOS segment.
//

#define SYSTEM_BIOS_LENGTH 0x10000

//
// PnP BIOS structure signature.
//

#define PNPBIOS_SIGNATURE   'PnP$'

typedef
BOOLEAN
(* PFN_RULE)(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

typedef struct _PNP_BIOS_TABLE PNP_BIOS_TABLE, *PPNP_BIOS_TABLE;

#pragma pack(push, 1)

struct _PNP_BIOS_TABLE
{
    ULONG   Signature;
    UCHAR   Version;
    UCHAR   Length;
    USHORT  ControlField;
    UCHAR   CheckSum;
    ULONG   EventNotification;
    USHORT  RMOffset;
    USHORT  RMSegment;
    USHORT  PMOffset;
    ULONG   PMSegment;
    ULONG   Oem;
    USHORT  RMData;
    ULONG   PMData;
};

#pragma pack(pop)

ULONG
CmpComputeChecksum(
    IN PCHAR    Address,
    IN ULONG    Size
    );

NTSTATUS
CmpFindRSDTTable(
    OUT PACPI_BIOS_MULTI_NODE   *Rsdt
    );

NTSTATUS
CmpGetRegistryValue(
    IN  HANDLE                          KeyName,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION  *Information
    );

PDESCRIPTION_HEADER
CmpFindACPITable(
    IN ULONG                            Signature,
    IN OUT PULONG                       Length
    );

BOOLEAN
CmpCheckOperator(
    IN PCHAR Operator,
    IN ULONG Lhs,
    IN ULONG Rhs
    );

PVOID
CmpMapPhysicalAddress(
    IN OUT PVOID *BaseAddress,
    IN ULONG Address,
    IN ULONG Size
    );

BOOLEAN
CmpGetInfData(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG KeyIndex,
    IN ULONG LineIndex,
    IN OUT PCHAR Buffer,
    IN OUT PULONG BufferSize
    );

PVOID
CmpFindPattern(
    IN PCHAR Buffer,
    IN ULONG BufSize,
    IN PCHAR Pattern,
    IN ULONG PatSize,
    IN BOOLEAN IgnoreCase,
    IN ULONG Step
    );

 ULONG
 CmpGetPnPBIOSTableAddress(
    VOID
    );

BOOLEAN
CmpMatchDescription(
    IN PVOID InfHandle,
    IN PCHAR Description
    );

BOOLEAN
CmpMatchDateRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchMemoryRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchSearchRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchNextMatchRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchPointerRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchOemIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchPModeRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchRmPmSameRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchInstallRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchAcpiOemIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchAcpiOemTableIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchAcpiOemRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchAcpiRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

BOOLEAN
CmpMatchAcpiCreatorRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    );

//
// Number of rules currently implemented.
//

#define NUM_OF_RULES    14

//
// Rule table.
//

struct {
    PCHAR       Name;
    PFN_RULE    Action;
} gRuleTable[NUM_OF_RULES] =
{
    {"Date", CmpMatchDateRule},
    {"Memory", CmpMatchMemoryRule},
    {"Search", CmpMatchSearchRule},
    {"NextMatch", CmpMatchNextMatchRule},
    {"Pointer", CmpMatchPointerRule},
    {"OemId", CmpMatchOemIdRule},
    {"PMode", CmpMatchPModeRule},
    {"RmPmSame", CmpMatchRmPmSameRule},
    {"Install", CmpMatchInstallRule},
    {"ACPIOemId", CmpMatchAcpiOemIdRule},
    {"ACPIOemTableId", CmpMatchAcpiOemTableIdRule},
    {"ACPIOemRevision", CmpMatchAcpiOemRevisionRule},
    {"ACPIRevision", CmpMatchAcpiRevisionRule},
    {"ACPICreatorRevision", CmpMatchAcpiCreatorRevisionRule}
};

PVOID   gSearchAddress = NULL;

static WCHAR rgzMultiFunctionAdapter[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter";
static WCHAR rgzAcpiConfigurationData[] = L"Configuration Data";
static WCHAR rgzAcpiIdentifier[] = L"Identifier";
static WCHAR rgzBIOSIdentifier[] = L"ACPI BIOS";

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,CmpFindACPITable)
#pragma alloc_text(INIT,CmpFindRSDTTable)
#pragma alloc_text(INIT,CmpComputeChecksum)
#pragma alloc_text(INIT,CmpCheckOperator)
#pragma alloc_text(INIT,CmpMapPhysicalAddress)
#pragma alloc_text(INIT,CmpGetInfData)
#pragma alloc_text(INIT,CmpFindPattern)
#pragma alloc_text(INIT,CmpGetPnPBIOSTableAddress)
#pragma alloc_text(INIT,CmpMatchInfList)
#pragma alloc_text(INIT,CmpMatchDescription)
#pragma alloc_text(INIT,CmpMatchDateRule)
#pragma alloc_text(INIT,CmpMatchMemoryRule)
#pragma alloc_text(INIT,CmpMatchSearchRule)
#pragma alloc_text(INIT,CmpMatchNextMatchRule)
#pragma alloc_text(INIT,CmpMatchPointerRule)
#pragma alloc_text(INIT,CmpMatchOemIdRule)
#pragma alloc_text(INIT,CmpMatchPModeRule)
#pragma alloc_text(INIT,CmpMatchRmPmSameRule)
#pragma alloc_text(INIT,CmpMatchInstallRule)
#pragma alloc_text(INIT,CmpMatchAcpiOemIdRule)
#pragma alloc_text(INIT,CmpMatchAcpiOemTableIdRule)
#pragma alloc_text(INIT,CmpMatchAcpiOemRevisionRule)
#pragma alloc_text(INIT,CmpMatchAcpiRevisionRule)
#pragma alloc_text(INIT,CmpMatchAcpiCreatorRevisionRule)
#endif


BOOLEAN
CmpMatchInfList(
    IN PVOID InfImage,
    IN ULONG ImageSize,
    IN PCHAR Section
    )

/*++

    Routine Description:

    Input Parameters:

        InfImage - Pointer to the inf image in memory.

        ImageSize - Size of the inf image.

        Section - Section name containing the descriptions.

        Description -

    Return Value:

        TRUE if the machine matches any one of the descriptions in the inf.

--*/

{
    PCHAR   computerName;
    ULONG   i = 0;
    PVOID   infHandle;
    BOOLEAN result = FALSE;

    infHandle = CmpOpenInfFile(InfImage, ImageSize);

    if (infHandle)
    {
        //
        // Do any clean-up specified in the inf.
        //

        CmpGenInstall(infHandle, "Cleanup");

        //
        // Go through each description in this section and try to match
        // this machine to it.
        //

        while ((computerName = CmpGetSectionLineIndex(infHandle, Section, i++, 0)))
        {
            //
            // Reset search result from previous description.
            //

            gSearchAddress = NULL;

            //
            // If this description matches, we are done.
            // NOTE: IF MORE THAN ONE DESCRIPTION MATCHES,
            // WE STOP AT THE FIRST ONE.
            //

            if (CmpMatchDescription(infHandle, computerName))
            {
                DbgPrint("CmpMatchInfList: Machine matches %s description!\n", computerName);
                result = TRUE;
            }
        }

        CmpCloseInfFile(infHandle);
    }

    //
    // None of the descriptions match.
    //

    return (result);
}

BOOLEAN
CmpMatchDescription(
    IN PVOID InfHandle,
    IN PCHAR Description
    )

/*++

    Routine Description:

        This routine processes all the rules in the specified description.

    Input Parameters:

        InfHandle - Handle to the inf containing the description.

        Description - Section name containing the rules.

    Return Value:

        TRUE iff all the rules in the description succeed.

--*/

{
    ULONG   ruleNumber;
    ULONG   i;
    PCHAR   ruleName;

    //
    // Proceed only if the section does exist.
    //

    if (CmpSearchInfSection(InfHandle, Description))
    {
        //
        // Go through all the rules in the description and try to match
        // each of them.
        //

        ruleNumber = 0;
        while ((ruleName = CmpGetKeyName(InfHandle, Description, ruleNumber)))
        {
            //
            // Search for the rule in our table.
            //

            for (   i = 0;
                    i < NUM_OF_RULES &&
                        _stricmp(ruleName, gRuleTable[i].Name);
                    i++);

            //
            // If we did not find the rule or the rule failed,
            // return failure.
            //

            if (    i >= NUM_OF_RULES ||
                    !(*gRuleTable[i].Action)(InfHandle, Description, ruleNumber++))
            {
                return (FALSE);
            }
        }

        //
        // Description matches if we found at least one rule and all rules
        // succeeded.
        //

        if (ruleNumber)
        {
            return (TRUE);
        }
    }

    //
    // Description did not match.
    //

    return (FALSE);
}

BOOLEAN
CmpMatchDateRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )

/*++

    Routine Description:

        This routine checks if the machine satisfies the DATE rule. The BIOS date
        is stored in a standard location in the BIOS ROM at FFFF:5.

        Syntax -

        DATE=operator,month,day,year
            where operator [=, ==, !=, <>, <, <=, =<, >, >=, =>]

        Examples -

        date="<=",2,1,95
            is TRUE if the BIOS date on this machine is less than or equal to
            02/01/95.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Description - Name of the section containing the rule info.

        RuleIndex - Line number for the rule in the description section.

    Return Value:

        TRUE if the BIOS on this machine has the specified relation with the
        date specified in the rule.

--*/

{
    PCHAR   op;
    PCHAR   month;
    PCHAR   day;
    PCHAR   year;
    ULONG   infDate;
    ULONG   yr;
    ULONG   biosDate;
    CHAR    temp[3];
    PVOID   baseAddress;
    PCHAR   address;

    op = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
    month = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 1);
    day = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 2);
    year = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 3);

    if (op && month && day && year)
    {
        yr = strtoul(year, NULL, 16);
        infDate = ((yr < 0x80) ? 0x20000000 : 0x19000000) +
                    (yr << 16) +
                    (strtoul(month, NULL, 16) << 8) +
                    (strtoul(day, NULL, 16));

        address = CmpMapPhysicalAddress(&baseAddress, 0xFFFF5, 8);
        if (address)
        {
            temp[2] = '\0';

            RtlCopyBytes(temp, address + 6, 2);
            yr = strtoul(temp, NULL, 16);
            biosDate = ((yr < 0x80) ? 0x20000000 : 0x19000000) +
                        (yr << 16);

            RtlCopyBytes(temp, address, 2);
            biosDate |= (strtoul(temp, NULL, 16) << 8);

            RtlCopyBytes(temp, address + 3, 2);
            biosDate |= strtoul(temp, NULL, 16);

            ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);

            if (CmpCheckOperator(op, biosDate, infDate))
            {
                return (TRUE);
            }
        }
    }

    return (FALSE);
}

BOOLEAN
CmpMatchMemoryRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )

/*++

    Routine Description:

        This routine checks if the machine satisfies the MEMORY rule.

        Syntax -

        MEMORY=segment,offset,type,data
            where type ["S", "B"]

        Examples -

        memory=f000,e000,S,"TOSHIBA"
            is TRUE if the memory in this machine at physical address f000:e000
            has the string "TOSHIBA".

        memory=ffff,5,B,01,02
            is TRUE if the memory in this machine at physical memory ffff:5
            has the bytes 0x01 and 0x02.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Description - Name of the section containing the rule info.

        RuleIndex - Line number for the rule in the description section.

    Return Value:

        TRUE iff the MEMORY in this machine at the specified address
        contains the specified data.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               segment;
    PCHAR               offset;
    CHAR                data[MAX_DESCRIPTION_LEN + 1];
    ULONG               cbData;
    PVOID               baseAddress;
    PCHAR               address;
    ULONG               memory;

    //
    // Read in the segment and offset of the address specified.
    //

    segment = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
    offset = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 1);

    if (segment && offset)
    {
        //
        // Get the data specified in the inf.
        //

        cbData = sizeof(data);
        if (CmpGetInfData(InfHandle, Description, RuleIndex, 2, data, &cbData))
        {
            memory = (strtoul(segment, NULL, 16) << 4) + strtoul(offset, NULL, 16);

            //
            // Map in the physical address.
            //

            address = CmpMapPhysicalAddress(&baseAddress, memory, cbData);
            if (address)
            {

                //
                // Check if the inf data matches data in memory.
                //

                match = (RtlCompareMemory(address, data, cbData) == cbData);

                //
                // Unmap the physical address.
                //

                ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchSearchRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )

/*++

    Routine Description:

        This routine checks to see if the machine matches the SEARCH rule.

        Syntax -

        SEARCH=segment,offset,length,type,data
            where type ["S", "B"]

        Examples -

        search=f000,e000,7f,S,"SurePath"
            is TRUE if the string "SurePath" is somewhere in memory range
            F000:E000 to F000:E07F (inclusive).

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Description - Name of the section containing the rule info.

        RuleIndex - Line number for the rule in the description section.

    Return Value:

        TRUE iff the specified pattern is found within the specified address
        range.

--*/

{
    BOOLEAN match = FALSE;
    PCHAR   segment;
    PCHAR   offset;
    PCHAR   size;
    CHAR    data[MAX_DESCRIPTION_LEN + 1];
    ULONG   cbData;
    ULONG   memory;
    ULONG   length;
    PVOID   baseAddress;
    PCHAR   address;

    segment = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
    offset = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 1);
    size = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 2);

    if (segment && offset && size)
    {
        //
        // Get the data specified in the inf.
        //

        cbData = sizeof(data);
        if (CmpGetInfData(InfHandle, Description, RuleIndex, 3, data, &cbData))
        {
            memory = (strtoul(segment, NULL, 16) << 4) + strtoul(offset, NULL, 16);

            //
            // Map in the physical address.
            //

            length = strtoul(size, NULL, 16);
            address = CmpMapPhysicalAddress(&baseAddress, memory, length);
            if (address)
            {
                gSearchAddress = CmpFindPattern(address, length, data, cbData, FALSE, 0);
                if (gSearchAddress)
                {
                    //
                    // If we found the pattern, compute the actual address for it.
                    //

                    (PCHAR)gSearchAddress -= (ULONG)address;
                    (PCHAR)gSearchAddress += memory;
                    match = TRUE;
                }

                //
                // Unmap the physical address.
                //

                ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchNextMatchRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )

/*++

    Routine Description:

        This routine checks to see if the machine matches the NEXTMATCH rule.

        Syntax -

        NEXTMATCH=offset,type,data
            where type ["S", "B"]

        Examples -

        nextmatch=f0,S,"Atlanta"
            is TRUE if the string "Atlanta" is at offset 0xF0 from the previous
            successful SEARCH or NEXTMATCH rule.

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Description - Name of the section containing the rule info.

        RuleIndex - Line number for the rule in the description section.

    Return Value:

        TRUE iff the specified pattern is found at the specified offset
        from the previous successful SEARCH or NEXTMATCH.

--*/

{
    BOOLEAN match = FALSE;
    PCHAR   offset;
    CHAR    data[MAX_DESCRIPTION_LEN + 1];
    ULONG   cbData;
    PVOID   baseAddress;
    PCHAR   address;

    if (gSearchAddress)
    {
        offset = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
        if (offset)
        {
            //
            // Get the data specified in the inf.
            //

            cbData = sizeof(data);

            if (CmpGetInfData(InfHandle, Description, RuleIndex, 1, data, &cbData))
            {
                (PCHAR)gSearchAddress += strtoul(offset, NULL, 16);

                //
                // Map in the physical address.
                //

                address = CmpMapPhysicalAddress(&baseAddress, (ULONG)gSearchAddress, cbData);
                if (address)
                {

                    //
                    // Check if the inf data matches data in memory.
                    //

                    match = (RtlCompareMemory(address, data, cbData) == cbData);

                    //
                    // Unmap the physical address.
                    //

                    ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
                }
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchPointerRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
{
    BOOLEAN match = FALSE;
    PCHAR   segment1;
    PCHAR   offset1;
    PCHAR   segment2;
    PCHAR   offset2;
    PCHAR   index;
    PCHAR   op;
    CHAR    data[MAX_DESCRIPTION_LEN + 1];
    ULONG   cbData;
    ULONG   memory;
    ULONG   pointer;
    PVOID   baseAddress;
    PCHAR   address;

    segment1 = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
    offset1 = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 1);
    segment2 = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 2);
    offset2 = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 3);
    index = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 4);
    op = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 5);

    if (    segment1 && offset1 &&
            segment2 && offset2 &&
            index && op)
    {
        //
        // Get the data specified in the inf.
        //

        cbData = sizeof(data);

        if (CmpGetInfData(InfHandle, Description, RuleIndex, 6, data, &cbData))
        {
            if (strlen(offset2) == 0)
            {
                memory = strtoul(segment2, NULL, 16) << 4;
            }
            else
            {
                memory = (strtoul(segment2, NULL, 16) << 4) + strtoul(offset2, NULL, 16);
            }

            address = CmpMapPhysicalAddress(&baseAddress, memory, 4);
            if (address)
            {
                pointer = *((PUSHORT)address);

                //
                // Unmap the physical address.
                //

                ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);

                if (strlen(offset1) == 0)
                {
                    memory = (strtoul(segment1, NULL, 16) << 4) + pointer;
                }
                else
                {
                    memory = (strtoul(segment1, NULL, 16) << 4) + strtoul(offset1, NULL, 16);
                    address = CmpMapPhysicalAddress(&baseAddress, memory, 2);
                    if (address)
                    {
                        memory = ((*(PUSHORT)address) << 4) + pointer;

                        //
                        // Unmap the physical address.
                        //

                        ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
                    }
                }

                memory += strtoul(index, NULL, 16);

                //
                // Map in the physical address.
                //

                address = CmpMapPhysicalAddress(&baseAddress, memory, cbData);
                if (address)
                {
                    match = CmpCheckOperator(op, RtlCompareMemory(address, data, cbData), cbData);

                    //
                    // Unmap the physical address.
                    //

                    ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
                }
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchOemIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
{
    BOOLEAN         match = FALSE;
    ULONG           address;
    PCHAR           op;
    PCHAR           oemIdStr;
    ULONG           oemId;
    PCHAR           baseAddress;
    PPNP_BIOS_TABLE biosTable;

    //
    // Search for the PnPBIOS structure in the BIOS ROM.
    //

    address = CmpGetPnPBIOSTableAddress();

    //
    // Proceed if we found the PnP BIOS structure.
    //

    if (address)
    {
        op = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
        oemIdStr = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 1);
        if (op && oemIdStr)
        {

            if (    strlen(oemIdStr) == 7 &&
                    isalpha(oemIdStr[0]) &&
                    isalpha(oemIdStr[1]) &&
                    isalpha(oemIdStr[2]) &&
                    isxdigit(oemIdStr[3]) &&
                    isxdigit(oemIdStr[4]) &&
                    isxdigit(oemIdStr[5]) &&
                    isxdigit(oemIdStr[6]))
            {

                biosTable = (PPNP_BIOS_TABLE)CmpMapPhysicalAddress(&baseAddress, address, sizeof(PNP_BIOS_TABLE));
                if (biosTable)
                {
                    oemId = ((ULONG)(oemIdStr[0] & 0x1F) << 26) +
                            ((ULONG)(oemIdStr[1] & 0x1F) << 21) +
                            ((ULONG)(oemIdStr[2] & 0x1F) << 16) +
                            strtoul(&oemIdStr[3], NULL, 16);

                    //
                    // We only support EQUAL and NOT EQUAL operators.
                    //

                    if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0)
                    {
                        match = (oemId == biosTable->Oem);
                    }
                    else if(    strcmp(op, "<>") == 0 ||
                                strcmp(op, "!=") == 0 ||
                                strcmp(op, "=!") == 0)
                    {
                        match = (oemId != biosTable->Oem);
                    }

                    //
                    // Unmap the physical address.
                    //

                    ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
                }
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchPModeRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
{
    BOOLEAN         match = FALSE;
    ULONG           address;
    CHAR            data[MAX_DESCRIPTION_LEN + 1];
    ULONG           cbData;
    PVOID           baseAddress;
    PPNP_BIOS_TABLE biosTable;
    ULONG           pmAddress;
    PCHAR           pmodeEntry;

    //
    // Search for the PnPBIOS structure in the BIOS ROM.
    //

    address = CmpGetPnPBIOSTableAddress();

    //
    // Proceed if we found the PnP BIOS structure.
    //

    if (address)
    {
        //
        // Get the data specified in the inf.
        //

        cbData = sizeof(data);
        if (CmpGetInfData(InfHandle, Description, RuleIndex, 0, data, &cbData))
        {
            biosTable = (PPNP_BIOS_TABLE)CmpMapPhysicalAddress(&baseAddress, address, sizeof(PNP_BIOS_TABLE));
            if (biosTable)
            {
                pmAddress = (biosTable->PMSegment << 4) + biosTable->PMOffset;

                //
                // Unmap the physical address.
                //

                ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);

                pmodeEntry = CmpMapPhysicalAddress(&baseAddress, pmAddress, SYSTEM_BIOS_LENGTH);
                if (pmodeEntry)
                {
                    if (*pmodeEntry == 0xE9)
                    {
                        pmodeEntry += (3 + (*((PUSHORT)&pmodeEntry[1])));
                    }

                    match = (RtlCompareMemory(pmodeEntry, data, cbData) == cbData);

                    //
                    // Unmap the physical address.
                    //

                    ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
                }
            }
        }
    }

    return (match);
}

BOOLEAN
CmpMatchRmPmSameRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
{
    BOOLEAN match = FALSE;
    ULONG           address;
    PCHAR           baseAddress;
    PPNP_BIOS_TABLE biosTable;

    //
    // Search for the PnPBIOS structure in the BIOS ROM.
    //

    address = CmpGetPnPBIOSTableAddress();

    //
    // Proceed if we found the PnP BIOS structure.
    //

    if (address)
    {
        biosTable = CmpMapPhysicalAddress(&baseAddress, address, sizeof(PNP_BIOS_TABLE));
        if (biosTable)
        {
            match = (   biosTable->RMSegment == biosTable->PMSegment &&
                        biosTable->RMOffset == biosTable->PMOffset);

            //
            // Unmap the physical address.
            //

            ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
        }
    }

    return (match);
}

BOOLEAN
CmpMatchInstallRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
{
    BOOLEAN match = FALSE;
    PCHAR   install;

    install = CmpGetSectionLineIndex(InfHandle, Description, RuleIndex, 0);
    if (install)
    {
        if (CmpGenInstall(InfHandle, install))
        {
            //
            // Successfully installed the specified section.
            //

            match = TRUE;
        }
    }

    return (match);
}

BOOLEAN
CmpMatchAcpiOemIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI OEM ID rule from an INF file

    Examples:

        AcpiOemId="RSDT", "123456"

    is true if the RSDT has the OEM ID of 123456.

        AcpiOemId="DSDT", "768000"

    is true if the DSDT has the OEM ID of 768000.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    BOOLEAN             anyCase = FALSE;
    BOOLEAN             match = FALSE;
    PCHAR               tableName;
    PCHAR               oemId;
    PCHAR               optionalArgs;
    ULONG               length;
    PDESCRIPTION_HEADER header;
    CHAR                tableOemId[7];
    STRING              acpiString;
    STRING              tableString;

    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    oemId = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    if (tableName && oemId) {

        //
        // See if we have to do a case insensitive match
        //
        optionalArgs = CmpGetSectionLineIndex(
            InfHandle,
            Description,
            RuleIndex,
            2
            );
        if (optionalArgs) {

            if (_stricmp(optionalArgs,"any") == 0) {

                anyCase = TRUE;

            }

        }

        //
        // Find the specified table in the BIOS ROM.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header) {

            //
            // Build the OEM id from the table
            //
            RtlZeroMemory(tableOemId, sizeof(tableOemId));
            RtlCopyMemory(tableOemId, header->OEMID, sizeof(header->OEMID));
            RtlInitString( &tableString, tableOemId );

            //
            // And one from the string in the file
            //
            RtlInitString( &acpiString, oemId );

            //
            // Now see if they are equal
            //
            match = RtlEqualString( &acpiString, &tableString, anyCase );

            //
            // Unmap the table
            //
            MmUnmapIoSpace(header, length );

        }

    }
    return (match);
}

BOOLEAN
CmpMatchAcpiOemTableIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI OEM Table ID rule from an INF file.

    Examples:

    AcpiOemTableId="RSDT", "12345678"

        is true if the RSDT has the Oem Table ID of 12345678.

    AcpiOemTableId="DSDT", "87654321"

        is true if the DSDT has the Oem Table ID of 87654321.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM Table ID.

    FALSE - the computer does not have the specified ACPI OEM Table ID.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               tableName;
    PCHAR               oemTableId;
    ULONG               length;
    PDESCRIPTION_HEADER header;
    ULONG               idLength;
    CHAR                acpiOemTableId[8];

    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    oemTableId = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    if (tableName && oemTableId) {

        //
        // Find the specified table in the BIOS ROM.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header) {

            RtlZeroMemory(acpiOemTableId, sizeof(acpiOemTableId));
            idLength = strlen(oemTableId);
            if (idLength > sizeof(acpiOemTableId)) {

                idLength = sizeof(acpiOemTableId);

            }
            RtlCopyMemory(acpiOemTableId, oemTableId, idLength);
            match = RtlEqualMemory(acpiOemTableId, header->OEMTableID, sizeof(header->OEMTableID));
            MmUnmapIoSpace( header, length );

        }

    }
    return (match);
}

BOOLEAN
CmpMatchAcpiOemRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI Oem Revision rule from an INF file.

    Examples:

    AcpiOemRevision="=","RSDT", 1234

        is true if the RSDT has the Oem Revision EQUAL to 1234.

    AcpiOemRevision=">","DSDT", 4321

        is true if the DSDT has the Oem Revision GREATER than 4321.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI Oem Revision.

    FALSE - the computer does not have the specified ACPI Oem Revision.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               op;
    PCHAR               tableName;
    PCHAR               oemRevisionStr;
    ULONG               oemRevision;
    ULONG               length;
    PDESCRIPTION_HEADER header;

    op = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    oemRevisionStr = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        2
        );
    if (op && tableName && oemRevisionStr) {

        //
        // Find the specified table.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header) {

            RtlCharToInteger(oemRevisionStr, 16, &oemRevision);
            match = CmpCheckOperator(op, header->OEMRevision, oemRevision);
            MmUnmapIoSpace(header, length);

        }

    }
    return(match);

}

BOOLEAN
CmpMatchAcpiRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI Revision rule from an INF file.

    Examples:

        AcpiRevision="=", "RSDT", 1234

    is true if the RSDT ACPI Revision is EQUAL to 1234.

        AcpiRevision=">", "DSDT", 4321

    is true if the DSDT ACPI Revision is GREATER than 4321.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI Revision.

    FALSE - the computer does not have the specified ACPI Revision.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               op;
    PCHAR               tableName;
    PCHAR               revisionStr;
    ULONG               revision;
    ULONG               length;
    PDESCRIPTION_HEADER header;

    op = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    revisionStr = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        2
        );
    if (op && tableName && revisionStr){

        //
        // Find the specified table.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header) {

            RtlCharToInteger(revisionStr, 16, &revision);
            match = CmpCheckOperator(op, header->Revision, revision);
            MmUnmapIoSpace(header, length);

        }

    }
    return(match);

}

BOOLEAN
CmpMatchAcpiCreatorRevisionRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI Creator Revision rule from an INF file.

    Examples:

        AcpiCreatorRevision="=", "RSDT", 1234

    is true if the RSDT ACPI Creator Revision is EQUAL to 1234.

        AcpiCreatorRevision=">", "DSDT", 4321

    is true if the DSDT ACPI Creator Revision is GREATER than 4321.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI Creator Revision.

    FALSE - the computer does not have the specified ACPI Creator Revision.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               op;
    PCHAR               tableName;
    PCHAR               creatorRevisionStr;
    ULONG               creatorRevision;
    ULONG               length;
    PDESCRIPTION_HEADER header;

    op = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    creatorRevisionStr = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        2
        );
    if (op && tableName && creatorRevisionStr) {

        //
        // Find the specified table.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header){

            RtlCharToInteger(creatorRevisionStr, 16, &creatorRevision);
            match = CmpCheckOperator(op, header->CreatorRev, creatorRevision);
            MmUnmapIoSpace( header, length );

        }

    }
    return(match);
}

BOOLEAN
CmpMatchAcpiCreatorIdRule(
    IN PVOID InfHandle,
    IN PCHAR Description,
    IN ULONG RuleIndex
    )
/*++

Routine Description:

    This function processes a ACPI Creator ID rule from an INF file.

    Examples:

        AcpiCreatorId="RSDT", "MSFT"

    is true if the RSDT has the Creator ID of MSFT.

Arguments:

    InfHandle - Handle of the inf containing the rule.

    Description - Specifies the section name the rule is in

    RuleIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI Creator ID.

    FALSE - the computer does not have the specified ACPI Creator ID.

--*/

{
    BOOLEAN             match = FALSE;
    PCHAR               tableName;
    PCHAR               creatorId;
    ULONG               length;
    PDESCRIPTION_HEADER header;
    ULONG               idLength;
    CHAR                acpiCreatorId[6];

    tableName = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        0
        );
    creatorId = CmpGetSectionLineIndex(
        InfHandle,
        Description,
        RuleIndex,
        1
        );
    if (tableName && creatorId) {

        //
        // Find the specified table.
        //
        header = CmpFindACPITable(*(PULONG)tableName, &length);
        if (header) {

            RtlZeroMemory(acpiCreatorId, sizeof(acpiCreatorId));
            idLength = strlen(creatorId);
            if (idLength > sizeof(acpiCreatorId)) {

                idLength = sizeof(acpiCreatorId);

            }
            RtlCopyMemory(acpiCreatorId, creatorId, idLength);
            match = RtlEqualMemory(acpiCreatorId, header->CreatorID, sizeof(header->CreatorID));
            MmUnmapIoSpace( header, length );

        }

    }
    return(match);
}

BOOLEAN
CmpGetInfData(
    IN PVOID InfHandle,
    IN PCHAR Section,
    IN ULONG LineIndex,
    IN ULONG ValueIndex,
    IN OUT PCHAR Buffer,
    IN OUT PULONG BufferSize
    )

/*++

    Routine Description:

        This routine reads and parses data from the inf. It understands
        two kinds of data 1. String 2. Binary.

        Examples-

        B,02 - byte 0x02
        B,72,0D,FF,0F - sequence of bytes 0x72 0x0D 0xFF 0x0F or the DWORD 0x0FFF0D72
        S,COMPAQ - ASCII string "COMPAQ"

    Input Parameters:

        InfHandle - Handle to the inf to be read.

        Section - Section name to be read.

        LineIndex - Index of the line in the Section to be read.

        ValueIndex - First value to be read on the LineIndex.

        Buffer - Parsed data gets returned in this buffer.

        BufferSize - On entry, contains the size of Buffer.
        The number of bytes parsed in gets returned in this
        variable.

    Return Value:

        TRUE iff data was parsed in successfully. Else FALSE.

--*/

{
    BOOLEAN result = FALSE;
    ULONG   cbData;
    PCHAR   data;
    ULONG   remainingBytes;

    //
    // Validate input parameters.
    //

    if (Buffer && BufferSize && *BufferSize)
    {
        //
        // Read in the data type "S" or "B".
        //

        PCHAR type = CmpGetSectionLineIndex(InfHandle, Section, LineIndex, ValueIndex++);
        if (type)
        {
            //
            // Initialize local data.
            //

            remainingBytes = *BufferSize;

            //
            // Process Binary data.
            //

            if (_stricmp(type, "B") == 0)
            {

                //
                // Parse data as long as there is more data and the buffer is not full.
                //

                for (result = TRUE; result == TRUE && remainingBytes; remainingBytes--)
                {
                    CHAR    value;

                    //
                    // Read in the data.
                    //

                    data = CmpGetSectionLineIndex(InfHandle, Section, LineIndex, ValueIndex++);
                    if (data)
                    {
                        //
                        // Convert the data read in and validate that is indeed a HEX value.
                        //

                        value = (CHAR)strtoul(data, NULL, 16);
                        if (value == 0 && strcmp(data, "00") && strcmp(data, "0"))
                        {
                            result = FALSE;
                        }
                        else
                        {
                            *Buffer++ = value;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                //
                // Return the number of bytes parsed in.
                //

                *BufferSize -= remainingBytes;
            }

            //
            // Process String data.
            //

            else if(_stricmp(type, "S") == 0)
            {
                //
                // Read in the string.
                //

                data = CmpGetSectionLineIndex(InfHandle, Section, LineIndex, ValueIndex);

                //
                // Only copy as much data as the buffer can hold.
                //

                cbData = min(remainingBytes, strlen(data));
                RtlCopyBytes(Buffer, data, cbData);

                //
                // Return the number of bytes actually copied.
                //

                *BufferSize = cbData;
                result = TRUE;
            }
        }
    }

    return (result);
}

PVOID
CmpMapPhysicalAddress(
    IN OUT PVOID *BaseAddress,
    IN ULONG Address,
    IN ULONG Size
    )

/*++

    Routine Description:

        This routine maps the specified physical segment into the process
        virtual memory.

    Input Parameters:

        Segment - Segment to be mapped.

        Size - Segment size to be mapped.

    Return Value:

        Virtual address for the mapped segment.

--*/

{
    UNICODE_STRING      sectionName;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              sectionHandle;
    NTSTATUS            status;
    PVOID               baseAddress;
    ULONG               viewSize;
    LARGE_INTEGER       viewBase;
    PVOID               ptr = NULL;

    *BaseAddress = NULL;

    RtlInitUnicodeString(&sectionName, L"\\Device\\PhysicalMemory");
    InitializeObjectAttributes( &objectAttributes,
                                &sectionName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE)NULL,
                                (PSECURITY_DESCRIPTOR)NULL);
    status = ZwOpenSection( &sectionHandle,
                            SECTION_MAP_READ,
                            &objectAttributes);
    if (NT_SUCCESS(status))
    {
        baseAddress = NULL;
        viewSize = Size;
        viewBase.LowPart = Address & ~(0xFFF);
        viewBase.HighPart = 0;
        status = ZwMapViewOfSection(    sectionHandle,
                                        NtCurrentProcess(),
                                        &baseAddress,
                                        0,
                                        viewSize,
                                        &viewBase,
                                        &viewSize,
                                        ViewUnmap,
                                        MEM_DOS_LIM,
                                        PAGE_READWRITE);
        if (NT_SUCCESS(status))
        {
            ptr = (PVOID)((PCHAR)baseAddress + (Address & 0xFFF));
            *BaseAddress = baseAddress;
        }
    }

    return (ptr);
}

    BOOLEAN
CmpCheckOperator(
    IN PCHAR Operator,
    IN ULONG Lhs,
    IN ULONG Rhs
    )

/*++

    Routine Description:

        This routine tests condition specified by the operator by
        applying it to the specified LHS and RHS arguments.

    Input Parameters:

        Operator - Is the operator to be tested.

        Lhs - Left Hand Side argument for the Operator.

        Rhs - Right Hand Side argument for the Operator.

    Return Value:

        True iff the condition Lhs Operator Rhs is satisfied.

--*/

{
    BOOLEAN result = FALSE;

    //
    // We are pretty lenient about which operators we support.
    //

    //
    // "=" or "==" for EQUAL.
    //

    if (strcmp(Operator, "=") == 0 || strcmp(Operator, "==") == 0)
    {
        result = (Lhs == Rhs);
    }

    //
    // "!=" or "=!" or "<>" for NOT EQUAL.
    //

    else if(    strcmp(Operator, "!=") == 0 ||
                strcmp(Operator, "<>") == 0 ||
                strcmp(Operator, "=!") == 0)
    {
        result = (Lhs != Rhs);
    }

    //
    // "<" for LESS THAN.
    //

    else if(strcmp(Operator, "<") == 0)
    {
        result = (Lhs < Rhs);
    }

    //
    // "<=" or "=<" for LESS THAN or EQUAL.
    //

    else if(strcmp(Operator, "<=") == 0 || strcmp(Operator, "=<") == 0)
    {
        result = (Lhs <= Rhs);
    }

    //
    // ">" for GREATER THAN.
    //

    else if(strcmp(Operator, ">") == 0)
    {
        result = (Lhs > Rhs);
    }

    //
    // ">=" or "=>" for GREATER THAN or EQUAL.
    //

    else if(strcmp(Operator, ">=") == 0 || strcmp(Operator, "=>") == 0)
    {
        result = (Lhs >= Rhs);
    }
    else
    {
        DbgPrint("Invalid operator %s used!\n", Operator);
    }

    return (result);
}

PVOID
CmpFindPattern(
    IN PCHAR Buffer,
    IN ULONG BufSize,
    IN PCHAR Pattern,
    IN ULONG PatSize,
    IN BOOLEAN IgnoreCase,
    IN ULONG Step
    )

/*++

    Routine Description:

        This routine searches the buffer for the specified pattern of data.

    Input Parameters:

        Buffer - Buffer to be searched.

        BufSize - Size of this buffer.

        Pattern - Pattern to be searched.

        PatSize - Size of the pattern.

        IgnoreCase - TRUE if the search is to be case insensitive.

    Return Value:

        Returns the pointer into the buffer where the pattern is first found.

--*/

{
    PCHAR   bufEnd;

    if (PatSize > BufSize)
    {
        return (NULL);
    }

    if (PatSize == 0)
    {
        PatSize = strlen(Pattern);
    }

    if (Step == 0)
    {
        Step = 1;
    }

    for (   bufEnd = Buffer + BufSize;
            Buffer + PatSize < bufEnd;
            Buffer += Step)
    {
        if (IgnoreCase)
        {
            if (_strnicmp(Buffer, Pattern, PatSize) == 0)
            {
                return (Buffer);
            }
        }
        else
        {
            if (strncmp(Buffer, Pattern, PatSize) == 0)
            {
                return (Buffer);
            }
        }
    }

    return (NULL);
 }

 ULONG
 CmpGetPnPBIOSTableAddress(
    VOID
    )

/*++

    Routine Description:

        This routine searches the BIOS ROM for the PnP BIOS installation
        structure.

    Input Parameters:

        None.

    Return Value:

        Returns the physical address in the ROM BIOS where the PnP
        BIOS structure is located.

--*/

{
    static ULONG    tableAddress = (ULONG)-1;
    PVOID           baseAddress;
    PPNP_BIOS_TABLE address;
    PPNP_BIOS_TABLE lastAddress;
    ULONG           i;
    ULONG           checksum;

    if (tableAddress == (ULONG)-1)
    {
        //
        // Search for the PnPBIOS structure in the BIOS ROM.
        //

        address = (PPNP_BIOS_TABLE)CmpMapPhysicalAddress(&baseAddress, 0xF0000, SYSTEM_BIOS_LENGTH);
        if (address)
        {
            for (   lastAddress = (PPNP_BIOS_TABLE)((PCHAR)address + SYSTEM_BIOS_LENGTH - 0x10);
                    address < lastAddress;
                    (PCHAR)address += 0x10)
            {
                if (address->Signature == PNPBIOS_SIGNATURE)
                {
                    for (   i = 0, checksum = 0;
                            i < address->Length;
                            i++)
                    {
                        checksum += ((PUCHAR)address)[i];
                    }

                    if (    (checksum & 0xFF) == 0 &&
                            address->Length >= 0x21)
                    {
                        tableAddress = 0xF0000 + (SYSTEM_BIOS_LENGTH - 10) - ((PCHAR)lastAddress - (PCHAR)address);
                        break;
                    }
                }
            }

            //
            // Unmap the physical address.
            //

            ZwUnmapViewOfSection(NtCurrentProcess(), baseAddress);
        }
    }

    return (tableAddress);
}

PDESCRIPTION_HEADER
CmpFindACPITable(
    IN ULONG        Signature,
    IN OUT PULONG   Length
    )
{
    PDESCRIPTION_HEADER     header      = NULL;
    PDESCRIPTION_HEADER     tempHeader  = NULL;
    static PHYSICAL_ADDRESS rsdtAddress = { -1, -1 };
    ULONG                   length      = 0;

    //
    // Use the cached location of RSDT address if available.
    //
    if (rsdtAddress.QuadPart == -1) {

        NTSTATUS                status;
        PACPI_BIOS_MULTI_NODE   rsdpMulti;

        rsdtAddress.QuadPart = 0;
        //
        // Get the multinode
        //
        status = CmpFindRSDTTable( &rsdpMulti );
        if (!NT_SUCCESS(status)) {

            return NULL;

        }

        //
        // Map the address
        //
        rsdtAddress.LowPart = rsdpMulti->RsdtAddress.LowPart;
        rsdtAddress.HighPart = rsdpMulti->RsdtAddress.HighPart;

        //
        // Done with the multinode
        //
        ExFreePool( rsdpMulti );

    }

    //
    // If we have an address
    //
    if (rsdtAddress.QuadPart) {

        //
        // Map in the the rsdt table
        //
        tempHeader = MmMapIoSpace(
            rsdtAddress,
            sizeof(DESCRIPTION_HEADER),
            MmCached
            );
        if (tempHeader == NULL) {

            DbgPrint("CmpFindACPITable: Cannot map RSDT at %I64x\n", rsdtAddress.QuadPart);
            return NULL;

        }

        //
        // If what we are looking for is the RSDT, then we are done
        //
        if (Signature == RSDT_SIGNATURE) {

            header = tempHeader;
            length = sizeof(DESCRIPTION_HEADER);

        } else if (Signature == DSDT_SIGNATURE) {

            PFADT               fadt;
            PHYSICAL_ADDRESS    dsdtAddress;
            ULONG               tempLength;

            fadt = (PFADT) CmpFindACPITable( FADT_SIGNATURE, &length );
            if (fadt) {

                dsdtAddress.HighPart = 0;
                dsdtAddress.LowPart = fadt->dsdt;

                //
                // Done with the FADT
                //
                MmUnmapIoSpace( fadt, length );

                //
                // Map in the dsdt table
                //
                header = MmMapIoSpace(
                    dsdtAddress,
                    sizeof(DESCRIPTION_HEADER),
                    MmCached
                    );
                if (header == NULL) {

                    DbgPrint(
                        "CmpFindACPITable: Cannot map DSDT at %I64x\n",
                        dsdtAddress.QuadPart
                        );
                    MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );
                    return NULL;

                }
                length = sizeof(DESCRIPTION_HEADER);

            } else {

                DbgPrint("CmpFindACPITable: Cannot find FADT\n");
                MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );
                return NULL;

            }

        } else {

            PHYSICAL_ADDRESS    tableAddress;
            PRSDT               rsdt;
            ULONG               i;
            ULONG               num;
            ULONG               rsdtLength;

            //
            // Map in the entire RSDT
            //
            rsdtLength = tempHeader->Length;
            rsdt = (PRSDT) MmMapIoSpace( rsdtAddress, rsdtLength, MmCached );
            if (rsdt == NULL) {

                DbgPrint(
                    "CmpFindACPITable: Cannot map RSDT at %I64x\n",
                    rsdtAddress.QuadPart
                    );
                MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );
                return NULL;

            }

            //
            // Done with the temp header
            //
            MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );

            //
            // Look at all the table entries for the header that we care about
            //
            num = TABLE_ENTRIES_FROM_RSDT_POINTER( rsdt );
            for (i = 0; i < num ; i ++) {

                //
                // Get the address of the table
                //
                tableAddress.HighPart = 0;
                tableAddress.LowPart = rsdt->Tables[i];

                //
                // Map in the header
                //
                tempHeader = MmMapIoSpace(
                    tableAddress,
                    sizeof(DESCRIPTION_HEADER),
                    MmCached
                    );
                if (!tempHeader) {

                    DbgPrint(
                        "CmpFindACPITable: Cannot map header at %I64x\n",
                        tableAddress.QuadPart
                        );
                    MmUnmapIoSpace( rsdt, rsdtLength );
                    return NULL;

                }

                //
                // Signature check
                //
                if (tempHeader->Signature != Signature) {

                    MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );
                    continue;

                }

                //
                // Are we looking at the FADT?
                //
                if (Signature == FADT_SIGNATURE) {

                    //
                    // Map the entire table for this one
                    //
                    length = tempHeader->Length;
                    header = MmMapIoSpace( tableAddress, length, MmCached );

                    //
                    // Unmap the old table
                    //
                    MmUnmapIoSpace( tempHeader, sizeof(DESCRIPTION_HEADER) );

                    //
                    // Did we successfully map the header?
                    //
                    if (header == NULL ) {

                        DbgPrint(
                            "CmpFindACPITable: Cannot map FADT at %I64x\n",
                            tableAddress.QuadPart
                            );
                        MmUnmapIoSpace( rsdt, rsdtLength );
                        return NULL;

                    }

                } else {

                    //
                    // Remember where the table and length are stored
                    //
                    length = sizeof(DESCRIPTION_HEADER);
                    header = tempHeader;

                }

            } // for

            //
            // Done with the rsdt
            //
            MmUnmapIoSpace( rsdt, rsdtLength );

        }

        //
        // If we found the table, return its length.
        //
        if (Length) {

            if (header) {

                *Length = length;

            } else {

                *Length = 0;

            }

        }

    }

    return (header);
}

NTSTATUS
CmpFindRSDTTable(
    OUT PACPI_BIOS_MULTI_NODE   *Rsdt
    )
/*++

Routine Description:

    This function looks into the registry to find the ACPI RSDT,
    which was stored there by ntdetect.com

Arguments:

    RsdtPtr - Pointer to a buffer that contains the ACPI
              Root System Description Pointer Structure.
              The caller is responsible for freeing this
              buffer.  Note:  This is returned in non-paged
              pool.

Return Value:

    A NTSTATUS code to indicate the result of the initialization.

--*/
{
    BOOLEAN                         same;
    HANDLE                          hMFunc;
    HANDLE                          hBus;
    NTSTATUS                        status;
    OBJECT_ATTRIBUTES               objectAttributes;
    PACPI_BIOS_MULTI_NODE           multiNode;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PCM_PARTIAL_RESOURCE_LIST       prl;
    PKEY_VALUE_PARTIAL_INFORMATION  valueInfo;
    PWSTR                           p;
    ULONG                           i;
    ULONG                           length;
    ULONG                           multiNodeSize;
    UNICODE_STRING                  unicodeString;
    UNICODE_STRING                  unicodeValueName;
    UNICODE_STRING                  biosId;
    WCHAR                           wbuffer[10];

    PAGED_CODE();

    //
    // Look in the registry for the "ACPI BIOS bus" data
    //
    RtlInitUnicodeString( &unicodeString, rgzMultiFunctionAdapter );
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,       // handle
        NULL
        );
    status = ZwOpenKey( &hMFunc, KEY_READ, &objectAttributes );
    if (!NT_SUCCESS(status)) {

        DbgPrint("CmpFindRSDTTable: Cannot open MultifunctionAdapter registry key.\n");
        return status;

    }

    //
    // We will need to make a unicode string that we can use to enumerate
    // the subkeys of the MFA key
    //
    unicodeString.Buffer = wbuffer;
    unicodeString.MaximumLength = sizeof(wbuffer);
    RtlInitUnicodeString( &biosId, rgzBIOSIdentifier );

    //
    // Loop over all subkeys
    //
    for (i = 0; TRUE; i++) {

        //
        // Turn the number into a key name
        //
        RtlIntegerToUnicodeString( i, 10, &unicodeString);
        InitializeObjectAttributes(
            &objectAttributes,
            &unicodeString,
            OBJ_CASE_INSENSITIVE,
            hMFunc,
            NULL
            );

        //
        // Open the named subkey
        //
        status = ZwOpenKey( &hBus, KEY_READ, &objectAttributes );
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //
            DbgPrint("CmpFindRSDTTable: ACPI BIOS MultifunctionAdapter registry key not found.\n");
            ZwClose (hMFunc);
            return STATUS_UNSUCCESSFUL;

        }

        //
        // Check the Indentifier to see if this is an ACPI BIOS entry
        //
        status = CmpGetRegistryValue( hBus, rgzAcpiIdentifier, &valueInfo );
        if (!NT_SUCCESS (status)) {

            ZwClose( hBus );
            continue;

        }

        p = (PWSTR) ((PUCHAR) valueInfo->Data);
        unicodeValueName.Buffer = p;
        unicodeValueName.MaximumLength = (USHORT)valueInfo->DataLength;
        length = valueInfo->DataLength;

        //
        // Determine the real length of the ID string
        //
        while (length) {

            if (p[length / sizeof(WCHAR) - 1] == UNICODE_NULL) {

                length -= 2;

            } else {

                break;
            }

        }

        //
        // Do we have a match the "ACPI BIOS" identifier?
        //
        unicodeValueName.Length = (USHORT)length;
        same = RtlEqualUnicodeString( &biosId, &unicodeValueName, TRUE );
        ExFreePool( valueInfo );
        if (!same) {

            ZwClose( hBus );
            continue;

        }

        //
        // We do, so get the configuration data
        //
        status = CmpGetRegistryValue(
            hBus,
            rgzAcpiConfigurationData,
            &valueInfo
            );
        ZwClose( hBus );
        if (!NT_SUCCESS(status)) {

            continue ;

        }

        //
        // The data that we want is at the end of the PARTIAL_RESOURCE_LIST
        // descriptor
        //
        prl = (PCM_PARTIAL_RESOURCE_LIST)(valueInfo->Data);
        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)
            ( (PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST) );
        break;

    }

    //
    // Calculate the size of the data so that we can make a copy
    //
    multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) +
        ( (ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY) );
    *Rsdt = (PACPI_BIOS_MULTI_NODE) ExAllocatePoolWithTag(
        NonPagedPool,
        multiNodeSize,
        'IPCA'
        );
    if (*Rsdt == NULL) {

        ExFreePool( valueInfo );
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlCopyMemory(*Rsdt, multiNode, multiNodeSize);

    //
    // Done with the key memory
    //
    ExFreePool(valueInfo);

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CmpGetRegistryValue(
    IN  HANDLE                          KeyHandle,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION  *Information
    )
/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    NTSTATUS                        status;
    PKEY_VALUE_PARTIAL_INFORMATION  infoBuffer;
    ULONG                           keyValueLength;
    UNICODE_STRING                  unicodeString;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //
    status = ZwQueryValueKey(
        KeyHandle,
        &unicodeString,
        KeyValuePartialInformation,
        (PVOID) NULL,
        0,
        &keyValueLength
        );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {

        return status;

    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //
    infoBuffer = ExAllocatePoolWithTag(
        NonPagedPool,
        keyValueLength,
        'IPCA'
        );
    if (!infoBuffer) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Query the data for the key value.
    //
    status = ZwQueryValueKey(
        KeyHandle,
        &unicodeString,
        KeyValuePartialInformation,
        infoBuffer,
        keyValueLength,
        &keyValueLength
        );
    if (!NT_SUCCESS( status )) {

        ExFreePool( infoBuffer );

        return status;

    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //
    *Information = infoBuffer;
    return STATUS_SUCCESS;

}


