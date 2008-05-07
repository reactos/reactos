using System;

namespace TechBot.Library
{
	public abstract class BugCommand : Command
	{
        private string m_BugID = null;

		public BugCommand()
		{
		}

        [CommandParameter("id", "The bug ID")]
        public string BugID
        {
            get { return m_BugID; }
            set { m_BugID = value; }
        }

        public override void ExecuteCommand()
        {
            if (BugID == null)
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
