using System;
using System.Diagnostics;
using System.Collections;
using HtmlHelp.ChmDecoding;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>TableOfContents</c> holds the TOC of the htmlhelp system class.
	/// </summary>
	public class TableOfContents
	{
		private ArrayList _toc = new ArrayList();

		/// <summary>
		/// Standard constructor
		/// </summary>
		public TableOfContents()
		{
		}

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="toc"></param>
		public TableOfContents(ArrayList toc)
		{
			_toc = toc;
		}

		/// <summary>
		/// Gets the internal stored table of contents
		/// </summary>
		public ArrayList TOC
		{
			get { return _toc; }
		}

		/// <summary>
		/// Clears the current toc
		/// </summary>
		public void Clear()
		{
			if(_toc!=null)
				_toc.Clear();
		}

		/// <summary>
		/// Gets the number of topics in the toc
		/// </summary>
		/// <returns>Returns the number of topics in the toc</returns>
		public int Count()
		{
			if(_toc!=null)
				return _toc.Count;
			else
				return 0;
		}

		/// <summary>
		/// Merges the <c>arrToC</c> list to the one in this instance
		/// </summary>
		/// <param name="arrToC">the toc list which should be merged with the current one</param>
		internal void MergeToC( ArrayList arrToC )
		{
			if(_toc==null)
				_toc = new ArrayList();

			MergeToC(_toc, arrToC, null);
		}

		/// <summary>
		/// Merges the <c>arrToC</c> list to the one in this instance (called if merged files
		/// were found in a CHM)
		/// </summary>
		/// <param name="arrToC">the toc list which should be merged with the current one</param>
		/// <param name="openFiles">An arraylist of CHMFile instances.</param>
		internal void MergeToC( ArrayList arrToC, ArrayList openFiles )
		{
			if(_toc==null)
				_toc = new ArrayList();
			MergeToC(_toc, arrToC, openFiles);
		}

		/// <summary>
		/// Internal method for recursive toc merging
		/// </summary>
		/// <param name="globalLevel">level of global toc</param>
		/// <param name="localLevel">level of local toc</param>
		/// <param name="openFiles">An arraylist of CHMFile instances.</param>
		private void MergeToC( ArrayList globalLevel, ArrayList localLevel, ArrayList openFiles )
		{
			foreach( TOCItem curItem in localLevel)
			{
				// if it is a part of the merged-links, we have to do nothing, 
				// because the method HtmlHelpSystem.RecalculateMergeLinks() has already
				// placed this item at its correct position.
				if(!IsMergedItem(curItem.Name, curItem.Local, openFiles))
				{
					TOCItem globalItem = ContainsToC(globalLevel, curItem.Name);
					if(globalItem == null)
					{
						// the global toc doesn't have a topic with this name
						// so we need to add the complete toc node to the global toc

						globalLevel.Add( curItem );
					} 
					else 
					{
						// the global toc contains the current topic
						// advance to the next level

						if( (globalItem.Local.Length <= 0) && (curItem.Local.Length > 0) )
						{
							// set the associated url
							globalItem.Local = curItem.Local;
							globalItem.ChmFile = curItem.ChmFile;
						}

						MergeToC(globalItem.Children, curItem.Children);
					}
				}
			}
		}

		/// <summary>
		/// Checks if the item is part of the merged-links
		/// </summary>
		/// <param name="name">name of the topic</param>
		/// <param name="local">local of the topic</param>
		/// <param name="openFiles">An arraylist of CHMFile instances.</param>
		/// <returns>Returns true if this item is part of the merged-links</returns>
		private bool IsMergedItem(string name, string local, ArrayList openFiles)
		{
			if(openFiles==null)
				return false;

			foreach(CHMFile curFile in openFiles)
			{
				foreach(TOCItem curItem in curFile.MergLinks)
					if( (curItem.Name == name) && (curItem.Local == local) )
						return true;
			}
			return false;
		}

		/// <summary>
		/// Checks if a topicname exists in a SINGLE toc level 
		/// </summary>
		/// <param name="arrToC">toc list</param>
		/// <param name="Topic">topic to search</param>
		/// <returns>Returns the topic item if found, otherwise null</returns>
		private TOCItem ContainsToC(ArrayList arrToC, string Topic)
		{
			foreach(TOCItem curItem in arrToC)
			{
				if(curItem.Name == Topic)
					return curItem;
			}

			return null;
		}
		
		/// <summary>
		/// Searches the table of contents for a special topic
		/// </summary>
		/// <param name="topic">topic to search</param>
		/// <returns>Returns an instance of TOCItem if found, otherwise null</returns>
		public TOCItem SearchTopic(string topic)
		{
			return SearchTopic(topic, _toc);
		}

		/// <summary>
		/// Internal recursive tree search
		/// </summary>
		/// <param name="topic">topic to search</param>
		/// <param name="searchIn">tree level list to look in</param>
		/// <returns>Returns an instance of TOCItem if found, otherwise null</returns>
		private TOCItem SearchTopic(string topic, ArrayList searchIn)
		{
			foreach(TOCItem curItem in searchIn)
			{
				if(curItem.Name.ToLower() == topic.ToLower() )
					return curItem;

				if(curItem.Children.Count>0)
				{
					TOCItem nf = SearchTopic(topic, curItem.Children);
					if(nf != null)
						return nf;
				}
			}

			return null;
		}
	}
}
