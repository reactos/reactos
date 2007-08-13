using System;
using System.Data;
using System.Diagnostics;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;
using System.Collections;
using System.Globalization;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>FullTextSearcher</c> implements a fulltext searcher for a single chm file !
	/// </summary>
	internal sealed class FullTextEngine : IDisposable
	{
		#region Internal helper classes
		/// <summary>
		/// Internal class for decoding the header
		/// </summary>
		private sealed class FTHeader
		{
			/// <summary>
			/// Internal member storing the number of indexed files
			/// </summary>
			private int _numberOfIndexFiles = 0;
			/// <summary>
			/// Internal member storing the offset of the root node
			/// </summary>
			private int _rootOffset = 0;
			/// <summary>
			/// Internal member storing the index-page count
			/// </summary>
			private int _pageCount = 0;
			/// <summary>
			/// Internal member storing the depth of the tree
			/// </summary>
			private int _depth = 0;
			/// <summary>
			/// Internal member storing the scale param for document index en-/decoding 
			/// </summary>
			private byte _scaleDocIdx = 0;
			/// <summary>
			/// Internal member storing the scale param for code-count en-/decoding 
			/// </summary>
			private byte _scaleCodeCnt = 0;
			/// <summary>
			/// Internal member storing the scale param for location codes en-/decoding 
			/// </summary>
			private byte _scaleLocCodes = 0;
			/// <summary>
			/// Internal member storing the root param for document index en-/decoding 
			/// </summary>
			private byte _rootDocIdx = 0;
			/// <summary>
			/// Internal member storing the root param for code-count en-/decoding 
			/// </summary>
			private byte _rootCodeCnt = 0;
			/// <summary>
			/// Internal member storing the root param for location codes en-/decoding 
			/// </summary>
			private byte _rootLocCodes = 0;
			/// <summary>
			/// Internal member storing the size of the nodes in bytes
			/// </summary>
			private int _nodeSize = 0;
			/// <summary>
			/// Internal member storing the length of the longest word
			/// </summary>
			private int _lengthOfLongestWord = 0;
			/// <summary>
			/// Internal member storing the total number of words
			/// </summary>
			private int _totalNumberOfWords = 0;
			/// <summary>
			/// Internal member storing the total number of unique words
			/// </summary>
			private int _numberOfUniqueWords = 0;
			/// <summary>
			/// Internal member storing the codepage identifier
			/// </summary>
			private int _codePage = 1252;
			/// <summary>
			/// Internal member storing the language code id
			/// </summary>
			private int _lcid = 1033;
			/// <summary>
			/// Internal member storing the text encoder
			/// </summary>
			private Encoding _textEncoder = Encoding.Default;

			/// <summary>
			/// Constructor of the header
			/// </summary>
			/// <param name="binaryData">binary data from which the header will be extracted</param>
			public FTHeader(byte[] binaryData)
			{
				DecodeHeader(binaryData);
			}

			/// <summary>
			/// Internal constructor for reading from dump
			/// </summary>
			internal FTHeader()
			{
			}

			/// <summary>
			/// Decodes the binary header information and fills the members
			/// </summary>
			/// <param name="binaryData">binary data from which the header will be extracted</param>
			private void DecodeHeader(byte[] binaryData)
			{
				MemoryStream memStream = new MemoryStream(binaryData);
				BinaryReader binReader = new BinaryReader(memStream);

				binReader.ReadBytes(4); // 4 unknown bytes
				
				_numberOfIndexFiles = binReader.ReadInt32();  // number of indexed files

				binReader.ReadInt32(); // unknown
				binReader.ReadInt32(); // unknown

				_pageCount = binReader.ReadInt32();  // page-count
				_rootOffset = binReader.ReadInt32(); // file offset of the root node
				_depth = binReader.ReadInt16(); // depth of the tree

				binReader.ReadInt32(); // unknown

				_scaleDocIdx = binReader.ReadByte();
				_rootDocIdx =  binReader.ReadByte();
				_scaleCodeCnt = binReader.ReadByte();
				_rootCodeCnt = binReader.ReadByte();
				_scaleLocCodes = binReader.ReadByte();
				_rootLocCodes = binReader.ReadByte();

				if( (_scaleDocIdx != 2) || ( _scaleCodeCnt != 2 ) || ( _scaleLocCodes != 2 ) )
				{
					Debug.WriteLine("Unsupported scale for s/r encoding !");
					throw new InvalidOperationException("Unsupported scale for s/r encoding !");
				}

				binReader.ReadBytes(10); // unknown

				_nodeSize = binReader.ReadInt32();

				binReader.ReadInt32(); // unknown
				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important

				_lengthOfLongestWord = binReader.ReadInt32();
				_totalNumberOfWords = binReader.ReadInt32();
				_numberOfUniqueWords = binReader.ReadInt32();

				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important
				binReader.ReadInt32(); // not important

				binReader.ReadBytes(24); // not important

				_codePage = binReader.ReadInt32();
				_lcid = binReader.ReadInt32();

				CultureInfo ci = new CultureInfo(_lcid);
				_textEncoder = Encoding.GetEncoding( ci.TextInfo.ANSICodePage );

				// rest of header is not important for us
			}

			/// <summary>
			/// Dump the class data to a binary writer
			/// </summary>
			/// <param name="writer">writer to write the data</param>
			internal void Dump(ref BinaryWriter writer)
			{
				writer.Write( _numberOfIndexFiles );
				writer.Write( _rootOffset );
				writer.Write( _pageCount );
				writer.Write( _depth );
				writer.Write( _scaleDocIdx );
				writer.Write( _rootDocIdx );
				writer.Write( _scaleCodeCnt );
				writer.Write( _rootCodeCnt );
				writer.Write( _scaleLocCodes );
				writer.Write( _rootLocCodes );
				writer.Write( _nodeSize );
				writer.Write( _lengthOfLongestWord );
				writer.Write( _totalNumberOfWords );
				writer.Write( _numberOfUniqueWords );
			}

			/// <summary>
			/// Reads the object data from a dump store
			/// </summary>
			/// <param name="reader">reader to read the data</param>
			internal void ReadDump(ref BinaryReader reader)
			{
				_numberOfIndexFiles = reader.ReadInt32();
				_rootOffset = reader.ReadInt32();
				_pageCount = reader.ReadInt32();
				_depth = reader.ReadInt32();
				
				_scaleDocIdx = reader.ReadByte();
				_rootDocIdx = reader.ReadByte();
				_scaleCodeCnt = reader.ReadByte();
				_rootCodeCnt = reader.ReadByte();
				_scaleLocCodes = reader.ReadByte();
				_rootLocCodes = reader.ReadByte();

				_nodeSize = reader.ReadInt32();
				_lengthOfLongestWord = reader.ReadInt32();
				_totalNumberOfWords = reader.ReadInt32();
				_numberOfUniqueWords = reader.ReadInt32();
			}

			/// <summary>
			/// Gets the number of indexed files
			/// </summary>
			public int IndexedFileCount
			{
				get { return _numberOfIndexFiles; }
			}

			/// <summary>
			/// Gets the file offset of the root node
			/// </summary>
			public int RootOffset
			{
				get { return _rootOffset; }
			}

			/// <summary>
			/// Gets the page count
			/// </summary>
			public int PageCount
			{
				get { return _pageCount; }
			}

			/// <summary>
			/// Gets the index depth
			/// </summary>
			public int Depth
			{
				get { return _depth; }
			}

			/// <summary>
			/// Gets the scale param for document index en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte ScaleDocumentIndex
			{
				get { return _scaleDocIdx; }
			}

			/// <summary>
			/// Gets the root param for the document index en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte RootDocumentIndex
			{
				get { return _rootDocIdx; }
			}

			/// <summary>
			/// Gets the scale param for the code-count en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte ScaleCodeCount
			{
				get { return _scaleCodeCnt; }
			}

			/// <summary>
			/// Gets the root param for the code-count en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte RootCodeCount
			{
				get { return _rootCodeCnt; }
			}

			/// <summary>
			/// Gets the scale param for the location codes en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte ScaleLocationCodes
			{
				get { return _scaleLocCodes; }
			}

			/// <summary>
			/// Gets the root param for the location codes en-/decoding
			/// </summary>
			/// <remarks>The scale and root method of integer encoding needs two parameters, 
			/// which I'll call s (scale) and r (root size).
			/// The integer is encoded as two parts, p (prefix) and q (actual bits). 
			/// p determines how many bits are stored, as well as implicitly determining 
			/// the high-order bit of the integer. </remarks>
			public byte RootLocationCodes
			{
				get { return _rootLocCodes; }
			}

			/// <summary>
			/// Gets the size in bytes of each index/leaf node
			/// </summary>
			public int NodeSize
			{
				get { return _nodeSize; }
			}

			/// <summary>
			/// Gets the length of the longest word in the index
			/// </summary>
			private int LengthOfLongestWord
			{
				get { return _lengthOfLongestWord; }
			}

			/// <summary>
			/// Gets the total number of words indexed (including duplicates)
			/// </summary>
			public int TotalWordCount
			{
				get { return _totalNumberOfWords; }
			}

			/// <summary>
			/// Gets the total number of unique words indexed (excluding duplicates)
			/// </summary>
			public int UniqueWordCount
			{
				get { return _numberOfUniqueWords; }
			}

			/// <summary>
			/// Gets the codepage identifier
			/// </summary>
			public int CodePage
			{
				get { return _codePage; }
			}

			/// <summary>
			/// Gets the language code id
			/// </summary>
			public int LCID
			{
				get { return _lcid; }
			}

			public Encoding TextEncoder
			{
				get 
				{ 
					return _textEncoder;
				}
			}
		}


		/// <summary>
		/// Internal class for easier hit recording and rate-calculation
		/// </summary>
		private sealed class HitHelper : IComparable
		{
			/// <summary>
			/// Internal member storing the associated document index
			/// </summary>
			private int _documentIndex = 0;
			/// <summary>
			/// Internal member storing the title
			/// </summary>
			private string _title = "";
			/// <summary>
			/// Internal member storing the locale
			/// </summary>
			private string _locale = "";
			/// <summary>
			/// Internal member storing the location
			/// </summary>
			private string _location = "";
			/// <summary>
			/// Internal member storing the url
			/// </summary>
			private string _url = "";
			/// <summary>
			/// Internal member storing the rating
			/// </summary>
			private double _rating = 0;
			/// <summary>
			/// Internal member used for rating calculation
			/// </summary>
			private Hashtable _partialRating = new Hashtable();

			/// <summary>
			/// Constructor of the class
			/// </summary>
			/// <param name="documentIndex">document index</param>
			/// <param name="title">title</param>
			/// <param name="locale">locale parameter</param>
			/// <param name="location">location</param>
			/// <param name="url">url of document</param>
			/// <param name="rating">rating</param>
			public HitHelper(int documentIndex, string title, string locale, string location, string url, double rating)
			{
				_documentIndex = documentIndex;
				_title = title;
				_locale = locale;
				_location = location;
				_url = url;
				_rating = rating;
			}

			/// <summary>
			/// Updates the rating for a found word
			/// </summary>
			/// <param name="word">word found</param>
			public void UpdateRating(string word)
			{
				if( _partialRating[word] == null)
				{
					_partialRating[word] = 100.0;
				}
				else 
				{
					_partialRating[word] = ((double)_partialRating[word])*1.01;
				}

				_rating = 0.0;

				foreach(double val in _partialRating.Values)
				{
					_rating += val;
				}
			}

			/// <summary>
			/// Implements the CompareTo method of the IComparable interface. 
			/// Allows an easy sort by the document rating
			/// </summary>
			/// <param name="obj">object to compare</param>
			/// <returns>0 ... equal, -1 ... this instance is less than obj, 1 ... this instance is greater than obj</returns>
			public int CompareTo(object obj)
			{
				if( obj is HitHelper )
				{
					HitHelper hObj = (HitHelper)obj;

					return this.Rating.CompareTo( hObj.Rating );
				}

				return -1;
			}

			/// <summary>
			/// Gets the internal hashtable used for counting word hits of the document
			/// </summary>
			internal Hashtable PartialRating
			{
				get { return _partialRating; }
			}

			/// <summary>
			/// Gets the document index of the hit helper instance
			/// </summary>
			public int DocumentIndex
			{
				get { return _documentIndex; }
			}

			/// <summary>
			/// Gets the title
			/// </summary>
			public string Title
			{
				get { return _title; }
			}

			/// <summary>
			/// Gets the locale
			/// </summary>
			public string Locale
			{
				get { return _locale; }
			}

			/// <summary>
			/// Gets the location
			/// </summary>
			public string Location
			{
				get { return _location; }
			}

			/// <summary>
			/// Gets the url
			/// </summary>
			public string URL
			{
				get { return _url; }
			}

			/// <summary>
			/// Gets the rating
			/// </summary>
			public double Rating
			{
				get { return _rating; }
			}

		}

		#endregion

		/// <summary>
		/// Regular expression getting the text between to quotes
		/// </summary>
		private string RE_Quotes = @"\""(?<innerText>.*?)\""";
		/// <summary>
		/// Internal flag specifying if the object is going to be disposed
		/// </summary>
		private bool disposed = false;
		/// <summary>
		/// Internal member storing the binary file data
		/// </summary>
		private byte[] _binaryFileData = null;
		/// <summary>
		/// Internal datatable storing the search hits
		/// </summary>
		private DataTable _hits =null;
		/// <summary>
		/// Internal arraylist for hit management
		/// </summary>
		private ArrayList _hitsHelper = new ArrayList();
		/// <summary>
		/// Internal member storing the header of the file
		/// </summary>
		private FTHeader _header = null;
		/// <summary>
		/// Internal member storing the associated chmfile object
		/// </summary>
		private CHMFile _associatedFile = null;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="binaryFileData">binary file data of the $FIftiMain file</param>
		/// <param name="associatedFile">associated chm file</param>
		public FullTextEngine(byte[] binaryFileData, CHMFile associatedFile)
		{
			_binaryFileData = binaryFileData;
			_associatedFile = associatedFile;

			if(_associatedFile.SystemFile.FullTextSearch)
			{
				_header = new FTHeader(_binaryFileData); // reading header
			}
		}

		/// <summary>
		/// Standard constructor
		/// </summary>
		internal FullTextEngine()
		{
		}

		#region Data dumping
		/// <summary>
		/// Dump the class data to a binary writer
		/// </summary>
		/// <param name="writer">writer to write the data</param>
		internal void Dump(ref BinaryWriter writer)
		{
			_header.Dump(ref writer);
			writer.Write( _binaryFileData.Length );
			writer.Write(_binaryFileData);
		}

		/// <summary>
		/// Reads the object data from a dump store
		/// </summary>
		/// <param name="reader">reader to read the data</param>
		internal void ReadDump(ref BinaryReader reader)
		{
			_header = new FTHeader();
			_header.ReadDump(ref reader);

			int nCnt = reader.ReadInt32();
			_binaryFileData = reader.ReadBytes(nCnt);
		}

		/// <summary>
		/// Sets the associated CHMFile instance
		/// </summary>
		/// <param name="associatedFile">instance to set</param>
		internal void SetCHMFile(CHMFile associatedFile)
		{
			_associatedFile = associatedFile;
		}
		#endregion

		/// <summary>
		/// Gets a flag if full-text searching is available for this chm file.
		/// </summary>
		public bool CanSearch
		{
			get { return (_associatedFile.SystemFile.FullTextSearch && (_header != null) ); }
		}

		/// <summary>
		/// Performs a fulltext search of a single file.
		/// </summary>
		/// <param name="search">word(s) or phrase to search</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if only search in titles</param>
		/// <remarks>Hits are available through the <see cref="Hits">Hists property</see>.</remarks>
		public bool Search(string search, bool partialMatches, bool titleOnly)
		{
			return	Search(search, -1, partialMatches, titleOnly);
		}

		/// <summary>
		/// Performs a fulltext search of a single file.
		/// </summary>
		/// <param name="search">word(s) or phrase to search</param>
		/// <param name="MaxHits">max hits. If this number is reached, the search will be interrupted</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if only search in titles</param>
		/// <remarks>Hits are available through the <see cref="Hits">Hists property</see>.</remarks>
		public bool Search(string search, int MaxHits, bool partialMatches, bool titleOnly)
		{
			if(CanSearch)
			{
				string searchString = search;

				// Check if this is a quoted string
				bool IsQuoted = (search.IndexOf("\"")>-1);

				if(IsQuoted)
					searchString = search.Replace("\"",""); // remove the quotes during search

				bool bRet = true;

				_hitsHelper = null;
				_hitsHelper = new ArrayList();

				_hits = null;
				CreateHitsTable();

				string[] words = searchString.Split(new char[] {' '});

				for(int i=0; i<words.Length; i++)
				{
					bRet &= SearchSingleWord(words[i], MaxHits, partialMatches, titleOnly);
					if(_hitsHelper.Count >= MaxHits)
						break;
				}

				if(bRet && IsQuoted)
				{
					FinalizeQuoted(search);
				}

				if(bRet)
				{
					_hitsHelper.Sort();

					int nhCount = MaxHits;

					if( MaxHits < 0)
					{
						nhCount = _hitsHelper.Count;
					}

					if( nhCount > _hitsHelper.Count )
						nhCount = _hitsHelper.Count;

					// create hits datatable
					for(int i=nhCount; i > 0; i--)
					{
						HitHelper curHlp = (HitHelper)(_hitsHelper[i-1]);

						DataRow newRow = _hits.NewRow();

						newRow["Rating"] = curHlp.Rating;
						newRow["Title"] = curHlp.Title;
						newRow["Locale"] = curHlp.Locale;
						newRow["Location"] = curHlp.Location;
						newRow["URL"] = curHlp.URL;

						_hits.Rows.Add( newRow );
					}
				}
				return bRet;
			}

			return false;
		}

		/// <summary>
		/// Gets rid of all search hits which doesn't match the quoted phrase
		/// </summary>
		/// <param name="search">full search string entered by the user</param>
		/// <remarks>Phrase search is not possible using the internal full-text index. We're just filtering all 
		/// documents which don't contain all words of the phrase.</remarks>
		private void FinalizeQuoted(string search)
		{
			Regex quoteRE = new Regex(RE_Quotes, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			int innerTextIdx = quoteRE.GroupNumberFromName("innerText");
			int nIndex = 0;

			// get all phrases
			while( quoteRE.IsMatch(search, nIndex) )
			{
				Match m = quoteRE.Match(search, nIndex);

				string phrase = m.Groups["innerText"].Value;

				string[] wordsInPhrase = phrase.Split( new char[] {' '} );
				int nCnt = _hitsHelper.Count;

				for(int i=0; i < _hitsHelper.Count; i++)
				{
					if( ! CheckHit( ((HitHelper)(_hitsHelper[i])), wordsInPhrase) )
						_hitsHelper.RemoveAt(i--);
				}

				nIndex = m.Index+m.Length;
			}
		}

		/// <summary>
		/// Eliminates all search hits where not all of the words have been found
		/// </summary>
		/// <param name="hit">hithelper instance to check</param>
		/// <param name="wordsInPhrase">word list</param>
		private bool CheckHit(HitHelper hit, string[] wordsInPhrase)
		{

			for(int i=0; i<wordsInPhrase.Length;i++)
			{
				if( (hit.PartialRating[wordsInPhrase[i]] == null) || (((double)(hit.PartialRating[wordsInPhrase[i]])) == 0.0) )
					return false;
			}
			return true;
		}

		/// <summary>
		/// Performs a search for a single word in the index
		/// </summary>
		/// <param name="word">word to search</param>
		/// <param name="MaxHits">maximal hits to return</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if only search in titles</param>
		/// <returns>Returns true if succeeded</returns>
		private bool SearchSingleWord(string word,int MaxHits, bool partialMatches, bool titleOnly)
		{
			string wordLower = word.ToLower();

			MemoryStream memStream = new MemoryStream(_binaryFileData);
			BinaryReader binReader = new BinaryReader(memStream);

			// seek to root node
			binReader.BaseStream.Seek( _header.RootOffset, SeekOrigin.Begin );

			if( _header.Depth > 2 )
			{
				// unsupported index depth
				Debug.WriteLine("FullTextSearcher.SearchSingleWord() - Failed with message: Unsupported index depth !");
				Debug.WriteLine("File: " + _associatedFile.ChmFilePath);
				Debug.WriteLine(" ");
				return false;
			}

			if( _header.Depth > 1 )
			{
				// seek to the right leaf node ( if depth == 1, we are at the leaf node)
				int freeSpace = binReader.ReadInt16();

				for(int i=0; i < _header.PageCount; ++i)
				{
					// exstract index entries
					int nWLength = (int)binReader.ReadByte();
					int nCPosition = (int)binReader.ReadByte();

					string sName = BinaryReaderHelp.ExtractString(ref binReader, nWLength-1, 0, true, _header.TextEncoder);

					int nLeafOffset = binReader.ReadInt32();
					binReader.ReadInt16(); // unknown
		
					if( sName.CompareTo(wordLower) >= 0)
					{
						// store current position
						long curPos = binReader.BaseStream.Position;

						// seek to leaf offset
						binReader.BaseStream.Seek( nLeafOffset, SeekOrigin.Begin );

						// read leafnode
						ReadLeafNode(ref binReader, word, MaxHits, partialMatches, titleOnly);

						// return to current position and continue reading index nodes
						binReader.BaseStream.Seek( curPos, SeekOrigin.Begin );
					}
				}
			}

			return true;
		}

		/// <summary>
		/// Reads a leaf node and extracts documents which holds the searched word
		/// </summary>
		/// <param name="binReader">reference to the reader</param>
		/// <param name="word">word to search</param>
		/// <param name="MaxHits">maximal hits to return</param>
		/// <param name="partialMatches">true if partial word should be matched also 
		/// ( if this is true a search of 'support' will match 'supports', otherwise not )</param>
		/// <param name="titleOnly">true if only search in titles</param>
		private void ReadLeafNode(ref BinaryReader binReader, string word, int MaxHits, bool partialMatches, bool titleOnly) 
		{
			int nNextPageOffset = binReader.ReadInt32();
			binReader.ReadInt16(); // unknown
			int lfreeSpace = binReader.ReadInt16();
			string curFullWord = "";
			bool bFound = false;
			string wordLower = word.ToLower();

			for(;;)
			{
				if(binReader.BaseStream.Position >= binReader.BaseStream.Length)
					break;

				int nWLength = (int)binReader.ReadByte();
				
				if(nWLength == 0)
					break;

				int nCPosition = (int)binReader.ReadByte();

				string sName = BinaryReaderHelp.ExtractString(ref binReader, nWLength-1, 0, true, _header.TextEncoder);

				int Context = (int)binReader.ReadByte(); // 0...body tag, 1...title tag, others unknown

				long nrOfWCL = BinaryReaderHelp.ReadENCINT(ref binReader);
				int wclOffset = binReader.ReadInt32();
				
				binReader.ReadInt16(); // unknown

				long bytesOfWCL = BinaryReaderHelp.ReadENCINT(ref binReader);

				if( nCPosition > 0)
				{
					curFullWord = CombineStrings(curFullWord, sName, nCPosition);
				} 
				else 
				{
					curFullWord = sName;
				}

				bFound = false;
				if(partialMatches)
					bFound = ( curFullWord.IndexOf(wordLower) >= 0 );
				else
					bFound = (curFullWord == wordLower);

				if( bFound )
				{
					if( (titleOnly && (Context==1)) || (!titleOnly) )
					{						
						// store actual offset
						long curPos = binReader.BaseStream.Position;

						// found the word, begin with WCL encoding
						binReader.BaseStream.Seek(wclOffset, SeekOrigin.Begin );

						byte[] wclBytes = binReader.ReadBytes((int)bytesOfWCL);

						DecodeWCL(wclBytes, MaxHits, word);

						// back and continue reading leafnodes
						binReader.BaseStream.Seek(curPos, SeekOrigin.Begin );
					}
				}
			}
		}

		/// <summary>
		/// Decodes the s/r encoded WordCodeList (=wcl) and creates hit entries
		/// </summary>
		/// <param name="wclBytes">wcl encoded byte array</param>
		/// <param name="MaxHits">maximal hits</param>
		/// <param name="word">the word to find</param>
		private void DecodeWCL(byte[] wclBytes,int MaxHits, string word)
		{
			byte[] wclBits = new byte[ wclBytes.Length*8 ];
			
			int nBitIdx=0;

			for(int i=0; i<wclBytes.Length; i++)
			{
				for(int j=0; j<8; j++)
				{
					wclBits[nBitIdx] = ((byte)(wclBytes[i] & ((byte)( (byte)0x1 << (7-j) )))) > (byte)0 ? (byte)1 : (byte)0;
					nBitIdx++;
				}
			}

			nBitIdx = 0;

			int nDocIdx = 0; // delta encoded

			while(nBitIdx < wclBits.Length)
			{
				nDocIdx += BinaryReaderHelp.ReadSRItem(wclBits, _header.ScaleDocumentIndex, _header.RootDocumentIndex, ref nBitIdx);
				int nCodeCnt = BinaryReaderHelp.ReadSRItem(wclBits, _header.ScaleCodeCount, _header.RootCodeCount, ref nBitIdx);
			
				int nWordLocation = 0; // delta encoded

				for(int locidx=0; locidx<nCodeCnt; locidx++)
				{
					 nWordLocation += BinaryReaderHelp.ReadSRItem(wclBits, _header.ScaleLocationCodes, _header.RootLocationCodes, ref nBitIdx);
				}
				// apply padding
				while( (nBitIdx % 8) != 0)
					nBitIdx++;

				// Record hit
				HitHelper hitObj = DocumentHit(nDocIdx);

				if(hitObj == null)
				{
					if(_hitsHelper.Count > MaxHits)
						return;

					hitObj = new HitHelper(nDocIdx, ((TopicEntry)(_associatedFile.TopicsFile.TopicTable[nDocIdx])).Title,
						((TopicEntry)(_associatedFile.TopicsFile.TopicTable[nDocIdx])).Locale, _associatedFile.CompileFile,
						((TopicEntry)(_associatedFile.TopicsFile.TopicTable[nDocIdx])).URL, 0.0);

					for(int k=0;k<nCodeCnt;k++)
						hitObj.UpdateRating(word);

					_hitsHelper.Add(hitObj);
				} 
				else 
				{
					for(int k=0;k<nCodeCnt;k++)
						hitObj.UpdateRating(word);
				}
			}
		}

		/// <summary>
		/// Combines a "master" word with a partial word.
		/// </summary>
		/// <param name="word">the master word</param>
		/// <param name="partial">the partial word</param>
		/// <param name="partialPosition">position to place the parial word</param>
		/// <returns>returns a combined string</returns>
		private string CombineStrings(string word, string partial, int partialPosition)
		{
			string sCombined = word;
			int i=0;

			for(i=0; i<partial.Length; i++)
			{
				if( (i+partialPosition) > (sCombined.Length-1) )
				{
					sCombined += partial[i];
				} 
				else 
				{
					StringBuilder sb = new StringBuilder(sCombined);

					sb.Replace( sCombined[partialPosition+i], partial[i], partialPosition+i, 1);
					sCombined = sb.ToString();
				}
			}

			if(! ((i+partialPosition) > (sCombined.Length-1)) )
			{
				sCombined = sCombined.Substring(0, partialPosition+partial.Length);
			}

			return sCombined;
		}

		/// <summary>
		/// Gets the HitHelper instance for a specific document index
		/// </summary>
		/// <param name="index">document index</param>
		/// <returns>The reference of the hithelper instance for this document index, otherwise null</returns>
		private HitHelper DocumentHit(int index)
		{
			foreach(HitHelper curObj in _hitsHelper)
			{
				if( curObj.DocumentIndex == index)
					return curObj;
			}

			return null;
		}

		/// <summary>
		/// Creates a DataTable for storing the hits
		/// </summary>
		private void CreateHitsTable()
		{
			_hits = new DataTable("FT_Search_Hits");

			DataColumn ftColumn;

			ftColumn = new DataColumn();
			ftColumn.DataType = System.Type.GetType("System.Double");
			ftColumn.ColumnName = "Rating";
			ftColumn.ReadOnly = false;
			ftColumn.Unique = false;

			_hits.Columns.Add(ftColumn);

			ftColumn = new DataColumn();
			ftColumn.DataType = System.Type.GetType("System.String");
			ftColumn.ColumnName = "Title";
			ftColumn.ReadOnly = false;
			ftColumn.Unique = false;

			_hits.Columns.Add(ftColumn);

			ftColumn = new DataColumn();
			ftColumn.DataType = System.Type.GetType("System.String");
			ftColumn.ColumnName = "Locale";
			ftColumn.ReadOnly = false;
			ftColumn.Unique = false;

			_hits.Columns.Add(ftColumn);

			ftColumn = new DataColumn();
			ftColumn.DataType = System.Type.GetType("System.String");
			ftColumn.ColumnName = "Location";
			ftColumn.ReadOnly = false;
			ftColumn.Unique = false;

			_hits.Columns.Add(ftColumn);

			ftColumn = new DataColumn();
			ftColumn.DataType = System.Type.GetType("System.String");
			ftColumn.ColumnName = "URL";
			ftColumn.ReadOnly = false;
			ftColumn.Unique = false;

			_hits.Columns.Add(ftColumn);
		}

		/// <summary>
		/// Gets an datatable containing the hits of the last search
		/// </summary>
		public DataTable Hits
		{
			get { return _hits; }
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
