/******************************************************************************
 *
 * Name: acutils.h -- prototypes for the common (subsystem-wide) procedures
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef _ACUTILS_H
#define _ACUTILS_H


extern const UINT8                      AcpiGbl_ResourceAmlSizes[];
extern const UINT8                      AcpiGbl_ResourceAmlSerialBusSizes[];

/* Strings used by the disassembler and debugger resource dump routines */

#if defined(ACPI_DEBUG_OUTPUT) || defined (ACPI_DISASSEMBLER) || defined (ACPI_DEBUGGER)

extern const char                       *AcpiGbl_BmDecode[];
extern const char                       *AcpiGbl_ConfigDecode[];
extern const char                       *AcpiGbl_ConsumeDecode[];
extern const char                       *AcpiGbl_DecDecode[];
extern const char                       *AcpiGbl_HeDecode[];
extern const char                       *AcpiGbl_IoDecode[];
extern const char                       *AcpiGbl_LlDecode[];
extern const char                       *AcpiGbl_MaxDecode[];
extern const char                       *AcpiGbl_MemDecode[];
extern const char                       *AcpiGbl_MinDecode[];
extern const char                       *AcpiGbl_MtpDecode[];
extern const char                       *AcpiGbl_RngDecode[];
extern const char                       *AcpiGbl_RwDecode[];
extern const char                       *AcpiGbl_ShrDecode[];
extern const char                       *AcpiGbl_SizDecode[];
extern const char                       *AcpiGbl_TrsDecode[];
extern const char                       *AcpiGbl_TtpDecode[];
extern const char                       *AcpiGbl_TypDecode[];
extern const char                       *AcpiGbl_PpcDecode[];
extern const char                       *AcpiGbl_IorDecode[];
extern const char                       *AcpiGbl_DtsDecode[];
extern const char                       *AcpiGbl_CtDecode[];
extern const char                       *AcpiGbl_SbtDecode[];
extern const char                       *AcpiGbl_AmDecode[];
extern const char                       *AcpiGbl_SmDecode[];
extern const char                       *AcpiGbl_WmDecode[];
extern const char                       *AcpiGbl_CphDecode[];
extern const char                       *AcpiGbl_CpoDecode[];
extern const char                       *AcpiGbl_DpDecode[];
extern const char                       *AcpiGbl_EdDecode[];
extern const char                       *AcpiGbl_BpbDecode[];
extern const char                       *AcpiGbl_SbDecode[];
extern const char                       *AcpiGbl_FcDecode[];
extern const char                       *AcpiGbl_PtDecode[];
#endif

/*
 * For the iASL compiler case, the output is redirected to stderr so that
 * any of the various ACPI errors and warnings do not appear in the output
 * files, for either the compiler or disassembler portions of the tool.
 */
#ifdef ACPI_ASL_COMPILER

#include <stdio.h>

#define ACPI_MSG_REDIRECT_BEGIN \
    FILE                    *OutputFile = AcpiGbl_OutputFile; \
    AcpiOsRedirectOutput (stderr);

#define ACPI_MSG_REDIRECT_END \
    AcpiOsRedirectOutput (OutputFile);

#else
/*
 * non-iASL case - no redirection, nothing to do
 */
#define ACPI_MSG_REDIRECT_BEGIN
#define ACPI_MSG_REDIRECT_END
#endif

/*
 * Common error message prefixes
 */
#define ACPI_MSG_ERROR          "ACPI Error: "
#define ACPI_MSG_EXCEPTION      "ACPI Exception: "
#define ACPI_MSG_WARNING        "ACPI Warning: "
#define ACPI_MSG_INFO           "ACPI: "

#define ACPI_MSG_BIOS_ERROR     "ACPI BIOS Error (bug): "
#define ACPI_MSG_BIOS_WARNING   "ACPI BIOS Warning (bug): "

/*
 * Common message suffix
 */
#define ACPI_MSG_SUFFIX \
    AcpiOsPrintf (" (%8.8X/%s-%u)\n", ACPI_CA_VERSION, ModuleName, LineNumber)


/* Types for Resource descriptor entries */

