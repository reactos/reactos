using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMUrlstr</c> implements a string collection storing the URL strings of the help file
	/// </summary>
	internal sealed class CHMUrlstr : IDisposable
	{
		/// <summary>
		/// Constant specifying the size of the string blocks
		/// </summary>
		private const int BLOCK_SIZE = 0x1000;
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal member storing the url dictionary
		/// </summary>
		private Hashtable _urlDictionary = new Hashtable();
		/// <summary>
		/// Internal member storing the framename dictionary
		/// </summary>
		private Hashtable _framenameDictionary = new Hashtable();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #URLSTR file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMUrlstr(byte[] binaryFileData, CHMFile associatedFile)
		{
			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;
			DecodeData();

			// clear internal binary data after extraction
			_binaryFileData = null;
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		internal CHMUrlstr()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _urlDictionary.Count );

			if (_urlDictionary.Count != 0)
			{
				IDictionaryEnumerator iDictionaryEnumerator = _urlDictionary.GetEnumerator();
				while (iDictionaryEnumerator.MoveNext())
				{
					DictionaryEntry dictionaryEntry = (DictionaryEntry)iDictionaryEnumerator.Current;
					writer.Write( Int32.Parse(dictionaryEntry.Key.ToString()) );
					writer.Write( dictionaryEntry.Value.ToString() );
				}
			}

			writer.Write( _framenameDictionary.Count );

			if (_framenameDictionary.Count != 0)
			{
				IDictionaryEnumerator iDictionaryEnumerator = _framenameDictionary.GetEnumerator();
				while (iDictionaryEnumerator.MoveNext())
				{
					DictionaryEntry dictionaryEntry = (DictionaryEntry)iDictionaryEnumerator.Current;
					writer.Write( Int32.Parse(dictionaryEntry.Key.ToString()) );
					writer.Write( dictionaryEntry.Value.ToString() );
				}
			}
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			int i=0;
			int nCnt = reader.ReadInt32();

			for(i=0; i<nCnt;i++)
			{
				int nKey = reader.ReadInt32();
				string sValue = reader.ReadString();
				
				_urlDictionary[nKey.ToString()] = sValue;
			}

			nCnt = reader.ReadInt32();

			for(i=0; i<nCnt;i++)
			{
				int nKey = reader.ReadInt32();
				string sValue = reader.ReadString();
				
				_framenameDictionary[nKey.ToString()] = sValue;
			}
		}

		/// <summary>
		/// Sets the associated CHMFile instance
		/// </summary>
		/// <param name="associatedFile">instance to set</param>
		internal void SetCHMFile(CHMFile associatedFile)
		{
			_associatedFile = associatedFile;
		}
		#endregion

		/// <summary>
		/// Decodes the binary file data and fills the internal properties
		/// </summary>
		/// <returns>true if succeeded</returns>
		private bool DecodeData()
		{
			bool bRet = true;

			MemoryStream memStream = new MemoryStream(_binaryFileData);
			BinaryReader binReader = new BinaryReader(memStream);
			
			int nCurOffset = 0;

			while( (memStream.Position < memStream.Length) && (bRet) )
			{
				nCurOffset = (int)memStream.Position;
				byte [] dataBlock = binReader.ReadBytes(BLOCK_SIZE);
				bRet &= DecodeBlock(dataBlock, ref nCurOffset);
			}

			return bRet;
		}

		/// <summary>
		/// Decodes a block of url-string data
		/// </summary>
		/// <param name="dataBlock">block of data</param>
		/// <param name="nOffset">current file offset</param>
		/// <returns>true if succeeded</returns>
		private bool DecodeBlock( byte[] dataBlock, ref int nOffset )
		{
			bool bRet = true;
			int blockOffset = nOffset;

			MemoryStream memStream = new MemoryStream(dataBlock);
			BinaryReader binReader = new BinaryReader(memStream);

			if(nOffset==0)
				binReader.ReadByte(); // first block starts with an unknown byte

			while( (memStream.Position < (memStream.Length-8)) && (bRet) )
			{
				int entryOffset = blockOffset + (int)memStream.Position;

				int urlOffset = binReader.ReadInt32();
				int frameOffset = binReader.ReadInt32();


				// There is one way to tell where the end of the URL/FrameName 
				// pairs occurs: Repeat the following: read 2 DWORDs and if both 
				// are less than the current offset then this is the start of the Local 
				// strings else skip two NT strings.
				//				if(( (urlOffset < (entryOffset+8)) && (frameOffset < (entryOffset+8)) ))
				//				{
				//					//TODO: add correct string reading if an offset has been found
				//					/*
				//					int curOffset = (int)memStream.Position;
				//
				//					memStream.Seek( (long)(blockOffset-urlOffset), SeekOrigin.Begin);
				//					string sTemp = CHMReader.ExtractString(ref binReader, 0, true);
				//
				//					memStream.Seek( (long)(blockOffset-frameOffset), SeekOrigin.Begin);
				//					sTemp = CHMReader.ExtractString(ref binReader, 0, true);
				//
				//					memStream.Seek((long)curOffset, SeekOrigin.Begin);
				//					*/
				//
				//
				//					int curOffs = (int)memStream.Position;
				//					BinaryReaderHelp.ExtractString(ref binReader, 0, true, _associatedFile.TextEncoding);
				//					nOffset += (int)memStream.Position - curOffs;
				//
				//					curOffs = (int)memStream.Position;
				//					BinaryReaderHelp.ExtractString(ref binReader, 0, true, _associatedFile.TextEncoding);
				//					nOffset += (int)memStream.Position - curOffs;
				//				} 
				//				else
			{
				bool bFoundTerminator = false;

				string sTemp = BinaryReaderHelp.ExtractString(ref binReader, ref bFoundTerminator, 0, true, _associatedFile.TextEncoding);

				if(sTemp == "")
				{
					//nOffset = nOffset + 1 + (int)memStream.Length - (int)memStream.Position;
					memStream.Seek(memStream.Length-1, SeekOrigin.Begin);
				} 
				else 
				{
					_urlDictionary[entryOffset.ToString()] = sTemp.ToString();
					_framenameDictionary[ entryOffset.ToString() ] =  sTemp.ToString() ;
				}
			}
			}

			return bRet;
		}

		/// <summary>
		/// Gets the url at a given offset
		/// </summary>
		/// <param name="offset">offset of url</param>
		/// <returns>the url at the given offset</returns>
		public string GetURLatOffset(int offset)
		{
			if(offset == -1)
				return String.Empty;

			string sTemp = (string)_urlDictionary[ offset.ToString() ];

			if(sTemp == null)
				return String.Empty;

			return sTemp;
		}

		/// <summary>
		/// Gets the framename at a given offset
		/// </summary>
		/// <param name="offset">offset of the framename</param>
		/// <returns>the frame name at the given offset</returns>
		public string GetFrameNameatOffset(int offset)
		{
			if(offset == -1)
				return String.Empty;

			string sTemp = (string)_framenameDictionary[ offset.ToString() ];

			if(sTemp == null)
				return String.Empty;

			return sTemp;
		}

		/// <summary>
		/// Implement IDisposable.
		/// </summary>
		public void Dispose()
		{
			Dispose(true);
			// This object will be cleaned up by the Dispose method.
			// Therefore, you should call GC.SupressFinalize to
			// take this object off the finalization queue 
			// and prevent finalization code for this object
			// from executing a second time.
			GC.SuppressFinalize(this);
		}

		/// <summary>
		/// Dispose(bool disposing) executes in two distinct scenarios. 
		/// If disposing equals true, the method has been called directly 
		/// or indirectly by a user's code. Managed and unmanaged resources 
		/// can be disposed. 
		/// If disposing equals false, the method has been called by the 
		/// runtime from inside the finalizer and you should not reference  
		/// other objects. Only unmanaged resources can be disposed.
		/// </summary>
		/// <param name="disposing">disposing flag</param>
		private void Dispose(bool disposing)
		{
			// Check to see if Dispose has already been called.
			if(!this.disposed)
			{
				// If disposing equals true, dispose all managed 
				// and unmanaged resources.
				if(disposing)
				{
					// Dispose managed resources.
					_binaryFileData = null;
					_urlDictionary = null;
					_framenameDictionary = null;
				}
			}
			disposed = true;         
		}
	}
}
