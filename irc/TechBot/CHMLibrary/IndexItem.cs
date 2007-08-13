using System;
using System.IO;
using System.Text;
using System.Collections;

using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>IndexItem</c> implements an help-index item
	/// </summary>
	public sealed class IndexItem : IComparable
	{
		/// <summary>
		/// Internal member storing the keyword
		/// </summary>
		private string _keyWord = "";
		/// <summary>
		/// Internal member storing all associated information type strings
		/// </summary>
		private ArrayList _infoTypeStrings = new ArrayList();
		/// <summary>
		/// Internal member storing the flag if this is a see-also keyword
		/// </summary>
		private bool _isSeeAlso = false;
		/// <summary>
		/// Internal member storing the indent of the keyword
		/// </summary>
		private int _indent = 0;
		/// <summary>
		/// Internal member storing the last index of the keyword in the seperated list
		/// </summary>
		private int _charIndex = 0;
		/// <summary>
		/// Internal member storing the entry index
		/// </summary>
		private int _entryIndex = 0;
		/// <summary>
		/// Internal member storing an array of see-also values
		/// </summary>
		private string[] _seeAlso = new string[0];
		/// <summary>
		/// Internal member storing an array of topic offsets
		/// </summary>
		private int[] _nTopics = new int[0];
		/// <summary>
		/// Internal member storing the topics
		/// </summary>
		private ArrayList _Topics = null;
		/// <summary>
		/// Associated CHMFile instance
		/// </summary>
		private CHMFile _chmFile = null;
		/// <summary>
		/// Internal flag specifying the chm file path
		/// </summary>
		private string _chmFileName = "";

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="chmFile">associated CHMFile instance</param>
		/// <param name="keyWord">keyword</param>
		/// <param name="isSeeAlso">true if it is a see-also keyword</param>
		/// <param name="indent">indent of the entry</param>
		/// <param name="charIndex">char index of the last keyword in the separated list</param>
		/// <param name="entryIndex">index of the entry</param>
		/// <param name="seeAlsoValues">string array with see-also values</param>
		/// <param name="topicOffsets">integer array with topic offsets</param>
		internal IndexItem(CHMFile chmFile, string keyWord, bool isSeeAlso, int indent, int charIndex, int entryIndex, string[] seeAlsoValues, int[] topicOffsets)
		{
			_chmFile = chmFile;
			_chmFileName = _chmFile.ChmFilePath;
			_keyWord = keyWord;
			_isSeeAlso = isSeeAlso;
			_indent = indent;
			_charIndex = charIndex;
			_entryIndex = entryIndex;
			_seeAlso = seeAlsoValues;
			_nTopics = topicOffsets;
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		public IndexItem()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		/// <param name="writeFileName">true if the chm filename should be written</param>
		internal void Dump(ref BinaryWriter writer, bool writeFileName)
		{
			int i=0;

			writer.Write(_keyWord);
			writer.Write(_isSeeAlso);
			writer.Write(_indent);

			if(writeFileName)
				writer.Write(_chmFileName);

			writer.Write(_infoTypeStrings.Count);

			for(i=0; i<_infoTypeStrings.Count; i++)
				writer.Write( (_infoTypeStrings[i]).ToString() );

			writer.Write(_seeAlso.Length);

			for(i=0; i<_seeAlso.Length; i++)
			{
				if(_seeAlso[i] == null)
					writer.Write("");
				else
					writer.Write( _seeAlso[i] );
			}

			writer.Write(Topics.Count);

			for(i=0; i<Topics.Count; i++)
			{
				IndexTopic topic = ((IndexTopic)(Topics[i]));
				topic.Dump(ref writer);
			}
		}

		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			Dump(ref writer, false);
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		/// <param name="filesList">filelist from helpsystem</param>
		internal bool ReadDump(ref BinaryReader reader, ArrayList filesList)
		{
			int i=0;
			_keyWord = reader.ReadString();
			_isSeeAlso = reader.ReadBoolean();
			_indent = reader.ReadInt32();
			_chmFileName = reader.ReadString();

			foreach(CHMFile curFile in filesList)
			{
				if(curFile.ChmFilePath == _chmFileName)
				{
					_chmFile = curFile;
					break;
				}
			}

			if(_chmFile==null)
				return false;

			int nCnt = reader.ReadInt32();

			for(i=0; i<nCnt; i++)
			{
				string sIT = reader.ReadString();
				_infoTypeStrings.Add(sIT);
			}

			nCnt = reader.ReadInt32();

			_seeAlso = new string[nCnt];

			for(i=0; i<nCnt; i++)
			{
				_seeAlso[i] = reader.ReadString();
			}

			nCnt = reader.ReadInt32();

			for(i=0; i<nCnt; i++)
			{
				IndexTopic topic = new IndexTopic("","","","");
				topic.SetChmInfo( _chmFile.CompileFile, _chmFile.ChmFilePath);
				topic.AssociatedFile = _chmFile;
				topic.ReadDump(ref reader);
				
				Topics.Add(topic);
			}

			return true;
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			int i=0;
			_keyWord = reader.ReadString();
			_isSeeAlso = reader.ReadBoolean();
			_indent = reader.ReadInt32();

			int nCnt = reader.ReadInt32();

			for(i=0; i<nCnt; i++)
			{
				string sIT = reader.ReadString();
				_infoTypeStrings.Add(sIT);
			}

			nCnt = reader.ReadInt32();

			_seeAlso = new string[nCnt];

			for(i=0; i<nCnt; i++)
			{
				_seeAlso[i] = reader.ReadString();
			}

			nCnt = reader.ReadInt32();

			for(i=0; i<nCnt; i++)
			{
				IndexTopic topic = new IndexTopic("","","","");
				topic.AssociatedFile = _chmFile;
				topic.SetChmInfo( _chmFile.CompileFile, _chmFile.ChmFilePath);
				topic.ReadDump(ref reader);
				Topics.Add(topic);
			}
		}
		#endregion

		/// <summary>
		/// Implements the compareto method which allows sorting.
		/// </summary>
		/// <param name="obj">object to compare to</param>
		/// <returns>See <see cref="System.IComparable">IComparable.CompareTo()</see></returns>
		public int CompareTo(object obj)
		{
			if( obj.GetType() == this.GetType() )
			{
				IndexItem cmp = (IndexItem)obj;

				return this.KeyWordPath.CompareTo( cmp.KeyWordPath );
			}

			return 0;
		}

		/// <summary>
		/// Gets/Sets the associated CHMFile instance
		/// </summary>
		internal CHMFile ChmFile
		{
			get { return _chmFile; }
			set { _chmFile = value; }
		}

		/// <summary>
		/// Gets the ArrayList which holds all information types/categories this item is associated
		/// </summary>
		internal ArrayList InfoTypeStrings
		{
			get { return _infoTypeStrings; }
		}

		/// <summary>
		/// Adds a see-also string to the index item and marks it as see also item
		/// </summary>
		/// <param name="seeAlsoString">see also string to add</param>
		internal void AddSeeAlso(string seeAlsoString)
		{
			string[] seeAlso = new string[ _seeAlso.Length +1 ];
			for(int i=0; i<_seeAlso.Length; i++)
				seeAlso[i] = _seeAlso[i];

			seeAlso[_seeAlso.Length] = seeAlsoString;
			_seeAlso = seeAlso;
			_isSeeAlso = true;
		}

		/// <summary>
		/// Gets/Sets the full keyword-path of this item ( ", " separated list)
		/// </summary>
		public string KeyWordPath
		{
			get { return _keyWord; }
			set { _keyWord = value; }
		}

		/// <summary>
		/// Gets the keyword of this item
		/// </summary>
		public string KeyWord
		{
			get 
			{ 
				return _keyWord.Substring(_charIndex, _keyWord.Length-_charIndex); 
			}
		}

		/// <summary>
		/// Gets the keyword of this item with prefixing indent spaces
		/// </summary>
		public string IndentKeyWord
		{
			get
			{
				string sKW = this.KeyWord;
				StringBuilder sb = new StringBuilder("",this.Indent*3 + sKW.Length);
				for(int i=0; i<this.Indent; i++)
					sb.Append("   ");
				sb.Append(sKW);
				return sb.ToString();
			}
		}

		/// <summary>
		/// Gets/Sets the see-also flag of this item
		/// </summary>
		public bool IsSeeAlso
		{
			get { return _isSeeAlso; }
			set { _isSeeAlso = value; }
		}

		/// <summary>
		/// Gets/Sets the listbox indent for this item
		/// </summary>
		public int Indent
		{
			get { return _indent; }
			set { _indent = value; }
		}

		/// <summary>
		/// Gets/Sets the character index of an indent keyword
		/// </summary>
		public int CharIndex
		{
			get { return _charIndex; }
			set { _charIndex = value; }
		}

		/// <summary>
		/// Gets the see-also values of this item
		/// </summary>
		public string[] SeeAlso
		{
			get { return _seeAlso; }
		}

		/// <summary>
		/// Gets an array with the associated topics
		/// </summary>
		public ArrayList Topics
		{
			get 
			{
				if( _Topics == null )
				{
					if(IsSeeAlso)
					{
						_Topics = new ArrayList();
					} 
					else 
					{
						if( (_chmFile != null) && (_chmFile.TopicsFile != null) )
						{
							_Topics = new ArrayList();

							for(int i=0; i<_nTopics.Length; i++)
							{
								IndexTopic newTopic = IndexTopic.FromTopicEntry((TopicEntry)_chmFile.TopicsFile.TopicTable[ _nTopics[i] ]);
								newTopic.AssociatedFile = _chmFile;
								_Topics.Add( newTopic );
							}
						} 
						else 
						{
							_Topics = new ArrayList();
						}
					}
				}

				return _Topics;
			}
		}
	}
}
