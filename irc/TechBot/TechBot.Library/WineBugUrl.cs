using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    class WineBugUrl : BugCommand
    {
        public WineBugUrl(TechBotService techBot)
            : base(techBot)
        {
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "winebug" }; }
        }

        protected override string BugUrl
        {
            get { return "http://bugs.winehq.org/show_bug.cgi?id={0}"; }
        }

        public override string Help()
        {
            return "!winebug <number>";
        }
    }
}
