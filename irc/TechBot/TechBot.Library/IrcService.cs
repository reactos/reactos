using System;
using System.Collections;
using System.Threading;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public class IrcService : IServiceOutput
	{
		private string hostname;
		private int port;
		private string channelnames;
		private string botname;
		private string password;
		private string chmPath;
		private string mainChm;
		private string ntstatusXml;
		private string winerrorXml;
		private string hresultXml;
		private string wmXml;
		private string svnCommand;
		private string bugUrl, WineBugUrl, SambaBugUrl;
		private IrcClient client;
		private ArrayList channels = new ArrayList(); /* IrcChannel */
		private TechBotService service;
		private bool isStopped = false;

		public IrcService(string hostname,
		                  int port,
		                  string channelnames,
		                  string botname,
		                  string password,
		                  string chmPath,
		                  string mainChm,
		                  string ntstatusXml,
		                  string winerrorXml,
		                  string hresultXml,
		                  string wmXml,
		                  string svnCommand,
		                  string BugUrl,
		                  string WineBugUrl,
		                  string SambaBugUrl)
		{
			this.hostname = hostname;
			this.port = port;
			this.channelnames = channelnames;
			this.botname = botname;
			if (password == null || password.Trim() == "")
				this.password = null;
			else
				this.password = password;
			this.chmPath = chmPath;
			this.mainChm = mainChm;
			this.ntstatusXml = ntstatusXml;
			this.winerrorXml = winerrorXml;
			this.hresultXml = hresultXml;
			this.wmXml = wmXml;
			this.svnCommand = svnCommand;
			this.bugUrl = BugUrl;
			this.WineBugUrl = WineBugUrl;
			this.SambaBugUrl = SambaBugUrl;
		}

		public void Run()
		{
			service = new TechBotService(this,
			                             chmPath,
			                             mainChm,
			                             ntstatusXml,
			                             winerrorXml,
			                             hresultXml,
			                             wmXml,
			                             svnCommand,
			                             bugUrl,
			                             WineBugUrl,
			                             SambaBugUrl);
			service.Run();

			client = new IrcClient();
			client.Encoding = System.Text.Encoding.GetEncoding("iso-8859-1");
			client.MessageReceived += new MessageReceivedHandler(client_MessageReceived);
			client.ChannelUserDatabaseChanged += new ChannelUserDatabaseChangedHandler(client_ChannelUserDatabaseChanged);
			System.Console.WriteLine(String.Format("Connecting to {0} port {1}",
			                                       hostname, port));
			client.Connect(hostname, port);
			System.Console.WriteLine("Connected...");
			client.Register(botname, password, null);
			System.Console.WriteLine(String.Format("Registered as {0}...", botname));
			JoinChannels();
			
			while (!isStopped)
			{
				Thread.Sleep(1000);
			}

			PartChannels();
			client.Diconnect();
			System.Console.WriteLine("Disconnected...");
		}

		public void Stop()
		{
			isStopped = true;
		}

		private void JoinChannels()
		{
			foreach (string channelname in channelnames.Split(new char[] { ';' }))
			{
				IrcChannel channel = client.JoinChannel(channelname);
				channels.Add(channel);
				System.Console.WriteLine(String.Format("Joined channel #{0}...",
				                                       channel.Name));
			}
		}

		private void PartChannels()
		{
			foreach (IrcChannel channel in channels)
			{
				client.PartChannel(channel, "Caught in the bitstream...");
				System.Console.WriteLine(String.Format("Parted channel #{0}...",
				                                       channel.Name));
			}
		}

		private string GetMessageSource(MessageContext context)
		{
			if (context is ChannelMessageContext)
			{
				ChannelMessageContext channelContext = context as ChannelMessageContext;
				return String.Format("#{0}",
				                     channelContext.Channel.Name);
			}
			else if (context is UserMessageContext)
			{
				UserMessageContext userContext = context as UserMessageContext;
				return userContext.User.Nickname;
			}
			else
			{
				throw new InvalidOperationException(String.Format("Unhandled message context '{0}'",
				                                                  context.GetType()));
			}
		}

		public void WriteLine(MessageContext context,
		                      string message)
		{
			if (context is ChannelMessageContext)
			{
				ChannelMessageContext channelContext = context as ChannelMessageContext;
				channelContext.Channel.Talk(message);
			}
			else if (context is UserMessageContext)
			{
				UserMessageContext userContext = context as UserMessageContext;
				userContext.User.Talk(message);
			}
			else
			{
				throw new InvalidOperationException(String.Format("Unhandled message context '{0}'",
				                                                  context.GetType()));
			}
		}

		private void ExtractMessage(string parameters,
		                            out string message)
		{
			int startIndex = parameters.IndexOf(':');
			if (startIndex != -1)
			{
				message = parameters.Substring(startIndex + 1);
			}
			else
			{
				message = parameters;
			}
		}

		private bool GetChannelName(IrcMessage message,
		                           out string channelName)
		{
			if (message.Parameters == null || !message.Parameters.StartsWith("#"))
			{
				channelName = null;
				return false;
			}

			int index = message.Parameters.IndexOf(' ');
			if (index == -1)
				index = message.Parameters.Length;
			else
				index = index - 1;
			channelName = message.Parameters.Substring(1, index);
			return true;
		}

		private bool GetTargetNickname(IrcMessage message,
		                               out string nickname)
		{
			if (message.Parameters == null)
			{
				nickname = null;
				return false;
			}

			int index = message.Parameters.IndexOf(' ');
			if (index == -1)
				index = message.Parameters.Length;
			nickname = message.Parameters.Substring(0, index);
			Console.WriteLine("nickname: " + nickname);
			return true;
		}

		private bool ShouldAcceptMessage(IrcMessage message,
		                                 out MessageContext context)
		{
			if (message.Command.ToUpper().Equals("PRIVMSG"))
			{
				string channelName;
				string nickname;
				if (GetChannelName(message,
				                   out channelName))
				{
					foreach (IrcChannel channel in channels)
					{
						if (String.Compare(channel.Name, channelName, true) == 0)
						{
							context = new ChannelMessageContext(channel);
							return true;
						}
					}
				}
				else if (GetTargetNickname(message,
				                           out nickname))
				{
					IrcUser targetUser = new IrcUser(client,
					                                 nickname);
					if (String.Compare(targetUser.Nickname, botname, true) == 0)
					{
						IrcUser sourceUser = new IrcUser(client,
						                                 message.PrefixNickname);
						context = new UserMessageContext(sourceUser);
						return true;
					}
				}
			}
			context = null;
			return false;
		}
				
		private void client_MessageReceived(IrcMessage message)
		{
			try
			{
				if (message.Command != null &&
				    message.Parameters != null)
				{
					string injectMessage;
					ExtractMessage(message.Parameters,
					               out injectMessage);
					MessageContext context;
					if (ShouldAcceptMessage(message,
					                        out context))
					{
						Console.WriteLine(String.Format("Injecting: {0} from {1}",
						                                injectMessage,
						                                GetMessageSource(context)));
						service.InjectMessage(context,
						                      injectMessage);
					}
					else
					{
						Console.WriteLine("Received: " + message.Line);
					}
				}
				else
				{
					Console.WriteLine("Received: " + message.Line);
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(String.Format("Exception: {0}", ex));
			}
		}
		
		private void client_ChannelUserDatabaseChanged(IrcChannel channel)
		{
		}
	}
}
