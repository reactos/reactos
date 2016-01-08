/******************************************************************************
 *
 * Name: acinterp.h - Interpreter subcomponent prototypes and defines
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
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

#ifndef __ACINTERP_H__
#define __ACINTERP_H__


#define ACPI_WALK_OPERANDS          (&(WalkState->Operands [WalkState->NumOperands -1]))

/* Macros for tables used for debug output */

#define ACPI_EXD_OFFSET(f)          (UINT8) ACPI_OFFSET (ACPI_OPERAND_OBJECT,f)
#define ACPI_EXD_NSOFFSET(f)        (UINT8) ACPI_OFFSET (ACPI_NAMESPACE_NODE,f)
#define ACPI_EXD_TABLE_SIZE(name)   (sizeof(name) / sizeof (ACPI_EXDUMP_INFO))

/*
 * If possible, pack the following structures to byte alignment, since we
 * don't care about performance for debug output. Two cases where we cannot
 * pack the structures:
 *
 * 1) Hardware does not support misaligned memory transfers
 * 2) Compiler does not support pointers within packed structures
 */
#if (!defined(ACPI_MISALIGNMENT_NOT_SUPPORTED) && !defined(ACPI_PACKED_POINTERS_NOT_SUPPORTED))
#pragma pack(1)
#endif

typedef const struct acpi_exdump_info
{
    UINT8                   Opcode;
    UINT8                   Offset;
    char                    *Name;

} ACPI_EXDUMP_INFO;

/* Values for the Opcode field above */

#define ACPI_EXD_INIT                   0
#define ACPI_EXD_TYPE                   1
#define ACPI_EXD_UINT8                  2
#define ACPI_EXD_UINT16                 3
#define ACPI_EXD_UINT32                 4
#define ACPI_EXD_UINT64                 5
#define ACPI_EXD_LITERAL                6
#define ACPI_EXD_POINTER                7
#define ACPI_EXD_ADDRESS                8
#define ACPI_EXD_STRING                 9
#define ACPI_EXD_BUFFER                 10
#define ACPI_EXD_PACKAGE                11
#define ACPI_EXD_FIELD                  12
#define ACPI_EXD_REFERENCE              13
#define ACPI_EXD_LIST                   14 /* Operand object list */
#define ACPI_EXD_HDLR_LIST              15 /* Address Handler list */
#define ACPI_EXD_RGN_LIST               16 /* Region list */
#define ACPI_EXD_NODE                   17 /* Namespace Node */

/* restore default alignment */

#pragma pack()


/*
 * exconvrt - object conversion
 */
ACPI_STATUS
AcpiExConvertToInteger (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  Flags);

ACPI_STATUS
AcpiExConvertToBuffer (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc);

ACPI_STATUS
AcpiExConvertToString (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    UINT32                  Type);

/* Types for ->String conversion */

#define ACPI_EXPLICIT_BYTE_COPY         0x00000000
#define ACPI_EXPLICIT_CONVERT_HEX       0x00000001
#define ACPI_IMPLICIT_CONVERT_HEX       0x00000002
#define ACPI_EXPLICIT_CONVERT_DECIMAL   0x00000003

ACPI_STATUS
AcpiExConvertToTargetType (
    ACPI_OBJECT_TYPE        DestinationType,
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc,
    ACPI_WALK_STATE         *WalkState);


/*
 * exdebug - AML debug object
 */
void
AcpiExDoDebugObject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    UINT32                  Level,
    UINT32                  Index);

