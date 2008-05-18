using System;
using System.Xml;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("hresult", Help = "!hresult <value>")]
    public class HResultCommand : XmlLookupCommand
	{
        public HResultCommand()
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.HResultXml; }
        }

		public override void ExecuteCommand()
		{
            if (string.IsNullOrEmpty(Text))
            {
                Say("Please provide a valid HRESULT value.");
            }
            else
            {
                NumberParser np = new NumberParser();
                long hresult = np.Parse(Text);
                if (np.Error)
                {
                    Say("{0} is not a valid HRESULT value.", Text);
                    return;
                }

                string description = GetHresultDescription(hresult);
                if (description != null)
                {
                    Say("{0} is {1}.",
                        Text,
                        description);
                }
                else
                {
                    Say("I don't know about HRESULT {0}.", Text);
                }
            }
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
