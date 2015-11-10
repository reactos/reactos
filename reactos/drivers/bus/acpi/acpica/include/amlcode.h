/******************************************************************************
 *
 * Name: amlcode.h - Definitions for AML, as included in "definition blocks"
 *                   Declarations and definitions contained herein are derived
 *                   directly from the ACPI specification.
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

#ifndef __AMLCODE_H__
#define __AMLCODE_H__

/* primary opcodes */

#define AML_NULL_CHAR               (UINT16) 0x00

#define AML_ZERO_OP                 (UINT16) 0x00
#define AML_ONE_OP                  (UINT16) 0x01
#define AML_UNASSIGNED              (UINT16) 0x02
#define AML_ALIAS_OP                (UINT16) 0x06
#define AML_NAME_OP                 (UINT16) 0x08
#define AML_BYTE_OP                 (UINT16) 0x0a
#define AML_WORD_OP                 (UINT16) 0x0b
#define AML_DWORD_OP                (UINT16) 0x0c
#define AML_STRING_OP               (UINT16) 0x0d
#define AML_QWORD_OP                (UINT16) 0x0e     /* ACPI 2.0 */
#define AML_SCOPE_OP                (UINT16) 0x10
#define AML_BUFFER_OP               (UINT16) 0x11
#define AML_PACKAGE_OP              (UINT16) 0x12
#define AML_VAR_PACKAGE_OP          (UINT16) 0x13     /* ACPI 2.0 */
#define AML_METHOD_OP               (UINT16) 0x14
#define AML_EXTERNAL_OP             (UINT16) 0x15     /* ACPI 6.0 */
#define AML_DUAL_NAME_PREFIX        (UINT16) 0x2e
#define AML_MULTI_NAME_PREFIX_OP    (UINT16) 0x2f
#define AML_NAME_CHAR_SUBSEQ        (UINT16) 0x30
#define AML_NAME_CHAR_FIRST         (UINT16) 0x41
#define AML_EXTENDED_OP_PREFIX      (UINT16) 0x5b
#define AML_ROOT_PREFIX             (UINT16) 0x5c
#define AML_PARENT_PREFIX           (UINT16) 0x5e
#define AML_LOCAL_OP                (UINT16) 0x60
#define AML_LOCAL0                  (UINT16) 0x60
#define AML_LOCAL1                  (UINT16) 0x61
#define AML_LOCAL2                  (UINT16) 0x62
#define AML_LOCAL3                  (UINT16) 0x63
#define AML_LOCAL4                  (UINT16) 0x64
#define AML_LOCAL5                  (UINT16) 0x65
#define AML_LOCAL6                  (UINT16) 0x66
#define AML_LOCAL7                  (UINT16) 0x67
#define AML_ARG_OP                  (UINT16) 0x68
#define AML_ARG0                    (UINT16) 0x68
#define AML_ARG1                    (UINT16) 0x69
#define AML_ARG2                    (UINT16) 0x6a
#define AML_ARG3                    (UINT16) 0x6b
#define AML_ARG4                    (UINT16) 0x6c
#define AML_ARG5                    (UINT16) 0x6d
#define AML_ARG6                    (UINT16) 0x6e
#define AML_STORE_OP                (UINT16) 0x70
#define AML_REF_OF_OP               (UINT16) 0x71
#define AML_ADD_OP                  (UINT16) 0x72
#define AML_CONCAT_OP               (UINT16) 0x73
#define AML_SUBTRACT_OP             (UINT16) 0x74
#define AML_INCREMENT_OP            (UINT16) 0x75
#define AML_DECREMENT_OP            (UINT16) 0x76
#define AML_MULTIPLY_OP             (UINT16) 0x77
#define AML_DIVIDE_OP               (UINT16) 0x78
#define AML_SHIFT_LEFT_OP           (UINT16) 0x79
#define AML_SHIFT_RIGHT_OP          (UINT16) 0x7a
#define AML_BIT_AND_OP              (UINT16) 0x7b
#define AML_BIT_NAND_OP             (UINT16) 0x7c
#define AML_BIT_OR_OP               (UINT16) 0x7d
#define AML_BIT_NOR_OP              (UINT16) 0x7e
#define AML_BIT_XOR_OP              (UINT16) 0x7f
#define AML_BIT_NOT_OP              (UINT16) 0x80
#define AML_FIND_SET_LEFT_BIT_OP    (UINT16) 0x81
#define AML_FIND_SET_RIGHT_BIT_OP   (UINT16) 0x82
#define AML_DEREF_OF_OP             (UINT16) 0x83
#define AML_CONCAT_RES_OP           (UINT16) 0x84     /* ACPI 2.0 */
#define AML_MOD_OP                  (UINT16) 0x85     /* ACPI 2.0 */
#define AML_NOTIFY_OP               (UINT16) 0x86
#define AML_SIZE_OF_OP              (UINT16) 0x87
#define AML_INDEX_OP                (UINT16) 0x88
#define AML_MATCH_OP                (UINT16) 0x89
#define AML_CREATE_DWORD_FIELD_OP   (UINT16) 0x8a
#define AML_CREATE_WORD_FIELD_OP    (UINT16) 0x8b
#define AML_CREATE_BYTE_FIELD_OP    (UINT16) 0x8c
#define AML_CREATE_BIT_FIELD_OP     (UINT16) 0x8d
#define AML_TYPE_OP                 (UINT16) 0x8e
#define AML_CREATE_QWORD_FIELD_OP   (UINT16) 0x8f     /* ACPI 2.0 */
#define AML_LAND_OP                 (UINT16) 0x90
#define AML_LOR_OP                  (UINT16) 0x91
#define AML_LNOT_OP                 (UINT16) 0x92
#define AML_LEQUAL_OP               (UINT16) 0x93
#define AML_LGREATER_OP             (UINT16) 0x94
#define AML_LLESS_OP                (UINT16) 0x95
#define AML_TO_BUFFER_OP            (UINT16) 0x96     /* ACPI 2.0 */
#define AML_TO_DECSTRING_OP         (UINT16) 0x97     /* ACPI 2.0 */
#define AML_TO_HEXSTRING_OP         (UINT16) 0x98     /* ACPI 2.0 */
#define AML_TO_INTEGER_OP           (UINT16) 0x99     /* ACPI 2.0 */
#define AML_TO_STRING_OP            (UINT16) 0x9c     /* ACPI 2.0 */
#define AML_COPY_OP                 (UINT16) 0x9d     /* ACPI 2.0 */
#define AML_MID_OP                  (UINT16) 0x9e     /* ACPI 2.0 */
#define AML_CONTINUE_OP             (UINT16) 0x9f     /* ACPI 2.0 */
#define AML_IF_OP                   (UINT16) 0xa0
#define AML_ELSE_OP                 (UINT16) 0xa1
#define AML_WHILE_OP                (UINT16) 0xa2
#define AML_NOOP_OP                 (UINT16) 0xa3
#define AML_RETURN_OP               (UINT16) 0xa4
#define AML_BREAK_OP                (UINT16) 0xa5
#define AML_BREAK_POINT_OP          (UINT16) 0xcc
#define AML_ONES_OP                 (UINT16) 0xff

