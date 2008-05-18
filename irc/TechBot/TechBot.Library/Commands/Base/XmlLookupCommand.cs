using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public abstract class XmlLookupCommand : XmlCommand
    {
        protected string m_Text = null;

        public virtual string Text
        {
            get { return Parameters; }
            set { Parameters = value; }
        }
    }
}
