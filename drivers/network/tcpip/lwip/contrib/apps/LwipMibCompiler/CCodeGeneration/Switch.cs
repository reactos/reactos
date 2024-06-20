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
	public class SwitchCase : CodeContainerBase
	{
		public string Value { get; set; }

		public SwitchCase()
		{
		}

		public SwitchCase(string value)
		{
			this.Value = value;
		}

		public bool IsDefault
		{
			get { return (this.Value.ToLowerInvariant() == "default"); }
		}

		public static SwitchCase GenerateDefault()
		{
			return new SwitchCase("default");
		}

		public override void GenerateCode(int level, CGenerator generator)
		{
			if (!String.IsNullOrWhiteSpace(this.Value))
			{
				generator.IndentLine(level);
				if (this.IsDefault)
				{
					generator.OutputStream.Write("default:");
				}
				else
				{
					generator.OutputStream.Write(String.Format("case {0}:", this.Value));
				}
				generator.WriteNewLine();
				generator.IndentLine(level + 1);
				generator.OutputStream.Write("{");
				generator.WriteNewLine();

				base.GenerateCode(level + 1, generator);

				generator.IndentLine(level + 1);
				generator.OutputStream.Write("}");
				generator.WriteNewLine();

				generator.IndentLine(level + 1);
				generator.OutputStream.Write("break;");
				generator.WriteNewLine();
			}
		}
	}

	public class Switch: CodeElement
	{
		public string SwitchVar { get; set; }

		private List<SwitchCase> switches = new List<SwitchCase>();

		public Switch()
		{
		}

		public Switch(string switchVar)
		{
			this.SwitchVar = switchVar;
		}

		public List<SwitchCase> Switches
		{
			get { return this.switches; }
		}

		public override void GenerateCode(int level, CGenerator generator)
		{
			if (!String.IsNullOrWhiteSpace(this.SwitchVar))
			{
				generator.IndentLine(level);
				generator.OutputStream.Write(String.Format("switch ({0})", this.SwitchVar));
				generator.WriteNewLine();
				generator.IndentLine(level);
				generator.OutputStream.Write("{");
				generator.WriteNewLine();

				SwitchCase defaultCase = null; // generate 'default' always as last case
				foreach (SwitchCase switchCase in this.switches)
				{
					if (switchCase.IsDefault)
					{
						defaultCase = switchCase;
					}
					else
					{
						switchCase.GenerateCode(level + 1, generator);
					}
				}
				if (defaultCase != null)
				{
					defaultCase.GenerateCode(level + 1, generator);
				}

				generator.IndentLine(level);
				generator.OutputStream.Write("}");
				generator.WriteNewLine();
			}
		}
	}
}