void
AcpiExStartTraceMethod (
    ACPI_NAMESPACE_NODE     *MethodNode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

void
AcpiExStopTraceMethod (
    ACPI_NAMESPACE_NODE     *MethodNode,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

void
AcpiExStartTraceOpcode (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState);

void
AcpiExStopTraceOpcode (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_WALK_STATE         *WalkState);

void
AcpiExTracePoint (
    ACPI_TRACE_EVENT_TYPE   Type,
    BOOLEAN                 Begin,
    UINT8                   *Aml,
    char                    *Pathname);


/*
 * exfield - ACPI AML (p-code) execution - field manipulation
 */
ACPI_STATUS
AcpiExCommonBufferSetup (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  BufferLength,
    UINT32                  *DatumCount);

ACPI_STATUS
AcpiExWriteWithUpdateRule (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT64                  Mask,
    UINT64                  FieldValue,
    UINT32                  FieldDatumByteOffset);

void
AcpiExGetBufferDatum(
    UINT64                  *Datum,
    void                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  ByteGranularity,
    UINT32                  BufferOffset);

void
AcpiExSetBufferDatum (
    UINT64                  MergedDatum,
    void                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  ByteGranularity,
    UINT32                  BufferOffset);

ACPI_STATUS
AcpiExReadDataFromField (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **RetBufferDesc);

ACPI_STATUS
AcpiExWriteDataToField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ResultDesc);


/*
 * exfldio - low level field I/O
 */
ACPI_STATUS
AcpiExExtractFromField (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    void                    *Buffer,
    UINT32                  BufferLength);

ACPI_STATUS
AcpiExInsertIntoField (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    void                    *Buffer,
    UINT32                  BufferLength);

ACPI_STATUS
AcpiExAccessRegion (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  FieldDatumByteOffset,
    UINT64                  *Value,
    UINT32                  ReadWrite);


/*
 * exmisc - misc support routines
 */
ACPI_STATUS
AcpiExGetObjectReference (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **ReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExConcatTemplate (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc2,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExDoConcatenate (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc2,
    ACPI_OPERAND_OBJECT     **ActualReturnDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExDoLogicalNumericOp (
    UINT16                  Opcode,
    UINT64                  Integer0,
    UINT64                  Integer1,
    BOOLEAN                 *LogicalResult);

ACPI_STATUS
AcpiExDoLogicalOp (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     *Operand0,
    ACPI_OPERAND_OBJECT     *Operand1,
    BOOLEAN                 *LogicalResult);

UINT64
AcpiExDoMathOp (
    UINT16                  Opcode,
    UINT64                  Operand0,
    UINT64                  Operand1);

ACPI_STATUS
AcpiExCreateMutex (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateProcessor (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreatePowerResource (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateRegion (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    UINT8                   RegionSpace,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateEvent (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateAlias (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExCreateMethod (
    UINT8                   *AmlStart,
    UINT32                  AmlLength,
    ACPI_WALK_STATE         *WalkState);


/*
 * exconfig - dynamic table load/unload
 */
ACPI_STATUS
AcpiExLoadOp (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     *Target,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExLoadTableOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     **ReturnDesc);

ACPI_STATUS
AcpiExUnloadTable (
    ACPI_OPERAND_OBJECT     *DdbHandle);


/*
 * exmutex - mutex support
 */
ACPI_STATUS
AcpiExAcquireMutex (
    ACPI_OPERAND_OBJECT     *TimeDesc,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExAcquireMutexObject (
    UINT16                  Timeout,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_THREAD_ID          ThreadId);

ACPI_STATUS
AcpiExReleaseMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExReleaseMutexObject (
    ACPI_OPERAND_OBJECT     *ObjDesc);

void
AcpiExReleaseAllMutexes (
    ACPI_THREAD_STATE       *Thread);

void
AcpiExUnlinkMutex (
    ACPI_OPERAND_OBJECT     *ObjDesc);


/*
 * exprep - ACPI AML execution - prep utilities
 */
ACPI_STATUS
AcpiExPrepCommonFieldObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT8                   FieldFlags,
    UINT8                   FieldAttribute,
    UINT32                  FieldBitPosition,
    UINT32                  FieldBitLength);

ACPI_STATUS
AcpiExPrepFieldValue (
    ACPI_CREATE_FIELD_INFO  *Info);


/*
 * exsystem - Interface to OS services
 */
ACPI_STATUS
AcpiExSystemDoNotifyOp (
    ACPI_OPERAND_OBJECT     *Value,
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemDoSleep(
    UINT64                  Time);

ACPI_STATUS
AcpiExSystemDoStall (
    UINT32                  Time);

ACPI_STATUS
AcpiExSystemSignalEvent(
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemWaitEvent(
    ACPI_OPERAND_OBJECT     *Time,
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemResetEvent(
    ACPI_OPERAND_OBJECT     *ObjDesc);

ACPI_STATUS
AcpiExSystemWaitSemaphore (
    ACPI_SEMAPHORE          Semaphore,
    UINT16                  Timeout);

ACPI_STATUS
AcpiExSystemWaitMutex (
    ACPI_MUTEX              Mutex,
    UINT16                  Timeout);

/*
 * exoparg1 - ACPI AML execution, 1 operand
 */
ACPI_STATUS
AcpiExOpcode_0A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_1T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_1A_1T_0R (
    ACPI_WALK_STATE         *WalkState);

/*
 * exoparg2 - ACPI AML execution, 2 operands
 */
ACPI_STATUS
AcpiExOpcode_2A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_0T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_1T_1R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_2A_2T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exoparg3 - ACPI AML execution, 3 operands
 */
ACPI_STATUS
AcpiExOpcode_3A_0T_0R (
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExOpcode_3A_1T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exoparg6 - ACPI AML execution, 6 operands
 */
ACPI_STATUS
AcpiExOpcode_6A_0T_1R (
    ACPI_WALK_STATE         *WalkState);


/*
 * exresolv - Object resolution and get value functions
 */
ACPI_STATUS
AcpiExResolveToValue (
    ACPI_OPERAND_OBJECT     **StackPtr,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExResolveMultiple (
    ACPI_WALK_STATE         *WalkState,
    ACPI_OPERAND_OBJECT     *Operand,
    ACPI_OBJECT_TYPE        *ReturnType,
    ACPI_OPERAND_OBJECT     **ReturnDesc);


/*
 * exresnte - resolve namespace node
 */
ACPI_STATUS
AcpiExResolveNodeToValue (
    ACPI_NAMESPACE_NODE     **StackPtr,
    ACPI_WALK_STATE         *WalkState);


/*
 * exresop - resolve operand to value
 */
ACPI_STATUS
AcpiExResolveOperands (
    UINT16                  Opcode,
    ACPI_OPERAND_OBJECT     **StackPtr,
    ACPI_WALK_STATE         *WalkState);


/*
 * exdump - Interpreter debug output routines
 */
void
AcpiExDumpOperand (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Depth);

void
AcpiExDumpOperands (
    ACPI_OPERAND_OBJECT     **Operands,
    const char              *OpcodeName,
    UINT32                  NumOpcodes);

void
AcpiExDumpObjectDescriptor (
    ACPI_OPERAND_OBJECT     *Object,
    UINT32                  Flags);

void
AcpiExDumpNamespaceNode (
    ACPI_NAMESPACE_NODE     *Node,
    UINT32                  Flags);


/*
 * exnames - AML namestring support
 */
ACPI_STATUS
AcpiExGetNameString (
    ACPI_OBJECT_TYPE        DataType,
    UINT8                   *InAmlAddress,
    char                    **OutNameString,
    UINT32                  *OutNameLength);


/*
 * exstore - Object store support
 */
ACPI_STATUS
AcpiExStore (
    ACPI_OPERAND_OBJECT     *ValDesc,
    ACPI_OPERAND_OBJECT     *DestDesc,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExStoreObjectToNode (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_NAMESPACE_NODE     *Node,
    ACPI_WALK_STATE         *WalkState,
    UINT8                   ImplicitConversion);

#define ACPI_IMPLICIT_CONVERSION        TRUE
#define ACPI_NO_IMPLICIT_CONVERSION     FALSE


/*
 * exstoren - resolve/store object
 */
ACPI_STATUS
AcpiExResolveObject (
    ACPI_OPERAND_OBJECT     **SourceDescPtr,
    ACPI_OBJECT_TYPE        TargetType,
    ACPI_WALK_STATE         *WalkState);

ACPI_STATUS
AcpiExStoreObjectToObject (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *DestDesc,
    ACPI_OPERAND_OBJECT     **NewDesc,
    ACPI_WALK_STATE         *WalkState);


/*
 * exstorob - store object - buffer/string
 */
ACPI_STATUS
AcpiExStoreBufferToBuffer (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExStoreStringToString (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);


/*
 * excopy - object copy
 */
ACPI_STATUS
AcpiExCopyIntegerToIndexField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExCopyIntegerToBankField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);

ACPI_STATUS
AcpiExCopyDataToNamedField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_NAMESPACE_NODE     *Node);

ACPI_STATUS
AcpiExCopyIntegerToBufferField (
    ACPI_OPERAND_OBJECT     *SourceDesc,
    ACPI_OPERAND_OBJECT     *TargetDesc);


/*
 * exutils - interpreter/scanner utilities
 */
void
AcpiExEnterInterpreter (
    void);

void
AcpiExExitInterpreter (
    void);

BOOLEAN
AcpiExTruncateFor32bitTable (
    ACPI_OPERAND_OBJECT     *ObjDesc);

void
AcpiExAcquireGlobalLock (
    UINT32                  Rule);

void
AcpiExReleaseGlobalLock (
    UINT32                  Rule);

void
AcpiExEisaIdToString (
    char                    *Dest,
    UINT64                  CompressedId);

void
AcpiExIntegerToString (
    char                    *Dest,
    UINT64                  Value);

void
AcpiExPciClsToString (
    char                    *Dest,
    UINT8                   ClassCode[3]);

BOOLEAN
AcpiIsValidSpaceId (
    UINT8                   SpaceId);


/*
 * exregion - default OpRegion handlers
 */
ACPI_STATUS
AcpiExSystemMemorySpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExSystemIoSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExPciConfigSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExCmosSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExPciBarSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExEmbeddedControllerSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

ACPI_STATUS
AcpiExSmBusSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);


ACPI_STATUS
AcpiExDataTableSpaceHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

#endif /* __INTERP_H__ */
