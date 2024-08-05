/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Martin Hentschel <info@cl-soft.de>
 *
 */

using System;

namespace LwipSnmpCodeGeneration
{
	public static class LwipOpts
	{
		public static bool GenerateEmptyFolders = false;
		/// <summary>
		/// If a tree node only has scalar nodes as child nodes, it is replaced by
		/// a single scalar array node in order to save memory and have only one single get/test/set method for all scalars.
		/// </summary>
		public static bool GenerateScalarArrays = true;
		/// <summary>
		/// If a tree node has multiple scalars as subnodes as well as other treenodes it
		/// defines a single get/test/set method for all scalar child node.
		/// (without other treenodes as child it would have been converted to scalar array node).
		/// </summary>
		public static bool GenerateSingleAccessMethodsForTreeNodeScalars = GenerateScalarArrays;
	}

	public static class LwipDefs
	{
		public const string Null        = "NULL";
		public const string Vt_U8       = "u8_t";
		public const string Vt_U16      = "u16_t";
		public const string Vt_U32      = "u32_t";
		public const string Vt_S8       = "s8_t";
		public const string Vt_S16      = "s16_t";
		public const string Vt_S32      = "s32_t";
		public const string Vt_Snmp_err = "snmp_err_t";

		public const string Incl_SnmpOpts = "lwip/apps/snmp_opts.h";
		public const string Opt_SnmpEnabled = "LWIP_SNMP";

		public const string Vt_StMib                = "struct snmp_mib";
		public const string Vt_StObjectId           = "struct snmp_obj_id";
		public const string Vt_StNode               = "struct snmp_node";
		public const string Vt_StNodeInstance       = "struct snmp_node_instance";
		public const string Vt_StTreeNode           = "struct snmp_tree_node";
		public const string Vt_StScalarNode         = "struct snmp_scalar_node";
		public const string Vt_StScalarArrayNode    = "struct snmp_scalar_array_node";
		public const string Vt_StScalarArrayNodeDef = "struct snmp_scalar_array_node_def";
		public const string Vt_StTableNode          = "struct snmp_table_node";
		public const string Vt_StTableColumnDef     = "struct snmp_table_col_def";
		public const string Vt_StNextOidState       = "struct snmp_next_oid_state";

		public const string Def_NodeAccessReadOnly      = "SNMP_NODE_INSTANCE_READ_ONLY";
		public const string Def_NodeAccessReadWrite     = "SNMP_NODE_INSTANCE_READ_WRITE";
		public const string Def_NodeAccessWriteOnly     = "SNMP_NODE_INSTANCE_WRITE_ONLY";
		public const string Def_NodeAccessNotAccessible = "SNMP_NODE_INSTANCE_NOT_ACCESSIBLE";

		public const string Def_ErrorCode_Ok             = "SNMP_ERR_NOERROR";
		public const string Def_ErrorCode_WrongValue     = "SNMP_ERR_WRONGVALUE";
		public const string Def_ErrorCode_NoSuchInstance = "SNMP_ERR_NOSUCHINSTANCE";

		public const string FnctSuffix_GetValue        = "_get_value";
		public const string FnctSuffix_SetTest         = "_set_test";
		public const string FnctSuffix_SetValue        = "_set_value";
		public const string FnctSuffix_GetInstance     = "_get_instance";
		public const string FnctSuffix_GetNextInstance = "_get_next_instance";

		public const string FnctName_SetTest_Ok         = "snmp_set_test_ok";

		public static string GetLwipDefForSnmpAccessMode(SnmpAccessMode am)
		{
			switch (am)
			{
				case SnmpAccessMode.ReadOnly: return Def_NodeAccessReadOnly;
				case SnmpAccessMode.ReadWrite: return Def_NodeAccessReadWrite;
				case SnmpAccessMode.NotAccessible: return Def_NodeAccessNotAccessible;
				case SnmpAccessMode.WriteOnly: return Def_NodeAccessWriteOnly;
				default: throw new NotSupportedException("Unknown SnmpAccessMode!");
			}
		}

		public static string GetAsn1DefForSnmpDataType(SnmpDataType dt)
		{
			switch (dt)
			{
				// primitive
				case SnmpDataType.Null:
					return "SNMP_ASN1_TYPE_NULL";
				case SnmpDataType.Bits:
				case SnmpDataType.OctetString:
					return "SNMP_ASN1_TYPE_OCTET_STRING";
				case SnmpDataType.ObjectIdentifier:
					return "SNMP_ASN1_TYPE_OBJECT_ID";				
				case SnmpDataType.Integer:
					return "SNMP_ASN1_TYPE_INTEGER";

				// application
				case SnmpDataType.IpAddress:
					return "SNMP_ASN1_TYPE_IPADDR";
				case SnmpDataType.Counter:
					return "SNMP_ASN1_TYPE_COUNTER";
				case SnmpDataType.Gauge:
					return "SNMP_ASN1_TYPE_GAUGE";
				case SnmpDataType.TimeTicks:
					return "SNMP_ASN1_TYPE_TIMETICKS";
				case SnmpDataType.Opaque:
					return "SNMP_ASN1_TYPE_OPAQUE";
				case SnmpDataType.Counter64:
					return "SNMP_ASN1_TYPE_COUNTER64";
				default:
					throw new NotSupportedException("Unknown SnmpDataType!");
			}
		}

		public static string GetLengthForSnmpDataType(SnmpDataType dt)
		{
			switch (dt)
			{
				case SnmpDataType.Null:
					return "0";

				case SnmpDataType.Integer:
				case SnmpDataType.Counter:
				case SnmpDataType.IpAddress:
				case SnmpDataType.Gauge:
				case SnmpDataType.TimeTicks:
					return "4";

				case SnmpDataType.Counter64:
					return "8";

				case SnmpDataType.OctetString:
				case SnmpDataType.ObjectIdentifier:
				case SnmpDataType.Bits:
				case SnmpDataType.Opaque:
					return null;

				default:
					throw new NotSupportedException("Unknown SnmpDataType!");
			}
		}
	}

	public enum SnmpDataType
	{
		Null,

		Integer, // INTEGER, Integer32

		Counter, // Counter, Counter32
		Gauge,  // Gauge, Gauge32, Unsigned32
		TimeTicks,

		Counter64,

		OctetString,
		Opaque,
		Bits,

		ObjectIdentifier,
		
		IpAddress,
	}

	public enum SnmpAccessMode
	{
		ReadOnly,
		ReadWrite,
		WriteOnly,
		NotAccessible
	}

}
