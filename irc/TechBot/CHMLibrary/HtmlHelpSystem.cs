using System;
using System.IO;
using System.Collections;
using System.Data;

using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>HtmlHelpSystem</c> implements the main object for reading chm files
	/// </summary>
	public sealed class HtmlHelpSystem
	{
		/// <summary>
		/// Private shared instance of current HtmlHelpSystem class
		/// </summary>
		private static HtmlHelpSystem _current=null;
		/// <summary>
		/// Internal member storing the attached files
		/// </summary>
		private ArrayList _chmFiles = new ArrayList();
		/// <summary>
		/// Internal member storing a merged table of contents
		/// </summary>
		private TableOfContents _toc = new TableOfContents();
		/// <summary>
		/// Internal member storing a merged index
		/// </summary>
		private Index _index = new Index();
		/// <summary>
		/// URL prefix for specifying a chm destination
		/// </summary>
		private static string _urlPrefix = "ms-its:";
		/// <summary>
		/// Internal flag specifying if the system should use the tree-images list
		/// from HtmlHelp2. If false the standard CHM-Viewer pics will be used.
		/// </summary>
		private static bool _useHH2TreePics = false;
		/// <summary>
		/// Internal member storing the read information types
		/// </summary>
		private ArrayList _informationTypes = new ArrayList();
		/// <summary>
		/// Internal member storing the read categories
		/// </summary>
		private ArrayList _categories = new ArrayList();

		/// <summary>
		/// Gets/Sets the url prefix for specifying a chm destination
		/// </summary>
		public static string UrlPrefix
		{
			get { return _urlPrefix; }
			set { _urlPrefix = value; }
		}

		public CHMStream.CHMStream BaseStream
		{
			get 
			{
				CHMFile chm=(CHMFile)_chmFiles[0];
				return chm.BaseStream; 
			}
		}

		/// <summary>
		/// Gets/Sets the flag specifying if the system should use the tree-images list
		/// from HtmlHelp2. If false the standard CHM-Viewer pics will be used.
		/// </summary>
		public static bool UseHH2TreePics
		{
			get { return _useHH2TreePics; }
			set { _useHH2TreePics = value; }
		}

		/// <summary>
		/// Gets the current HtmlHelpSystem instance
		/// </summary>
		public static HtmlHelpSystem Current
		{
			get 
			{
				return _current;
			}
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		public HtmlHelpSystem() : this("")
		{
		}

		/// <summary>
		/// Constructor of the reader class
		/// </summary>
		/// <param name="chmFile">chm file to attach with the reader</param>
		public HtmlHelpSystem(string chmFile)
		{
			_current = this;
			OpenFile(chmFile);
		}


		/// <summary>
		/// Opens a chm file and creates
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to open</param>
		/// <remarks>If you call this method, all existing merged files will be cleared.</remarks>
		public void OpenFile(string chmFile)
		{
			OpenFile(chmFile, null);
		}

		/// <summary>
		/// Opens a chm file and creates
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to open</param>
		/// <param name="dmpInfo">dumping info</param>
		/// <remarks>If you call this method, all existing merged files will be cleared.</remarks>
		public void OpenFile(string chmFile, DumpingInfo dmpInfo)
		{
			if( File.Exists(chmFile ) )
			{
				_chmFiles.Clear();
				_toc.Clear();
				_index.Clear();
				_informationTypes.Clear();
				_categories.Clear();

				CHMFile newFile = new CHMFile(this, chmFile, dmpInfo);

				_toc = new TableOfContents( newFile.TOC );
				_index = new Index( newFile.IndexKLinks, newFile.IndexALinks );

				_chmFiles.Add(newFile);
				// add all infotypes and categories of the read file to this system instance
				MergeFileInfoTypesCategories(newFile);

				// check if the file has a merged files list
				if( newFile.MergedFiles.Length > 0 )
				{
					// extract the path of the chm file (usually merged files are in the same path)
					FileInfo fi = new FileInfo(chmFile);
					string sPath = fi.DirectoryName;

					for(int i=0; i<newFile.MergedFiles.Length; i++)
					{
						string sFile = newFile.MergedFiles[i];

						if(sFile.Length > 0)
						{
							if(sFile[1] != ':') // no full path setting
							{
								sFile = Path.Combine(sPath, sFile);
							}

							MergeFile(sFile, dmpInfo, true);
						}
					}

					// if (newFile.MergLinks.Count>0)
					// 	RecalculateMergeLinks(newFile);

					RemoveMergeLinks(); // clear all merge-links which have no target !
				}
			}
		}

		/// <summary>
		/// Merges a chm file to the current help contents
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to merge</param>
		public void MergeFile(string chmFile)
		{
			MergeFile(chmFile, null);
		}

		/// <summary>
		/// Merges a chm file to the current help contents
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to merge</param>
		/// <param name="dmpInfo">dumping info</param>
		public void MergeFile(string chmFile, DumpingInfo dmpInfo)
		{
			MergeFile(chmFile, dmpInfo, false);
		}

		/// <summary>
		/// Merges a chm file to the current help contents
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to merge</param>
		/// <param name="dmpInfo">dumping info</param>
		/// <param name="mergedFileList">true if the merge is done because a merged file list 
		/// was found in the previously loaded CHM.</param>
		internal void MergeFile(string chmFile, DumpingInfo dmpInfo, bool mergedFileList)
		{
			if( File.Exists(chmFile ) )
			{
				if( _chmFiles.Count == 1)
				{
					// if we open the first file, we directly point into the toc and index of this file.
					// So that we don't merge the new toc's indexe's into the first file, we have to 
					// clone the internal arraylists first to a new instance of the toc/index holder classes.
					ArrayList atoc = _toc.TOC;
					ArrayList alinks = _index.ALinks;
					ArrayList klinks = _index.KLinks;

					_toc = new TableOfContents();
					_index = new Index();

					_toc.MergeToC( atoc );
					_index.MergeIndex( alinks, IndexType.AssiciativeLinks );
					_index.MergeIndex( klinks, IndexType.KeywordLinks );
				}

				CHMFile newFile = new CHMFile(this, chmFile, dmpInfo);

				if(mergedFileList) // if we've called this method due to a merged file list merge
				{
					RecalculateMergeLinks(newFile);

					_toc.MergeToC( newFile.TOC, _chmFiles );
					_index.MergeIndex( newFile.IndexALinks, IndexType.AssiciativeLinks );
					_index.MergeIndex( newFile.IndexKLinks, IndexType.KeywordLinks );

					_chmFiles.Add(newFile);

					// add all infotypes and categories of the read file to this system instance
					MergeFileInfoTypesCategories(newFile);
				}  
				else 
				{
					_toc.MergeToC( newFile.TOC, _chmFiles );
					_index.MergeIndex( newFile.IndexALinks, IndexType.AssiciativeLinks );
					_index.MergeIndex( newFile.IndexKLinks, IndexType.KeywordLinks );

					_chmFiles.Add(newFile);

					// add all infotypes and categories of the read file to this system instance
					MergeFileInfoTypesCategories(newFile);

					// check if the file has a merged files list
					if( newFile.MergedFiles.Length > 0 )
					{
						// extract the path of the chm file (usually merged files are in the same path)
						FileInfo fi = new FileInfo(chmFile);
						string sPath = fi.DirectoryName;

						for(int i=0; i<newFile.MergedFiles.Length; i++)
						{
							string sFile = newFile.MergedFiles[i];

							if(sFile.Length > 0)
							{
								if(sFile[1] != ':') // no full path setting
								{
									sFile = Path.Combine(sPath, sFile);
								}

								MergeFile(sFile, dmpInfo, true);
							}
						}

						RemoveMergeLinks(); // clear all merge-links which have no target !
					}
				}
			}
		}

		/// <summary>
		/// Checks all Merg-links read till now. Checks if the merg-link points to the 
		/// file <c>currentFile</c>. If yes the link will be replaced by the contents of the 
		/// merged file.
		/// </summary>
		/// <param name="currentFile">Current CHMFile instance</param>
		internal void RecalculateMergeLinks(CHMFile currentFile)
		{
			foreach(CHMFile curFile in _chmFiles)
			{
				if( curFile.MergLinks.Count > 0)
				{
					for(int i=0; i<curFile.MergLinks.Count; i++)
					{
						TOCItem curItem = curFile.MergLinks[i] as TOCItem;

						string sMerge = curItem.MergeLink;
						string [] sSplit = sMerge.Split( new char[]{':'} );

						string sFName = "";
						string sTarget = "";

						if( sSplit.Length > 3) // merge info contains path name
						{
							sFName = sSplit[0] + ":" + sSplit[1];
							sTarget = sSplit[3];
						} 
						else if( sSplit.Length == 3)// merge info contains only file name
						{
							FileInfo fi = new FileInfo(currentFile.ChmFilePath);
							string sPath = fi.DirectoryName;

							string sFile = sSplit[0];

							if(sFile.Length > 0)
							{
								if(sFile[1] != ':') // no full path setting
								{
									sFile = Path.Combine(sPath, sFile);
								}
							}

							sFName = sFile;
							sTarget = sSplit[2];
						}

						ArrayList arrToc = null;
						if( (sFName.Length>0) && (sTarget.Length>0) )
						{
						// if this link points into the current file
							if( sFName.ToLower() == currentFile.ChmFilePath.ToLower() )
							{
								if(sTarget.ToLower().IndexOf(".hhc") >= 0)
								{
									string sfCheck = sTarget;

									// remove prefixing ./
									while( (sfCheck[0]=='.') || (sfCheck[0]=='/') )
									{
										sfCheck = sfCheck.Substring(1);
									}

									if( currentFile.ContentsFile.ToLower() != sfCheck )
									{
										arrToc = currentFile.ParseHHC( sTarget );

										if( arrToc.Count > 0)
										{
										}
									} 
									else 
									{
										arrToc = currentFile.TOC;
									}

									// target points to a complete TOC
									int nCnt = 0;

									foreach(TOCItem chkItem in arrToc)
									{
										if(nCnt == 0)
										{
											curItem.AssociatedFile = currentFile;
											curItem.Children = chkItem.Children;
											curItem.ChmFile = currentFile.ChmFilePath;
											curItem.ImageIndex = chkItem.ImageIndex;
											curItem.Local = chkItem.Local;
											curItem.MergeLink = chkItem.MergeLink;
											curItem.Name = chkItem.Name;
											curItem.TocMode = chkItem.TocMode;
											curItem.TopicOffset = chkItem.TopicOffset;

											MarkChildrenAdded(chkItem.Children, curFile.MergLinks);
										} 
										else 
										{
											ArrayList checkList = null;

											if(curItem.Parent != null)
												checkList = curItem.Parent.Children;
											else
												checkList = curFile.TOC;
											
											int nIdx = checkList.IndexOf(curItem);
											if((nIdx+nCnt)>checkList.Count)
												checkList.Add(chkItem);
											else
												checkList.Insert(nIdx+nCnt, chkItem);
											
											curFile.MergLinks.Add(chkItem);
											MarkChildrenAdded(chkItem.Children, curFile.MergLinks);
										}

										nCnt++;
									}
								} 
								else 
								{
								
									// target points to a single topic
									TOCItem chkItem = currentFile.GetTOCItemByLocal(sTarget);
									if(chkItem != null)
									{
										curItem.AssociatedFile = currentFile;
										curItem.Children = chkItem.Children;
										curItem.ChmFile = currentFile.ChmFilePath;
										curItem.ImageIndex = chkItem.ImageIndex;
										curItem.Local = chkItem.Local;
										curItem.MergeLink = chkItem.MergeLink;
										curItem.Name = chkItem.Name;
										curItem.TocMode = chkItem.TocMode;
										curItem.TopicOffset = chkItem.TopicOffset;

										curFile.MergLinks.Add(chkItem);
										MarkChildrenAdded(chkItem.Children, curFile.MergLinks);
									}
								}
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Adds sub-items of an TOC-entry to the merg-linked list. 
		/// This will mark this item as "added" during the extra merge run 
		/// of the HtmlHelpSystem class.
		/// </summary>
		/// <param name="tocs">TOCItem list</param>
		/// <param name="merged">Arraylist which holds the merged-items</param>
		internal void MarkChildrenAdded(ArrayList tocs, ArrayList merged)
		{
			foreach(TOCItem curItem in tocs)
			{
				if(!merged.Contains(curItem))
				{
					merged.Add(curItem);

					MarkChildrenAdded(curItem.Children, merged);
				}
			}
		}

		/// <summary>
		/// Removes merge-links from the toc of files which were not loaded
		/// </summary>
		internal void RemoveMergeLinks()
		{
			foreach(CHMFile curFile in _chmFiles)
			{
				if( curFile.MergLinks.Count > 0)
				{
					while(curFile.MergLinks.Count > 0)
					{
						TOCItem curItem = curFile.MergLinks[0] as TOCItem;
						if(curItem.MergeLink.Length > 0)
							curFile.RemoveTOCItem(curItem);

						curFile.MergLinks.RemoveAt(0);
					}
				}
			}
		}

		/// <summary>
		/// Merges the information types and categories read by the CHMFile instance 
		/// into the system instance
		/// </summary>
		/// <param name="chmFile">file instance</param>
		private void MergeFileInfoTypesCategories(CHMFile chmFile)
		{
			if(chmFile.HasInformationTypes)
			{
				for(int i=0; i<chmFile.InformationTypes.Count;i++)
				{
					InformationType curType = chmFile.InformationTypes[i] as InformationType;
					InformationType sysType = GetInformationType( curType.Name );

					if( sysType == null)
						_informationTypes.Add(curType);
					else
						curType.ReferenceCount++;
				}
			}

			if(chmFile.HasCategories)
			{
				for(int i=0; i<chmFile.Categories.Count;i++)
				{
					Category curCat = chmFile.Categories[i] as Category;
					Category sysCat = GetCategory( curCat.Name );

					if(sysCat == null)
						_categories.Add(curCat);
					else
						curCat.ReferenceCount++;
				}
			}
		}

		/// <summary>
		/// Removes the information types and categories read by the CHMFile instance 
		/// </summary>
		/// <param name="chmFile">file instance</param>
		private void RemoveFileInfoTypesCategories(CHMFile chmFile)
		{
			if(chmFile.HasInformationTypes)
			{
				for(int i=0; i<chmFile.InformationTypes.Count;i++)
				{
					InformationType curType = chmFile.InformationTypes[i] as InformationType;
					InformationType sysType = GetInformationType( curType.Name );

					if(sysType != null)
					{
						sysType.ReferenceCount--;

						if(sysType.ReferenceCount<=0)
							_informationTypes.Remove(sysType);
					}
				}
			}

			if(chmFile.HasCategories)
			{
				for(int i=0; i<chmFile.Categories.Count;i++)
				{
					Category curCat = chmFile.Categories[i] as Category;
					Category sysCat = GetCategory( curCat.Name );

					if(sysCat != null)
					{
						sysCat.ReferenceCount--;

						if(sysCat.ReferenceCount<=0)
							_categories.Remove(sysCat);
					}
				}
			}
		}

		/// <summary>
		/// Removes a chm file from the internal file collection
		/// </summary>
		/// <param name="chmFile">full file path of the chm file to remove</param>
		public void RemoveFile(string chmFile)
		{
			int nIdx = -1;
			CHMFile removeInstance=null;

			foreach(CHMFile curFile in _chmFiles)
			{
				nIdx++;

				if( curFile.ChmFilePath.ToLower() == chmFile.ToLower() )
				{
					removeInstance = curFile;
					break;
				}
			}

			if(nIdx >= 0)
			{
				_toc.Clear(); // forces a rebuild of the merged toc
				_index.Clear(); // force a rebuild of the merged index
				
				RemoveFileInfoTypesCategories(removeInstance);
				_chmFiles.RemoveAt(nIdx);
			}
		}

		/// <summary>
		/// Closes all files and destroys TOC/index
		/// </summary>
		public void CloseAllFiles()
		{
			for(int i=0; i < _chmFiles.Count; i++)
			{
				CHMFile curFile = _chmFiles[i] as CHMFile;

				_chmFiles.RemoveAt(i);
				curFile.Dispose();
				i--;
			}

			_chmFiles.Clear();
			_toc.Clear();
			_index.Clear();
			_informationTypes.Clear();
			_categories.Clear();
		}

		/// <summary>
		/// Gets an array of loaded chm files. 
		/// </summary>
		public CHMFile[] FileList
		{
			get
			{
				CHMFile[] ret = new CHMFile[ _chmFiles.Count ];
				for(int i=0;i<_chmFiles.Count;i++)
					ret[i] = (CHMFile)_chmFiles[i];

				return ret;
			}
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

		/// <summary>
		/// Gets the default topic
		/// </summary>
		public string DefaultTopic
		{
			get
			{
				if( _chmFiles.Count > 0 )
				{
					foreach(CHMFile curFile in _chmFiles)
					{
						if( curFile.DefaultTopic.Length > 0)
						{
							return curFile.FormURL( curFile.DefaultTopic );
						}
					}
				}

				return "about:blank";
			}
		}

		/// <summary>
		/// Gets a merged table of contents of all opened chm files
		/// </summary>
		public TableOfContents TableOfContents
		{
			get
			{
				if( _chmFiles.Count > 0 )
				{
					if( _toc.Count() <= 0)
					{
						// merge toc of files
						foreach(CHMFile curFile in _chmFiles)
						{
							_toc.MergeToC( curFile.TOC );
						}
					}
				}

				return _toc;
			}
		}

		/// <summary>
		/// Gets a merged index  of all opened chm files
		/// </summary>
		public Index Index
		{
			get
			{
				if( _chmFiles.Count > 0 )
				{
					if( (_index.Count(IndexType.KeywordLinks)+_index.Count(IndexType.AssiciativeLinks)) <= 0)
					{
						// merge index files
						foreach(CHMFile curFile in _chmFiles)
						{
							_index.MergeIndex( curFile.IndexKLinks, IndexType.KeywordLinks);
							_index.MergeIndex( curFile.IndexALinks, IndexType.AssiciativeLinks);
						}
					}
				}

				return _index;
			}
		}

		/// <summary>
		/// Gets a flag if the current instance offers a table of contents
		/// </summary>
		public bool HasTableOfContents
		{
			get 
			{
				return (TableOfContents.Count() > 0);
			}
		}

		/// <summary>
		/// Gets a flag if the current instance offers an index
		/// </summary>
		public bool HasIndex
		{
			get 
			{
				return (HasALinks || HasKLinks);
			}
		}

		/// <summary>
		/// Gets a flag if the index holds klinks
		/// </summary>
		public bool HasKLinks
		{
			get
			{
				return (_index.Count(IndexType.KeywordLinks) > 0);
			}
		}

		/// <summary>
		/// Gets a flag if the index holds alinks
		/// </summary>
		public bool HasALinks
		{
			get
			{
				return (_index.Count(IndexType.AssiciativeLinks) > 0);
			}
		}

		/// <summary>
		/// Gets a flag if the current instance supports fulltext searching
		/// </summary>
		public bool FullTextSearch
		{
			get 
			{
				bool bRet = false;

				foreach(CHMFile curFile in _chmFiles)
				{
					bRet |= curFile.FullTextSearch;
				}

				return bRet;
			}
		}

		/// <summary>
		/// Performs a full-text search over the chm files
		/// </summary>
		/// <param name="words">words to search</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if titles only</param>
		/// <returns>A DataTable containing the search hits</returns>
		public DataTable PerformSearch(string words, bool partialMatches, bool titleOnly)
		{
			return PerformSearch(words, -1, partialMatches, titleOnly);
		}

		/// <summary>
		/// Performs a full-text search over the chm files
		/// </summary>
		/// <param name="words">words to search</param>
		/// <param name="MaxResults">maximal number of hits to return</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if titles only</param>
		/// <returns>A DataTable containing the search hits</returns>
		public DataTable PerformSearch(string words, int MaxResults, bool partialMatches, bool titleOnly)
		{
			if( ! FullTextSearch )
				return null;

			DataTable dtResult = null;

			int nCnt = 0;

			foreach(CHMFile curFile in _chmFiles)
			{
				if(nCnt == 0)
				{
					if(curFile.FullTextSearchEngine.CanSearch)
					{
						if(curFile.FullTextSearchEngine.Search(words, MaxResults, partialMatches, titleOnly))
						{
							dtResult = curFile.FullTextSearchEngine.Hits;
							dtResult.DefaultView.Sort = "Rating DESC";
						}
					}
				}
				else
				{
					if(curFile.FullTextSearchEngine.CanSearch)
					{
						if(curFile.FullTextSearchEngine.Search(words, MaxResults, partialMatches, titleOnly))
						{
							DataTable table = curFile.FullTextSearchEngine.Hits;
							
							// append rows from 2nd file
							foreach(DataRow curRow in table.Rows)
							{
								dtResult.ImportRow( curRow );
							}

							dtResult.DefaultView.Sort = "Rating DESC";

							// adjust max hits
							if(MaxResults >= 0)
							{
								if(dtResult.DefaultView.Count > MaxResults)
								{
									for(int i=MaxResults-1; i<dtResult.DefaultView.Count-1;i++)
									{
										if(dtResult.DefaultView[i].Row.RowState != DataRowState.Deleted)
										{
											dtResult.DefaultView[i].Row.Delete();
											dtResult.DefaultView[i].Row.AcceptChanges();
											i--;
										}
									}

									dtResult.AcceptChanges();
									dtResult.DefaultView.Sort = "Rating DESC";
								}
							}
						}
					}
				}

				nCnt++;
			}

			return dtResult;
		}
	}
}
