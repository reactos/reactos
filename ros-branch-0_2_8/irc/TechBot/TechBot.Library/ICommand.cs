using System;

namespace TechBot.Library
{
	public interface ICommand
	{
		bool CanHandle(string commandName);
		void Handle(MessageContext context,
		            string commandName,
		            string parameters);
		string Help();
	}

	
	
	public class BaseCommand
	{
		protected bool CanHandle(string commandName,
		                         string[] availableCommands)
		{
			foreach (string availableCommand in availableCommands)
			{
				if (String.Compare(availableCommand, commandName, true) == 0)
					return true;
			}
			return false;
		}
	}
}
