/******************************************************************************
 *
 * Name: acstruct.h - Internal structs
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

#ifndef __ACSTRUCT_H__
#define __ACSTRUCT_H__

/* acpisrc:StructDefs -- for acpisrc conversion */

/*****************************************************************************
 *
 * Tree walking typedefs and structs
 *
 ****************************************************************************/


/*
 * Walk state - current state of a parse tree walk. Used for both a leisurely
 * stroll through the tree (for whatever reason), and for control method
 * execution.
 */
#define ACPI_NEXT_OP_DOWNWARD       1
#define ACPI_NEXT_OP_UPWARD         2

/*
 * Groups of definitions for WalkType used for different implementations of
 * walkers (never simultaneously) - flags for interpreter:
 */
#define ACPI_WALK_NON_METHOD        0
#define ACPI_WALK_METHOD            0x01
#define ACPI_WALK_METHOD_RESTART    0x02


typedef struct acpi_walk_state
{
    struct acpi_walk_state          *Next;              /* Next WalkState in list */
    UINT8                           DescriptorType;     /* To differentiate various internal objs */
    UINT8                           WalkType;
    UINT16                          Opcode;             /* Current AML opcode */
    UINT8                           NextOpInfo;         /* Info about NextOp */
    UINT8                           NumOperands;        /* Stack pointer for Operands[] array */
    UINT8                           OperandIndex;       /* Index into operand stack, to be used by AcpiDsObjStackPush */
    ACPI_OWNER_ID                   OwnerId;            /* Owner of objects created during the walk */
    BOOLEAN                         LastPredicate;      /* Result of last predicate */
    UINT8                           CurrentResult;
    UINT8                           ReturnUsed;
    UINT8                           ScopeDepth;
    UINT8                           PassNumber;         /* Parse pass during table load */
    BOOLEAN                         NamespaceOverride;  /* Override existing objects */
    UINT8                           ResultSize;         /* Total elements for the result stack */
    UINT8                           ResultCount;        /* Current number of occupied elements of result stack */
    UINT8                           *Aml;
    UINT32                          ArgTypes;
    UINT32                          MethodBreakpoint;   /* For single stepping */
    UINT32                          UserBreakpoint;     /* User AML breakpoint */
    UINT32                          ParseFlags;

    ACPI_PARSE_STATE                ParserState;        /* Current state of parser */
    UINT32                          PrevArgTypes;
    UINT32                          ArgCount;           /* push for fixed or var args */
    UINT16                          MethodNestingDepth;
    UINT8                           MethodIsNested;

    struct acpi_namespace_node      Arguments[ACPI_METHOD_NUM_ARGS];        /* Control method arguments */
    struct acpi_namespace_node      LocalVariables[ACPI_METHOD_NUM_LOCALS]; /* Control method locals */
    union acpi_operand_object       *Operands[ACPI_OBJ_NUM_OPERANDS + 1];   /* Operands passed to the interpreter (+1 for NULL terminator) */
    union acpi_operand_object       **Params;

    UINT8                           *AmlLastWhile;
    union acpi_operand_object       **CallerReturnDesc;
    ACPI_GENERIC_STATE              *ControlState;      /* List of control states (nested IFs) */
    struct acpi_namespace_node      *DeferredNode;      /* Used when executing deferred opcodes */
    union acpi_operand_object       *ImplicitReturnObj;
    struct acpi_namespace_node      *MethodCallNode;    /* Called method Node*/
    ACPI_PARSE_OBJECT               *MethodCallOp;      /* MethodCall Op if running a method */
    union acpi_operand_object       *MethodDesc;        /* Method descriptor if running a method */
    struct acpi_namespace_node      *MethodNode;        /* Method node if running a method */
    char                            *MethodPathname;    /* Full pathname of running method */
    ACPI_PARSE_OBJECT               *Op;                /* Current parser op */
    const ACPI_OPCODE_INFO          *OpInfo;            /* Info on current opcode */
    ACPI_PARSE_OBJECT               *Origin;            /* Start of walk [Obsolete] */
    union acpi_operand_object       *ResultObj;
    ACPI_GENERIC_STATE              *Results;           /* Stack of accumulated results */
    union acpi_operand_object       *ReturnDesc;        /* Return object, if any */
    ACPI_GENERIC_STATE              *ScopeInfo;         /* Stack of nested scopes */
    ACPI_PARSE_OBJECT               *PrevOp;            /* Last op that was processed */
    ACPI_PARSE_OBJECT               *NextOp;            /* next op to be processed */
    ACPI_THREAD_STATE               *Thread;
    ACPI_PARSE_DOWNWARDS            DescendingCallback;
    ACPI_PARSE_UPWARDS              AscendingCallback;

} ACPI_WALK_STATE;


/* Info used by AcpiNsInitializeObjects and AcpiDsInitializeObjects */

