using System;
using System.Xml;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("winerror", Help = "!winerror <value>")]
    public class WinErrorCommand : XmlLookupCommand
	{
        public WinErrorCommand()
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.WinErrorXml; }
        }

		public override void ExecuteCommand()
		{
            if (string.IsNullOrEmpty(Text))
            {
                Say("Please provide a valid System Error Code value.");
            }
            else
            {
                NumberParser np = new NumberParser();
                long winerror = np.Parse(Text);
                if (np.Error)
                {
                    Say("{0} is not a valid System Error Code value.", Text);
                    return;
                }

                string description = GetWinerrorDescription(winerror);
                if (description != null)
                {
                    Say("{0} is {1}.",
                        Text,
                        description);
                }
                else
                {
                    Say("I don't know about System Error Code {0}.", Text);
                }
            }
		}

		public string GetWinerrorDescription(long winerror)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
			XmlNode node = root.SelectSingleNode(String.Format("Winerror[@value='{0}']",
                                                               winerror.ToString()));
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
