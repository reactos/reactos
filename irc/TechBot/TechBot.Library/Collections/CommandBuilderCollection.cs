using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public class CommandBuilderCollection : List<CommandBuilder>
    {
        public CommandBuilder Find(string name)
        {
            foreach (CommandBuilder command in this)
            {
                if (command.Name == name)
                    return command;
            }

            return null;
        }
    }
}
