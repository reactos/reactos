using System;

namespace TechBot.Library
{
    /*
	public interface ICommand
	{
		bool CanHandle(string commandName);
		void Handle(MessageContext context,
		            string commandName,
		            string parameters);
		//string Help();
	}*/

    public abstract class Command
    {
        protected TechBotService m_TechBotService = null;

        public Command(TechBotService techbot)
        {
            m_TechBotService = techbot;
        }

        public TechBotService TechBot
        {
            get { return m_TechBotService; }
        }

        public abstract string[] AvailableCommands { get; }

        public abstract void Handle(MessageContext context,
            string commandName,
            string parameters);

        /*
        protected bool CanHandle(string commandName,
                                 string[] availableCommands)
        {
            foreach (string availableCommand in availableCommands)
            {
                if (String.Compare(availableCommand, commandName, true) == 0)
                    return true;
            }
            return false;
        }
        */

        public virtual string Help()
        {
            return "No help is available for this command";
        }
    }
}
