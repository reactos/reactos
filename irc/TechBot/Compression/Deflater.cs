// Deflater.cs
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

namespace ICSharpCode.SharpZipLib.Zip.Compression 
{
	
	/// <summary>
	/// This is the Deflater class.  The deflater class compresses input
	/// with the deflate algorithm described in RFC 1951.  It has several
	/// compression levels and three different strategies described below.
	///
	/// This class is <i>not</i> thread safe.  This is inherent in the API, due
	/// to the split of deflate and setInput.
	/// 
	/// author of the original java version : Jochen Hoenicke
	/// </summary>
	public class Deflater
	{
		/// <summary>
		/// The best and slowest compression level.  This tries to find very
		/// long and distant string repetitions.
		/// </summary>
		public static  int BEST_COMPRESSION = 9;
		
		/// <summary>
		/// The worst but fastest compression level.
		/// </summary>
		public static  int BEST_SPEED = 1;
		
		/// <summary>
		/// The default compression level.
		/// </summary>
		public static  int DEFAULT_COMPRESSION = -1;
		
		/// <summary>
		/// This level won't compress at all but output uncompressed blocks.
		/// </summary>
		public static  int NO_COMPRESSION = 0;
				
		/// <summary>
		/// The compression method.  This is the only method supported so far.
		/// There is no need to use this constant at all.
		/// </summary>
		public static  int DEFLATED = 8;
		
		/*
		* The Deflater can do the following state transitions:
			*
			* (1) -> INIT_STATE   ----> INIT_FINISHING_STATE ---.
			*        /  | (2)      (5)                         |
			*       /   v          (5)                         |
			*   (3)| SETDICT_STATE ---> SETDICT_FINISHING_STATE |(3)
			*       \   | (3)                 |        ,-------'
			*        |  |                     | (3)   /
			*        v  v          (5)        v      v
			* (1) -> BUSY_STATE   ----> FINISHING_STATE
			*                                | (6)
			*                                v
			*                           FINISHED_STATE
			*    \_____________________________________/
			*          | (7)
			*          v
			*        CLOSED_STATE
			*
			* (1) If we should produce a header we start in INIT_STATE, otherwise
			*     we start in BUSY_STATE.
			* (2) A dictionary may be set only when we are in INIT_STATE, then
			*     we change the state as indicated.
			* (3) Whether a dictionary is set or not, on the first call of deflate
			*     we change to BUSY_STATE.
			* (4) -- intentionally left blank -- :)
			* (5) FINISHING_STATE is entered, when flush() is called to indicate that
			*     there is no more INPUT.  There are also states indicating, that
			*     the header wasn't written yet.
			* (6) FINISHED_STATE is entered, when everything has been flushed to the
			*     internal pending output buffer.
			* (7) At any time (7)
			*
			*/
			
		private static  int IS_SETDICT              = 0x01;
		private static  int IS_FLUSHING             = 0x04;
		private static  int IS_FINISHING            = 0x08;
		
		private static  int INIT_STATE              = 0x00;
		private static  int SETDICT_STATE           = 0x01;
		//		private static  int INIT_FINISHING_STATE    = 0x08;
		//		private static  int SETDICT_FINISHING_STATE = 0x09;
		private static  int BUSY_STATE              = 0x10;
		private static  int FLUSHING_STATE          = 0x14;
		private static  int FINISHING_STATE         = 0x1c;
		private static  int FINISHED_STATE          = 0x1e;
		private static  int CLOSED_STATE            = 0x7f;
		
		/// <summary>
		/// Compression level.
		/// </summary>
		private int level;
		
		/// <summary>
		/// should we include a header.
		/// </summary>
		private bool noHeader;
		
		//		/// <summary>
		//		/// Compression strategy.
		//		/// </summary>
		//		private int strategy;
		
		/// <summary>
		/// The current state.
		/// </summary>
		private int state;
		