#define ACPI_INVALID_RESOURCE           0
#define ACPI_FIXED_LENGTH               1
#define ACPI_VARIABLE_LENGTH            2
#define ACPI_SMALL_VARIABLE_LENGTH      3

typedef
ACPI_STATUS (*ACPI_WALK_AML_CALLBACK) (
    UINT8                   *Aml,
    UINT32                  Length,
    UINT32                  Offset,
    UINT8                   ResourceIndex,
    void                    **Context);

typedef
ACPI_STATUS (*ACPI_PKG_CALLBACK) (
    UINT8                   ObjectType,
    ACPI_OPERAND_OBJECT     *SourceObject,
    ACPI_GENERIC_STATE      *State,
    void                    *Context);

typedef struct acpi_pkg_info
{
    UINT8                   *FreeSpace;
    ACPI_SIZE               Length;
    UINT32                  ObjectSpace;
    UINT32                  NumPackages;

} ACPI_PKG_INFO;

/* Object reference counts */

#define REF_INCREMENT       (UINT16) 0
#define REF_DECREMENT       (UINT16) 1

/* AcpiUtDumpBuffer */

#define DB_BYTE_DISPLAY     1
#define DB_WORD_DISPLAY     2
#define DB_DWORD_DISPLAY    4
#define DB_QWORD_DISPLAY    8

/*
 * utglobal - Global data structures and procedures
 */
ACPI_STATUS
AcpiUtInitGlobals (
    void);

#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

char *
AcpiUtGetMutexName (
    UINT32                  MutexId);

const char *
AcpiUtGetNotifyName (
    UINT32                  NotifyValue,
    ACPI_OBJECT_TYPE        Type);
#endif

char *
AcpiUtGetTypeName (
    ACPI_OBJECT_TYPE        Type);

char *
AcpiUtGetNodeName (
    void                    *Object);

char *
AcpiUtGetDescriptorName (
    void                    *Object);

const char *
AcpiUtGetReferenceName (
    ACPI_OPERAND_OBJECT     *Object);

char *
AcpiUtGetObjectTypeName (
    ACPI_OPERAND_OBJECT     *ObjDesc);

char *
AcpiUtGetRegionName (
    UINT8                   SpaceId);

char *
AcpiUtGetEventName (
    UINT32                  EventId);

char
AcpiUtHexToAsciiChar (
    UINT64                  Integer,
    UINT32                  Position);

UINT8
AcpiUtAsciiCharToHex (
    int                     HexChar);

BOOLEAN
AcpiUtValidObjectType (
    ACPI_OBJECT_TYPE        Type);


/*
 * utinit - miscellaneous initialization and shutdown
 */
ACPI_STATUS
AcpiUtHardwareInitialize (
    void);

void
AcpiUtSubsystemShutdown (
    void);


/*
 * utclib - Local implementations of C library functions
 */
#ifndef ACPI_USE_SYSTEM_CLIBRARY

ACPI_SIZE
AcpiUtStrlen (
    const char              *String);

char *
AcpiUtStrchr (
    const char              *String,
    int                     ch);

char *
AcpiUtStrcpy (
    char                    *DstString,
    const char              *SrcString);

char *
AcpiUtStrncpy (
    char                    *DstString,
    const char              *SrcString,
    ACPI_SIZE               Count);

int
AcpiUtMemcmp (
    const char              *Buffer1,
    const char              *Buffer2,
    ACPI_SIZE               Count);

int
AcpiUtStrncmp (
    const char              *String1,
    const char              *String2,
    ACPI_SIZE               Count);

int
AcpiUtStrcmp (
    const char              *String1,
    const char              *String2);

char *
AcpiUtStrcat (
    char                    *DstString,
    const char              *SrcString);

char *
AcpiUtStrncat (
    char                    *DstString,
    const char              *SrcString,
    ACPI_SIZE               Count);

UINT32
AcpiUtStrtoul (
    const char              *String,
    char                    **Terminator,
    UINT32                  Base);

char *
AcpiUtStrstr (
    char                    *String1,
    char                    *String2);

void *
AcpiUtMemcpy (
    void                    *Dest,
    const void              *Src,
    ACPI_SIZE               Count);

