using System;
using System.Xml;

namespace TechBot.Library
{
	public class WMCommand : XmlCommand
	{
        public WMCommand(TechBotService techBot)
            : base(techBot)
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.WMXml; }
        }
		
        public override string[] AvailableCommands
        {
            get { return new string[] { "wm" }; }
        }

		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string wmText = parameters;
			if (wmText.Equals(String.Empty))
			{
				TechBot.ServiceOutput.WriteLine(context,
				                        "Please provide a valid window message value or name.");
				return;
			}

			NumberParser np = new NumberParser();
			long wm = np.Parse(wmText);
			string output;
			if (np.Error)
			{
				// Assume "!wm <name>" form.
				output = GetWmNumber(wmText);
			}
			else
			{
				output = GetWmDescription(wm);
			}

			if (output != null)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      wmText,
			        	                              output));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("I don't know about window message {0}.",
			        	                              wmText));
			}
		}

        public override string Help()
		{
			return "!wm <value> or !wm <name>";
		}
		
		private string GetWmDescription(long wm)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("WindowMessage[@value='{0}']",
			                                                   wm));
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
		
		private string GetWmNumber(string wmName)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("WindowMessage[@text='{0}']",
			                                                   wmName));
			if (node != null)
			{
				XmlAttribute value = node.Attributes["value"];
				if (value == null)
					throw new Exception("Node has no value attribute.");
				return value.Value;
			}
			else
				return null;
		}
	}
}
