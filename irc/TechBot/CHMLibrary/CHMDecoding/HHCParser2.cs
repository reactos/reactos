using System;
using System.Collections;
using System.Text;
using System.Text.RegularExpressions;

namespace HtmlHelp.ChmDecoding
{	
	/// <summary>
	/// The class <c>HHCParser</c> implements a parser for HHC contents files.
	/// </summary>
	// internal sealed class HHCParser : IHHCParser
	public class HHCParser2
	{
		static private string m_text1="";
		static private string m_text2="";
		static private int m_CurrentPos=0;

		/// <summary>
		/// Parses a HHC file and returns an ArrayList with the table of contents (TOC) tree
		/// </summary>
		/// <param name="hhcFile">string content of the hhc file</param>
		/// <param name="chmFile">CHMFile instance</param>
		/// <returns>Returns an ArrayList with the table of contents (TOC) tree</returns>
		public static ArrayList ParseHHC(string hhcFile, CHMFile chmFile)
		{
			DateTime StartTime=DateTime.Now;

			ArrayList tocList = new ArrayList();

			m_text2=hhcFile;
			m_text1=hhcFile.ToLower();

			int idx=m_text1.IndexOf("<ul>");
			if (idx==-1)
				return null;			
			m_CurrentPos=idx+4;

			ParamRE = new Regex(RE_ParamBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			AttributesRE = new Regex(RE_QuoteAttributes, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

			ParseTree(tocList,chmFile);

			DateTime EndTime=DateTime.Now;
			TimeSpan Diff=EndTime-StartTime;
			string x=Diff.ToString();

			return tocList;
		}

		/// <summary>
		/// Recursively parses a sitemap tree
		/// </summary>
		/// <param name="text">content text</param>
		/// <param name="arrNodes">arraylist which receives the extracted nodes</param>
		/// <param name="chmFile">CHMFile instance</param>
		static private void ParseTree( ArrayList arrNodes, CHMFile chmFile )
		{	
			bool bProcessing=true;
			do
			{
				bProcessing=false;

				// Indent
				int idxa=m_text1.IndexOf("<ul>",m_CurrentPos);
				int idxb=m_text1.IndexOf("<li>",m_CurrentPos);
				int idxc=m_text1.IndexOf("</ul>",m_CurrentPos);
				
				if ((idxa<idxb) && (idxa<idxc) && (idxa>-1))				
				{
					bProcessing=true;
					m_CurrentPos=idxa+4;
					if (arrNodes.Count<1)
					{
						ParseTree(arrNodes,chmFile);
					}
					else
					{
						ParseTree(((TOCItem)(arrNodes[arrNodes.Count-1])).Children,chmFile);
					}
					continue;
				}

				// new item
				if ((idxb<idxa) && (idxb<idxc) && (idxb>-1))
				{

					bProcessing=true;
					m_CurrentPos=idxb+4;
				
					int idx2=m_text1.IndexOf("<object",m_CurrentPos);
					if (idx2!=-1)
					{				
						int idx3=m_text1.IndexOf("</object>",idx2+7);
						if (idx3!=-1)
						{
							string text=m_text2.Substring(idx2,idx3-idx2);

							m_CurrentPos=idx3+9;							

							// Parse items in text.
							TOCItem tocItem=ParseItems(text, chmFile);							
							if (tocItem!=null)
							{
								arrNodes.Add(tocItem);
							}
						}
					}
				}

				// Undent
				if ((idxc<idxa) && (idxc<idxb) && (idxc>-1))
				{
					m_CurrentPos=idxc+5;
					bProcessing=true;
					return;
				}				
			}
			while (bProcessing);			
		}

		
		private static string RE_ParamBoundaries = @"\<param(?<innerText>.*?)\>";
		private const string RE_QuoteAttributes = @"( |\t)*(?<attributeName>[\-a-zA-Z0-9]*)( |\t)*=( |\t)*(?<attributeTD>[\""\'])?(?<attributeValue>.*?(?(attributeTD)\k<attributeTD>|([\s>]|.$)))";		
		private static Regex ParamRE;
		private static Regex AttributesRE;

		/// <summary>
		/// Parses tree nodes from the text
		/// </summary>
		/// <param name="itemstext">text containing the items</param>
		/// <param name="arrNodes">arraylist where the nodes should be added</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static TOCItem ParseItems( string itemstext, CHMFile chmFile)
		{			
			int innerPTextIdx = ParamRE.GroupNumberFromName("innerText");

			// get group-name indexes
			int nameIndex = AttributesRE.GroupNumberFromName("attributeName");
			int valueIndex = AttributesRE.GroupNumberFromName("attributeValue");
			int tdIndex = AttributesRE.GroupNumberFromName("attributeTD");

			TOCItem tocItem = new TOCItem();

			// read parameters
			int nParamIndex = 0;

			while( ParamRE.IsMatch(itemstext, nParamIndex) )
			{
				Match mP = ParamRE.Match(itemstext, nParamIndex);
				
				string innerP = mP.Groups[innerPTextIdx].Value;

				string paramName = "";
				string paramValue = "";

				int nAttrIdx = 0;

				while( AttributesRE.IsMatch( innerP, nAttrIdx ) )
				{
					Match mA = AttributesRE.Match(innerP, nAttrIdx);

					string attributeName = mA.Groups[nameIndex].Value;
					string attributeValue = mA.Groups[valueIndex].Value;
					string attributeTD = mA.Groups[tdIndex].Value;

					if(attributeTD.Length > 0)
					{
						// delete the trailing textqualifier
						if( attributeValue.Length > 0)
						{
							int ltqi = attributeValue.LastIndexOf( attributeTD );

							if(ltqi >= 0)
							{
								attributeValue = attributeValue.Substring(0,ltqi);
							}
						}
					}

					if( attributeName.ToLower() == "name")
					{
						paramName = attributeValue;
					}

					if( attributeName.ToLower() == "value")
					{
						paramValue = attributeValue;
					}

					nAttrIdx = mA.Index+mA.Length;
				}

				tocItem.Params[paramName] = paramValue;
				switch(paramName.ToLower())
				{
					case "name":
					{
						tocItem.Name = paramValue;
					};break;
					case "local":
					{
						tocItem.Local = paramValue;
					};break;
					case "imagenumber":
					{
						tocItem.ImageIndex = Int32.Parse(paramValue);

						if( tocItem.ImageIndex == 2)
							tocItem.ImageIndex = TOCItem.STD_FOLDER_HH1;
					};break;
				}

				nParamIndex = mP.Index+mP.Length;
			}

			tocItem.ChmFile = chmFile.ChmFilePath;
			return tocItem;
		}
	}
}
