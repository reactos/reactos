using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public abstract class XmlLookupCommand : XmlCommand
    {
        public virtual string Text
        {
            get { return Parameters; }
            set { Parameters = value; }
        }
    }
}
