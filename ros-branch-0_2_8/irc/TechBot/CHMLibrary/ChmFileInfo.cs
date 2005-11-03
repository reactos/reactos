using System;
using System.Collections;
using System.Text;
using System.IO;
using System.Globalization;
using System.Diagnostics;
using System.ComponentModel;

using HtmlHelp.ChmDecoding;
// using HtmlHelp.Storage;

namespace HtmlHelp
{
	/// <summary>
	/// The class <c>ChmFileInfo</c> only extracts system information from a CHM file. 
	/// It doesn't build the index and table of contents.
	/// </summary>
	public class ChmFileInfo
	{
		/// <summary>
		/// Internal member storing the full filename
		/// </summary>
		private string _chmFileName = "";
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor for extrating the file information of the provided file. 
		/// The constructor opens the chm-file and reads its system data.
		/// </summary>
		/// <param name="chmFile">full file name which information should be extracted</param>
		public ChmFileInfo(string chmFile)
		{
			if(!File.Exists(chmFile))
				throw new ArgumentException("Chm file must exist on disk !", "chmFileName");

			if( ! chmFile.ToLower().EndsWith(".chm") )
				throw new ArgumentException("HtmlHelp file must have the extension .chm !", "chmFile");

			_chmFileName = chmFile;
			_associatedFile = new CHMFile(null, chmFile, true); // only load system data of chm
		}

		/// <summary>
		/// Internal constructor used in the class <see cref="HtmlHelp.ChmDecoding.CHMFile">CHMFile</see>.
		/// </summary>
		/// <param name="associatedFile">associated chm file</param>
		internal ChmFileInfo(CHMFile associatedFile)
		{
			_associatedFile = associatedFile;

			if( _associatedFile == null)
				throw new ArgumentException("Associated CHMFile instance must not be null !", "associatedFile");
		}

		#region default info properties
		/// <summary>
		/// Gets the full filename of the chm file
		/// </summary>		
		public string ChmFileName
		{
			get 
			{ 
				return _associatedFile.ChmFilePath; 
			}
		}

		/// <summary>
		/// Gets a <see cref="System.IO.FileInfo">FileInfo</see> instance for the chm file.
		/// </summary>
		public FileInfo FileInfo
		{
			get { return new FileInfo(_associatedFile.ChmFilePath); }
		}
		#endregion

		#region #SYSTEM properties
		/// <summary>
		/// Gets the file version of the chm file. 
		/// 2 for Compatibility=1.0,  3 for Compatibility=1.1
		/// </summary>		
		public int FileVersion
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.FileVersion;
					
				return 0; 
			}
		}

		/// <summary>
		/// Gets the contents file name
		/// </summary>
	
		public string ContentsFile
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.ContentsFile;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the index file name
		/// </summary>
	
		public string IndexFile
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.IndexFile;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the default help topic
		/// </summary>
	
		public string DefaultTopic
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.DefaultTopic;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the title of the help window
		/// </summary>
	
		public string HelpWindowTitle
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.HelpWindowTitle;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the flag if DBCS is in use
		/// </summary>
	
		public bool DBCS
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.DBCS;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the flag if full-text-search is available
		/// </summary>
	
		public bool FullTextSearch
		{
			get 
			{
				if(_associatedFile != null)
					return _associatedFile.FullTextSearch;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the flag if the file has ALinks
		/// </summary>
	
		public bool HasALinks
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.HasALinks;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the flag if the file has KLinks
		/// </summary>
	
		public bool HasKLinks
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.HasKLinks;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the default window name
		/// </summary>
	
		public string DefaultWindow
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.DefaultWindow;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the file name of the compile file
		/// </summary>
	
		public string CompileFile
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.CompileFile;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
	
		public bool BinaryIndex
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.BinaryIndex;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary index file
		/// </summary>
	
		public string CompilerVersion
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.CompilerVersion;
					
				return ""; 
			}
		}

		/// <summary>
		/// Gets the flag if the chm has a binary toc file
		/// </summary>
	
		public bool BinaryTOC
		{
			get 
			{ 
				if(_associatedFile != null)
					return _associatedFile.BinaryTOC;
					
				return false; 
			}
		}

		/// <summary>
		/// Gets the font face of the read font property.
		/// Empty string for default font.
		/// </summary>
	
		public string FontFace
		{
			get
			{
				if(_associatedFile != null)
					return _associatedFile.FontFace;
					
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
				if(_associatedFile != null) 
					return _associatedFile.FontSize;
					
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
				if(_associatedFile != null) 
					return _associatedFile.CharacterSet;
					
				return 1; 
			}
		}

		/// <summary>
		/// Gets the codepage depending on the read font property
		/// </summary>
	
		public int CodePage
		{
			get
			{
				if(_associatedFile != null) 
					return _associatedFile.CodePage;
					
				return 0; 
			}
		}

		/// <summary>
		/// Gets the assiciated culture info
		/// </summary>		
		public CultureInfo Culture
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.Culture;
					
				return CultureInfo.CurrentCulture; 
			}
		}
		#endregion

		#region #IDXHDR properties
		/// <summary>
		/// Gets the number of topic nodes including the contents and index files
		/// </summary>
	
		public int NumberOfTopicNodes
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.NumberOfTopicNodes;
				
				return 0;
			}
		}

		/// <summary>
		/// Gets the ImageList string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
	
		public string ImageList
		{
			get 
			{
				if(_associatedFile != null) 
					return _associatedFile.ImageList;

				return "";
			}
		}

		/// <summary>
		/// Gets the background setting 
		/// </summary>
	
		public int Background
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.Background;

				return 0;
			}
		}

		/// <summary>
		/// Gets the foreground setting 
		/// </summary>
	
		public int Foreground
		{
			get 
			{ 
				if(_associatedFile != null) 
					return _associatedFile.Foreground;

				return 0;
			}
		}

		/// <summary>
		/// Gets the FrameName string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
	
		public string FrameName
		{
			get 
			{
				if(_associatedFile != null) 
					return _associatedFile.FrameName;

				return "";
			}
		}

		/// <summary>
		/// Gets the WindowName string specyfied in the #IDXHDR file.
		/// </summary>
		/// <remarks>This property uses the #STRINGS file to extract the string at a given offset.</remarks>
	
		public string WindowName
		{
			get 
			{
				if(_associatedFile != null) 
					return _associatedFile.WindowName;

				return "";
			}
		}

		/// <summary>
		/// Gets a string array containing the merged file names
		/// </summary>
		public string[] MergedFiles
		{
			get
			{
				if(_associatedFile != null) 
					return _associatedFile.MergedFiles;

				return new string[0];
			}
		}

		#endregion
	}
}
