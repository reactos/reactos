using System;
using System.IO;
using System.Collections;
using System.Text;
using System.Text.RegularExpressions;

namespace HtmlHelp.ChmDecoding
{
	/// <summary>
	/// The class <c>HHKParser</c> implements a parser for HHK contents files.
	/// </summary>
	internal sealed class HHKParser
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
		/// Parses a HHK file and returns an ArrayList with the index tree
		/// </summary>
		/// <param name="hhkFile">string content of the hhk file</param>
		/// <param name="chmFile">CHMFile instance</param>
		/// <returns>Returns an ArrayList with the index tree</returns>
		public static ArrayList ParseHHK(string hhkFile, CHMFile chmFile)
		{
			ArrayList indexList = new ArrayList();

			ulRE = new Regex(RE_ULBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			NestedRE = new Regex(RE_NestedBoundaries, RegexOptions.IgnorePatternWhitespace | RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			ObjectRE = new Regex(RE_ObjectBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			ParamRE = new Regex(RE_ParamBoundaries, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);
			AttributesRE = new Regex(RE_QuoteAttributes, RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

			int innerTextIdx = ulRE.GroupNumberFromName("innerText");

			if( ulRE.IsMatch(hhkFile, 0) )
			{
				Match m = ulRE.Match(hhkFile, 0);

				if( ObjectRE.IsMatch(hhkFile, 0) ) // first object block contains information types and categories
				{
					Match mO = ObjectRE.Match(hhkFile, 0);
					int iOTxt = ObjectRE.GroupNumberFromName("innerText");

					string globalText = mO.Groups[iOTxt].Value;

					ParseGlobalSettings( globalText, chmFile );
				}

				string innerText = m.Groups["innerText"].Value;

				innerText = innerText.Replace("(", "&#040;");
				innerText = innerText.Replace(")", "&#041;");
				innerText = Regex.Replace(innerText, RE_ULOpening, "(", RegexOptions.IgnoreCase);
				innerText = Regex.Replace(innerText, RE_ULClosing, ")", RegexOptions.IgnoreCase);
			
				ParseTree( innerText, null, indexList, chmFile );
			}

			return indexList;
		}

		/// <summary>
		/// Recursively parses a sitemap tree
		/// </summary>
		/// <param name="text">content text</param>
		/// <param name="parent">Parent for all read items</param>
		/// <param name="arrNodes">arraylist which receives the extracted nodes</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static void ParseTree( string text, IndexItem parent, ArrayList arrNodes, CHMFile chmFile )
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
					IndexItem p = ((IndexItem)(arrNodes[arrNodes.Count-1]));
					ParseTree( innerText, p, arrNodes, chmFile );
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
		/// Parses nodes from the text
		/// </summary>
		/// <param name="itemstext">text containing the items</param>
		/// <param name="parentItem">parent index item</param>
		/// <param name="arrNodes">arraylist where the nodes should be added</param>
		/// <param name="chmFile">CHMFile instance</param>
		private static void ParseItems( string itemstext, IndexItem parentItem, ArrayList arrNodes, CHMFile chmFile)
		{
			int innerTextIdx = ObjectRE.GroupNumberFromName("innerText");
			int innerPTextIdx = ParamRE.GroupNumberFromName("innerText");

			// get group-name indexes
			int nameIndex = AttributesRE.GroupNumberFromName("attributeName");
			int valueIndex = AttributesRE.GroupNumberFromName("attributeValue");
			int tdIndex = AttributesRE.GroupNumberFromName("attributeTD");

			int nObjStartIndex = 0;
			int nLastObjStartIndex = 0;
			string sKeyword = "";

			while( ObjectRE.IsMatch(itemstext, nObjStartIndex) )
			{
				Match m = ObjectRE.Match(itemstext, nObjStartIndex);

				string innerText = m.Groups[innerTextIdx].Value;

				IndexItem idxItem = new IndexItem();

				// read parameters
				int nParamIndex = 0;
				int nNameCnt = 0;

				string paramTitle = "";
				string paramLocal = "";
				bool bAdded = false;

				while( ParamRE.IsMatch(innerText, nParamIndex) )
				{
					Match mP = ParamRE.Match(innerText, nParamIndex);
					
					string innerP = mP.Groups[innerPTextIdx].Value;

					string paramName = "";
					string paramValue = "";

					int nAttrIdx = 0;
					//sKeyword = "";

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
							nNameCnt++;
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

					if( nNameCnt == 1) // first "Name" param = keyword
					{
						sKeyword = "";

						if(parentItem != null)
							sKeyword = parentItem.KeyWordPath + ",";

						string sOldKW = sKeyword;

						sKeyword += paramValue;

						IndexItem idxFind = FindByKeyword(arrNodes, sKeyword);

						if(idxFind != null)
						{
							idxItem = idxFind;
						}
						else
						{
							if( sKeyword.Split(new char[] {','}).Length > 1 )
							{
								idxItem.CharIndex = sKeyword.Length - paramValue.Length;
							}
							else
							{
								sKeyword = paramValue;
								sOldKW = sKeyword;
								idxItem.CharIndex = 0;
							}

							idxItem.KeyWordPath = sKeyword;
							idxItem.Indent = sKeyword.Split(new char[] {','}).Length - 1;
							idxItem.IsSeeAlso = false;
							
							sKeyword = sOldKW;
						}
					} 
					else 
					{

						if( (nNameCnt > 2) && (paramName.ToLower()=="name") )
						{
							bAdded = true;
							IndexTopic idxTopic = new IndexTopic(paramTitle, paramLocal, chmFile.CompileFile, chmFile.ChmFilePath);

							idxItem.Topics.Add( idxTopic );

							paramTitle = "";
							paramLocal = "";
						} 

						switch(paramName.ToLower())
						{
							case "name":
								//case "keyword":
							{
								paramTitle = paramValue;
							};break;
							case "local":
							{
								paramLocal = paramValue.Replace("../", "").Replace("./", "");
							};break;
							case "type":	// information type assignment for item
							{
								idxItem.InfoTypeStrings.Add( paramValue );
							};break;
							case "see also":
							{
								idxItem.AddSeeAlso(paramValue);
								idxItem.IsSeeAlso = true;
								bAdded = true;
							};break;
						}
					}
					
					nParamIndex = mP.Index+mP.Length;
				}

				if(!bAdded)
				{
					bAdded=false;
					IndexTopic idxTopic = new IndexTopic(paramTitle, paramLocal, chmFile.CompileFile, chmFile.ChmFilePath);

					idxItem.Topics.Add( idxTopic );

					paramTitle = "";
					paramLocal = "";
				}

				idxItem.ChmFile = chmFile;
				arrNodes.Add( idxItem );

				nLastObjStartIndex = nObjStartIndex;
				nObjStartIndex = m.Index+m.Length;
			}
		}

		/// <summary>
		/// Searches an index-keyword in the index list
		/// </summary>
		/// <param name="indexList">index list to search</param>
		/// <param name="Keyword">keyword to find</param>
		/// <returns>Returns an <see cref="IndexItem">IndexItem</see> instance if found, otherwise null.</returns>
		private static IndexItem FindByKeyword(ArrayList indexList, string Keyword)
		{
			foreach(IndexItem curItem in indexList)
			{
				if( curItem.KeyWordPath == Keyword)
					return curItem;
			}

			return null;
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
