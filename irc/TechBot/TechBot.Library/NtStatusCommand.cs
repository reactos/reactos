using System;
using System.Xml;

namespace TechBot.Library
{
	public class NtStatusCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private string ntstatusXml;
		private XmlDocument ntstatusXmlDocument;

		public NtStatusCommand(IServiceOutput serviceOutput,
		                       string ntstatusXml)
		{
			this.serviceOutput = serviceOutput;
			this.ntstatusXml = ntstatusXml;
			ntstatusXmlDocument = new XmlDocument();
			ntstatusXmlDocument.Load(ntstatusXml);
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "ntstatus" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string ntstatusText = parameters;
			if (ntstatusText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide a valid NTSTATUS value.");
				return;
			}

			NumberParser np = new NumberParser();
			long ntstatus = np.Parse(ntstatusText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid NTSTATUS value.",
				                                      ntstatusText));
				return;
			}
			
			string description = GetNtstatusDescription(ntstatus);
			if (description != null)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      ntstatusText,
				                                      description));
			}
			else
			{
				serviceOutput.WriteLine(context,
				                        String.Format("I don't know about NTSTATUS {0}.",
				                                      ntstatusText));
			}
		}
		
		public string Help()
		{
			return "!ntstatus <value>";
		}
		
		public string GetNtstatusDescription(long ntstatus)
		{
			XmlElement root = ntstatusXmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("Ntstatus[@value='{0}']",
			                                                   ntstatus.ToString("X8")));
			if (node != null)
			{
				XmlAttribute text = node.Attributes["text"];
				if (text == null)
					throw new Exception("Node has no text attribute.");
				return text.Value;
			}
			else
				return null;
		}
	}
}
