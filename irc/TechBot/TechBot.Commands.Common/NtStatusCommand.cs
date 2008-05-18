using System;
using System.Xml;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("ntstatus", Help = "!ntstatus <value>")]
	public class NtStatusCommand : XmlLookupCommand
	{
        public NtStatusCommand()
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.NtStatusXml; }
        }

		public override void ExecuteCommand()
		{
            if (string.IsNullOrEmpty(Text))
            {
                Say("Please provide a valid NTSTATUS value.");
            }
            else
            {
                NumberParser np = new NumberParser();
                long ntstatus = np.Parse(Text);
                if (np.Error)
                {
                    Say("{0} is not a valid NTSTATUS value.", Text);
                    return;
                }

                string description = GetNtstatusDescription(ntstatus);
                if (description != null)
                {
                    Say("{0} is {1}.",
                        Text,
                        description);
                }
                else
                {
                    Say("I don't know about NTSTATUS {0}.", Text);
                }
            }
		}
		
		public string GetNtstatusDescription(long ntstatus)
		{
			XmlElement root = base.m_XmlDocument.DocumentElement;
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
