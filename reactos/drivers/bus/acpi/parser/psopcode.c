/******************************************************************************
 *
 * Module Name: psopcode - Parser opcode information table
 *              $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "acpi.h"
#include "acparser.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("psopcode")


#define _UNK                        0x6B
/*
 * Reserved ASCII characters.  Do not use any of these for
 * internal opcodes, since they are used to differentiate
 * name strings from AML opcodes
 */
#define _ASC                        0x6C
#define _NAM                        0x6C
#define _PFX                        0x6D
#define _UNKNOWN_OPCODE             0x02    /* An example unknown opcode */

#define MAX_EXTENDED_OPCODE         0x88
#define NUM_EXTENDED_OPCODE         MAX_EXTENDED_OPCODE + 1
#define MAX_INTERNAL_OPCODE
#define NUM_INTERNAL_OPCODE         MAX_INTERNAL_OPCODE + 1


/*******************************************************************************
 *
 * NAME:        Acpi_gbl_Aml_op_info
 *
 * DESCRIPTION: Opcode table. Each entry contains <opcode, type, name, operands>
 *              The name is a simple ascii string, the operand specifier is an
 *              ascii string with one letter per operand.  The letter specifies
 *              the operand type.
 *
 ******************************************************************************/


/*
 * Flags byte: 0-4 (5 bits) = Opcode Type
 *             5   (1 bit)  = Has arguments flag
 *             6-7 (2 bits) = Reserved
 */
#define AML_NO_ARGS         0
#define AML_HAS_ARGS        ACPI_OP_ARGS_MASK

/*
 * All AML opcodes and the parse-time arguments for each.  Used by the AML parser  Each list is compressed
 * into a 32-bit number and stored in the master opcode table at the end of this file.
 */