/* prefixed opcodes */

#define AML_EXTENDED_OPCODE         (UINT16) 0x5b00     /* prefix for 2-byte opcodes */

#define AML_MUTEX_OP                (UINT16) 0x5b01
#define AML_EVENT_OP                (UINT16) 0x5b02
#define AML_SHIFT_RIGHT_BIT_OP      (UINT16) 0x5b10
#define AML_SHIFT_LEFT_BIT_OP       (UINT16) 0x5b11
#define AML_COND_REF_OF_OP          (UINT16) 0x5b12
#define AML_CREATE_FIELD_OP         (UINT16) 0x5b13
#define AML_LOAD_TABLE_OP           (UINT16) 0x5b1f     /* ACPI 2.0 */
#define AML_LOAD_OP                 (UINT16) 0x5b20
#define AML_STALL_OP                (UINT16) 0x5b21
#define AML_SLEEP_OP                (UINT16) 0x5b22
#define AML_ACQUIRE_OP              (UINT16) 0x5b23
#define AML_SIGNAL_OP               (UINT16) 0x5b24
#define AML_WAIT_OP                 (UINT16) 0x5b25
#define AML_RESET_OP                (UINT16) 0x5b26
#define AML_RELEASE_OP              (UINT16) 0x5b27
#define AML_FROM_BCD_OP             (UINT16) 0x5b28
#define AML_TO_BCD_OP               (UINT16) 0x5b29
#define AML_UNLOAD_OP               (UINT16) 0x5b2a
#define AML_REVISION_OP             (UINT16) 0x5b30
#define AML_DEBUG_OP                (UINT16) 0x5b31
#define AML_FATAL_OP                (UINT16) 0x5b32
#define AML_TIMER_OP                (UINT16) 0x5b33     /* ACPI 3.0 */
#define AML_REGION_OP               (UINT16) 0x5b80
#define AML_FIELD_OP                (UINT16) 0x5b81
#define AML_DEVICE_OP               (UINT16) 0x5b82
#define AML_PROCESSOR_OP            (UINT16) 0x5b83
#define AML_POWER_RES_OP            (UINT16) 0x5b84
#define AML_THERMAL_ZONE_OP         (UINT16) 0x5b85
#define AML_INDEX_FIELD_OP          (UINT16) 0x5b86
#define AML_BANK_FIELD_OP           (UINT16) 0x5b87
#define AML_DATA_REGION_OP          (UINT16) 0x5b88     /* ACPI 2.0 */


