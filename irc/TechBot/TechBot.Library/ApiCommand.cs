using System;
using System.IO;
using System.Data;
using System.Text.RegularExpressions;
using HtmlHelp;
using HtmlHelp.ChmDecoding;

namespace TechBot.Library
{
	public class ApiCommand : Command
	{
		private const bool IsVerbose = false;

		private HtmlHelpSystem chm;
		private string chmPath;
		private string mainChm;

        public ApiCommand(TechBotService techBot)
            : base(techBot)
		{
			Run();
		}
		
		private void WriteIfVerbose(MessageContext context,
		                            string message)
		{
			if (IsVerbose)
                TechBot.ServiceOutput.WriteLine(context,
				                        message);
		}

		private void Run()
		{
			string CHMFilename = Path.Combine(chmPath, mainChm);
			chm = new HtmlHelpSystem();
			chm.OpenFile(CHMFilename, null);
			
			Console.WriteLine(String.Format("Loaded main CHM: {0}",
			                                Path.GetFileName(CHMFilename)));
			foreach (string filename in Directory.GetFiles(chmPath))
			{
				if (!Path.GetExtension(filename).ToLower().Equals(".chm"))
					continue;
				if (Path.GetFileName(filename).ToLower().Equals(mainChm))
					continue;

				Console.WriteLine(String.Format("Loading CHM: {0}",
				                                Path.GetFileName(filename)));
				try
				{
					chm.MergeFile(filename);
				}
				catch (Exception ex)
				{
					Console.WriteLine(String.Format("Could not load CHM: {0}. Exception {1}",
					                                Path.GetFileName(filename),
					                                ex));
				}
			}
			Console.WriteLine(String.Format("Loaded {0} CHMs",
			                                chm.FileList.Length));
		}

        public override string[] AvailableCommands
        {
            get { return new string[] { "api" }; }
        }

        /*
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "api" });
		}
*/
		
		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			if (parameters.Trim().Equals(String.Empty))
				DisplayNoKeyword(context);
			else
				Search(context,
				       parameters);
		}

        public override string Help()
		{
			return "!api <apiname>";
		}
		
		private bool SearchIndex(MessageContext context,
		                         string keyword)
		{
			if (chm.HasIndex)
			{
				IndexItem item = chm.Index.SearchIndex(keyword,
				                                       IndexType.KeywordLinks);
				if (item != null && item.Topics.Count > 0)
				{
					WriteIfVerbose(context,
					               String.Format("Keyword {0} found in index",
					                             item.KeyWord));
					IndexTopic indexTopic = item.Topics[0] as IndexTopic;
					return DisplayResult(context,
					                     keyword,
					                     indexTopic);
				}
				else
				{
					WriteIfVerbose(context,
					               String.Format("Keyword {0} not found in index",
					                             keyword));
					return false;
				}
			}
			else
				return false;
		}

		private void SearchFullText(MessageContext context,
		                            string keyword)
		{
			string sort = "Rating ASC";
			WriteIfVerbose(context,
			               String.Format("Searching fulltext database for {0}",
                                         keyword));

			bool partialMatches = false;
			bool titlesOnly = true;
			int maxResults = 100;
			DataTable results = chm.PerformSearch(keyword,
			                                      maxResults,
			                                      partialMatches,
			                                      titlesOnly);
			WriteIfVerbose(context,
			               String.Format("results.Rows.Count = {0}",
			                             results != null ?
			                             results.Rows.Count.ToString() : "(none)"));
			if (results != null && results.Rows.Count > 0)
			{
				results.DefaultView.Sort = sort;
				if (!DisplayResult(context,
				                   keyword,
				                   results))
				{
					DisplayNoResult(context,
					                keyword);
				}
			}
			else
			{
				DisplayNoResult(context,
				                keyword);
			}
		}

		private void Search(MessageContext context,
		                    string keyword)
		{
			if (!SearchIndex(context,
			                 keyword))
				SearchFullText(context,
				               keyword);
		}
		
		private bool DisplayResult(MessageContext context,
		                           string keyword,
		                           IndexTopic indexTopic)
		{
			keyword = keyword.Trim().ToLower();
			string url = indexTopic.URL;
			WriteIfVerbose(context,
			               String.Format("URL from index search {0}",
                                         url));
			string prototype = ExtractPrototype(context,
			                                    url);
			if (prototype == null || prototype.Trim().Equals(String.Empty))
				return false;
			string formattedPrototype = FormatPrototype(prototype);
            TechBot.ServiceOutput.WriteLine(context,
			                        formattedPrototype);
			return true;
		}
		
