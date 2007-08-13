using System;
using System.IO;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>UrlTableEntry</c> stores data for an URL-Table entry
	/// </summary>
	internal sealed class UrlTableEntry
	{
		/// <summary>
		/// Internal member storing the offset of this entry
		/// </summary>
		private int _entryOffset = 0;
		/// <summary>
		/// Internal member storing a unique id
		/// </summary>
		private uint _uniqueID = 0;
		/// <summary>
		/// Internal member storing the topics index
		/// </summary>
		private int _topicsIndex = 0;
		/// <summary>
		/// Internal member storing the offset in the urlstr table
		/// </summary>
		private int _urlStrOffset = 0;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="uniqueID">unique id</param>
		/// <param name="entryOffset">offset of the entry</param>
		/// <param name="topicIndex">topic index</param>
		/// <param name="urlstrOffset">urlstr offset for filename</param>
		public UrlTableEntry(uint uniqueID, int entryOffset, int topicIndex, int urlstrOffset) : this(uniqueID, entryOffset, topicIndex, urlstrOffset, null)
		{
		}

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="uniqueID">unique id</param>
		/// <param name="entryOffset">offset of the entry</param>
		/// <param name="topicIndex">topic index</param>
		/// <param name="urlstrOffset">urlstr offset for filename</param>
		/// <param name="associatedFile">associated chm file</param>
		internal UrlTableEntry(uint uniqueID, int entryOffset, int topicIndex, int urlstrOffset, CHMFile associatedFile)
		{
			_uniqueID = uniqueID;
			_entryOffset = entryOffset;
			_topicsIndex = topicIndex;
			_urlStrOffset = urlstrOffset;
			_associatedFile = associatedFile;
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		internal UrlTableEntry()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _urlStrOffset );
			writer.Write( _entryOffset );
			writer.Write( _topicsIndex );
			writer.Write( _urlStrOffset );
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			_urlStrOffset = reader.ReadInt32();
			_entryOffset = reader.ReadInt32();
			_topicsIndex = reader.ReadInt32();
			_urlStrOffset = reader.ReadInt32();
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
		/// Gets the unique id of the entry
		/// </summary>
		internal uint UniqueID
		{
			get {return _uniqueID; }
		}

		/// <summary>
		/// Gets the offset of the entry
		/// </summary>
		internal int EntryOffset
		{
			get {return _entryOffset; }
		}

		/// <summary>
		/// Gets the topics index
		/// </summary>
		internal int TopicIndex
		{
			get {return _topicsIndex; }
		}

		/// <summary>
		/// Gets the urlstr offset
		/// </summary>
		internal int UrlstrOffset
		{
			get { return _urlStrOffset; }
		}
		
		/// <summary>
		/// Gets the url of the entry
		/// </summary>
		public string URL
		{
			get
			{
				if(_associatedFile == null)
					return String.Empty;

				if(_associatedFile.UrlstrFile == null)
					return String.Empty;

				string sTemp = (string)_associatedFile.UrlstrFile.GetURLatOffset( _urlStrOffset );

				if( sTemp == null)
					return String.Empty;

				return sTemp;
			}
		}

		/// <summary>
		/// Gets the associated topic for this url entry
		/// </summary>
		internal TopicEntry Topic
		{
			get
			{
				if(_associatedFile == null)
					return null;

				if(_associatedFile.TopicsFile == null)
					return null;

				TopicEntry tentry = _associatedFile.TopicsFile[ _topicsIndex*16 ];

				return tentry;
			}
		}
	}
}
