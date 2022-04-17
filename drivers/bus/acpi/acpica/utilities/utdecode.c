/******************************************************************************
 *
 * Module Name: utdecode - Utility decoding routines (value-to-string)
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
#include "acnamesp.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utdecode")


/*
 * Properties of the ACPI Object Types, both internal and external.
 * The table is indexed by values of ACPI_OBJECT_TYPE
 */
const UINT8                     AcpiGbl_NsProperties[ACPI_NUM_NS_TYPES] =
{
    ACPI_NS_NORMAL,                     /* 00 Any              */
    ACPI_NS_NORMAL,                     /* 01 Number           */
    ACPI_NS_NORMAL,                     /* 02 String           */
    ACPI_NS_NORMAL,                     /* 03 Buffer           */
    ACPI_NS_NORMAL,                     /* 04 Package          */
    ACPI_NS_NORMAL,                     /* 05 FieldUnit        */
    ACPI_NS_NEWSCOPE,                   /* 06 Device           */
    ACPI_NS_NORMAL,                     /* 07 Event            */
    ACPI_NS_NEWSCOPE,                   /* 08 Method           */
    ACPI_NS_NORMAL,                     /* 09 Mutex            */
    ACPI_NS_NORMAL,                     /* 10 Region           */
    ACPI_NS_NEWSCOPE,                   /* 11 Power            */
    ACPI_NS_NEWSCOPE,                   /* 12 Processor        */
    ACPI_NS_NEWSCOPE,                   /* 13 Thermal          */
    ACPI_NS_NORMAL,                     /* 14 BufferField      */
    ACPI_NS_NORMAL,                     /* 15 DdbHandle        */
    ACPI_NS_NORMAL,                     /* 16 Debug Object     */
    ACPI_NS_NORMAL,                     /* 17 DefField         */
    ACPI_NS_NORMAL,                     /* 18 BankField        */
    ACPI_NS_NORMAL,                     /* 19 IndexField       */
    ACPI_NS_NORMAL,                     /* 20 Reference        */
    ACPI_NS_NORMAL,                     /* 21 Alias            */
    ACPI_NS_NORMAL,                     /* 22 MethodAlias      */
    ACPI_NS_NORMAL,                     /* 23 Notify           */
    ACPI_NS_NORMAL,                     /* 24 Address Handler  */
    ACPI_NS_NEWSCOPE | ACPI_NS_LOCAL,   /* 25 Resource Desc    */
    ACPI_NS_NEWSCOPE | ACPI_NS_LOCAL,   /* 26 Resource Field   */
    ACPI_NS_NEWSCOPE,                   /* 27 Scope            */
    ACPI_NS_NORMAL,                     /* 28 Extra            */
    ACPI_NS_NORMAL,                     /* 29 Data             */
    ACPI_NS_NORMAL                      /* 30 Invalid          */
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetRegionName
 *
 * PARAMETERS:  Space ID            - ID for the region
 *
 * RETURN:      Decoded region SpaceId name
 *
 * DESCRIPTION: Translate a Space ID into a name string (Debug only)
 *
 ******************************************************************************/

/* Region type decoding */

const char        *AcpiGbl_RegionTypes[ACPI_NUM_PREDEFINED_REGIONS] =
{
    "SystemMemory",       /* 0x00 */
    "SystemIO",           /* 0x01 */
    "PCI_Config",         /* 0x02 */
    "EmbeddedControl",    /* 0x03 */
    "SMBus",              /* 0x04 */
    "SystemCMOS",         /* 0x05 */
    "PCIBARTarget",       /* 0x06 */
    "IPMI",               /* 0x07 */
    "GeneralPurposeIo",   /* 0x08 */
    "GenericSerialBus",   /* 0x09 */
    "PCC",                /* 0x0A */
    "PlatformRtMechanism" /* 0x0B */
};


const char *
AcpiUtGetRegionName (
    UINT8                   SpaceId)
{

    if (SpaceId >= ACPI_USER_REGION_BEGIN)
    {
        return ("UserDefinedRegion");
    }
    else if (SpaceId == ACPI_ADR_SPACE_DATA_TABLE)
    {
        return ("DataTable");
    }
    else if (SpaceId == ACPI_ADR_SPACE_FIXED_HARDWARE)
    {
        return ("FunctionalFixedHW");
    }
    else if (SpaceId >= ACPI_NUM_PREDEFINED_REGIONS)
    {
        return ("InvalidSpaceId");
    }

    return (AcpiGbl_RegionTypes[SpaceId]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetEventName
 *
 * PARAMETERS:  EventId             - Fixed event ID
 *
 * RETURN:      Decoded event ID name
 *
 * DESCRIPTION: Translate a Event ID into a name string (Debug only)
 *
 ******************************************************************************/

/* Event type decoding */

static const char        *AcpiGbl_EventTypes[ACPI_NUM_FIXED_EVENTS] =
{
    "PM_Timer",
    "GlobalLock",
    "PowerButton",
    "SleepButton",
    "RealTimeClock",
};


const char *
AcpiUtGetEventName (
    UINT32                  EventId)
{

    if (EventId > ACPI_EVENT_MAX)
    {
        return ("InvalidEventID");
    }

    return (AcpiGbl_EventTypes[EventId]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetTypeName
 *
 * PARAMETERS:  Type                - An ACPI object type
 *
 * RETURN:      Decoded ACPI object type name
 *
 * DESCRIPTION: Translate a Type ID into a name string (Debug only)
 *
 ******************************************************************************/

/*
 * Elements of AcpiGbl_NsTypeNames below must match
 * one-to-one with values of ACPI_OBJECT_TYPE
 *
 * The type ACPI_TYPE_ANY (Untyped) is used as a "don't care" when searching;
 * when stored in a table it really means that we have thus far seen no
 * evidence to indicate what type is actually going to be stored for this
 & entry.
 */
static const char           AcpiGbl_BadType[] = "UNDEFINED";

/* Printable names of the ACPI object types */

static const char           *AcpiGbl_NsTypeNames[] =
{
    /* 00 */ "Untyped",
    /* 01 */ "Integer",
    /* 02 */ "String",
    /* 03 */ "Buffer",
    /* 04 */ "Package",
    /* 05 */ "FieldUnit",
    /* 06 */ "Device",
    /* 07 */ "Event",
    /* 08 */ "Method",
    /* 09 */ "Mutex",
    /* 10 */ "Region",
    /* 11 */ "Power",
    /* 12 */ "Processor",
    /* 13 */ "Thermal",
    /* 14 */ "BufferField",
    /* 15 */ "DdbHandle",
    /* 16 */ "DebugObject",
    /* 17 */ "RegionField",
    /* 18 */ "BankField",
    /* 19 */ "IndexField",
    /* 20 */ "Reference",
    /* 21 */ "Alias",
    /* 22 */ "MethodAlias",
    /* 23 */ "Notify",
    /* 24 */ "AddrHandler",
    /* 25 */ "ResourceDesc",
    /* 26 */ "ResourceFld",
    /* 27 */ "Scope",
    /* 28 */ "Extra",
    /* 29 */ "Data",
    /* 30 */ "Invalid"
};


const char *
AcpiUtGetTypeName (
    ACPI_OBJECT_TYPE        Type)
{

    if (Type > ACPI_TYPE_INVALID)
    {
        return (AcpiGbl_BadType);
    }

    return (AcpiGbl_NsTypeNames[Type]);
}


const char *
AcpiUtGetObjectTypeName (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_FUNCTION_TRACE (UtGetObjectTypeName);


    if (!ObjDesc)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Null Object Descriptor\n"));
        return_STR ("[NULL Object Descriptor]");
    }

    /* These descriptor types share a common area */

    if ((ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) != ACPI_DESC_TYPE_OPERAND) &&
        (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) != ACPI_DESC_TYPE_NAMED))
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "Invalid object descriptor type: 0x%2.2X [%s] (%p)\n",
            ACPI_GET_DESCRIPTOR_TYPE (ObjDesc),
            AcpiUtGetDescriptorName (ObjDesc), ObjDesc));

        return_STR ("Invalid object");
    }

    return_STR (AcpiUtGetTypeName (ObjDesc->Common.Type));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetNodeName
 *
 * PARAMETERS:  Object               - A namespace node
 *
 * RETURN:      ASCII name of the node
 *
 * DESCRIPTION: Validate the node and return the node's ACPI name.
 *
 ******************************************************************************/

