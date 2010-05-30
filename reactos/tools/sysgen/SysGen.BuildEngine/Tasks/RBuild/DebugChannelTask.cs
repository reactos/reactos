using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("debugchannel")]
    public class DebugChannelTask : Task
    {
        RBuildDebugChannel m_DebugChannel = new RBuildDebugChannel();

        [TaskAttribute("name")]
        [TaskValue]
        public string ChannelName
        {
            get { return m_DebugChannel.Name; }
            set { m_DebugChannel.Name = value; }
        }

        [TaskAttribute("warning")]
        public bool Warning
        {
            get { return m_DebugChannel.Warn; }
            set { m_DebugChannel.Warn = value; }
        }

        [TaskAttribute("trace")]
        public bool Trace
        {
            get { return m_DebugChannel.Trace; }
            set { m_DebugChannel.Trace = value; }
        }

        [TaskAttribute("fixme")]
        public bool Fixme
        {
            get { return m_DebugChannel.Fixme; }
            set { m_DebugChannel.Fixme = value; }
        }

        [TaskAttribute("error")]
        public bool Error
        {
            get { return m_DebugChannel.Error; }
            set { m_DebugChannel.Error = value; }
        }

        protected override void ExecuteTask()
        {
            Project.DebugChannels.Add(m_DebugChannel);
        }
    }
}