void *
AcpiUtMemset (
    void                    *Dest,
    UINT8                   Value,
    ACPI_SIZE               Count);

int
AcpiUtToUpper (
    int                     c);

int
AcpiUtToLower (
    int                     c);

extern const UINT8 _acpi_ctype[];

#define _ACPI_XA     0x00    /* extra alphabetic - not supported */
#define _ACPI_XS     0x40    /* extra space */
#define _ACPI_BB     0x00    /* BEL, BS, etc. - not supported */
#define _ACPI_CN     0x20    /* CR, FF, HT, NL, VT */
#define _ACPI_DI     0x04    /* '0'-'9' */
#define _ACPI_LO     0x02    /* 'a'-'z' */
#define _ACPI_PU     0x10    /* punctuation */
#define _ACPI_SP     0x08    /* space */
#define _ACPI_UP     0x01    /* 'A'-'Z' */
#define _ACPI_XD     0x80    /* '0'-'9', 'A'-'F', 'a'-'f' */

#define ACPI_IS_DIGIT(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_DI))
#define ACPI_IS_SPACE(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_SP))
#define ACPI_IS_XDIGIT(c) (_acpi_ctype[(unsigned char)(c)] & (_ACPI_XD))
#define ACPI_IS_UPPER(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_UP))
#define ACPI_IS_LOWER(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_LO))
#define ACPI_IS_PRINT(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_LO | _ACPI_UP | _ACPI_DI | _ACPI_XS | _ACPI_PU))
#define ACPI_IS_ALPHA(c)  (_acpi_ctype[(unsigned char)(c)] & (_ACPI_LO | _ACPI_UP))

#endif /* !ACPI_USE_SYSTEM_CLIBRARY */

#define ACPI_IS_ASCII(c)  ((c) < 0x80)


/*
 * utcopy - Object construction and conversion interfaces
 */
ACPI_STATUS
AcpiUtBuildSimpleObject(
    ACPI_OPERAND_OBJECT     *Obj,
    ACPI_OBJECT             *UserObj,
    UINT8                   *DataSpace,
    UINT32                  *BufferSpaceUsed);

ACPI_STATUS
AcpiUtBuildPackageObject (
    ACPI_OPERAND_OBJECT     *Obj,
    UINT8                   *Buffer,
    UINT32                  *SpaceUsed);

ACPI_STATUS
AcpiUtCopyIobjectToEobject (
    ACPI_OPERAND_OBJECT     *Obj,
    ACPI_BUFFER             *RetBuffer);

ACPI_STATUS
AcpiUtCopyEobjectToIobject (
    ACPI_OBJECT             *Obj,
    ACPI_OPERAND_OBJECT     **InternalObj);

ACPI_STATUS
AcpiUtCopyISimpleToIsimple (
    ACPI_OPERAND_OBJECT     *SourceObj,
    ACPI_OPERAND_OBJECT     *DestObj);

ACPI_STATUS
AcpiUtCopyIobjectToIobject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     **DestDesc,
    ACPI_WALK_STATE         *WalkState);


/*
 * utcreate - Object creation
 */
ACPI_STATUS
AcpiUtUpdateObjectReference (
    ACPI_OPERAND_OBJECT     *Object,
    UINT16                  Action);


/*
 * utdebug - Debug interfaces
 */
void
AcpiUtInitStackPtrTrace (
    void);

void
AcpiUtTrackStackPtr (
    void);

void
AcpiUtTrace (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId);

void
AcpiUtTracePtr (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    void                    *Pointer);

void
AcpiUtTraceU32 (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT32                  Integer);

void
AcpiUtTraceStr (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    char                    *String);

void
AcpiUtExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId);

void
AcpiUtStatusExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    ACPI_STATUS             Status);

void
AcpiUtValueExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT64                  Value);

void
AcpiUtPtrExit (
    UINT32                  LineNumber,
    const char              *FunctionName,
    const char              *ModuleName,
    UINT32                  ComponentId,
    UINT8                   *Ptr);

void
AcpiUtDebugDumpBuffer (
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display,
    UINT32                  ComponentId);

