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
		private string chmPath;
		private string mainChm;
		private string ntstatusXml;
		private string winerrorXml;
		private string hresultXml;
		private string svnCommand;
		private IrcClient client;
		private ArrayList channels = new ArrayList(); /* IrcChannel */
		private TechBotService service;
		private bool isStopped = false;

		public IrcService(string hostname,
		                  int port,
		                  string channelnames,
		                  string botname,
		                  string chmPath,
		                  string mainChm,
		                  string ntstatusXml,
		                  string winerrorXml,
		                  string hresultXml,
		                  string svnCommand)
		{
			this.hostname = hostname;
			this.port = port;
			this.channelnames = channelnames;
		    this.botname = botname;
		    this.chmPath = chmPath;
		    this.mainChm = mainChm;
		    this.ntstatusXml = ntstatusXml;
		    this.winerrorXml = winerrorXml;
		    this.hresultXml = hresultXml;
		    this.svnCommand = svnCommand;
		}

		public void Run()
		{
			service = new TechBotService(this,
			                             chmPath,
			                             mainChm,
			                             ntstatusXml,
			                             winerrorXml,
			                             hresultXml,
			                             svnCommand);
			service.Run();

			client = new IrcClient();
			client.Encoding = System.Text.Encoding.GetEncoding("iso-8859-1");
			client.MessageReceived += new MessageReceivedHandler(client_MessageReceived);
			client.ChannelUserDatabaseChanged += new ChannelUserDatabaseChangedHandler(client_ChannelUserDatabaseChanged);
			System.Console.WriteLine(String.Format("Connecting to {0} port {1}",
			                                       hostname, port));
			client.Connect(hostname, port);
			System.Console.WriteLine("Connected...");
			client.Register(botname, null);
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

		public void WriteLine(MessageContext context,
		                      string message)
		{
			Console.WriteLine(String.Format("Sending: {0} to #{1}",
			                                message,
			                                context.Channel != null ? context.Channel.Name : "(null)"));
			context.Channel.Talk(message);
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
			channelName = message.Parameters.Substring(1, index - 1);
			return true;
		}

		private bool ShouldAcceptMessage(IrcMessage message,
		                                 out MessageContext context)
		{
			if (message.Command.ToUpper().Equals("PRIVMSG"))
			{
				string channelName;
				if (GetChannelName(message,
				                   out channelName))
				{
					foreach (IrcChannel channel in channels)
					{
						if (String.Compare(channel.Name, channelName, true) == 0)
						{
							context = new MessageContext(channel);
							return true;
						}
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
						Console.WriteLine(String.Format("Injecting: {0} from #{1}",
						                                injectMessage,
						                                context.Channel.Name));
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
