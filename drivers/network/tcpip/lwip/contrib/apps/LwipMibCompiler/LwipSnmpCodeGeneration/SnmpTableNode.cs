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
using System.Collections.Generic;
using System.Text;
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public class SnmpTableNode: SnmpScalarAggregationNode
	{
		private readonly List<SnmpScalarNode> cellNodes = new List<SnmpScalarNode>();
		private readonly List<SnmpScalarNode> indexNodes = new List<SnmpScalarNode>();
		private string augmentedTableRow = null;


		public SnmpTableNode(SnmpTreeNode parentNode)
			: base(parentNode)
		{
		}

		public List<SnmpScalarNode> CellNodes
		{
			get { return cellNodes; }
		}

		public List<SnmpScalarNode> IndexNodes
		{
			get { return indexNodes; }
		}

		public string AugmentedTableRow
		{
			get { return this.augmentedTableRow; }
			set { this.augmentedTableRow = value; }
		}

		public override string FullNodeName
		{
			get
			{
				string result = this.Name.ToLowerInvariant();
				if (!result.Contains("table"))
				{
					result += "_table";
				}

				return result;				
			}
		}

		protected override IEnumerable<SnmpScalarNode> AggregatedScalarNodes
		{
			get { return this.cellNodes; }
		}

		public override void GenerateCode(MibCFile mibFile)
		{
			FunctionDeclaration getInstanceMethodDecl = new FunctionDeclaration(this.FullNodeName + LwipDefs.FnctSuffix_GetInstance, isStatic: true);
			getInstanceMethodDecl.Parameter.Add(new VariableType("column", LwipDefs.Vt_U32, "*", ConstType.Value));
			getInstanceMethodDecl.Parameter.Add(new VariableType("row_oid", LwipDefs.Vt_U32, "*", ConstType.Value));
			getInstanceMethodDecl.Parameter.Add(new VariableType("row_oid_len", LwipDefs.Vt_U8, ""));
			getInstanceMethodDecl.Parameter.Add(new VariableType("cell_instance", LwipDefs.Vt_StNodeInstance, "*"));
			getInstanceMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);
			mibFile.Declarations.Add(getInstanceMethodDecl);

			Function getInstanceMethod = Function.FromDeclaration(getInstanceMethodDecl);
			GenerateGetInstanceMethodCode(getInstanceMethod);
			mibFile.Implementation.Add(getInstanceMethod);


			FunctionDeclaration getNextInstanceMethodDecl = new FunctionDeclaration(this.FullNodeName + LwipDefs.FnctSuffix_GetNextInstance, isStatic: true);
			getNextInstanceMethodDecl.Parameter.Add(new VariableType("column", LwipDefs.Vt_U32, "*", ConstType.Value));
			getNextInstanceMethodDecl.Parameter.Add(new VariableType("row_oid", LwipDefs.Vt_StObjectId, "*"));
			getNextInstanceMethodDecl.Parameter.Add(new VariableType("cell_instance", LwipDefs.Vt_StNodeInstance, "*"));
			getNextInstanceMethodDecl.ReturnType = new VariableType(null, LwipDefs.Vt_Snmp_err);
			mibFile.Declarations.Add(getNextInstanceMethodDecl);

			Function getNextInstanceMethod = Function.FromDeclaration(getNextInstanceMethodDecl);
			GenerateGetNextInstanceMethodCode(getNextInstanceMethod);
			mibFile.Implementation.Add(getNextInstanceMethod);

			
			VariableType instanceType = new VariableType("cell_instance", LwipDefs.Vt_StNodeInstance, "*");
			GenerateAggregatedCode(
				mibFile,
				instanceType,
				String.Format("SNMP_TABLE_GET_COLUMN_FROM_OID({0}->instance_oid.id)", instanceType.Name));


			#region create and add column/table definitions

			StringBuilder colDefs = new StringBuilder();
			foreach (SnmpScalarNode colNode in this.cellNodes)
			{
				colDefs.AppendFormat("  {{{0}, {1}, {2}}}, /* {3} */ \n",
					colNode.Oid,
					LwipDefs.GetAsn1DefForSnmpDataType(colNode.DataType),
					LwipDefs.GetLwipDefForSnmpAccessMode(colNode.AccessMode),
					colNode.Name);
			}
			if (colDefs.Length > 0)
			{
				colDefs.Length--;
			}

			VariableDeclaration colDefsDecl = new VariableDeclaration(
				new VariableType(this.FullNodeName + "_columns", LwipDefs.Vt_StTableColumnDef, null, ConstType.Value, String.Empty),
				"{\n" + colDefs + "\n}",
				isStatic: true);

			mibFile.Declarations.Add(colDefsDecl);

			string nodeInitialization = String.Format("SNMP_TABLE_CREATE({0}, {1}, {2}, {3}, {4}, {5}, {6})",
				this.Oid,
				colDefsDecl.Type.Name,
				getInstanceMethodDecl.Name, getNextInstanceMethodDecl.Name,
				(this.GetMethodRequired) ? this.GetMethodName : LwipDefs.Null,
				(this.TestMethodRequired) ? this.TestMethodName : LwipDefs.Null,
				(this.SetMethodRequired) ? this.SetMethodName : LwipDefs.Null
				);

			mibFile.Declarations.Add(new VariableDeclaration(
				new VariableType(this.FullNodeName, LwipDefs.Vt_StTableNode, null, ConstType.Value),
				nodeInitialization,
				isStatic: true));
							
			#endregion
		}

		protected virtual void GenerateGetInstanceMethodCode(Function getInstanceMethod)
		{
			VariableDeclaration returnValue = new VariableDeclaration((VariableType)getInstanceMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_NoSuchInstance);
			returnValue.Type.Name = "err";
			getInstanceMethod.Declarations.Add(returnValue);

			int instanceOidLength = 0;
			StringBuilder indexColumns = new StringBuilder();
			foreach (SnmpScalarNode indexNode in this.indexNodes)
			{
				if (instanceOidLength >= 0)
				{
					if (indexNode.OidRepresentationLen >= 0)
					{
						instanceOidLength += indexNode.OidRepresentationLen;
					}
					else
					{
						// at least one index column has a variable length -> we cannot perform a static check
						instanceOidLength = -1;
					}
				}

				indexColumns.AppendFormat(
					" {0} ({1}, OID length = {2})\n",
					indexNode.Name,
					indexNode.DataType,
					(indexNode.OidRepresentationLen >= 0) ? indexNode.OidRepresentationLen.ToString() : "variable");
			}
			if (indexColumns.Length > 0)
			{
				indexColumns.Length--;

				getInstanceMethod.Declarations.Insert(0, new Comment(String.Format(
					"The instance OID of this table consists of following (index) column(s):\n{0}",
					indexColumns)));
			}

			string augmentsHint = "";
			if (!String.IsNullOrWhiteSpace(this.augmentedTableRow))
			{
				augmentsHint = String.Format(
					"This table augments table '{0}'! Index columns therefore belong to table '{0}'!\n" + 
					"You may simply call the '*{1}' method of this table.\n\n",
					(this.augmentedTableRow.ToLowerInvariant().EndsWith("entry")) ? this.augmentedTableRow.Substring(0, this.augmentedTableRow.Length-5) : this.augmentedTableRow,
					LwipDefs.FnctSuffix_GetInstance);
			}

			CodeContainerBase ccb = getInstanceMethod;
			if (instanceOidLength > 0)
			{
				IfThenElse ite = new IfThenElse(String.Format("{0} == {1}", getInstanceMethod.Parameter[2].Name, instanceOidLength));
				getInstanceMethod.AddElement(ite);
				ccb = ite;
			}

			ccb.AddCodeFormat("LWIP_UNUSED_ARG({0});", getInstanceMethod.Parameter[0].Name);
			ccb.AddCodeFormat("LWIP_UNUSED_ARG({0});", getInstanceMethod.Parameter[1].Name);
			if (instanceOidLength <= 0)
			{
				ccb.AddCodeFormat("LWIP_UNUSED_ARG({0});", getInstanceMethod.Parameter[2].Name);
			}
			ccb.AddCodeFormat("LWIP_UNUSED_ARG({0});", getInstanceMethod.Parameter[3].Name);

			ccb.AddElement(new Comment(String.Format(
				"TODO: check if '{0}'/'{1}' params contain a valid instance oid for a row\n" +
				"If so, set '{2} = {3};'\n\n" + 
				"snmp_oid_* methods may be used for easier processing of oid\n\n" + 
				"{4}" + 
				"In order to avoid decoding OID a second time in subsequent get_value/set_test/set_value methods,\n" +
				"you may store an arbitrary value (like a pointer to target value object) in '{5}->reference'/'{5}->reference_len'.\n" +
				"But be aware that not always a subsequent method is called -> Do NOT allocate memory here and try to release it in subsequent methods!\n\n" +
				"You also may replace function pointers in '{5}' param for get/test/set methods which contain the default values from table definition,\n" +
				"in order to provide special methods, for the currently processed cell. Changed pointers are only valid for current request.",
				getInstanceMethod.Parameter[1].Name,
				getInstanceMethod.Parameter[2].Name,
				returnValue.Type.Name,
				LwipDefs.Def_ErrorCode_Ok,
				augmentsHint,
				getInstanceMethod.Parameter[3].Name
				)));

			getInstanceMethod.AddCodeFormat("return {0};", returnValue.Type.Name);
		}

		protected virtual void GenerateGetNextInstanceMethodCode(Function getNextInstanceMethod)
		{
			getNextInstanceMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getNextInstanceMethod.Parameter[0].Name);
			getNextInstanceMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getNextInstanceMethod.Parameter[1].Name);
			getNextInstanceMethod.AddCodeFormat("LWIP_UNUSED_ARG({0});", getNextInstanceMethod.Parameter[2].Name);

			VariableDeclaration returnValue = new VariableDeclaration((VariableType)getNextInstanceMethod.ReturnType.Clone(), LwipDefs.Def_ErrorCode_NoSuchInstance);
			returnValue.Type.Name = "err";
			getNextInstanceMethod.Declarations.Add(returnValue);

			StringBuilder indexColumns = new StringBuilder();
			foreach (SnmpScalarNode indexNode in this.indexNodes)
			{
				indexColumns.AppendFormat(
					" {0} ({1}, OID length = {2})\n",
					indexNode.Name,
					indexNode.DataType,
					(indexNode.OidRepresentationLen >= 0) ? indexNode.OidRepresentationLen.ToString() : "variable");
			}
			if (indexColumns.Length > 0)
			{
				indexColumns.Length--;

				getNextInstanceMethod.Declarations.Insert(0, new Comment(String.Format(
					"The instance OID of this table consists of following (index) column(s):\n{0}",
					indexColumns)));
			}

			string augmentsHint = "";
			if (!String.IsNullOrWhiteSpace(this.augmentedTableRow))
			{
				augmentsHint = String.Format(
					"This table augments table '{0}'! Index columns therefore belong to table '{0}'!\n" + 
					"You may simply call the '*{1}' method of this table.\n\n",
					(this.augmentedTableRow.ToLowerInvariant().EndsWith("entry")) ? this.augmentedTableRow.Substring(0, this.augmentedTableRow.Length-5) : this.augmentedTableRow,
					LwipDefs.FnctSuffix_GetNextInstance);
			}

			getNextInstanceMethod.AddElement(new Comment(String.Format(
				"TODO: analyze '{0}->id'/'{0}->len' and return the subsequent row instance\n" +
				"Be aware that '{0}->id'/'{0}->len' must not point to a valid instance or have correct instance length.\n" +
				"If '{0}->len' is 0, return the first instance. If '{0}->len' is longer than expected, cut superfluous OID parts.\n" +
				"If a valid next instance is found, store it in '{0}->id'/'{0}->len' and set '{1} = {2};'\n\n" + 
				"snmp_oid_* methods may be used for easier processing of oid\n\n" + 
				"{3}" + 
				"In order to avoid decoding OID a second time in subsequent get_value/set_test/set_value methods,\n" +
				"you may store an arbitrary value (like a pointer to target value object) in '{4}->reference'/'{4}->reference_len'.\n" +
				"But be aware that not always a subsequent method is called -> Do NOT allocate memory here and try to release it in subsequent methods!\n\n" +
				"You also may replace function pointers in '{4}' param for get/test/set methods which contain the default values from table definition,\n" +
				"in order to provide special methods, for the currently processed cell. Changed pointers are only valid for current request.",
				getNextInstanceMethod.Parameter[1].Name,
				returnValue.Type.Name,
				LwipDefs.Def_ErrorCode_Ok,
				augmentsHint,
				getNextInstanceMethod.Parameter[2].Name
				)));

			getNextInstanceMethod.AddElement(new Comment(String.Format(
				"For easier processing and getting the next instance, you may use the 'snmp_next_oid_*' enumerator.\n" +
				"Simply pass all known instance OID's to it and it returns the next valid one:\n\n" +
				"{0} state;\n" +
				"{1} result_buf;\n" +
				"snmp_next_oid_init(&state, {2}->id, {2}->len, result_buf.id, SNMP_MAX_OBJ_ID_LEN);\n" + 
				"while ({{not all instances passed}}) {{\n" + 
				"  {1} test_oid;\n" + 
				"  {{fill test_oid to create instance oid for next instance}}\n" + 
				"  snmp_next_oid_check(&state, test_oid.id, test_oid.len, {{target_data_ptr}});\n" + 
				"}}\n" + 
				"if(state.status == SNMP_NEXT_OID_STATUS_SUCCESS) {{\n" + 
				"  snmp_oid_assign(row_oid, state.next_oid, state.next_oid_len);\n" +
				"  {3}->reference.ptr = state.reference; //==target_data_ptr, for usage in subsequent get/test/set\n" + 
				"  {4} = {5};\n" + 
				"}}"
				,
				LwipDefs.Vt_StNextOidState,
				LwipDefs.Vt_StObjectId,
				getNextInstanceMethod.Parameter[1].Name,
				getNextInstanceMethod.Parameter[2].Name,
				returnValue.Type.Name,
				LwipDefs.Def_ErrorCode_Ok
				)));

			getNextInstanceMethod.AddCodeFormat("return {0};", returnValue.Type.Name);
		}

	}
}
