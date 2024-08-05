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

using System.Collections.Generic;
using System;

namespace CCodeGeneration
{
	public class CodeContainerBase: CodeElement
	{
		private readonly List<CodeElement> declarations = new List<CodeElement>();
		private readonly List<CodeElement> innerElements = new List<CodeElement>();
		private bool increaseLevel = true;

		public List<CodeElement> Declarations
		{
			get { return this.declarations; }
		}

		public List<CodeElement> InnerElements
		{
			get { return this.innerElements; }
		}

		protected bool IncreaseLevel
		{
			get { return this.increaseLevel; }
			set { this.increaseLevel = value; }
		}

		public void AddElements(IList<CodeElement> elements, params CodeElement[] spacerElements)
		{
			if (elements != null)
			{
				if ((spacerElements == null) || (spacerElements.Length == 0))
				{
					this.innerElements.AddRange(elements);
				}
				else
				{
					bool spacerAdded = false;

					foreach (CodeElement element in elements)
					{
						this.innerElements.Add(element);
						this.innerElements.AddRange(spacerElements);
						spacerAdded = true;
					}

					if (spacerAdded)
					{
						// remove last spacer again
						this.innerElements.RemoveRange(this.innerElements.Count - spacerElements.Length, spacerElements.Length);
					}					
				}
			}
		}
 
		public CodeElement AddElement(CodeElement element)
		{
			if (element != null)
			{
				this.innerElements.Add(element);
			}

			return element;
		}

		public Code AddCode(string code)
		{
			return this.AddElement(new Code(code)) as Code;
		}

		public Code AddCodeFormat(string codeFormat, params object[] args)
		{
			return this.AddElement(new Code(String.Format(codeFormat, args))) as Code;
		}

		public CodeElement AddDeclaration(CodeElement declaration)
		{
			if (declaration != null)
			{
				this.declarations.Add(declaration);
			}

			return declaration;
		}

		public override void GenerateCode(int level, CGenerator generator)
		{
			if (this.increaseLevel)
				level++;

			if (this.declarations.Count > 0)
			{
				foreach (CodeElement element in this.declarations)
				{
					element.GenerateCode(level, generator);
				}

				EmptyLine.SingleLine.GenerateCode(level, generator);
			}

			foreach (CodeElement element in this.innerElements)
			{
				element.GenerateCode(level, generator);
			}
		}
	}
}