/*
 * Combination opcodes (actually two one-byte opcodes)
 * Used by the disassembler and iASL compiler
 */
#define AML_LGREATEREQUAL_OP        (UINT16) 0x9295
#define AML_LLESSEQUAL_OP           (UINT16) 0x9294
#define AML_LNOTEQUAL_OP            (UINT16) 0x9293


/*
 * Opcodes for "Field" operators
 */
#define AML_FIELD_OFFSET_OP         (UINT8) 0x00
#define AML_FIELD_ACCESS_OP         (UINT8) 0x01
#define AML_FIELD_CONNECTION_OP     (UINT8) 0x02        /* ACPI 5.0 */
#define AML_FIELD_EXT_ACCESS_OP     (UINT8) 0x03        /* ACPI 5.0 */


/*
 * Internal opcodes
 * Use only "Unknown" AML opcodes, don't attempt to use
 * any valid ACPI ASCII values (A-Z, 0-9, '-')
 */
#define AML_INT_NAMEPATH_OP         (UINT16) 0x002d
#define AML_INT_NAMEDFIELD_OP       (UINT16) 0x0030
#define AML_INT_RESERVEDFIELD_OP    (UINT16) 0x0031
#define AML_INT_ACCESSFIELD_OP      (UINT16) 0x0032
#define AML_INT_BYTELIST_OP         (UINT16) 0x0033
#define AML_INT_METHODCALL_OP       (UINT16) 0x0035
#define AML_INT_RETURN_VALUE_OP     (UINT16) 0x0036
#define AML_INT_EVAL_SUBTREE_OP     (UINT16) 0x0037
#define AML_INT_CONNECTION_OP       (UINT16) 0x0038
#define AML_INT_EXTACCESSFIELD_OP   (UINT16) 0x0039

#define ARG_NONE                    0x0

/*
 * Argument types for the AML Parser
 * Each field in the ArgTypes UINT32 is 5 bits, allowing for a maximum of 6 arguments.
 * There can be up to 31 unique argument types
 * Zero is reserved as end-of-list indicator
 */
