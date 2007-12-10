using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Data;
using System.Threading;
using TechBot.IRCLibrary;

namespace TechBot.Library
{
	public class TechBotService
	{
		private IServiceOutput serviceOutput;
		private string chmPath;
		private string mainChm;
		private string ntstatusXml;
		private string winerrorXml;
		private string hresultXml;
		private string wmXml;
		private string svnCommand;
		private string bugUrl, WineBugUrl, SambaBugUrl;
        private List<Command> commands = new List<Command>();
		
		public TechBotService(IServiceOutput serviceOutput,
		                      string chmPath,
		                      string mainChm)
                              //string ntstatusXml,
                              //string winerrorXml,
                              //string hresultXml,
                              //string wmXml,
		                      //string svnCommand,
                              //string bugUrl,
                              //string WineBugUrl,
                              //string SambaBugUrl)
		{
			this.serviceOutput = serviceOutput;
			this.chmPath = chmPath;
			this.mainChm = mainChm;
			this.ntstatusXml = ntstatusXml;
			this.winerrorXml = winerrorXml;
			this.hresultXml = hresultXml;
			this.wmXml = wmXml;
			this.svnCommand = svnCommand;
			this.bugUrl = bugUrl;
			this.WineBugUrl = WineBugUrl;
			this.SambaBugUrl = SambaBugUrl;
		}

        public void Run()
        {
            commands.Add(new HelpCommand(this));
            /*commands.Add(new ApiCommand(serviceOutput,
                                        chmPath,
                                        mainChm));*/
            commands.Add(new NtStatusCommand(this));
            commands.Add(new WinerrorCommand(this));
            commands.Add(new HResultCommand(this));
            commands.Add(new ErrorCommand(this));
            commands.Add(new WMCommand(this));
            commands.Add(new SvnCommand(this));
            commands.Add(new ReactOSBugUrl(this));
            commands.Add(new SambaBugUrl(this));
            commands.Add(new WineBugUrl(this));
        }

        public IServiceOutput ServiceOutput
        {
            get { return serviceOutput; }
        }

        public IList<Command> Commands
        {
            get { return commands; }
        }
		
		public void InjectMessage(MessageContext context,
		                          string message)
		{
			if (message.StartsWith("!"))
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

			foreach (Command command in commands)
			{
                foreach (string cmd in command.AvailableCommands)
                {
                    if (cmd == commandName)
                    {
                        command.Handle(context,
                                       commandName, 
                                       commandParams);
                        return;
                    }
                }
			}
		}
	}
}
