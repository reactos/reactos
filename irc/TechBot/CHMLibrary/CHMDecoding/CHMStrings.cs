using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMStrings</c> implements a string collection read from the #STRINGS file
	/// </summary>
	internal sealed class CHMStrings : IDisposable
	{
		/// <summary>
		/// Constant specifying the size of the string blocks
		/// </summary>
		private const int STRING_BLOCK_SIZE = 4096;
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal member storing the string dictionary
		/// </summary>
		private Hashtable _stringDictionary = new Hashtable();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #STRINGS file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMStrings(byte[] binaryFileData, CHMFile associatedFile)
		{
			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;
			DecodeData();
		}

		
		/// <summary>
		/// Standard constructor
		/// </summary>
		internal CHMStrings()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _stringDictionary.Count );

			if (_stringDictionary.Count != 0)
			{
				IDictionaryEnumerator iDictionaryEnumerator = _stringDictionary.GetEnumerator();
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
			int nCnt = reader.ReadInt32();

			for(int i=0; i<nCnt;i++)
			{
				int nKey = reader.ReadInt32();
				string sValue = reader.ReadString();
				
				_stringDictionary[nKey.ToString()] = sValue;
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
	
			//binReader.ReadByte(); // file starts with a NULL character for 0-based offset indexing
			
			int nStringOffset = 0;
			int nSubsetOffset = 0;

			while( (memStream.Position < memStream.Length) && (bRet) )
			{
				nStringOffset = (int)memStream.Position;
				byte [] stringBlock = binReader.ReadBytes(STRING_BLOCK_SIZE);
				bRet &= DecodeBlock(stringBlock, ref nStringOffset, ref nSubsetOffset);
			}

			return bRet;
		}

		/// <summary>
		/// Decodes a string block
		/// </summary>
		/// <param name="stringBlock">byte array which represents the string block</param>
		/// <param name="nStringOffset">current string offset number</param>
		/// <param name="nSubsetOffset">reference to a subset variable</param>
		/// <returns>true if succeeded</returns>
		/// <remarks>If a string crosses the end of a block then it will be cut off 
		/// without a NT and repeated in full, with a NT, at the start of the next block. 
		/// For eg "To customize the appearance of a contents file" might become 
		/// "To customize the (block ending)To customize the appearance of a contents file" 
		/// when there are 17 bytes left at the end of the block. </remarks>
		private bool DecodeBlock( byte[] stringBlock, ref int nStringOffset, ref int nSubsetOffset)
		{
			bool bRet = true;

			MemoryStream memStream = new MemoryStream(stringBlock);
			BinaryReader binReader = new BinaryReader(memStream);

			while( (memStream.Position < memStream.Length) && (bRet) )
			{
				bool bFoundTerminator = false;

				int nCurOffset = nStringOffset + (int)memStream.Position;

				string sTemp = BinaryReaderHelp.ExtractString(ref binReader, ref bFoundTerminator, 0, true, _associatedFile.TextEncoding);

				if(nSubsetOffset != 0)
				{
					_stringDictionary[nSubsetOffset.ToString()] = sTemp.ToString();
				} 
				else 
				{
					_stringDictionary[nCurOffset.ToString()] = sTemp.ToString();
				}

				if( bFoundTerminator )
				{
					nSubsetOffset = 0;
				} 
				else 
				{
					nSubsetOffset = nCurOffset;
				}
			}

			return bRet;
		}

		/// <summary>
		/// Indexer which returns the string at a given offset
		/// </summary>
		public string this[int offset]
		{
			get
			{
				if(offset == -1)
					return String.Empty;

				string sTemp = (string)_stringDictionary[ offset.ToString() ];

				if(sTemp == null)
					return String.Empty;

				return sTemp;
			}
		}

		/// <summary>
		/// Indexer which returns the string at a given offset
		/// </summary>
		public string this[string offset]
		{
			get
			{
				if(offset == "-1")
					return String.Empty;

				string sTemp = (string)_stringDictionary[ offset ];

				if(sTemp == null)
					return String.Empty;

				return sTemp;
			}
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
					_stringDictionary = null;
				}
			}
			disposed = true;         
		}
	}
}
