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
using System.IO;

namespace CCodeGeneration
{
	public class CGenerator
	{
		public TextWriter OutputStream { get; private set; }
		public string File { get; private set; }
		public uint IndentCount { get; private set; }
		public string IndentChar { get; private set; }
		public string NewLine { get; private set; }

		public CGenerator(System.IO.TextWriter outputStream, string file, uint indentCount, string indentChar, string newLine)
		{
			this.OutputStream = outputStream;
			this.File     = file;
			this.IndentCount  = indentCount;
			this.IndentChar   = indentChar;
			this.NewLine      = newLine;
		}

		public string FileName
		{
			get
			{
				if (!String.IsNullOrWhiteSpace(this.File))
				{
					return Path.GetFileName(this.File);
				}

				return null;
			}
		}

		public void WriteSequence(string value, uint repetitions)
		{
			while (repetitions > 0)
			{
				this.OutputStream.Write(value);
				repetitions--;
			}
		}

		public void IndentLine(int level)
		{
			while (level > 0)
			{
				WriteSequence(this.IndentChar, this.IndentCount);
				level--;
			}
		}

		public void WriteNewLine()
		{
			this.OutputStream.Write(this.NewLine);
		}

		public void WriteMultilineString(string value, int level = 0)
		{
			if (String.IsNullOrEmpty(value))
			{
				return;
			}

			// only \n and \r\n are recognized as linebreaks
			string[] lines = value.Split(new char[] { '\n' }, StringSplitOptions.None);

			for (int l = 0; l < (lines.Length - 1); l++)
			{
				if (lines[l].EndsWith("\r"))
				{
					this.OutputStream.Write(lines[l].Substring(0, lines[l].Length-1));
				}
				else
				{
					this.OutputStream.Write(lines[l]);
				}

				this.WriteNewLine();
				this.IndentLine(level);
			}

			this.OutputStream.Write(lines[lines.Length - 1]);
		}
	}
}
