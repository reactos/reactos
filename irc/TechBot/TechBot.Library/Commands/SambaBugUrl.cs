using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    [Command("sambabug", Help = "!sambabug <number>")]
    class SambaBugUrl : BugCommand
    {
        public SambaBugUrl()
        {
        }

        protected override string BugUrl
        {
            get { return "https://bugzilla.samba.org/show_bug.cgi?id={0}"; }
        }
    }
}
