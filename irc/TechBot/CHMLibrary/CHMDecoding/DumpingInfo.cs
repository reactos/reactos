using System;
using System.IO;
using System.Text;
using System.Diagnostics;
using System.Collections.Specialized;

using ICSharpCode.SharpZipLib;
using ICSharpCode.SharpZipLib.Zip;
using ICSharpCode.SharpZipLib.Zip.Compression;
using ICSharpCode.SharpZipLib.Zip.Compression.Streams;

using HtmlHelp;
// using HtmlHelp.Storage;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// Enumeration for specifying the dumping compression
	/// </summary>
	public enum DumpCompression
	{
		/// <summary>
		/// None - no data compression will be used. 
		/// Fastest but most memory intensive
		/// </summary>
		None = 0,
		/// <summary>
		/// Minimum - a minimum data compression will be used.
		/// Fast but not much data reduction
		/// </summary>
		Minimum = 1,
		/// <summary>
		/// Medium - a medium data compression will be used.
		/// Slower but medium data reduction
		/// </summary>
		Medium = 2,
		/// <summary>
		/// Maximum - a maximum data compression will be used.
		/// Slowest but maximum data reduction
		/// </summary>
		Maximum = 3
	}

	/// <summary>
	/// Flags which specify which data should be dumped
	/// </summary>
	[FlagsAttribute()]
	public enum DumpingFlags
	{
		/// <summary>
		/// DumpTextTOC - if this flag is set, text-based TOCs (sitemap format) will be dumped
		/// </summary>
		DumpTextTOC = 1,
		/// <summary>
		/// DumpBinaryTOC - if this flag is set, binary TOCs will be dumped
		/// </summary>
		DumpBinaryTOC = 2,
		/// <summary>
		/// DumpTextIndex - if this flag is set, the text-based index (sitemap format) will be dumped
		/// </summary>
		DumpTextIndex = 4,
		/// <summary>
		/// DumpBinaryIndex - if this flag is set, the binary index will be dumped
		/// </summary>
		DumpBinaryIndex = 8,
		/// <summary>
		/// DumpStrings - if this flag is set, the internal #STRINGS file will be dumped
		/// </summary>
		DumpStrings = 16,
		/// <summary>
		/// DumpUrlStr - if this flag is set, the internal #URLSTR file will be dumped
		/// </summary>
		DumpUrlStr = 32,
		/// <summary>
		/// DumpUrlTbl - if this flag is set, the internal #URLTBL file will be dumped
		/// </summary>
		DumpUrlTbl = 64,
		/// <summary>
		/// DumpTopics - if this flag is set, the internal #TOPICS file will be dumped
		/// </summary>
		DumpTopics = 128,
		/// <summary>
		/// DumpFullText - if this flag is set, the internal $FIftiMain file will be dumped
		/// </summary>
		DumpFullText = 256
	}

	/// <summary>
	/// The class <c>DumpingInfo</c> implements information properties for the CHMFile class 
	/// if and how data dumping should be used.
	/// </summary>
	public sealed class DumpingInfo
	{
		public bool m_bAllowSaveDump=true;

		private readonly static BitVector32.Section DumpFlags = BitVector32.CreateSection(512);

		private const string _dumpHeader = "HtmlHelpSystem dump file 1.0";

		private string _outputDir = ""; // emtpy string means, same directory as chm file
		private DumpCompression _compressionLevel = DumpCompression.Maximum;
		private CHMFile _chmFile = null;

		private DeflaterOutputStream _outputStream = null;
		private InflaterInputStream _inputStream = null;

		private BinaryWriter _writer = null;
		private BinaryReader _reader = null;

		private BitVector32 _flags;

		/// <summary>
		/// Constructor of the class
		/// </summary>
		/// <param name="flags">Combine flag values to specify which data should be dumped.</param>
		/// <param name="outputDir">output directory. emtpy string means, 
		/// same directory as chm file (only if destination = ExternalFile)</param>
		/// <param name="compressionLevel">compression which should be used</param>
		public DumpingInfo(DumpingFlags flags, string outputDir, DumpCompression compressionLevel)
		{
			_flags = new BitVector32(0);
			int i = _flags[DumpFlags];
			_flags[DumpFlags] = i | (int)flags;

			_outputDir = outputDir;
			_compressionLevel = compressionLevel;
		}

		/// <summary>
		/// Gets the flag if text-based TOCs will be written to the dumping file
		/// </summary>
		public bool DumpTextTOC
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpTextTOC) != 0); }
		}

		/// <summary>
		/// Gets the flag if binary TOCs will be written to the dumping file
		/// </summary>
		public bool DumpBinaryTOC
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpBinaryTOC) != 0); }
		}

		/// <summary>
		/// Gets the flag if the text-based index will be written to the dumping file
		/// </summary>
		public bool DumpTextIndex
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpTextIndex) != 0); }
		}

		/// <summary>
		/// Gets the flag if the binary index will be written to the dumping file
		/// </summary>
		public bool DumpBinaryIndex
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpBinaryIndex) != 0); }
		}

		/// <summary>
		/// Gets the flag if the #STRINGS file will be written to the dumping file
		/// </summary>
		public bool DumpStrings
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpStrings) != 0); }
		}

		/// <summary>
		/// Gets the flag if the #URLSTR file will be written to the dumping file
		/// </summary>
		public bool DumpUrlStr
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpUrlStr) != 0); }
		}

		/// <summary>
		/// Gets the flag if the #URLTBL file will be written to the dumping file
		/// </summary>
		public bool DumpUrlTbl
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpUrlTbl) != 0); }
		}

		/// <summary>
		/// Gets the flag if the #TOPICS file will be written to the dumping file
		/// </summary>
		public bool DumpTopics
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpTopics) != 0); }
		}

		/// <summary>
		/// Gets the flag if the $FIftiMain file will be written to the dumping file
		/// </summary>
		public bool DumpFullText
		{
			get { return ((_flags[DumpFlags] & (int)DumpingFlags.DumpFullText) != 0); }
		}

		/// <summary>
		/// Gets the dump output directory.
		/// </summary>
		/// <value>emtpy string means, same directory as chm file</value>
		/// <remarks>If Destination is set to DumpingOutput.InternalFile this property will be ignored</remarks>
		public string OutputDir
		{
			get { return _outputDir; }
		}

		/// <summary>
		/// The compression level used.
		/// </summary>
		public DumpCompression CompressionLevel
		{
			get { return _compressionLevel; }
		}

		/// <summary>
		/// Gets/Sets the CHMFile instance associated with this object
		/// </summary>
		internal CHMFile ChmFile
		{
			get { return _chmFile; }
			set { _chmFile = value; }
		}

		/// <summary>
		/// Translates the compression level to the deflater constants
		/// </summary>
		private int CompLvl
		{
			get
			{
				switch(CompressionLevel)
				{
					case DumpCompression.None: return Deflater.NO_COMPRESSION;
					case DumpCompression.Minimum: return Deflater.BEST_SPEED;
					case DumpCompression.Medium: return Deflater.DEFAULT_COMPRESSION;
					case DumpCompression.Maximum: return Deflater.BEST_COMPRESSION;
				}

				return Deflater.BEST_COMPRESSION;
			}
		}

		/// <summary>
		/// Checks if a dump exists
		/// </summary>
		internal bool DumpExists
		{
			get
			{
				if(_flags[DumpFlags] == 0)
					return false;

				// we have a reader or writer to the dump so it must exist
				if( (_reader != null) || (_writer != null) )
					return true;

				string sDmpFile = _chmFile.ChmFilePath;
				sDmpFile=sDmpFile.ToLower().Replace(".chm",".CHB");

				return File.Exists(sDmpFile);
			}
		}

		/// <summary>
		/// Gets a binary writer instance which allows you to write to the dump
		/// </summary>
		internal BinaryWriter Writer
		{
			get
			{
				if (m_bAllowSaveDump==false)
					return null;

				if(_flags[DumpFlags] == 0)
					throw new InvalidOperationException("Nothing to dump. No flags have been set !");

				if(_reader != null)
					throw new InvalidOperationException("Can't write and read at the same time !");

				if(_chmFile == null)
					throw new InvalidOperationException("Only usable with an associated CHMFile instance !");

				if(_writer==null)
				{
					string sDmpFile = _chmFile.ChmFilePath;
					sDmpFile=sDmpFile.ToLower().Replace(".chm",".CHB");
					StreamWriter stream = new StreamWriter(sDmpFile, false, _chmFile.TextEncoding);

					// write header info uncompressed
					BinaryWriter _hwriter = new BinaryWriter(stream.BaseStream);
					_hwriter.Write(_dumpHeader);
					_hwriter.Write((int)CompressionLevel);

					if(_compressionLevel == DumpCompression.None)
					{
						_writer = new BinaryWriter(stream.BaseStream);
					} 
					else 
					{
						_outputStream = new DeflaterOutputStream(stream.BaseStream, new Deflater(CompLvl));

						_writer = new BinaryWriter(_outputStream);
					}
				}

				return _writer;

			}
		}

		/// <summary>
		/// Gets a binary reader which allows you to read from the dump
		/// </summary>
		internal BinaryReader Reader
		{
			get
			{
				if(_writer != null)
					throw new InvalidOperationException("Can't write and read at the same time !");

				if(_chmFile == null)
					throw new InvalidOperationException("Only usable with an associated CHMFile instance !");

				if(_reader==null)
				{
					string sDmpFile = _chmFile.ChmFilePath;
					sDmpFile=sDmpFile.ToLower().Replace(".chm",".CHB");
					StreamReader stream = new StreamReader(sDmpFile, _chmFile.TextEncoding);

					BinaryReader _hReader = new BinaryReader(stream.BaseStream);
					string sH = _hReader.ReadString();

					if(sH != _dumpHeader)
					{
						_hReader.Close();
						Debug.WriteLine("Unexpected dump-file header !");
						throw new FormatException("DumpingInfo.Reader - Unexpected dump-file header !");
					}

					_compressionLevel = (DumpCompression)_hReader.ReadInt32();
//					if(_compressionLevel != (DumpCompression)_hReader.ReadInt32())
//					{
//						_hReader.Close();
//						return null;
//					}

					if(_compressionLevel == DumpCompression.None)
					{
						_reader = new BinaryReader(stream.BaseStream);
					} 
					else 
					{
						_inputStream = new InflaterInputStream(stream.BaseStream, new Inflater());

						_reader = new BinaryReader(_inputStream);
					}
				}

				return _reader;
			}
		}
			
		/// <summary>
		/// Saves data and closes the dump
		/// </summary>
		/// <returns>true if succeed</returns>
		internal bool SaveData()
		{
			if (m_bAllowSaveDump==false)
				return true;

			if(_writer != null)
			{
				if(_writer!=null)
					_writer.Close();
				_outputStream = null;
				_writer = null;
			}

			if(_reader != null)
			{
				if(_reader!=null)
					_reader.Close();
				_inputStream = null;
				_reader = null;
			}

			return true;
		}
	}
}
