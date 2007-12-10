using System;
using System.Collections.Generic;
using System.Text;

namespace TechBot.Library
{
    class ReactOSBugUrl : BugCommand
    {
        public ReactOSBugUrl(TechBotService techBot)
            : base(techBot)
        {
        }

        public override string[] AvailableCommands
        {
            get { return new string[] { "rosbug" }; }
        }

        protected override string BugUrl
        {
            get { return "http://www.reactos.org/bugzilla/show_bug.cgi?id={0}"; }
        }

        public override string Help()
        {
            return "!rosbug <number>";
        }
    }
}
