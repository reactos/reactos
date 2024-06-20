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

namespace LwipSnmpCodeGeneration
{
	public interface IRestriction
	{
		string GetCheckCodeValid(string varNameToCheck);
		string GetCheckCodeInvalid(string varNameToCheck);
	}

	public class BitMaskRestriction : IRestriction
	{
		UInt32 mask;

		public BitMaskRestriction(UInt32 mask)
		{
			this.mask = mask;
		}

		public string GetCheckCodeValid(string varNameToCheck)
		{
			return String.Format("(({0} & {1}) == {0})", varNameToCheck, this.mask);
		}

		public string GetCheckCodeInvalid(string varNameToCheck)
		{
			return String.Format("(({0} & {1}) != {0})", varNameToCheck, this.mask);
		}
	}

	public class IsEqualRestriction : IRestriction
	{
		private Int64 value;

		public IsEqualRestriction(Int64 value)
		{
			this.value = value;
		}

		public long Value
		{
			get { return value; }
		}

		public string GetCheckCodeValid(string varNameToCheck)
		{
			return String.Format("({0} == {1})", varNameToCheck, this.value);
		}

		public string GetCheckCodeInvalid(string varNameToCheck)
		{
			return String.Format("({0} != {1})", varNameToCheck, this.value);
		}
	}

	public class IsInRangeRestriction : IRestriction
	{
		private Int64 rangeStart;
		private Int64 rangeEnd;

		public IsInRangeRestriction(Int64 rangeStart, Int64 rangeEnd)
		{
			this.rangeStart = rangeStart;
			this.rangeEnd   = rangeEnd;
		}

		public long RangeStart
		{
			get { return this.rangeStart; }
		}

		public long RangeEnd
		{
			get { return this.rangeEnd; }
		}

		public string GetCheckCodeValid(string varNameToCheck)
		{
			return String.Format("(({0} >= {1}) && ({0} <= {2}))", varNameToCheck, this.rangeStart, this.rangeEnd);
		}

		public string GetCheckCodeInvalid(string varNameToCheck)
		{
			return String.Format("(({0} < {1}) || ({0} > {2}))", varNameToCheck, this.rangeStart, this.rangeEnd);
		}
	}

}
