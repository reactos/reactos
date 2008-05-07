using System;

namespace TechBot.Library
{
    [Command("svn", Help = "!svn")]
	public class SvnCommand : Command
	{
		private string m_SvnRoot;

        public SvnCommand()
		{
			m_SvnRoot = Settings.Default.SVNRoot;
		}
		
		public override void ExecuteCommand()
		{
            Say("svn co {0}", m_SvnRoot);
		}
	}
}