void
AcpiUtDumpBuffer (
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display,
    UINT32                  Offset);

#ifdef ACPI_APPLICATION
void
AcpiUtDumpBufferToFile (
    ACPI_FILE               File,
    UINT8                   *Buffer,
    UINT32                  Count,
    UINT32                  Display,
    UINT32                  BaseOffset);
#endif

void
AcpiUtReportError (
    char                    *ModuleName,
    UINT32                  LineNumber);

void
AcpiUtReportInfo (
    char                    *ModuleName,
    UINT32                  LineNumber);

void
AcpiUtReportWarning (
    char                    *ModuleName,
    UINT32                  LineNumber);

/*
 * utdelete - Object deletion and reference counts
 */
void
AcpiUtAddReference (
    ACPI_OPERAND_OBJECT     *Object);

void
AcpiUtRemoveReference (
    ACPI_OPERAND_OBJECT     *Object);

void
AcpiUtDeleteInternalPackageObject (
    ACPI_OPERAND_OBJECT     *Object);

void
AcpiUtDeleteInternalSimpleObject (
    ACPI_OPERAND_OBJECT     *Object);

void
AcpiUtDeleteInternalObjectList (
    ACPI_OPERAND_OBJECT     **ObjList);


/*
 * uteval - object evaluation
 */
ACPI_STATUS
AcpiUtEvaluateObject (
    ACPI_NAMESPACE_NODE     *PrefixNode,
    char                    *Path,
    UINT32                  ExpectedReturnBtypes,
    ACPI_OPERAND_OBJECT     **ReturnDesc);

ACPI_STATUS
AcpiUtEvaluateNumericObject (
    char                    *ObjectName,
    ACPI_NAMESPACE_NODE     *DeviceNode,
    UINT64                  *Value);

ACPI_STATUS
AcpiUtExecute_STA (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    UINT32                  *StatusFlags);

ACPI_STATUS
AcpiUtExecutePowerMethods (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    const char              **MethodNames,
    UINT8                   MethodCount,
    UINT8                   *OutValues);


/*
 * utfileio - file operations
 */
#ifdef ACPI_APPLICATION
ACPI_STATUS
AcpiUtReadTableFromFile (
    char                    *Filename,
    ACPI_TABLE_HEADER       **Table);
#endif


/*
 * utids - device ID support
 */
ACPI_STATUS
AcpiUtExecute_HID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId);

ACPI_STATUS
AcpiUtExecute_UID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId);

ACPI_STATUS
AcpiUtExecute_SUB (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID      **ReturnId);

ACPI_STATUS
AcpiUtExecute_CID (
    ACPI_NAMESPACE_NODE     *DeviceNode,
    ACPI_PNP_DEVICE_ID_LIST **ReturnCidList);


/*
 * utlock - reader/writer locks
 */
ACPI_STATUS
AcpiUtCreateRwLock (
    ACPI_RW_LOCK            *Lock);

void
AcpiUtDeleteRwLock (
    ACPI_RW_LOCK            *Lock);

ACPI_STATUS
AcpiUtAcquireReadLock (
    ACPI_RW_LOCK            *Lock);

ACPI_STATUS
AcpiUtReleaseReadLock (
    ACPI_RW_LOCK            *Lock);

ACPI_STATUS
AcpiUtAcquireWriteLock (
    ACPI_RW_LOCK            *Lock);

void
AcpiUtReleaseWriteLock (
    ACPI_RW_LOCK            *Lock);


/*
 * utobject - internal object create/delete/cache routines
 */
ACPI_OPERAND_OBJECT  *
AcpiUtCreateInternalObjectDbg (
    const char              *ModuleName,
    UINT32                  LineNumber,
    UINT32                  ComponentId,
    ACPI_OBJECT_TYPE        Type);

void *
AcpiUtAllocateObjectDescDbg (
    const char              *ModuleName,
    UINT32                  LineNumber,
    UINT32                  ComponentId);

#define AcpiUtCreateInternalObject(t)   AcpiUtCreateInternalObjectDbg (_AcpiModuleName,__LINE__,_COMPONENT,t)
#define AcpiUtAllocateObjectDesc()      AcpiUtAllocateObjectDescDbg (_AcpiModuleName,__LINE__,_COMPONENT)

