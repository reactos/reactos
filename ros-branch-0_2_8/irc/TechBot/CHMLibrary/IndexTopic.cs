using System;
using System.IO;

using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>IndexTopic</c> implements an entry for the <see cref="IndexItem">IndexItem</see> topics list.
	/// </summary>
	public sealed class IndexTopic
	{
		private DataMode _topicMode = DataMode.TextBased;
		private string _title="";
		private string _local="";
		private string _compileFile = "";
		private string _chmPath = "";
		private int _topicOffset = -1;
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Creates a new instance of the class based on an existing TopicEntry
		/// </summary>
		/// <param name="entry"></param>
		internal static IndexTopic FromTopicEntry(TopicEntry entry)
		{
			return new IndexTopic(entry.EntryOffset, entry.ChmFile);
			//return new IndexTopic( entry.Title, entry.Locale, entry.ChmFile.CompileFile, entry.ChmFile.ChmFilePath);
		}

		/// <summary>
		/// Creates a new instance of the class (binary extraction mode)
		/// </summary>
		/// <param name="topicOffset">offset of the topic entry</param>
		/// <param name="associatedFile">associated CHMFile instance</param>
		internal IndexTopic(int topicOffset, CHMFile associatedFile)
		{
			_topicMode = DataMode.Binary;
			_topicOffset = topicOffset;
			_associatedFile = associatedFile;
		}

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="Title">topic title</param>
		/// <param name="local">topic local (content filename)</param>
		/// <param name="compilefile">name of the chm file (location of topic)</param>
		/// <param name="chmpath">path of the chm file</param>
		public IndexTopic(string Title, string local, string compilefile, string chmpath)
		{
			_topicMode = DataMode.TextBased;
			_title = Title;
			_local = local;
			_compileFile = compilefile;
			_chmPath = chmpath;
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			writer.Write((int)_topicMode);

			if(_topicMode==DataMode.TextBased)
			{
				writer.Write(_title);
				writer.Write(_local);
			} 
			else 
			{
				writer.Write(_topicOffset);
			}
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			_topicMode = (DataMode)reader.ReadInt32();

			if(_topicMode==DataMode.TextBased)
			{
				_title = reader.ReadString();
				_local = reader.ReadString();
			} 
			else 
			{
				_topicOffset = reader.ReadInt32();
			}
		}
		#endregion

		/// <summary>
		/// Internally used to set the chm-finos when reading from dump store
		/// </summary>
		/// <param name="compilefile"></param>
		/// <param name="chmpath"></param>
		internal void SetChmInfo(string compilefile, string chmpath)
		{
			_compileFile = compilefile;
			_chmPath = chmpath;
		}

		/// <summary>
		/// Gets/Sets the associated CHMFile instance
		/// </summary>
		internal CHMFile AssociatedFile
		{
			get { return _associatedFile; }
			set { _associatedFile = value; }
		}

		/// <summary>
		/// Gets the topic title
		/// </summary>
		public string Title
		{
			get 
			{ 
				if((_topicMode == DataMode.Binary )&&(_associatedFile!=null))
				{
					if( _topicOffset >= 0)
					{
						TopicEntry te = (TopicEntry) (_associatedFile.TopicsFile[_topicOffset]);
						if(te != null)
						{
							return te.Title;
						}
					}
				}

				return _title; 
			}
		}

		/// <summary>
		/// Gets the local (content filename)
		/// </summary>
		public string Local
		{
			get 
			{
				if((_topicMode == DataMode.Binary )&&(_associatedFile!=null))
				{
					if( _topicOffset >= 0)
					{
						TopicEntry te = (TopicEntry) (_associatedFile.TopicsFile[_topicOffset]);
						if(te != null)
						{
							return te.Locale;
						}
					}
				}

				return _local; 
			}
		}

		/// <summary>
		/// Gets the compile file (location)
		/// </summary>
		public string CompileFile
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.CompileFile;

				return _compileFile; 
			}
		}

		/// <summary>
		/// Gets the chm file path
		/// </summary>
		public string ChmFilePath
		{
			get 
			{
				if(_associatedFile != null)
					return _associatedFile.ChmFilePath;

				return _chmPath; 
			}
		}

		/// <summary>
		/// Gets the url
		/// </summary>
		public string URL
		{
			get
			{
				string sL = Local;

				if(sL.Length<=0)
					return "";//"about:blank";

				if( (sL.ToLower().IndexOf("http://") >= 0) ||
					(sL.ToLower().IndexOf("https://") >= 0) ||
					(sL.ToLower().IndexOf("mailto:") >= 0) ||
					(sL.ToLower().IndexOf("ftp://") >= 0) ||
					(sL.ToLower().IndexOf("ms-its:") >= 0))
					return sL;

				return HtmlHelpSystem.UrlPrefix + ChmFilePath + "::/" + sL;
			}
		}
	}
}