#define ARGP_ZERO_OP                    ARG_NONE
#define ARGP_ONE_OP                     ARG_NONE
#define ARGP_ALIAS_OP                   ARGP_LIST2 (ARGP_NAMESTRING, ARGP_NAME)
#define ARGP_NAME_OP                    ARGP_LIST2 (ARGP_NAME,       ARGP_DATAOBJ)
#define ARGP_BYTE_OP                    ARGP_LIST1 (ARGP_BYTEDATA)
#define ARGP_WORD_OP                    ARGP_LIST1 (ARGP_WORDDATA)
#define ARGP_DWORD_OP                   ARGP_LIST1 (ARGP_DWORDDATA)
#define ARGP_STRING_OP                  ARGP_LIST1 (ARGP_CHARLIST)
#define ARGP_QWORD_OP                   ARGP_LIST1 (ARGP_QWORDDATA)
#define ARGP_SCOPE_OP                   ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_TERMLIST)
#define ARGP_BUFFER_OP                  ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_TERMARG,       ARGP_BYTELIST)
#define ARGP_PACKAGE_OP                 ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_BYTEDATA,      ARGP_DATAOBJLIST)
#define ARGP_VAR_PACKAGE_OP             ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_BYTEDATA,      ARGP_DATAOBJLIST)
#define ARGP_METHOD_OP                  ARGP_LIST4 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_BYTEDATA,      ARGP_TERMLIST)
#define ARGP_LOCAL0                     ARG_NONE
#define ARGP_LOCAL1                     ARG_NONE
#define ARGP_LOCAL2                     ARG_NONE
#define ARGP_LOCAL3                     ARG_NONE
#define ARGP_LOCAL4                     ARG_NONE
#define ARGP_LOCAL5                     ARG_NONE
#define ARGP_LOCAL6                     ARG_NONE
#define ARGP_LOCAL7                     ARG_NONE
#define ARGP_ARG0                       ARG_NONE
#define ARGP_ARG1                       ARG_NONE
#define ARGP_ARG2                       ARG_NONE
#define ARGP_ARG3                       ARG_NONE
#define ARGP_ARG4                       ARG_NONE
#define ARGP_ARG5                       ARG_NONE
#define ARGP_ARG6                       ARG_NONE
#define ARGP_STORE_OP                   ARGP_LIST2 (ARGP_TERMARG,    ARGP_SUPERNAME)
#define ARGP_REF_OF_OP                  ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_ADD_OP                     ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_CONCAT_OP                  ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_SUBTRACT_OP                ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_INCREMENT_OP               ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_DECREMENT_OP               ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_MULTIPLY_OP                ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_DIVIDE_OP                  ARGP_LIST4 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET,    ARGP_TARGET)
#define ARGP_SHIFT_LEFT_OP              ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_SHIFT_RIGHT_OP             ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_AND_OP                 ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_NAND_OP                ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_OR_OP                  ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_NOR_OP                 ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_XOR_OP                 ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_BIT_NOT_OP                 ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_FIND_SET_LEFT_BIT_OP       ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_FIND_SET_RIGHT_BIT_OP      ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_DEREF_OF_OP                ARGP_LIST1 (ARGP_TERMARG)
#define ARGP_CONCAT_RES_OP              ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_MOD_OP                     ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_NOTIFY_OP                  ARGP_LIST2 (ARGP_SUPERNAME,  ARGP_TERMARG)
#define ARGP_SIZE_OF_OP                 ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_INDEX_OP                   ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_MATCH_OP                   ARGP_LIST6 (ARGP_TERMARG,    ARGP_BYTEDATA,      ARGP_TERMARG,   ARGP_BYTEDATA,  ARGP_TERMARG,   ARGP_TERMARG)
#define ARGP_DWORD_FIELD_OP             ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_NAME)
#define ARGP_WORD_FIELD_OP              ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_NAME)
#define ARGP_BYTE_FIELD_OP              ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_NAME)
#define ARGP_BIT_FIELD_OP               ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_NAME)
#define ARGP_TYPE_OP                    ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_QWORD_FIELD_OP             ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_NAME)
#define ARGP_LAND_OP                    ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LOR_OP                     ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LNOT_OP                    ARGP_LIST1 (ARGP_TERMARG)
#define ARGP_LEQUAL_OP                  ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LGREATER_OP                ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LLESS_OP                   ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_TO_BUFFER_OP               ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_TO_DEC_STR_OP              ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_TO_HEX_STR_OP              ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_TO_INTEGER_OP              ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_TO_STRING_OP               ARGP_LIST3 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TARGET)
#define ARGP_COPY_OP                    ARGP_LIST2 (ARGP_SUPERNAME,  ARGP_SIMPLENAME)
#define ARGP_MID_OP                     ARGP_LIST4 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TERMARG,   ARGP_TARGET)
#define ARGP_CONTINUE_OP                ARG_NONE
#define ARGP_IF_OP                      ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_TERMARG, ARGP_TERMLIST)
#define ARGP_ELSE_OP                    ARGP_LIST2 (ARGP_PKGLENGTH,  ARGP_TERMLIST)
#define ARGP_WHILE_OP                   ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_TERMARG, ARGP_TERMLIST)
#define ARGP_NOOP_OP                    ARG_NONE
#define ARGP_RETURN_OP                  ARGP_LIST1 (ARGP_TERMARG)
#define ARGP_BREAK_OP                   ARG_NONE
#define ARGP_BREAK_POINT_OP             ARG_NONE
#define ARGP_ONES_OP                    ARG_NONE
#define ARGP_MUTEX_OP                   ARGP_LIST2 (ARGP_NAME,       ARGP_BYTEDATA)
#define ARGP_EVENT_OP                   ARGP_LIST1 (ARGP_NAME)
#define ARGP_COND_REF_OF_OP             ARGP_LIST2 (ARGP_SUPERNAME,  ARGP_SUPERNAME)
#define ARGP_CREATE_FIELD_OP            ARGP_LIST4 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TERMARG,   ARGP_NAME)
#define ARGP_LOAD_TABLE_OP              ARGP_LIST6 (ARGP_TERMARG,    ARGP_TERMARG,       ARGP_TERMARG,   ARGP_TERMARG,  ARGP_TERMARG,   ARGP_TERMARG)
#define ARGP_LOAD_OP                    ARGP_LIST2 (ARGP_NAMESTRING, ARGP_SUPERNAME)
#define ARGP_STALL_OP                   ARGP_LIST1 (ARGP_TERMARG)
#define ARGP_SLEEP_OP                   ARGP_LIST1 (ARGP_TERMARG)
#define ARGP_ACQUIRE_OP                 ARGP_LIST2 (ARGP_SUPERNAME,  ARGP_WORDDATA)
#define ARGP_SIGNAL_OP                  ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_WAIT_OP                    ARGP_LIST2 (ARGP_SUPERNAME,  ARGP_TERMARG)
#define ARGP_RESET_OP                   ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_RELEASE_OP                 ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_FROM_BCD_OP                ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_TO_BCD_OP                  ARGP_LIST2 (ARGP_TERMARG,    ARGP_TARGET)
#define ARGP_UNLOAD_OP                  ARGP_LIST1 (ARGP_SUPERNAME)
#define ARGP_REVISION_OP                ARG_NONE
#define ARGP_DEBUG_OP                   ARG_NONE
#define ARGP_FATAL_OP                   ARGP_LIST3 (ARGP_BYTEDATA,   ARGP_DWORDDATA,     ARGP_TERMARG)
#define ARGP_REGION_OP                  ARGP_LIST4 (ARGP_NAME,       ARGP_BYTEDATA,      ARGP_TERMARG,   ARGP_TERMARG)
#define ARGP_DEF_FIELD_OP               ARGP_LIST4 (ARGP_PKGLENGTH,  ARGP_NAMESTRING,    ARGP_BYTEDATA,  ARGP_FIELDLIST)
#define ARGP_DEVICE_OP                  ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_OBJLIST)
#define ARGP_PROCESSOR_OP               ARGP_LIST6 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_BYTEDATA,  ARGP_DWORDDATA, ARGP_BYTEDATA,  ARGP_OBJLIST)
#define ARGP_POWER_RES_OP               ARGP_LIST5 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_BYTEDATA,  ARGP_WORDDATA,  ARGP_OBJLIST)
#define ARGP_THERMAL_ZONE_OP            ARGP_LIST3 (ARGP_PKGLENGTH,  ARGP_NAME,          ARGP_OBJLIST)
#define ARGP_INDEX_FIELD_OP             ARGP_LIST5 (ARGP_PKGLENGTH,  ARGP_NAMESTRING,    ARGP_NAMESTRING,ARGP_BYTEDATA,  ARGP_FIELDLIST)
#define ARGP_BANK_FIELD_OP              ARGP_LIST6 (ARGP_PKGLENGTH,  ARGP_NAMESTRING,    ARGP_NAMESTRING,ARGP_TERMARG,   ARGP_BYTEDATA,  ARGP_FIELDLIST)
#define ARGP_DATA_REGION_OP             ARGP_LIST4 (ARGP_NAMESTRING, ARGP_TERMARG,       ARGP_TERMARG,   ARGP_TERMARG)
#define ARGP_LNOTEQUAL_OP               ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LLESSEQUAL_OP              ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_LGREATEREQUAL_OP           ARGP_LIST2 (ARGP_TERMARG,    ARGP_TERMARG)
#define ARGP_NAMEPATH_OP                ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_METHODCALL_OP              ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_BYTELIST_OP                ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_RESERVEDFIELD_OP           ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_NAMEDFIELD_OP              ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_ACCESSFIELD_OP             ARGP_LIST1 (ARGP_NAMESTRING)
#define ARGP_STATICSTRING_OP            ARGP_LIST1 (ARGP_NAMESTRING)


/*
 * All AML opcodes and the runtime arguments for each.  Used by the AML interpreter  Each list is compressed
 * into a 32-bit number and stored in the master opcode table at the end of this file.
 *
 * (Used by Acpi_aml_prep_operands procedure and the ASL Compiler)
 */

