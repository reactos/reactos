using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMTopics</c> implements functionality to decode the #TOPICS internal file
	/// </summary>
	internal sealed class CHMTopics : IDisposable
	{
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;
		/// <summary>
		/// Internal member storing the topic list
		/// </summary>
		private ArrayList _topicTable = new ArrayList();

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #TOPICS file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMTopics(byte[] binaryFileData, CHMFile associatedFile)
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
		internal CHMTopics()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _topicTable.Count );
			foreach(TopicEntry curItem in _topicTable)
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
				TopicEntry newItem = new TopicEntry();
				newItem.SetCHMFile(_associatedFile);
				newItem.ReadDump(ref reader);
				_topicTable.Add(newItem);
			}
		}

		/// <summary>
		/// Sets the associated CHMFile instance
		/// </summary>
		/// <param name="associatedFile">instance to set</param>
		internal void SetCHMFile(CHMFile associatedFile)
		{
			_associatedFile = associatedFile;

			foreach(TopicEntry curEntry in _topicTable)
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
				int entryOffset = nCurOffset;
				int tocIdx = binReader.ReadInt32();
				int titleOffset = binReader.ReadInt32();
				int urltablOffset = binReader.ReadInt32();
				int visibilityMode = binReader.ReadInt16();
				int unknownMode = binReader.ReadInt16();

				TopicEntry newEntry = new TopicEntry(entryOffset, tocIdx, titleOffset, urltablOffset, visibilityMode, unknownMode, _associatedFile);
				_topicTable.Add( newEntry );

				nCurOffset = (int)memStream.Position;
			}

			return bRet;
		}

		/// <summary>
		/// Gets the arraylist containing all topic entries.
		/// </summary>
		public ArrayList TopicTable
		{
			get
			{
				return _topicTable;
			}
		}

		/// <summary>
		/// Gets the topic entry of a given offset
		/// </summary>
		public TopicEntry this[int offset]
		{
			get
			{
				foreach(TopicEntry curEntry in _topicTable)
					if(curEntry.EntryOffset == offset)
						return curEntry;

				return null;
			}
		}

		/// <summary>
		/// Searches a topic by the locale name
		/// </summary>
		/// <param name="locale">locale name to search</param>
		/// <returns>The topicentry instance if found, otherwise null</returns>
		public TopicEntry GetByLocale(string locale)
		{
			foreach(TopicEntry curEntry in TopicTable)
			{
				if(curEntry.Locale.ToLower() == locale.ToLower())
					return curEntry;
			}

			return null;
		}

		/// <summary>
		/// Searches the topics for all files with a given file extension
		/// </summary>
		/// <param name="fileExtension">extension to search</param>
		/// <returns>An arraylist of TopicEntry instances or null if no topic was found</returns>
		public ArrayList GetByExtension(string fileExtension)
		{
			ArrayList arrRet = new ArrayList();

			foreach(TopicEntry curEntry in TopicTable)
			{
				if(curEntry.Locale.ToLower().EndsWith(fileExtension.ToLower()))
					arrRet.Add(curEntry);
			}

			if(arrRet.Count > 0)
				return arrRet;

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
					_topicTable=null;
				}
			}
			disposed = true;         
		}
	}
}
