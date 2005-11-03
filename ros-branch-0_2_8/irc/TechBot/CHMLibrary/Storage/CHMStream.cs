using System;
using System.Diagnostics;
using System.Text;
using System.Data;
using System.Text.RegularExpressions;
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Runtime.InteropServices;

namespace CHMStream
{
	/// <summary>
	/// Summary description for CHMFile.
	/// </summary>
	///
	public class CHMStream : IDisposable
	{
		public MemoryStream OpenStream(chmUnitInfo Info)
		{
			if (Info==null)
				return null;

			MemoryStream st=new MemoryStream();
			this.ExtractFile(Info,st);
			return st;
		}

		public MemoryStream OpenStream(string FileName)
		{
			chmUnitInfo info=this.GetFileInfo(FileName);
			if (info==null)
				return null;
			return OpenStream(info);
		}

		private string m_CHMFileName;
		public string CHMFileName
		{
			get
			{
				return m_CHMFileName;
			}
		}

		public CHMStream()
		{
		}

		public CHMStream(string CHMFileName)
		{
			OpenCHM(CHMFileName);
		}

		public void OpenCHM(string CHMFileName)
		{			
			m_CHMFileName=CHMFileName;
			FileInfo fi=new FileInfo(m_CHMFileName);
			Dir=fi.DirectoryName;
			m_CHMName=fi.Name;
			fi=null;			
			chm_open(m_CHMFileName);
		}

		private bool m_bCHMLoaded=false;
		public bool CHMLoaded
		{
			get
			{
				return m_bCHMLoaded;
			}
		}

		private string m_CHMName="";
		public string CHMName
		{
			get 
			{
				return m_CHMName;
			}
		}

