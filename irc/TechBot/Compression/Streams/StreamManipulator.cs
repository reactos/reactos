// StreamManipulator.cs
// Copyright (C) 2001 Mike Krueger
//
// This file was translated from java, it was part of the GNU Classpath
// Copyright (C) 2001 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Linking this library statically or dynamically with other modules is
// making a combined work based on this library.  Thus, the terms and
// conditions of the GNU General Public License cover the whole
// combination.
// 
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent
// modules, and to copy and distribute the resulting executable under
// terms of your choice, provided that you also meet, for each linked
// independent module, the terms and conditions of the license of that
// module.  An independent module is a module which is not derived from
// or based on this library.  If you modify this library, you may extend
// this exception to your version of the library, but you are not
// obligated to do so.  If you do not wish to do so, delete this
// exception statement from your version.

using System;

namespace ICSharpCode.SharpZipLib.Zip.Compression.Streams 
{
	
	/// <summary>
	/// This class allows us to retrieve a specified amount of bits from
	/// the input buffer, as well as copy big byte blocks.
	///
	/// It uses an int buffer to store up to 31 bits for direct
	/// manipulation.  This guarantees that we can get at least 16 bits,
	/// but we only need at most 15, so this is all safe.
	///
	/// There are some optimizations in this class, for example, you must
	/// never peek more then 8 bits more than needed, and you must first
	/// peek bits before you may drop them.  This is not a general purpose
	/// class but optimized for the behaviour of the Inflater.
	///
	/// authors of the original java version : John Leuner, Jochen Hoenicke
	/// </summary>
	public class StreamManipulator
	{
		private byte[] window;
		private int window_start = 0;
		private int window_end = 0;
		
		private uint buffer = 0;
		private int bits_in_buffer = 0;
		
		/// <summary>
		/// Get the next n bits but don't increase input pointer.  n must be
		/// less or equal 16 and if you if this call succeeds, you must drop
		/// at least n-8 bits in the next call.
		/// </summary>
		/// <returns>
		/// the value of the bits, or -1 if not enough bits available.  */
		/// </returns>
		public int PeekBits(int n)
		{
			if (bits_in_buffer < n) {
				if (window_start == window_end) {
					return -1; // ok
				}
				buffer |= (uint)((window[window_start++] & 0xff |
				                 (window[window_start++] & 0xff) << 8) << bits_in_buffer);
				bits_in_buffer += 16;
			}
			return (int)(buffer & ((1 << n) - 1));
		}
		
		/// <summary>
		/// Drops the next n bits from the input.  You should have called peekBits
		/// with a bigger or equal n before, to make sure that enough bits are in
		/// the bit buffer.
		/// </summary>
		public void DropBits(int n)
		{
			buffer >>= n;
			bits_in_buffer -= n;
		}
		
		/// <summary>
		/// Gets the next n bits and increases input pointer.  This is equivalent
		/// to peekBits followed by dropBits, except for correct error handling.
		/// </summary>
		/// <returns>
		/// the value of the bits, or -1 if not enough bits available.
		/// </returns>
		public int GetBits(int n)
		{
			int bits = PeekBits(n);
			if (bits >= 0) {
				DropBits(n);
			}
			return bits;
		}
		
		/// <summary>
		/// Gets the number of bits available in the bit buffer.  This must be
		/// only called when a previous peekBits() returned -1.
		/// </summary>
		/// <returns>
		/// the number of bits available.
		/// </returns>
		public int AvailableBits {
			get {
				return bits_in_buffer;
			}
		}
		
		/// <summary>
		/// Gets the number of bytes available.
		/// </summary>
		/// <returns>
		/// the number of bytes available.
		/// </returns>
		public int AvailableBytes {
			get {
				return window_end - window_start + (bits_in_buffer >> 3);
			}
		}
		
		/// <summary>
		/// Skips to the next byte boundary.
		/// </summary>
		public void SkipToByteBoundary()
		{
			buffer >>= (bits_in_buffer & 7);
			bits_in_buffer &= ~7;
		}
		
		public bool IsNeedingInput {
			get {
				return window_start == window_end;
			}
		}
		
		/// <summary>
		/// Copies length bytes from input buffer to output buffer starting
		/// at output[offset].  You have to make sure, that the buffer is
		/// byte aligned.  If not enough bytes are available, copies fewer
		/// bytes.
		/// </summary>
		/// <param name="output">
		/// the buffer.
		/// </param>
		/// <param name="offset">
		/// the offset in the buffer.
		/// </param>
		/// <param name="length">
		/// the length to copy, 0 is allowed.
		/// </param>
		/// <returns>
		/// the number of bytes copied, 0 if no byte is available.
		/// </returns>
		public int CopyBytes(byte[] output, int offset, int length)
		{
			if (length < 0) {
				throw new ArgumentOutOfRangeException("length negative");
			}
			if ((bits_in_buffer & 7) != 0) {
				/* bits_in_buffer may only be 0 or 8 */
				throw new InvalidOperationException("Bit buffer is not aligned!");
			}
			
			int count = 0;
			while (bits_in_buffer > 0 && length > 0) {
				output[offset++] = (byte) buffer;
				buffer >>= 8;
				bits_in_buffer -= 8;
				length--;
				count++;
			}
			if (length == 0) {
				return count;
			}
			
			int avail = window_end - window_start;
			if (length > avail) {
				length = avail;
			}
			System.Array.Copy(window, window_start, output, offset, length);
			window_start += length;
			
			if (((window_start - window_end) & 1) != 0) {
				/* We always want an even number of bytes in input, see peekBits */
				buffer = (uint)(window[window_start++] & 0xff);
				bits_in_buffer = 8;
			}
			return count + length;
		}
		
		public StreamManipulator()
		{
		}
		
		public void Reset()
		{
			buffer = (uint)(window_start = window_end = bits_in_buffer = 0);
		}
		
		public void SetInput(byte[] buf, int off, int len)
		{
			if (window_start < window_end) {
				throw new InvalidOperationException("Old input was not completely processed");
			}
			
			int end = off + len;
			
			/* We want to throw an ArrayIndexOutOfBoundsException early.  The
			* check is very tricky: it also handles integer wrap around.
			*/
			if (0 > off || off > end || end > buf.Length) {
				throw new ArgumentOutOfRangeException();
			}
			
			if ((len & 1) != 0) {
				/* We always want an even number of bytes in input, see peekBits */
				buffer |= (uint)((buf[off++] & 0xff) << bits_in_buffer);
				bits_in_buffer += 8;
			}
			
			window = buf;
			window_start = off;
			window_end = end;
		}
	}
}
