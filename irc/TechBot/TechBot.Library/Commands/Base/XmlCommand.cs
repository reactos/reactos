using System;
using System.Xml;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public abstract class XmlCommand : Command
    {
        protected XmlDocument m_XmlDocument;

        public XmlCommand(TechBotService techBot)
            : base(techBot)
        {
            m_XmlDocument = new XmlDocument();
            m_XmlDocument.Load(XmlFile);
        }

        public abstract string XmlFile { get; }

        public XmlDocument XmlDocument
        {
            get { return m_XmlDocument; }
        }
    }
}