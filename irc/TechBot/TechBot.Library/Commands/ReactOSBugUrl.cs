using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    [Command("rosbug", Help = "!rosbug <number>")]
    class ReactOSBugUrl : BugCommand
    {
        public ReactOSBugUrl()
        {
        }

        protected override string BugUrl
        {
            get { return "http://www.reactos.org/bugzilla/show_bug.cgi?id={0}"; }
        }
    }
}
