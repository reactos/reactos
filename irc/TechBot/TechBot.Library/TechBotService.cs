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
		private const bool IsVerbose = false;
		
		private IServiceOutput serviceOutput;
		private string chmPath;
		private string mainChm;
		private string ntstatusXml;
		private string winerrorXml;
		private string hresultXml;
		private string svnCommand;
		private ArrayList commands = new ArrayList();
		
		public TechBotService(IServiceOutput serviceOutput,
		                      string chmPath,
			                  string mainChm,
			                  string ntstatusXml,
			                  string winerrorXml,
			                  string hresultXml,
			                  string svnCommand)
		{
			this.serviceOutput = serviceOutput;
			this.chmPath = chmPath;
			this.mainChm = mainChm;
			this.ntstatusXml = ntstatusXml;
			this.winerrorXml = winerrorXml;
			this.hresultXml = hresultXml;
			this.svnCommand = svnCommand;
		}
		
		private void WriteIfVerbose(string message)
		{
			if (IsVerbose)
				serviceOutput.WriteLine(message);
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
			commands.Add(new SvnCommand(serviceOutput,
			                            svnCommand));
		}
		
		public void InjectMessage(string message)
		{
			if (message.StartsWith("!"))
			{
				ParseCommandMessage(message);
			}
		}
		
		private bool IsCommandMessage(string message)
		{
			return message.StartsWith("!");
		}

		public void ParseCommandMessage(string message)
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
					command.Handle(commandName, parameters);
					return;
				}
			}
		}
	}
}
