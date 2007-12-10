using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    class SambaBugUrl : BugCommand
    {
        public SambaBugUrl(TechBotService techBot)
            : base(techBot)
        {
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "sambabug" }; }
        }

        protected override string BugUrl
        {
            get { return "https://bugzilla.samba.org/show_bug.cgi?id={0}"; }
        }

        public override string Help()
        {
            return "!sambabug <number>";
        }
    }
}
