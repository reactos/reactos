using System;
using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Globalization;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMSystem</c> reads the #SYSTEM file of the chm and stores its settings
	/// </summary>
	internal sealed class CHMSystem : IDisposable
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
		/// Internal member storing the file version 
		/// </summary>
		private int _fileVersion = 0;
		/// <summary>
		/// Internal member storing the contents file path
		/// </summary>
		private string _contentsFile = "";
		/// <summary>
		/// Internal member storing the index file path
		/// </summary>
		private string _indexFile = "";
		/// <summary>
		/// Internal member storing the default help topic
		/// </summary>
		private string _defaultTopic = "";
		/// <summary>
		/// Internal member storing the help-window title
		/// </summary>
		private string _title = "";
		/// <summary>
		/// Internal flag if dbcs is on
		/// </summary>
		private bool _dbcs = false;
		/// <summary>
		/// Internal flag if fulltext search is enabled
		/// </summary>
		private bool _fullTextSearch = false;
		/// <summary>
		/// Internal flag if KLinks are in the file
		/// </summary>
		private bool _hasKLinks = false;
		/// <summary>
		/// Internal flag if ALinks are in the file
		/// </summary>
		private bool _hasALinks = false;
		/// <summary>
		/// Internal member storing the name of the default window
		/// </summary>
		private string _defaultWindow = "";
		/// <summary>
		/// Internal member storing the filename of the compiled file
		/// </summary>
		private string _compileFile = "";
		/// <summary>
		/// Internal flag storing the offset value of the binary index
		/// </summary>
		private uint _binaryIndexURLTableID = 0;
		/// <summary>
		/// Inernal member storing the compiler version this file was compiled
		/// </summary>
		private string _compilerVersion = "";
		/// <summary>
		/// Internal flag storing the offset value of the binary TOC
		/// </summary>
		private uint _binaryTOCURLTableID = 0;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;
		/// <summary>
		/// Internal member storing the default fontface, size, charset
		/// </summary>
		private string _defaultFont = "";
		/// <summary>
		/// Internal member storing the culture info of the file
		/// </summary>
		private CultureInfo _culture;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #SYSTEM file</param>
		/// <param name="associatedFile">associated chm file</param>
		public CHMSystem(byte[] binaryFileData, CHMFile associatedFile)
		{
			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;
			DecodeData();

			if(_culture == null)
			{
				// Set the text encoder of the chm file to the read charset/codepage
				_associatedFile.TextEncoding = Encoding.GetEncoding( this.CodePage );
			}
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

			// First entry = DWORD for version number
			_fileVersion = (int) binReader.ReadInt32();

			while( (memStream.Position < memStream.Length) && (bRet) )
			{
				bRet &= DecodeEntry(ref binReader);
			}

			return bRet;
		}

		/// <summary>
		/// Decodes an #system file entry
		/// </summary>
		/// <param name="binReader">binary reader reference</param>
		/// <returns>true if succeeded</returns>
		private bool DecodeEntry(ref BinaryReader binReader)
		{
			bool bRet = true;

			int code = (int) binReader.ReadInt16(); // entry code, WORD
			int length = (int) binReader.ReadInt16(); // length of entry

			switch(code)
			{
				case 0:
				{
					_contentsFile = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 1:
				{
					_indexFile = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 2:
				{
					_defaultTopic = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 3:
				{
					_title = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 4:
				{
					int nTemp = 0;
					nTemp = binReader.ReadInt32(); // read DWORD LCID
					_culture = new CultureInfo(nTemp);

					if(_culture != null)
						_associatedFile.TextEncoding = Encoding.GetEncoding(_culture.TextInfo.ANSICodePage);

					nTemp = binReader.ReadInt32(); // read DWORD DBCS
					_dbcs = (nTemp == 1);

					nTemp = binReader.ReadInt32(); // read DWORD Fulltext search
					_fullTextSearch = (nTemp == 1);

					nTemp = binReader.ReadInt32(); // read DWORD has klinks
					_hasKLinks = (nTemp != 0);

					nTemp = binReader.ReadInt32(); // read DWORD has alinks
					_hasALinks = (nTemp != 0);
					
					// read the rest of code 4 (not important for us)
					byte[] temp = new byte[length-(5*4)];
					temp = binReader.ReadBytes(length-(5*4));
				};break;
				case 5:
				{
					_defaultWindow = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 6:
				{
					_compileFile = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 7:
				{
					if(_fileVersion > 2)
					{
						_binaryIndexURLTableID = (uint) binReader.ReadInt32();
					} 
					else 
					{
						byte[] read = binReader.ReadBytes(length);
						int i=read.Length;
					}
				};break;
				case 8:
				{
					// abbreviation (not interresting for us)
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 9:
				{
					_compilerVersion = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);
				};break;
				case 10:
				{
					// timestamp of the file (not interresting for us)
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 11:
				{
					if(_fileVersion > 2)
					{
						_binaryTOCURLTableID = (uint) binReader.ReadInt32();
					} 
					else 
					{
						byte[] read = binReader.ReadBytes(length);
						int i=read.Length;
					}
				};break;
				case 12:
				{
					// number of information bytes
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 13:
				{
					// copy of file #idxhdr
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 14:
				{
					// custom tabs for HH viewer
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 15:
				{
					// a checksum
					byte[] read = binReader.ReadBytes(length);
					int i=read.Length;
				};break;
				case 16:
				{
					// Default Font=string,number,number  
					// The string is the name of the font, the first number is the 
					// point size & the last number is the character set used by the font. 
					// For acceptable values see *_CHARSET defines in wingdi.h from the 
					// Windows SDK or the same file in MinGW or Wine. 
					// Most of the time you will only want to use 0, which is the value for ANSI, 
					// which is the subset of ASCII used by Windows. 
					_defaultFont = BinaryReaderHelp.ExtractString(ref binReader,length, 0, true, _associatedFile.TextEncoding);

				};break;
				default:
				{
					byte[] temp = new byte[length];
					temp = binReader.ReadBytes(length);
					//bRet = false;
					int i=temp.Length;
				};break;
			}

			return bRet;
		}

		/// <summary>
		/// Reads all HHC files and checks which one has the global object tag.
		/// This hhc file will be returned as master hhc
		/// </summary>
		/// <param name="hhcTopics">list of topics containing the extension hhc</param>
		/// <param name="TopicItemArrayList">true if the arraylist contains topic items</param>
		/// <returns>the filename of the found master toc</returns>
		private string GetMasterHHC(ArrayList hhcTopics, bool TopicItemArrayList)
		{
			string sRet = "";

			if( (hhcTopics!=null) && (hhcTopics.Count > 0) )
			{
				if( TopicItemArrayList )
				{
					if(hhcTopics.Count == 1)
					{
						sRet = ((TopicEntry)hhcTopics[0]).Locale;
					} 
					else 
					{
						foreach(TopicEntry curEntry in hhcTopics)
						{
							CHMStream.CHMStream iw=null;
							MemoryStream fileObject=null;			

							if( _associatedFile.CurrentStorageWrapper == null)
							{
								iw=new CHMStream.CHMStream();			
								iw.OpenCHM(_associatedFile.ChmFilePath);			
							} 
							else 
							{
								iw = _associatedFile.CurrentStorageWrapper;
							}

							fileObject = iw.OpenStream(curEntry.Locale);
							if( fileObject != null)
							{
								string fileString =_associatedFile.TextEncoding.GetString(fileObject.ToArray(),0,(int)fileObject.Length);
								fileObject.Close();

								if( HHCParser.HasGlobalObjectTag(fileString, _associatedFile) )
								{
									sRet = curEntry.Locale;
									break;
								}
							}
						}
					}
				} 
				else 
				{
					if(hhcTopics.Count == 1)
					{
						sRet = ((string)hhcTopics[0]);
					} 
					else 
					{
						foreach(string curEntry in hhcTopics)
						{
							CHMStream.CHMStream iw=null;
							MemoryStream fileObject=null;			

							if( _associatedFile.CurrentStorageWrapper == null)
							{								
								iw=new CHMStream.CHMStream();			
								iw.OpenCHM(_associatedFile.ChmFilePath);			
							} 
							else 
							{
								iw = _associatedFile.CurrentStorageWrapper;
							}

							fileObject = iw.OpenStream(curEntry);
							if( fileObject != null)
							{
								string fileString =_associatedFile.TextEncoding.GetString(fileObject.ToArray(),0,(int)fileObject.Length);								
								fileObject.Close();

								if( HHCParser.HasGlobalObjectTag(fileString, _associatedFile) )
								{
									sRet = curEntry;
									break;
								}
							}
						}
					}
				}
			}

			return sRet;
		}

		/// <summary>
		/// Gets the file version of the chm file. 
		/// 2 for Compatibility=1.0,  3 for Compatibility=1.1
		/// </summary>
		public int FileVersion
		{
			get { return _fileVersion; }
		}

		/// <summary>
		/// Gets the contents file name
		/// </summary>
		public string ContentsFile
		{
			get 
			{ 
				if( BinaryTOC ) // if the file contains a binary TOC
				{
					// make sure the CHMFile instance exists and has loaded the file #URLTBL
					if( (_associatedFile != null) && (_associatedFile.UrltblFile != null ) )
					{
						// Get an url-table entry by its unique id
						UrlTableEntry entry = _associatedFile.UrltblFile.GetByUniqueID( this.BinaryTOCURLTableID );
						if(entry != null)
						{
							// entry found, return the url ( = filename )
							return entry.URL;
						}
					}
				} 
				else 
				{
					if(_contentsFile.Length <= 0)
					{
						string sCheck = "Table of Contents.hhc"; // default HHP contents filename

						if( (_associatedFile != null) && (_associatedFile.TopicsFile != null ) )
						{
							TopicEntry te = _associatedFile.TopicsFile.GetByLocale( sCheck );

							if( te == null)
							{
								sCheck = "toc.hhc"; // default HHP contents filename

								te = _associatedFile.TopicsFile.GetByLocale( sCheck );

								if( te == null)
								{
									sCheck = CompileFile + ".hhc";

									te = _associatedFile.TopicsFile.GetByLocale( sCheck );

									if( te == null)
									{
										ArrayList arrExt = _associatedFile.TopicsFile.GetByExtension("hhc");

										if( arrExt == null )
										{
											arrExt = _associatedFile.EnumFilesByExtension("hhc");

											if( arrExt == null )
											{
												Debug.WriteLine("CHMSystem.ContentsFile - Failed, contents file not found !");
											} 
											else 
											{
												if(arrExt.Count > 1)
												{
													sCheck = GetMasterHHC(arrExt, false);
													_contentsFile = sCheck;
												} 
												else 
												{
													_contentsFile = ((string)arrExt[0]);
													sCheck = _contentsFile;
												}
											}
										} 
										else 
										{
											if(arrExt.Count > 1)
											{
												sCheck = GetMasterHHC(arrExt, true);
												_contentsFile = sCheck;
											} 
											else 
											{
												_contentsFile = ((TopicEntry)arrExt[0]).Locale;
												sCheck = _contentsFile;
											}
										}
									} 
									else 
									{
										_contentsFile = sCheck;
									}
								} 
								else 
								{
									_contentsFile = sCheck;
								}
							} 
							else 
							{
								_contentsFile = sCheck;
							}
						}

						return sCheck;
					}
				}

				return _contentsFile; 
			}
		}

		/// <summary>
		/// Gets the index file name
		/// </summary>
		public string IndexFile
		{
			get 
			{ 
				if( BinaryIndex ) // if the file contains a binary index
				{
					// make sure the CHMFile instance exists and has loaded the file #URLTBL
					if( (_associatedFile != null) && (_associatedFile.UrltblFile != null ) )
					{
						// Get an url-table entry by its unique id
						UrlTableEntry entry = _associatedFile.UrltblFile.GetByUniqueID( this.BinaryIndexURLTableID );
						if(entry != null)
						{
							// entry found, return the url ( = filename )
							return entry.URL;
						}
					}
				} 
				else 
				{
					if(_indexFile.Length <= 0)
					{
						string sCheck = "Index.hhk"; // default HHP index filename

						if( (_associatedFile != null) && (_associatedFile.TopicsFile != null ) )
						{
							TopicEntry te = _associatedFile.TopicsFile.GetByLocale( sCheck );

							if( te == null)
							{
								sCheck = CompileFile + ".hhk";

								te = _associatedFile.TopicsFile.GetByLocale( sCheck );

								if( te == null)
								{
									ArrayList arrExt = _associatedFile.TopicsFile.GetByExtension("hhk");

									if( arrExt == null )
									{
										Debug.WriteLine("CHMSystem.IndexFile - Failed, index file not found !");
									} 
									else 
									{
										_indexFile = ((TopicEntry)arrExt[0]).Locale;
										sCheck = _indexFile;
									}
								} 
								else 
								{
									_indexFile = sCheck;
								}
							} 
							else 
							{
								_indexFile = sCheck;
							}
						}

						return sCheck;
					}
				}
				return _indexFile; 
			}
		}

		/// <summary>
		/// Sets the default topic of this file
		/// </summary>
		/// <param name="local">new local value of the topic</param>
		internal void SetDefaultTopic(string local)
		{
			_defaultTopic = local;
		}

		/// <summary>
		/// Gets the default help topic
		/// </summary>
		public string DefaultTopic
		{
			get { return _defaultTopic; }
		}

		/// <summary>
		/// Gets the title of the help window
		/// </summary>
		public string Title
		{
			get { return _title; }
		}

		/// <summary>
		/// Gets the flag if DBCS is in use
		/// </summary>
		public bool DBCS
		{
			get { return _dbcs; }
		}

		/// <summary>
		/// Gets the flag if full-text-search is available
		/// </summary>
		public bool FullTextSearch
		{
			get { return _fullTextSearch; }
		}

		/// <summary>
		/// Gets the flag if the file has ALinks
		/// </summary>
		public bool HasALinks
		{
			get { return _hasALinks; }
		}

		/// <summary>
		/// Gets the flag if the file has KLinks
		/// </summary>
		public bool HasKLinks
		{
			get { return _hasKLinks; }
		}

		/// <summary>
		/// Gets the default window name
		/// </summary>
		public string DefaultWindow
		{
			get { return _defaultWindow; }
		}

		/// <summary>
		/// Gets the file name of the compile file
		/// </summary>
		public string CompileFile
		{
			get { return _compileFile; }
		}

		/// <summary>
		/// Gets the id of the binary index in the url table
		/// </summary>
		public uint BinaryIndexURLTableID
		{
			get { return _binaryIndexURLTableID; }
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
		public bool BinaryIndex
		{
			get { return (_binaryIndexURLTableID>0); }
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
		public string CompilerVersion
		{
			get { return _compilerVersion; }
		}

		/// <summary>
		/// Gets the id of the binary toc in the url table
		/// </summary>
		public uint BinaryTOCURLTableID
		{
			get { return _binaryTOCURLTableID; }
		}

		/// <summary>
		/// Gets the flag if the chm has a binary toc file
		/// </summary>
		public bool BinaryTOC
		{
			get { return (_binaryTOCURLTableID>0); }
		}

		/// <summary>
		/// Gets the font face of the read font property.
		/// Empty string for default font.
		/// </summary>
		public string FontFace
		{
			get
			{
				if( _defaultFont.Length > 0)
				{
					string [] fontSplit = _defaultFont.Split( new char[]{','});
					if(fontSplit.Length > 0)
						return fontSplit[0].Trim();
				}

				return "";
			}
		}

		/// <summary>
		/// Gets the font size of the read font property.
		/// 0 for default font size
		/// </summary>
		public double FontSize
		{
			get
			{
				if( _defaultFont.Length > 0)
				{
					string [] fontSplit = _defaultFont.Split( new char[]{','});
					if(fontSplit.Length > 1)
						return double.Parse(fontSplit[1].Trim());
				}

				return 0.0;
			}
		}

		/// <summary>
		/// Gets the character set of the read font property
		/// 1 for default
		/// </summary>
		public int CharacterSet
		{
			get
			{
				if( _defaultFont.Length > 0)
				{
					string [] fontSplit = _defaultFont.Split( new char[]{','});
					if(fontSplit.Length > 2)
						return Int32.Parse(fontSplit[2].Trim());
				}

				return 0;
			}
		}

		/// <summary>
		/// Gets the codepage depending on the read font property
		/// </summary>
		public int CodePage
		{
			get
			{
				// if we've read a LCID from the system file
				// ignore the font-property settings and return
				// the codepage generated from the culture info
				if(_culture != null)
				{
					return _culture.TextInfo.ANSICodePage;
				}

				int nRet = 1252; // default codepage windows-1252

				int nCSet = CharacterSet;

				switch(nCSet)
				{
					case 0x00: nRet = 1252;break;	// ANSI_CHARSET
					case 0xCC: nRet = 1251;break;	// RUSSIAN_CHARSET
					case 0xEE: nRet = 1250;break;	// EE_CHARSET
					case 0xA1: nRet = 1253;break;	// GREEK_CHARSET
					case 0xA2: nRet = 1254;break;	// TURKISH_CHARSET
					case 0xBA: nRet = 1257;break;	// BALTIC_CHARSET
					case 0xB1: nRet = 1255;break;	// HEBREW_CHARSET
					case 0xB2: nRet = 1256;break;	// ARABIC_CHARSET
					case 0x80: nRet =  932;break;	// SHIFTJIS_CHARSET
					case 0x81: nRet =  949;break;	// HANGEUL_CHARSET
					case 0x86: nRet =  936;break;	// GB2313_CHARSET
					case 0x88: nRet =  950;break;	// CHINESEBIG5_CHARSET
				}

				return nRet;
			}
		}

		/// <summary>
		/// Gets the assiciated culture info
		/// </summary>
		public CultureInfo Culture
		{
			get { return _culture; }
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
