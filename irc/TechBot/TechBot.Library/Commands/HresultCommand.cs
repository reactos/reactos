using System;
using System.Xml;

namespace TechBot.Library
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
            if (Text.Equals(String.Empty))
			{
                TechBot.ServiceOutput.WriteLine(Context,
				                        "Please provide a valid HRESULT value.");
				return;
			}

			NumberParser np = new NumberParser();
            long hresult = np.Parse(Text);
			if (np.Error)
			{
                TechBot.ServiceOutput.WriteLine(Context,
				                        String.Format("{0} is not a valid HRESULT value.",
                                                      Text));
				return;
			}
			
			string description = GetHresultDescription(hresult);
			if (description != null)
			{
                TechBot.ServiceOutput.WriteLine(Context,
				                        String.Format("{0} is {1}.",
                                                      Text,
				                                      description));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(Context,
				                        String.Format("I don't know about HRESULT {0}.",
                                                      Text));
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