#define ARGI_ZERO_OP                    ARG_NONE
#define ARGI_ONE_OP                     ARG_NONE
#define ARGI_ALIAS_OP                   ARGI_INVALID_OPCODE
#define ARGI_NAME_OP                    ARGI_INVALID_OPCODE
#define ARGI_BYTE_OP                    ARGI_INVALID_OPCODE
#define ARGI_WORD_OP                    ARGI_INVALID_OPCODE
#define ARGI_DWORD_OP                   ARGI_INVALID_OPCODE
#define ARGI_STRING_OP                  ARGI_INVALID_OPCODE
#define ARGI_QWORD_OP                   ARGI_INVALID_OPCODE
#define ARGI_SCOPE_OP                   ARGI_INVALID_OPCODE
#define ARGI_BUFFER_OP                  ARGI_INVALID_OPCODE
#define ARGI_PACKAGE_OP                 ARGI_INVALID_OPCODE
#define ARGI_VAR_PACKAGE_OP             ARGI_INVALID_OPCODE
#define ARGI_METHOD_OP                  ARGI_INVALID_OPCODE
#define ARGI_LOCAL0                     ARG_NONE
#define ARGI_LOCAL1                     ARG_NONE
#define ARGI_LOCAL2                     ARG_NONE
#define ARGI_LOCAL3                     ARG_NONE
#define ARGI_LOCAL4                     ARG_NONE
#define ARGI_LOCAL5                     ARG_NONE
#define ARGI_LOCAL6                     ARG_NONE
#define ARGI_LOCAL7                     ARG_NONE
#define ARGI_ARG0                       ARG_NONE
#define ARGI_ARG1                       ARG_NONE
#define ARGI_ARG2                       ARG_NONE
#define ARGI_ARG3                       ARG_NONE
#define ARGI_ARG4                       ARG_NONE
#define ARGI_ARG5                       ARG_NONE
#define ARGI_ARG6                       ARG_NONE
#define ARGI_STORE_OP                   ARGI_LIST2 (ARGI_ANYTYPE,    ARGI_TARGETREF)
#define ARGI_REF_OF_OP                  ARGI_LIST1 (ARGI_OBJECT_REF)
#define ARGI_ADD_OP                     ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_CONCAT_OP                  ARGI_LIST3 (ARGI_COMPUTEDATA,ARGI_COMPUTEDATA,   ARGI_TARGETREF)
#define ARGI_SUBTRACT_OP                ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_INCREMENT_OP               ARGI_LIST1 (ARGI_INTEGER_REF)
#define ARGI_DECREMENT_OP               ARGI_LIST1 (ARGI_INTEGER_REF)
#define ARGI_MULTIPLY_OP                ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_DIVIDE_OP                  ARGI_LIST4 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF,    ARGI_TARGETREF)
#define ARGI_SHIFT_LEFT_OP              ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_SHIFT_RIGHT_OP             ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_AND_OP                 ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_NAND_OP                ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_OR_OP                  ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_NOR_OP                 ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_XOR_OP                 ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_BIT_NOT_OP                 ARGI_LIST2 (ARGI_INTEGER,    ARGI_TARGETREF)
#define ARGI_FIND_SET_LEFT_BIT_OP       ARGI_LIST2 (ARGI_INTEGER,    ARGI_TARGETREF)
#define ARGI_FIND_SET_RIGHT_BIT_OP      ARGI_LIST2 (ARGI_INTEGER,    ARGI_TARGETREF)
#define ARGI_DEREF_OF_OP                ARGI_LIST1 (ARGI_REFERENCE)
#define ARGI_CONCAT_RES_OP              ARGI_LIST3 (ARGI_BUFFER,     ARGI_BUFFER,        ARGI_TARGETREF)
#define ARGI_MOD_OP                     ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_NOTIFY_OP                  ARGI_LIST2 (ARGI_DEVICE_REF, ARGI_INTEGER)
#define ARGI_SIZE_OF_OP                 ARGI_LIST1 (ARGI_DATAOBJECT)
#define ARGI_INDEX_OP                   ARGI_LIST3 (ARGI_COMPLEXOBJ, ARGI_INTEGER,       ARGI_TARGETREF)
#define ARGI_MATCH_OP                   ARGI_LIST6 (ARGI_PACKAGE,    ARGI_INTEGER,       ARGI_INTEGER,      ARGI_INTEGER,   ARGI_INTEGER,   ARGI_INTEGER)
#define ARGI_DWORD_FIELD_OP             ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_REFERENCE)
#define ARGI_WORD_FIELD_OP              ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_REFERENCE)
#define ARGI_BYTE_FIELD_OP              ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_REFERENCE)
#define ARGI_BIT_FIELD_OP               ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_REFERENCE)
#define ARGI_TYPE_OP                    ARGI_LIST1 (ARGI_ANYTYPE)
#define ARGI_QWORD_FIELD_OP             ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_REFERENCE)
#define ARGI_LAND_OP                    ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_LOR_OP                     ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_LNOT_OP                    ARGI_LIST1 (ARGI_INTEGER)
#define ARGI_LEQUAL_OP                  ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_LGREATER_OP                ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_LLESS_OP                   ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_TO_BUFFER_OP               ARGI_LIST2 (ARGI_COMPUTEDATA,ARGI_FIXED_TARGET)
#define ARGI_TO_DEC_STR_OP              ARGI_LIST2 (ARGI_COMPUTEDATA,ARGI_FIXED_TARGET)
#define ARGI_TO_HEX_STR_OP              ARGI_LIST2 (ARGI_COMPUTEDATA,ARGI_FIXED_TARGET)
#define ARGI_TO_INTEGER_OP              ARGI_LIST2 (ARGI_COMPUTEDATA,ARGI_FIXED_TARGET)
#define ARGI_TO_STRING_OP               ARGI_LIST3 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_FIXED_TARGET)
#define ARGI_COPY_OP                    ARGI_LIST2 (ARGI_ANYTYPE,    ARGI_SIMPLE_TARGET)
#define ARGI_MID_OP                     ARGI_LIST4 (ARGI_BUFFERSTRING,ARGI_INTEGER,      ARGI_INTEGER,      ARGI_TARGETREF)
#define ARGI_CONTINUE_OP                ARGI_INVALID_OPCODE
#define ARGI_IF_OP                      ARGI_INVALID_OPCODE
#define ARGI_ELSE_OP                    ARGI_INVALID_OPCODE
#define ARGI_WHILE_OP                   ARGI_INVALID_OPCODE
#define ARGI_NOOP_OP                    ARG_NONE
#define ARGI_RETURN_OP                  ARGI_INVALID_OPCODE
#define ARGI_BREAK_OP                   ARG_NONE
#define ARGI_BREAK_POINT_OP             ARG_NONE
#define ARGI_ONES_OP                    ARG_NONE
#define ARGI_MUTEX_OP                   ARGI_INVALID_OPCODE
#define ARGI_EVENT_OP                   ARGI_INVALID_OPCODE
#define ARGI_COND_REF_OF_OP             ARGI_LIST2 (ARGI_OBJECT_REF, ARGI_TARGETREF)
#define ARGI_CREATE_FIELD_OP            ARGI_LIST4 (ARGI_BUFFER,     ARGI_INTEGER,       ARGI_INTEGER,      ARGI_REFERENCE)
#define ARGI_LOAD_TABLE_OP              ARGI_LIST6 (ARGI_STRING,     ARGI_STRING,        ARGI_STRING,       ARGI_STRING,    ARGI_STRING, ARGI_TARGETREF)
#define ARGI_LOAD_OP                    ARGI_LIST2 (ARGI_REGION,     ARGI_TARGETREF)
#define ARGI_STALL_OP                   ARGI_LIST1 (ARGI_INTEGER)
#define ARGI_SLEEP_OP                   ARGI_LIST1 (ARGI_INTEGER)
#define ARGI_ACQUIRE_OP                 ARGI_LIST2 (ARGI_MUTEX,      ARGI_INTEGER)
#define ARGI_SIGNAL_OP                  ARGI_LIST1 (ARGI_EVENT)
#define ARGI_WAIT_OP                    ARGI_LIST2 (ARGI_EVENT,      ARGI_INTEGER)
#define ARGI_RESET_OP                   ARGI_LIST1 (ARGI_EVENT)
#define ARGI_RELEASE_OP                 ARGI_LIST1 (ARGI_MUTEX)
#define ARGI_FROM_BCD_OP                ARGI_LIST2 (ARGI_INTEGER,    ARGI_TARGETREF)
#define ARGI_TO_BCD_OP                  ARGI_LIST2 (ARGI_INTEGER,    ARGI_FIXED_TARGET)
#define ARGI_UNLOAD_OP                  ARGI_LIST1 (ARGI_DDBHANDLE)
#define ARGI_REVISION_OP                ARG_NONE
#define ARGI_DEBUG_OP                   ARG_NONE
#define ARGI_FATAL_OP                   ARGI_LIST3 (ARGI_INTEGER,    ARGI_INTEGER,       ARGI_INTEGER)
#define ARGI_REGION_OP                  ARGI_LIST2 (ARGI_INTEGER,    ARGI_INTEGER)
#define ARGI_DEF_FIELD_OP               ARGI_INVALID_OPCODE
#define ARGI_DEVICE_OP                  ARGI_INVALID_OPCODE
#define ARGI_PROCESSOR_OP               ARGI_INVALID_OPCODE
#define ARGI_POWER_RES_OP               ARGI_INVALID_OPCODE
#define ARGI_THERMAL_ZONE_OP            ARGI_INVALID_OPCODE
#define ARGI_INDEX_FIELD_OP             ARGI_INVALID_OPCODE
#define ARGI_BANK_FIELD_OP              ARGI_INVALID_OPCODE
#define ARGI_DATA_REGION_OP             ARGI_LIST3 (ARGI_STRING,     ARGI_STRING,       ARGI_STRING)
#define ARGI_LNOTEQUAL_OP               ARGI_INVALID_OPCODE
#define ARGI_LLESSEQUAL_OP              ARGI_INVALID_OPCODE
#define ARGI_LGREATEREQUAL_OP           ARGI_INVALID_OPCODE
#define ARGI_NAMEPATH_OP                ARGI_INVALID_OPCODE
#define ARGI_METHODCALL_OP              ARGI_INVALID_OPCODE
#define ARGI_BYTELIST_OP                ARGI_INVALID_OPCODE
#define ARGI_RESERVEDFIELD_OP           ARGI_INVALID_OPCODE
#define ARGI_NAMEDFIELD_OP              ARGI_INVALID_OPCODE
#define ARGI_ACCESSFIELD_OP             ARGI_INVALID_OPCODE
#define ARGI_STATICSTRING_OP            ARGI_INVALID_OPCODE


