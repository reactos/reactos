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

namespace CCodeGeneration
{
	public enum ConstType
	{
		None,
		Value,
		Indirection,
		Both
	}

	public class VariableType : ICloneable
	{
		public const string VoidString = "void";
		public static readonly VariableType Void = new VariableType(null, "void");

		public string Name { get; set; }
		public string Type { get; set; }
		public string Indirection { get; set; }
		public ConstType Const { get; set; }
		public string ArraySpecifier { get; set; }

		public VariableType()
		{
		}

		public VariableType(string name, string type, string indirection = null, ConstType const_ = ConstType.None, string arraySpecifier = null)
		{
			this.Name           = name;
			this.Type           = type;
			this.Indirection    = indirection;
			this.Const          = const_;
			this.ArraySpecifier = arraySpecifier;
		}

		public void GenerateCode(CGenerator generator)
		{
			if (!String.IsNullOrWhiteSpace(this.Type))
			{
				generator.OutputStream.Write(this.ToString().Trim());
			}
		}

		public override string ToString()
		{
			if (!String.IsNullOrWhiteSpace(this.Type))
			{
				StringBuilder vt = new StringBuilder();

				if ((this.Const == ConstType.Value) || (this.Const == ConstType.Both))
				{
					vt.Append("const ");
				}

				vt.Append(this.Type);
				vt.Append(" ");

				if (!String.IsNullOrWhiteSpace(this.Indirection))
				{
					vt.Append(this.Indirection);
				}

				if ((this.Const == ConstType.Indirection) || (this.Const == ConstType.Both))
				{
					vt.Append("const ");
				}

				if (!String.IsNullOrWhiteSpace(this.Name))
				{
					vt.Append(this.Name);
				}

				if (this.ArraySpecifier != null)
				{
					vt.Append("[");
					vt.Append(this.ArraySpecifier);
					vt.Append("]");
				}

				return vt.ToString().Trim();
			}

			return base.ToString();
		}

		#region ICloneable Member

		public object Clone()
		{
			// we only have value types as members -> simply use .net base function
			return this.MemberwiseClone();
		}

		#endregion
	}
}
