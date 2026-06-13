/*
 * Copyright 2009 Piotr Caban
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windef.h>

#define JSCRIPT_MAJOR_VERSION 5
#define JSCRIPT_MINOR_VERSION 8
#define JSCRIPT_BUILD_VERSION 16475

#define IDS_TO_PRIMITIVE                    0x0001
#define IDS_INVALID_CALL_ARG                0x0005
#define IDS_SUBSCRIPT_OUT_OF_RANGE          0x0009
#define IDS_STACK_OVERFLOW                  0x001C
#define IDS_OBJECT_REQUIRED                 0x01A8
#define IDS_CREATE_OBJ_ERROR                0x01AD
#define IDS_NO_PROPERTY                     0x01B6
#define IDS_UNSUPPORTED_ACTION              0x01BD
#define IDS_ARG_NOT_OPT                     0x01c1
#define IDS_OBJECT_NOT_COLLECTION           0x01c3
#define IDS_SYNTAX_ERROR                    0x03EA
#define IDS_SEMICOLON                       0x03EC
#define IDS_LBRACKET                        0x03ED
#define IDS_RBRACKET                        0x03EE
#define IDS_EXPECTED_IDENTIFIER             0x03f2
#define IDS_EXPECTED_ASSIGN                 0x03f3
#define IDS_INVALID_CHAR                    0x03F6
#define IDS_UNTERMINATED_STR                0x03F7
#define IDS_MISPLACED_RETURN                0x03FA
#define IDS_INVALID_BREAK                   0x03FB
#define IDS_INVALID_CONTINUE                0x03FC
#define IDS_LABEL_REDEFINED                 0x0401
#define IDS_LABEL_NOT_FOUND                 0x0402
#define IDS_EXPECTED_CCEND                  0x0405
#define IDS_DISABLED_CC                     0x0406
#define IDS_EXPECTED_AT                     0x0408
#define IDS_NOT_FUNC                        0x138A
#define IDS_NOT_DATE                        0x138E
#define IDS_NOT_NUM                         0x1389
#define IDS_OBJECT_EXPECTED                 0x138F
#define IDS_ILLEGAL_ASSIGN                  0x1390
#define IDS_UNDEFINED                       0x1391
#define IDS_NOT_BOOL                        0x1392
#define IDS_INVALID_DELETE                  0x1394
#define IDS_NOT_VBARRAY                     0x1395
#define IDS_JSCRIPT_EXPECTED                0x1396
#define IDS_ENUMERATOR_EXPECTED             0x1397
#define IDS_REGEXP_EXPECTED                 0x1398
#define IDS_REGEXP_SYNTAX_ERROR             0x1399
#define IDS_UNEXPECTED_QUANTIFIER           0x139A
#define IDS_EXCEPTION_THROWN                0x139E
#define IDS_URI_INVALID_CHAR                0x13A0
#define IDS_URI_INVALID_CODING              0x13A1
#define IDS_FRACTION_DIGITS_OUT_OF_RANGE    0x13A2
#define IDS_PRECISION_OUT_OF_RANGE          0x13A3
#define IDS_INVALID_LENGTH                  0x13A5
#define IDS_ARRAY_EXPECTED                  0x13A7
#define IDS_INVALID_WRITABLE_PROP_DESC      0x13AC
#define IDS_CYCLIC_PROTO_VALUE              0x13B0
#define IDS_CREATE_FOR_NONEXTENSIBLE        0x13B6
#define IDS_OBJECT_NONEXTENSIBLE            0x13D5
#define IDS_NONCONFIGURABLE_REDEFINED       0x13D6
#define IDS_NONWRITABLE_MODIFIED            0x13D7
#define IDS_NOT_DATAVIEW                    0x13DF
#define IDS_DATAVIEW_NO_ARGUMENT            0x13E0
#define IDS_DATAVIEW_INVALID_ACCESS         0x13E1
#define IDS_DATAVIEW_INVALID_OFFSET         0x13E2
#define IDS_WRONG_THIS                      0x13FC
#define IDS_KEY_NOT_OBJECT                  0x13FD
#define IDS_ARRAYBUFFER_EXPECTED            0x15E4
/* FIXME: This is not compatible with native, but we would
 * conflict with IDS_UNSUPPORTED_ACTION otherwise */
#define IDS_PROP_DESC_MISMATCH              0x1F00

#define IDS_COMPILATION_ERROR               0x1000
#define IDS_RUNTIME_ERROR                   0x1001
#define IDS_UNKNOWN_ERROR                   0x1002
