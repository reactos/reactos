using System;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public class MessageContext
	{
		private IrcChannel channel;

		public IrcChannel Channel
		{
			get
			{
				return channel;
			}
		}
		
		public MessageContext(IrcChannel channel)
		{
			this.channel = channel;
		}
	}
}