void
AcpiUtDeleteObjectDesc (
    ACPI_OPERAND_OBJECT     *Object);

BOOLEAN
AcpiUtValidInternalObject (
    void                    *Object);

ACPI_OPERAND_OBJECT *
AcpiUtCreatePackageObject (
    UINT32                  Count);

ACPI_OPERAND_OBJECT *
AcpiUtCreateIntegerObject (
    UINT64                  Value);

ACPI_OPERAND_OBJECT *
AcpiUtCreateBufferObject (
    ACPI_SIZE               BufferSize);

ACPI_OPERAND_OBJECT *
AcpiUtCreateStringObject (
    ACPI_SIZE               StringSize);

ACPI_STATUS
AcpiUtGetObjectSize(
    ACPI_OPERAND_OBJECT     *Obj,
    ACPI_SIZE               *ObjLength);


/*
 * utosi - Support for the _OSI predefined control method
 */
ACPI_STATUS
AcpiUtInitializeInterfaces (
    void);

ACPI_STATUS
AcpiUtInterfaceTerminate (
    void);

ACPI_STATUS
AcpiUtInstallInterface (
    ACPI_STRING             InterfaceName);

ACPI_STATUS
AcpiUtRemoveInterface (
    ACPI_STRING             InterfaceName);

ACPI_STATUS
AcpiUtUpdateInterfaces (
    UINT8                   Action);

ACPI_INTERFACE_INFO *
AcpiUtGetInterface (
    ACPI_STRING             InterfaceName);

ACPI_STATUS
AcpiUtOsiImplementation (
    ACPI_WALK_STATE         *WalkState);


/*
 * utpredef - support for predefined names
 */
const ACPI_PREDEFINED_INFO *
AcpiUtGetNextPredefinedMethod (
    const ACPI_PREDEFINED_INFO  *ThisName);

const ACPI_PREDEFINED_INFO *
AcpiUtMatchPredefinedMethod (
    char                        *Name);

void
AcpiUtGetExpectedReturnTypes (
    char                    *Buffer,
    UINT32                  ExpectedBtypes);

#if (defined ACPI_ASL_COMPILER || defined ACPI_HELP_APP)
const ACPI_PREDEFINED_INFO *
AcpiUtMatchResourceName (
    char                        *Name);

void
AcpiUtDisplayPredefinedMethod (
    char                        *Buffer,
    const ACPI_PREDEFINED_INFO  *ThisName,
    BOOLEAN                     MultiLine);

UINT32
AcpiUtGetResourceBitWidth (
    char                    *Buffer,
    UINT16                  Types);
#endif


/*
 * utstate - Generic state creation/cache routines
 */
void
AcpiUtPushGenericState (
    ACPI_GENERIC_STATE      **ListHead,
    ACPI_GENERIC_STATE      *State);

ACPI_GENERIC_STATE *
AcpiUtPopGenericState (
    ACPI_GENERIC_STATE      **ListHead);


ACPI_GENERIC_STATE *
AcpiUtCreateGenericState (
    void);

ACPI_THREAD_STATE *
AcpiUtCreateThreadState (
    void);

ACPI_GENERIC_STATE *
AcpiUtCreateUpdateState (
    ACPI_OPERAND_OBJECT     *Object,
    UINT16                  Action);

ACPI_GENERIC_STATE *
AcpiUtCreatePkgState (
    void                    *InternalObject,
    void                    *ExternalObject,
    UINT16                  Index);

ACPI_STATUS
AcpiUtCreateUpdateStateAndPush (
    ACPI_OPERAND_OBJECT     *Object,
    UINT16                  Action,
    ACPI_GENERIC_STATE      **StateList);

ACPI_GENERIC_STATE *
AcpiUtCreateControlState (
    void);

void
AcpiUtDeleteGenericState (
    ACPI_GENERIC_STATE      *State);


/*
 * utmath
 */
