using System;
using System.IO;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>TopicEntry</c> stores the data for one topic entry
	/// </summary>
	internal sealed class TopicEntry
	{
		/// <summary>
		/// Internal member storing the offset of this topic entry
		/// </summary>
		private int _entryOffset = 0;
		/// <summary>
		/// Internal member storing the index of the binary toc
		/// </summary>
		private int _tocidxOffset = 0;
		/// <summary>
		/// Internal member storing the string offset of the title
		/// </summary>
		private int _titleOffset = 0;
		/// <summary>
		/// Internal member storuing the urltable offset
		/// </summary>
		private int _urltableOffset = 0;
		/// <summary>
		/// Internal member storing the visibility mode
		/// </summary>
		private int _visibilityMode = 0;
		/// <summary>
		/// Internal member storing an unknown mode
		/// </summary>
		private int _unknownMode = 0;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="entryOffset">offset of this entry</param>
		/// <param name="tocidxOffset">offset in the binary toc index</param>
		/// <param name="titleOffset">offset of the title (in the #STRINGS file)</param>
		/// <param name="urltableOffset">offset in the urltable containing the urlstr offset for the url</param>
		/// <param name="visibilityMode">visibility mode 2 indicates not in contents, 6 indicates that it is in the contents, 0/4 something else (unknown)</param>
		/// <param name="unknownMode">0, 2, 4, 8, 10, 12, 16, 32 (unknown)</param>
		public TopicEntry(int entryOffset, int tocidxOffset, int titleOffset, int urltableOffset, int visibilityMode, int unknownMode) :this(entryOffset, tocidxOffset, titleOffset, urltableOffset, visibilityMode, unknownMode, null)
		{
			
		}

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="entryOffset">offset of this entry</param>
		/// <param name="tocidxOffset">offset in the binary toc index</param>
		/// <param name="titleOffset">offset of the title (in the #STRINGS file)</param>
		/// <param name="urltableOffset">offset in the urltable containing the urlstr offset for the url</param>
		/// <param name="visibilityMode">visibility mode 2 indicates not in contents, 6 indicates that it is in the contents, 0/4 something else (unknown)</param>
		/// <param name="unknownMode">0, 2, 4, 8, 10, 12, 16, 32 (unknown)</param>
		/// <param name="associatedFile">associated chmfile object</param>
		internal TopicEntry(int entryOffset, int tocidxOffset, int titleOffset, int urltableOffset, int visibilityMode, int unknownMode, CHMFile associatedFile)
		{
			_entryOffset = entryOffset;
			_tocidxOffset = tocidxOffset;
			_titleOffset = titleOffset;
			_urltableOffset = urltableOffset;
			_visibilityMode = visibilityMode;
			_unknownMode = unknownMode;
			_associatedFile = associatedFile;
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		internal TopicEntry()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write( _entryOffset );
			writer.Write( _tocidxOffset );
			writer.Write( _titleOffset );
			writer.Write( _urltableOffset );
			writer.Write( _visibilityMode );
			writer.Write( _unknownMode );
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			_entryOffset = reader.ReadInt32();
			_tocidxOffset = reader.ReadInt32();
			_titleOffset = reader.ReadInt32();
			_urltableOffset = reader.ReadInt32();
			_visibilityMode = reader.ReadInt32();
			_unknownMode = reader.ReadInt32();
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
		/// Gets the associated chm file
		/// </summary>
		internal CHMFile ChmFile
		{
			get { return _associatedFile; }
		}

		/// <summary>
		/// Gets the offset of this entry
		/// </summary>
		internal int EntryOffset
		{
			get { return _entryOffset; }
		}

		/// <summary>
		/// Gets the tocidx offset
		/// </summary>
		internal int TOCIdxOffset
		{
			get { return _tocidxOffset; }
		}

		/// <summary>
		/// Gets the title offset of the #STRINGS file
		/// </summary>
		internal int TitleOffset
		{
			get { return _titleOffset; }
		}

		/// <summary>
		/// Gets the urltable offset
		/// </summary>
		internal int UrlTableOffset
		{
			get { return _urltableOffset; }
		}

		/// <summary>
		/// Gets the title of the topic entry
		/// </summary>
		public string Title
		{
			get
			{
				if( _associatedFile == null)
					return String.Empty;

				if( _associatedFile.StringsFile == null)
					return String.Empty;

				string sTemp = (string)_associatedFile.StringsFile[ _titleOffset ];

				if(sTemp == null)
					return String.Empty;

				return sTemp;
			}
		}

		/// <summary>
		/// Gets the url of the topic
		/// </summary>
		public string Locale
		{
			get
			{
				if( _associatedFile == null)
					return String.Empty;

				if( _associatedFile.UrltblFile == null)
					return String.Empty;

				UrlTableEntry utEntry = (UrlTableEntry)_associatedFile.UrltblFile[ _urltableOffset ];

				if(utEntry == null)
					return String.Empty;

				if(utEntry.URL == "")
					return String.Empty;

				return utEntry.URL;
			}
		}

		/// <summary>
		/// Gets the URL of this topic
		/// </summary>
		public string URL
		{
			get
			{
				if(Locale.Length <= 0)
					return "about:blank";

				if( (Locale.ToLower().IndexOf("http://") >= 0) ||
					(Locale.ToLower().IndexOf("https://") >= 0) ||
					(Locale.ToLower().IndexOf("mailto:") >= 0) ||
					(Locale.ToLower().IndexOf("ftp://") >= 0) ||
					(Locale.ToLower().IndexOf("ms-its:") >= 0))
					return Locale;

				return HtmlHelpSystem.UrlPrefix + _associatedFile.ChmFilePath + "::/" + Locale;
			}
		}

		/// <summary>
		/// Gets the visibility mode
		/// </summary>
		public int VisibilityMode
		{
			get { return _visibilityMode; }
		}

		/// <summary>
		/// Gets the unknown mode
		/// </summary>
		public int UknownMode
		{
			get { return _unknownMode; }
		}		
	}
}
