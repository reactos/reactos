using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    [Command("winebug", Help = "!winebug <number>")]
    class WineBugUrl : BugCommand
    {
        public WineBugUrl()
        {
        }

        protected override string BugUrl
        {
            get { return "http://bugs.winehq.org/show_bug.cgi?id={0}"; }
        }
    }
}
