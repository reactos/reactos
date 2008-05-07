using System;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public abstract class MessageContext
	{
	}

	public class ChannelMessageContext : MessageContext
	{
		private IrcChannel m_IrcChannel;

        public IrcChannel Channel
        {
            get { return m_IrcChannel; }
        }
		
		public ChannelMessageContext(IrcChannel channel)
		{
			m_IrcChannel = channel;
		}
	}
	
	public class UserMessageContext : MessageContext
	{
		private IrcUser m_IrcUser;

        public IrcUser User
        {
            get { return m_IrcUser; }
        }
		
		public UserMessageContext(IrcUser user)
		{
			m_IrcUser = user;
		}
	}
}
