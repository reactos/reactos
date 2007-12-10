using System;
using System.Collections;

namespace TechBot.Library
{
	public class HelpCommand : Command
	{
        public HelpCommand(TechBotService techBot)
            : base(techBot)
		{
		}

        public override string[] AvailableCommands
        {
            get { return new string[] { "help" }; }
        }

        public override void Handle(
            MessageContext context,
                           string commandName,
                           string parameters)
        {
            TechBot.ServiceOutput.WriteLine(context, "I support the following commands:");

            foreach (Command command in TechBot.Commands)
            {
                TechBot.ServiceOutput.WriteLine(context,
                                        command.Help());
            }
        }

        public override string Help()
		{
			return "!help";
		}
	}
}