ACPI_STATUS
AcpiUtDivide (
    UINT64                  InDividend,
    UINT64                  InDivisor,
    UINT64                  *OutQuotient,
    UINT64                  *OutRemainder);

ACPI_STATUS
AcpiUtShortDivide (
    UINT64                  InDividend,
    UINT32                  Divisor,
    UINT64                  *OutQuotient,
    UINT32                  *OutRemainder);


/*
 * utmisc
 */
const ACPI_EXCEPTION_INFO *
AcpiUtValidateException (
    ACPI_STATUS             Status);

BOOLEAN
AcpiUtIsPciRootBridge (
    char                    *Id);

#if (defined ACPI_ASL_COMPILER || defined ACPI_EXEC_APP)
BOOLEAN
AcpiUtIsAmlTable (
    ACPI_TABLE_HEADER       *Table);
#endif

ACPI_STATUS
AcpiUtWalkPackageTree (
    ACPI_OPERAND_OBJECT     *SourceObject,
    void                    *TargetObject,
    ACPI_PKG_CALLBACK       WalkCallback,
    void                    *Context);


/* Values for Base above (16=Hex, 10=Decimal) */

#define ACPI_ANY_BASE        0

UINT32
AcpiUtDwordByteSwap (
    UINT32                  Value);

void
AcpiUtSetIntegerWidth (
    UINT8                   Revision);

#ifdef ACPI_DEBUG_OUTPUT
void
AcpiUtDisplayInitPathname (
    UINT8                   Type,
    ACPI_NAMESPACE_NODE     *ObjHandle,
    char                    *Path);
#endif


/*
 * utownerid - Support for Table/Method Owner IDs
 */
ACPI_STATUS
AcpiUtAllocateOwnerId (
    ACPI_OWNER_ID           *OwnerId);

void
AcpiUtReleaseOwnerId (
    ACPI_OWNER_ID           *OwnerId);


/*
 * utresrc
 */
ACPI_STATUS
AcpiUtWalkAmlResources (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *Aml,
    ACPI_SIZE               AmlLength,
    ACPI_WALK_AML_CALLBACK  UserFunction,
    void                    **Context);

ACPI_STATUS
AcpiUtValidateResource (
    ACPI_WALK_STATE         *WalkState,
    void                    *Aml,
    UINT8                   *ReturnIndex);

UINT32
AcpiUtGetDescriptorLength (
    void                    *Aml);

UINT16
AcpiUtGetResourceLength (
    void                    *Aml);

UINT8
AcpiUtGetResourceHeaderLength (
    void                    *Aml);

UINT8
AcpiUtGetResourceType (
    void                    *Aml);

ACPI_STATUS
AcpiUtGetResourceEndTag (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   **EndTag);


/*
 * utstring - String and character utilities
 */
void
AcpiUtStrupr (
    char                    *SrcString);

#ifdef ACPI_ASL_COMPILER
void
AcpiUtStrlwr (
    char                    *SrcString);

int
AcpiUtStricmp (
    char                    *String1,
    char                    *String2);
#endif

ACPI_STATUS
AcpiUtStrtoul64 (
    char                    *String,
    UINT32                  Base,
    UINT64                  *RetInteger);

void
AcpiUtPrintString (
    char                    *String,
    UINT16                  MaxLength);

#if defined ACPI_ASL_COMPILER || defined ACPI_EXEC_APP
void
UtConvertBackslashes (
    char                    *Pathname);
#endif

BOOLEAN
AcpiUtValidAcpiName (
    char                    *Name);

BOOLEAN
AcpiUtValidAcpiChar (
    char                    Character,
    UINT32                  Position);

void
AcpiUtRepairName (
    char                    *Name);

#if defined (ACPI_DEBUGGER) || defined (ACPI_APPLICATION)
BOOLEAN
AcpiUtSafeStrcpy (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source);

BOOLEAN
AcpiUtSafeStrcat (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source);

BOOLEAN
AcpiUtSafeStrncat (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source,
    ACPI_SIZE               MaxTransferLength);
#endif


/*
 * utmutex - mutex support
 */
ACPI_STATUS
AcpiUtMutexInitialize (
    void);

void
AcpiUtMutexTerminate (
    void);