/*
 * Master Opcode information table.  A summary of everything we know about each opcode, all in one place.
 */


static ACPI_OPCODE_INFO    aml_op_info[] =
{
/* Index          Opcode                                   Type                   Class                 Has Arguments?   Name                 Parser Args             Interpreter Args */

/*  00 */   /* AML_ZERO_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONSTANT|        AML_NO_ARGS,  "Zero",               ARGP_ZERO_OP,           ARGI_ZERO_OP),
/*  01 */   /* AML_ONE_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONSTANT|        AML_NO_ARGS,  "One",                ARGP_ONE_OP,            ARGI_ONE_OP),
/*  02 */   /* AML_ALIAS_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Alias",              ARGP_ALIAS_OP,          ARGI_ALIAS_OP),
/*  03 */   /* AML_NAME_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Name",               ARGP_NAME_OP,           ARGI_NAME_OP),
/*  04 */   /* AML_BYTE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Byte_const",         ARGP_BYTE_OP,           ARGI_BYTE_OP),
/*  05 */   /* AML_WORD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Word_const",         ARGP_WORD_OP,           ARGI_WORD_OP),
/*  06 */   /* AML_DWORD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Dword_const",        ARGP_DWORD_OP,          ARGI_DWORD_OP),
/*  07 */   /* AML_STRING_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "String",             ARGP_STRING_OP,         ARGI_STRING_OP),
/*  08 */   /* AML_SCOPE_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Scope",              ARGP_SCOPE_OP,          ARGI_SCOPE_OP),
/*  09 */   /* AML_BUFFER_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DATA_TERM|       AML_HAS_ARGS, "Buffer",             ARGP_BUFFER_OP,         ARGI_BUFFER_OP),
/*  0A */   /* AML_PACKAGE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DATA_TERM|       AML_HAS_ARGS, "Package",            ARGP_PACKAGE_OP,        ARGI_PACKAGE_OP),
/*  0B */   /* AML_METHOD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Method",             ARGP_METHOD_OP,         ARGI_METHOD_OP),
/*  0C */   /* AML_LOCAL0 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local0",             ARGP_LOCAL0,            ARGI_LOCAL0),
/*  0D */   /* AML_LOCAL1 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local1",             ARGP_LOCAL1,            ARGI_LOCAL1),
/*  0E */   /* AML_LOCAL2 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local2",             ARGP_LOCAL2,            ARGI_LOCAL2),
/*  0F */   /* AML_LOCAL3 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local3",             ARGP_LOCAL3,            ARGI_LOCAL3),
/*  10 */   /* AML_LOCAL4 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local4",             ARGP_LOCAL4,            ARGI_LOCAL4),
/*  11 */   /* AML_LOCAL5 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local5",             ARGP_LOCAL5,            ARGI_LOCAL5),
/*  12 */   /* AML_LOCAL6 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local6",             ARGP_LOCAL6,            ARGI_LOCAL6),
/*  13 */   /* AML_LOCAL7 */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LOCAL_VARIABLE|  AML_NO_ARGS,  "Local7",             ARGP_LOCAL7,            ARGI_LOCAL7),
/*  14 */   /* AML_ARG0 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg0",               ARGP_ARG0,              ARGI_ARG0),
/*  15 */   /* AML_ARG1 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg1",               ARGP_ARG1,              ARGI_ARG1),
/*  16 */   /* AML_ARG2 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg2",               ARGP_ARG2,              ARGI_ARG2),
/*  17 */   /* AML_ARG3 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg3",               ARGP_ARG3,              ARGI_ARG3),
/*  18 */   /* AML_ARG4 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg4",               ARGP_ARG4,              ARGI_ARG4),
/*  19 */   /* AML_ARG5 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg5",               ARGP_ARG5,              ARGI_ARG5),
/*  1_a */  /* AML_ARG6 */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_ARGUMENT| AML_NO_ARGS,  "Arg6",               ARGP_ARG6,              ARGI_ARG6),
/*  1_b */  /* AML_STORE_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Store",              ARGP_STORE_OP,          ARGI_STORE_OP),
/*  1_c */  /* AML_REF_OF_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Ref_of",             ARGP_REF_OF_OP,         ARGI_REF_OF_OP),
/*  1_d */  /* AML_ADD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Add",                ARGP_ADD_OP,            ARGI_ADD_OP),
/*  1_e */  /* AML_CONCAT_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Concatenate",        ARGP_CONCAT_OP,         ARGI_CONCAT_OP),
/*  1_f */  /* AML_SUBTRACT_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Subtract",           ARGP_SUBTRACT_OP,       ARGI_SUBTRACT_OP),
/*  20 */   /* AML_INCREMENT_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Increment",          ARGP_INCREMENT_OP,      ARGI_INCREMENT_OP),
/*  21 */   /* AML_DECREMENT_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Decrement",          ARGP_DECREMENT_OP,      ARGI_DECREMENT_OP),
/*  22 */   /* AML_MULTIPLY_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Multiply",           ARGP_MULTIPLY_OP,       ARGI_MULTIPLY_OP),
/*  23 */   /* AML_DIVIDE_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Divide",             ARGP_DIVIDE_OP,         ARGI_DIVIDE_OP),
/*  24 */   /* AML_SHIFT_LEFT_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Shift_left",         ARGP_SHIFT_LEFT_OP,     ARGI_SHIFT_LEFT_OP),
/*  25 */   /* AML_SHIFT_RIGHT_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Shift_right",        ARGP_SHIFT_RIGHT_OP,    ARGI_SHIFT_RIGHT_OP),
/*  26 */   /* AML_BIT_AND_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "And",                ARGP_BIT_AND_OP,        ARGI_BIT_AND_OP),
/*  27 */   /* AML_BIT_NAND_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "NAnd",               ARGP_BIT_NAND_OP,       ARGI_BIT_NAND_OP),
/*  28 */   /* AML_BIT_OR_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Or",                 ARGP_BIT_OR_OP,         ARGI_BIT_OR_OP),
/*  29 */   /* AML_BIT_NOR_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "NOr",                ARGP_BIT_NOR_OP,        ARGI_BIT_NOR_OP),
/*  2_a */  /* AML_BIT_XOR_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "XOr",                ARGP_BIT_XOR_OP,        ARGI_BIT_XOR_OP),
/*  2_b */  /* AML_BIT_NOT_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Not",                ARGP_BIT_NOT_OP,        ARGI_BIT_NOT_OP),
/*  2_c */  /* AML_FIND_SET_LEFT_BIT_OP */  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Find_set_left_bit",  ARGP_FIND_SET_LEFT_BIT_OP, ARGI_FIND_SET_LEFT_BIT_OP),
/*  2_d */  /* AML_FIND_SET_RIGHT_BIT_OP */ OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Find_set_right_bit", ARGP_FIND_SET_RIGHT_BIT_OP, ARGI_FIND_SET_RIGHT_BIT_OP),
/*  2_e */  /* AML_DEREF_OF_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Deref_of",           ARGP_DEREF_OF_OP,       ARGI_DEREF_OF_OP),
/*  2_f */  /* AML_NOTIFY_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC1|         AML_HAS_ARGS, "Notify",             ARGP_NOTIFY_OP,         ARGI_NOTIFY_OP),
/*  30 */   /* AML_SIZE_OF_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Size_of",            ARGP_SIZE_OF_OP,        ARGI_SIZE_OF_OP),
/*  31 */   /* AML_INDEX_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_INDEX|           AML_HAS_ARGS, "Index",              ARGP_INDEX_OP,          ARGI_INDEX_OP),
/*  32 */   /* AML_MATCH_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MATCH|           AML_HAS_ARGS, "Match",              ARGP_MATCH_OP,          ARGI_MATCH_OP),
/*  33 */   /* AML_DWORD_FIELD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_dWord_field", ARGP_DWORD_FIELD_OP,    ARGI_DWORD_FIELD_OP),
/*  34 */   /* AML_WORD_FIELD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_word_field",  ARGP_WORD_FIELD_OP,     ARGI_WORD_FIELD_OP),
/*  35 */   /* AML_BYTE_FIELD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_byte_field",  ARGP_BYTE_FIELD_OP,     ARGI_BYTE_FIELD_OP),
/*  36 */   /* AML_BIT_FIELD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_bit_field",   ARGP_BIT_FIELD_OP,      ARGI_BIT_FIELD_OP),
/*  37 */   /* AML_TYPE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "Object_type",        ARGP_TYPE_OP,           ARGI_TYPE_OP),
/*  38 */   /* AML_LAND_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2|         AML_HAS_ARGS, "LAnd",               ARGP_LAND_OP,           ARGI_LAND_OP),
/*  39 */   /* AML_LOR_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2|         AML_HAS_ARGS, "LOr",                ARGP_LOR_OP,            ARGI_LOR_OP),
/*  3_a */  /* AML_LNOT_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2|        AML_HAS_ARGS, "LNot",               ARGP_LNOT_OP,           ARGI_LNOT_OP),
/*  3_b */  /* AML_LEQUAL_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2|         AML_HAS_ARGS, "LEqual",             ARGP_LEQUAL_OP,         ARGI_LEQUAL_OP),
/*  3_c */  /* AML_LGREATER_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2|         AML_HAS_ARGS, "LGreater",           ARGP_LGREATER_OP,       ARGI_LGREATER_OP),
/*  3_d */  /* AML_LLESS_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2|         AML_HAS_ARGS, "LLess",              ARGP_LLESS_OP,          ARGI_LLESS_OP),
/*  3_e */  /* AML_IF_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_HAS_ARGS, "If",                 ARGP_IF_OP,             ARGI_IF_OP),
/*  3_f */  /* AML_ELSE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_HAS_ARGS, "Else",               ARGP_ELSE_OP,           ARGI_ELSE_OP),
/*  40 */   /* AML_WHILE_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_HAS_ARGS, "While",              ARGP_WHILE_OP,          ARGI_WHILE_OP),
/*  41 */   /* AML_NOOP_OP   */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_NO_ARGS,  "Noop",               ARGP_NOOP_OP,           ARGI_NOOP_OP),
/*  42 */   /* AML_RETURN_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_HAS_ARGS, "Return",             ARGP_RETURN_OP,         ARGI_RETURN_OP),
/*  43 */   /* AML_BREAK_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_NO_ARGS,  "Break",              ARGP_BREAK_OP,          ARGI_BREAK_OP),
/*  44 */   /* AML_BREAK_POINT_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_NO_ARGS,  "Break_point",        ARGP_BREAK_POINT_OP,    ARGI_BREAK_POINT_OP),
/*  45 */   /* AML_ONES_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONSTANT|        AML_NO_ARGS,  "Ones",               ARGP_ONES_OP,           ARGI_ONES_OP),

/* Prefixed opcodes (Two-byte opcodes with a prefix op) */

