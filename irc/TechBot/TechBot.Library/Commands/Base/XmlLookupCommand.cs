using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public abstract class XmlLookupCommand : XmlCommand
    {
        private string m_Text = null;

        [CommandParameter("text", "The value to check")]
        public string Text
        {
            get { return m_Text; }
            set { m_Text = value; }
        }
    }
}
