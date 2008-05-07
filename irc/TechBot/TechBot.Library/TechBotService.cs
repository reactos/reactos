using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Data;
using System.Threading;

using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public abstract class TechBotService
	{
		protected IServiceOutput serviceOutput;
		private string chmPath;
		private string mainChm;
		
		public TechBotService(IServiceOutput serviceOutput,
		                      string chmPath,
		                      string mainChm)
		{
			this.serviceOutput = serviceOutput;
			this.chmPath = chmPath;
			this.mainChm = mainChm;
		}

        public virtual void Run()
        {
            CommandFactory.LoadPlugins();
        }

        public IServiceOutput ServiceOutput
        {
            get { return serviceOutput; }
        }

        public CommandBuilderCollection Commands
        {
            get { return CommandFactory.Commands; }
        }

        public void InjectMessage(MessageContext context, string message)
        {
            ParseCommandMessage(context,
                                message);
        }
		
		private bool IsCommandMessage(string message)
		{
			return message.StartsWith("!");
		}

		public void ParseCommandMessage(MessageContext context,
		                                string message)
		{
			if (!IsCommandMessage(message))
				return;

			message = message.Substring(1).Trim();
			int index = message.IndexOf(' ');
			string commandName;
			string commandParams = "";
			if (index != -1)
			{
				commandName = message.Substring(0, index).Trim();
				commandParams = message.Substring(index).Trim();
			}
			else
				commandName = message.Trim();

            foreach (CommandBuilder command in Commands)
            {
                if (command.Name == commandName)
                {
                    //Create a new instance of the required command type
                    Command cmd = command.CreateCommand();

                    cmd.TechBot = this;
                    cmd.Context = context;
                    cmd.ParseParameters(message);
                    cmd.ExecuteCommand();

                    return;
                }
            }
		}
	}
}
