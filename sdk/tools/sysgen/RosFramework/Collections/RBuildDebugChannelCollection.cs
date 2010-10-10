using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildDebugChannelCollection : List<RBuildDebugChannel>
    {
        public RBuildDebugChannel GetByName(string name)
        {
            foreach (RBuildDebugChannel channel in this)
            {
                if (channel.Name == name)
                    return channel;
            }

            return null;
        }

        public string Text
        {
            get
            {
                StringBuilder sBuilder = new StringBuilder();

                foreach (RBuildDebugChannel channel in this)
                {
                    sBuilder.Append(channel.Text);
                }

                return sBuilder.ToString();
            }
        }
    }
}