/*  46 */   /* AML_MUTEX_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Mutex",              ARGP_MUTEX_OP,          ARGI_MUTEX_OP),
/*  47 */   /* AML_EVENT_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_NO_ARGS,  "Event",              ARGP_EVENT_OP,          ARGI_EVENT_OP),
/*  48 */   /* AML_COND_REF_OF_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Cond_ref_of",        ARGP_COND_REF_OF_OP,    ARGI_COND_REF_OF_OP),
/*  49 */   /* AML_CREATE_FIELD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_field",       ARGP_CREATE_FIELD_OP,   ARGI_CREATE_FIELD_OP),
/*  4_a */  /* AML_LOAD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_RECONFIGURATION| AML_HAS_ARGS, "Load",               ARGP_LOAD_OP,           ARGI_LOAD_OP),
/*  4_b */  /* AML_STALL_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC1|        AML_HAS_ARGS, "Stall",              ARGP_STALL_OP,          ARGI_STALL_OP),
/*  4_c */  /* AML_SLEEP_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC1|        AML_HAS_ARGS, "Sleep",              ARGP_SLEEP_OP,          ARGI_SLEEP_OP),
/*  4_d */  /* AML_ACQUIRE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_s|       AML_HAS_ARGS, "Acquire",            ARGP_ACQUIRE_OP,        ARGI_ACQUIRE_OP),
/*  4_e */  /* AML_SIGNAL_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC1|        AML_HAS_ARGS, "Signal",             ARGP_SIGNAL_OP,         ARGI_SIGNAL_OP),
/*  4_f */  /* AML_WAIT_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_s|       AML_HAS_ARGS, "Wait",               ARGP_WAIT_OP,           ARGI_WAIT_OP),
/*  50 */   /* AML_RESET_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC1|        AML_HAS_ARGS, "Reset",              ARGP_RESET_OP,          ARGI_RESET_OP),
/*  51 */   /* AML_RELEASE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC1|        AML_HAS_ARGS, "Release",            ARGP_RELEASE_OP,        ARGI_RELEASE_OP),
/*  52 */   /* AML_FROM_BCD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "From_bCD",           ARGP_FROM_BCD_OP,       ARGI_FROM_BCD_OP),
/*  53 */   /* AML_TO_BCD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_bCD",             ARGP_TO_BCD_OP,         ARGI_TO_BCD_OP),
/*  54 */   /* AML_UNLOAD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_RECONFIGURATION| AML_HAS_ARGS, "Unload",             ARGP_UNLOAD_OP,         ARGI_UNLOAD_OP),
/*  55 */   /* AML_REVISION_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONSTANT|        AML_NO_ARGS,  "Revision",           ARGP_REVISION_OP,       ARGI_REVISION_OP),
/*  56 */   /* AML_DEBUG_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONSTANT|        AML_NO_ARGS,  "Debug",              ARGP_DEBUG_OP,          ARGI_DEBUG_OP),
/*  57 */   /* AML_FATAL_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_FATAL|           AML_HAS_ARGS, "Fatal",              ARGP_FATAL_OP,          ARGI_FATAL_OP),
/*  58 */   /* AML_REGION_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Op_region",          ARGP_REGION_OP,         ARGI_REGION_OP),
/*  59 */   /* AML_DEF_FIELD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Field",              ARGP_DEF_FIELD_OP,      ARGI_DEF_FIELD_OP),
/*  5_a */  /* AML_DEVICE_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Device",             ARGP_DEVICE_OP,         ARGI_DEVICE_OP),
/*  5_b */  /* AML_PROCESSOR_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Processor",          ARGP_PROCESSOR_OP,      ARGI_PROCESSOR_OP),
/*  5_c */  /* AML_POWER_RES_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Power_resource",     ARGP_POWER_RES_OP,      ARGI_POWER_RES_OP),
/*  5_d */  /* AML_THERMAL_ZONE_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Thermal_zone",       ARGP_THERMAL_ZONE_OP,   ARGI_THERMAL_ZONE_OP),
/*  5_e */  /* AML_INDEX_FIELD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Index_field",        ARGP_INDEX_FIELD_OP,    ARGI_INDEX_FIELD_OP),
/*  5_f */  /* AML_BANK_FIELD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_NAMED_OBJECT|    AML_HAS_ARGS, "Bank_field",         ARGP_BANK_FIELD_OP,     ARGI_BANK_FIELD_OP),

/* Internal opcodes that map to invalid AML opcodes */

