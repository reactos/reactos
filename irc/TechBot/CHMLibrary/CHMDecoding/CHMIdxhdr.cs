using System;
using System.Collections;
using System.IO;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>CHMIdxhdr</c> implements t properties which have been read from the #IDXHDR file.
	/// </summary>
	internal sealed class CHMIdxhdr : IDisposable
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
		/// Internal member storing the number of topic nodes including the contents and index files
		/// </summary>
		private int _numberOfTopicNodes = 0;
		/// <summary>
		/// Internal member storing the offset in the #STRINGS file of the ImageList param of the "text/site properties" object of the sitemap contents
		/// </summary>
		private int _imageListOffset = 0;
		/// <summary>
		/// True if the value of the ImageType param of the "text/site properties" object of the sitemap contents is "Folder". 
		/// </summary>
		private bool _imageTypeFolder = false;
		/// <summary>
		/// Internal member storing the background value
		/// </summary>
		private int _background = 0;
		/// <summary>
		/// Internal member storing the foreground value
		/// </summary>
		private int _foreground = 0;
		/// <summary>
		/// Internal member storing the offset in the #STRINGS file of the Font param of the "text/site properties" object of the sitemap contents
		/// </summary>
		private int _fontOffset = 0;
		/// <summary>
		/// Internal member storing the offset in the #STRINGS file of the FrameName param of the "text/site properties" object of the sitemap contents
		/// </summary>
		private int _frameNameOffset = 0;
		/// <summary>
		/// Internal member storing the offset in the #STRINGS file of the WindowName param of the "text/site properties" object of the sitemap contents
		/// </summary>
		private int _windowNameOffset = 0;
		/// <summary>
		/// Internal member storing the number of merged files
		/// </summary>
		private int _numberOfMergedFiles = 0;
		/// <summary>
		/// Internal member storing the offset in the #STRINGS file of the merged file names
		/// </summary>
		private ArrayList _mergedFileOffsets = new ArrayList();
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the #IDXHDR file</param>
		/// <param name="associatedFile">associated CHMFile instance</param>
		public CHMIdxhdr(byte[] binaryFileData, CHMFile associatedFile)
		{
			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;
			DecodeData();
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

			int nTemp = 0;

			// 4 character T#SM
			binReader.ReadBytes(4);
			// unknown timestamp DWORD
			nTemp = binReader.ReadInt32();

			// unknown 1
			nTemp = binReader.ReadInt32();

			// number of topic nodes including the contents & index files
			_numberOfTopicNodes = binReader.ReadInt32();
			
			// unknown DWORD
			nTemp = binReader.ReadInt32();

			// offset in the strings file
			_imageListOffset = binReader.ReadInt32();
			if( _imageListOffset == 0)
				_imageListOffset = -1; // 0/-1 = none

			// unknown DWORD
			nTemp = binReader.ReadInt32();

			// 1 if the value of the ImageType param of the "text/site properties" object of the sitemap contents is "Folder". 
			nTemp = binReader.ReadInt32();
			_imageTypeFolder = (nTemp == 1);

			// offset in the strings file
			_background = binReader.ReadInt32();
			// offset in the strings file
			_foreground = binReader.ReadInt32();

			// offset in the strings file
			_fontOffset = binReader.ReadInt32();

			// window styles DWORD
			nTemp = binReader.ReadInt32();
			// window styles DWORD
			nTemp = binReader.ReadInt32();

			// unknown DWORD
			nTemp = binReader.ReadInt32();

			// offset in the strings file
			_frameNameOffset = binReader.ReadInt32();
			if( _frameNameOffset == 0)
				_frameNameOffset = -1; // 0/-1 = none
			// offset in the strings file
			_windowNameOffset = binReader.ReadInt32();
			if( _windowNameOffset == 0)
				_windowNameOffset = -1; // 0/-1 = none

			// informations types DWORD
			nTemp = binReader.ReadInt32();

			// unknown DWORD
			nTemp = binReader.ReadInt32();

			// number of merged files in the merged file list DWORD
			_numberOfMergedFiles = binReader.ReadInt32();

			nTemp = binReader.ReadInt32();

			for(int i = 0; i < _numberOfMergedFiles; i++)
			{
				// DWORD offset value of merged file
				nTemp = binReader.ReadInt32();
				
				if(nTemp > 0)
					_mergedFileOffsets.Add(nTemp);
			}

			return bRet;
		}

		/// <summary>
		/// Gets the number of topic nodes including the contents and index files
		/// </summary>
		public int NumberOfTopicNodes
		{
			get { return _numberOfTopicNodes; }
		}

		/// <summary>
		/// Gets the offset in the #STRINGS file of the ImageList 
		/// param of the "text/site properties" object of the sitemap contents
		/// </summary>
		public int ImageListOffset
		{
			get { return _imageListOffset; }
		}

		/// <summary>
		/// True if the value of the ImageType param of the 
		/// "text/site properties" object of the sitemap contents is "Folder". 
		/// </summary>
		/// <remarks>If this is set to true, the help will display folders instead of books</remarks>
		public bool ImageTypeFolder
		{
			get { return _imageTypeFolder; }
		}

		/// <summary>
		/// Gets the background setting 
		/// </summary>
		public int Background
		{
			get { return _background; }
		}

		/// <summary>
		/// Gets the foreground setting 
		/// </summary>
		public int Foreground
		{
			get { return _foreground; }
		}

		/// <summary>
		/// Gets the offset in the #STRINGS file of the Font 
		/// param of the "text/site properties" object of the sitemap contents
		/// </summary>
		public int WindowNameOffset
		{
			get { return _fontOffset; }
		}

		/// <summary>
		/// Gets the offset in the #STRINGS file of the FrameName 
		/// param of the "text/site properties" object of the sitemap contents
		/// </summary>
		public int FrameNameOffset
		{
			get { return _frameNameOffset; }
		}

		/// <summary>
		/// Gets the offset in the #STRINGS file of the WindowName 
		/// param of the "text/site properties" object of the sitemap contents
		/// </summary>
		public int FontOffset
		{
			get { return _windowNameOffset; }
		}

		/// <summary>
		/// Gets an array list of offset numbers in the #STRINGS file of the 
		/// merged file names.
		/// </summary>
		public ArrayList MergedFileOffsets
		{
			get { return _mergedFileOffsets; }
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
					_mergedFileOffsets = null;
				}
             
				          
			}
			disposed = true;         
		}
	}
}
