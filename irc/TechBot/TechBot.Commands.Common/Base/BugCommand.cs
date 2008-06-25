using System;

using TechBot.Library;

namespace TechBot.Commands.Common
{
	public abstract class BugCommand : Command
	{
//        private string m_BugID = null;

		public BugCommand()
		{
		}

        public string BugID
        {
            get { return Parameters; }
            set { Parameters = value; }
        }

        public override void ExecuteCommand()
        {
            if (string.IsNullOrEmpty(BugID))
            {
                Say("Please provide a valid bug number.");
            }
            else
            {
                try
                {
                    Say(BugUrl, Int32.Parse(BugID));
                }
                catch (Exception)
                {
                    Say("{0} is not a valid bug number.", BugID);
                }
            }
        }

        protected abstract string BugUrl { get; }
	}
}
