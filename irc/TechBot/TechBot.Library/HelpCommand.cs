using System;
using System.Collections;

namespace TechBot.Library
{
	public class HelpCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private ArrayList commands;
		
		public HelpCommand(IServiceOutput serviceOutput,
		                   ArrayList commands)
		{
			this.serviceOutput = serviceOutput;
			this.commands = commands;
		}

		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "help" });
		}
		
		public void Handle(string commandName,
		                   string parameters)
		{
			serviceOutput.WriteLine("I support the following commands:");
			foreach (ICommand command in commands)
				serviceOutput.WriteLine(command.Help());
		}
		
		public string Help()
		{
			return "!help";
		}
	}
}