#define ARGP_BYTEDATA               0x01
#define ARGP_BYTELIST               0x02
#define ARGP_CHARLIST               0x03
#define ARGP_DATAOBJ                0x04
#define ARGP_DATAOBJLIST            0x05
#define ARGP_DWORDDATA              0x06
#define ARGP_FIELDLIST              0x07
#define ARGP_NAME                   0x08
#define ARGP_NAMESTRING             0x09
#define ARGP_OBJLIST                0x0A
#define ARGP_PKGLENGTH              0x0B
#define ARGP_SUPERNAME              0x0C
#define ARGP_TARGET                 0x0D
#define ARGP_TERMARG                0x0E
#define ARGP_TERMLIST               0x0F
#define ARGP_WORDDATA               0x10
#define ARGP_QWORDDATA              0x11
#define ARGP_SIMPLENAME             0x12

/*
 * Resolved argument types for the AML Interpreter
 * Each field in the ArgTypes UINT32 is 5 bits, allowing for a maximum of 6 arguments.
 * There can be up to 31 unique argument types (0 is end-of-arg-list indicator)
 *
 * Note1: These values are completely independent from the ACPI_TYPEs
 *        i.e., ARGI_INTEGER != ACPI_TYPE_INTEGER
 *
 * Note2: If and when 5 bits becomes insufficient, it would probably be best
 * to convert to a 6-byte array of argument types, allowing 8 bits per argument.
 */

/* Single, simple types */

#define ARGI_ANYTYPE                0x01    /* Don't care */
#define ARGI_PACKAGE                0x02
#define ARGI_EVENT                  0x03
#define ARGI_MUTEX                  0x04
#define ARGI_DDBHANDLE              0x05

/* Interchangeable types (via implicit conversion) */

#define ARGI_INTEGER                0x06
#define ARGI_STRING                 0x07
#define ARGI_BUFFER                 0x08
#define ARGI_BUFFER_OR_STRING       0x09    /* Used by MID op only */
#define ARGI_COMPUTEDATA            0x0A    /* Buffer, String, or Integer */

/* Reference objects */

#define ARGI_INTEGER_REF            0x0B
#define ARGI_OBJECT_REF             0x0C
#define ARGI_DEVICE_REF             0x0D
#define ARGI_REFERENCE              0x0E
#define ARGI_TARGETREF              0x0F    /* Target, subject to implicit conversion */
#define ARGI_FIXED_TARGET           0x10    /* Target, no implicit conversion */
#define ARGI_SIMPLE_TARGET          0x11    /* Name, Local, Arg -- no implicit conversion */

/* Multiple/complex types */

#define ARGI_DATAOBJECT             0x12    /* Buffer, String, package or reference to a Node - Used only by SizeOf operator*/
#define ARGI_COMPLEXOBJ             0x13    /* Buffer, String, or package (Used by INDEX op only) */
#define ARGI_REF_OR_STRING          0x14    /* Reference or String (Used by DEREFOF op only) */
#define ARGI_REGION_OR_BUFFER       0x15    /* Used by LOAD op only */
#define ARGI_DATAREFOBJ             0x16

/* Note: types above can expand to 0x1F maximum */

#define ARGI_INVALID_OPCODE         0xFFFFFFFF


/*
 * hash offsets
 */
#define AML_EXTOP_HASH_OFFSET       22
#define AML_LNOT_HASH_OFFSET        19


/*
 * opcode groups and types
 */
#define OPGRP_NAMED                 0x01
#define OPGRP_FIELD                 0x02
#define OPGRP_BYTELIST              0x04


/*
 * Opcode information
 */

/* Opcode flags */

#define AML_LOGICAL                 0x0001
#define AML_LOGICAL_NUMERIC         0x0002
#define AML_MATH                    0x0004
#define AML_CREATE                  0x0008
#define AML_FIELD                   0x0010
#define AML_DEFER                   0x0020
#define AML_NAMED                   0x0040
#define AML_NSNODE                  0x0080
#define AML_NSOPCODE                0x0100
#define AML_NSOBJECT                0x0200
#define AML_HAS_RETVAL              0x0400
#define AML_HAS_TARGET              0x0800
#define AML_HAS_ARGS                0x1000
#define AML_CONSTANT                0x2000
#define AML_NO_OPERAND_RESOLVE      0x4000

/* Convenient flag groupings */

