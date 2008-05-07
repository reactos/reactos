using System;
using System.Reflection;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    public class CommandBuilder
    {
        private Type m_CommandType;
        private string m_CommandName;
        private string m_CommandHelp;
        private string m_CommandDesc;

        public CommandBuilder(Type commandType)
        {
            m_CommandType = commandType;

            CommandAttribute commandAttribute = (CommandAttribute)
                Attribute.GetCustomAttribute(commandType, typeof(CommandAttribute));

            m_CommandName = commandAttribute.Name;
            m_CommandHelp = commandAttribute.Help;
            m_CommandDesc = commandAttribute.Description;
        }

        public string Name
        {
            get { return m_CommandName; }
        }

        public string Help
        {
            get { return m_CommandHelp; }
        }

        public string Description
        {
            get { return m_CommandDesc; }
        }

        public Type Type
        {
            get { return m_CommandType; }
        }

        public Command CreateCommand()
        {
            return (Command)Type.Assembly.CreateInstance(Type.FullName, true);
        }
    }
}
