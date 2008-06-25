using System;
using System.Text.RegularExpressions;

namespace TechBot.Library
{
    public abstract class Command
    {
        protected TechBotService m_TechBotService = null;
        protected MessageContext m_Context = null;
        protected string m_Params = null;

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

        public virtual bool AnswerInPublic
        {
            get { return true; }
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

        public string Parameters
        {
            get { return m_Params; }
            set { m_Params = value; }
        }

        protected virtual void Say()
        {
            TechBot.ServiceOutput.WriteLine(Context, string.Empty);
        }

        protected virtual void Say(string message)
        {
            TechBot.ServiceOutput.WriteLine(Context, message);
        }

        protected virtual void Say(string format , params object[] args)
        {
            TechBot.ServiceOutput.WriteLine(Context, String.Format(format, args));
        }

        public void Run()
        {
            if (Context is ChannelMessageContext)
            {
                if (AnswerInPublic)
                {
                    ExecuteCommand();
                }
                else
                {
                    Say("Sorry, I only respond '{0}' in private , PM me!", Name);
                }
            }
            else
            {
                ExecuteCommand();
            }
        }

        public abstract void ExecuteCommand();

        public virtual void Initialize()
        {
        }

        public virtual void DeInitialize()
        {
        }
    }
}
