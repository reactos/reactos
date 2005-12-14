using System;

namespace TechBot.Library
{
	public class BugCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private string bugUrl;

		public BugCommand(IServiceOutput serviceOutput,
		                  string bugUrl)
		{
			this.serviceOutput = serviceOutput;
			this.bugUrl = bugUrl;
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "bug" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string bugText = parameters;
			if (bugText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide a valid bug number.");
				return;
			}

			NumberParser np = new NumberParser();
			long bug = np.Parse(bugText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid bug number.",
				                                      bugText));
				return;
			}

			serviceOutput.WriteLine(context,
			                        String.Format(bugUrl, bug));
		}
		
		public string Help()
		{
			return "!bug <number>";
		}
	}
}
