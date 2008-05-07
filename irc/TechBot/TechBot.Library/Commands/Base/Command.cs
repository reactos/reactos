using System;

namespace TechBot.Library
{
    public abstract class Command
    {
        protected TechBotService m_TechBotService = null;
        protected MessageContext m_Context = null;

        public TechBotService TechBot
        {
            get { return m_TechBotService; }
            set { m_TechBotService = value; }
        }

        public MessageContext Context
        {
            get { return m_Context; }
            set { m_Context = value; }
        }

        public string Name
        {
            get
            {
                CommandAttribute commandAttribute = (CommandAttribute)
                    Attribute.GetCustomAttribute(GetType(), typeof(CommandAttribute));

                return commandAttribute.Name;
            }
        }

        public void ParseParameters(string paramaters)
        {
            ParametersParser parser = new ParametersParser(paramaters, this);
            parser.Parse();
        }

        protected virtual void Say(string message)
        {
            TechBot.ServiceOutput.WriteLine(Context, message);
        }

        protected virtual void Say(string format , params object[] args)
        {
            TechBot.ServiceOutput.WriteLine(Context, String.Format(format, args));
        }

        public abstract void ExecuteCommand();
    }
}
