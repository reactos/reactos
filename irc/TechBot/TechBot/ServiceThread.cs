using System;
using System.Configuration;
using System.Diagnostics;
using TechBot.Library;

namespace TechBot
{
	public class ServiceThread
	{
		private string IRCServerHostName;
		private int IRCServerHostPort;
		private string IRCChannelNames;
		private string IRCBotName;
		private string IRCBotPassword;
		private string ChmPath;
		private string MainChm;
		private string NtstatusXml;
		private string HresultXml;
		private string WmXml;
		private string WinerrorXml;
		private string SvnCommand;
		private string BugUrl, WineBugUrl, SambaBugUrl;
		private EventLog eventLog;
		
		public ServiceThread(EventLog eventLog)
		{
			this.eventLog = eventLog;
		}
		
		private void SetupConfiguration()
		{
			IRCServerHostName = ConfigurationSettings.AppSettings["IRCServerHostName"];
			IRCServerHostPort = Int32.Parse(ConfigurationSettings.AppSettings["IRCServerHostPort"]);
			IRCChannelNames = ConfigurationSettings.AppSettings["IRCChannelNames"];
			IRCBotName = ConfigurationSettings.AppSettings["IRCBotName"];
			IRCBotPassword = ConfigurationSettings.AppSettings["IRCBotPassword"];
			ChmPath = ConfigurationSettings.AppSettings["ChmPath"];
			MainChm = ConfigurationSettings.AppSettings["MainChm"];
			NtstatusXml = ConfigurationSettings.AppSettings["NtstatusXml"];
			HresultXml = ConfigurationSettings.AppSettings["HresultXml"];
			WmXml = ConfigurationSettings.AppSettings["WmXml"];
			WinerrorXml = ConfigurationSettings.AppSettings["WinerrorXml"];
			SvnCommand = ConfigurationSettings.AppSettings["SvnCommand"];
			BugUrl = ConfigurationSettings.AppSettings["BugUrl"];
			WineBugUrl = ConfigurationSettings.AppSettings["WineBugUrl"];
			SambaBugUrl = ConfigurationSettings.AppSettings["SambaBugUrl"];
		}
		
		public void Run()
		{
			SetupConfiguration();
			System.Console.WriteLine("TechBot irc service...");
			
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
		
		public void Start()
		{
			try
			{
				Run();
			}
			catch (Exception ex)
			{
				eventLog.WriteEntry(String.Format("Ex. {0}", ex));
			}
		}
	}
}
