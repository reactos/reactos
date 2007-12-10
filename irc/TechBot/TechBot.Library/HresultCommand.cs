using System;
using System.Xml;

namespace TechBot.Library
{
	public class HResultCommand : XmlCommand
	{
        public HResultCommand(TechBotService techBot)
            : base(techBot)
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.HResultXml; }
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "hresult" }; }
        }

        /*
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "hresult" });
		}
        */

		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string hresultText = parameters;
			if (hresultText.Equals(String.Empty))
			{
				TechBot.ServiceOutput.WriteLine(context,
				                        "Please provide a valid HRESULT value.");
				return;
			}

			NumberParser np = new NumberParser();
			long hresult = np.Parse(hresultText);
			if (np.Error)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid HRESULT value.",
				                                      hresultText));
				return;
			}
			
			string description = GetHresultDescription(hresult);
			if (description != null)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      hresultText,
				                                      description));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("I don't know about HRESULT {0}.",
				                                      hresultText));
			}
		}
		
		public override string Help()
		{
			return "!hresult <value>";
		}
		
		public string GetHresultDescription(long hresult)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
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
