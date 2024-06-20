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
	public class SnmpScalarNodeUint : SnmpScalarNode
	{
		public SnmpScalarNodeUint(SnmpDataType dataType, SnmpTreeNode parentNode)
			: base(parentNode)
		{
			System.Diagnostics.Debug.Assert(
				(dataType == SnmpDataType.Counter) || 
				 (dataType == SnmpDataType.Gauge) || 
				 (dataType == SnmpDataType.TimeTicks));

			this.DataType = dataType;
		}

		protected override void GenerateTestMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			System.Diagnostics.Trace.Assert(this.Restrictions.Count > 0);

			StringBuilder ifCond = new StringBuilder();
			foreach (IRestriction restriction in this.Restrictions)
			{
				ifCond.Append(restriction.GetCheckCodeValid("*" + localValueVarName));
				ifCond.Append(" || ");

				localValueVarUsed = true;
			}

			ifCond.Length -= 4;

			IfThenElse ite = new IfThenElse(ifCond.ToString());
			ite.AddCode(String.Format("{0} = {1};", retErrVarName, LwipDefs.Def_ErrorCode_Ok));
			container.AddElement(ite);
		}

		protected override bool GenerateValueDeclaration(CodeContainerBase container, string variableName, string sourceName)
		{
			container.AddDeclaration(new VariableDeclaration(
				new VariableType(variableName, LwipDefs.Vt_U32, "*"),
				"(" + new VariableType(null, LwipDefs.Vt_U32, "*") + ")" + sourceName));

			return true;
		}

		public override string FixedValueLength
		{
			get { return String.Format("sizeof({0})", LwipDefs.Vt_U32); }
		}

		public override int OidRepresentationLen
		{
			get { return 1; }
		}
	}
}
