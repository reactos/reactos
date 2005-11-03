using System;
using System.Collections;
using System.Text;
using System.Text.RegularExpressions;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>HHCParser</c> implements a parser for HHC contents files.
	/// </summary>
	internal sealed class HHCParser
	{
		/// <summary>
		/// regular expressions for replacing the sitemap boundary tags 
		/// </summary>
		private static string RE_ULOpening = @"\<ul\>";  // will be replaced by a '(' for nested parsing
		private static string RE_ULClosing = @"\</ul\>"; // will be replaced by a ')' for nested parsing

		/// <summary>
		/// Matching ul-tags
		/// </summary>
		private static string RE_ULBoundaries = @"\<ul\>(?<innerText>.*)\</ul\>";
		/// <summary>
		/// Matching the nested tree structure.
		/// </summary>
		private static string RE_NestedBoundaries = @"\( (?> [^()]+ | \( (?<DEPTH>) | \) (?<-DEPTH>) )* (?(DEPTH)(?!)) \)";
		/// <summary>
		/// Matching object-tags
		/// </summary>
		private static string RE_ObjectBoundaries = @"\<object(?<innerText>.*?)\</object\>";
		/// <summary>
		/// Matching param tags
		/// </summary>
		private static string RE_ParamBoundaries = @"\<param(?<innerText>.*?)\>";
		/// <summary>
		/// Extracting tag attributes
		/// </summary>
		private const string RE_QuoteAttributes = @"( |\t)*(?<attributeName>[\-a-zA-Z0-9]*)( |\t)*=( |\t)*(?<attributeTD>[\""\'])?(?<attributeValue>.*?(?(attributeTD)\k<attributeTD>|([\s>]|.$)))";

		/// <summary>
		/// private regular expressionobjects 
		/// </summary>
		private static Regex ulRE;
		private static Regex NestedRE;
		private static Regex ObjectRE;
		private static Regex ParamRE;
		private static Regex AttributesRE;

		/// <summary>
		/// Internal member storing the list of TOCItems which are holding merge links
		/// </summary>
		private static ArrayList _mergeItems = null;

		/// <summary>
		/// Internal member storing the last read regular topic item.
		/// This is used to handle "Merge" entries and add them as child to this instance.
		/// </summary>
		private static TOCItem _lastTopicItem = null;

		/// <summary>
		/// Parses a HHC file and returns an ArrayList with the table of contents (TOC) tree
		/// </summary>
		/// <param name="hhcFile">string content of the hhc file</param>
		/// <param name="chmFile">CHMFile instance</param>
		/// <returns>Returns an ArrayList with the table of contents (TOC) tree</returns>
		public static ArrayList ParseHHC(string hhcFile, CHMFile chmFile)
		{
			_lastTopicItem = null;
			_mergeItems = null; // clear merged item list
			ArrayList tocList = new ArrayList();

			ulRE = new Regex(RE_ULBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			NestedRE = new Regex(RE_NestedBoundaries, RegexOptions.IgnorePatternWhitespace | RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			ObjectRE = new Regex(RE_ObjectBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			ParamRE = new Regex(RE_ParamBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			AttributesRE = new Regex(RE_QuoteAttributes, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

			int innerTextIdx = ulRE.GroupNumberFromName("innerText");

			if( ulRE.IsMatch(hhcFile, 0) )
			{
				Match m = ulRE.Match(hhcFile, 0);

				int nFirstUL = 0;

				nFirstUL = hhcFile.ToLower().IndexOf("<ul>");

				if(nFirstUL == -1)
					nFirstUL = hhcFile.ToLower().IndexOf("<il>");

				if( ObjectRE.IsMatch(hhcFile, 0) ) // first object block contains information types and categories
				{
					Match mO = ObjectRE.Match(hhcFile, 0);
					int iOTxt = ObjectRE.GroupNumberFromName("innerText");

					string globalText = mO.Groups[iOTxt].Value;

					if( mO.Groups[iOTxt].Index <= nFirstUL)
						ParseGlobalSettings( globalText, chmFile );
				}

				// parse toc tree
				string innerText = m.Groups["innerText"].Value;

				innerText = innerText.Replace("(", "&#040;");
				innerText = innerText.Replace(")", "&#041;");
				innerText = Regex.Replace(innerText, RE_ULOpening, "(", RegexOptions.IgnoreCase);
				innerText = Regex.Replace(innerText, RE_ULClosing, ")", RegexOptions.IgnoreCase);
			
				ParseTree( innerText, null, tocList, chmFile );
				
			}

			return tocList;
		}

		/// <summary>
		/// Checks if the hhc file contains a global object tag.
		/// </summary>
		/// <param name="hhcFile">string content of the hhc file</param>
		/// <param name="chmFile">chm file</param>
		/// <returns>true if the hhc content contains a global object tag</returns>
		public static bool HasGlobalObjectTag(string hhcFile, CHMFile chmFile)
		{
			bool bRet = false;

			ulRE = new Regex(RE_ULBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			ObjectRE = new Regex(RE_ObjectBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

			int innerTextIdx = ulRE.GroupNumberFromName("innerText");

			if( ulRE.IsMatch(hhcFile, 0) )
			{
				Match m = ulRE.Match(hhcFile, 0);

				int nFirstUL = 0;

				nFirstUL = hhcFile.ToLower().IndexOf("<ul>");

				if(nFirstUL == -1)
					nFirstUL = hhcFile.ToLower().IndexOf("<il>");

				if( ObjectRE.IsMatch(hhcFile, 0) ) // first object block contains information types and categories
				{
					Match mO = ObjectRE.Match(hhcFile, 0);
					int iOTxt = ObjectRE.GroupNumberFromName("innerText");

					string globalText = mO.Groups[iOTxt].Value;

					if( mO.Groups[iOTxt].Index <= nFirstUL)
						bRet = true;
				}
			}

			return bRet;
		}

		/// <summary>
		/// Gets true if the previously done parsing found merge-links
		/// </summary>
		public static bool HasMergeLinks
		{
			get 
			{
				if(_mergeItems==null)
					return false;

				return _mergeItems.Count > 0;
			}
		}

		/// <summary>
		/// Gets all TOCItem references which are holding merge-links
		/// </summary>
		public static ArrayList MergeItems
		{
			get { return _mergeItems; }
		}

		/// <summary>
		/// Recursively parses a sitemap tree
		/// </summary>
		/// <param name="text">content text</param>
		/// <param name="parent">Parent for all read items</param>
		/// <param name="arrNodes">arraylist which receives the extracted nodes</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static void ParseTree( string text, TOCItem parent, ArrayList arrNodes, CHMFile chmFile )
		{
			string strPreItems="", strPostItems="";
			string innerText = "";

			int nIndex = 0;

			while( NestedRE.IsMatch(text, nIndex) )
			{
				Match m = NestedRE.Match(text, nIndex);

				innerText = m.Value.Substring( 1, m.Length-2);

				strPreItems = text.Substring(nIndex,m.Index-nIndex);

				ParseItems(strPreItems, parent, arrNodes, chmFile);

				if((arrNodes.Count>0) && (innerText.Length > 0) )
				{
					TOCItem p = ((TOCItem)(arrNodes[arrNodes.Count-1]));
					ParseTree( innerText, p, p.Children, chmFile );
				}

				nIndex = m.Index+m.Length;
			} 

			if( nIndex == 0)
			{
				strPostItems = text.Substring(nIndex, text.Length-nIndex);
				ParseItems(strPostItems, parent, arrNodes, chmFile);
			} 
			else if( nIndex < text.Length-1)
			{
				strPostItems = text.Substring(nIndex, text.Length-nIndex);
				ParseTree(strPostItems, parent, arrNodes, chmFile);
			}
		}

		/// <summary>
		/// Parses tree nodes from the text
		/// </summary>
		/// <param name="itemstext">text containing the items</param>
		/// <param name="parent">Parent for all read items</param>
		/// <param name="arrNodes">arraylist where the nodes should be added</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static void ParseItems( string itemstext, TOCItem parent, ArrayList arrNodes, CHMFile chmFile)
		{
			int innerTextIdx = ObjectRE.GroupNumberFromName("innerText");
			int innerPTextIdx = ParamRE.GroupNumberFromName("innerText");

			// get group-name indexes
			int nameIndex = AttributesRE.GroupNumberFromName("attributeName");
			int valueIndex = AttributesRE.GroupNumberFromName("attributeValue");
			int tdIndex = AttributesRE.GroupNumberFromName("attributeTD");

			int nObjStartIndex = 0;

			while( ObjectRE.IsMatch(itemstext, nObjStartIndex) )
			{
				Match m = ObjectRE.Match(itemstext, nObjStartIndex);

				string innerText = m.Groups[innerTextIdx].Value;

				TOCItem tocItem = new TOCItem();
				tocItem.TocMode = DataMode.TextBased;
				tocItem.AssociatedFile = chmFile;
				tocItem.Parent = parent;

				// read parameters
				int nParamIndex = 0;

				while( ParamRE.IsMatch(innerText, nParamIndex) )
				{
					Match mP = ParamRE.Match(innerText, nParamIndex);
					
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
							paramName = HttpUtility.HtmlDecode(attributeValue); // for unicode encoded values
						}

						if( attributeName.ToLower() == "value")
						{
							paramValue = HttpUtility.HtmlDecode(attributeValue); // for unicode encoded values
							// delete trailing /
							while((paramValue.Length>0)&&(paramValue[paramValue.Length-1] == '/'))
								paramValue = paramValue.Substring(0,paramValue.Length-1);
							
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
							tocItem.Local = paramValue.Replace("../", "").Replace("./", "");
						};break;
						case "imagenumber":
						{
							tocItem.ImageIndex = Int32.Parse(paramValue);
							tocItem.ImageIndex-=1;

							int nFolderAdd = 0;

							if((chmFile != null) && (chmFile.ImageTypeFolder))
							{
								// get the value which should be added, to display folders instead of books
								if(HtmlHelpSystem.UseHH2TreePics) 
									nFolderAdd = 8;
								else
									nFolderAdd = 4;
							}

							if(tocItem.ImageIndex%2 != 0)
							{
								if(tocItem.ImageIndex==1)
									tocItem.ImageIndex=0;
							}
							if(HtmlHelpSystem.UseHH2TreePics)
								if( tocItem.ImageIndex == 0)
									tocItem.ImageIndex = TOCItem.STD_FOLDER_HH2+nFolderAdd;
						};break;
						case "merge": // this item contains topics or a full TOC from a merged CHM
						{
							tocItem.MergeLink = paramValue;

							// "register" this item as merge-link
							if(_mergeItems==null)
								_mergeItems=new ArrayList();

							_mergeItems.Add(tocItem);

						};break;
						case "type":	// information type assignment for item
						{
							tocItem.InfoTypeStrings.Add( paramValue );
						};break;
					}

					nParamIndex = mP.Index+mP.Length;
				}

				tocItem.ChmFile = chmFile.ChmFilePath;

				if(tocItem.MergeLink.Length > 0)
				{
					if(_lastTopicItem != null)
					{
						tocItem.Parent = _lastTopicItem;
						_lastTopicItem.Children.Add(tocItem);
					}
					else
						arrNodes.Add( tocItem );
				} 
				else 
				{
					_lastTopicItem = tocItem;
					arrNodes.Add( tocItem );
				}

				nObjStartIndex = m.Index+m.Length;
			}
		}

		/// <summary>
		/// Parses the very first &lt;OBJECT&gt; tag in the sitemap file and extracts 
		/// information types and categories.
		/// </summary>
		/// <param name="sText">text of the object tag</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static void ParseGlobalSettings(string sText, CHMFile chmFile)
		{
			int innerPTextIdx = ParamRE.GroupNumberFromName("innerText");

			// get group-name indexes
			int nameIndex = AttributesRE.GroupNumberFromName("attributeName");
			int valueIndex = AttributesRE.GroupNumberFromName("attributeValue");
			int tdIndex = AttributesRE.GroupNumberFromName("attributeTD");

			// read parameters
			int nParamIndex = 0;
			
			// 0... unknown
			// 1... inclusinve info type name
			// 2... exclusive info type name
			// 3... hidden info type name
			// 4... category name
			// 5... incl infotype name for category
			// 6... excl infotype name for category
			// 7... hidden infotype name for category
			int prevItem = 0;

			string sName = "";
			string sDescription = "";
			string curCategory = "";

			while( ParamRE.IsMatch(sText, nParamIndex) )
			{
				Match mP = ParamRE.Match(sText, nParamIndex);
					
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
						paramName = HttpUtility.HtmlDecode(attributeValue); // for unicode encoded values
					}

					if( attributeName.ToLower() == "value")
					{
						paramValue = HttpUtility.HtmlDecode(attributeValue); // for unicode encoded values
						// delete trailing /
						while((paramValue.Length>0)&&(paramValue[paramValue.Length-1] == '/'))
							paramValue = paramValue.Substring(0,paramValue.Length-1);
							
					}

					nAttrIdx = mA.Index+mA.Length;
				}

				switch(paramName.ToLower())
				{
					case "savetype":	// inclusive information type name
					{
						prevItem = 1;
						sName = paramValue;
					};break;
					case "savetypedesc":	// description of information type
					{
						InformationTypeMode mode = InformationTypeMode.Inclusive;
						sDescription = paramValue;

						if( prevItem == 1)
							mode = InformationTypeMode.Inclusive;
						if( prevItem == 2)
							mode = InformationTypeMode.Exclusive;
						if( prevItem == 3)
							mode = InformationTypeMode.Hidden;

						if( chmFile.GetInformationType( sName ) == null)
						{
							// check if the HtmlHelpSystem already holds such an information type
							if( chmFile.SystemInstance.GetInformationType( sName ) == null)
							{
								// info type not found yet

								InformationType newType = new InformationType(sName, sDescription, mode);
								chmFile.InformationTypes.Add(newType);
							} 
							else 
							{
								InformationType sysType = chmFile.SystemInstance.GetInformationType( sName );
								chmFile.InformationTypes.Add( sysType );
							}
						}

						prevItem = 0;
					};break;
					case "saveexclusive":	// exclusive information type name
					{
						prevItem = 2;
						sName = paramValue;
					};break;
					case "savehidden":	// hidden information type name
					{
						prevItem = 3;
						sName = paramValue;
					};break;
					case "category":	// category name
					{
						prevItem = 4;
						sName = paramValue;
						curCategory = sName;
					};break;
					case "categorydesc":	// category description
					{
						sDescription = paramValue;

						if( chmFile.GetCategory( sName ) == null)
						{
							// check if the HtmlHelpSystem already holds such a category
							if( chmFile.SystemInstance.GetCategory( sName ) == null)
							{
								// add category 
								Category newCat = new Category(sName, sDescription);
								chmFile.Categories.Add(newCat);
							} 
							else 
							{
								Category sysCat = chmFile.SystemInstance.GetCategory( sName );
								chmFile.Categories.Add( sysCat );
							}
						}

						prevItem = 0;
					};break;
					case "type":	// inclusive information type which is member of the previously read category
					{
						prevItem = 5;
						sName = paramValue;
					};break;
					case "typedesc":	// description of type for category
					{
						sDescription = paramValue;
						Category cat = chmFile.GetCategory( curCategory );

						if( cat != null)
						{
							// category found
							InformationType infoType = chmFile.GetInformationType( sName );
							
							if( infoType != null)
							{
								if( !cat.ContainsInformationType(infoType))
								{
									infoType.SetCategoryFlag(true);
									cat.AddInformationType(infoType);
								}
							}
						}

						prevItem = 0;
					};break;
					case "typeexclusive":	// exclusive information type which is member of the previously read category
					{
						prevItem = 6;
						sName = paramValue;
					};break;
					case "typehidden":	// hidden information type which is member of the previously read category
					{
						prevItem = 7;
						sName = paramValue;
					};break;
					default:
					{
						prevItem = 0;
						sName = "";
						sDescription = "";
					};break;
				}

				nParamIndex = mP.Index+mP.Length;
			}
		}
	}
}
