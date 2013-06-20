/******************************************************************************
 *
 * Name: acexcep.h - Exception codes returned by the ACPI subsystem
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code.  No other license or right
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
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
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
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
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

#ifndef __ACEXCEP_H__
#define __ACEXCEP_H__


/*
 * Exceptions returned by external ACPI interfaces
 */
#define AE_CODE_ENVIRONMENTAL           0x0000
#define AE_CODE_PROGRAMMER              0x1000
#define AE_CODE_ACPI_TABLES             0x2000
#define AE_CODE_AML                     0x3000
#define AE_CODE_CONTROL                 0x4000
#define AE_CODE_MASK                    0xF000


#define ACPI_SUCCESS(a)                 (!(a))
#define ACPI_FAILURE(a)                 (a)


#define AE_OK                           (ACPI_STATUS) 0x0000

/*
 * Environmental exceptions
 */
#define AE_ERROR                        (ACPI_STATUS) (0x0001 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_ACPI_TABLES               (ACPI_STATUS) (0x0002 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_NAMESPACE                 (ACPI_STATUS) (0x0003 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_MEMORY                    (ACPI_STATUS) (0x0004 | AE_CODE_ENVIRONMENTAL)
#define AE_NOT_FOUND                    (ACPI_STATUS) (0x0005 | AE_CODE_ENVIRONMENTAL)
#define AE_NOT_EXIST                    (ACPI_STATUS) (0x0006 | AE_CODE_ENVIRONMENTAL)
#define AE_ALREADY_EXISTS               (ACPI_STATUS) (0x0007 | AE_CODE_ENVIRONMENTAL)
#define AE_TYPE                         (ACPI_STATUS) (0x0008 | AE_CODE_ENVIRONMENTAL)
#define AE_NULL_OBJECT                  (ACPI_STATUS) (0x0009 | AE_CODE_ENVIRONMENTAL)
#define AE_NULL_ENTRY                   (ACPI_STATUS) (0x000A | AE_CODE_ENVIRONMENTAL)
#define AE_BUFFER_OVERFLOW              (ACPI_STATUS) (0x000B | AE_CODE_ENVIRONMENTAL)
#define AE_STACK_OVERFLOW               (ACPI_STATUS) (0x000C | AE_CODE_ENVIRONMENTAL)
#define AE_STACK_UNDERFLOW              (ACPI_STATUS) (0x000D | AE_CODE_ENVIRONMENTAL)
#define AE_NOT_IMPLEMENTED              (ACPI_STATUS) (0x000E | AE_CODE_ENVIRONMENTAL)
#define AE_SUPPORT                      (ACPI_STATUS) (0x000F | AE_CODE_ENVIRONMENTAL)
#define AE_LIMIT                        (ACPI_STATUS) (0x0010 | AE_CODE_ENVIRONMENTAL)
#define AE_TIME                         (ACPI_STATUS) (0x0011 | AE_CODE_ENVIRONMENTAL)
#define AE_ACQUIRE_DEADLOCK             (ACPI_STATUS) (0x0012 | AE_CODE_ENVIRONMENTAL)
#define AE_RELEASE_DEADLOCK             (ACPI_STATUS) (0x0013 | AE_CODE_ENVIRONMENTAL)
#define AE_NOT_ACQUIRED                 (ACPI_STATUS) (0x0014 | AE_CODE_ENVIRONMENTAL)
#define AE_ALREADY_ACQUIRED             (ACPI_STATUS) (0x0015 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_HARDWARE_RESPONSE         (ACPI_STATUS) (0x0016 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_GLOBAL_LOCK               (ACPI_STATUS) (0x0017 | AE_CODE_ENVIRONMENTAL)
#define AE_ABORT_METHOD                 (ACPI_STATUS) (0x0018 | AE_CODE_ENVIRONMENTAL)
#define AE_SAME_HANDLER                 (ACPI_STATUS) (0x0019 | AE_CODE_ENVIRONMENTAL)
#define AE_NO_HANDLER                   (ACPI_STATUS) (0x001A | AE_CODE_ENVIRONMENTAL)
#define AE_OWNER_ID_LIMIT               (ACPI_STATUS) (0x001B | AE_CODE_ENVIRONMENTAL)

#define AE_CODE_ENV_MAX                 0x001B


/*
 * Programmer exceptions
 */
