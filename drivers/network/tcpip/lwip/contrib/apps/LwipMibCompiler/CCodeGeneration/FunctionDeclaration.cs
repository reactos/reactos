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

namespace CCodeGeneration
{
	public class FunctionDeclaration: CodeElement
	{
		public string Name { get; set; }
		public bool IsStatic { get; set; }
		public bool IsExtern { get; set; }

		private readonly List<VariableType> parameter = new List<VariableType>();
		private VariableType returnType = VariableType.Void;

		public FunctionDeclaration()
		{
		}

		public FunctionDeclaration(string name, bool isStatic = false, bool isExtern = false)
		{
			this.Name = name;
			this.IsStatic = isStatic;
			this.IsExtern = isExtern;
		}

		public List<VariableType> Parameter
		{
			get { return this.parameter; }
		}

		public VariableType ReturnType
		{
			get { return this.returnType; }
			set
			{
				if (value == null)
				{
					throw new ArgumentNullException("ReturnValue");
				}
				this.returnType = value;
			}
		}

		public override void GenerateCode(int level, CGenerator generator)
		{
			generator.IndentLine(level);

			if (this.IsExtern)
			{
				generator.OutputStream.Write("extern ");
			}

			if (this.IsStatic)
			{
				generator.OutputStream.Write("static ");
			}

			this.returnType.GenerateCode(generator);
			generator.OutputStream.Write(" " + this.Name + "(");

			if (this.Parameter.Count > 0)
			{
				for (int i = 0; i < this.parameter.Count; i++)
				{
					this.parameter[i].GenerateCode(generator);

					if (i < (this.parameter.Count - 1))
					{
						generator.OutputStream.Write(", ");
					}
				}
			}
			else
			{
				generator.OutputStream.Write("void");
			}

			generator.OutputStream.Write(");");
			generator.WriteNewLine();
		}
	}
}
