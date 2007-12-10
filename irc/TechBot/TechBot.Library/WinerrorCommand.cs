using System;
using System.Xml;

namespace TechBot.Library
{
	public class WinerrorCommand : XmlCommand
	{
        public WinerrorCommand(TechBotService techBot)
            : base(techBot)
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.WinErrorXml; }
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "winerror" }; }
        }

		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string winerrorText = parameters;
			if (winerrorText.Equals(String.Empty))
			{
				TechBot.ServiceOutput.WriteLine(context,
				                        "Please provide a valid System Error Code value.");
				return;
			}

			NumberParser np = new NumberParser();
			long winerror = np.Parse(winerrorText);
			if (np.Error)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid System Error Code value.",
				                                      winerrorText));
				return;
			}
			
			string description = GetWinerrorDescription(winerror);
			if (description != null)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      winerrorText,
				                                      description));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("I don't know about System Error Code {0}.",
				                                      winerrorText));
			}
		}

        public override string Help()
		{
			return "!winerror <value>";
		}
		
		public string GetWinerrorDescription(long winerror)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
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