#define AE_BAD_PARAMETER                (ACPI_STATUS) (0x0001 | AE_CODE_PROGRAMMER)
#define AE_BAD_CHARACTER                (ACPI_STATUS) (0x0002 | AE_CODE_PROGRAMMER)
#define AE_BAD_PATHNAME                 (ACPI_STATUS) (0x0003 | AE_CODE_PROGRAMMER)
#define AE_BAD_DATA                     (ACPI_STATUS) (0x0004 | AE_CODE_PROGRAMMER)
#define AE_BAD_HEX_CONSTANT             (ACPI_STATUS) (0x0005 | AE_CODE_PROGRAMMER)
#define AE_BAD_OCTAL_CONSTANT           (ACPI_STATUS) (0x0006 | AE_CODE_PROGRAMMER)
#define AE_BAD_DECIMAL_CONSTANT         (ACPI_STATUS) (0x0007 | AE_CODE_PROGRAMMER)
#define AE_MISSING_ARGUMENTS            (ACPI_STATUS) (0x0008 | AE_CODE_PROGRAMMER)
#define AE_BAD_ADDRESS                  (ACPI_STATUS) (0x0009 | AE_CODE_PROGRAMMER)

#define AE_CODE_PGM_MAX                 0x0009


/*
 * Acpi table exceptions
 */
#define AE_BAD_SIGNATURE                (ACPI_STATUS) (0x0001 | AE_CODE_ACPI_TABLES)
#define AE_BAD_HEADER                   (ACPI_STATUS) (0x0002 | AE_CODE_ACPI_TABLES)
#define AE_BAD_CHECKSUM                 (ACPI_STATUS) (0x0003 | AE_CODE_ACPI_TABLES)
#define AE_BAD_VALUE                    (ACPI_STATUS) (0x0004 | AE_CODE_ACPI_TABLES)
#define AE_INVALID_TABLE_LENGTH         (ACPI_STATUS) (0x0005 | AE_CODE_ACPI_TABLES)

#define AE_CODE_TBL_MAX                 0x0005


/*
 * AML exceptions.  These are caused by problems with
 * the actual AML byte stream
 */
#define AE_AML_BAD_OPCODE               (ACPI_STATUS) (0x0001 | AE_CODE_AML)
#define AE_AML_NO_OPERAND               (ACPI_STATUS) (0x0002 | AE_CODE_AML)
#define AE_AML_OPERAND_TYPE             (ACPI_STATUS) (0x0003 | AE_CODE_AML)
#define AE_AML_OPERAND_VALUE            (ACPI_STATUS) (0x0004 | AE_CODE_AML)
#define AE_AML_UNINITIALIZED_LOCAL      (ACPI_STATUS) (0x0005 | AE_CODE_AML)
#define AE_AML_UNINITIALIZED_ARG        (ACPI_STATUS) (0x0006 | AE_CODE_AML)
#define AE_AML_UNINITIALIZED_ELEMENT    (ACPI_STATUS) (0x0007 | AE_CODE_AML)
#define AE_AML_NUMERIC_OVERFLOW         (ACPI_STATUS) (0x0008 | AE_CODE_AML)
#define AE_AML_REGION_LIMIT             (ACPI_STATUS) (0x0009 | AE_CODE_AML)
#define AE_AML_BUFFER_LIMIT             (ACPI_STATUS) (0x000A | AE_CODE_AML)
#define AE_AML_PACKAGE_LIMIT            (ACPI_STATUS) (0x000B | AE_CODE_AML)
#define AE_AML_DIVIDE_BY_ZERO           (ACPI_STATUS) (0x000C | AE_CODE_AML)
#define AE_AML_BAD_NAME                 (ACPI_STATUS) (0x000D | AE_CODE_AML)
#define AE_AML_NAME_NOT_FOUND           (ACPI_STATUS) (0x000E | AE_CODE_AML)
#define AE_AML_INTERNAL                 (ACPI_STATUS) (0x000F | AE_CODE_AML)
#define AE_AML_INVALID_SPACE_ID         (ACPI_STATUS) (0x0010 | AE_CODE_AML)
#define AE_AML_STRING_LIMIT             (ACPI_STATUS) (0x0011 | AE_CODE_AML)
#define AE_AML_NO_RETURN_VALUE          (ACPI_STATUS) (0x0012 | AE_CODE_AML)
#define AE_AML_METHOD_LIMIT             (ACPI_STATUS) (0x0013 | AE_CODE_AML)
#define AE_AML_NOT_OWNER                (ACPI_STATUS) (0x0014 | AE_CODE_AML)
#define AE_AML_MUTEX_ORDER              (ACPI_STATUS) (0x0015 | AE_CODE_AML)
#define AE_AML_MUTEX_NOT_ACQUIRED       (ACPI_STATUS) (0x0016 | AE_CODE_AML)
#define AE_AML_INVALID_RESOURCE_TYPE    (ACPI_STATUS) (0x0017 | AE_CODE_AML)
#define AE_AML_INVALID_INDEX            (ACPI_STATUS) (0x0018 | AE_CODE_AML)
#define AE_AML_REGISTER_LIMIT           (ACPI_STATUS) (0x0019 | AE_CODE_AML)
#define AE_AML_NO_WHILE                 (ACPI_STATUS) (0x001A | AE_CODE_AML)
#define AE_AML_ALIGNMENT                (ACPI_STATUS) (0x001B | AE_CODE_AML)
#define AE_AML_NO_RESOURCE_END_TAG      (ACPI_STATUS) (0x001C | AE_CODE_AML)
#define AE_AML_BAD_RESOURCE_VALUE       (ACPI_STATUS) (0x001D | AE_CODE_AML)
#define AE_AML_CIRCULAR_REFERENCE       (ACPI_STATUS) (0x001E | AE_CODE_AML)
#define AE_AML_BAD_RESOURCE_LENGTH      (ACPI_STATUS) (0x001F | AE_CODE_AML)
#define AE_AML_ILLEGAL_ADDRESS          (ACPI_STATUS) (0x0020 | AE_CODE_AML)
#define AE_AML_INFINITE_LOOP            (ACPI_STATUS) (0x0021 | AE_CODE_AML)

