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
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public class SnmpScalarNodeObjectIdentifier: SnmpScalarNode
	{
		public SnmpScalarNodeObjectIdentifier(SnmpTreeNode parentNode)
			: base(parentNode)
		{
			this.DataType = SnmpDataType.ObjectIdentifier;
		}

		protected override bool GenerateValueDeclaration(CodeContainerBase container, string variableName, string sourceName)
		{
			container.AddDeclaration(new VariableDeclaration(
				new VariableType(variableName, LwipDefs.Vt_U32, "*"),
				"(" + new VariableType(null, LwipDefs.Vt_U32, "*") + ")" + sourceName));

			return true;
		}

		protected override void GenerateGetMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string retLenVarName)
		{
			container.AddElement(new Comment(String.Format("TODO: put requested value to '*{0}' here. '{0}' has to be interpreted as {1}[]", localValueVarName, LwipDefs.Vt_U32), singleLine: true));
			container.AddElement(EmptyLine.SingleLine);
			container.AddCode(String.Format("{0} = 0; // TODO: return real value length here (should be 'numOfElements * sizeof({1})')", retLenVarName, LwipDefs.Vt_U32));
		}

		protected override void GenerateTestMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			VariableDeclaration objIdLenVar = new VariableDeclaration(
				new VariableType(localValueVarName + "_len", LwipDefs.Vt_U8),
				String.Format("{0} / sizeof({1})", lenVarName, LwipDefs.Vt_U32));
			lenVarUsed = true;

			container.Declarations.Add(objIdLenVar);

			base.GenerateTestMethodCodeCore(container, localValueVarName, ref localValueVarUsed, lenVarName, ref lenVarUsed, retErrVarName);

			container.AddCode(String.Format("LWIP_UNUSED_ARG({0});", objIdLenVar.Type.Name));
		}

		protected override void GenerateSetMethodCodeCore(CodeContainerBase container, string localValueVarName, ref bool localValueVarUsed, string lenVarName, ref bool lenVarUsed, string retErrVarName)
		{
			VariableDeclaration objIdLenVar = new VariableDeclaration(
				new VariableType(localValueVarName + "_len", LwipDefs.Vt_U8),
				String.Format("{0} / sizeof({1})", lenVarName, LwipDefs.Vt_U32));
			lenVarUsed = true;

			container.Declarations.Add(objIdLenVar);

			base.GenerateSetMethodCodeCore(container, localValueVarName, ref localValueVarUsed, lenVarName, ref lenVarUsed, retErrVarName);

			container.AddCode(String.Format("LWIP_UNUSED_ARG({0});", objIdLenVar.Type.Name));
		}
	}
}