typedef struct acpi_init_walk_info
{
    UINT32                          TableIndex;
    UINT32                          ObjectCount;
    UINT32                          MethodCount;
    UINT32                          SerialMethodCount;
    UINT32                          NonSerialMethodCount;
    UINT32                          SerializedMethodCount;
    UINT32                          DeviceCount;
    UINT32                          OpRegionCount;
    UINT32                          FieldCount;
    UINT32                          BufferCount;
    UINT32                          PackageCount;
    UINT32                          OpRegionInit;
    UINT32                          FieldInit;
    UINT32                          BufferInit;
    UINT32                          PackageInit;
    ACPI_OWNER_ID                   OwnerId;

} ACPI_INIT_WALK_INFO;


typedef struct acpi_get_devices_info
{
    ACPI_WALK_CALLBACK              UserFunction;
    void                            *Context;
    char                            *Hid;

} ACPI_GET_DEVICES_INFO;


typedef union acpi_aml_operands
{
    ACPI_OPERAND_OBJECT             *Operands[7];

    struct
    {
        ACPI_OBJECT_INTEGER             *Type;
        ACPI_OBJECT_INTEGER             *Code;
        ACPI_OBJECT_INTEGER             *Argument;

    } Fatal;

    struct
    {
        ACPI_OPERAND_OBJECT             *Source;
        ACPI_OBJECT_INTEGER             *Index;
        ACPI_OPERAND_OBJECT             *Target;

    } Index;

    struct
    {
        ACPI_OPERAND_OBJECT             *Source;
        ACPI_OBJECT_INTEGER             *Index;
        ACPI_OBJECT_INTEGER             *Length;
        ACPI_OPERAND_OBJECT             *Target;

    } Mid;

} ACPI_AML_OPERANDS;


/*
 * Structure used to pass object evaluation information and parameters.
 * Purpose is to reduce CPU stack use.
 */
typedef struct acpi_evaluate_info
{
    /* The first 3 elements are passed by the caller to AcpiNsEvaluate */

    ACPI_NAMESPACE_NODE             *PrefixNode;        /* Input: starting node */
    const char                      *RelativePathname;  /* Input: path relative to PrefixNode */
    ACPI_OPERAND_OBJECT             **Parameters;       /* Input: argument list */

    ACPI_NAMESPACE_NODE             *Node;              /* Resolved node (PrefixNode:RelativePathname) */
    ACPI_OPERAND_OBJECT             *ObjDesc;           /* Object attached to the resolved node */
    char                            *FullPathname;      /* Full pathname of the resolved node */

    const ACPI_PREDEFINED_INFO      *Predefined;        /* Used if Node is a predefined name */
    ACPI_OPERAND_OBJECT             *ReturnObject;      /* Object returned from the evaluation */
    union acpi_operand_object       *ParentPackage;     /* Used if return object is a Package */

    UINT32                          ReturnFlags;        /* Used for return value analysis */
    UINT32                          ReturnBtype;        /* Bitmapped type of the returned object */
    UINT16                          ParamCount;         /* Count of the input argument list */
    UINT16                          NodeFlags;          /* Same as Node->Flags */
    UINT8                           PassNumber;         /* Parser pass number */
    UINT8                           ReturnObjectType;   /* Object type of the returned object */
    UINT8                           Flags;              /* General flags */

} ACPI_EVALUATE_INFO;

/* Values for Flags above */

#define ACPI_IGNORE_RETURN_VALUE    1

/* Defines for ReturnFlags field above */

#define ACPI_OBJECT_REPAIRED        1
#define ACPI_OBJECT_WRAPPED         2


/* Info used by AcpiNsInitializeDevices */

typedef struct acpi_device_walk_info
{
    ACPI_TABLE_DESC                 *TableDesc;
    ACPI_EVALUATE_INFO              *EvaluateInfo;
    UINT32                          DeviceCount;
    UINT32                          Num_STA;
    UINT32                          Num_INI;

} ACPI_DEVICE_WALK_INFO;


/* Info used by Acpi  AcpiDbDisplayFields */

typedef struct acpi_region_walk_info
{
    UINT32                          DebugLevel;
    UINT32                          Count;
    ACPI_OWNER_ID                   OwnerId;
    UINT8                           DisplayType;
    UINT32                          AddressSpaceId;

} ACPI_REGION_WALK_INFO;


/* TBD: [Restructure] Merge with struct above */

typedef struct acpi_walk_info
{
    UINT32                          DebugLevel;
    UINT32                          Count;
    ACPI_OWNER_ID                   OwnerId;
    UINT8                           DisplayType;

} ACPI_WALK_INFO;

/* Display Types */

#define ACPI_DISPLAY_SUMMARY        (UINT8) 0
#define ACPI_DISPLAY_OBJECTS        (UINT8) 1
#define ACPI_DISPLAY_MASK           (UINT8) 1

#define ACPI_DISPLAY_SHORT          (UINT8) 2


#endif
