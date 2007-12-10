using System;
using System.Configuration;
using TechBot.Library;

namespace TechBot.Console
{
	public class ConsoleServiceOutput : IServiceOutput
	{
		public void WriteLine(MessageContext context,
		                      string message)
		{
			System.Console.WriteLine(message);
		}
	}

	
	class MainClass
	{
		private static void VerifyRequiredOption(string optionName,
		                                         string optionValue)
		{
			if (optionValue == null)
			{
				throw new Exception(String.Format("Option '{0}' not set.",
				                                  optionName));
			}
		}
		
		private static string IRCServerHostName
		{
			get
			{
				string optionName = "IRCServerHostName";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static int IRCServerHostPort
		{
			get
			{
				string optionName = "IRCServerHostPort";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return Int32.Parse(s);
			}
		}

		private static string IRCChannelNames
		{
			get
			{
				string optionName = "IRCChannelNames";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string IRCBotName
		{
			get
			{
				string optionName = "IRCBotName";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string IRCBotPassword
		{
			get
			{
				string optionName = "IRCBotPassword";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string ChmPath
		{
			get
			{
				string optionName = "ChmPath";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}
		
		private static string MainChm
		{
			get
			{
				string optionName = "MainChm";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string NtstatusXml
		{
			get
			{
				string optionName = "NtstatusXml";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string WinerrorXml
		{
			get
			{
				string optionName = "WinerrorXml";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string HresultXml
		{
			get
			{
				string optionName = "HresultXml";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string WmXml
		{
			get
			{
				string optionName = "WmXml";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string SvnCommand
		{
			get
			{
				string optionName = "SvnCommand";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string BugUrl
		{
			get
			{
				string optionName = "BugUrl";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}

		private static string WineBugUrl
		{
			get
			{
				string optionName = "WineBugUrl";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}


		private static string SambaBugUrl
		{
			get
			{
				string optionName = "SambaBugUrl";
				string s = ConfigurationSettings.AppSettings[optionName];
				VerifyRequiredOption(optionName,
				                     s);
				return s;
			}
		}


		private static void RunIrcService()
		{
			IrcService ircService = new IrcService(IRCServerHostName,
			                                       IRCServerHostPort,
			                                       IRCChannelNames,
			                                       IRCBotName,
			                                       IRCBotPassword,
			                                       ChmPath,
			                                       MainChm,
                                                   //NtstatusXml,
                                                   //WinerrorXml,
                                                   //HresultXml,
                                                   //WmXml,
			                                       //SvnCommand,
			                                       BugUrl,
			                                       WineBugUrl,
			                                       SambaBugUrl);
			ircService.Run();
		}
		
		public static void Main(string[] args)
		{
			if (args.Length > 0 && args[0].ToLower().Equals("irc"))
			{
				RunIrcService();
				return;
			}
			
			System.Console.WriteLine("TechBot running console service...");
			TechBotService service = new TechBotService(new ConsoleServiceOutput(),
			                                            ChmPath,
			                                            MainChm);
                                                        //NtstatusXml,
                                                        //WinerrorXml,
                                                        //HresultXml,
                                                        //WmXml,
			                                            //SvnCommand,
                                                        //BugUrl,
                                                        //WineBugUrl,
                                                        //SambaBugUrl);
			service.Run();
			while (true)
			{
				string s = System.Console.ReadLine();
				service.InjectMessage(null,
				                      s);
			}
		}
	}
}
