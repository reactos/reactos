using System;
using System.IO;
using System.Collections;
using System.Windows.Forms;

using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>TOCItem</c> implements a toc-entry item
	/// </summary>
	public sealed class TOCItem
	{
		/// <summary>
		/// Constant for standard folder (closed) image index (HH2 image list)
		/// </summary>
		public const int STD_FOLDER_HH2 = 4;
		/// <summary>
		/// Constant for standard folder (opened) image index (HH2 image list)
		/// </summary>
		public const int STD_FOLDER_OPEN_HH2 = 6;
		/// <summary>
		/// Constant for standard file image index (HH2 image list)
		/// </summary>
		public const int STD_FILE_HH2 = 16;

		/// <summary>
		/// Constant for standard folder (closed) image index (HH1 image list)
		/// </summary>
		public const int STD_FOLDER_HH1 = 0;
		/// <summary>
		/// Constant for standard folder (opened) image index (HH1 image list)
		/// </summary>
		public const int STD_FOLDER_OPEN_HH1 = 1;
		/// <summary>
		/// Constant for standard file image index (HH1 image list)
		/// </summary>
		public const int STD_FILE_HH1 = 10;

		/// <summary>
		/// Internal flag specifying the data extraction mode used for this item
		/// </summary>
		private DataMode _tocMode = DataMode.TextBased;
		/// <summary>
		/// Internal member storing the offset (only used in binary tocs)
		/// </summary>
		private int _offset = 0;
		/// <summary>
		/// Internal member storing the offset of the next item(only used in binary tocs)
		/// </summary>
		private int _offsetNext = 0;
		/// <summary>
		/// Internal member storing a merge link. 
		/// If the target file is in the merged files list of the CHM,
		/// this item will be replaced with the target TOC or Topic, if not it will 
		/// be removed from TOC.
		/// </summary>
		private string _mergeLink = "";
		/// <summary>
		/// Internal member storing the toc name
		/// </summary>
		private string _name = "";
		/// <summary>
		/// Internal member storing the toc loca (content file)
		/// </summary>
		private string _local = "";
		/// <summary>
		/// Internal member storing all associated information type strings
		/// </summary>
		private ArrayList _infoTypeStrings = new ArrayList();
		/// <summary>
		/// Internal member storing the associated chm file
		/// </summary>
		private string _chmFile = "";
		/// <summary>
		/// Internal member storing the image index
		/// </summary>
		private int _imageIndex = -1;
		/// <summary>
		/// Internal member storing the offset of the associated topic entry (for binary tocs)
		/// </summary>
		private int _topicOffset = -1;
		/// <summary>
		/// Internal member storing the toc children
		/// </summary>
		private ArrayList _children = new ArrayList();
		/// <summary>
		/// Internal member storing the parameter collection
		/// </summary>
		private Hashtable _otherParams = new Hashtable();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;
		/// <summary>
		/// Parent item
		/// </summary>
		private TOCItem _parent=null;

		/// <summary>
		/// Holds a pointer to the next item in the TOC
		/// </summary>
		public TOCItem Next=null;

		/// <summary>
		/// Holds a pointer to the previous item in the TOC
		/// </summary>
		public TOCItem Prev=null;

		/// <summary>
		/// Holds a pointer to the TreeNode where this TOC Item is used
		/// </summary>
		public System.Windows.Forms.TreeNode treeNode=null;

		/// <summary>
		/// Constructor of the class used during text-based data extraction
		/// </summary>
		/// <param name="name">name of the item</param>
		/// <param name="local">local content file</param>
		/// <param name="ImageIndex">image index</param>
		/// <param name="chmFile">associated chm file</param>
		public TOCItem(string name, string local, int ImageIndex, string chmFile)
		{
			_tocMode = DataMode.TextBased;
			_name = name;
			_local = local;
			_imageIndex = ImageIndex;
			_chmFile = chmFile;
		}

		/// <summary>
		/// Constructor of the class used during binary data extraction
		/// </summary>
		/// <param name="topicOffset">offset of the associated topic entry</param>
		/// <param name="ImageIndex">image index to use</param>
		/// <param name="associatedFile">associated chm file</param>
		public TOCItem(int topicOffset, int ImageIndex, CHMFile associatedFile)
		{
			_tocMode = DataMode.Binary;
			_associatedFile = associatedFile;
			_chmFile = associatedFile.ChmFilePath;
			_topicOffset = topicOffset;
			_imageIndex = ImageIndex;
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		public TOCItem()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		/// <param name="writeFilename">true if the chmfile name should be written</param>
		internal void Dump(ref BinaryWriter writer, bool writeFilename)
		{
			writer.Write((int)_tocMode);
			writer.Write(_topicOffset);
			writer.Write(_name);

			if((_tocMode == DataMode.TextBased)||(_topicOffset<0))
			{
				writer.Write(_local);
			}

			writer.Write(_imageIndex);

			writer.Write(_mergeLink);

			if(writeFilename)
				writer.Write(_chmFile);

			writer.Write(_infoTypeStrings.Count);

			for(int i=0; i<_infoTypeStrings.Count; i++)
				writer.Write( (_infoTypeStrings[i]).ToString() );

			writer.Write(_children.Count);

			for(int i=0; i<_children.Count; i++)
			{
				TOCItem child = ((TOCItem)(_children[i]));
				child.Dump(ref writer, writeFilename);
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
		/// <param name="readFilename">true if the chmfile name should be read</param>
		internal void ReadDump(ref BinaryReader reader, bool readFilename)
		{
			int i=0;
			_tocMode = (DataMode)reader.ReadInt32();
			_topicOffset = reader.ReadInt32();
			_name = reader.ReadString();

			if((_tocMode == DataMode.TextBased)||(_topicOffset<0))
			{
				_local = reader.ReadString();
			}
			
			_imageIndex = reader.ReadInt32();

			_mergeLink = reader.ReadString();

			if(readFilename)
				_chmFile = reader.ReadString();

			int nCnt = reader.ReadInt32();

			for(i=0; i<nCnt; i++)
			{
				string sIT = reader.ReadString();
				_infoTypeStrings.Add(sIT);
			}

			nCnt = reader.ReadInt32();

			if(_associatedFile != null)
				_chmFile = _associatedFile.ChmFilePath;

			for(i=0; i<nCnt; i++)
			{
				TOCItem child = new TOCItem();
				child.AssociatedFile = _associatedFile;

				child.ReadDump(ref reader, readFilename);

				if(_associatedFile != null)
					child.ChmFile = _associatedFile.ChmFilePath;
				else if(!readFilename)
					child.ChmFile = _chmFile;
				child.Parent = this;

				_children.Add(child);

				if(child.MergeLink.Length > 0)
					_associatedFile.MergLinks.Add(child);
			}
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			ReadDump(ref reader, false);
		}
		#endregion

		/// <summary>
		/// Gets/Sets the data extraction mode with which this item was created.
		/// </summary>
		internal DataMode TocMode
		{
			get { return _tocMode; }
			set { _tocMode = value; }
		}

		/// <summary>
		/// Gets/Sets the offset of the associated topic entry
		/// </summary>
		internal int TopicOffset
		{
			get { return _topicOffset; }
			set { _topicOffset = value; }
		}

		/// <summary>
		/// Gets/Sets the associated CHMFile instance
		/// </summary>
		internal CHMFile AssociatedFile
		{
			get { return _associatedFile; }
			set 
			{ 
				_associatedFile = value; 
			}
		}

		/// <summary>
		/// Gets/Sets the offset of the item.
		/// </summary>
		/// <remarks>Only used in binary tocs</remarks>
		internal int Offset
		{
			get	{ return _offset; }
			set	{ _offset = value; }
		}

		/// <summary>
		/// Gets/Sets the offset of the next item.
		/// </summary>
		/// <remarks>Only used in binary tocs</remarks>
		internal int OffsetNext
		{
			get	{ return _offsetNext; }
			set	{ _offsetNext = value; }
		}

		/// <summary>
		/// Gets the ArrayList which holds all information types/categories this item is associated
		/// </summary>
		internal ArrayList InfoTypeStrings
		{
			get { return _infoTypeStrings; }
		}

		/// <summary>
		/// Gets/Sets the parent of this item
		/// </summary>
		public TOCItem Parent
		{
			get { return _parent; }
			set { _parent = value; }
		}

		/// <summary>
		/// Gets/Sets the mergelink for this item. 
		/// <b>You should not set the mergedlink by your own !</b>
		/// This is only for loading merged CHMs.
		/// </summary>
		public string MergeLink
		{
			get { return _mergeLink; }
			set { _mergeLink = value; }
		}

		/// <summary>
		/// Gets/Sets the name of the item
		/// </summary>
		public string Name
		{
			get	
			{ 
				if(_mergeLink.Length > 0)
					return "";

				if(_name.Length <= 0)
				{
					if((_tocMode == DataMode.Binary )&&(_associatedFile!=null))
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
				}

				return _name; 
			}
			set	
			{ 
				_name = value; 
			}
		}

		/// <summary>
		/// Gets/Sets the local of the item
		/// </summary>
		public string Local
		{
			get	
			{ 
				if(_mergeLink.Length > 0)
					return "";

				if(_local.Length <= 0)
				{
					if((_tocMode == DataMode.Binary )&&(_associatedFile!=null))
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
				}

				return _local; 
			}
			set	{ _local = value; }
		}

		/// <summary>
		/// Gets/Sets the chm file
		/// </summary>
		public string ChmFile
		{
			get	
			{ 
				if(_associatedFile!=null)
					return _associatedFile.ChmFilePath;

				return _chmFile; 
			}
			set	{ _chmFile = value; }
		}

		/// <summary>
		/// Gets the url for the webbrowser for this file
		/// </summary>
		public string Url
		{
			get
			{
				string sL = Local;

				if( (sL.ToLower().IndexOf("http://") >= 0) ||
					(sL.ToLower().IndexOf("https://") >= 0) ||
					(sL.ToLower().IndexOf("mailto:") >= 0) ||
					(sL.ToLower().IndexOf("ftp://") >= 0) || 
					(sL.ToLower().IndexOf("ms-its:") >= 0))
					return sL;

				return HtmlHelpSystem.UrlPrefix + ChmFile + "::/" + sL;
			}
		}

		/// <summary>
		/// Gets/Sets the image index of the item
		/// </summary>
		/// <remarks>Set this to -1 for a default icon</remarks>
		public int ImageIndex
		{
			get	
			{ 
				if( _imageIndex == -1)
				{
					int nFolderAdd = 0;

					if((_associatedFile != null) && (_associatedFile.ImageTypeFolder))
					{
						// get the value which should be added, to display folders instead of books
						if(HtmlHelpSystem.UseHH2TreePics) 
							nFolderAdd = 8;
						else
							nFolderAdd = 4;
					}
					

					if( _children.Count > 0)
						return (HtmlHelpSystem.UseHH2TreePics ? (STD_FOLDER_HH2+nFolderAdd) : (STD_FOLDER_HH1+nFolderAdd));

					return (HtmlHelpSystem.UseHH2TreePics ? STD_FILE_HH2 : STD_FILE_HH1);
				}
				return _imageIndex; 
			}
			set	{ _imageIndex = value; }
		}

		/// <summary>
		/// Gets/Sets the children of this item.
		/// </summary>
		/// <remarks>Each entry in the ArrayList is of type TOCItem</remarks>
		public ArrayList Children
		{
			get { return _children; }
			set { _children = value; }
		}

		/// <summary>
		/// Gets the internal hashtable storing all params
		/// </summary>
		public Hashtable Params
		{
			get { return _otherParams; }
		}
	}
}
