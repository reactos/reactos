using System;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public abstract class MessageContext
	{
	}



	public class ChannelMessageContext : MessageContext
	{
		private IrcChannel channel;

		public IrcChannel Channel
		{
			get
			{
				return channel;
			}
		}
		
		public ChannelMessageContext(IrcChannel channel)
		{
			this.channel = channel;
		}
	}

	
	
	public class UserMessageContext : MessageContext
	{
		private IrcUser user;

		public IrcUser User
		{
			get
			{
				return user;
			}
		}
		
		public UserMessageContext(IrcUser user)
		{
			this.user = user;
		}
	}
}