		private string Dir="";
		private string m_FileFind="";
		private string m_FileFindLastPart="";
		private chmUnitInfo m_FileInfo=null;
		private int m_FileCount=0;
		public void FindFile(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{
			string LocalFile=Info.path;
			LocalFile=LocalFile.Replace("/",@"\");
			if (!LocalFile.StartsWith(@"\"))
				LocalFile=@"\"+LocalFile;
			LocalFile=LocalFile.ToLower();
							
			if (m_FileFind.Length<=LocalFile.Length)
			{				
				if (LocalFile.IndexOf(m_FileFind)==LocalFile.Length-m_FileFind.Length)
				{
					Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_SUCCESS;
					m_FileInfo=Info;
					return;
				}			
			}
			Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
		}

		public void FileCount(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{
			m_FileCount++;
			Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
		}

		private ArrayList m_FileList=null;
		private string m_strByExt="";
		public void FileList(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{			
			m_FileList.Add(Info.path);
			Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
		}

		public void FileListByExtension(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{			
			FileInfo fi=new FileInfo(Info.path);
			if (fi.Extension.ToLower()==m_strByExt.ToLower())
				m_FileList.Add(Info.path);
			Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
		}

		public void FindFileIndex(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{			
			if (m_FileCount==m_FileFindIndex)
			{
				m_FileInfo=Info;
				Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_SUCCESS;
			}
			else
			{
				m_FileCount++;
				Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
			}
		}

		public int GetFileCount()
		{
			if (!m_bCHMLoaded)
				return 0;

			m_FileCount=0;
			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FileCount);			
			this.chm_enumerate(CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FileCount);
			return m_FileCount;
		}		

		public ArrayList GetFileList()
		{
			if (!m_bCHMLoaded)
				return null;

			m_FileList=null;
			m_FileList=new ArrayList(1000);
			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FileList);			
			this.chm_enumerate(CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FileList);
			return m_FileList;
		}		

		public ArrayList GetFileListByExtenstion(string Ext)
		{
			if (!m_bCHMLoaded)
				return null;

			m_FileList=null;
			m_FileList=new ArrayList(1000);
			m_strByExt=Ext;
			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FileListByExtension);			
			this.chm_enumerate(CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FileListByExtension);
			return m_FileList;
		}		

		public chmUnitInfo GetFileInfo(string FileName)
		{
			if (!m_bCHMLoaded)
				return null;
			
			m_FileFind=FileName.ToLower().Replace("/",@"\");	

			// Remove all leading '..\'			
			do
			{				
				if (m_FileFind.StartsWith(@"..\"))
					m_FileFind=m_FileFind.Substring(3);					
				else 
					break;
			}
			while(true);

			if (!m_FileFind.StartsWith(@"\"))
				m_FileFind=@"\"+m_FileFind;
				
			string []parts=m_FileFind.Split('\\');
			m_FileFindLastPart=@"\"+parts[parts.GetUpperBound(0)];			

			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FindFile);
			m_FileInfo=null;
			this.chm_enumerate(CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FindFile);
			return m_FileInfo;
		}		
		
		private int m_FileFindIndex=0;
		public chmUnitInfo GetFileInfo(int FileIndex)
		{
			if (!m_bCHMLoaded)
				return null;

			m_FileFindIndex=FileIndex;

			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FindFileIndex);
			m_FileCount=0;
			m_FileInfo=null;
			this.chm_enumerate(CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FindFileIndex);
			return m_FileInfo;
		}

		public chmUnitInfo GetFileInfoByExtension(string Ext)
		{
			this.CHMFileFoundEvent+=new CHMStream.CHMFileFound(FindFileByExtension);
			m_FileInfo=null;
			m_FileFind=Ext.ToLower();
			this.chm_enumerate(CHMStream.CHM_ENUMERATE.CHM_ENUMERATE_ALL);
			this.CHMFileFoundEvent-=new CHMStream.CHMFileFound(FindFileByExtension);
			return m_FileInfo;
		}

		public bool ExtractFile(string FileName, System.IO.Stream st)
		{
			if (!m_bCHMLoaded)
				return false;

			chmUnitInfo Info=GetFileInfo(FileName);
			return ExtractFile(Info,st);
		}

		public bool ExtractFile(chmUnitInfo Info, System.IO.Stream st)
		{		
			if (!m_bCHMLoaded)
				return false;

			if (Info==null)
				return false;
			else
			{
				chm_retrieve_object(Info,st,0,Info.length);				
			}
			return true;
		}

		public string ExtractTextFile(string FileName)
		{	
			if (!m_bCHMLoaded)
				return "CHM File not loaded";

			chmUnitInfo Info=GetFileInfo(FileName);
			return ExtractTextFile(Info);
		}

		public string ExtractTextFile(chmUnitInfo Info)
		{				
			if (!m_bCHMLoaded)
				return "CHM File not loaded";

			if (Info==null)
				return "";
			
			if (Info.path.Length>=2)
			{
				if (Info.path.Substring(0,2).CompareTo("/#")==0)
					return "";
				if (Info.path.Substring(0,2).CompareTo("/$")==0)
					return "";
			}

			MemoryStream st=new MemoryStream((int)Info.length);
			this.chm_retrieve_object(Info,st,0,Info.length);			

			if (st.Length==0)
				return "";

			string Text="";			
			
			ASCIIEncoding ascii=new ASCIIEncoding();			
			Text=ascii.GetString(st.ToArray(),0,50);

			// UTF Decoding
			if (Text.IndexOf("UTF-8")!=-1)
			{
				UTF8Encoding utf8 = new UTF8Encoding();
				Text=utf8.GetString(st.ToArray(),0,(int)st.Length);
			}
			else
				Text=ascii.GetString(st.ToArray(),0,(int)st.Length);
	
			return Text;
		}	
		
		public void FindFileByExtension(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{			
			if ((Info.path.StartsWith("::")) || (Info.path.StartsWith("#")) ||(Info.path.StartsWith("$")))
			{
				Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
				return;
			}			

			FileInfo Fi=new FileInfo(Info.path);
			if (Fi.Extension.ToLower()==m_FileFind)
			{
				Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_SUCCESS;
				m_FileInfo=Info;
			}
			else
				Status=CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
		}

		public bool GetCHMParts(string Url, ref string CHMFileName, ref string FileName, ref string Anchor)
		{
			Regex ParseURLRegEx= new Regex( @"ms-its:(?'CHMFile'.*)::(?'Topic'.*)", RegexOptions.IgnoreCase| RegexOptions.Singleline | RegexOptions.ExplicitCapture| RegexOptions.IgnorePatternWhitespace| RegexOptions.Compiled);

			// Parse URL - Get CHM Filename & Page Name
			//	Format 'ms-its:file name.chm::/topic.htm'
			if (ParseURLRegEx.IsMatch(Url))
			{
				Match m=ParseURLRegEx.Match(Url);
				CHMFileName=m.Groups["CHMFile"].Value;
				string Topic=m.Groups["Topic"].Value;
				int idx=Topic.IndexOf("#");
				if (idx>-1)
				{
					FileName=Topic.Substring(0,idx);
					Anchor=Topic.Substring(idx+1);
				}
				else
					FileName=Topic;
				return true;
			}
			return false;
		}

		private string m_TempDir="";		
		string ReplaceFileName(Match m)
		{
			string strReplace = m.ToString();

			// Process string.
			if (m.Groups["FileName"]==null)
				return strReplace;

			string FileName=m.Groups["FileName"].Value;
			string FileName2=FileName.Replace("/",@"\");
			int idx=FileName2.IndexOf("::");
			if (idx!=-1)
				FileName2=FileName2.Substring(idx+2);
			string []parts=FileName2.Split('\\');			
			string NewName=@"file://"+m_TempDir+parts[parts.GetUpperBound(0)];

			strReplace=strReplace.Replace(FileName,NewName);	
			return strReplace;
		}

		public ArrayList GetFileList(ref string Text, string TempDir)
		{			
			if (!m_bCHMLoaded)
				return null;

			m_TempDir=TempDir;

			ArrayList FilesList=new ArrayList();

			// Parse HTML for CCS, ima, etc			
			string regexContent=@"[\x2f a-zA-Z0-9\x5C\x2E\x28\x29\x23\x24\x25\x26\x27\x22\x21\x3F\x3E\x3D\x3C\x3B\x3A\x5B\x5D\x5E\x5F\x7D\x7C\x7B\x7E\x40\x2D\x2C\x2B\x2A]*\s*";
			string regexFileName=@"\s*=\s*[""|'](?'FileName'[^""^']*)[""|']\s*";

			Regex ScriptRegex = new Regex(@"<script[^>]*>.*</script>",
				RegexOptions.IgnoreCase
				| RegexOptions.Multiline
				| RegexOptions.Singleline
				| RegexOptions.IgnorePatternWhitespace
				| RegexOptions.Compiled);

			Regex XMLRegex = new Regex(@"<\?xml.*\?>",
				RegexOptions.IgnoreCase
				| RegexOptions.Multiline
				| RegexOptions.Singleline
				| RegexOptions.IgnorePatternWhitespace
				| RegexOptions.Compiled);

			Regex XMLRegex2 = new Regex(@"<xml[^>]*>.*</xml>",
				RegexOptions.IgnoreCase
				| RegexOptions.Multiline
				| RegexOptions.Singleline
				| RegexOptions.IgnorePatternWhitespace
				| RegexOptions.Compiled);

			Regex SRCRegex = new Regex( 
				@"src"+regexFileName,
				RegexOptions.IgnoreCase
				| RegexOptions.Multiline
				| RegexOptions.Singleline
				| RegexOptions.IgnorePatternWhitespace
				| RegexOptions.Compiled);

			Regex StyleSheetRegex = new Regex(				
				@"<link\s*"+regexContent+@"rel\s*=\s*[""|']stylesheet[""|']\s*"+regexContent + "href"+regexFileName,
				RegexOptions.IgnoreCase
				| RegexOptions.Multiline
				| RegexOptions.Singleline
				| RegexOptions.IgnorePatternWhitespace
				| RegexOptions.Compiled);
			
			// Remove Script Tags
			Text=ScriptRegex.Replace(Text,"");

			// Remove XML Tags
			Text=XMLRegex.Replace(Text,"");
			Text=XMLRegex2.Replace(Text,"");			


			StringBuilder s=new StringBuilder(Text);
			
			if (StyleSheetRegex.IsMatch(Text))
			{				
				Match m = StyleSheetRegex.Match(Text);
				while (m.Success)
				{										
					string FileName=m.Groups["FileName"].ToString();
					FilesList.Add(FileName);					
					m=m.NextMatch();
				}
				Text=StyleSheetRegex.Replace(Text,new MatchEvaluator(ReplaceFileName));
			}

			if (SRCRegex.IsMatch(Text))
			{								
				Match m = SRCRegex.Match(Text);
				while (m.Success)
				{					
					string FileName=m.Groups["FileName"].ToString();
					FilesList.Add(FileName);
					m=m.NextMatch();
				}
				Text=SRCRegex.Replace(Text,new MatchEvaluator(ReplaceFileName));
			}
			
			return FilesList;
		}

		public string GetHTMLAndFiles(string TempDir, string Url)
		{
			string HTMLText="";
			if (TempDir.EndsWith(@"\")) TempDir=TempDir.Substring(TempDir.Length-1);			
	
			// Delete Temp Directory
			if (Directory.Exists(TempDir))
				Directory.Delete(TempDir,true);

			// Create Temp Directory
			if (!Directory.Exists(TempDir))
				Directory.CreateDirectory(TempDir);

			if (!TempDir.EndsWith(@"\")) TempDir+=@"\";		

			string m_TopicName="";

			string m_CHMFile=CHMFileName;
			string Anchor="";
			if (!GetCHMParts(Url,ref m_CHMFile, ref m_TopicName, ref Anchor))
			{
				m_CHMFile=this.CHMFileName;
				m_TopicName=Url;
			}
				
			if (m_TopicName=="")
				return "#No TopicName defined in Url : "+ Url;		

			m_TopicName=m_TopicName.Replace("/",@"\");			
			if (!m_CHMFile.StartsWith(@"\"))
				m_CHMFile=this.Dir+@"\"+m_CHMFile;

			// Open CHM			
			CHMStream LocalCHM=this;
			
			if (this.CHMFileName.ToLower().CompareTo(m_CHMFile.ToLower())!=0)
				LocalCHM=new CHMStream(m_CHMFile);

			// Get HTML
			HTMLText=LocalCHM.ExtractTextFile(m_TopicName);
			if (HTMLText=="")
				return "#Failed to find Topic in CHM File : "+Url;

			HTMLText=GetFiles(TempDir, HTMLText, LocalCHM);

			return HTMLText;
		}

		public string GetFiles(string TempDir, string HTMLText, CHMStream chm)
		{
			return GetFiles(TempDir, HTMLText, chm,0);
		}

		public string GetFiles(string TempDir, string HTMLText, CHMStream  chm, int Level)
		{
			// Get FilesList & Extract Files to Temp Dir
			ArrayList FileList=chm.GetFileList(ref HTMLText, TempDir);
			if (FileList!=null)
			{
				foreach(object obj in FileList)
				{
					string FileName=(string)obj;

					string CHMFileName="";
					string TopicName="";					
					string Anchor="";
					CHMStream NewCHM=chm;					
					if (GetCHMParts(FileName,ref CHMFileName, ref TopicName, ref Anchor))
					{	
						NewCHM=new CHMStream(chm.Dir+@"\"+CHMFileName);
						if (!NewCHM.CHMLoaded)
							NewCHM=null;
						FileName=TopicName;
					}
					else
					{
						CHMFileName=chm.CHMFileName;
						NewCHM=chm;
						TopicName=FileName;
					}
					if (NewCHM==null)
						continue;

					if (((FileName.ToLower().EndsWith(".htm")) || (FileName.ToLower().EndsWith(".html"))) && (Level<2))
					{
						string HTMLText2=NewCHM.ExtractTextFile(FileName);
						FileInfo Fi=new FileInfo(FileName);
						string path=TempDir+Fi.Name;
						HTMLText2=GetFiles(TempDir,HTMLText2, chm, Level+1);

						if (File.Exists(path))
							File.Delete(path);

						StreamWriter st=new StreamWriter(path);
						st.WriteLine(HTMLText2);
						st.Close();
					}
					else
					{
						// Extract all other files as is						
						string FileName2=FileName.Replace("/",@"\");
						if (FileName2.Substring(0,1)==@"\")
							FileName2=FileName2.Substring(1);

						string []parts=FileName2.Split('\\');
						string path=TempDir+parts[parts.GetUpperBound(0)];
						if (File.Exists(path))
							File.Delete(path);
						System.IO.FileStream st=new FileStream(path,FileMode.CreateNew);
						NewCHM.ExtractFile(FileName2,st);
						st.Close();
					}				
				}
			}

			// return HTML string of main page
			return HTMLText;
		}
		
		#region CHMStream Enums
		// the two available spaces in a CHM file                      
		// N.B.: The format supports arbitrarily many spaces, but only 
		//       two appear to be used at present.                     
		public enum CHM_COMPRESSION { CHM_UNCOMPRESSED=0, CHM_COMPRESSED=1};

		// resolve a particular object from the archive 
		public enum CHM_RESOLVE { CHM_RESOLVE_SUCCESS=0, CHM_RESOLVE_FAILURE=1};
														  
		// retrieve part of an object from the archive 
		public enum CHM_ENUMERATE
		{
			None=0,
			CHM_ENUMERATE_NORMAL	=1,
			CHM_ENUMERATE_META		=2,
			CHM_ENUMERATE_SPECIAL	=4,
			CHM_ENUMERATE_FILES		=8,
			CHM_ENUMERATE_DIRS		=16,
			CHM_ENUMERATE_ALL		=31};

		public enum CHM_ENUMERATOR
		{ 
			CHM_ENUMERATOR_FAILURE	=0, 
			CHM_ENUMERATOR_SUCCESS	=2, 
			CHM_ENUMERATOR_CONTINUE =1
		};
		#endregion

		#region Internal Parameters
		private int ffs(int val)
		{
			int bit=1;
			int idx=1;
			while (bit != 0  &&  (val & bit) == 0)
			{
				bit <<= 1;
				++idx;
			}
			if (bit == 0)
				return 0;
			else
				return idx;
		}

		// names of sections essential to decompression 
		private const string _CHMU_RESET_TABLE = @"::DataSpace/Storage/MSCompressed/Transform/{7FC28940-9D31-11D0-9B27-00A0C91E9C7C}/InstanceData/ResetTable";
		private const string _CHMU_LZXC_CONTROLDATA = @"::DataSpace/Storage/MSCompressed/ControlData";
		private const string _CHMU_CONTENT = @"::DataSpace/Storage/MSCompressed/Content";
		private const string _CHMU_SPANINFO =	@"::DataSpace/Storage/MSCompressed/SpanInfo";

	    private UInt64              dir_offset=0;
		private UInt64              dir_len=0;    
		private UInt64              data_offset=0;
		private Int32               index_root=0;
		private Int32               index_head=0;
		private UInt32              block_len=0;     		
		
		private chmUnitInfo			rt_unit;
		private chmUnitInfo			cn_unit;
		private chmLzxcResetTable	reset_table;
		private bool				compression_enabled=false;

		// LZX control data 
		private int					window_size=0;
		private UInt32              reset_interval=0;
		private UInt32              reset_blkcount=0;
		private BinaryReader		st=null;

		// decompressor state 
		private lzw		lzx_state;		
		private int                 lzx_last_block=0;
		#endregion

		#region Open CHM Stream
		private bool CheckSig(string Sig1, char[] Sig2)
		{
			int  i=0;
			foreach(char ch in Sig1.ToCharArray())
			{
				if(ch!=Sig2[i])
					return false;
				i++;
			}
			return true;	
		}

		// open an ITS archive 
		private bool chm_open(string filename)
		{
			chmItsfHeader        itsfHeader=new chmItsfHeader();
			chmItspHeader        itspHeader=new chmItspHeader();
			chmUnitInfo          uiSpan=new chmUnitInfo();
			chmUnitInfo          uiLzxc=new chmUnitInfo();
			chmLzxcControlData   ctlData=new chmLzxcControlData();
			
			m_bCHMLoaded=false;
			if (!File.Exists(filename))
				return false;

			st=new BinaryReader(File.OpenRead(filename));
			if (st==null)
				return false;

			// read and verify header 			
			if (itsfHeader.Read_itsf_header(st)==0)
			{
				st.Close();
				return false;
			}
			st.BaseStream.Seek((long)itsfHeader.dir_offset,SeekOrigin.Begin);			

			// stash important values from header 
			dir_offset  = itsfHeader.dir_offset;
			dir_len     = itsfHeader.dir_len;
			data_offset = itsfHeader.data_offset;

			// now, read and verify the directory header chunk 
			if (itspHeader.Read_itsp_header(st)==0)
			{
				st.Close();
				return false;
			}

			// grab essential information from ITSP header 
			dir_offset += (UInt64)itspHeader.header_len;
			dir_len    -= (UInt64)itspHeader.header_len;
			index_root  = itspHeader.index_root;
			index_head  = itspHeader.index_head;
			block_len   = itspHeader.block_len;

			// if the index root is -1, this means we don't have any PMGI blocks.
			// as a result, we must use the sole PMGL block as the index root
	     
			if (index_root == -1)
				index_root = index_head;

			compression_enabled=true;

			// prefetch most commonly needed unit infos 
			// if (CHM_RESOLVE.CHM_RESOLVE_SUCCESS != chm_resolve_object(_CHMU_SPANINFO, ref uiSpan) 
			//  || uiSpan.space == CHM_COMPRESSION.CHM_COMPRESSED || 
			if (CHM_RESOLVE.CHM_RESOLVE_SUCCESS != chm_resolve_object(_CHMU_RESET_TABLE, ref rt_unit)    
				|| rt_unit.space == CHM_COMPRESSION.CHM_COMPRESSED                        
				|| CHM_RESOLVE.CHM_RESOLVE_SUCCESS != chm_resolve_object(_CHMU_CONTENT,ref cn_unit)    
				|| cn_unit.space == CHM_COMPRESSION.CHM_COMPRESSED                        
				|| CHM_RESOLVE.CHM_RESOLVE_SUCCESS != chm_resolve_object(_CHMU_LZXC_CONTROLDATA, ref uiLzxc)                
				|| uiLzxc.space == CHM_COMPRESSION.CHM_COMPRESSED)
			{
				compression_enabled=false;
				// st.Close();				
				// return false;
			}

			// try to read span 
			// N.B.: we've already checked that uiSpan is in the uncompressed section,
			//        so this should not require attempting to decompress, which may
			//        rely on having a valid "span"
		     
			if (compression_enabled)
			{		
				reset_table=new chmLzxcResetTable();
				st.BaseStream.Seek((long)((long)data_offset + (long)rt_unit.start),SeekOrigin.Begin);
				if (reset_table.Read_lzxc_reset_table(st)!=1)
				{
					compression_enabled=false;
				}
			}

			if (compression_enabled)
			{			
				// read control data 
				ctlData=new chmLzxcControlData();
				st.BaseStream.Seek((long)((long)data_offset + (long)uiLzxc.start),SeekOrigin.Begin);
				if (ctlData.Read_lzxc_control_data(st)!=1)
				{
					compression_enabled=false;
				}			

				window_size = (int)ctlData.windowSize;
				reset_interval = ctlData.resetInterval;
				try
				{
					reset_blkcount = (uint)(reset_interval    /
						(window_size / 2) *
						ctlData.windowsPerReset);
				}
				catch(Exception)
				{
					reset_blkcount=0;
				}
			}

			m_bCHMLoaded=true;

			return true;
		}
		#endregion

		#region Close CHM Stream
		// close an ITS archive 
		private  void chm_close()
		{
			if (!m_bCHMLoaded)
				return;

			st.Close();
			lzx_state.LZXteardown();
			lzx_state=null;
		}
		#endregion
			
		#region Find File in CHM Stream
		// resolve a particular object from the archive 
		private CHMStream.CHM_RESOLVE chm_resolve_object(string objPath, ref chmUnitInfo ui)
		{     			
			Int32 curPage;
	
			// starting page 
			curPage = index_root;

			// until we have either returned or given up 
			while (curPage != -1)
			{
				st.BaseStream.Seek((long)((long)dir_offset + (long)(curPage*block_len)),SeekOrigin.Begin);
				
				char[] sig=st.ReadChars(4);
				st.BaseStream.Seek(-4,SeekOrigin.Current);
				if (CheckSig("PMGL",sig))
				{
					chmPmglHeader PmglHeader=new chmPmglHeader();
					if (PmglHeader.Read_pmgl_header(st)==1)
					{
						// scan block 
						ui=PmglHeader.FindObject(st,block_len,objPath);
						if (ui== null) 
							return CHMStream.CHM_RESOLVE.CHM_RESOLVE_FAILURE;

						// parse entry and return 
						return CHMStream.CHM_RESOLVE.CHM_RESOLVE_SUCCESS;
					}
				}	
				else if (CheckSig("PMGI",sig))
				{			
					chmPmgiHeader pmgiHeader=new chmPmgiHeader();
					pmgiHeader.Read_pmgi_header(st);
					curPage = pmgiHeader._chm_find_in_PMGI(st, block_len, objPath);
				}				
				else
					// else, we are confused.  give up. 
					return CHMStream.CHM_RESOLVE.CHM_RESOLVE_FAILURE;
			}

			// didn't find anything.  fail. 
			return CHMStream.CHM_RESOLVE.CHM_RESOLVE_FAILURE;
		}
		#endregion

		#region Extract File from CHM Stream

		//	* utility methods for dealing with compressed data		 
		// get the bounds of a compressed block.  return 0 on failure 			
		private int _chm_get_cmpblock_bounds(System.IO.BinaryReader st, UInt64 block, ref UInt64 start, ref UInt64 len)
		{
			// for all but the last block, use the reset table 
			if (block < reset_table.block_count-1)
			{
				// unpack the start address 				
				st.BaseStream.Seek((long)data_offset + (long)rt_unit.start + (long)reset_table.table_offset + (long)(block*8),SeekOrigin.Begin);
				start=st.ReadUInt64();
				len=st.ReadUInt64();
			}

			// for the last block, use the span in addition to the reset table 
			else
			{				
				// unpack the start address 
				st.BaseStream.Seek((long)data_offset + (long)rt_unit.start + (long)reset_table.table_offset + (long)(block*8),SeekOrigin.Begin);
				start=st.ReadUInt64();
				len = reset_table.compressed_len;
			}

			// compute the length and absolute start address 
			len -= start;
			start += data_offset + cn_unit.start;

			return 1;
		}

		// decompress the block.  must have lzx_mutex. 
		private ulong _chm_decompress_block(UInt64 block, System.IO.Stream OutBuffer)
		{
			// byte []cbuffer = new byte(reset_table.block_len + 6144);
			ulong cmpStart=0;                                    // compressed start  
			ulong cmpLen=0;                                       // compressed len    
			UInt32 blockAlign = (UInt32)(block % reset_blkcount); // reset intvl. aln. 			
			
			// check if we need previous blocks 
			if (blockAlign != 0)			
			{
				/* fetch all required previous blocks since last reset */
				for (UInt32 i = blockAlign; i > 0; i--)
				{
					UInt32 curBlockIdx = (UInt32)(block-i);
					
					/* check if we most recently decompressed the previous block */
					if ((ulong)lzx_last_block != curBlockIdx)
					{
						if ((curBlockIdx % reset_blkcount)==0)
						{
							lzx_state.LZXreset();
						}
						
						_chm_get_cmpblock_bounds(st,curBlockIdx, ref cmpStart, ref cmpLen);
						st.BaseStream.Seek((long)cmpStart,SeekOrigin.Begin);						
						if (lzx_state.LZXdecompress(st,OutBuffer, ref cmpLen, ref reset_table.block_len) != lzw.DECR_OK)
							return (Int64)0;
					}
					lzx_last_block = (int)(curBlockIdx);
				}
			}
			else
			{
				if ((block % reset_blkcount)==0)
				{
					lzx_state.LZXreset();
				}		
			}

			// decompress the block we actually want 
			if (_chm_get_cmpblock_bounds(st, block, ref cmpStart, ref cmpLen)==0)
				return 0;

			st.BaseStream.Seek((long)cmpStart,SeekOrigin.Begin);
							
			if (lzx_state.LZXdecompress(st, OutBuffer, ref cmpLen,ref reset_table.block_len) != lzw.DECR_OK)
				return (Int64)0;

			lzx_last_block = (int)block;

			// XXX: modify LZX routines to return the length of the data they
			// * decompressed and return that instead, for an extra sanity check.
		     return reset_table.block_len;
		}

		// grab a region from a compressed block 
		private ulong _chm_decompress_region(Stream buf, ulong  start, ulong len)
		{
			ulong nBlock, nOffset;
			ulong nLen;
			ulong gotLen;
			// byte [] ubuffer=null;

			if (len <= 0)
				return (Int64)0;

			// figure out what we need to read 
			nBlock = start / reset_table.block_len;
			nOffset = start % reset_table.block_len;
			nLen = len;
			if (nLen > (reset_table.block_len - nOffset))
				nLen = reset_table.block_len - nOffset;

			// data request not satisfied, so... start up the decompressor machine 			
			if (lzx_state==null)
			{
				int window_size = ffs(this.window_size) - 1;
				lzx_last_block = -1;

				lzx_state=new lzw();
				lzx_state.LZXinit(window_size);
			}

			// decompress some data 
			MemoryStream ms=new MemoryStream((int)reset_table.block_len+6144);			
			gotLen = _chm_decompress_block(nBlock, ms);
			if (gotLen < nLen)
				nLen = gotLen;	
			
			// memcpy(buf, ubuffer+nOffset, (unsigned int)nLen);
			ms.Position=(long)nOffset;
			for(ulong i=0;i<nLen;i++)
				buf.WriteByte((byte)ms.ReadByte());
			buf.Flush();
			return nLen;
		}

		// retrieve (part of) an object 
		private ulong chm_retrieve_object(chmUnitInfo ui, Stream buf, ulong addr, ulong len)
		{
			// starting address must be in correct range 
			if (addr < 0  ||  addr >= (ulong)ui.length)
				return (Int64)0;

			// clip length 
			if (addr + (ulong)len > (ulong)ui.length)
				len = (ulong)ui.length - (ulong)addr;

			// if the file is uncompressed, it's simple 
			if (ui.space == CHMStream.CHM_COMPRESSION.CHM_UNCOMPRESSED)
			{
				// read data 
				long FilePos=st.BaseStream.Position;				
				st.BaseStream.Seek((long)((long)data_offset + (long)ui.start + (long)addr),SeekOrigin.Begin);
				// byte [] buffer=st.ReadBytes((int)len);
				buf.Write(st.ReadBytes((int)len),0,(int) len);
				st.BaseStream.Seek(FilePos,SeekOrigin.Begin);
				return (ulong)len;
			}

			// else if the file is compressed, it's a little trickier 
			else // ui->space == CHM_COMPRESSED 
			{			
				if (lzx_state!=null)
				{
					lzx_state.LZXteardown();
					lzx_state=null;
				}
				ulong swath=0, total=0;
				do 
				{
					if (!compression_enabled)
						return total;

					// swill another mouthful 
					swath = _chm_decompress_region(buf, ui.start + addr, len);

					// if we didn't get any... 
					if (swath == 0)
					{
						Trace.Assert((total!=ui.length),"De-compress failed","Length Required = "+ui.length.ToString()+" Length returned = "+total.ToString());
						return total;
					}

					// update stats 
					total += swath;
					len -= swath;
					addr += swath;
				} while (len != 0);		
				lzx_state=null;

				Trace.Assert((len!=ui.length),"De-compress failed","Length Required = "+ui.length.ToString()+" Length returned = "+len.ToString());
				return len;
			}
		}
		#endregion
	
		#region Enumerate functions				
		// Enumerate the objects in the .chm archive 
		// Use delegate to handle callback

		public delegate void CHMFileFound(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status);
		public event CHMFileFound CHMFileFoundEvent;

		public void OnFileFound(chmUnitInfo Info, ref CHMStream.CHM_ENUMERATOR Status)
		{
			if (CHMFileFoundEvent!=null)
				CHMFileFoundEvent(Info,ref Status);
		}
		private int chm_enumerate(CHM_ENUMERATE what)
		{
			Int32 curPage;

			// buffer to hold whatever page we're looking at 			
			chmPmglHeader header;
			uint end=0;
			uint cur=0;			

			// the current ui 
			chmUnitInfo ui= new chmUnitInfo();
			CHMStream.CHM_ENUMERATE flag=CHMStream.CHM_ENUMERATE.None;

			// starting page 
			curPage = index_head;

			// until we have either returned or given up 
			while (curPage != -1)
			{
				st.BaseStream.Seek((long)((long)dir_offset + (long)(curPage*block_len)),SeekOrigin.Begin);

				// figure out start and end for this page 
				cur = (uint)st.BaseStream.Position;
				
				header=new chmPmglHeader();
				if (header.Read_pmgl_header(st)==0)				
					return 0;

				end = (uint)(st.BaseStream.Position + block_len - (header.free_space)- chmPmglHeader._CHM_PMGL_LEN);

				// loop over this page 
				while (st.BaseStream.Position < end)
				{
					if (header._chm_parse_PMGL_entry(st,ref ui)==0)
						return 0;

					// check for DIRS 
					if (ui.length == 0  &&  ((what & CHM_ENUMERATE.CHM_ENUMERATE_DIRS)==0))
						continue;

					// check for FILES 
					if (ui.length != 0  &&  ((what & CHM_ENUMERATE.CHM_ENUMERATE_FILES)==0))
						continue;

					// check for NORMAL vs. META 
					if (ui.path[0] == '/')
					{
						// check for NORMAL vs. SPECIAL 
						if (ui.path.Length>2)
						{
							if (ui.path[1] == '#'  ||  ui.path[1] == '$')
								flag = CHMStream.CHM_ENUMERATE.CHM_ENUMERATE_SPECIAL;
							else
								flag = CHMStream.CHM_ENUMERATE.CHM_ENUMERATE_NORMAL;
						}					
						else
							flag = CHMStream.CHM_ENUMERATE.CHM_ENUMERATE_META;
						if ((what & flag)==0)
							continue;
					}

					// call the enumerator 
					{
						CHMStream.CHM_ENUMERATOR status = CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE;
						OnFileFound(ui,ref status);
													
						switch (status)
						{
							case CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_FAILURE:  
								return 0;

							case CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_CONTINUE: 
								break;

							case CHMStream.CHM_ENUMERATOR.CHM_ENUMERATOR_SUCCESS:  
								return 1;

							default:
								break;
						}
					}
				}

				// advance to next page 
				curPage = header.block_next;
			}

			return 1;
		}
		#endregion

		#region IDisposable Members

		private bool disposed=false;
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
				}
			}
			disposed = true;         
		}

		#endregion
	}

	#region Structures used by CHM Storage
	public class BaseStructure
	{
		public bool CheckSig(string Sig1, char[] Sig2)
		{
			int i=0;
			foreach(char ch in Sig1.ToCharArray())
			{
				if (ch!=Sig2[i])
					return false;
				i++;
			}
			return true;
		}

		// skip a compressed dword 
		public void skip_cword(BinaryReader st)
		{		
			byte b=0;
			while ((b=st.ReadByte())>= 0x80);			
		}

		// skip the data from a PMGL entry 
		public void _chm_skip_PMGL_entry_data(BinaryReader st)
		{
			skip_cword(st);
			skip_cword(st);
			skip_cword(st);
		}

		// parse a compressed dword 
		public UInt64 _chm_parse_cword(BinaryReader st)
		{
			UInt64 accum = 0;
			byte temp=0;			
			while ((temp=st.ReadByte()) >= 0x80)
			{
				accum <<= 7;
				accum += (ulong)(temp & 0x7f);
			}

			return (accum << 7) + temp;
		}

		// parse a utf-8 string into an ASCII char buffer 
		public int _chm_parse_UTF8(BinaryReader st, UInt64 count, ref string path)
		{
			UTF8Encoding utf8=new UTF8Encoding();
			path=utf8.GetString(st.ReadBytes((int)count),0,(int)count);							
			return 1;
		}
	}

	public class chmUnitInfo
	{
		public UInt64		start=0;
		public UInt64		length=0;
		public CHMStream.CHM_COMPRESSION space=CHMStream.CHM_COMPRESSION.CHM_UNCOMPRESSED;
		public string		path="";
	}

	// structure of ITSF headers 						
	public class chmItsfHeader : BaseStructure
	{
		public const int _CHM_ITSF_V2_LEN=0x58;
		public const int _CHM_ITSF_V3_LEN=0x60;

		public char[]	signature=null;				//  0 (ITSF) 
		public Int32	version=0;					//  4 
		public Int32	header_len=0;				//  8 
		public Int32	unknown_000c=0;				//  c 
		public UInt32	last_modified=0;			// 10 
		public UInt32	lang_id=0;					// 14 			
		public Guid		dir_uuid;					// 18 
		public Guid		stream_uuid;				// 28 
		public UInt64	unknown_offset=0;			// 38 
		public UInt64	unknown_len=0;				// 40 
		public UInt64	dir_offset=0;				// 48 
		public UInt64	dir_len=0;					// 50 
		public UInt64	data_offset=0;				// 58 (Not present before V3) 

		public int Read_itsf_header(BinaryReader st)
		{							
			signature=st.ReadChars(4);
			if (CheckSig("ITSF",signature)==false)
				return 0;
			
			version=st.ReadInt32();
			header_len=st.ReadInt32();
			unknown_000c=st.ReadInt32();
			last_modified=st.ReadUInt32();
			lang_id=st.ReadUInt32();
			dir_uuid=new Guid(st.ReadBytes(16));
			stream_uuid=new Guid(st.ReadBytes(16));			
			unknown_offset=st.ReadUInt64();
			unknown_len=st.ReadUInt64();
			dir_offset=st.ReadUInt64();
			dir_len=st.ReadUInt64();

			if (version==2)
			{
				if (header_len != chmItsfHeader._CHM_ITSF_V2_LEN)
					return 0;
			}
			else if (version==3)
			{
				if (header_len != chmItsfHeader._CHM_ITSF_V3_LEN)
					return 0;
			}
			else return 0;

			if (version==3)
				data_offset=st.ReadUInt64();
			else
				data_offset = dir_offset + dir_len;

			return 1;
		}
	}

	// structure of ITSP headers 
	public class chmItspHeader : BaseStructure
	{
		const int CHM_ITSP_V1_LEN=0x54;
	
		public char[]	signature=null;		//  0 (ITSP) 
		public Int32    version=0;         
		public Int32    header_len=0;      
		public Int32    unknown_000c=0;    
		public UInt32   block_len=0;       
		public Int32    blockidx_intvl=0;  
		public Int32    index_depth=0;     
		public Int32    index_root=0;      		
		public Int32    index_head=0;			
		public Int32    unknown_0024=0;    
		public Int32    num_blocks=0;    
		public Int32    unknown_002c=0;    
		public UInt32	lang_id=0;         
		public Guid		system_uuid;
		public Guid		unknown_0044;

		public int Read_itsp_header(BinaryReader st)
		{							
			signature=st.ReadChars(4);		//  0 (ITSP) 			
			if (CheckSig("ITSP",signature)==false)
				return 0;

			version=st.ReadInt32();
			header_len=st.ReadInt32();
		
			if (header_len!=CHM_ITSP_V1_LEN)
				return 0;

			unknown_000c=st.ReadInt32();
			block_len=st.ReadUInt32();
			blockidx_intvl=st.ReadInt32();
			index_depth=st.ReadInt32();
			index_root=st.ReadInt32();			
			index_head=st.ReadInt32();				
			unknown_0024=st.ReadInt32();
			num_blocks=st.ReadInt32();
			unknown_002c=st.ReadInt32();
			lang_id=st.ReadUInt32();
			system_uuid=new Guid(st.ReadBytes(16));
			unknown_0044=new Guid(st.ReadBytes(16));

			return 1;
		}
	}

	public class chmPmglHeader : BaseStructure
	{
		public const int _CHM_PMGL_LEN=0x14;
		public char[]	signature=null;           //  0 (PMGL) 
		public UInt32	free_space=0;             //  4 
		public UInt32	unknown_0008=0;           //  8 
		public Int32	block_prev=0;             //  c 
		public Int32	block_next=0;             // 10 

		public int Read_pmgl_header(BinaryReader st)
		{
			signature=st.ReadChars(4);
			if (CheckSig("PMGL",signature)==false)
				return 0;

			free_space=st.ReadUInt32();
			unknown_0008=st.ReadUInt32();
			block_prev=st.ReadInt32();
			block_next=st.ReadInt32();
			return 1;
		}

		// parse a PMGL entry into a chmUnitInfo struct; return 1 on success. 
		public int _chm_parse_PMGL_entry(BinaryReader st, ref chmUnitInfo ui)
		{
			UInt64 strLen;

			// parse str len 
			strLen = _chm_parse_cword(st);			

			// parse path 			
			if (_chm_parse_UTF8(st, strLen, ref ui.path)==0)
				return 0;

			// parse info 
			ui.space  = (CHMStream.CHM_COMPRESSION)_chm_parse_cword(st);
			ui.start  = _chm_parse_cword(st);
			ui.length = _chm_parse_cword(st);
			return 1;
		}

		public chmUnitInfo FindObject(BinaryReader st, UInt32 block_len, string objPath)
		{	
			UInt32 end = (UInt32)st.BaseStream.Position+ block_len - free_space - _CHM_PMGL_LEN;			

			// now, scan progressively 
			chmUnitInfo FoundObject=new chmUnitInfo();

			while (st.BaseStream.Position < end)
			{				
				_chm_parse_PMGL_entry(st,ref FoundObject);
				if (FoundObject.path.ToLower().CompareTo(objPath.ToLower())==0)
					return FoundObject;
			}
			FoundObject=null;

			return null;
		}
	}

	public class chmPmgiHeader : BaseStructure
	{
		public const int _CHM_PMGI_LEN=0x8;

		public char[]	signature=null;           //  0 (PMGL) 
		public UInt32	free_space=0;             //  4 			

		public int Read_pmgi_header(BinaryReader st)
		{
			signature=st.ReadChars(4);
		
			if ((signature[0]!='P') || (signature[1]!='M') || (signature[2]!='G') || (signature[3]!='I'))
				return 0;

			free_space=st.ReadUInt32();				
			return 1;
		}

		public Int32 _chm_find_in_PMGI(BinaryReader st, UInt32 block_len, string objPath)
		{		     			
			int page=-1;
			UInt64 strLen;
			string buffer="";
			uint end = (uint)st.BaseStream.Position + block_len - free_space - _CHM_PMGI_LEN;

			// now, scan progressively 
			while (st.BaseStream.Position < end)
			{
				// grab the name 
				strLen = _chm_parse_cword(st);
				buffer="";
				if (_chm_parse_UTF8(st, strLen, ref buffer)==0)
					return -1;

				// check if it is the right name 
				if (buffer.ToLower().CompareTo(objPath.ToLower())>0)
					return page;

				// load next value for path 
				page = (int)_chm_parse_cword(st);
			}
			return page;
		}
	}

	public class chmLzxcResetTable:BaseStructure
	{		
		public UInt32      version=0;
		public UInt32      block_count=0;
		public UInt32      unknown=0;
		public UInt32      table_offset=0;
		public UInt64      uncompressed_len=0;
		public UInt64      compressed_len=0;
		public UInt64      block_len=0;     
		
		public int Read_lzxc_reset_table(BinaryReader st)
		{
			version=st.ReadUInt32();  
			block_count=st.ReadUInt32();  
			unknown=st.ReadUInt32();  
			table_offset=st.ReadUInt32();  
			uncompressed_len=st.ReadUInt64();  
			compressed_len=st.ReadUInt64();  
			block_len=st.ReadUInt64();  
  				
			// check structure 
			if (version != 2)
				return 0;
			else
				return 1;
		}
	}
	
	// structure of LZXC control data block 
	public class chmLzxcControlData:BaseStructure
	{
		public const int _CHM_LZXC_MIN_LEN=0x18;
		public const int _CHM_LZXC_V2_LEN=0x1c; 

		public UInt32   size=0;                   //  0        
		public char[]	signature=null;			  //  4 (LZXC) 
		public UInt32   version=0;                //  8        
		public UInt32   resetInterval=0;          //  c        
		public UInt32   windowSize=0;             // 10        
		public UInt32   windowsPerReset=0;        // 14        
		public UInt32   unknown_18=0;             // 18        
	
		public int Read_lzxc_control_data(BinaryReader st)
		{
			size=st.ReadUInt32();
			signature=st.ReadChars(4);

			if (CheckSig("LZXC",signature)==false)
				return 0;

			version=st.ReadUInt32();
			resetInterval=st.ReadUInt32();
			windowSize=st.ReadUInt32();
			windowsPerReset=st.ReadUInt32();			

			if (size>=_CHM_LZXC_V2_LEN)
				unknown_18=st.ReadUInt32();
			else
				unknown_18 = 0;

			if (version == 2)
			{
				resetInterval *= 0x8000;
				windowSize *= 0x8000;				
			}
			if (windowSize == 0  ||  resetInterval == 0)
				return 0;

			// for now, only support resetInterval a multiple of windowSize/2 
			if (windowSize == 1)
				return 0;
			if ((resetInterval % (windowSize/2)) != 0)
				return 0;
	
			return 1;
		}
	}
	#endregion

	#region LZW Decoder

	internal class lzx_bits 
	{
		public UInt32 bb=0;
		public int bl=0;			
	}

	internal class lzw
	{
		public lzw()
		{		
		}

		/* $Id: lzx.c,v 1.5 2002/10/09 01:16:33 jedwin Exp $ */
		/***************************************************************************
		*                        lzx.c - LZX decompression routines               *
		*                           -------------------                           *
		*                                                                         *
		*  maintainer: Jed Wing <jedwin@ugcs.caltech.edu>                         *
		*  source:     modified lzx.c from cabextract v0.5                        *
		*  notes:      This file was taken from cabextract v0.5, which was,       *
		*              itself, a modified version of the lzx decompression code   *
		*              from unlzx.                                                *
		*                                                                         *
		*  platforms:  In its current incarnation, this file has been tested on   *
		*              two different Linux platforms (one, redhat-based, with a   *
		*              2.1.2 glibc and gcc 2.95.x, and the other, Debian, with    *
		*              2.2.4 glibc and both gcc 2.95.4 and gcc 3.0.2).  Both were *
		*              Intel x86 compatible machines.                             *
		***************************************************************************/

		/***************************************************************************
		*                                                                         *
		*   This program is free software; you can redistribute it and/or modify  *
		*   it under the terms of the GNU General Public License as published by  *
		*   the Free Software Foundation; either version 2 of the License, or     *
		*   (at your option) any later version.  Note that an exemption to this   *
		*   license has been granted by Stuart Caie for the purposes of           *
		*   distribution with CHMFile.  This does not, to the best of my           *
		*   knowledge, constitute a change in the license of this (the LZX) code  *
		*   in general.                                                           *
		*                                                                         *
		***************************************************************************/

		/* some constants defined by the LZX specification */
		private  const int  LZX_MIN_MATCH                = 2;
		private  const int  LZX_MAX_MATCH                = 257;
		private  const int  LZX_NUM_CHARS                = 256;
		private  const int  LZX_BLOCKTYPE_INVALID        = 0;   /* also blocktypes 4-7 invalid */
		private  const int  LZX_BLOCKTYPE_VERBATIM       = 1;
		private  const int  LZX_BLOCKTYPE_ALIGNED        = 2;
		private  const int  LZX_BLOCKTYPE_UNCOMPRESSED   = 3;
		private  const int  LZX_PRETREE_NUM_ELEMENTS     = 20;
		private  const int  LZX_ALIGNED_NUM_ELEMENTS     = 8;   /* aligned offset tree #elements */
		private  const int  LZX_NUM_PRIMARY_LENGTHS      = 7;   /* this one missing from spec! */
		private  const int  LZX_NUM_SECONDARY_LENGTHS    = 249; /* length tree #elements */

		/* LZX huffman defines: tweak tablebits as desired */
		private  const int  LZX_PRETREE_MAXSYMBOLS  = LZX_PRETREE_NUM_ELEMENTS;
		private  const int  LZX_PRETREE_TABLEBITS   = 6;
		private  const int  LZX_MAINTREE_MAXSYMBOLS = LZX_NUM_CHARS + 50*8;
		private  const int  LZX_MAINTREE_TABLEBITS  = 12;
		private  const int  LZX_LENGTH_MAXSYMBOLS   = LZX_NUM_SECONDARY_LENGTHS+1;
		private  const int  LZX_LENGTH_TABLEBITS    = 12;
		private  const int  LZX_ALIGNED_MAXSYMBOLS  = LZX_ALIGNED_NUM_ELEMENTS;
		private  const int  LZX_ALIGNED_TABLEBITS   = 7;
		private  const int  LZX_LENTABLE_SAFETY	  = 64; /* we allow length table decoding overruns */

		public	 const int  DECR_OK				= 0;
		public	 const int  DECR_DATAFORMAT		= 1;
		public	 const int  DECR_ILLEGALDATA	= 2;
		public	 const int  DECR_NOMEMORY		= 3;

		private  byte[] window;         /* the actual decoding window              */
		private  ulong window_size;     /* window size (32Kb through 2Mb)          */
		private  ulong actual_size;     /* window size when it was first allocated */
		private  ulong window_posn;     /* current offset within the window        */
		private  ulong R0, R1, R2;      /* for the LRU offset system               */
		private  UInt32 main_elements;   /* number of main tree elements            */
		private  int   header_read;     /* have we started decoding at all yet?    */
		private  UInt32 block_type;      /* type of this block                      */
		private  ulong block_length;    /* uncompressed length of this block       */
		private  ulong block_remaining; /* uncompressed bytes still left to decode */
		private  ulong frames_read;     /* the number of CFDATA blocks processed   */
		private  long  intel_filesize;  /* magic header value used for transform   */
		private  long  intel_curpos;    /* current offset in transform space       */
		private  int   intel_started;   /* have we seen any translatable data yet? */


		private  uint [] PRETREE_table = new uint[(1<<(6)) + (((20))<<1)]; 
		private  byte [] PRETREE_len = new byte [((20)) + (64)];

		private  uint [] MAINTREE_table= new uint[(1<<(12)) + (((256) + 50*8)<<1)]; 
		private  byte [] MAINTREE_len = new byte [((256) + 50*8) + (64)];

		private  uint [] LENGTH_table= new uint[(1<<(12)) + (((249)+1)<<1)]; 
		private  byte [] LENGTH_len = new byte [((249)+1) + (64)];

		private  uint [] ALIGNED_table= new uint[(1<<(7)) + (((8))<<1)]; 
		private  byte [] ALIGNED_len = new byte [((8)) + (64)];		
		private System.IO.BinaryReader BitSource=null;
		private System.IO.Stream OutputStream=null;

		/* LZX decruncher */

		/* Microsoft's LZX document and their implementation of the
		* com.ms.util.cab Java package do not concur.
		*
		* In the LZX document, there is a table showing the correlation between
		* window size and the number of position slots. It states that the 1MB
		* window = 40 slots and the 2MB window = 42 slots. In the implementation,
		* 1MB = 42 slots, 2MB = 50 slots. The actual calculation is 'find the
		* first slot whose position base is equal to or more than the required
		* window size'. This would explain why other tables in the document refer
		* to 50 slots rather than 42.
		*
		* The constant NUM_PRIMARY_LENGTHS used in the decompression pseudocode
		* is not defined in the specification.
		*
		* The LZX document does not state the uncompressed block has an
		* uncompressed length field. Where does this length field come from, so
		* we can know how large the block is? The implementation has it as the 24
		* bits following after the 3 blocktype bits, before the alignment
		* padding.
		*
		* The LZX document states that aligned offset blocks have their aligned
		* offset huffman tree AFTER the main and length trees. The implementation
		* suggests that the aligned offset tree is BEFORE the main and length
		* trees.
		*
		* The LZX document decoding algorithm states that, in an aligned offset
		* block, if an extra_bits value is 1, 2 or 3, then that number of bits
		* should be read and the result added to the match offset. This is
		* correct for 1 and 2, but not 3, where just a huffman symbol (using the
		* aligned tree) should be read.
		*
		* Regarding the E8 preprocessing, the LZX document states 'No translation
		* may be performed on the last 6 bytes of the input block'. This is
		* correct.  However, the pseudocode provided checks for the *E8 leader*
		* up to the last 6 bytes. If the leader appears between -10 and -7 bytes
		* from the end, this would cause the next four bytes to be modified, at
		* least one of which would be in the last 6 bytes, which is not allowed
		* according to the spec.
		*
		* The specification states that the huffman trees must always contain at
		* least one element. However, many CAB files contain blocks where the
		* length tree is completely empty (because there are no matches), and
		* this is expected to succeed.
		*/

		/* LZX uses what it calls 'position slots' to represent match offsets.
		* What this means is that a small 'position slot' number and a small
		* offset from that slot are encoded instead of one large offset for
		* every match.
		* - position_base is an index to the position slot bases
		* - extra_bits states how many bits of offset-from-base data is needed.
		*/
		private byte [] extra_bits = {
										 0,  0,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,
										 7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14,
										 15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
										 17, 17, 17
									 };

		private ulong [] position_base = {
											 0,       1,       2,      3,      4,      6,      8,     12,     16,     24,     32,       48,      64,      96,     128,     192,
											 256,     384,     512,    768,   1024,   1536,   2048,   3072,   4096,   6144,   8192,    12288,   16384,   24576,   32768,   49152,
											 65536,   98304,  131072, 196608, 262144, 393216, 524288, 655360, 786432, 917504, 1048576, 1179648, 1310720, 1441792, 1572864, 1703936,
											 1835008, 1966080, 2097152
										 };
		
		private UInt32 ReadUInt16()
		{
			UInt32 rc=0;
			UInt32 Byte1=0;
			UInt32 Byte2=0;
			try
			{				
				Byte1=BitSource.ReadByte();
				Byte2=BitSource.ReadByte();
			}
			catch(Exception)
			{
			}			
			rc=(Byte2<<8)+Byte1;
			return rc;
		}

		public bool LZXinit(int WindowSize)
		{			
			ulong wndsize = (ulong)(1 << WindowSize);
			int i, posn_slots;

			/* LZX supports window sizes of 2^15 (32Kb) through 2^21 (2Mb) */
			/* if a previously allocated window is big enough, keep it     */
			if (WindowSize< 15 || WindowSize> 21) return false;

			/* allocate state and associated window */		
			window = new byte[wndsize];
			if (window==null)
			{					
				return false;
			}

			actual_size = wndsize;
			window_size = wndsize;

			/* calculate required position slots */
			if (WindowSize == 20) posn_slots = 42;
			else if (WindowSize== 21) posn_slots = 50;
			else posn_slots = WindowSize << 1;

			/** alternatively **/
			/* posn_slots=i=0; while (i < wndsize) i += 1 << extra_bits[posn_slots++]; */

			/* initialize other state */
			R0  =  R1  = R2 = 1;
			main_elements   = (uint)(LZX_NUM_CHARS + (posn_slots << 3));
			header_read     = 0;
			frames_read     = 0;
			block_remaining = 0;
			block_type      = LZX_BLOCKTYPE_INVALID;
			intel_curpos    = 0;
			intel_started   = 0;
			window_posn     = 0;

			/* initialise tables to 0 (because deltas will be applied to them) */
			for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) MAINTREE_len[i] = 0;
			for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   LENGTH_len[i]   = 0;			

			return true;
		}

		public void LZXteardown()
		{
			window=null;			
		}

		public int LZXreset()
		{
			R0  =  R1  = R2 = 1;
			header_read     = 0;
			frames_read     = 0;
			block_remaining = 0;
			block_type      = LZX_BLOCKTYPE_INVALID;
			intel_curpos    = 0;
			intel_started   = 0;
			window_posn     = 0;

			for (int i = 0; i < LZX_MAINTREE_MAXSYMBOLS + LZX_LENTABLE_SAFETY; i++) MAINTREE_len[i] = 0;
			for (int i = 0; i < LZX_LENGTH_MAXSYMBOLS + LZX_LENTABLE_SAFETY; i++)   LENGTH_len[i]   = 0;

			return DECR_OK;
		}


		/* Bitstream reading macros:
		*
		* INIT_BITSTREAM    should be used first to set up the system
		* READ_BITS(var,n)  takes N bits from the buffer and puts them in var
		*
		* ENSURE_BITS(n)    ensures there are at least N bits in the bit buffer
		* PEEK_BITS(n)      extracts (without removing) N bits from the bit buffer
		* REMOVE_BITS(n)    removes N bits from the bit buffer
		*
		* These bit access routines work by using the area beyond the MSB and the
		* LSB as a free source of zeroes. This avoids having to mask any bits.
		* So we have to know the bit width of the bitbuffer variable. This is
		* sizeof(ulong) * 8, also defined as ULONG_BITS
		*/

		/* number of bits in ulong. Note: This must be at multiple of 16, and at
		* least 32 for the bitbuffer code to work (ie, it must be able to ensure
		* up to 17 bits - that's adding 16 bits when there's one bit left, or
		* adding 32 bits when there are no bits left. The code should work fine
		* for machines where ulong >= 32 bits.
		*/
		private int ULONG_BITS()
		{
			int rc=(System.Runtime.InteropServices.Marshal.SizeOf(typeof(System.UInt32))<<3);
			return rc;
		}

		/* make_decode_table(nsyms, nbits, length[], table[])
		*
		* This function was coded by David Tritscher. It builds a fast huffman
		* decoding table out of just a canonical huffman code lengths table.
		*
		* nsyms  = total number of symbols in this huffman tree.
		* nbits  = any symbols with a code length of nbits or less can be decoded
		*          in one lookup of the table.
		* length = A table to get code lengths from [0 to syms-1]
		* table  = The table to fill up with decoded symbols and pointers.
		*
		* Returns 0 for OK or 1 for error
		*/

		private int make_decode_table(ulong nsyms, byte nbits, ref byte [] length, ref UInt32[] table) 
		{
			ulong sym;
			ulong leaf;
			byte bit_num = 1;
			ulong fill;
			ulong pos         = 0; /* the current position in the decode table */
			ulong table_mask  = (ulong)(1 << nbits);
			ulong bit_mask    = table_mask >> 1; /* don't do 0 length codes */
			ulong next_symbol = bit_mask; /* base of allocation for long codes */

			/* fill entries for codes short enough for a direct mapping */
			while (bit_num <= nbits) 
			{
				for (sym = 0; sym < nsyms; sym++) 
				{
					if (length[sym] == bit_num) 
					{
						leaf = pos;

						if((pos += bit_mask) > table_mask) return 1; /* table overrun */

						/* fill all possible lookups of this symbol with the symbol itself */
						fill = bit_mask;
						while (fill-- > 0) table[leaf++] = (uint)sym;
					}
				}
				bit_mask >>= 1;
				bit_num++;
			}

			/* if there are any codes longer than nbits */
			if (pos != table_mask) 
			{
				/* clear the remainder of the table */
				for (sym = pos; sym < table_mask; sym++) table[sym] = 0;

				/* give ourselves room for codes to grow by up to 16 more bits */
				pos <<= 16;
				table_mask <<= 16;
				bit_mask = 1 << 15;

				while (bit_num <= 16) 
				{
					for (sym = 0; sym < nsyms; sym++) 
					{
						if (length[sym] == bit_num) 
						{
							leaf = pos >> 16;
							for (fill = 0; fill < (ulong)(bit_num - nbits); fill++) 
							{
								/* if this path hasn't been taken yet, 'allocate' two entries */
								if (table[leaf] == 0) 
								{
									table[(next_symbol << 1)] = 0;
									table[(next_symbol << 1) + 1] = 0;
									table[leaf] = (uint)next_symbol++;
								}
								/* follow the path and select either left or right for next bit */
								leaf = table[leaf] << 1;
								if (((pos >> (byte)(15-fill)) & 1)==1) 
									leaf++;
							}
							table[leaf] = (uint)sym;

							if ((pos += bit_mask) > table_mask) 
								return 1; /* table overflow */
						}
					}
					bit_mask >>= 1;
					bit_num++;
				}
			}

			/* full table? */
			if (pos == table_mask) 
				return 0;

			/* either erroneous table, or all elements are 0 - let's find out. */
			for (sym = 0; sym < nsyms; sym++) if (length[(uint)sym]!=0) 
												  return 1;

			return 0;
		}		

		private int lzx_read_lens(byte []lens, ulong first, ulong last, ref lzx_bits lb) 
		{
			ulong i,j, x,y;
			int z;
			
			UInt32 bitbuf = lb.bb;
			int bitsleft = lb.bl;			

			UInt32 [] hufftbl=null;			

			for (x = 0; x < 20; x++) 
			{
				do
				{
				while (bitsleft < (4)) 
				{ 
					bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
					bitsleft += 16; 
				} 
					y = (bitbuf >> (ULONG_BITS()- (4))); 
					bitbuf <<= 4; 
					bitsleft -= 4; 
				}
				while (false);
				PRETREE_len[x] = (byte)y;
			}	
			if (make_decode_table( 20, 6, ref PRETREE_len, ref PRETREE_table)!=0)
				return 2;			

			for (x = first; x < last; ) 
			{
				do
				{
				while (bitsleft < 16) 
				{ 
					bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
					bitsleft += 16; 
				} 
					hufftbl = PRETREE_table; 
					if ((i = hufftbl[((ulong)bitbuf >> (ULONG_BITS()- 6))]) >= 20) 
					{ 
						j = (ulong)(1 << (byte)(ULONG_BITS()- ((6)))); 
						do 
						{
							j >>= 1; 
							i <<= 1; 
							if ((bitbuf & j)!=0)
								i|=1;
							else
								i|=0;

							if (j==0) 
							{
								return (2); 
							} 
						} 
						while ((i = hufftbl[i]) >= 20); 
					} 
					z = (int)i;
					j = PRETREE_len[z]; 
					bitbuf <<= (byte)j;
					bitsleft -= (int)j; 
				}
				while (false);

				if (z == 17) 
				{
					do
					{
					while (bitsleft < (4)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 
					} 
						y = (bitbuf >> (ULONG_BITS()- (4))); 
						bitbuf <<= 4;
						bitsleft -= 4; 
					}
					while(false);
					y += 4;
				
					while ((y--)!=0) 
						lens[x++] = 0;
				}
				else if (z == 18) 
				{					
					do
					{
					while (bitsleft < (5)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16;
					} 
						(y) = (bitbuf >> (ULONG_BITS()- (5))); 
						bitbuf <<= 5;
						bitsleft -= 5; 
					}
					while (false);
				
					y += 20;

					while ((y--)!=0) 
						lens[x++] = 0;
				}
				else if (z == 19) 
				{
					do
					{
					while (bitsleft < (1)) 
					{
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 
					} 
						y = (bitbuf >> (ULONG_BITS()- (1))); 
						bitbuf <<= 1;
						bitsleft -= 1; 
					}
					while(false);
					y += 4;
					do
					{
					while (bitsleft < (16)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 							
					} 
						hufftbl = (PRETREE_table); 
						if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 6))]) >= 20) 
						{ 
							j = (ulong)1 << (byte)(ULONG_BITS()- 6); 
							do 
							{ 
								j >>= 1; 
								i <<= 1; 
								if ((bitbuf & j)==0)
									i|=0;
								else
									i|=1;
								if (j==0) 
								{ 
									return (2); 
								} 
							} 
							while ((i = hufftbl[i]) >= 20); 
						} 
						z = (int)i;
						j = PRETREE_len[z]; 					

						bitbuf <<= (byte)j;
						bitsleft -= (int)j; 
					}
					while(false);
					z = lens[x] - z; 
					if (z < 0) 
						z += 17;

					while ((y--)!=0) 
						lens[x++] = (byte)z;
				}
				else 
				{
					z = lens[x] - z; 
					if (z < 0) 
						z += 17;
					lens[x++] = (byte)z;
				}
			}
			lb.bb = bitbuf;
			lb.bl = bitsleft;
			return 0;
		}

		public int LZXdecompress(System.IO.BinaryReader inpos, System.IO.Stream outpos, ref ulong inlen, ref ulong outlen) 
		{
			BitSource=inpos;
			OutputStream=outpos;
			
			long endinp = BitSource.BaseStream.Position+(long)inlen;						
			ulong runsrc, rundest;
			UInt32 [] hufftbl; /* used in READ_HUFFSYM macro as chosen decoding table */

			UInt32 bitbuf;
			int bitsleft;
			ulong match_offset, i,j,k; /* ijk used in READ_HUFFSYM macro */
			lzx_bits lb; /* used in READ_LENGTHS macro */
			lb=new lzx_bits();			

			int togo = (int)outlen, this_run, main_element, aligned_bits;
			int match_length, length_footer, extra, verbatim_bits;

			bitsleft = 0; 
			bitbuf = 0; 

			/* read header if necessary */
			if (header_read==0) 
			{
				i = j = 0;	
				do
				{
				while (bitsleft < (1)) 
				{ 
					bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
					bitsleft += 16; 
				} 
					k = (bitbuf >> (ULONG_BITS()- (1))); 
					bitbuf <<= 1;
					bitsleft -= 1;
				}
				while(false);

				if (k!=0)	
				{
					do
					{
					while (bitsleft < (16)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() -16 -  bitsleft); 
						bitsleft += 16; 
					} 
						i = (bitbuf >> (ULONG_BITS()- (16))); 
						bitbuf <<= 16;
						bitsleft -= 1;
					}
					while(false);

					do
					{
					while (bitsleft < (16)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 							
					} 
						j = (bitbuf >> (ULONG_BITS()- (16))); 
						bitbuf <<= 16;
						bitsleft -= 16;					
					}
					while(false);
				}
				intel_filesize = (long)((i << 16) | j); 
				header_read = 1;
			}

			/* main decoding loop */
			while (togo > 0) 
			{
				if (block_remaining == 0) 
				{
					if (block_type == (3)) 
					{
						if ((block_length & 1)!=0) 
							BitSource.ReadByte();
						bitsleft = 0; 
						bitbuf = 0; 
					}

					do 
					{ 
					while (bitsleft < (3)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 
					} 
						(block_type) = (uint)(bitbuf >> (ULONG_BITS()- (3))); 
						bitbuf <<= 3; 
						bitsleft -= 3; 
					} 
					while (false);

					do 
					{ 
					while (bitsleft < (16)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 
					} 
						(i) = (bitbuf >> (ULONG_BITS()- (16))); 
						bitbuf <<= 16;  
						bitsleft -= 16; 
					} 
					while (false);

					do 
					{ 
					while (bitsleft < (8)) 
					{ 
						bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
						bitsleft += 16; 
					} 
						(j) = (bitbuf >> (ULONG_BITS()- (8))); 
						bitbuf <<= 8; 
						bitsleft -= 8; 
					} 
					while (false);
					block_remaining = block_length = (i << 8) | j;

					switch (block_type) 
					{
						case (LZX_BLOCKTYPE_ALIGNED):
							for (i = 0; i < 8; i++) 
							{ 
								do 
								{ 
								while (bitsleft < (3)) 
								{ 
									bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
									bitsleft += 16; 
								} 
									(j) = (bitbuf >> (ULONG_BITS()- (3))); 
									bitbuf <<= 3; 
									bitsleft -= 3; 
								} 
								while (false); 
								(ALIGNED_len)[i] = (byte)j; 
							}
							if (make_decode_table( 8, 7, ref ALIGNED_len, ref ALIGNED_table)!=0) 
							{ 
								return (2); 
							}

							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 								
								if (lzx_read_lens(MAINTREE_len,0,256,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 								
							} 
							while (false);
							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 
								if (lzx_read_lens(MAINTREE_len,256,main_elements,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 
							} 
							while (false);

							if (make_decode_table( (256 + 50*8), 12, ref MAINTREE_len, ref MAINTREE_table)!=0) 
							{ 
								return (2); 
							}
							if (MAINTREE_len[0xE8] != 0) intel_started = 1;

							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 
								if (lzx_read_lens(LENGTH_len,0,249,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 
							} 
							while (false);
							if (make_decode_table( (249+1), 12, ref LENGTH_len, ref LENGTH_table)!=0) 
							{ 
								return (2); 
							}
							break;							
		                    
						case (LZX_BLOCKTYPE_VERBATIM):
							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 								
								if (lzx_read_lens(MAINTREE_len,0,256,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 								
							} 
							while (false);
							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 								
								if (lzx_read_lens(MAINTREE_len,256,main_elements,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 
							} 
							while (false);

							if (make_decode_table( (256 + 50*8), 12, ref MAINTREE_len, ref MAINTREE_table)!=0) 
							{ 
								return (2); 
							}
							if (MAINTREE_len[0xE8] != 0) intel_started = 1;

							do 
							{ 
								lb.bb = bitbuf; 
								lb.bl = bitsleft; 
								if (lzx_read_lens(LENGTH_len,0,249,ref lb)!=0) 
								{ 
									return (2); 
								} 
								bitbuf = lb.bb; 
								bitsleft = lb.bl; 
							} 
							while (false);
							if (make_decode_table( (249+1), 12, ref LENGTH_len, ref LENGTH_table)!=0) 
							{ 
								return (2); 
							}
							break;

						case (LZX_BLOCKTYPE_UNCOMPRESSED):
							intel_started = 1; 
							while (bitsleft < (16)) 
							{ 
								bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - bitsleft); 
								bitsleft += 16;
							} 
							if (bitsleft > 16)
							{
								BitSource.BaseStream.Seek(-2,System.IO.SeekOrigin.Current);								
							}
							R0 = (ulong)(BitSource.ReadByte()+(BitSource.ReadByte()<<8)+(BitSource.ReadByte()<<16)+(BitSource.ReadByte()<<24));
							R1 = (ulong)(BitSource.ReadByte()+(BitSource.ReadByte()<<8)+(BitSource.ReadByte()<<16)+(BitSource.ReadByte()<<24));
							R2 = (ulong)(BitSource.ReadByte()+(BitSource.ReadByte()<<8)+(BitSource.ReadByte()<<16)+(BitSource.ReadByte()<<24));
							break;

						default:
							return (DECR_ILLEGALDATA);
					}
				}

				/* buffer exhaustion check */
				if (BitSource.BaseStream.Position > (long) endinp) 
				{
					/* it's possible to have a file where the next run is less than
					* 16 bits in size. In this case, the READ_HUFFSYM() macro used
					* in building the tables will exhaust the buffer, so we should
					* allow for this, but not allow those accidentally read bits to
					* be used (so we check that there are at least 16 bits
					* remaining - in this boundary case they aren't really part of
					* the compressed data)
					*/
					if (BitSource.BaseStream.Position> (long)(endinp+2) || bitsleft < 16) 
						return DECR_ILLEGALDATA;
				}

				while ((this_run = (int)block_remaining) > 0 && togo > 0) 
				{
					if (this_run > togo)
						this_run = togo;

					togo -= this_run;
					block_remaining -= (ulong)this_run;

					/* apply 2^x-1 mask */
					window_posn &= window_size - 1;

					/* runs can't straddle the window wraparound */
					if ((window_posn + (ulong)this_run) > window_size)
						return DECR_DATAFORMAT;

					switch (block_type) 
					{
						case LZX_BLOCKTYPE_VERBATIM:
							while (this_run > 0) 
							{
								do 
								{ 
								while (bitsleft < (16)) 
								{ 
									bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
									bitsleft += 16; 
								} 
									hufftbl = MAINTREE_table; 
									if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 12))]) >= 256 + 50*8) 
									{ 
										j = (ulong)(1 << (ULONG_BITS()- 12)); 
										do 
										{ 
											j >>= 1; 
											i <<= 1; 
											if ((bitbuf & j)!=0) 
												i|=1; 
											else 
												i|=0; 
											if (j==0) 
											{ 
												return (2); 
											} 
										} 
										while ((i = hufftbl[i]) >= (((256) + 50*8))); 
									} 
									j = MAINTREE_len[main_element = (int)i]; 
									bitbuf <<= (byte)j; 
									bitsleft -= (byte)j; 
								} 
								while (false);

								if (main_element < (256)) 
								{	                            
									window[window_posn++] = (byte)main_element;
									this_run--;
								}
								else 
								{	                            
									main_element -= (256);

									match_length = main_element & (7);
									if (match_length == (7)) 
									{
										do 
										{ 
										while (bitsleft < (16)) 
										{ 
											bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
											bitsleft += 16; 
										} 
											hufftbl = (LENGTH_table); 
											if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 12))]) >= (((249)+1))) 
											{ 
												j = (ulong)(1 << (ULONG_BITS()- ((12)))); 
												do 
												{ 
													j >>= 1; 
													i <<= 1; 
													if ((bitbuf & j)!=0) 
														i|=1; 
													else 
														i|=0; 
													if (j==0) 
													{ 
														return (2); 
													} 
												} 
												while ((i = hufftbl[i]) >= (((249)+1))); 
											} 
											j = LENGTH_len[(length_footer) = (int)i]; 
											bitbuf <<= (byte)j; 
											bitsleft -= (byte)j; 
										} 
										while (false);

										match_length += length_footer;
									}
									match_length += (2);

									match_offset = (ulong)(main_element >> 3);

									if (match_offset > 2) 
									{	                                
										if (match_offset != 3) 
										{
											extra = extra_bits[match_offset];
											do 
											{ 
											while (bitsleft < (extra)) 
											{ 
												bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
												bitsleft += 16; 
											} 
												verbatim_bits = (int)(bitbuf >> (ULONG_BITS()- (extra))); 
												bitbuf <<= extra; 
												bitsleft -= extra; 
											} 
											while (false);
											match_offset = position_base[match_offset] - 2 + (ulong)verbatim_bits;
										}
										else 
										{
											match_offset = 1;
										}
	                            
										R2 = R1; R1 = R0; R0 = match_offset;
									}
									else if (match_offset == 0) 
									{
										match_offset = R0;
									}
									else if (match_offset == 1) 
									{
										match_offset = R1;
										R1 = R0; R0 = match_offset;
									}
									else  
									{
										match_offset = R2;
										R2 = R0; R0 = match_offset;
									}

									rundest = window_posn;
									// rundest= window+window_posn
									runsrc  = rundest - match_offset;								
									window_posn += (ulong)match_length;
									this_run -= match_length;
	                        
									// runsrc < window
									while ((runsrc<0) && (match_length-- > 0))
									{
										window[rundest++]=window[runsrc+window_size];
										// *rundest++ = *(runsrc + window_size); 
										runsrc++;
									}
	                        
									while (match_length-- > 0) 
									{
										window[rundest++]=window[runsrc++];
										// *rundest++ = *runsrc++;
									}
								}
							}
							break;

						case LZX_BLOCKTYPE_ALIGNED:
							while (this_run > 0) 
							{
								do 
								{ 
								while (bitsleft < (16)) 
								{ 
									bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
									bitsleft += 16; 									
								} 
									hufftbl = MAINTREE_table; 
									if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 12))]) >= (((256) + 50*8))) 
									{ 
										j = (ulong)1 << (ULONG_BITS()- ((12))); 
										do 
										{ 
											j >>= 1; 
											i <<= 1; 
											if ((bitbuf & j)!=0) 
												i|=1; 
											else 
												i|=0; 
											if (j==0) 
											{ 
												return (2); 
											} 
										} 
										while ((i = hufftbl[i]) >= (((256) + 50*8))); 
									} 
									j = MAINTREE_len[(main_element) = (int)i]; 
									bitbuf <<= (int)j; 
									bitsleft -= (int)j; 
								} 
								while (false);

								if (main_element < (256)) 
								{                            
									window[window_posn++] = (byte)main_element;
									this_run--;
								}
								else 
								{                            
									main_element -= (256);
									match_length = main_element & (7);
									if (match_length == (7)) 
									{
										do 
										{ 
										while (bitsleft < (16)) 
										{ 
											bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
											bitsleft += 16; 
										} 
											hufftbl = LENGTH_table; 
											if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 12))]) >= (((249)+1))) 
											{ 
												j = (ulong) 1 << (ULONG_BITS()- ((12))); 
												do 
												{ 
													j >>= 1; 
													i <<= 1; 
													if ((bitbuf & j)!=0)
														i|=1;
													else
														i|=0;

													if (j==0) 
													{ 
														return (2); 
													} 
												} 
												while ((i = hufftbl[i]) >= (((249)+1))); 
											} 
											j = LENGTH_len[length_footer = (int)i]; 
											bitbuf <<= (int)j; 
											bitsleft -= (int)j; 
										} 
										while (false);
										match_length += length_footer;
									}
									match_length += (2);

