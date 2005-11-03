using System;

namespace TechBot.Library
{
	public class SvnCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private string svnCommand;

		public SvnCommand(IServiceOutput serviceOutput,
		                       string svnCommand)
		{
			this.serviceOutput = serviceOutput;
			this.svnCommand = svnCommand;
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "svn" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			serviceOutput.WriteLine(context,
			                        svnCommand);
		}
		
		public string Help()
		{
			return "!svn";
		}
	}
}
