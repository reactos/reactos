using System;
using System.Diagnostics;
using System.Collections;
using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// Enumeration for specifying the index type
	/// </summary>
	public enum IndexType
	{
		/// <summary>
		/// Keyword links should be used
		/// </summary>
		KeywordLinks = 0,
		/// <summary>
		/// Associative links should be used
		/// </summary>
		AssiciativeLinks = 1
	}

	/// <summary>
	/// The class <c>Index</c> holds the (keyword links) KLinks and (associative links) ALinks of the htmlhelp 
	/// system. It implements methods for easy index-based searching.
	/// </summary>
	public class Index
	{
		private ArrayList _kLinks = new ArrayList();
		private ArrayList _aLinks = new ArrayList();

		/// <summary>
		/// Standard constructor
		/// </summary>
		public Index()
		{
		}

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="kLinks">arraylist with keyword links</param>
		/// <param name="aLinks">arraylist with associative links</param>
		public Index(ArrayList kLinks, ArrayList aLinks)
		{
			_kLinks= kLinks;
			_aLinks = aLinks;
		}

		/// <summary>
		/// Clears the current toc
		/// </summary>
		public void Clear()
		{
			if(_aLinks != null)
				_aLinks.Clear();
			if(_kLinks != null)
				_kLinks.Clear();
		}

		/// <summary>
		/// Gets the number of index items for a specific type
		/// </summary>
		/// <param name="typeOfIndex">type of index</param>
		/// <returns>Returns the number of index items for a specific type</returns>
		public int Count(IndexType typeOfIndex)
		{
			ArrayList _index = null;

			switch( typeOfIndex )
			{
				case IndexType.AssiciativeLinks: _index = _aLinks; break;
				case IndexType.KeywordLinks: _index = _kLinks; break;
			}

			if(_index != null)
				return _index.Count;

			return 0;
		}

		/// <summary>
		/// Gets the internal index list of keyword links
		/// </summary>
		public ArrayList KLinks
		{
			get 
			{
				if(_kLinks==null)
					_kLinks = new ArrayList();

				return _kLinks;
			}
		}

		/// <summary>
		/// Gets the internal index list of associative links
		/// </summary>
		public ArrayList ALinks
		{
			get 
			{
				if(_aLinks==null)
					_aLinks = new ArrayList();

				return _aLinks;
			}
		}

		/// <summary>
		/// Merges the the index list <c>arrIndex</c> into the current one
		/// </summary>
		/// <param name="arrIndex">indexlist which should be merged with the current one</param>
		/// <param name="typeOfIndex">type of index to merge</param>
		public void MergeIndex( ArrayList arrIndex, IndexType typeOfIndex )
		{
			ArrayList _index = null;

			switch(typeOfIndex)
			{
				case IndexType.AssiciativeLinks: _index = _aLinks;break;
				case IndexType.KeywordLinks: _index = _kLinks;break;
			}
			
			foreach(IndexItem curItem in arrIndex)
			{
				//IndexItem searchItem = ContainsIndex(_index, curItem.KeyWordPath);
				int insertIndex=0;
				IndexItem searchItem = BinSearch(0, _index.Count-1, _index, curItem.KeyWordPath, false, false, ref insertIndex);

				if(searchItem != null)
				{
					// extend the keywords topics
					foreach(IndexTopic curEntry in curItem.Topics)
					{
						searchItem.Topics.Add( curEntry );
					}
				} 
				else 
				{
					// add the item to the global collection
					//_index.Add( curItem );

					if(insertIndex > _index.Count)
						_index.Add(curItem);
					else
					_index.Insert(insertIndex, curItem);
				}
			}			
		}

		/// <summary>
		/// Searches an index entry using recursive binary search algo (divide and conquer).
		/// </summary>
		/// <param name="nStart">start index for searching</param>
		/// <param name="nEnd">end index for searching</param>
		/// <param name="arrIndex">arraylist containing sorted IndexItem entries</param>
		/// <param name="keywordPath">keyword path to search</param>
		/// <param name="searchKeyword">true if the keywordPath will only contain the keyword not the complete path</param>
		/// <param name="caseInsensitive">True if case should be ignored</param>
		/// <param name="insertIndex">out reference. will receive the index where the item with the
		/// keywordPath should be inserted if not found (receives -1 if the item was found)</param>
		/// <returns>Returns an IndexItem instance if found, otherwise null 
		/// (use insertIndex for inserting the new item in a sorted order)</returns>
		private IndexItem BinSearch(int nStart, int nEnd, ArrayList arrIndex, string keywordPath, 
			bool searchKeyword, bool caseInsensitive, ref int insertIndex)
		{
			if( arrIndex.Count <= 0 )
			{
				insertIndex=0;
				return null;
			}

			if(caseInsensitive)
				keywordPath = keywordPath.ToLower();

			if( (nEnd - nStart) > 1)
			{
				int nCheck = nStart + (nEnd-nStart)/2;

				IndexItem iC = arrIndex[nCheck] as IndexItem;

				string sCompare = iC.KeyWordPath;

				if(searchKeyword)
					sCompare = iC.KeyWord;

				if(caseInsensitive)
					sCompare = sCompare.ToLower();

				if( sCompare == keywordPath )
				{
					insertIndex=-1;
					return iC;
				}

				if( keywordPath.CompareTo(sCompare) < 0 )
				{
					return BinSearch(nStart, nCheck-1, arrIndex, keywordPath, searchKeyword, caseInsensitive, ref insertIndex);
				}

				if( keywordPath.CompareTo(sCompare) > 0 )
				{
					return BinSearch(nCheck+1, nEnd, arrIndex, keywordPath, searchKeyword, caseInsensitive, ref insertIndex);
				}
			}
			else if(nEnd-nStart == 1)
			{
				IndexItem i1 = arrIndex[nStart] as IndexItem;
				IndexItem i2 = arrIndex[nEnd] as IndexItem;

				string sCompare1 = i1.KeyWordPath;

				if(searchKeyword)
					sCompare1 = i1.KeyWord;

				if(caseInsensitive)
					sCompare1 = sCompare1.ToLower();

				string sCompare2 = i2.KeyWordPath;

				if(searchKeyword)
					sCompare2 = i2.KeyWord;

				if(caseInsensitive)
					sCompare2 = sCompare2.ToLower();

				if( sCompare1 == keywordPath)
				{
					insertIndex = -1;
					return i1;
				}

				if( sCompare2 == keywordPath)
				{
					insertIndex = -1;
					return i2;
				}

				if( sCompare1.CompareTo(keywordPath) > 0)
				{
					insertIndex = nStart;
					return null;
				} 
				else if( sCompare2.CompareTo(keywordPath) > 0)
				{
					insertIndex = nEnd;
					return null;
				} 
				else 
				{
					insertIndex = nEnd+1;
				}
			}

			IndexItem itm = arrIndex[nEnd] as IndexItem;

			string sCompareI = itm.KeyWordPath;

			if(searchKeyword)
				sCompareI = itm.KeyWord;

			if(caseInsensitive)
				sCompareI = sCompareI.ToLower();

			if( sCompareI.CompareTo(keywordPath) > 0)
			{
				insertIndex = nStart;
				return null;
			} 
			else if( sCompareI.CompareTo(keywordPath) < 0)
			{
				insertIndex = nEnd+1;
				return null;
			}
			else
			{
				insertIndex = -1;
				return arrIndex[nEnd] as IndexItem;
			}
		}

		/// <summary>
		/// Checks if a keyword exists in a index collection
		/// </summary>
		/// <param name="arrIndex">index to search (arraylist of IndexItems)</param>
		/// <param name="keywordPath">keywordpath to search</param>
		/// <returns>Returns the found IndexItem, otherwise null</returns>
		private IndexItem ContainsIndex(ArrayList arrIndex, string keywordPath)
		{
			foreach(IndexItem curItem in arrIndex)
			{
				if(curItem.KeyWordPath == keywordPath)
					return curItem;
			}

			return null;
		}

		/// <summary>
		/// Searches the alinks- or klinks-index for a specific keyword/associative
		/// </summary>
		/// <param name="search">keyword/associative to search</param>
		/// <param name="typeOfIndex">type of index to search</param>
		/// <returns>Returns an ArrayList which contains IndexTopic items or null if nothing was found</returns>
		public IndexItem SearchIndex(string search, IndexType typeOfIndex)
		{
			ArrayList _index = null;

			switch( typeOfIndex )
			{
				case IndexType.AssiciativeLinks: _index = _aLinks;break;
				case IndexType.KeywordLinks: _index = _kLinks;break;
			}

			int insertIdx=0;
			IndexItem foundItem = BinSearch(0, _index.Count, _index, search, true, true, ref insertIdx);

			return foundItem;
		}
	}
}
