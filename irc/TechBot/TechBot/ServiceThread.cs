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
		private string ChmPath;
		private string MainChm;
		private string NtstatusXml;
		private string HresultXml;
		private string WinerrorXml;
		private string SvnCommand;
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
			ChmPath = ConfigurationSettings.AppSettings["ChmPath"];
			MainChm = ConfigurationSettings.AppSettings["MainChm"];
			NtstatusXml = ConfigurationSettings.AppSettings["NtstatusXml"];
			HresultXml = ConfigurationSettings.AppSettings["HresultXml"];
			WinerrorXml = ConfigurationSettings.AppSettings["WinerrorXml"];
			SvnCommand = ConfigurationSettings.AppSettings["SvnCommand"];
		}
		
		public void Run()
		{
			SetupConfiguration();
			System.Console.WriteLine("TechBot irc service...");
			
			IrcService ircService = new IrcService(IRCServerHostName,
			                                       IRCServerHostPort,
			                                       IRCChannelNames,
			                                       IRCBotName,
			                                       ChmPath,
			                                       MainChm,
			                                       NtstatusXml,
			                                       WinerrorXml,
			                                       HresultXml,
			                                       SvnCommand);
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
