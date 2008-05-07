using System;
using System.Collections;

namespace TechBot.Library
{
    [Command("help", Help = "!help")]
	public class HelpCommand : Command
	{
        private string m_CommandName = null;

        public HelpCommand()
		{
		}

        [CommandParameter("Name", "The command name to show help")]
        public string CommandName
        {
            get { return m_CommandName; }
            set { m_CommandName = value; }
        }

        public override void ExecuteCommand()
        {
            if (CommandName == null)
            {
                Say("I support the following commands:");

                foreach (CommandBuilder command in TechBot.Commands)
                {
                    Say("!{0} - {1}",
                        command.Name,
                        command.Description);
                }
            }
            else
            {
                CommandBuilder cmdBuilder = TechBot.Commands.Find(CommandName);

                if (cmdBuilder == null)
                {
                    Say("Command '{0}' is not recognized. Type '!help' to show all available commands", CommandName);
                }
                else
                {
                    Say("Command '{0}' help:", CommandName);
                    Say("");
                }
            }
        }
	}
}
