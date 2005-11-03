using System;
using System.Diagnostics;
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Runtime.InteropServices;
using System.Globalization;
// using HtmlHelp.Storage;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// Internal enumeration for specifying the type of the html-help file
	/// </summary>
	internal enum HtmlHelpFileType
	{
		/// <summary>
		/// CHM - compiled contents file
		/// </summary>
		/// <remarks>A file with this extension must always exist. If the file would be too long, some parts 
		/// can be splitted into the filestypes below.</remarks>
		CHM = 0,
		/// <summary>
		/// CHI - compiled system file
		/// </summary>
		CHI = 1,
		/// <summary>
		/// CHQ - compiled fulltext search file
		/// </summary>
		CHQ = 2,
		/// <summary>
		/// CHW - compiled index file
		/// </summary>
		CHW = 3

	}

	/// <summary>
	/// The class <c>CHMFile</c> implemts methods and properties for handling a single chmfile.
	/// </summary>
	public sealed class CHMFile : IDisposable
	{
		/// <summary>
		/// Internal member storing a reference to the hosting HtmlHelpSystem instance
		/// </summary>
		private HtmlHelpSystem _systemInstance = null;
		/// <summary>
		/// Internal flag specifying if only system data has been loaded
		/// </summary>
		private bool _onlySystem = false;
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal arraylist containing the table of contents
		/// </summary>
		private ArrayList _toc = new ArrayList();
		/// <summary>
		/// Internal arraylist containing items of the toc which are merge-Links
		/// </summary>
		private ArrayList _mergeLinks = new ArrayList();
		/// <summary>
		/// Internal member storing the read information types
		/// </summary>
		private ArrayList _informationTypes = new ArrayList();
		/// <summary>
		/// Internal member storing the read categories
		/// </summary>
		private ArrayList _categories = new ArrayList();
		/// <summary>
		/// Internal arraylist containing the index (klinks)
		/// </summary>
		private ArrayList _indexKLinks = new ArrayList();
		/// <summary>
		/// Internal arraylist containing the index (alinks)
		/// </summary>
		private ArrayList _indexALinks = new ArrayList();
		/// <summary>
		/// Internal member storing the full filename
		/// </summary>
		private string _chmFileName = "";
		/// <summary>
		/// Internal member storing the full filename of the chi-file (includes all system files)
		/// The file name is zero-length if there is no chi-file
		/// </summary>
		private string _chiFileName = "";
		/// <summary>
		/// Internal member storing the full filename of the chw-file (includes the help index)
		/// The file name is zero-length if there is no chw-file
		/// </summary>
		private string _chwFileName = "";
		/// <summary>
		/// Internal member storing the full filename of the chq-file (includes the fulltext contents)
		/// The file name is zero-length if there is no chq-file
		/// </summary>
		private string _chqFileName = "";
		/// <summary>
		/// Internal member storing the decoded information from the internal #SYSTEM file
		/// </summary>
		private CHMSystem _systemFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #IDXHDR file
		/// </summary>
		private CHMIdxhdr _idxhdrFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #STRINGS file
		/// </summary>
		private CHMStrings _stringsFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #URLSTR file
		/// </summary>
		private CHMUrlstr _urlstrFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #URLTBL file
		/// </summary>
		private CHMUrltable _urltblFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #TOPICS file
		/// </summary>
		private CHMTopics _topicsFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal #TOCIDX file
		/// </summary>
		private CHMTocidx _tocidxFile = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal binary index file (KLinks).
		/// </summary>
		private CHMBtree _kLinks = null;
		/// <summary>
		/// Internal member storing the decoded information from the internal binary index file (ALinks).
		/// </summary>
		private CHMBtree _aLinks = null;
		/// <summary>
		/// Internal member storing the fulltext searcher for this file
		/// </summary>
		private FullTextEngine _ftSearcher = null;
		/// <summary>
		/// Internal member storing the default encoder
		/// </summary>
		private Encoding _textEncoding = Encoding.GetEncoding(1252); // standard windows-1252 encoder
		/// <summary>
		/// Internal memebr storing the chm file info
		/// </summary>
		private ChmFileInfo _chmFileInfo = null;
		/// <summary>
		/// Internal flag specifying if the dump must be written (if enabled)
		/// </summary>
		private bool _mustWriteDump = false;
		/// <summary>
		/// Internal flag specifying if data was read using the dump
		/// </summary>
		private bool _dumpRead = false;
		/// <summary>
		/// Internal member for specifying the number of dump-reading trys.
		/// If dump-reading fails, this is used that it will not be opened a second time
		/// (in CHM-Systems with CHM, CHI, etc. files)
		/// </summary>
		private int _dumpReadTrys = 0;
		/// <summary>
		/// Internal member storing the dumping info instance
		/// </summary>
		private DumpingInfo _dmpInfo = null;

		private CHMStream.CHMStream _currentWrapper;
		private CHMStream.CHMStream _baseStream=null;
		public CHMStream.CHMStream BaseStream
		{
			get 
			{ 
				if (_baseStream==null)
					_baseStream=new CHMStream.CHMStream(this.ChmFilePath);
				return _baseStream; 
			}
		}

		/// <summary>
		/// Creates a new instance of the class
		/// </summary>
		/// <param name="systemInstance">a reference to the hosting HtmlHelpSystem instance</param>
		/// <param name="chmFile">chm file to read</param>
		public CHMFile(HtmlHelpSystem systemInstance, string chmFile) : this(systemInstance, chmFile, false, null)
		{
		}

		/// <summary>
		/// Creates a new instance of the class
		/// </summary>
		/// <param name="systemInstance">a reference to the hosting HtmlHelpSystem instance</param>
		/// <param name="chmFile">chm file to read</param>
		/// <param name="dmpInfo">A dumping info class</param>
		public CHMFile(HtmlHelpSystem systemInstance, string chmFile, DumpingInfo dmpInfo) : this(systemInstance, chmFile, false, dmpInfo)
		{
		}

		/// <summary>
		/// Creates a new instance of the class
		/// </summary>
		/// <param name="systemInstance">a reference to the hosting HtmlHelpSystem instance</param>
		/// <param name="chmFile">chm file to read</param>
		/// <param name="onlySystemData">true if only system data should be extracted (no index or toc)</param>
		internal CHMFile(HtmlHelpSystem systemInstance, string chmFile, bool onlySystemData) : this(systemInstance, chmFile, onlySystemData, null)
		{
		}

		/// <summary>
		/// Creates a new instance of the class
		/// </summary>
		/// <param name="systemInstance">a reference to the hosting HtmlHelpSystem instance</param>
		/// <param name="chmFile">chm file to read</param>
		/// <param name="onlySystemData">true if only system data should be extracted (no index or toc)</param>
		/// <param name="dmpInfo">A dumping info class</param>
		internal CHMFile(HtmlHelpSystem systemInstance, string chmFile, bool onlySystemData, DumpingInfo dmpInfo)
		{
			_systemInstance = systemInstance;
			_dumpReadTrys=0;

			_dmpInfo = dmpInfo;
			if(dmpInfo != null)
			{
				dmpInfo.ChmFile = this;
			}

			if( ! chmFile.ToLower().EndsWith(".chm") )
			{
				throw new ArgumentException("HtmlHelp file must have the extension .chm !", "chmFile");
			}

			_chmFileName = chmFile;
			_chiFileName = "";

			// Read the IStorage file system
			if( File.Exists(chmFile) )
			{
				_onlySystem = onlySystemData;
				
				DateTime dtStartHH = DateTime.Now;

				string sCHIName = _chmFileName.Substring(0, _chmFileName.Length-3) + "chi";
				string sCHQName = _chmFileName.Substring(0, _chmFileName.Length-3) + "chq";
				string sCHWName = _chmFileName.Substring(0, _chmFileName.Length-3) + "chw";

				// If there is a CHI file present (this file includes the internal system files for the current chm file)
				if( File.Exists(sCHIName) )
				{
					_chiFileName = sCHIName;
				
					ReadFile(_chiFileName, HtmlHelpFileType.CHI);
				}

				// If there is a CHW file present (this file includes the internal binary index of the help)
				if(( File.Exists(sCHWName) ) && (!_onlySystem) )
				{
					_chwFileName = sCHWName;
				
					ReadFile(_chwFileName, HtmlHelpFileType.CHW);
				}

				// If there is a CHQ file present (this file includes the fulltext-search data)
				if(( File.Exists(sCHQName) ) && (!_onlySystem) )
				{
					_chqFileName = sCHQName;
				
					ReadFile(_chqFileName, HtmlHelpFileType.CHQ);
				}

				ReadFile(chmFile, HtmlHelpFileType.CHM);

				if(_mustWriteDump)
				{					
					_mustWriteDump = !SaveDump(dmpInfo);

				}

				// check the default-topic setting
				if(_systemFile.DefaultTopic.Length > 0)
				{
					CHMStream.CHMStream iw=null;
					iw = new CHMStream.CHMStream(chmFile);
					_currentWrapper=iw;

					// tryo to open the topic file
					MemoryStream fileObject = iw.OpenStream( _systemFile.DefaultTopic);
					if( fileObject != null)
					{
						// if succeed, the topic default topic is OK
						fileObject.Close();
					} 
					else 
					{
						// set the first topic of the toc-tree as default topic
						if(_toc.Count > 0)
						{
							_systemFile.SetDefaultTopic( ((TOCItem) _toc[0]).Local );
						}
					}
					_currentWrapper=null;
				} 
				else 
				{
					// set the first topic of the toc-tree as default topic
					if(_toc.Count > 0)
					{
						_systemFile.SetDefaultTopic( ((TOCItem) _toc[0]).Local );
					}
				}

				_chmFileInfo = new ChmFileInfo(this);
			} 
			else 
			{
				throw new ArgumentException("File '" + chmFile + "' not found !", "chmFile");
			}
		}

		/// <summary>
		/// Read a IStorage file
		/// </summary>
		/// <param name="fname">filename</param>
		/// <param name="type">type of file</param>
		private void ReadFile(string fname, HtmlHelpFileType type)
		{
			CHMStream.CHMStream iw=null;
			iw=new CHMStream.CHMStream();			
			iw.OpenCHM(fname);
			_currentWrapper=iw;
			MemoryStream fileObject=null;			

			// ITStorageWrapper iw = null;

			// Open the internal chm system files and parse their content
			// FileObject fileObject = null;
			// iw = new ITStorageWrapper(fname, false);			

			if( (type != HtmlHelpFileType.CHQ) && (type != HtmlHelpFileType.CHW) )
			{				
				fileObject = iw.OpenStream("#SYSTEM");
				if ((fileObject != null) && (fileObject.Length>0))
					_systemFile = new CHMSystem(fileObject.ToArray(), this);									

				fileObject = iw.OpenStream("#IDXHDR");
				if ((fileObject != null) && (fileObject.Length>0))
					_idxhdrFile = new CHMIdxhdr(fileObject.ToArray(), this);					
				
				// try to read Dump
				if((!_dumpRead)&&(CheckDump(_dmpInfo))&&(_dumpReadTrys==0))
				{
					_dumpReadTrys++;
					_dumpRead = LoadDump(_dmpInfo);					
				}

				if( (!_dumpRead)||(!_dmpInfo.DumpStrings) )
				{
					fileObject = iw.OpenStream( "#STRINGS");
					if ((fileObject != null) && (fileObject.Length>0))
						_stringsFile = new CHMStrings(fileObject.ToArray(), this);
				}

				if( (!_dumpRead)||(!_dmpInfo.DumpUrlStr) )
				{
					fileObject = iw.OpenStream( "#URLSTR");
					if ((fileObject != null) && (fileObject.Length>0))
						_urlstrFile = new CHMUrlstr(fileObject.ToArray(), this);
				}

				if( (!_dumpRead)||(!_dmpInfo.DumpUrlTbl) )
				{
					fileObject = iw.OpenStream( "#URLTBL");
					if ((fileObject != null) && (fileObject.Length>0))
						_urltblFile = new CHMUrltable(fileObject.ToArray(), this);
				}

				if( (!_dumpRead)||(!_dmpInfo.DumpTopics) )
				{
					fileObject = iw.OpenStream( "#TOPICS");
					if ((fileObject != null) && (fileObject.Length>0))
						_topicsFile = new CHMTopics(fileObject.ToArray(), this);
				}

				if(!_onlySystem)
				{
					if( (!_dumpRead)||(!_dmpInfo.DumpBinaryTOC) )
					{
						fileObject = iw.OpenStream( "#TOCIDX");
						if( (fileObject != null) && (fileObject.Length>0) && (_systemFile.BinaryTOC) )
						{
							_tocidxFile = new CHMTocidx(fileObject.ToArray(), this);
							_toc = _tocidxFile.TOC;							
						}
					}
				}

				if( (_systemFile != null) && (!_onlySystem) )
				{
					if(!_systemFile.BinaryTOC)
					{
						if( (!_dumpRead)||(!_dmpInfo.DumpTextTOC) )
						{
							CHMStream.chmUnitInfo HHCInfo=iw.GetFileInfoByExtension(".hhc");
							if (HHCInfo!=null)
							{
								fileObject = iw.OpenStream(HHCInfo);							
								if ((fileObject != null) && (fileObject.Length>0))
								{
									ASCIIEncoding ascii=new ASCIIEncoding();			
									string fileString =ascii.GetString(fileObject.ToArray(),0,(int)fileObject.Length);

									_toc = HHCParser.ParseHHC(fileString, this);
								}

								if(HHCParser.HasMergeLinks)
									_mergeLinks = HHCParser.MergeItems;
							}							
						}
					} 
				}
			}

			if( type != HtmlHelpFileType.CHQ ) // no index information in CHQ files (only fulltext search)
			{
				if( (_systemFile != null) && (!_onlySystem) )
				{
					if( ! _systemFile.BinaryIndex )
					{
						if( (!_dumpRead)||(!_dmpInfo.DumpTextIndex) )
						{
							fileObject = iw.OpenStream( _systemFile.IndexFile);
							if ((fileObject != null) && (fileObject.Length>0))
							{

								string fileString = this.TextEncoding.GetString(fileObject.ToArray(),0,(int)fileObject.Length);
								// string fileString = fileObject.ReadFromFile(this.TextEncoding);
								fileObject.Close();

								_indexKLinks = HHKParser.ParseHHK(fileString, this);
								_indexKLinks.Sort();
							}
						}
					} 
					else 
					{
						if( (!_dumpRead)||(!_dmpInfo.DumpBinaryIndex) )
						{
							fileObject=iw.OpenStream(@"$WWKeywordLinks\BTree");
							if ((fileObject != null) && (fileObject.Length>0))
							{
								_kLinks = new CHMBtree(fileObject.ToArray(), this);
								_indexKLinks = _kLinks.IndexList;
							}

							fileObject =iw.OpenStream(@"$WWAssociativeLinks\BTree");
							if ((fileObject != null) && (fileObject.Length>0))
							{									
								_aLinks = new CHMBtree(fileObject.ToArray(), this);
								_indexALinks = _aLinks.IndexList;
								_indexALinks.Sort();									
							}							
						}
					}
				}
			}

			if( (type != HtmlHelpFileType.CHI) && (type != HtmlHelpFileType.CHW) )
			{
				if( (_systemFile != null) && (!_onlySystem) )
				{
					if( _systemFile.FullTextSearch)
					{
						if( (!_dumpRead)||(!_dmpInfo.DumpFullText) )
						{
							fileObject = iw.OpenStream("$FIftiMain");
							if(( fileObject != null) && (fileObject .Length>0))
								_ftSearcher = new FullTextEngine(fileObject .ToArray(), this);							
						}
					}
				}
			}
			_currentWrapper=null;
		}

		/// <summary>
		/// Enumerates the files in the chm storage and gets all files matching a given extension.
		/// </summary>
		/// <param name="extension">extension to return</param>
		/// <returns>Returns an arraylist of filenames or null if nothing found</returns>
		/// <remarks>On large CHMs, enumerations are very slow. Only use it if necessary !</remarks>
		internal ArrayList EnumFilesByExtension(string extension)
		{
			ArrayList arrRet = new ArrayList();

			CHMStream.CHMStream iw = null;
			if(_currentWrapper == null)
				iw = new CHMStream.CHMStream(_chmFileName);
			else
				iw = _currentWrapper;

			arrRet=iw.GetFileListByExtenstion(extension);
			
			if(arrRet.Count > 0)
				return arrRet;

			return null;
		}

		/// <summary>
		/// Searches an TOC entry using the local
		/// </summary>
		/// <param name="local">local to search</param>
		/// <returns>Returns the TOC item</returns>
		internal TOCItem GetTOCItemByLocal(string local)
		{
			return GetTOCItemByLocal(this.TOC, local);
		}

		/// <summary>
		/// Recursively searches an TOC entry using its local
		/// </summary>
		/// <param name="arrTOC">toc level list</param>
		/// <param name="local">local to search</param>
		/// <returns>Returns the TOC item</returns>
		private TOCItem GetTOCItemByLocal(ArrayList arrTOC, string local)
		{
			TOCItem ret = null;
			foreach(TOCItem curItem in arrTOC)
			{
				string scL = curItem.Local.ToLower();
				string sL = local.ToLower();

				while(scL[0]=='/') // delete prefixing '/'
					scL = scL.Substring(1);

				while(sL[0]=='/') // delete prefixing '/'
					sL = sL.Substring(1);

				if(scL == sL)
					return curItem;

				if(curItem.Children.Count > 0)
				{
					ret =  GetTOCItemByLocal(curItem.Children, local);
					if(ret != null)
						return ret;
				}
			}

			return ret;
		}

		/// <summary>
		/// Removes a TOCItem from the toc
		/// </summary>
		/// <param name="rem">item to remove</param>
		/// <returns>Returns true if removed</returns>
		internal bool RemoveTOCItem(TOCItem rem)
		{
			if(rem == null)
				return false;

			return RemoveTOCItem(this.TOC, rem);
		}

		/// <summary>
		/// Recursively searches a TOCItem and removes it if found
		/// </summary>
		/// <param name="arrTOC">toc level list</param>
		/// <param name="rem">item to remove</param>
		/// <returns>Returns true if removed</returns>
		private bool RemoveTOCItem(ArrayList arrTOC, TOCItem rem)
		{
			for(int i=0; i<arrTOC.Count;i++)
			{
				TOCItem curItem = arrTOC[i] as TOCItem;

				if(curItem == rem)
				{
					arrTOC.RemoveAt(i);
					return true;
				}

				if(curItem.Children.Count > 0)
				{
					bool bRem = RemoveTOCItem(curItem.Children, rem);
					if(bRem)
						return true;
				}
			}

			return false;
		}

		/// <summary>
		/// Returns true if the HtmlHelpSystem instance contains 1 or more information types
		/// </summary>
		public bool HasInformationTypes
		{
			get { return (_informationTypes.Count>0); }
		}

		/// <summary>
		/// Returns true if the HtmlHelpSystem instance contains 1 or more categories
		/// </summary>
		public bool HasCategories
		{
			get { return (_categories.Count>0); }
		}

		/// <summary>
		/// Gets an ArrayList of <see cref="InformationType">InformationType</see> items
		/// </summary>
		public ArrayList InformationTypes
		{
			get { return _informationTypes; }
		}

		/// <summary>
		/// Gets an ArrayList of <see cref="Category">Category</see> items
		/// </summary>
		public ArrayList Categories
		{
			get { return _categories; }
		}

		/// <summary>
		/// Gets the information type specified by its name
		/// </summary>
		/// <param name="name">name of the information type to receive</param>
		/// <returns>Returns the Instance for the name or null if not found</returns>
		public InformationType GetInformationType(string name)
		{
			if(HasInformationTypes)
			{
				for(int i=0; i<_informationTypes.Count;i++)
				{
					InformationType iT = _informationTypes[i] as InformationType;

					if(iT.Name == name)
						return iT;
				}
			}

			return null;
		}

		/// <summary>
		/// Gets the category specifiyd by its name
		/// </summary>
		/// <param name="name">name of the category</param>
		/// <returns>Returns the Instance for the name or null if not found</returns>
		public Category GetCategory(string name)
		{
			if(HasCategories)
			{
				for(int i=0; i<_categories.Count;i++)
				{
					Category cat = _categories[i] as Category;

					if(cat.Name == name)
						return cat;
				}
			}

			return null;
		}

		#region Dumping methods
		
		/// <summary>
		/// Checks if a dump for this file exists and if it can be read
		/// </summary>
		/// <param name="dmpInfo">dumping info class</param>
		/// <returns>true if it can be read</returns>
		private bool CheckDump(DumpingInfo dmpInfo)
		{
			if(_dumpReadTrys<=0)
				_mustWriteDump = false;

			if(_onlySystem)
				return false;

			if( dmpInfo != null )
			{
				if(_dumpReadTrys > 0)
					return _mustWriteDump;

				_mustWriteDump = !dmpInfo.DumpExists;
				return !_mustWriteDump;
			}

			return false;
		}

		/// <summary>
		/// Saves the the toc and index into a data dump
		/// </summary>
		/// <param name="dmpInfo">dumping info</param>
		/// <returns>true if succeed</returns>
		private bool SaveDump(DumpingInfo dmpInfo)
		{
			if(dmpInfo == null)
				return false;

			bool bRet = false;


			BinaryWriter writer = dmpInfo.Writer;

			int nCnt = 0;
			try
			{
				Debug.WriteLine("writing dump-file header");
				FileInfo fi = new FileInfo(_chmFileName);
				string ftime = fi.LastWriteTime.ToString("dd.MM.yyyy HH:mm:ss.ffffff");

				writer.Write("HtmlHelpSystem dump file 1.0");
				writer.Write(ftime);
				writer.Write(_textEncoding.CodePage);

				// strings dumping
				if(dmpInfo.DumpStrings)
				{
					writer.Write(true); // data should be in dump

					if(_stringsFile==null)
					{
						writer.Write(false); // data not supported by the chm
					} 
					else 
					{
						Debug.WriteLine("writing #STRINGS");
						writer.Write(true); // data supported and following
						_stringsFile.Dump(ref writer);
					}
				} 
				else 
				{
					writer.Write(false); // data is not in dump
				}

				// urlstr dumping
				if(dmpInfo.DumpUrlStr)
				{
					writer.Write(true);

					if(_urlstrFile==null)
					{
						writer.Write(false);
					} 
					else 
					{
						Debug.WriteLine("writing #URLSTR");
						writer.Write(true);
						_urlstrFile.Dump(ref writer);
					}
				} 
				else 
				{
					writer.Write(false);
				}

				// urltbl dumping
				if(dmpInfo.DumpUrlTbl)
				{
					writer.Write(true);

					if(_urltblFile==null)
					{
						writer.Write(false);
					} 
					else 
					{
						Debug.WriteLine("writing #URLTBL");
						writer.Write(true);
						_urltblFile.Dump(ref writer);
					}
				} 
				else 
				{
					writer.Write(false);
				}

				// topics dumping
				if(dmpInfo.DumpTopics)
				{
					writer.Write(true);

					if(_topicsFile==null)
					{
						writer.Write(false);
					} 
					else 
					{
						Debug.WriteLine("writing #TOPICS");
						writer.Write(true);
						_topicsFile.Dump(ref writer);
					}
				} 
				else 
				{
					writer.Write(false);
				}

				// ftsearch dumping
				if(dmpInfo.DumpFullText)
				{
					writer.Write(true);

					if(_ftSearcher==null)
					{
						writer.Write(false);
					} 
					else 
					{
						Debug.WriteLine("writing $FIftiMain");
						writer.Write(true);
						_ftSearcher.Dump(ref writer);
					}
				} 
				else 
				{
					writer.Write(false);
				}

				// TOC dumping
				bool bWriteTOC = false;

				if( (_systemFile.BinaryTOC) && (dmpInfo.DumpBinaryTOC) )
				{
					Debug.WriteLine("writing binary TOC");
					bWriteTOC = true;
				}

				if( (!_systemFile.BinaryTOC) && (dmpInfo.DumpTextTOC) )
				{
					Debug.WriteLine("writing text-based TOC");
					bWriteTOC = true;
				}

				writer.Write(bWriteTOC);

				if(bWriteTOC)
				{
					// write table of contents
					writer.Write( _toc.Count );

					for(nCnt=0; nCnt < _toc.Count; nCnt++)
					{
						TOCItem curItem = ((TOCItem)(_toc[nCnt]));
						curItem.Dump( ref writer );
					}
				}

				// Index dumping
				bool bWriteIdx = false;

				if( (_systemFile.BinaryIndex) && (dmpInfo.DumpBinaryIndex) )
				{
					Debug.WriteLine("writing binary index");
					bWriteIdx = true;
				}

				if( (!_systemFile.BinaryIndex) && (dmpInfo.DumpTextIndex) )
				{
					Debug.WriteLine("writing text-based index");
					bWriteIdx = true;
				}

				writer.Write(bWriteIdx);

				if(bWriteIdx)
				{
					// write index
					writer.Write( _indexALinks.Count );
					for(nCnt=0; nCnt < _indexALinks.Count; nCnt++)
					{
						IndexItem curItem = ((IndexItem)(_indexALinks[nCnt]));
						curItem.Dump( ref writer );
					}

					writer.Write( _indexKLinks.Count );
					for(nCnt=0; nCnt < _indexKLinks.Count; nCnt++)
					{
						IndexItem curItem = ((IndexItem)(_indexKLinks[nCnt]));
						curItem.Dump( ref writer );
					}
				}

				// Information types dumping
				writer.Write( _informationTypes.Count );

				Debug.WriteLine("writing " + _informationTypes.Count.ToString() + " information types");

				for(nCnt=0; nCnt<_informationTypes.Count;nCnt++)
				{
					InformationType curType = _informationTypes[nCnt] as InformationType;
					
					curType.Dump(ref writer);
				}

				// Categories dumping
				writer.Write( _categories.Count );

				Debug.WriteLine("writing " + _categories.Count.ToString() + " categories");

				for(nCnt=0; nCnt<_categories.Count; nCnt++)
				{
					Category curCat = _categories[nCnt] as Category;

					curCat.Dump( ref writer);
				}

				bRet=true;
			}
			finally
			{
				dmpInfo.SaveData();
			}

			return bRet;
		}

		/// <summary>
		/// Parses a HHC file which is located in the current CHM.
		/// </summary>
		/// <param name="hhcFile">hhc file to parse</param>
		/// <returns>an arraylist with toc items</returns>
		public ArrayList ParseHHC(string hhcFile)
		{
			ArrayList arrRet = new ArrayList();
			
			CHMStream.CHMStream iw=null;
			iw=new CHMStream.CHMStream();			
			iw.OpenCHM(_chmFileName);			
			MemoryStream fileObject=null;			

			fileObject = iw.OpenStream(hhcFile);
			if( fileObject != null)
			{	
				ASCIIEncoding ascii=new ASCIIEncoding();			
				string fileString =ascii.GetString(fileObject.ToArray(),0,(int)fileObject.Length);				
				fileObject.Close();

				arrRet = HHCParser.ParseHHC(fileString, this);

				if(HHCParser.HasMergeLinks)
				{
					foreach(TOCItem curItem in HHCParser.MergeItems)
					{
						_mergeLinks.Add(curItem);
					}
				}
			}

			return arrRet;
		}

		/// <summary>
		/// Loads the toc and index from a data dump
		/// </summary>
		/// <param name="dmpInfo">dumping info</param>
		/// <returns>true if succeed</returns>
		private bool LoadDump(DumpingInfo dmpInfo)
		{
			if(dmpInfo == null)
				return false;

			bool bRet = false;

			try
			{
				BinaryReader reader = dmpInfo.Reader;

				if(reader == null)
				{
					Debug.WriteLine("No reader returned !");
					dmpInfo.SaveData(); // closes the dump
					_mustWriteDump = true;
					return false;
				}

				int nCnt = 0;

				Debug.WriteLine("reading dump-file header");
				FileInfo fi = new FileInfo(_chmFileName);
				string ftime = fi.LastWriteTime.ToString("dd.MM.yyyy HH:mm:ss.ffffff");

				string header = reader.ReadString();

				if( header != "HtmlHelpSystem dump file 1.0")
				{
					Debug.WriteLine("Unsupported dump-file format !");
					dmpInfo.SaveData(); // closes the dump
					_mustWriteDump = true;
					return false;
				}

				string ftimecheck = reader.ReadString();

				reader.ReadInt32(); // codepage, we'll use the same as for the chm file which is already set.

//				if(ftimecheck != ftime)
//				{
//					Debug.WriteLine("Dump is out of date (CHM file changed during last dump creation) !");
//					dmpInfo.SaveData(); // closes the dump
//					_mustWriteDump = true;
//					return false; // force reload
//				}
				

				bool bFlag=false;  // true if data should be in dump
				bool bFlagSupp=false; // false if data is not supported by the chm

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					bFlagSupp = reader.ReadBoolean();

					if(!dmpInfo.DumpStrings)
					{
						Debug.WriteLine("Dumped #STRINGS found but not expected !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(bFlagSupp)
					{
						Debug.WriteLine("reading #STRINGS");
						_stringsFile = new CHMStrings();
						_stringsFile.SetCHMFile(this);
						_stringsFile.ReadDump(ref reader);
					}
				} 
				else 
				{
					if(dmpInfo.DumpStrings)
					{
						Debug.WriteLine("Dumped #STRINGS expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					bFlagSupp = reader.ReadBoolean();

					if(!dmpInfo.DumpUrlStr)
					{
						Debug.WriteLine("Dumped #URLSTR found but not expected !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(bFlagSupp)
					{
						Debug.WriteLine("reading #URLSTR");
						_urlstrFile = new CHMUrlstr();
						_urlstrFile.SetCHMFile(this);
						_urlstrFile.ReadDump(ref reader);
					}
				}
				else 
				{
					if(dmpInfo.DumpUrlStr)
					{
						Debug.WriteLine("Dumped #URLSTR expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					bFlagSupp = reader.ReadBoolean();

					if(!dmpInfo.DumpUrlTbl)
					{
						Debug.WriteLine("Dumped #URLTBL found but not expected !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(bFlagSupp)
					{
						Debug.WriteLine("reading #URLTBL");
						_urltblFile = new CHMUrltable();
						_urltblFile.SetCHMFile(this);
						_urltblFile.ReadDump(ref reader);
					}
				}
				else 
				{
					if(dmpInfo.DumpUrlTbl)
					{
						Debug.WriteLine("Dumped #URLTBL expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					bFlagSupp = reader.ReadBoolean();

					if(!dmpInfo.DumpTopics)
					{
						Debug.WriteLine("Dumped #TOPICS found but not expected !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(bFlagSupp)
					{
						Debug.WriteLine("reading #TOPICS");
						_topicsFile = new CHMTopics();
						_topicsFile.SetCHMFile(this);
						_topicsFile.ReadDump(ref reader);
					}
				}
				else 
				{
					if(dmpInfo.DumpTopics)
					{
						Debug.WriteLine("Dumped #TOPICS expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					bFlagSupp = reader.ReadBoolean();

					if(!dmpInfo.DumpFullText)
					{
						Debug.WriteLine("Dumped $FIftiMain found but not expected !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(bFlagSupp)
					{
						Debug.WriteLine("reading $FIftiMain");
						_ftSearcher = new FullTextEngine();
						_ftSearcher.SetCHMFile(this);
						_ftSearcher.ReadDump(ref reader);
					}
				}
				else 
				{
					if(dmpInfo.DumpFullText)
					{
						Debug.WriteLine("Dumped $FIftiMain expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				// read table of contents
				_toc.Clear();
				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					if((_systemFile.BinaryTOC)&&(!dmpInfo.DumpBinaryTOC))
					{
						Debug.WriteLine("Binary TOC expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					} 

					if((!_systemFile.BinaryTOC)&&(!dmpInfo.DumpTextTOC))
					{
						Debug.WriteLine("Text-based TOC expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(_systemFile.BinaryTOC)
						Debug.WriteLine("reading binary TOC");
					else
						Debug.WriteLine("reading text-based TOC");

					int nTocCnt = reader.ReadInt32();

					for(nCnt=0; nCnt < nTocCnt; nCnt++)
					{
						TOCItem item = new TOCItem();
						item.AssociatedFile = this;
						item.ChmFile = _chmFileName;
						item.ReadDump(ref reader);
						if(item.MergeLink.Length > 0)
							_mergeLinks.Add(item);

						_toc.Add(item);
					}
				} 
				else 
				{
					if((_systemFile.BinaryTOC)&&(dmpInfo.DumpBinaryTOC))
					{
						Debug.WriteLine("Binary TOC expected but no TOC dump !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					} 

					if((!_systemFile.BinaryTOC)&&(dmpInfo.DumpTextTOC))
					{
						Debug.WriteLine("Text-based TOC expected but no TOC dump !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				// read index
				_indexALinks.Clear();
				_indexKLinks.Clear();

				bFlag = reader.ReadBoolean();

				if(bFlag)
				{
					if((_systemFile.BinaryIndex)&&(!dmpInfo.DumpBinaryIndex))
					{
						Debug.WriteLine("Binary index expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if((!_systemFile.BinaryIndex)&&(!dmpInfo.DumpTextIndex))
					{
						Debug.WriteLine("Binary index expected but not found !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}

					if(_systemFile.BinaryIndex)
						Debug.WriteLine("reading binary index");
					else
						Debug.WriteLine("reading text-based index");

					int nIndxaCnt = reader.ReadInt32();

					for(nCnt=0; nCnt < nIndxaCnt; nCnt++)
					{
						IndexItem item = new IndexItem();
						item.ChmFile = this;
						item.ReadDump(ref reader);
						_indexALinks.Add(item);
					}

				
					int nIndxkCnt = reader.ReadInt32();

					for(nCnt=0; nCnt < nIndxkCnt; nCnt++)
					{
						IndexItem item = new IndexItem();
						item.ChmFile = this;
						item.ReadDump(ref reader);
						_indexKLinks.Add(item);
					}
				}
				else 
				{
					if((_systemFile.BinaryIndex)&&(dmpInfo.DumpBinaryIndex))
					{
						Debug.WriteLine("Binary index expected but no index in dump !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					} 

					if((!_systemFile.BinaryIndex)&&(dmpInfo.DumpTextIndex))
					{
						Debug.WriteLine("Text-based index expected but no index dump !");
						dmpInfo.SaveData(); // closes the dump
						_mustWriteDump = true;
						return false; // force reload
					}
				}

				// read information types from dump
				int nITCnt = reader.ReadInt32();

				Debug.WriteLine("Reading " + nITCnt.ToString() + " information types from dump !");

				for(nCnt=0; nCnt<nITCnt; nCnt++)
				{
					InformationType newType = new InformationType();
					newType.ReadDump(ref reader);

					if( SystemInstance.GetInformationType( newType.Name ) != null)
					{
						// information type of this name already exists in the helpsystem
						InformationType sysType = SystemInstance.GetInformationType( newType.Name );
						_informationTypes.Add(sysType);
					} 
					else 
					{
						_informationTypes.Add( newType );
					}
				}

				// read categories from dump
				int nCCnt = reader.ReadInt32();

				Debug.WriteLine("Reading " + nITCnt.ToString() + " categories from dump !");

				for(nCnt=0; nCnt<nCCnt; nCnt++)
				{
					Category newCat = new Category();
					newCat.ReadDump(ref reader, this);

					if( SystemInstance.GetCategory( newCat.Name ) != null)
					{
						// category of this name already exists in the helpsystem
						Category sysCat = SystemInstance.GetCategory( newCat.Name );

						sysCat.MergeInfoTypes( newCat );
						_categories.Add( sysCat );
					} 
					else 
					{
						_categories.Add( newCat );
					}
				}

				_dumpRead = true;
				bRet = true;
			}
			catch(Exception ex)
			{
				Debug.WriteLine("###ERROR :" + ex.Message);
				_mustWriteDump = true;
			}
			finally
			{
				dmpInfo.SaveData(); // closes the dump
			}

			return bRet;
		}

		#endregion

		#region Internal properties
		/// <summary>
		/// Gets the current storage wrapper.
		/// </summary>
		/// <remarks>This property will return not null, if there are currently file read actions running !</remarks>
		internal CHMStream.CHMStream CurrentStorageWrapper
		{
			get { return _currentWrapper;}
		}
		/// <summary>
		/// Gets/sets the hosting HtmlHelpSystem instance
		/// </summary>
		internal HtmlHelpSystem SystemInstance
		{
			get { return _systemInstance; }
			set { _systemInstance = value; }
		}
		/// <summary>
		/// Gets an arraylist of TOC items which contains merg-links to other CHMs
		/// </summary>
		internal ArrayList MergLinks
		{
			get { return _mergeLinks; }
		}

		/// <summary>
		/// Gets the internal system file instance
		/// </summary>
		internal CHMSystem SystemFile
		{
			get { return _systemFile; }
		}
		
		/// <summary>
		/// Gets the internal idxhdr file instance
		/// </summary>
		internal CHMIdxhdr IdxHdrFile
		{
			get { return _idxhdrFile; }
		}

		/// <summary>
		/// Gets the internal strings file instance
		/// </summary>
		internal CHMStrings StringsFile
		{
			get { return _stringsFile; }
		}
		
		/// <summary>
		/// Gets the internal urlstr file instance
		/// </summary>
		internal CHMUrlstr UrlstrFile
		{
			get { return _urlstrFile; }
		}

		/// <summary>
		/// Gets the internal urltbl file instance
		/// </summary>
		internal CHMUrltable UrltblFile
		{
			get { return _urltblFile; }
		}

		/// <summary>
		/// Gets the internal topics file instance
		/// </summary>
		internal CHMTopics TopicsFile
		{
			get { return _topicsFile; }
		}

		/// <summary>
		/// Gets the internal tocidx file instance
		/// </summary>
		internal CHMTocidx TocidxFile
		{
			get { return _tocidxFile; }
		}

		/// <summary>
		/// Gets the internal btree file instance for alinks
		/// </summary>
		internal CHMBtree ALinksFile
		{
			get { return _aLinks; }
		}

		/// <summary>
		/// Gets the internal btree file instance for klinks
		/// </summary>
		internal CHMBtree KLinksFile
		{
			get { return _kLinks; }
		}

		/// <summary>
		/// Gets/Sets the text encoding
		/// </summary>
		internal Encoding TextEncoding
		{
			get { return _textEncoding; }
			set { _textEncoding = value; }
		}

		#endregion

		#region Properties from the #SYSTEM file
		/// <summary>
		/// Gets the file version of the chm file. 
		/// 2 for Compatibility=1.0,  3 for Compatibility=1.1
		/// </summary>
		internal int FileVersion
		{
			get 
			{ 
				if(_systemFile==null)
					return 0;

				return _systemFile.FileVersion; 
			}
		}

		/// <summary>
		/// Gets the contents file name
		/// </summary>
		internal string ContentsFile
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.ContentsFile; 
			}
		}

		/// <summary>
		/// Gets the index file name
		/// </summary>
		internal string IndexFile
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.IndexFile; 
			}
		}

		/// <summary>
		/// Gets the default help topic
		/// </summary>
		internal string DefaultTopic
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.DefaultTopic; 
			}
		}

		/// <summary>
		/// Gets the title of the help window
		/// </summary>
		internal string HelpWindowTitle
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.Title; 
			}
		}

		/// <summary>
		/// Gets the flag if DBCS is in use
		/// </summary>
		internal bool DBCS
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.DBCS; 
			}
		}

		/// <summary>
		/// Gets the flag if full-text-search is available
		/// </summary>
		internal bool FullTextSearch
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.FullTextSearch; 
			}
		}

		/// <summary>
		/// Gets the flag if the file has ALinks
		/// </summary>
		internal bool HasALinks
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.HasALinks; 
			}
		}

		/// <summary>
		/// Gets the flag if the file has KLinks
		/// </summary>
		internal bool HasKLinks
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.HasKLinks; 
			}
		}

		/// <summary>
		/// Gets the default window name
		/// </summary>
		internal string DefaultWindow
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.DefaultWindow; 
			}
		}

		/// <summary>
		/// Gets the file name of the compile file
		/// </summary>
		internal string CompileFile
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.CompileFile; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
		internal bool BinaryIndex
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.BinaryIndex; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
		internal string CompilerVersion
		{
			get 
			{ 
				if(_systemFile==null)
					return String.Empty;

				return _systemFile.CompilerVersion; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary toc file
		/// </summary>
		internal bool BinaryTOC
		{
			get 
			{ 
				if(_systemFile==null)
					return false;

				return _systemFile.BinaryTOC; 
			}
		}

		/// <summary>
		/// Gets the font face of the read font property.
		/// Empty string for default font.
		/// </summary>
		internal string FontFace
		{
			get
			{
				if(_systemFile==null)
					return "";
					
				return _systemFile.FontFace; 
			}
		}

		/// <summary>
		/// Gets the font size of the read font property.
		/// 0 for default font size
		/// </summary>
		internal double FontSize
		{
			get
			{
				if(_systemFile==null)
					return 0;
					
				return _systemFile.FontSize; 
			}
		}

		/// <summary>
		/// Gets the character set of the read font property
		/// 1 for default
		/// </summary>
		internal int CharacterSet
		{
			get
			{
				if(_systemFile==null)
					return 1;
					
				return _systemFile.CharacterSet; 
			}
		}

		/// <summary>
		/// Gets the codepage depending on the read font property
		/// </summary>
		internal int CodePage
		{
			get
			{
				if(_systemFile==null)
					return CultureInfo.CurrentCulture.TextInfo.ANSICodePage;
					
				return _systemFile.CodePage; 
			}
		}

		/// <summary>
		/// Gets the assiciated culture info
		/// </summary>
		internal CultureInfo Culture
		{
			get 
			{ 
				if(_systemFile==null)
					return CultureInfo.CurrentCulture;
					
				return _systemFile.Culture; 
			}
		}

		#endregion 

		#region Properties from the #IDXHDR file
		/// <summary>
		/// Gets the number of topic nodes including the contents and index files
		/// </summary>
		internal int NumberOfTopicNodes
		{
			get 
			{
				if(_idxhdrFile==null)
					return 0;

				return _idxhdrFile.NumberOfTopicNodes; 
			}
		}

		/// <summary>
		/// Gets the ImageList string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
		internal string ImageList
		{
			get 
			{
				if( (_stringsFile == null) || (_idxhdrFile == null) )
					return "";

				return _stringsFile[ _idxhdrFile.ImageListOffset ];
			}
		}

		/// <summary>
		/// True if the value of the ImageType param of the 
		/// "text/site properties" object of the sitemap contents is "Folder". 
		/// </summary>
		/// <remarks>If this is set to true, the help will display folders instead of books</remarks>
		internal bool ImageTypeFolder
		{
			get 
			{ 
				if(_idxhdrFile==null)
					return false;

				return _idxhdrFile.ImageTypeFolder; 
			}
		}

		/// <summary>
		/// Gets the background setting 
		/// </summary>
		internal int Background
		{
			get 
			{ 
				if(_idxhdrFile==null)
					return 0;

				return _idxhdrFile.Background; 
			}
		}

		/// <summary>
		/// Gets the foreground setting 
		/// </summary>
		internal int Foreground
		{
			get 
			{ 
				if(_idxhdrFile==null)
					return 0;

				return _idxhdrFile.Foreground; 
			}
		}

		/// <summary>
		/// Gets the Font string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
		internal string FontName
		{
			get 
			{
				if( (_stringsFile == null) || (_idxhdrFile == null) )
					return "";

				return _stringsFile[ _idxhdrFile.FontOffset ];
			}
		}

		/// <summary>
		/// Gets the FrameName string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
		internal string FrameName
		{
			get 
			{
				if( (_stringsFile == null) || (_idxhdrFile == null) )
					return "";

				return _stringsFile[ _idxhdrFile.FrameNameOffset ];
			}
		}

		/// <summary>
		/// Gets the WindowName string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
		internal string WindowName
		{
			get 
			{
				if( (_stringsFile == null) || (_idxhdrFile == null) )
					return "";

				return _stringsFile[ _idxhdrFile.WindowNameOffset ];
			}
		}

		/// <summary>
		/// Gets a string array containing the merged file names
		/// </summary>
		internal string[] MergedFiles
		{
			get
			{
				if( (_stringsFile == null) || (_idxhdrFile == null) )
					return new string[0];

				string[] saRet = new string[ _idxhdrFile.MergedFileOffsets.Count ];

				for(int i=0; i < _idxhdrFile.MergedFileOffsets.Count; i++)
				{
					saRet[i] = _stringsFile[ (int)_idxhdrFile.MergedFileOffsets[i] ];
				}

				return saRet;
			}
		}
		#endregion

		/// <summary>
		/// Gets the file info associated with this instance
		/// </summary>
		public ChmFileInfo FileInfo
		{
			get { return _chmFileInfo; }
		}

		/// <summary>
		/// Gets the internal toc read from the text-based hhc file
		/// </summary>
		public ArrayList TOC
		{
			get { return _toc; }
		}

		/// <summary>
		/// Gets the internal index read from the chm file.
		/// </summary>
		public ArrayList IndexKLinks
		{
			get { return _indexKLinks; }
		}

		/// <summary>
		/// Gets the internal index read from the chm file.
		/// </summary>
		public ArrayList IndexALinks
		{
			get { return _indexALinks; }
		}

		/// <summary>
		/// Gets the full-text search engine for this file
		/// </summary>
		internal FullTextEngine FullTextSearchEngine
		{
			get { return _ftSearcher; }
		}

		/// <summary>
		/// Gets the full pathname of the file
		/// </summary>
		public string ChmFilePath
		{
			get { return _chmFileName; }
		}

		/// <summary>
		/// Gets the full pathname of the chi-file
		/// The file name is zero-length if there is no chi-file
		/// </summary>
		public string ChiFilePath
		{
			get { return _chiFileName; }
		}

		/// <summary>
		/// Gets the full pathname of the chw-file
		/// The file name is zero-length if there is no chw-file
		/// </summary>
		public string ChwFilePath
		{
			get { return _chwFileName; }
		}

		/// <summary>
		/// Gets the full pathname of the chq-file
		/// The file name is zero-length if there is no chq-file
		/// </summary>
		public string ChqFilePath
		{
			get { return _chqFileName; }
		}

		/// <summary>
		/// Forms an URL for the web browser
		/// </summary>
		/// <param name="local">local resource</param>
		/// <returns>a url for the web-browser</returns>
		internal string FormURL(string local)
		{
			if( (local.ToLower().IndexOf("http://") >= 0) ||
				(local.ToLower().IndexOf("https://") >= 0) ||
				(local.ToLower().IndexOf("mailto:") >= 0) ||
				(local.ToLower().IndexOf("ftp://") >= 0)  ||
				(local.ToLower().IndexOf("ms-its:") >= 0))				
				return local;

			return HtmlHelpSystem.UrlPrefix + _chmFileName + "::/" + local;
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
					_toc.Clear();
					_indexKLinks.Clear();
					_indexALinks.Clear();

					if(_systemFile!=null)
						_systemFile.Dispose();

					if(_idxhdrFile != null)
						_idxhdrFile.Dispose();

					if(_stringsFile != null)
						_stringsFile.Dispose();

					if(_urlstrFile != null)
						_urlstrFile.Dispose();

					if(_urltblFile != null)
						_urltblFile.Dispose();

					if(_topicsFile != null)
						_topicsFile.Dispose();

					if(_tocidxFile != null)
						_tocidxFile.Dispose();

					if(_kLinks != null)
						_kLinks.Dispose();

					if(_aLinks != null)
						_aLinks.Dispose();

					if(_ftSearcher != null)
						_ftSearcher.Dispose();

					if(_chmFileInfo != null)
						_chmFileInfo = null;
				}
			}
			disposed = true;         
		}

	}
}
