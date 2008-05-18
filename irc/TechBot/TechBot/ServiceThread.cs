using System;
using System.Configuration;
using System.Diagnostics;
using TechBot.Library;

namespace TechBot
{
	public class ServiceThread
	{
		private EventLog m_EventLog;
		
		public ServiceThread(EventLog eventLog)
		{
			m_EventLog = eventLog;
		}

        public void Run()
        {
            System.Console.WriteLine("TechBot irc service...");

            IrcTechBotService ircService = new IrcTechBotService(
                Settings.Default.IRCServerHostName,
                Settings.Default.IRCServerHostPort,
                Settings.Default.IRCChannelNames,
                Settings.Default.IRCBotName,
                Settings.Default.IRCBotPassword);

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
				m_EventLog.WriteEntry(String.Format("Ex. {0}", ex));
			}
		}
	}
}
