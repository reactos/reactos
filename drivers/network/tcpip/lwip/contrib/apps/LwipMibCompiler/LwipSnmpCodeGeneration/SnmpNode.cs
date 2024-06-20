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
using System.Text.RegularExpressions;
using CCodeGeneration;

namespace LwipSnmpCodeGeneration
{
	public abstract class SnmpNode
	{
		public static readonly Regex NameValidationRegex = new Regex(@"^\w+$");

		private string name;
		private readonly SnmpTreeNode parentNode;

		protected SnmpNode(SnmpTreeNode parentNode)
		{
			this.parentNode = parentNode;
		}

		public SnmpTreeNode ParentNode
		{
			get { return this.parentNode; }
		}

		public virtual uint Oid { get; set; }

		public abstract string FullNodeName
		{
			get;
		}

		public virtual string Name
		{
			get { return this.name; }
			set
			{
				if (value != this.name)
				{
					// check for valid name
					if (!NameValidationRegex.IsMatch(value))
					{
						throw new ArgumentOutOfRangeException("Name");
					}

					this.name = value;
				}
			}
		}

		public virtual void Generate(MibCFile generatedFile, MibHeaderFile generatedHeaderFile)
		{
			int declCount = generatedFile.Declarations.Count;
			int implCount = generatedFile.Implementation.Count;

			this.GenerateHeaderCode(generatedHeaderFile);
			this.GenerateCode(generatedFile);

			if (generatedFile.Declarations.Count != declCount)
			{
				generatedFile.Declarations.Add(EmptyLine.SingleLine);
			}
			if (generatedFile.Implementation.Count != implCount)
			{
				generatedFile.Implementation.Add(EmptyLine.SingleLine);
			}
		}

		public abstract void GenerateCode(MibCFile mibFile);

		public virtual void GenerateHeaderCode(MibHeaderFile mibHeaderFile)
		{
		}

		/// <summary>
		/// Called after node structure creation is completed and before code is created.
		/// Offers the possibility to perform operations depending on properties/subnodes.
		/// If the node shall be transformed to another node(-type) than the own instance
		/// may be replaced on parent node by the transformed instance.
		/// Calling sequence is always from leafs up to root. So a tree node can assume
		/// that the analyze method was already called on all child nodes.
		/// E.g. a tree node only has scalar sub nodes -> it transforms itself to a scalar array node
		/// </summary>
		/// <returns>The transformed node or null if nothing shall be changed in parent structure.</returns>
		public virtual void Analyze()
		{
		}
	}
}