#define AE_CODE_AML_MAX                 0x0021


/*
 * Internal exceptions used for control
 */
#define AE_CTRL_RETURN_VALUE            (ACPI_STATUS) (0x0001 | AE_CODE_CONTROL)
#define AE_CTRL_PENDING                 (ACPI_STATUS) (0x0002 | AE_CODE_CONTROL)
#define AE_CTRL_TERMINATE               (ACPI_STATUS) (0x0003 | AE_CODE_CONTROL)
#define AE_CTRL_TRUE                    (ACPI_STATUS) (0x0004 | AE_CODE_CONTROL)
#define AE_CTRL_FALSE                   (ACPI_STATUS) (0x0005 | AE_CODE_CONTROL)
#define AE_CTRL_DEPTH                   (ACPI_STATUS) (0x0006 | AE_CODE_CONTROL)
#define AE_CTRL_END                     (ACPI_STATUS) (0x0007 | AE_CODE_CONTROL)
#define AE_CTRL_TRANSFER                (ACPI_STATUS) (0x0008 | AE_CODE_CONTROL)
#define AE_CTRL_BREAK                   (ACPI_STATUS) (0x0009 | AE_CODE_CONTROL)
#define AE_CTRL_CONTINUE                (ACPI_STATUS) (0x000A | AE_CODE_CONTROL)
#define AE_CTRL_SKIP                    (ACPI_STATUS) (0x000B | AE_CODE_CONTROL)
#define AE_CTRL_PARSE_CONTINUE          (ACPI_STATUS) (0x000C | AE_CODE_CONTROL)
#define AE_CTRL_PARSE_PENDING           (ACPI_STATUS) (0x000D | AE_CODE_CONTROL)

#define AE_CODE_CTRL_MAX                0x000D


/* Exception strings for AcpiFormatException */

#ifdef DEFINE_ACPI_GLOBALS

/*
 * String versions of the exception codes above
 * These strings must match the corresponding defines exactly
 */