#define AML_FLAGS_EXEC_0A_0T_1R                                     AML_HAS_RETVAL
#define AML_FLAGS_EXEC_1A_0T_0R     AML_HAS_ARGS                                   /* Monadic1  */
#define AML_FLAGS_EXEC_1A_0T_1R     AML_HAS_ARGS |                  AML_HAS_RETVAL /* Monadic2  */
#define AML_FLAGS_EXEC_1A_1T_0R     AML_HAS_ARGS | AML_HAS_TARGET
#define AML_FLAGS_EXEC_1A_1T_1R     AML_HAS_ARGS | AML_HAS_TARGET | AML_HAS_RETVAL /* Monadic2R */
#define AML_FLAGS_EXEC_2A_0T_0R     AML_HAS_ARGS                                   /* Dyadic1   */
#define AML_FLAGS_EXEC_2A_0T_1R     AML_HAS_ARGS |                  AML_HAS_RETVAL /* Dyadic2   */
#define AML_FLAGS_EXEC_2A_1T_1R     AML_HAS_ARGS | AML_HAS_TARGET | AML_HAS_RETVAL /* Dyadic2R  */
#define AML_FLAGS_EXEC_2A_2T_1R     AML_HAS_ARGS | AML_HAS_TARGET | AML_HAS_RETVAL
#define AML_FLAGS_EXEC_3A_0T_0R     AML_HAS_ARGS
#define AML_FLAGS_EXEC_3A_1T_1R     AML_HAS_ARGS | AML_HAS_TARGET | AML_HAS_RETVAL
#define AML_FLAGS_EXEC_6A_0T_1R     AML_HAS_ARGS |                  AML_HAS_RETVAL


/*
 * The opcode Type is used in a dispatch table, do not change
 * without updating the table.
 */
#define AML_TYPE_EXEC_0A_0T_1R      0x00
#define AML_TYPE_EXEC_1A_0T_0R      0x01 /* Monadic1  */
#define AML_TYPE_EXEC_1A_0T_1R      0x02 /* Monadic2  */
#define AML_TYPE_EXEC_1A_1T_0R      0x03
#define AML_TYPE_EXEC_1A_1T_1R      0x04 /* Monadic2R */
#define AML_TYPE_EXEC_2A_0T_0R      0x05 /* Dyadic1   */
#define AML_TYPE_EXEC_2A_0T_1R      0x06 /* Dyadic2   */
#define AML_TYPE_EXEC_2A_1T_1R      0x07 /* Dyadic2R  */
#define AML_TYPE_EXEC_2A_2T_1R      0x08
#define AML_TYPE_EXEC_3A_0T_0R      0x09
#define AML_TYPE_EXEC_3A_1T_1R      0x0A
#define AML_TYPE_EXEC_6A_0T_1R      0x0B
/* End of types used in dispatch table */

#define AML_TYPE_LITERAL            0x0B
#define AML_TYPE_CONSTANT           0x0C
#define AML_TYPE_METHOD_ARGUMENT    0x0D
#define AML_TYPE_LOCAL_VARIABLE     0x0E
#define AML_TYPE_DATA_TERM          0x0F

/* Generic for an op that returns a value */

#define AML_TYPE_METHOD_CALL        0x10

/* Misc */

#define AML_TYPE_CREATE_FIELD       0x11
#define AML_TYPE_CREATE_OBJECT      0x12
#define AML_TYPE_CONTROL            0x13
#define AML_TYPE_NAMED_NO_OBJ       0x14
#define AML_TYPE_NAMED_FIELD        0x15
#define AML_TYPE_NAMED_SIMPLE       0x16
#define AML_TYPE_NAMED_COMPLEX      0x17
#define AML_TYPE_RETURN             0x18

#define AML_TYPE_UNDEFINED          0x19
#define AML_TYPE_BOGUS              0x1A

/* AML Package Length encodings */

#define ACPI_AML_PACKAGE_TYPE1      0x40
#define ACPI_AML_PACKAGE_TYPE2      0x4000
#define ACPI_AML_PACKAGE_TYPE3      0x400000
#define ACPI_AML_PACKAGE_TYPE4      0x40000000

