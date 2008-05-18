using System;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("svn", Help = "!svn" , Description="Where the ROS SVN repository is located")]
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
