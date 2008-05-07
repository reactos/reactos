using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public class CommandAttribute : Attribute
    {
        private string m_Name = null;
        private string m_Help = "No help for this command is available";
        private string m_Desc = "No description for this command is available";

        public CommandAttribute(string name)
        {
            m_Name = name;
        }

        public string Name
        {
            get { return m_Name; }
        }

        public string Help
        {
            get { return m_Help; }
            set { m_Help = value; }
        }

        public string Description
        {
            get { return m_Desc; }
            set { m_Desc = value; }
        }
    }
}