									match_offset = (ulong)(main_element >> 3);

									if (match_offset > 2) 
									{                                
										extra = extra_bits[match_offset];
										match_offset = position_base[match_offset] - 2;
										if (extra > 3) 
										{                                    
											extra -= 3;
											do 
											{ 
											while (bitsleft < (extra)) 
											{	
												bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
												bitsleft += 16; 
											} 
												verbatim_bits = (int)(bitbuf >> (ULONG_BITS()- (extra))); 
												bitbuf <<= extra; 
												bitsleft -= extra; 
											} 
											while (false);
											match_offset += (ulong)(verbatim_bits << 3);
											do 
											{ 
											while (bitsleft < (16)) 
											{ 
												bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
												bitsleft += 16; 
											} 
												hufftbl = (ALIGNED_table); 
												if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 7))]) >= 8) 
												{ 
													j = (ulong)1 << (ULONG_BITS()- ((7))); 
													do 
													{ 
														j >>= 1; 
														i <<= 1; 
														if ((bitbuf & j)!=0)
															i|=1;
														else
															i|=0;
														if (j==0) 
														{ 
															return (2); 
														} 
													} 
													while ((i = hufftbl[i]) >= (((8)))); 
												} 

												j = (ALIGNED_len)[(aligned_bits) = (int)i]; 
												bitbuf <<= (int)j; 
												bitsleft -= (int)j; 
											} 
											while (false);
											match_offset += (ulong)aligned_bits;
										}
										else if (extra == 3)	
										{                                    
											do 
											{ 
											while (bitsleft < (16)) 
											{ 
												bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
												bitsleft += 16; 
											} 
												hufftbl = (ALIGNED_table); 
												if ((i = hufftbl[(bitbuf >> (ULONG_BITS()- 7))]) >= 8) 
												{ 
													j = (ulong)1 << (ULONG_BITS()- ((7))); 
													do 
													{ 
														j >>= 1; 
														i <<= 1; 
														if ((bitbuf & j)!=0)
															i|=1;
														else
															i|=0;
														if (j!=0) 
														{ 
															return (2); 
														} 
													} 
													while ((i = hufftbl[i]) >= 8); 
												} 
												j = (ALIGNED_len)[(aligned_bits) = (int)i]; 
												bitbuf <<= (int)j; 
												bitsleft -= (int)j; 
											} 
											while (false);
											match_offset += (ulong)aligned_bits;
										}
										else if (extra > 0) 
										{                                     
											do 
											{ 
											while (bitsleft < (extra)) 
											{ 
												bitbuf |= (UInt32)ReadUInt16() << (ULONG_BITS() - 16 - bitsleft); 
												bitsleft += 16; 
											} 
												(verbatim_bits) = (int)(bitbuf >> (int)(ULONG_BITS()- (extra))); 
												bitbuf <<= extra;
												bitsleft -= extra; 
											} 
											while (false);
											match_offset += (ulong)verbatim_bits;
										}
										else  
										{                                    
											match_offset = 1;
										}
                                
										R2 = R1; R1 = R0; R0 = match_offset;
									}
									else if (match_offset == 0) 
									{
										match_offset = R0;
									}
									else if (match_offset == 1) 
									{
										match_offset = R1;
										R1 = R0; R0 = match_offset;
									}
									else  
									{
										match_offset = R2;
										R2 = R0; R0 = match_offset;
									}

									rundest = window_posn;
									runsrc  = rundest - match_offset;
									window_posn += (ulong)match_length;
									this_run -= match_length;
	                            
									while ((runsrc<0) && (match_length-- > 0))
									{
										// *rundest++ = *(runsrc + window_size); runsrc++;
										window[rundest++]=window[runsrc + window_size];
										runsrc++;
									}
	                            
									while (match_length-- > 0) 
									{
										// *rundest++ = *runsrc++;
										window[rundest++]=window[runsrc++];
									}
								}
							}
							break;

						case LZX_BLOCKTYPE_UNCOMPRESSED:
							if ((BitSource.BaseStream.Position + (long)this_run) > (long)endinp) 
								return (2);

							// memcpy(window + window_posn, inposCount, this_run);
							for(i=0; i<(ulong)this_run;i++)
							{
								window[window_posn+i]=BitSource.ReadByte();
							}
							window_posn += (ulong)this_run;
							break;

						default:
							return DECR_ILLEGALDATA; /* might as well */
					}

				}
			}

			if (togo != 0) return DECR_ILLEGALDATA;

			// memcpy(outpos, window + ((!window_posn) ? window_size : window_posn) - outlen, (size_t) outlen);
			ulong start=0;
			if (window_posn==0)
				start=(ulong)window_size;
			else
				start=(ulong)window_posn;
			
			start-=(ulong)outlen;

			long Pos=OutputStream.Position;
			for(i=0;i<(ulong)outlen;i++)
			{	
				OutputStream.WriteByte(window[start+i]);
			}
			OutputStream.Seek(Pos,System.IO.SeekOrigin.Begin);
			
			/* intel E8 decoding */
			if ((frames_read++ < 32768) && intel_filesize != 0) 
			{
				if (outlen <= 6 || (intel_started==0)) 
				{
					intel_curpos += (long)outlen;
				}
				else 
				{
					// UBYTE *data    = outpos;
					long dataend = OutputStream.Position + (int)outlen - 10;
					long curpos    = intel_curpos;
					long filesize  = intel_filesize;
					long abs_off, rel_off;

					intel_curpos = (long)curpos + (long)outlen;

					while (OutputStream.Position < dataend) 
					{
						if (OutputStream.ReadByte() != 0xE8) 
						{ 
							curpos++; 
							continue; 
						}

						abs_off = (long)(OutputStream.ReadByte() | (OutputStream.ReadByte() <<8) | (OutputStream.ReadByte() <<16) | (OutputStream.ReadByte() <<24));
						if (abs_off < filesize)
						{
							if (abs_off >= 0)
								rel_off = (long)(abs_off - curpos);
							else 
								rel_off = (long)abs_off + filesize;
							OutputStream.WriteByte((byte)(rel_off  & 0x000000ff));
							OutputStream.WriteByte((byte)((rel_off & 0x0000ff00)>>8));
							OutputStream.WriteByte((byte)((rel_off & 0x00ff0000)>>16));
							OutputStream.WriteByte((byte)((rel_off & 0xff000000)>>24));							
						}
						curpos += 5;
					}
				}
			}			
			
			return DECR_OK;
		}		
	}
	#endregion
}
