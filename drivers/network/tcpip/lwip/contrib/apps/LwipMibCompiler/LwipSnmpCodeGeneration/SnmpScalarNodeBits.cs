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
	public class SnmpScalarNodeBits : SnmpScalarNode
	{
		private readonly uint bitCount;

		public SnmpScalarNodeBits(SnmpTreeNode parentNode, uint bitCount)
			: base(parentNode)
		{
			this.DataType = SnmpDataType.Bits;
			this.bitCount = bitCount;
		}

		public override void GenerateGetMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string retLenVarName)
		{
			container.AddCode(String.Format(
				"{0} = snmp_encode_bits(({1} *){2}, SNMP_MAX_VALUE_SIZE, 0 /* TODO: pass real value here */, {3});",
				retLenVarName, 
				LwipDefs.Vt_U8,
				valueVarName,
				this.bitCount));
			
			valueVarUsed = true;
		}

		public override void GenerateTestMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			if (this.Restrictions.Count > 0)
			{
				const string bitVarName = "bits";

				container.Declarations.Add(new VariableDeclaration(new VariableType(bitVarName, LwipDefs.Vt_U32)));

				IfThenElse ite = new IfThenElse(String.Format(
					"snmp_decode_bits(({0} *){1}, {2}, &{3}) == ERR_OK",
					LwipDefs.Vt_U8,
					valueVarName,
					lenVarName,
					bitVarName));

				valueVarUsed = true;
				lenVarUsed = true;

				StringBuilder innerIfCond = new StringBuilder();
				foreach (IRestriction restriction in this.Restrictions)
				{
					innerIfCond.Append(restriction.GetCheckCodeValid(bitVarName));
					innerIfCond.Append(" || ");
				}

				innerIfCond.Length -= 4;

				IfThenElse innerIte = new IfThenElse(innerIfCond.ToString());
				innerIte.AddCode(String.Format("{0} = {1};", retErrVarName, LwipDefs.Def_ErrorCode_Ok));
				ite.AddElement(innerIte);
				container.AddElement(ite);
			}
			else
			{
				base.GenerateTestMethodCode(container, valueVarName, ref valueVarUsed, lenVarName, ref lenVarUsed, retErrVarName);
			}
		}

		public override void GenerateSetMethodCode(CodeContainerBase container, string valueVarName, ref bool valueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			const string bitVarName = "bits";

			container.Declarations.Add(new VariableDeclaration(new VariableType(bitVarName, LwipDefs.Vt_U32)));

			IfThenElse ite = new IfThenElse(String.Format(
				"snmp_decode_bits(({0} *){1}, {2}, &{3}) == ERR_OK",
				LwipDefs.Vt_U8,
				valueVarName,
				lenVarName,
				bitVarName));

			valueVarUsed = true;
			lenVarUsed = true;

			ite.AddElement(new Comment(String.Format("TODO: store new value contained in '{0}' here", bitVarName), singleLine: true));

			container.AddElement(ite);
		}
	}
}