/*
 * Opcode classes
 */
#define AML_CLASS_EXECUTE           0x00
#define AML_CLASS_CREATE            0x01
#define AML_CLASS_ARGUMENT          0x02
#define AML_CLASS_NAMED_OBJECT      0x03
#define AML_CLASS_CONTROL           0x04
#define AML_CLASS_ASCII             0x05
#define AML_CLASS_PREFIX            0x06
#define AML_CLASS_INTERNAL          0x07
#define AML_CLASS_RETURN_VALUE      0x08
#define AML_CLASS_METHOD_CALL       0x09
#define AML_CLASS_UNKNOWN           0x0A


/* Comparison operation codes for MatchOp operator */

typedef enum
{
    MATCH_MTR                       = 0,
    MATCH_MEQ                       = 1,
    MATCH_MLE                       = 2,
    MATCH_MLT                       = 3,
    MATCH_MGE                       = 4,
    MATCH_MGT                       = 5

} AML_MATCH_OPERATOR;

#define MAX_MATCH_OPERATOR          5


/*
 * FieldFlags
 *
 * This byte is extracted from the AML and includes three separate
 * pieces of information about the field:
 * 1) The field access type
 * 2) The field update rule
 * 3) The lock rule for the field
 *
 * Bits 00 - 03 : AccessType (AnyAcc, ByteAcc, etc.)
 *      04      : LockRule (1 == Lock)
 *      05 - 06 : UpdateRule
 */
#define AML_FIELD_ACCESS_TYPE_MASK  0x0F
#define AML_FIELD_LOCK_RULE_MASK    0x10
#define AML_FIELD_UPDATE_RULE_MASK  0x60


/* 1) Field Access Types */

typedef enum
{
    AML_FIELD_ACCESS_ANY            = 0x00,
    AML_FIELD_ACCESS_BYTE           = 0x01,
    AML_FIELD_ACCESS_WORD           = 0x02,
    AML_FIELD_ACCESS_DWORD          = 0x03,
    AML_FIELD_ACCESS_QWORD          = 0x04,    /* ACPI 2.0 */
    AML_FIELD_ACCESS_BUFFER         = 0x05     /* ACPI 2.0 */

} AML_ACCESS_TYPE;


/* 2) Field Lock Rules */

typedef enum
{
    AML_FIELD_LOCK_NEVER            = 0x00,
    AML_FIELD_LOCK_ALWAYS           = 0x10

} AML_LOCK_RULE;


/* 3) Field Update Rules */

typedef enum
{
    AML_FIELD_UPDATE_PRESERVE       = 0x00,
    AML_FIELD_UPDATE_WRITE_AS_ONES  = 0x20,
    AML_FIELD_UPDATE_WRITE_AS_ZEROS = 0x40

} AML_UPDATE_RULE;


/*
 * Field Access Attributes.
 * This byte is extracted from the AML via the
 * AccessAs keyword
 */
typedef enum
{
    AML_FIELD_ATTRIB_QUICK          = 0x02,
    AML_FIELD_ATTRIB_SEND_RCV       = 0x04,
    AML_FIELD_ATTRIB_BYTE           = 0x06,
    AML_FIELD_ATTRIB_WORD           = 0x08,
    AML_FIELD_ATTRIB_BLOCK          = 0x0A,
    AML_FIELD_ATTRIB_MULTIBYTE      = 0x0B,
    AML_FIELD_ATTRIB_WORD_CALL      = 0x0C,
    AML_FIELD_ATTRIB_BLOCK_CALL     = 0x0D,
    AML_FIELD_ATTRIB_RAW_BYTES      = 0x0E,
    AML_FIELD_ATTRIB_RAW_PROCESS    = 0x0F

} AML_ACCESS_ATTRIBUTE;


/* Bit fields in the AML MethodFlags byte */

#define AML_METHOD_ARG_COUNT        0x07
#define AML_METHOD_SERIALIZED       0x08
#define AML_METHOD_SYNC_LEVEL       0xF0


#endif /* __AMLCODE_H__ */
