using System;
using System.Xml;

namespace TechBot.Library
{
	public class ErrorCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private NtStatusCommand ntStatus;
		private WinerrorCommand winerror;
		private HresultCommand hresult;

		public ErrorCommand(IServiceOutput serviceOutput, string ntstatusXml,
									string winerrorXml, string hresultXml)
		{
			this.serviceOutput = serviceOutput;
			this.ntStatus = new NtStatusCommand(serviceOutput,
											 ntstatusXml);
			this.winerror = new WinerrorCommand(serviceOutput,
											winerrorXml);
			this.hresult = new HresultCommand(serviceOutput,
											hresultXml);
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "error" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string errorText = parameters;
			if (errorText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide an Error Code.");
				return;
			}

			NumberParser np = new NumberParser();
			long error = np.Parse(errorText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid Error Code.",
													  errorText));
				return;
			}

			string description = null;
			if (winerror.GetWinerrorDescription(error) != null)
			{
				description = winerror.GetWinerrorDescription(error);
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
													  error,
				                                      description));
			}
			if (ntStatus.GetNtstatusDescription(error) != null)
			{
				description = ntStatus.GetNtstatusDescription(error);
				serviceOutput.WriteLine(context,
										String.Format("{0} is {1}.",
													  errorText,
													  description));
			}
			if (hresult.GetHresultDescription(error) != null)
			{
				description = hresult.GetHresultDescription(error);
				serviceOutput.WriteLine(context,
										String.Format("{0} is {1}.",
													  errorText,
													  description));
			}
			if(description == null)
			{
				serviceOutput.WriteLine(context,
										String.Format("I don't know about Error Code {0}.",
													  errorText));
			}
		}
		
		public string Help()
		{
			return "!error <value>";
		}
	}
}
