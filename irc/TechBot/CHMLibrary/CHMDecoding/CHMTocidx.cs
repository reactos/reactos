using System;
using System.IO;
using System.Collections;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMTocidx</c> implements functions to decode the #TOCIDX internal file.
	/// </summary>
	internal sealed class CHMTocidx : IDisposable
	{
		/// <summary>
		/// Constant specifying the size of the data blocks
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
		/// Internal memebr storing the offset to the 20/28 byte structs
		/// </summary>
		private int _offset2028 = 0;
		/// <summary>
		/// Internal member storing the offset to the 16 byte structs
		/// </summary>
		private int _offset16structs = 0;
		/// <summary>
		/// Internal member storing the number of 16 byte structs
		/// </summary>
		private int _numberOf16structs = 0;
		/// <summary>
		/// Internal member storing the offset to the topic list
		/// </summary>
		private int _offsetOftopics = 0;
		/// <summary>
		/// Internal member storing the toc
		/// </summary>
		private ArrayList _toc = new ArrayList();
		/// <summary>
		/// Internal member for offset seeking
		/// </summary>
		private Hashtable _offsetTable = new Hashtable();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #TOCIDX file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMTocidx(byte[] binaryFileData, CHMFile associatedFile)
		{
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
			_toc = new ArrayList();
			_offsetTable = new Hashtable();

			bool bRet = true;

			MemoryStream memStream = new MemoryStream(_binaryFileData);
			BinaryReader binReader = new BinaryReader(memStream);
			
			int nCurOffset = 0;

			_offset2028 = binReader.ReadInt32();
			_offset16structs = binReader.ReadInt32();
			_numberOf16structs = binReader.ReadInt32();
			_offsetOftopics = binReader.ReadInt32();

			binReader.BaseStream.Seek( _offset2028, SeekOrigin.Begin );

			if( RecursivelyBuildTree(ref binReader, _offset2028, _toc, null) )
			{
				binReader.BaseStream.Seek( _offset16structs, SeekOrigin.Begin );
				nCurOffset = (int)binReader.BaseStream.Position;

				for(int i=0; i < _numberOf16structs; i++)
				{
					int tocOffset = binReader.ReadInt32();
					int sqNr = binReader.ReadInt32();
					int topOffset = binReader.ReadInt32();
					int hhctopicIdx = binReader.ReadInt32();

					nCurOffset = (int)binReader.BaseStream.Position;

					int topicIdx = -1;
					// if the topic offset is within the range of the stream
					// and is >= the offset of the first topic dword
					if((topOffset < (binReader.BaseStream.Length - 4)) && (topOffset >= _offsetOftopics))
					{
						// read the index of the topic for this item
						binReader.BaseStream.Seek( topOffset, SeekOrigin.Begin);
						topicIdx = binReader.ReadInt32();
						binReader.BaseStream.Seek( nCurOffset, SeekOrigin.Begin);
					

						TOCItem item = (TOCItem)_offsetTable[tocOffset.ToString()];
						if( item != null)
						{
							if(( topicIdx < _associatedFile.TopicsFile.TopicTable.Count)&&(topicIdx>=0))
							{
								TopicEntry te = (TopicEntry) (_associatedFile.TopicsFile.TopicTable[topicIdx]);
								if( (te != null) && (item.TopicOffset < 0) )
								{
									item.TopicOffset = te.EntryOffset;
								}
					
							}
						}
					}
				}
			}

			return bRet;
		}
		
		/// <summary>
		/// Recursively reads the binary toc tree from the file
		/// </summary>
		/// <param name="binReader">reference to binary reader</param>
		/// <param name="NodeOffset">offset of the first node in the current level</param>
		/// <param name="level">arraylist of TOCItems for the current level</param>
		/// <param name="parentItem">parent item for the item</param>
		/// <returns>Returns true if succeeded</returns>
		private bool RecursivelyBuildTree(ref BinaryReader binReader, int NodeOffset, ArrayList level, TOCItem parentItem)
		{
			bool bRet = true;
			int nextOffset=0;

			int nReadOffset = (int)binReader.BaseStream.Position;

			binReader.BaseStream.Seek(NodeOffset, SeekOrigin.Begin);

			do
			{
				int nCurOffset = (int)binReader.BaseStream.Position;

				int unkn1 = binReader.ReadInt16(); // unknown
				int unkn2 = binReader.ReadInt16(); // unknown

				int flag = binReader.ReadInt32();

				int nFolderAdd = 0;

				if((_associatedFile != null) && (_associatedFile.ImageTypeFolder))
				{
					// get the value which should be added, to display folders instead of books
					if(HtmlHelpSystem.UseHH2TreePics) 
						nFolderAdd = 8;
					else
						nFolderAdd = 4;
				}

				int nFolderImgIdx = (HtmlHelpSystem.UseHH2TreePics ? (TOCItem.STD_FOLDER_HH2+nFolderAdd) : (TOCItem.STD_FOLDER_HH1+nFolderAdd));
				int nFileImgIdx = (HtmlHelpSystem.UseHH2TreePics ? TOCItem.STD_FILE_HH2 : TOCItem.STD_FILE_HH1);

				int stdImage =  ((flag & 0x4)!=0) ? nFolderImgIdx : nFileImgIdx;

				int stringOffset = binReader.ReadInt32();

				int ParentOffset = binReader.ReadInt32();
				nextOffset = binReader.ReadInt32();

				int firstChildOffset = 0;
				int unkn3=0;

				if( (flag&0x4)!=0 )
				{
					firstChildOffset = binReader.ReadInt32();
					unkn3 = binReader.ReadInt32(); // unknown
				}

				TOCItem newItem = new TOCItem();
				newItem.ImageIndex = stdImage;
				newItem.Offset = nCurOffset;
				newItem.OffsetNext = nextOffset;
				newItem.AssociatedFile = _associatedFile;
				newItem.TocMode = DataMode.Binary;
				newItem.Parent = parentItem;

				if( (flag&0x08) == 0)
				{
					// toc item doesn't have a local value (=> stringOffset = offset of strings file)
					newItem.Name = _associatedFile.StringsFile[stringOffset];
				} 
				else 
				{
					// this item has a topic entry (=> stringOffset = index of topic entry)
					if((stringOffset < _associatedFile.TopicsFile.TopicTable.Count) && (stringOffset >= 0))
					{
						TopicEntry te = (TopicEntry) (_associatedFile.TopicsFile.TopicTable[stringOffset]);
						if(te != null)
						{
							newItem.TopicOffset = te.EntryOffset;
						}
					}
				}

				_offsetTable[nCurOffset.ToString()] = newItem;

				// if this item has children (firstChildOffset > 0)
				if( firstChildOffset > 0)
				{
					bRet &= RecursivelyBuildTree(ref binReader, firstChildOffset, newItem.Children, newItem);
				}

				level.Add( newItem );

				if(nCurOffset != nextOffset)
					binReader.BaseStream.Seek(nextOffset, SeekOrigin.Begin);

			}while(nextOffset != 0);

			binReader.BaseStream.Seek(nReadOffset, SeekOrigin.Begin);

			return bRet;
		}

		/// <summary>
		/// Gets the internal read toc
		/// </summary>
		internal ArrayList TOC
		{
			get { return _toc; }
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
					_toc = null;
					_offsetTable = null;
				}
			}
			disposed = true;         
		}
	}
}