		/// <summary>
		/// The total bytes of output written.
		/// </summary>
		private int totalOut;
		
		/// <summary>
		/// The pending output.
		/// </summary>
		private DeflaterPending pending;
		
		/// <summary>
		/// The deflater engine.
		/// </summary>
		private DeflaterEngine engine;
		
		/// <summary>
		/// Creates a new deflater with default compression level.
		/// </summary>
		public Deflater() : this(DEFAULT_COMPRESSION, false)
		{
			
		}
		
		/// <summary>
		/// Creates a new deflater with given compression level.
		/// </summary>
		/// <param name="lvl">
		/// the compression level, a value between NO_COMPRESSION
		/// and BEST_COMPRESSION, or DEFAULT_COMPRESSION.
		/// </param>
		/// <exception cref="System.ArgumentOutOfRangeException">if lvl is out of range.</exception>
		public Deflater(int lvl) : this(lvl, false)
		{
			
		}
		
		/// <summary>
		/// Creates a new deflater with given compression level.
		/// </summary>
		/// <param name="lvl">
		/// the compression level, a value between NO_COMPRESSION
		/// and BEST_COMPRESSION.
		/// </param>
		/// <param name="nowrap">
		/// true, if we should suppress the deflate header at the
		/// beginning and the adler checksum at the end of the output.  This is
		/// useful for the GZIP format.
		/// </param>
		/// <exception cref="System.ArgumentOutOfRangeException">if lvl is out of range.</exception>
		public Deflater(int lvl, bool nowrap)
		{
			if (lvl == DEFAULT_COMPRESSION) {
				lvl = 6;
			} else if (lvl < NO_COMPRESSION || lvl > BEST_COMPRESSION) {
				throw new ArgumentOutOfRangeException("lvl");
			}
			
			pending = new DeflaterPending();
			engine = new DeflaterEngine(pending);
			this.noHeader = nowrap;
			SetStrategy(DeflateStrategy.Default);
			SetLevel(lvl);
			Reset();
		}
		
		
		/// <summary>
		/// Resets the deflater.  The deflater acts afterwards as if it was
		/// just created with the same compression level and strategy as it
		/// had before.
		/// </summary>
		public void Reset()
		{
			state = (noHeader ? BUSY_STATE : INIT_STATE);
			totalOut = 0;
			pending.Reset();
			engine.Reset();
		}
		
		/// <summary>
		/// Gets the current adler checksum of the data that was processed so far.
		/// </summary>
		public int Adler {
			get {
				return engine.Adler;
			}
		}
		
		/// <summary>
		/// Gets the number of input bytes processed so far.
		/// </summary>
		public int TotalIn {
			get {
				return engine.TotalIn;
			}
		}
		
		/// <summary>
		/// Gets the number of output bytes so far.
		/// </summary>
		public int TotalOut {
			get {
				return totalOut;
			}
		}
		
		/// <summary>
		/// Flushes the current input block.  Further calls to deflate() will
		/// produce enough output to inflate everything in the current input
		/// block.  This is not part of Sun's JDK so I have made it package
		/// private.  It is used by DeflaterOutputStream to implement
		/// flush().
		/// </summary>
		public void Flush() 
		{
			state |= IS_FLUSHING;
		}
		
		/// <summary>
		/// Finishes the deflater with the current input block.  It is an error
		/// to give more input after this method was called.  This method must
		/// be called to force all bytes to be flushed.
		/// </summary>
		public void Finish() 
		{
			state |= IS_FLUSHING | IS_FINISHING;
		}
		
		/// <summary>
		/// Returns true if the stream was finished and no more output bytes
		/// are available.
		/// </summary>
		public bool IsFinished {
			get {
				return state == FINISHED_STATE && pending.IsFlushed;
			}
		}
		
		/// <summary>
		/// Returns true, if the input buffer is empty.
		/// You should then call setInput(). 
		/// NOTE: This method can also return true when the stream
		/// was finished.
		/// </summary>
		public bool IsNeedingInput {
			get {
				return engine.NeedsInput();
			}
		}
		