/*  60 */   /* AML_LNOTEQUAL_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_HAS_ARGS, "LNot_equal",         ARGP_LNOTEQUAL_OP,      ARGI_LNOTEQUAL_OP),
/*  61 */   /* AML_LLESSEQUAL_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_HAS_ARGS, "LLess_equal",        ARGP_LLESSEQUAL_OP,     ARGI_LLESSEQUAL_OP),
/*  62 */   /* AML_LGREATEREQUAL_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_HAS_ARGS, "LGreater_equal",     ARGP_LGREATEREQUAL_OP,  ARGI_LGREATEREQUAL_OP),
/*  63 */   /* AML_NAMEPATH_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Name_path",          ARGP_NAMEPATH_OP,       ARGI_NAMEPATH_OP),
/*  64 */   /* AML_METHODCALL_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_METHOD_CALL|     AML_HAS_ARGS, "Method_call",        ARGP_METHODCALL_OP,     ARGI_METHODCALL_OP),
/*  65 */   /* AML_BYTELIST_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Byte_list",          ARGP_BYTELIST_OP,       ARGI_BYTELIST_OP),
/*  66 */   /* AML_RESERVEDFIELD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_NO_ARGS,  "Reserved_field",     ARGP_RESERVEDFIELD_OP,  ARGI_RESERVEDFIELD_OP),
/*  67 */   /* AML_NAMEDFIELD_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_NO_ARGS,  "Named_field",        ARGP_NAMEDFIELD_OP,     ARGI_NAMEDFIELD_OP),
/*  68 */   /* AML_ACCESSFIELD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_NO_ARGS,  "Access_field",       ARGP_ACCESSFIELD_OP,    ARGI_ACCESSFIELD_OP),
/*  69 */   /* AML_STATICSTRING_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_BOGUS|           AML_NO_ARGS,  "Static_string",      ARGP_STATICSTRING_OP,   ARGI_STATICSTRING_OP),
/*  6_a */  /* AML_RETURN_VALUE_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_RETURN|          AML_HAS_ARGS, "[Return Value]",     ARG_NONE,               ARG_NONE),
/*  6_b */  /* UNKNOWN OPCODES */	 OP_INFO_ENTRY (ACPI_OP_TYPE_UNKNOWN | OPTYPE_BOGUS|          AML_HAS_ARGS, "UNKNOWN_OP!",        ARG_NONE,               ARG_NONE),
/*  6_c */  /* ASCII CHARACTERS */	   OP_INFO_ENTRY (ACPI_OP_TYPE_ASCII  | OPTYPE_BOGUS|           AML_HAS_ARGS, "ASCII_ONLY!",        ARG_NONE,               ARG_NONE),
/*  6_d */  /* PREFIX CHARACTERS */	  OP_INFO_ENTRY (ACPI_OP_TYPE_PREFIX | OPTYPE_BOGUS|           AML_HAS_ARGS, "PREFIX_ONLY!",       ARG_NONE,               ARG_NONE),


