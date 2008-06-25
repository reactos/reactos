using System;
using System.Reflection;
using System.Collections;

namespace TechBot.Library
{
    [Command("help", Help = "!help or !help -name:[CommandName]", Description = "Shows this help , type 'help -name:[CommandName]'")]
	public class HelpCommand : Command
	{
        public HelpCommand()
		{
		}

		public override bool AnswerInPublic
		{
			get { return false; }
		}

		[CommandParameter("Name", "The command name to show help")]
        public string CommandName
        {
            get { return Parameters; }
            set { Parameters = value; }
        }

        public override void ExecuteCommand()
        {
            if (string.IsNullOrEmpty(CommandName))
            {
                Say("I support the following commands:");

                foreach (CommandBuilder command in TechBot.Commands)
                {
                    Say("{0}{1} - {2}",
                        Settings.Default.CommandPrefix,
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
                    Say();
                    Say(cmdBuilder.Description);
                    Say();
                    Say(cmdBuilder.Help);
                    Say();
                    Say("Parameters :");
                    Say();

                    PropertyInfo[] propertyInfoArray = cmdBuilder.Type.GetProperties(BindingFlags.Public | BindingFlags.Instance);
                    foreach (PropertyInfo propertyInfo in propertyInfoArray)
                    {
                        CommandParameterAttribute[] commandAttributes = (CommandParameterAttribute[])
                            Attribute.GetCustomAttributes(propertyInfo, typeof(CommandParameterAttribute));

                        foreach (CommandParameterAttribute parameter in commandAttributes)
                        {
                            Say("\t-{0}: [{1}]", 
                                parameter.Name, 
                                parameter.Description);
                        }
                    }

                    Say();
                }
            }
        }
	}
}