char const   *AcpiGbl_ExceptionNames_Env[] =
{
    "AE_OK",
    "AE_ERROR",
    "AE_NO_ACPI_TABLES",
    "AE_NO_NAMESPACE",
    "AE_NO_MEMORY",
    "AE_NOT_FOUND",
    "AE_NOT_EXIST",
    "AE_ALREADY_EXISTS",
    "AE_TYPE",
    "AE_NULL_OBJECT",
    "AE_NULL_ENTRY",
    "AE_BUFFER_OVERFLOW",
    "AE_STACK_OVERFLOW",
    "AE_STACK_UNDERFLOW",
    "AE_NOT_IMPLEMENTED",
    "AE_SUPPORT",
    "AE_LIMIT",
    "AE_TIME",
    "AE_ACQUIRE_DEADLOCK",
    "AE_RELEASE_DEADLOCK",
    "AE_NOT_ACQUIRED",
    "AE_ALREADY_ACQUIRED",
    "AE_NO_HARDWARE_RESPONSE",
    "AE_NO_GLOBAL_LOCK",
    "AE_ABORT_METHOD",
    "AE_SAME_HANDLER",
    "AE_NO_HANDLER",
    "AE_OWNER_ID_LIMIT"
};

char const   *AcpiGbl_ExceptionNames_Pgm[] =
{
    NULL,
    "AE_BAD_PARAMETER",
    "AE_BAD_CHARACTER",
    "AE_BAD_PATHNAME",
    "AE_BAD_DATA",
    "AE_BAD_HEX_CONSTANT",
    "AE_BAD_OCTAL_CONSTANT",
    "AE_BAD_DECIMAL_CONSTANT",
    "AE_MISSING_ARGUMENTS",
    "AE_BAD_ADDRESS"
};

char const   *AcpiGbl_ExceptionNames_Tbl[] =
{
    NULL,
    "AE_BAD_SIGNATURE",
    "AE_BAD_HEADER",
    "AE_BAD_CHECKSUM",
    "AE_BAD_VALUE",
    "AE_INVALID_TABLE_LENGTH"
};

char const   *AcpiGbl_ExceptionNames_Aml[] =
{
    NULL,
    "AE_AML_BAD_OPCODE",
    "AE_AML_NO_OPERAND",
    "AE_AML_OPERAND_TYPE",
    "AE_AML_OPERAND_VALUE",
    "AE_AML_UNINITIALIZED_LOCAL",
    "AE_AML_UNINITIALIZED_ARG",
    "AE_AML_UNINITIALIZED_ELEMENT",
    "AE_AML_NUMERIC_OVERFLOW",
    "AE_AML_REGION_LIMIT",
    "AE_AML_BUFFER_LIMIT",
    "AE_AML_PACKAGE_LIMIT",
    "AE_AML_DIVIDE_BY_ZERO",
    "AE_AML_BAD_NAME",
    "AE_AML_NAME_NOT_FOUND",
    "AE_AML_INTERNAL",
    "AE_AML_INVALID_SPACE_ID",
    "AE_AML_STRING_LIMIT",
    "AE_AML_NO_RETURN_VALUE",
    "AE_AML_METHOD_LIMIT",
    "AE_AML_NOT_OWNER",
    "AE_AML_MUTEX_ORDER",
    "AE_AML_MUTEX_NOT_ACQUIRED",
    "AE_AML_INVALID_RESOURCE_TYPE",
    "AE_AML_INVALID_INDEX",
    "AE_AML_REGISTER_LIMIT",
    "AE_AML_NO_WHILE",
    "AE_AML_ALIGNMENT",
    "AE_AML_NO_RESOURCE_END_TAG",
    "AE_AML_BAD_RESOURCE_VALUE",
    "AE_AML_CIRCULAR_REFERENCE",
    "AE_AML_BAD_RESOURCE_LENGTH",
    "AE_AML_ILLEGAL_ADDRESS",
    "AE_AML_INFINITE_LOOP"
};

char const   *AcpiGbl_ExceptionNames_Ctrl[] =
{
    NULL,
    "AE_CTRL_RETURN_VALUE",
    "AE_CTRL_PENDING",
    "AE_CTRL_TERMINATE",
    "AE_CTRL_TRUE",
    "AE_CTRL_FALSE",
    "AE_CTRL_DEPTH",
    "AE_CTRL_END",
    "AE_CTRL_TRANSFER",
    "AE_CTRL_BREAK",
    "AE_CTRL_CONTINUE",
    "AE_CTRL_SKIP",
    "AE_CTRL_PARSE_CONTINUE",
    "AE_CTRL_PARSE_PENDING"
};

#endif /* ACPI GLOBALS */

#endif /* __ACEXCEP_H__ */
