using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMBtree</c> implements methods/properties to decode the binary help index. 
	/// This class automatically creates an index arraylist for the current CHMFile instance. 
	/// It does not store the index internally !
	/// </summary>
	/// <remarks>The binary index can be found in the storage file $WWKeywordLinks/BTree</remarks>
	internal sealed class CHMBtree : IDisposable
	{
		/// <summary>
		/// Constant specifying the size of the string blocks
		/// </summary>
		private const int BLOCK_SIZE = 2048;
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal member storing flags
		/// </summary>
		private int _flags = 0;
		/// <summary>
		/// Internal member storing the data format
		/// </summary>
		private byte[] _dataFormat = new byte[16];
		/// <summary>
		/// Internal member storing the index of the last listing block
		/// </summary>
		private int _indexOfLastListingBlock = 0;
		/// <summary>
		/// Internal member storing the index of the root block
		/// </summary>
		private int _indexOfRootBlock = 0;
		/// <summary>
		/// Internal member storing the number of blocks
		/// </summary>
		private int _numberOfBlocks = 0;
		/// <summary>
		/// Internal member storing the tree depth. 
		/// (1 if no index blocks, 2 one level of index blocks, ...)
		/// </summary>
		private int _treeDepth = 0;
		/// <summary>
		/// Internal member storing the number of keywords in the file
		/// </summary>
		private int _numberOfKeywords = 0;
		/// <summary>
		/// Internal member storing the codepage
		/// </summary>
		private int _codePage = 0;
		/// <summary>
		/// true if the index is from a CHI or CHM file, else CHW
		/// </summary>
		private bool _isCHI_CHM = true;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;
		/// <summary>
		/// Internal flag specifying if we have to read listing or index blocks
		/// </summary>
		private bool _readListingBlocks = true;
		/// <summary>
		/// Internal member storing an indexlist of the current file.
		/// </summary>
		private ArrayList _indexList = new ArrayList();

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the $WWKeywordLinks/BTree file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMBtree(byte[] binaryFileData, CHMFile associatedFile)
		{
			if( associatedFile == null)
			{
				throw new ArgumentException("CHMBtree.ctor() - Associated CHMFile must not be null !", "associatedFile");
			}

			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;
			DecodeData();

			// clear internal binary data after extraction
			_binaryFileData = null;
		}

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
			int nTemp = 0;

			// decode header
			binReader.ReadChars(2); // 2chars signature (not important)
			
			_flags = (int)binReader.ReadInt16(); // WORD flags

			binReader.ReadInt16(); // size of blocks (always 2048)

			_dataFormat = binReader.ReadBytes(16);
			
			binReader.ReadInt32();  // unknown DWORD

			_indexOfLastListingBlock = binReader.ReadInt32();
			_indexOfRootBlock = binReader.ReadInt32();

			binReader.ReadInt32(); // unknown DWORD

			_numberOfBlocks = binReader.ReadInt32();
			_treeDepth = binReader.ReadInt16();
			_numberOfKeywords = binReader.ReadInt32();
			_codePage = binReader.ReadInt32();

			binReader.ReadInt32(); // lcid DWORD
			
			nTemp = binReader.ReadInt32();
			_isCHI_CHM = (nTemp==1);

			binReader.ReadInt32(); // unknown DWORD
			binReader.ReadInt32(); // unknown DWORD
			binReader.ReadInt32(); // unknown DWORD
			binReader.ReadInt32(); // unknown DWORD

			// end of header decode

			while( (memStream.Position < memStream.Length) && (bRet) )
			{
				nCurOffset = (int)memStream.Position;
				byte [] dataBlock = binReader.ReadBytes(BLOCK_SIZE);
				bRet &= DecodeBlock(dataBlock, ref nCurOffset, _treeDepth-1);
			}

			return bRet;
		}

		/// <summary>
		/// Decodes a block of url-string data
		/// </summary>
		/// <param name="dataBlock">block of data</param>
		/// <param name="nOffset">current file offset</param>
		/// <param name="indexBlocks">number of index blocks</param>
		/// <returns>true if succeeded</returns>
		private bool DecodeBlock( byte[] dataBlock, ref int nOffset, int indexBlocks )
		{
			bool bRet = true;
			int nblockOffset = nOffset;

			MemoryStream memStream = new MemoryStream(dataBlock);
			BinaryReader binReader = new BinaryReader(memStream);

			int freeSpace = binReader.ReadInt16(); // length of freespace
			int nrOfEntries = binReader.ReadInt16(); // number of entries

			bool bListingEndReached = false;

			//while( (memStream.Position < (memStream.Length-freeSpace)) && (bRet) )
			//{
				int nIndexOfPrevBlock = -1;
				int nIndexOfNextBlock = -1;
				int nIndexOfChildBlock = 0;
				
				if(_readListingBlocks)
				{
					nIndexOfPrevBlock = binReader.ReadInt32(); // -1 if this is the header
					nIndexOfNextBlock = binReader.ReadInt32(); // -1 if this is the last block
				} 
				else 
				{
					nIndexOfChildBlock = binReader.ReadInt32(); 
				}

				for(int nE = 0; nE < nrOfEntries; nE++)
				{
					if(_readListingBlocks)
					{
						bListingEndReached = (nIndexOfNextBlock==-1);

						string keyWord = BinaryReaderHelp.ExtractUTF16String(ref binReader, 0, true, _associatedFile.TextEncoding);

						bool isSeeAlsoKeyword = (binReader.ReadInt16()!=0);
				
						int indent = binReader.ReadInt16(); // indent of entry
						int nCharIndex = binReader.ReadInt32();

						binReader.ReadInt32();

						int numberOfPairs =	binReader.ReadInt32();

						int[] nTopics = new int[numberOfPairs];
						string[] seeAlso = new string[numberOfPairs];

						for(int i=0; i < numberOfPairs; i++)
						{
							if(isSeeAlsoKeyword)
							{
								seeAlso[i] = BinaryReaderHelp.ExtractUTF16String(ref binReader, 0, true, _associatedFile.TextEncoding);
							} 
							else 
							{
								nTopics[i] = binReader.ReadInt32();
							}
						}

						binReader.ReadInt32(); // unknown

						int nIndexOfThisEntry = binReader.ReadInt32();

						IndexItem newItem = new IndexItem(_associatedFile, keyWord, isSeeAlsoKeyword, indent, nCharIndex, nIndexOfThisEntry, seeAlso, nTopics);
						_indexList.Add(newItem);
					} 
					else 
					{
						string keyWord = BinaryReaderHelp.ExtractUTF16String(ref binReader, 0, true, _associatedFile.TextEncoding);

						bool isSeeAlsoKeyword = (binReader.ReadInt16()!=0);
				
						int indent = binReader.ReadInt16(); // indent of entry
						int nCharIndex = binReader.ReadInt32();

						binReader.ReadInt32();

						int numberOfPairs =	binReader.ReadInt32();

						int[] nTopics = new int[numberOfPairs];
						string[] seeAlso = new string[numberOfPairs];

						for(int i=0; i < numberOfPairs; i++)
						{
							if(isSeeAlsoKeyword)
							{
								seeAlso[i] = BinaryReaderHelp.ExtractUTF16String(ref binReader, 0, true, _associatedFile.TextEncoding);
							} 
							else 
							{
								nTopics[i] = binReader.ReadInt32();
							}
						}

						int nIndexChild = binReader.ReadInt32();
						int nIndexOfThisEntry=-1;

						IndexItem newItem = new IndexItem(_associatedFile, keyWord, isSeeAlsoKeyword, indent, nCharIndex, nIndexOfThisEntry, seeAlso, nTopics);
						_indexList.Add(newItem);

					}
				}
			//}

			binReader.ReadBytes(freeSpace);
			

			if( bListingEndReached )
				_readListingBlocks = false;

			return bRet;
		}

		/// <summary>
		/// Gets the internal generated index list
		/// </summary>
		internal ArrayList IndexList
		{
			get { return _indexList; }
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
				}
			}
			disposed = true;         
		}
	}
}
