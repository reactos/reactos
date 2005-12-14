using System;
using System.Collections;
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
		private string bugUrl;
		private ArrayList commands = new ArrayList();
		
		public TechBotService(IServiceOutput serviceOutput,
		                      string chmPath,
		                      string mainChm,
		                      string ntstatusXml,
		                      string winerrorXml,
		                      string hresultXml,
		                      string wmXml,
		                      string svnCommand,
		                      string bugUrl)
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
		}
		
		public void Run()
		{
			commands.Add(new HelpCommand(serviceOutput,
			                             commands));
			commands.Add(new ApiCommand(serviceOutput,
			                            chmPath,
			                            mainChm));
			commands.Add(new NtStatusCommand(serviceOutput,
			                                 ntstatusXml));
			commands.Add(new WinerrorCommand(serviceOutput,
			                                 winerrorXml));
			commands.Add(new HresultCommand(serviceOutput,
			                                hresultXml));
			commands.Add(new WmCommand(serviceOutput,
			                           wmXml));
			commands.Add(new SvnCommand(serviceOutput,
			                            svnCommand));
			commands.Add(new BugCommand(serviceOutput,
			                            bugUrl));
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
			string parameters = "";
			if (index != -1)
			{
				commandName = message.Substring(0, index).Trim();
				parameters = message.Substring(index).Trim();
			}
			else
				commandName = message.Trim();

			foreach (ICommand command in commands)
			{
				if (command.CanHandle(commandName))
				{
					command.Handle(context,
					               commandName, parameters);
					return;
				}
			}
		}
	}
}
