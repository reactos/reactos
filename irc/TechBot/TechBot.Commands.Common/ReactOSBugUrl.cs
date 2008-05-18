using System;
using System.Collections.Generic;
using System.Text;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("rosbug", Help = "!rosbug <number>", Description = "Will give you a link to the reqested ReactOS bug")]
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
