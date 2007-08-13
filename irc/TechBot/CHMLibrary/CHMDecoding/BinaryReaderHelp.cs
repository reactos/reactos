using System;
using System.Collections;
using System.IO;
using System.Text;
using System.Globalization;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>BinaryReaderHelp</c> implements static helper methods for extracting binary data 
	/// from a binary reader object.
	/// </summary>
	internal class BinaryReaderHelp
	{
		/// <summary>
		/// Internal helper method to extract null-terminated strings from a binary reader
		/// </summary>
		/// <param name="binReader">reference to the binary reader</param>
		/// <param name="offset">offset in the stream</param>
		/// <param name="noOffset">true if the offset value should be used</param>
		/// <param name="encoder">encoder used for text encoding</param>
		/// <returns>An extracted string value</returns>
		internal static string ExtractString(ref BinaryReader binReader, int offset, bool noOffset, Encoding encoder)
		{
			string strReturn = "";

			if(encoder == null)
				encoder = Encoding.ASCII;

			ArrayList nameBytes = new ArrayList();
			byte curByte;
			
			if(!noOffset)
				binReader.BaseStream.Seek(offset, SeekOrigin.Begin);

			if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
				return "";

			curByte = binReader.ReadByte();
			while( (curByte != (byte)0) && (binReader.BaseStream.Position < binReader.BaseStream.Length) )
			{	
				nameBytes.Add( curByte );
				curByte = binReader.ReadByte();
			}

			byte[] name = (byte[]) (nameBytes.ToArray(System.Type.GetType("System.Byte")));
			strReturn = encoder.GetString(name,0,name.Length);

			return strReturn;
		}

		/// <summary>
		/// Internal helper method to extract a string with a specific length from the binary reader
		/// </summary>
		/// <param name="binReader">reference to the binary reader</param>
		/// <param name="length">length of the string (number of bytes)</param>
		/// <param name="offset">offset in the stream</param>
		/// <param name="noOffset">true if the offset value should be used</param>
		/// <param name="encoder">encoder used for text encoding</param>
		/// <returns>An extracted string value</returns>
		internal static string ExtractString(ref BinaryReader binReader, int length, int offset, bool noOffset, Encoding encoder)
		{
			string strReturn = "";

			if(length == 0)
				return "";

			if(encoder == null)
				encoder = Encoding.ASCII;

			ArrayList nameBytes = new ArrayList();
			byte curByte;
			
			if(!noOffset)
				binReader.BaseStream.Seek(offset, SeekOrigin.Begin);

			if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
				return "";

			curByte = binReader.ReadByte();
			while( (curByte != (byte)0) && (nameBytes.Count < length) && (binReader.BaseStream.Position < binReader.BaseStream.Length) )
			{	
				nameBytes.Add( curByte );

				if(nameBytes.Count < length)
					curByte = binReader.ReadByte();
			}

			byte[] name = (byte[]) (nameBytes.ToArray(System.Type.GetType("System.Byte")));
			strReturn = encoder.GetString(name,0,name.Length);

			return strReturn;
		}

		/// <summary>
		/// Internal helper method to extract a string with a specific length from the binary reader
		/// </summary>
		/// <param name="binReader">reference to the binary reader</param>
		/// <param name="bFoundTerminator">reference to a bool vairable which will receive true if the
		/// string terminator \0 was found. false indicates that the end of the stream was reached.</param>
		/// <param name="offset">offset in the stream</param>
		/// <param name="noOffset">true if the offset value should be used</param>
		/// <param name="encoder">encoder used for text encoding</param>
		/// <returns>An extracted string value</returns>
		internal static string ExtractString(ref BinaryReader binReader, ref bool bFoundTerminator, int offset, bool noOffset, Encoding encoder)
		{
			string strReturn = "";

			ArrayList nameBytes = new ArrayList();
			byte curByte;
			
			if(encoder == null)
				encoder = Encoding.ASCII;

			if(!noOffset)
				binReader.BaseStream.Seek(offset, SeekOrigin.Begin);

			if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
				return "";

			curByte = binReader.ReadByte();
			while( (curByte != (byte)0) && (binReader.BaseStream.Position < binReader.BaseStream.Length) )
			{	
				nameBytes.Add( curByte );
				curByte = binReader.ReadByte();

				if( curByte == (byte)0 )
				{
					bFoundTerminator = true;
				}
			}

			byte[] name = (byte[]) (nameBytes.ToArray(System.Type.GetType("System.Byte")));
			strReturn = encoder.GetString(name,0,name.Length);

			return strReturn;
		}

		/// <summary>
		/// Internal helper method to extract a null-terminated UTF-16/UCS-2 strings from a binary reader
		/// </summary>
		/// <param name="binReader">reference to the binary reader</param>
		/// <param name="offset">offset in the stream</param>
		/// <param name="noOffset">true if the offset value should be used</param>
		/// <param name="encoder">encoder used for text encoding</param>
		/// <returns>An extracted string value</returns>
		internal static string ExtractUTF16String(ref BinaryReader binReader, int offset, bool noOffset, Encoding encoder)
		{
			string strReturn = "";

			ArrayList nameBytes = new ArrayList();
			byte curByte;
			int lastByte=-1;
			
			if(!noOffset)
				binReader.BaseStream.Seek(offset, SeekOrigin.Begin);

			if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
				return "";

			if(encoder == null)
				encoder = Encoding.Unicode;

			curByte = binReader.ReadByte();
			int nCnt = 0;
			while( ((curByte != (byte)0) || (lastByte != 0) ) && (binReader.BaseStream.Position < binReader.BaseStream.Length) )
			{	
				nameBytes.Add( curByte );

				if(nCnt%2 == 0)
					lastByte = (int)curByte;

				curByte = binReader.ReadByte();

				nCnt++;
			}

			byte[] name = (byte[]) (nameBytes.ToArray(System.Type.GetType("System.Byte")));
			strReturn = Encoding.Unicode.GetString(name,0,name.Length);

			// apply text encoding
			name = Encoding.Default.GetBytes(strReturn);
			strReturn = encoder.GetString(name,0,name.Length);

			return strReturn;
		}

		/// <summary>
		/// Internal helper for reading ENCINT encoded integer values
		/// </summary>
		/// <param name="binReader">reference to the reader</param>
		/// <returns>a long value</returns>
		internal static long ReadENCINT(ref BinaryReader binReader)
		{
			long nRet = 0;
			byte buffer = 0;
			int shift = 0;

			if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
				return nRet;
			
			do
			{
				buffer = binReader.ReadByte();
				nRet |= ((long)((buffer & (byte)0x7F))) << shift;
				shift += 7;

			}while ( (buffer & (byte)0x80) != 0);

			return nRet;
		}

		/// <summary>
		/// Reads an s/r encoded value from the byte array and decodes it into an integer
		/// </summary>
		/// <param name="wclBits">a byte array containing all bits (contains only 0 or 1 elements)</param>
		/// <param name="s">scale param for encoding</param>
		/// <param name="r">root param for encoding</param>
		/// <param name="nBitIndex">current index in the wclBits array</param>
		/// <returns>Returns an decoded integer value.</returns>
		internal static int ReadSRItem(byte[] wclBits, int s, int r, ref int nBitIndex)
		{
			int nRet = 0;
			int q = r;

			int nPref1Cnt = 0;

			while( wclBits[nBitIndex++] == 1)
			{
				nPref1Cnt++;
			}

			if(nPref1Cnt == 0)
			{
				int nMask = 0;

				for(int nbits=0; nbits<q;nbits++)
				{
					nMask |= ( 0x01 & (int)wclBits[nBitIndex]) << (q-nbits-1);
					nBitIndex++;
				}

				nRet = nMask;
			} 
			else 
			{
				q += (nPref1Cnt-1);

				int nMask = 0;
				int nRMaxValue = 0;

				for(int nbits=0; nbits<q;nbits++)
				{
					nMask |= ( 0x01 & (int)wclBits[nBitIndex]) << (q-nbits-1);
					nBitIndex++;
				}

				for(int nsv=0; nsv<r; nsv++)
				{
					nRMaxValue = nRMaxValue << 1;
					nRMaxValue |= 0x1;
				}
	
				nRMaxValue++; // startvalue of s/r encoding with 1 prefixing '1'

				nRMaxValue *= (int) Math.Pow((double)2, (double)(nPref1Cnt-1));

				nRet = nRMaxValue + nMask;
			}

			return nRet;
		}
	}
}
