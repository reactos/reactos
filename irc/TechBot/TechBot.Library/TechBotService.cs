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
		protected IServiceOutput m_ServiceOutput;
		
		public TechBotService(IServiceOutput serviceOutput)
		{
			m_ServiceOutput = serviceOutput;
		}

        public virtual void Run()
        {
            CommandFactory.LoadPlugins();
        }

        public IServiceOutput ServiceOutput
        {
            get { return m_ServiceOutput; }
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
            return message.StartsWith(Settings.Default.CommandPrefix);
		}

        public void InjectMessage(string message)
        {
            ParseCommandMessage(null, message);
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
                    cmd.Parameters = commandParams;

                    try
                    {
                        cmd.Initialize();
                        cmd.Run();
                        cmd.DeInitialize();
                    }
                    catch (Exception e)
                    {
                        ServiceOutput.WriteLine(context, string.Format("Uops! Just crashed with exception '{0}' at {1}",
                            e.Message,
                            e.Source));

                        ServiceOutput.WriteLine(context, e.StackTrace);
                    }

                    return;
                }
            }
		}
	}
}
