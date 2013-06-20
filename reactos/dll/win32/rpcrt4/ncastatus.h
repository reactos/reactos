/*
 * NCA Status definitions
 *
 * Copyright 2007 Robert Shearman
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

#define NCA_S_COMM_FAILURE               0x1C010001
#define NCA_S_OP_RNG_ERROR               0x1C010002
#define NCA_S_UNK_IF                     0x1C010003
#define NCA_S_WRONG_BOOT_TIME            0x1C010006
#define NCA_S_YOU_CRASHED                0x1C010009
#define NCA_S_PROTO_ERROR                0x1C01000B
#define NCA_S_OUT_ARGS_TOO_BIG           0x1C010013
#define NCA_S_SERVER_TOO_BUSY            0x1C010014
#define NCA_S_FAULT_STRING_TOO_LONG      0x1C010015
#define NCA_S_UNSUPPORTED_TYPE           0x1C010017

#define NCA_S_FAULT_INT_DIV_BY_ZERO      0x1C000001
#define NCA_S_FAULT_ADDR_ERROR           0x1C000002
#define NCA_S_FAULT_FP_DIV_ZERO          0x1C000003
#define NCA_S_FAULT_FP_UNDERFLOW         0x1C000004
#define NCA_S_FAULT_FP_OVERFLOW          0x1C000005
#define NCA_S_FAULT_INVALID_TAG          0x1C000006
#define NCA_S_FAULT_INVALID_BOUND        0x1C000007
#define NCA_S_RPC_VERSION_MISMATCH       0x1C000008
#define NCA_S_UNSPEC_REJECT              0x1C000009
#define NCA_S_BAD_ACTID                  0x1C00000A
#define NCA_S_WHO_ARE_YOU_FAILED         0x1C00000B
#define NCA_S_MANAGER_NOT_ENTERED        0x1C00000C
#define NCA_S_FAULT_CANCEL               0x1C00000D
#define NCA_S_FAULT_ILL_INST             0x1C00000E
#define NCA_S_FAULT_FP_ERROR             0x1C00000F
#define NCA_S_FAULT_INT_OVERFLOW         0x1C000010
#define NCA_S_FAULT_UNSPEC               0x1C000012
#define NCA_S_FAULT_REMOTE_COMM_FAILURE  0x1C000013
#define NCA_S_FAULT_PIPE_EMPTY           0x1C000014
#define NCA_S_FAULT_PIPE_CLOSED          0x1C000015
#define NCA_S_FAULT_PIPE_ORDER           0x1C000016
#define NCA_S_FAULT_PIPE_DISCIPLINE      0x1C000017
#define NCA_S_FAULT_PIPE_COMM_ERROR      0x1C000018
#define NCA_S_FAULT_PIPE_MEMORY          0x1C000019
#define NCA_S_FAULT_CONTEXT_MISMATCH     0x1C00001A
#define NCA_S_FAULT_REMOTE_NO_MEMORY     0x1C00001B
#define NCA_S_INVALID_PRES_CONTEXT_ID    0x1C00001C
#define NCA_S_UNSUPPORTED_AUTHN_LEVEL    0x1C00001D
#define NCA_S_INVALID_CHECKSUM           0x1C00001F
#define NCA_S_INVALID_CRC                0x1C000020
#define NCA_S_FAULT_USER_DEFINED         0x1C000021
#define NCA_S_FAULT_TX_OPEN_FAILED       0x1C000022
#define NCA_S_FAULT_CODESET_CONV_ERROR   0x1C000023
#define NCA_S_FAULT_OBJECT_NOT_FOUND     0x1C000024
#define NCA_S_FAULT_NO_CLIENT_STUB       0x1C000025
