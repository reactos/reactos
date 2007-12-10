using System;
using System.Xml;

namespace TechBot.Library
{
	public class NtStatusCommand : XmlCommand
	{
        public NtStatusCommand(TechBotService techBot)
            : base(techBot)
		{
		}

        public override string XmlFile
        {
            get { return Settings.Default.NtStatusXml; }
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "ntstatus" }; }
        }
/*		
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "ntstatus" });
		}
*/
		public override void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string ntstatusText = parameters;
			if (ntstatusText.Equals(String.Empty))
			{
				TechBot.ServiceOutput.WriteLine(context,
				                        "Please provide a valid NTSTATUS value.");
				return;
			}

			NumberParser np = new NumberParser();
			long ntstatus = np.Parse(ntstatusText);
			if (np.Error)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid NTSTATUS value.",
				                                      ntstatusText));
				return;
			}
			
			string description = GetNtstatusDescription(ntstatus);
			if (description != null)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is {1}.",
				                                      ntstatusText,
				                                      description));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("I don't know about NTSTATUS {0}.",
				                                      ntstatusText));
			}
		}

        public override string Help()
		{
			return "!ntstatus <value>";
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