		/// <summary>
		/// Sets the data which should be compressed next.  This should be only
		/// called when needsInput indicates that more input is needed.
		/// If you call setInput when needsInput() returns false, the
		/// previous input that is still pending will be thrown away.
		/// The given byte array should not be changed, before needsInput() returns
		/// true again.
		/// This call is equivalent to <code>setInput(input, 0, input.length)</code>.
		/// </summary>
		/// <param name="input">
		/// the buffer containing the input data.
		/// </param>
		/// <exception cref="System.InvalidOperationException">
		/// if the buffer was finished() or ended().
		/// </exception>
		public void SetInput(byte[] input)
		{
			SetInput(input, 0, input.Length);
		}
		
		/// <summary>
		/// Sets the data which should be compressed next.  This should be
		/// only called when needsInput indicates that more input is needed.
		/// The given byte array should not be changed, before needsInput() returns
		/// true again.
		/// </summary>
		/// <param name="input">
		/// the buffer containing the input data.
		/// </param>
		/// <param name="off">
		/// the start of the data.
		/// </param>
		/// <param name="len">
		/// the length of the data.
		/// </param>
		/// <exception cref="System.InvalidOperationException">
		/// if the buffer was finished() or ended() or if previous input is still pending.
		/// </exception>
		public void SetInput(byte[] input, int off, int len)
		{
			if ((state & IS_FINISHING) != 0) {
				throw new InvalidOperationException("finish()/end() already called");
			}
			engine.SetInput(input, off, len);
		}
		
		/// <summary>
		/// Sets the compression level.  There is no guarantee of the exact
		/// position of the change, but if you call this when needsInput is
		/// true the change of compression level will occur somewhere near
		/// before the end of the so far given input.
		/// </summary>
		/// <param name="lvl">
		/// the new compression level.
		/// </param>
		public void SetLevel(int lvl)
		{
			if (lvl == DEFAULT_COMPRESSION) {
				lvl = 6;
			} else if (lvl < NO_COMPRESSION || lvl > BEST_COMPRESSION) {
				throw new ArgumentOutOfRangeException("lvl");
			}
			
			if (level != lvl) {
				level = lvl;
				engine.SetLevel(lvl);
			}
		}
		
		/// <summary>
		/// Sets the compression strategy. Strategy is one of
		/// DEFAULT_STRATEGY, HUFFMAN_ONLY and FILTERED.  For the exact
		/// position where the strategy is changed, the same as for
		/// setLevel() applies.
		/// </summary>
		/// <param name="stgy">
		/// the new compression strategy.
		/// </param>
		public void SetStrategy(DeflateStrategy stgy)
		{
			engine.Strategy = stgy;
		}
		
		/// <summary>
		/// Deflates the current input block to the given array.  It returns
		/// the number of bytes compressed, or 0 if either
		/// needsInput() or finished() returns true or length is zero.
		/// </summary>
		/// <param name="output">
		/// the buffer where to write the compressed data.
		/// </param>
		public int Deflate(byte[] output)
		{
			return Deflate(output, 0, output.Length);
		}
		
