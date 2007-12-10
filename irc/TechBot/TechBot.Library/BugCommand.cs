using System;

namespace TechBot.Library
{
	public abstract class BugCommand : Command//, ICommand
	{
		public BugCommand(TechBotService techBot) : base (techBot)
		{
		}

        public override void Handle(MessageContext context,
                           string commandName,
                           string parameters)
        {
            string bugText = parameters;
            if (bugText.Equals(String.Empty))
            {
                TechBot.ServiceOutput.WriteLine(context,
                                        "Please provide a valid bug number.");
                return;
            }

            NumberParser np = new NumberParser();
            long bug = np.Parse(bugText);
            if (np.Error)
            {
                TechBot.ServiceOutput.WriteLine(context,
                                        String.Format("{0} is not a valid bug number.",
                                                      bugText));
                return;
            }

            /*
            string bugUrl = this.RosBugUrl;

            if (context is ChannelMessageContext)
            {
                ChannelMessageContext channelContext = context as ChannelMessageContext;
                if (channelContext.Channel.Name == "winehackers")
                    bugUrl = this.WineBugUrl;
                else if (channelContext.Channel.Name == "samba-technical")
                    bugUrl = this.SambaBugUrl;
            }*/

            TechBot.ServiceOutput.WriteLine(context, String.Format(BugUrl, bug));
        }

        protected abstract string BugUrl { get; }
	}
}
