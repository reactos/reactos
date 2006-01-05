using System;
using System.Xml;

namespace TechBot.Library
{
	public class WinerrorCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private string winerrorXml;
		private XmlDocument winerrorXmlDocument;

		public WinerrorCommand(IServiceOutput serviceOutput,
		                       string winerrorXml)
		{
			this.serviceOutput = serviceOutput;
			this.winerrorXml = winerrorXml;
			winerrorXmlDocument = new XmlDocument();
			winerrorXmlDocument.Load(winerrorXml);
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "winerror" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string winerrorText = parameters;
			if (winerrorText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide a valid System Error Code value.");
				return;
			}

			NumberParser np = new NumberParser();
			long winerror = np.Parse(winerrorText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid System Error Code value.",
				                                      winerrorText));
				return;
			}
			
			string description = GetWinerrorDescription(winerror);
			if (description != null)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      winerrorText,
				                                      description));
			}
			else
			{
				serviceOutput.WriteLine(context,
				                        String.Format("I don't know about System Error Code {0}.",
				                                      winerrorText));
			}
		}
		
		public string Help()
		{
			return "!winerror <value>";
		}
		
		public string GetWinerrorDescription(long winerror)
		{
			XmlElement root = winerrorXmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("Winerror[@value='{0}']",
			                                                   winerror));
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
