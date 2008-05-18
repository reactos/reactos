using System;
using System.Configuration;
using TechBot.Library;

namespace TechBot.Console
{
	class MainClass
	{
        public static void Main(string[] args)
        {
            TechBotService m_TechBot = null;

            if (args.Length > 0 && args[0].ToLower().Equals("irc"))
            {
                m_TechBot = new IrcTechBotService(Settings.Default.IRCServerHostName,
                                                  Settings.Default.IRCServerHostPort,
                                                  Settings.Default.IRCChannelNames,
                                                  Settings.Default.IRCBotName,
                                                  Settings.Default.IRCBotPassword);
            }
            else
            {
                m_TechBot = new ConsoleTechBotService();
            }

            m_TechBot.Run();
        }
	}
}