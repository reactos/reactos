using System;

namespace TechBot.Library
{
	public class BugCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private string RosBugUrl;
		private string WineBugUrl;
		private string SambaBugUrl;

		public BugCommand(IServiceOutput serviceOutput,
		                  string RosBugUrl,
		                  string WineBugUrl,
		                  string SambaBugUrl)
		{
			this.serviceOutput = serviceOutput;
			this.RosBugUrl = RosBugUrl;
			this.WineBugUrl = WineBugUrl;
			this.SambaBugUrl = SambaBugUrl;
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "bug" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string bugText = parameters;
			if (bugText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide a valid bug number.");
				return;
			}

			NumberParser np = new NumberParser();
			long bug = np.Parse(bugText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid bug number.",
				                                      bugText));
				return;
			}
			
			string bugUrl = this.RosBugUrl;

			if (context is ChannelMessageContext)
			{
				ChannelMessageContext channelContext = context as ChannelMessageContext;
				if (channelContext.Channel.Name == "winehackers")
					bugUrl = this.WineBugUrl;
				else if (channelContext.Channel.Name == "samba-technical")
					bugUrl = this.SambaBugUrl;
			}
			
			serviceOutput.WriteLine(context,
			                        String.Format(bugUrl, bug));
		}
		
		public string Help()
		{
			return "!bug <number>";
		}
	}
}
