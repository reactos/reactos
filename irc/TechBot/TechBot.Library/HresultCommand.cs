using System;
using System.Xml;

namespace TechBot.Library
{
	public class HresultCommand : BaseCommand, ICommand
	{
		private IServiceOutput serviceOutput;
		private XmlDocument hresultXmlDocument;

		public HresultCommand(IServiceOutput serviceOutput,
		                      string hresultXml)
		{
			this.serviceOutput = serviceOutput;
			hresultXmlDocument = new XmlDocument();
			hresultXmlDocument.Load(hresultXml);
		}
		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "hresult" });
		}

		public void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string hresultText = parameters;
			if (hresultText.Equals(String.Empty))
			{
				serviceOutput.WriteLine(context,
				                        "Please provide a valid HRESULT value.");
				return;
			}

			NumberParser np = new NumberParser();
			long hresult = np.Parse(hresultText);
			if (np.Error)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid HRESULT value.",
				                                      hresultText));
				return;
			}
			
			string description = GetHresultDescription(hresult);
			if (description != null)
			{
				serviceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      hresultText,
				                                      description));
			}
			else
			{
				serviceOutput.WriteLine(context,
				                        String.Format("I don't know about HRESULT {0}.",
				                                      hresultText));
			}
		}
		
		public string Help()
		{
			return "!hresult <value>";
		}
		
		public string GetHresultDescription(long hresult)
		{
			XmlElement root = hresultXmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("Hresult[@value='{0}']",
			                                                   hresult.ToString("X8")));
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