		/// <summary>
		/// Deflates the current input block to the given array.  It returns
		/// the number of bytes compressed, or 0 if either
		/// needsInput() or finished() returns true or length is zero.
		/// </summary>
		/// <param name="output">
		/// the buffer where to write the compressed data.
		/// </param>
		/// <param name="offset">
		/// the offset into the output array.
		/// </param>
		/// <param name="length">
		/// the maximum number of bytes that may be written.
		/// </param>
		/// <exception cref="System.InvalidOperationException">
		/// if end() was called.
		/// </exception>
		/// <exception cref="System.ArgumentOutOfRangeException">
		/// if offset and/or length don't match the array length.
		/// </exception>
		public int Deflate(byte[] output, int offset, int length)
		{
			int origLength = length;
			
			if (state == CLOSED_STATE) {
				throw new InvalidOperationException("Deflater closed");
			}
			
			if (state < BUSY_STATE) {
				/* output header */
				int header = (DEFLATED +
					((DeflaterConstants.MAX_WBITS - 8) << 4)) << 8;
				int level_flags = (level - 1) >> 1;
				if (level_flags < 0 || level_flags > 3) {
					level_flags = 3;
				}
				header |= level_flags << 6;
				if ((state & IS_SETDICT) != 0) {
					/* Dictionary was set */
					header |= DeflaterConstants.PRESET_DICT;
				}
				header += 31 - (header % 31);
				
				
				pending.WriteShortMSB(header);
				if ((state & IS_SETDICT) != 0) {
					int chksum = engine.Adler;
					engine.ResetAdler();
					pending.WriteShortMSB(chksum >> 16);
					pending.WriteShortMSB(chksum & 0xffff);
				}
				
				state = BUSY_STATE | (state & (IS_FLUSHING | IS_FINISHING));
			}
			
			for (;;) {
				int count = pending.Flush(output, offset, length);
				offset   += count;
				totalOut += count;
				length   -= count;
				
				if (length == 0 || state == FINISHED_STATE) {
					break;
				}
				
				if (!engine.Deflate((state & IS_FLUSHING) != 0, (state & IS_FINISHING) != 0)) {
					if (state == BUSY_STATE) {
						/* We need more input now */
						return origLength - length;
					} else if (state == FLUSHING_STATE) {
						if (level != NO_COMPRESSION) {
							/* We have to supply some lookahead.  8 bit lookahead
							 * are needed by the zlib inflater, and we must fill
							 * the next byte, so that all bits are flushed.
							 */
							int neededbits = 8 + ((-pending.BitCount) & 7);
							while (neededbits > 0) {
								/* write a static tree block consisting solely of
								 * an EOF:
								 */
								pending.WriteBits(2, 10);
								neededbits -= 10;
							}
						}
						state = BUSY_STATE;
					} else if (state == FINISHING_STATE) {
						pending.AlignToByte();
						/* We have completed the stream */
						if (!noHeader) {
							int adler = engine.Adler;
							pending.WriteShortMSB(adler >> 16);
							pending.WriteShortMSB(adler & 0xffff);
						}
						state = FINISHED_STATE;
					}
				}
			}
			return origLength - length;
		}
		
		/// <summary>
		/// Sets the dictionary which should be used in the deflate process.
		/// This call is equivalent to <code>setDictionary(dict, 0, dict.Length)</code>.
		/// </summary>
		/// <param name="dict">
		/// the dictionary.
		/// </param>
		/// <exception cref="System.InvalidOperationException">
		/// if setInput () or deflate () were already called or another dictionary was already set.
		/// </exception>
		public void SetDictionary(byte[] dict)
		{
			SetDictionary(dict, 0, dict.Length);
		}
		
		/// <summary>
		/// Sets the dictionary which should be used in the deflate process.
		/// The dictionary should be a byte array containing strings that are
		/// likely to occur in the data which should be compressed.  The
		/// dictionary is not stored in the compressed output, only a
		/// checksum.  To decompress the output you need to supply the same
		/// dictionary again.
		/// </summary>
		/// <param name="dict">
		/// the dictionary.
		/// </param>
		/// <param name="offset">
		/// an offset into the dictionary.
		/// </param>
		/// <param name="length">
		/// the length of the dictionary.
		/// </param>
		/// <exception cref="System.InvalidOperationException">
		/// if setInput () or deflate () were already called or another dictionary was already set.
		/// </exception>
		public void SetDictionary(byte[] dict, int offset, int length)
		{
			if (state != INIT_STATE) {
				throw new InvalidOperationException();
			}
			
			state = SETDICT_STATE;
			engine.SetDictionary(dict, offset, length);
		}
	}
}