/* ACPI 2.0 (new) opcodes */

/*  6_e */  /* AML_QWORD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_LITERAL|         AML_NO_ARGS,  "Qword_const",        ARGP_QWORD_OP,          ARGI_QWORD_OP),
/*  6_f */  /* AML_VAR_PACKAGE_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DATA_TERM|       AML_HAS_ARGS, "Var_package",        ARGP_VAR_PACKAGE_OP,    ARGI_VAR_PACKAGE_OP),
/*  70 */   /* AML_CONCAT_RES_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Concat_res",         ARGP_CONCAT_RES_OP,     ARGI_CONCAT_RES_OP),
/*  71 */   /* AML_MOD_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_DYADIC2_r|       AML_HAS_ARGS, "Mod",                ARGP_MOD_OP,            ARGI_MOD_OP),
/*  72 */   /* AML_QWORD_FIELD_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CREATE_FIELD|    AML_HAS_ARGS, "Create_qWord_field", ARGP_QWORD_FIELD_OP,    ARGI_QWORD_FIELD_OP),
/*  73 */   /* AML_TO_BUFFER_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_buffer",          ARGP_TO_BUFFER_OP,      ARGI_TO_BUFFER_OP),
/*  74 */   /* AML_TO_DEC_STR_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_dec_string",      ARGP_TO_DEC_STR_OP,     ARGI_TO_DEC_STR_OP),
/*  75 */   /* AML_TO_HEX_STR_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_hex_string",      ARGP_TO_HEX_STR_OP,     ARGI_TO_HEX_STR_OP),
/*  76 */   /* AML_TO_INTEGER_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_integer",         ARGP_TO_INTEGER_OP,     ARGI_TO_INTEGER_OP),
/*  77 */   /* AML_TO_STRING_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "To_string",          ARGP_TO_STRING_OP,      ARGI_TO_STRING_OP),
/*  78 */   /* AML_COPY_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Copy",               ARGP_COPY_OP,           ARGI_COPY_OP),
/*  79 */   /* AML_MID_OP */	   OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Mid",                ARGP_MID_OP,            ARGI_MID_OP),
/*  7_a */  /* AML_CONTINUE_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_CONTROL|         AML_NO_ARGS,  "Continue",           ARGP_CONTINUE_OP,       ARGI_CONTINUE_OP),
/*  7_b */  /* AML_LOAD_TABLE_OP */	  OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Load_table",         ARGP_LOAD_TABLE_OP,     ARGI_LOAD_TABLE_OP),
/*  7_c */  /* AML_DATA_REGION_OP */	 OP_INFO_ENTRY (ACPI_OP_TYPE_OPCODE | OPTYPE_MONADIC2_r|      AML_HAS_ARGS, "Data_op_region",     ARGP_DATA_REGION_OP,    ARGI_DATA_REGION_OP),

};

