// InflaterInputStream.cs
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
using System.IO;

using ICSharpCode.SharpZipLib.Zip.Compression;
using ICSharpCode.SharpZipLib.Checksums;

namespace ICSharpCode.SharpZipLib.Zip.Compression.Streams 
{
	
	/// <summary>
	/// This filter stream is used to decompress data compressed baseInputStream the "deflate"
	/// format. The "deflate" format is described baseInputStream RFC 1951.
	///
	/// This stream may form the basis for other decompression filters, such
	/// as the <code>GzipInputStream</code>.
	///
	/// author of the original java version : John Leuner
	/// </summary>
	public class InflaterInputStream : Stream
	{
		//Variables
		
		/// <summary>
		/// Decompressor for this filter
		/// </summary>
		protected Inflater inf;
		
		/// <summary>
		/// Byte array used as a buffer
		/// </summary>
		protected byte[] buf;
		
		/// <summary>
		/// Size of buffer
		/// </summary>
		protected int len;
		
		//We just use this if we are decoding one byte at a time with the read() call
		private byte[] onebytebuffer = new byte[1];
		
		/// <summary>
		/// base stream the inflater depends on.
		/// </summary>
		protected Stream baseInputStream;
		
		protected long csize;
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override bool CanRead {
			get {
				return baseInputStream.CanRead;
			}
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override bool CanSeek {
			get {
				return false;
				//				return baseInputStream.CanSeek;
			}
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override bool CanWrite {
			get {
				return baseInputStream.CanWrite;
			}
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override long Length {
			get {
				return len;
			}
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override long Position {
			get {
				return baseInputStream.Position;
			}
			set {
				baseInputStream.Position = value;
			}
		}
		
		/// <summary>
		/// Flushes the baseInputStream
		/// </summary>
		public override void Flush()
		{
			baseInputStream.Flush();
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override long Seek(long offset, SeekOrigin origin)
		{
			throw new NotSupportedException("Seek not supported"); // -jr- 01-Dec-2003
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override void SetLength(long val)
		{
			baseInputStream.SetLength(val);
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override void Write(byte[] array, int offset, int count)
		{
			baseInputStream.Write(array, offset, count);
		}
		
		/// <summary>
		/// I needed to implement the abstract member.
		/// </summary>
		public override void WriteByte(byte val)
		{
			baseInputStream.WriteByte(val);
		}
		
		// -jr- 01-Dec-2003 This may be flawed for some base streams?  Depends on implementation of BeginWrite
		public override IAsyncResult BeginWrite(byte[] buffer, int offset, int count, AsyncCallback callback, object state)
		{
			throw new NotSupportedException("Asynch write not currently supported");
		}
		
		//Constructors
		
		/// <summary>
		/// Create an InflaterInputStream with the default decompresseor
		/// and a default buffer size.
		/// </summary>
		/// <param name = "baseInputStream">
		/// the InputStream to read bytes from
		/// </param>
		public InflaterInputStream(Stream baseInputStream) : this(baseInputStream, new Inflater(), 4096)
		{
			
		}
		
		/// <summary>
		/// Create an InflaterInputStream with the specified decompresseor
		/// and a default buffer size.
		/// </summary>
		/// <param name = "baseInputStream">
		/// the InputStream to read bytes from
		/// </param>
		/// <param name = "inf">
		/// the decompressor used to decompress data read from baseInputStream
		/// </param>
		public InflaterInputStream(Stream baseInputStream, Inflater inf) : this(baseInputStream, inf, 4096)
		{
		}
		
		/// <summary>
		/// Create an InflaterInputStream with the specified decompresseor
		/// and a specified buffer size.
		/// </summary>
		/// <param name = "baseInputStream">
		/// the InputStream to read bytes from
		/// </param>
		/// <param name = "inf">
		/// the decompressor used to decompress data read from baseInputStream
		/// </param>
		/// <param name = "size">
		/// size of the buffer to use
		/// </param>
		public InflaterInputStream(Stream baseInputStream, Inflater inf, int size)
		{
			this.baseInputStream = baseInputStream;
			this.inf = inf;
			try {
				this.len = (int)baseInputStream.Length;
			} catch (Exception) {
				// the stream may not support .Length
				this.len = 0;
			}
			
			if (size <= 0) {
				throw new ArgumentOutOfRangeException("size <= 0");
			}
			
			buf = new byte[size]; //Create the buffer
		}
		
		//Methods
		
		/// <summary>
		/// Returns 0 once the end of the stream (EOF) has been reached.
		/// Otherwise returns 1.
		/// </summary>
		public virtual int Available {
			get {
				return inf.IsFinished ? 0 : 1;
			}
		}
		
		/// <summary>
		/// Closes the input stream
		/// </summary>
		public override void Close()
		{
			baseInputStream.Close();
		}
		
		/// <summary>
		/// Fills the buffer with more data to decompress.
		/// </summary>
		protected void Fill()
		{
			len = baseInputStream.Read(buf, 0, buf.Length);
			// decrypting crypted data
			if (cryptbuffer != null) {
				DecryptBlock(buf, 0, System.Math.Min((int)(csize - inf.TotalIn), buf.Length));
			}
			
			if (len <= 0) {
				throw new ApplicationException("Deflated stream ends early.");
			}
			inf.SetInput(buf, 0, len);
		}
		
		/// <summary>
		/// Reads one byte of decompressed data.
		///
		/// The byte is baseInputStream the lower 8 bits of the int.
		/// </summary>
		public override int ReadByte()
		{
			int nread = Read(onebytebuffer, 0, 1); //read one byte
			if (nread > 0) {
				return onebytebuffer[0] & 0xff;
			}
			return -1; // ok
		}
		
		/// <summary>
		/// Decompresses data into the byte array
		/// </summary>
		/// <param name ="b">
		/// the array to read and decompress data into
		/// </param>
		/// <param name ="off">
		/// the offset indicating where the data should be placed
		/// </param>
		/// <param name ="len">
		/// the number of bytes to decompress
		/// </param>
		public override int Read(byte[] b, int off, int len)
		{
			for (;;) {
				int count;
				try {
					count = inf.Inflate(b, off, len);
				} catch (Exception e) {
					throw new ZipException(e.ToString());
				}
				
				if (count > 0) {
					return count;
				}
				
				if (inf.IsNeedingDictionary) {
					throw new ZipException("Need a dictionary");
				} else if (inf.IsFinished) {
					return 0;
				} else if (inf.IsNeedingInput) {
					Fill();
				} else {
					throw new InvalidOperationException("Don't know what to do");
				}
			}
		}
		
		/// <summary>
		/// Skip specified number of bytes of uncompressed data
		/// </summary>
		/// <param name ="n">
		/// number of bytes to skip
		/// </param>
		public long Skip(long n)
		{
			if (n < 0) {
				throw new ArgumentOutOfRangeException("n");
			}
			int len = 2048;
			if (n < len) {
				len = (int) n;
			}
			byte[] tmp = new byte[len];
			return (long)baseInputStream.Read(tmp, 0, tmp.Length);
		}
		
		#region Encryption stuff
		protected byte[] cryptbuffer = null;
		
		uint[] keys = null;
		protected byte DecryptByte()
		{
			uint temp = ((keys[2] & 0xFFFF) | 2);
			return (byte)((temp * (temp ^ 1)) >> 8);
		}
		
		protected void DecryptBlock(byte[] buf, int off, int len)
		{
			for (int i = off; i < off + len; ++i) {
				buf[i] ^= DecryptByte();
				UpdateKeys(buf[i]);
			}
		}
		
		protected void InitializePassword(string password)
		{
			keys = new uint[] {
				0x12345678,
				0x23456789,
				0x34567890
			};
			for (int i = 0; i < password.Length; ++i) {
				UpdateKeys((byte)password[i]);
			}
		}
		
		protected void UpdateKeys(byte ch)
		{
			keys[0] = Crc32.ComputeCrc32(keys[0], ch);
			keys[1] = keys[1] + (byte)keys[0];
			keys[1] = keys[1] * 134775813 + 1;
			keys[2] = Crc32.ComputeCrc32(keys[2], (byte)(keys[1] >> 24));
		}
		#endregion
	}
}
