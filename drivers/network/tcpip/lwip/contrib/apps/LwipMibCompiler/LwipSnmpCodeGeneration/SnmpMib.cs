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
using System.Text;
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public class SnmpMib : SnmpTreeNode
	{
		public uint[] BaseOid { get; set; }

		public SnmpMib()
			: base(null)
		{
		}

		public SnmpMib(uint[] baseOid)
			: base(null)
		{
			this.BaseOid = baseOid;
		}

		public override string FullNodeName
		{
			get { return this.Name.ToLowerInvariant() + "_root"; }
		}

		public override void GenerateCode(MibCFile mibFile)
		{
			base.GenerateCode(mibFile);

			System.Diagnostics.Debug.Assert((this.BaseOid != null) && (this.BaseOid.Length > 0));
			
			// create and add BaseOID declarations
			StringBuilder boidInitialization = new StringBuilder("{");
			foreach (uint t in this.BaseOid)
			{
				boidInitialization.Append(t);
				boidInitialization.Append(",");
			}
			boidInitialization.Length -= 1;
			boidInitialization.Append("}");

			VariableDeclaration boidDecl = new VariableDeclaration(
				new VariableType(this.Name.ToLowerInvariant() + "_base_oid", LwipDefs.Vt_U32, null, ConstType.Value, String.Empty),
				boidInitialization.ToString(), true);

			mibFile.Declarations.Add(boidDecl);
			mibFile.Declarations.Add(GetExportDeclaration());
		}

		public override void GenerateHeaderCode(MibHeaderFile mibHeaderFile)
		{
			mibHeaderFile.Includes.Add(new PP_Include("lwip/apps/snmp_core.h"));

			mibHeaderFile.VariableDeclarations.Add(VariablePrototype.FromVariableDeclaration(GetExportDeclaration()));
		}

		VariableDeclaration GetExportDeclaration()
		{
			return new VariableDeclaration(
				new VariableType(this.Name.ToLowerInvariant(), LwipDefs.Vt_StMib, null, ConstType.Value),
				String.Format("{{{0}_base_oid, LWIP_ARRAYSIZE({0}_base_oid), &{1}.node}}", this.Name.ToLowerInvariant(), this.FullNodeName));
		}
	}
}
