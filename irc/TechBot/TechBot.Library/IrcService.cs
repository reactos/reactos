using System;
using System.Threading;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public class IrcService : IServiceOutput
	{
		private string hostname;
		private int port;
		private string channelname;
		private string botname;
		private string chmPath;
		private string mainChm;
		private string ntstatusXml;
		private string winerrorXml;
		private string hresultXml;
		private string svnCommand;
		private IrcClient client;
		private IrcChannel channel1;
		private TechBotService service;
		private bool isStopped = false;

		public IrcService(string hostname,
		                  int port,
		                  string channelname,
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
			this.channelname = channelname;
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
			channel1 = client.JoinChannel(channelname);
			System.Console.WriteLine(String.Format("Joined channel {0}...", channelname));
			
			while (!isStopped)
			{
				Thread.Sleep(1000);
			}

			client.PartChannel(channel1, "Caught in the bitstream...");
			client.Diconnect();
			System.Console.WriteLine("Disconnected...");
		}
		
		public void Stop()
		{
			isStopped = true;
		}
	
		public void WriteLine(string message)
		{
			Console.WriteLine(String.Format("Sending: {0}", message));
			channel1.Talk(message);
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
		
		private void client_MessageReceived(IrcMessage message)
		{
			try
			{
				if (channel1 != null &&
				    channel1.Name != null &&
				    message.Parameters != null)
				{
					string injectMessage;
					ExtractMessage(message.Parameters, out injectMessage);
					if ((message.Command.ToUpper().Equals("PRIVMSG")) &&
					    (message.Parameters.ToLower().StartsWith("#" + channel1.Name.ToLower() + " ")))
					{
						Console.WriteLine("Injecting: " + injectMessage);
						service.InjectMessage(injectMessage);
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