ACPI_STATUS
AcpiUtAcquireMutex (
    ACPI_MUTEX_HANDLE       MutexId);

ACPI_STATUS
AcpiUtReleaseMutex (
    ACPI_MUTEX_HANDLE       MutexId);


/*
 * utalloc - memory allocation and object caching
 */
ACPI_STATUS
AcpiUtCreateCaches (
    void);

ACPI_STATUS
AcpiUtDeleteCaches (
    void);

ACPI_STATUS
AcpiUtValidateBuffer (
    ACPI_BUFFER             *Buffer);

ACPI_STATUS
AcpiUtInitializeBuffer (
    ACPI_BUFFER             *Buffer,
    ACPI_SIZE               RequiredLength);

#ifdef ACPI_DBG_TRACK_ALLOCATIONS
void *
AcpiUtAllocateAndTrack (
    ACPI_SIZE               Size,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line);

void *
AcpiUtAllocateZeroedAndTrack (
    ACPI_SIZE               Size,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line);

void
AcpiUtFreeAndTrack (
    void                    *Address,
    UINT32                  Component,
    const char              *Module,
    UINT32                  Line);

void
AcpiUtDumpAllocationInfo (
    void);

void
AcpiUtDumpAllocations (
    UINT32                  Component,
    const char              *Module);

ACPI_STATUS
AcpiUtCreateList (
    char                    *ListName,
    UINT16                  ObjectSize,
    ACPI_MEMORY_LIST        **ReturnCache);

#endif /* ACPI_DBG_TRACK_ALLOCATIONS */

/*
 * utaddress - address range check
 */
ACPI_STATUS
AcpiUtAddAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Length,
    ACPI_NAMESPACE_NODE     *RegionNode);

void
AcpiUtRemoveAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_NAMESPACE_NODE     *RegionNode);

UINT32
AcpiUtCheckAddressRange (
    ACPI_ADR_SPACE_TYPE     SpaceId,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  Length,
    BOOLEAN                 Warn);

void
AcpiUtDeleteAddressLists (
    void);

/*
 * utxferror - various error/warning output functions
 */
void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedWarning (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...);

void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedInfo (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...);

void ACPI_INTERNAL_VAR_XFACE
AcpiUtPredefinedBiosError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    char                    *Pathname,
    UINT8                   NodeFlags,
    const char              *Format,
    ...);

void
AcpiUtNamespaceError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *InternalName,
    ACPI_STATUS             LookupStatus);

void
AcpiUtMethodError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Message,
    ACPI_NAMESPACE_NODE     *Node,
    const char              *Path,
    ACPI_STATUS             LookupStatus);

/*
 * Utility functions for ACPI names and IDs
 */
const AH_PREDEFINED_NAME *
AcpiAhMatchPredefinedName (
    char                    *Nameseg);

const AH_DEVICE_ID *
AcpiAhMatchHardwareId (
    char                    *Hid);

const char *
AcpiAhMatchUuid (
    UINT8                   *Data);

/*
 * utprint - printf/vprintf output functions
 */
const char *
AcpiUtScanNumber (
    const char              *String,
    UINT64                  *NumberPtr);

const char *
AcpiUtPrintNumber (
    char                    *String,
    UINT64                  Number);

int
AcpiUtVsnprintf (
    char                    *String,
    ACPI_SIZE               Size,
    const char              *Format,
    va_list                 Args);

int
AcpiUtSnprintf (
    char                    *String,
    ACPI_SIZE               Size,
    const char              *Format,
    ...);

#ifdef ACPI_APPLICATION
int
AcpiUtFileVprintf (
    ACPI_FILE               File,
    const char              *Format,
    va_list                 Args);

int
AcpiUtFilePrintf (
    ACPI_FILE               File,
    const char              *Format,
    ...);
#endif

/*
 * utuuid -- UUID support functions
 */
#if (defined ACPI_ASL_COMPILER || defined ACPI_EXEC_APP || defined ACPI_HELP_APP)
void
AcpiUtConvertStringToUuid (
    char                    *InString,
    UINT8                   *UuidBuffer);
#endif

#endif /* _ACUTILS_H */