		private bool DisplayResult(MessageContext context,
		                           string keyword,
		                           DataTable results)
		{
			keyword = keyword.Trim().ToLower();
			for (int i = 0; i < results.DefaultView.Count; i++)
			{
				DataRowView row = results.DefaultView[i];
				string title = row["Title"].ToString();
				WriteIfVerbose(context,
				               String.Format("Examining {0}", title));
				if (title.Trim().ToLower().Equals(keyword))
				{
					string location = row["Location"].ToString();
					string rating = row["Rating"].ToString();
					string url = row["Url"].ToString();
					string prototype = ExtractPrototype(context,
					                                    url);
					if (prototype == null || prototype.Trim().Equals(String.Empty))
						continue;
					string formattedPrototype = FormatPrototype(prototype);
                    TechBot.ServiceOutput.WriteLine(context,
					                        formattedPrototype);
					return true;
				}
			}
			return false;
		}

		private void DisplayNoResult(MessageContext context,
		                             string keyword)
		{
            TechBot.ServiceOutput.WriteLine(context,
			                        String.Format("I don't know about keyword {0}",
			                                      keyword));
		}

		private void DisplayNoKeyword(MessageContext context)
		{
            TechBot.ServiceOutput.WriteLine(context,
			                        "Please give me a keyword.");
		}

		private string ReplaceComments(string s)
		{
			return Regex.Replace(s, "//(.+)\r\n", "");
		}

		private string ReplaceLineEndings(string s)
		{
			return Regex.Replace(s, "(\r\n)+", " ");
		}

		private string ReplaceSpaces(string s)
		{
			return Regex.Replace(s, @" +", " ");
		}
		
		private string ReplaceSpacesBeforeLeftParenthesis(string s)
		{
			return Regex.Replace(s, @"\( ", @"(");
		}

		private string ReplaceSpacesBeforeRightParenthesis(string s)
		{
			return Regex.Replace(s, @" \)", @")");
		}

		private string ReplaceSemicolon(string s)
		{
			return Regex.Replace(s, @";", @"");
		}

		private string FormatPrototype(string prototype)
		{
			string s = ReplaceComments(prototype);
			s = ReplaceLineEndings(s);
			s = ReplaceSpaces(s);
			s = ReplaceSpacesBeforeLeftParenthesis(s);
			s = ReplaceSpacesBeforeRightParenthesis(s);
			s = ReplaceSemicolon(s);
			return s;
		}
		
		private string ExtractPrototype(MessageContext context,
		                                string url)
		{
			string page = GetPage(context,
			                      url);
			Match match = Regex.Match(page,
			                          "<PRE class=\"?syntax\"?>(.+)</PRE>",
			                          RegexOptions.Multiline |
			                          RegexOptions.Singleline);
			if (match.Groups.Count > 1)
			{
				string prototype = match.Groups[1].ToString();
				return StripHtml(StripAfterSlashPre(prototype));
			}
			
			return "";
		}
		
		private string StripAfterSlashPre(string html)
		{
			int index = html.IndexOf("</PRE>");
			if (index != -1)
			{
				return html.Substring(0, index);
			}
			else
				return html;
		}
		
		private string StripHtml(string html)
		{
			return Regex.Replace(html, @"<(.|\n)*?>", String.Empty);
		}

		private string GetPage(MessageContext context,
		                       string url)
		{
			string CHMFileName = "";
			string topicName = "";
			string anchor = "";
			CHMStream.CHMStream baseStream;
			if (!chm.BaseStream.GetCHMParts(url, ref CHMFileName, ref topicName, ref anchor))
			{
				baseStream = chm.BaseStream;
				CHMFileName = baseStream.CHMFileName;
				topicName = url;
				anchor = "";
			}
			else
			{
				baseStream = GetBaseStreamFromCHMFileName(context,
				                                          CHMFileName);
			}

			if ((topicName == "") || (CHMFileName == "") || (baseStream == null))
			{
				return "";
			}

			return baseStream.ExtractTextFile(topicName);
		}

		private CHMStream.CHMStream GetBaseStreamFromCHMFileName(MessageContext context,
		                                                         string CHMFileName)
		{
			foreach (CHMFile file in chm.FileList)
			{
				WriteIfVerbose(context,
				               String.Format("Compare: {0} <> {1}",
				                             file.ChmFilePath,
				                             CHMFileName));
				if (file.ChmFilePath.ToLower().Equals(CHMFileName.ToLower()))
				{
					return file.BaseStream;
				}
			}
			WriteIfVerbose(context,
			               String.Format("Could not find loaded CHM file in list: {0}",
			                             CHMFileName));
			return null;
		}
	}
}
