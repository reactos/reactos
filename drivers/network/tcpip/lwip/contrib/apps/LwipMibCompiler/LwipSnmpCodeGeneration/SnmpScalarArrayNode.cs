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
	public class SnmpScalarArrayNode : SnmpScalarAggregationNode
	{
		private readonly List<SnmpScalarNode> scalarNodes;

		public SnmpScalarArrayNode(List<SnmpScalarNode> scalarNodes, SnmpTreeNode parentNode)
			: base(parentNode)
		{
			this.scalarNodes = scalarNodes;
		}

		public override string FullNodeName
		{
			get { return this.Name.ToLowerInvariant() + "_scalars"; }
		}

		protected override IEnumerable<SnmpScalarNode> AggregatedScalarNodes
		{
			get { return this.scalarNodes; }
		}

		public override void GenerateCode(MibCFile mibFile)
		{
			VariableType instanceType = new VariableType("node", LwipDefs.Vt_StScalarArrayNodeDef, "*", ConstType.Value);
			GenerateAggregatedCode(
				mibFile,
				instanceType,
				instanceType.Name + "->oid");


			// create and add node definitions
			StringBuilder nodeDefs = new StringBuilder();
			foreach (SnmpScalarNode scalarNode in this.scalarNodes)
			{
				nodeDefs.AppendFormat("  {{{0}, {1}, {2}}}, /* {3} */ \n",
					scalarNode.Oid,
					LwipDefs.GetAsn1DefForSnmpDataType(scalarNode.DataType),
					LwipDefs.GetLwipDefForSnmpAccessMode(scalarNode.AccessMode),
					scalarNode.Name);
			}
			if (nodeDefs.Length > 0)
				nodeDefs.Length--;

			VariableDeclaration nodeDefsDecl = new VariableDeclaration(
				new VariableType(this.FullNodeName + "_nodes", LwipDefs.Vt_StScalarArrayNodeDef, null, ConstType.Value, String.Empty),
				"{\n" + nodeDefs + "\n}" ,
				isStatic: true);

			mibFile.Declarations.Add(nodeDefsDecl);


			// create and add node declaration
			string nodeInitialization = String.Format("SNMP_SCALAR_CREATE_ARRAY_NODE({0}, {1}, {2}, {3}, {4})",
				this.Oid,
				nodeDefsDecl.Type.Name,
				(this.GetMethodRequired) ? this.GetMethodName : LwipDefs.Null,
				(this.TestMethodRequired) ? this.TestMethodName : LwipDefs.Null,
				(this.SetMethodRequired) ? this.SetMethodName : LwipDefs.Null
				);

			mibFile.Declarations.Add(new VariableDeclaration(
				new VariableType(this.FullNodeName, LwipDefs.Vt_StScalarArrayNode, null, ConstType.Value),
				nodeInitialization,
				isStatic: true));
		}
	}
}
