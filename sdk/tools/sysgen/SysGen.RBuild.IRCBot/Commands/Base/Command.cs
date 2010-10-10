using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.IRCBot
{
    public abstract class Command
    {
        //public abstract bool CanProcessCommand (string commandName);
        public abstract string Name { get; }

        protected void Say(string message)
        {
        }
    }
}