/*
 * This table is directly indexed by the opcodes, and returns an
 * index into the table above
 */

static u8 aml_short_op_info_index[256] =
{
/*              0     1     2     3     4     5     6     7  */
/*              8     9     A     B     C     D     E     F  */
/* 0x00 */	0x00, 0x01, _UNK, _UNK, _UNK, _UNK, 0x02, _UNK,
/* 0x08 */	0x03, _UNK, 0x04, 0x05, 0x06, 0x07, 0x6E, _UNK,
/* 0x10 */	0x08, 0x09, 0x0a, 0x6F, 0x0b, _UNK, _UNK, _UNK,
/* 0x18 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x20 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x28 */	_UNK, _UNK, _UNK, _UNK, _UNK, 0x63, _PFX, _PFX,
/* 0x30 */	0x67, 0x66, 0x68, 0x65, 0x69, 0x64, 0x6A, _UNK,
/* 0x38 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x40 */	_UNK, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x48 */	_ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x50 */	_ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/* 0x58 */	_ASC, _ASC, _ASC, _UNK, _PFX, _UNK, _PFX, _ASC,
/* 0x60 */	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
/* 0x68 */	0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, _UNK,
/* 0x70 */	0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
/* 0x78 */	0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
/* 0x80 */	0x2b, 0x2c, 0x2d, 0x2e, 0x70, 0x71, 0x2f, 0x30,
/* 0x88 */	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x72,
/* 0x90 */	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x73, 0x74,
/* 0x98 */	0x75, 0x76, _UNK, _UNK, 0x77, 0x78, 0x79, 0x7A,
/* 0xA0 */	0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x60, 0x61,
/* 0xA8 */	0x62, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xB0 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xB8 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xC0 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xC8 */	_UNK, _UNK, _UNK, _UNK, 0x44, _UNK, _UNK, _UNK,
/* 0xD0 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xD8 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xE0 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xE8 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xF0 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0xF8 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x45,
};


static u8 aml_long_op_info_index[NUM_EXTENDED_OPCODE] =
{
/*              0     1     2     3     4     5     6     7  */
/*              8     9     A     B     C     D     E     F  */
/* 0x00 */	_UNK, 0x46, 0x47, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x08 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x10 */	_UNK, _UNK, 0x48, 0x49, _UNK, _UNK, _UNK, _UNK,
/* 0x18 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x7B,
/* 0x20 */	0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
/* 0x28 */	0x52, 0x53, 0x54, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x30 */	0x55, 0x56, 0x57, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x38 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x40 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x48 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x50 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x58 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x60 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x68 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x70 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x78 */	_UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/* 0x80 */	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/* 0x88 */	0x7C,
};


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_opcode_info
 *
 * PARAMETERS:  Opcode              - The AML opcode
 *
 * RETURN:      A pointer to the info about the opcode.  NULL if the opcode was
 *              not found in the table.
 *
 * DESCRIPTION: Find AML opcode description based on the opcode.
 *              NOTE: This procedure must ALWAYS return a valid pointer!
 *
 ******************************************************************************/

ACPI_OPCODE_INFO *
acpi_ps_get_opcode_info (
	u16                     opcode)
{
	ACPI_OPCODE_INFO        *op_info;
	u8                      upper_opcode;
	u8                      lower_opcode;


	/* Split the 16-bit opcode into separate bytes */

	upper_opcode = (u8) (opcode >> 8);
	lower_opcode = (u8) opcode;

	/* Default is "unknown opcode" */

	op_info = &aml_op_info [_UNK];


	/*
	 * Detect normal 8-bit opcode or extended 16-bit opcode
	 */

	switch (upper_opcode) {
	case 0:

		/* Simple (8-bit) opcode: 0-255, can't index beyond table  */

		op_info = &aml_op_info [aml_short_op_info_index [lower_opcode]];
		break;


	case AML_EXTOP:

		/* Extended (16-bit, prefix+opcode) opcode */

		if (lower_opcode <= MAX_EXTENDED_OPCODE) {
			op_info = &aml_op_info [aml_long_op_info_index [lower_opcode]];
		}
		break;


	case AML_LNOT_OP:

		/* This case is for the bogus opcodes LNOTEQUAL, LLESSEQUAL, LGREATEREQUAL */
		/* TBD: [Investigate] remove this case? */

		break;


	default:

		break;
	}


	/* Get the Op info pointer for this opcode */

	return (op_info);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_opcode_name
 *
 * PARAMETERS:  Opcode              - The AML opcode
 *
 * RETURN:      A pointer to the name of the opcode (ASCII String)
 *              Note: Never returns NULL.
 *
 * DESCRIPTION: Translate an opcode into a human-readable string
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_ps_get_opcode_name (
	u16                     opcode)
{
	ACPI_OPCODE_INFO             *op;


	op = acpi_ps_get_opcode_info (opcode);

	/* Always guaranteed to return a valid pointer */

	return ("AE_NOT_CONFIGURED");
}


