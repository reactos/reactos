using System;
using System.IO;
using System.Collections;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMUrltable</c> implements methods to decode the #URLTBL internal file.
	/// </summary>
	internal sealed class CHMUrltable : IDisposable
	{
		/// <summary>
		/// Constant specifying the size of the data blocks
		/// </summary>
		private const int BLOCK_SIZE = 0x1000;
		/// <summary>
		/// Constant specifying the number of records per block
		/// </summary>
		private const int RECORDS_PER_BLOCK = 341;
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal member storing the url table
		/// </summary>
		private ArrayList _urlTable = new ArrayList();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #URLTBL file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMUrltable(byte[] binaryFileData, CHMFile associatedFile)
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
		internal CHMUrltable()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _urlTable.Count );
			foreach(UrlTableEntry curItem in _urlTable)
			{
				curItem.Dump(ref writer);
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
				UrlTableEntry newItem = new UrlTableEntry();
				newItem.SetCHMFile(_associatedFile);
				newItem.ReadDump(ref reader);
				_urlTable.Add(newItem);
			}
		}

		/// <summary>
		/// Sets the associated CHMFile instance
		/// </summary>
		/// <param name="associatedFile">instance to set</param>
		internal void SetCHMFile(CHMFile associatedFile)
		{
			_associatedFile = associatedFile;

			foreach(UrlTableEntry curEntry in _urlTable)
			{
				curEntry.SetCHMFile(associatedFile);
			}
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

			for(int i=0; i < RECORDS_PER_BLOCK; i++)
			{
				int recordOffset = blockOffset + (int)memStream.Position;

				uint nuniqueID = (uint) binReader.ReadInt32(); // unknown dword
				int ntopicsIdx = binReader.ReadInt32();
				int urlstrOffset = binReader.ReadInt32();

				UrlTableEntry newEntry = new UrlTableEntry(nuniqueID, recordOffset, ntopicsIdx, urlstrOffset, _associatedFile);
				_urlTable.Add(newEntry);

				if( memStream.Position >= memStream.Length)
					break;
			}

			if(dataBlock.Length == BLOCK_SIZE)
				binReader.ReadInt32();

			return bRet;
		}

		/// <summary>
		/// Gets the arraylist containing all urltable entries.
		/// </summary>
		public ArrayList UrlTable
		{
			get
			{
				return _urlTable;
			}
		}

		/// <summary>
		/// Gets the urltable entry of a given offset
		/// </summary>
		public UrlTableEntry this[int offset]
		{
			get
			{
				foreach(UrlTableEntry curEntry in _urlTable)
					if(curEntry.EntryOffset == offset)
						return curEntry;

				return null;
			}
		}

		/// <summary>
		/// Gets the urltable entry of a given uniqueID
		/// </summary>
		public UrlTableEntry GetByUniqueID(uint uniqueID)
		{
			foreach(UrlTableEntry curEntry in UrlTable)
			{
				if(curEntry.UniqueID == uniqueID)
					return curEntry;
			}

			return null;
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
					_urlTable = null;
				}
			}
			disposed = true;         
		}
	}
}