const char *
AcpiUtGetNodeName (
    void                    *Object)
{
    ACPI_NAMESPACE_NODE     *Node = (ACPI_NAMESPACE_NODE *) Object;


    /* Must return a string of exactly 4 characters == ACPI_NAMESEG_SIZE */

    if (!Object)
    {
        return ("NULL");
    }

    /* Check for Root node */

    if ((Object == ACPI_ROOT_OBJECT) ||
        (Object == AcpiGbl_RootNode))
    {
        return ("\"\\\" ");
    }

    /* Descriptor must be a namespace node */

    if (ACPI_GET_DESCRIPTOR_TYPE (Node) != ACPI_DESC_TYPE_NAMED)
    {
        return ("####");
    }

    /*
     * Ensure name is valid. The name was validated/repaired when the node
     * was created, but make sure it has not been corrupted.
     */
    AcpiUtRepairName (Node->Name.Ascii);

    /* Return the name */

    return (Node->Name.Ascii);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetDescriptorName
 *
 * PARAMETERS:  Object               - An ACPI object
 *
 * RETURN:      Decoded name of the descriptor type
 *
 * DESCRIPTION: Validate object and return the descriptor type
 *
 ******************************************************************************/

/* Printable names of object descriptor types */

static const char           *AcpiGbl_DescTypeNames[] =
{
    /* 00 */ "Not a Descriptor",
    /* 01 */ "Cached Object",
    /* 02 */ "State-Generic",
    /* 03 */ "State-Update",
    /* 04 */ "State-Package",
    /* 05 */ "State-Control",
    /* 06 */ "State-RootParseScope",
    /* 07 */ "State-ParseScope",
    /* 08 */ "State-WalkScope",
    /* 09 */ "State-Result",
    /* 10 */ "State-Notify",
    /* 11 */ "State-Thread",
    /* 12 */ "Tree Walk State",
    /* 13 */ "Parse Tree Op",
    /* 14 */ "Operand Object",
    /* 15 */ "Namespace Node"
};


const char *
AcpiUtGetDescriptorName (
    void                    *Object)
{

    if (!Object)
    {
        return ("NULL OBJECT");
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (Object) > ACPI_DESC_TYPE_MAX)
    {
        return ("Not a Descriptor");
    }

    return (AcpiGbl_DescTypeNames[ACPI_GET_DESCRIPTOR_TYPE (Object)]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetReferenceName
 *
 * PARAMETERS:  Object               - An ACPI reference object
 *
 * RETURN:      Decoded name of the type of reference
 *
 * DESCRIPTION: Decode a reference object sub-type to a string.
 *
 ******************************************************************************/

/* Printable names of reference object sub-types */

static const char           *AcpiGbl_RefClassNames[] =
{
    /* 00 */ "Local",
    /* 01 */ "Argument",
    /* 02 */ "RefOf",
    /* 03 */ "Index",
    /* 04 */ "DdbHandle",
    /* 05 */ "Named Object",
    /* 06 */ "Debug"
};

const char *
AcpiUtGetReferenceName (
    ACPI_OPERAND_OBJECT     *Object)
{

    if (!Object)
    {
        return ("NULL Object");
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (Object) != ACPI_DESC_TYPE_OPERAND)
    {
        return ("Not an Operand object");
    }

    if (Object->Common.Type != ACPI_TYPE_LOCAL_REFERENCE)
    {
        return ("Not a Reference object");
    }

    if (Object->Reference.Class > ACPI_REFCLASS_MAX)
    {
        return ("Unknown Reference class");
    }

    return (AcpiGbl_RefClassNames[Object->Reference.Class]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetMutexName
 *
 * PARAMETERS:  MutexId         - The predefined ID for this mutex.
 *
 * RETURN:      Decoded name of the internal mutex
 *
 * DESCRIPTION: Translate a mutex ID into a name string (Debug only)
 *
 ******************************************************************************/

/* Names for internal mutex objects, used for debug output */

static const char           *AcpiGbl_MutexNames[ACPI_NUM_MUTEX] =
{
    "ACPI_MTX_Interpreter",
    "ACPI_MTX_Namespace",
    "ACPI_MTX_Tables",
    "ACPI_MTX_Events",
    "ACPI_MTX_Caches",
    "ACPI_MTX_Memory",
};

const char *
AcpiUtGetMutexName (
    UINT32                  MutexId)
{

    if (MutexId > ACPI_MAX_MUTEX)
    {
        return ("Invalid Mutex ID");
    }

    return (AcpiGbl_MutexNames[MutexId]);
}


#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

/*
 * Strings and procedures used for debug only
 */

/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetNotifyName
 *
 * PARAMETERS:  NotifyValue     - Value from the Notify() request
 *
 * RETURN:      Decoded name for the notify value
 *
 * DESCRIPTION: Translate a Notify Value to a notify namestring.
 *
 ******************************************************************************/

/* Names for Notify() values, used for debug output */

static const char           *AcpiGbl_GenericNotify[ACPI_GENERIC_NOTIFY_MAX + 1] =
{
    /* 00 */ "Bus Check",
    /* 01 */ "Device Check",
    /* 02 */ "Device Wake",
    /* 03 */ "Eject Request",
    /* 04 */ "Device Check Light",
    /* 05 */ "Frequency Mismatch",
    /* 06 */ "Bus Mode Mismatch",
    /* 07 */ "Power Fault",
    /* 08 */ "Capabilities Check",
    /* 09 */ "Device PLD Check",
    /* 0A */ "Reserved",
    /* 0B */ "System Locality Update",
    /* 0C */ "Reserved (was previously Shutdown Request)",  /* Reserved in ACPI 6.0 */
    /* 0D */ "System Resource Affinity Update",
    /* 0E */ "Heterogeneous Memory Attributes Update",      /* ACPI 6.2 */
    /* 0F */ "Error Disconnect Recover"                     /* ACPI 6.3 */
};

static const char           *AcpiGbl_DeviceNotify[5] =
{
    /* 80 */ "Status Change",
    /* 81 */ "Information Change",
    /* 82 */ "Device-Specific Change",
    /* 83 */ "Device-Specific Change",
    /* 84 */ "Reserved"
};

static const char           *AcpiGbl_ProcessorNotify[5] =
{
    /* 80 */ "Performance Capability Change",
    /* 81 */ "C-State Change",
    /* 82 */ "Throttling Capability Change",
    /* 83 */ "Guaranteed Change",
    /* 84 */ "Minimum Excursion"
};

static const char           *AcpiGbl_ThermalNotify[5] =
{
    /* 80 */ "Thermal Status Change",
    /* 81 */ "Thermal Trip Point Change",
    /* 82 */ "Thermal Device List Change",
    /* 83 */ "Thermal Relationship Change",
    /* 84 */ "Reserved"
};


const char *
AcpiUtGetNotifyName (
    UINT32                  NotifyValue,
    ACPI_OBJECT_TYPE        Type)
{

    /* 00 - 0F are "common to all object types" (from ACPI Spec) */

    if (NotifyValue <= ACPI_GENERIC_NOTIFY_MAX)
    {
        return (AcpiGbl_GenericNotify[NotifyValue]);
    }

    /* 10 - 7F are reserved */

    if (NotifyValue <= ACPI_MAX_SYS_NOTIFY)
    {
        return ("Reserved");
    }

    /* 80 - 84 are per-object-type */

    if (NotifyValue <= ACPI_SPECIFIC_NOTIFY_MAX)
    {
        switch (Type)
        {
        case ACPI_TYPE_ANY:
        case ACPI_TYPE_DEVICE:
            return (AcpiGbl_DeviceNotify [NotifyValue - 0x80]);

        case ACPI_TYPE_PROCESSOR:
            return (AcpiGbl_ProcessorNotify [NotifyValue - 0x80]);

        case ACPI_TYPE_THERMAL:
            return (AcpiGbl_ThermalNotify [NotifyValue - 0x80]);

        default:
            return ("Target object type does not support notifies");
        }
    }

    /* 84 - BF are device-specific */

    if (NotifyValue <= ACPI_MAX_DEVICE_SPECIFIC_NOTIFY)
    {
        return ("Device-Specific");
    }

    /* C0 and above are hardware-specific */

    return ("Hardware-Specific");
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGetArgumentTypeName
 *
 * PARAMETERS:  ArgType             - an ARGP_* parser argument type
 *
 * RETURN:      Decoded ARGP_* type
 *
 * DESCRIPTION: Decode an ARGP_* parser type, as defined in the amlcode.h file,
 *              and used in the acopcode.h file. For example, ARGP_TERMARG.
 *              Used for debug only.
 *
 ******************************************************************************/

static const char           *AcpiGbl_ArgumentType[20] =
{
    /* 00 */ "Unknown ARGP",
    /* 01 */ "ByteData",
    /* 02 */ "ByteList",
    /* 03 */ "CharList",
    /* 04 */ "DataObject",
    /* 05 */ "DataObjectList",
    /* 06 */ "DWordData",
    /* 07 */ "FieldList",
    /* 08 */ "Name",
    /* 09 */ "NameString",
    /* 0A */ "ObjectList",
    /* 0B */ "PackageLength",
    /* 0C */ "SuperName",
    /* 0D */ "Target",
    /* 0E */ "TermArg",
    /* 0F */ "TermList",
    /* 10 */ "WordData",
    /* 11 */ "QWordData",
    /* 12 */ "SimpleName",
    /* 13 */ "NameOrRef"
};

const char *
AcpiUtGetArgumentTypeName (
    UINT32                  ArgType)
{

    if (ArgType > ARGP_MAX)
    {
        return ("Unknown ARGP");
    }

    return (AcpiGbl_ArgumentType[ArgType]);
}

#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidObjectType
 *
 * PARAMETERS:  Type            - Object type to be validated
 *
 * RETURN:      TRUE if valid object type, FALSE otherwise
 *
 * DESCRIPTION: Validate an object type
 *
 ******************************************************************************/

BOOLEAN
AcpiUtValidObjectType (
    ACPI_OBJECT_TYPE        Type)
{

    if (Type > ACPI_TYPE_LOCAL_MAX)
    {
        /* Note: Assumes all TYPEs are contiguous (external/local) */

        return (FALSE);
    }

    return (TRUE);
}
