using System;

namespace TechBot.Library
{
	public class SvnCommand : Command
	{
		private string m_SvnRoot;

        public SvnCommand(TechBotService techBot)
            : base(techBot)
		{
			m_SvnRoot = Settings.Default.SVNRoot;
		}

        public override string[] AvailableCommands
        {
            get { return new string[] { "svn" }; }
        }
		
		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			TechBot.ServiceOutput.WriteLine(context, string.Format("svn co {0}" , m_SvnRoot));
		}

        public override string Help()
		{
			return "!svn";
		}
	}
}
