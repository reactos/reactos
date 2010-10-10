using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformdebugchannel")]
    public class PlatformDebugChannelTask : ValueBaseTask
    {
        //private RBuildDebugChannel m_DebugChannel = null;

        //[TaskAttribute("name")]
        //public string ChannelName 
        //{
        //    get { return m_DebugChannel.Name; }
        //    set { m_DebugChannel.Name = value; } 
        //}

        //[TaskAttribute("warning")]
        //public bool Warning
        //{
        //    get { return m_DebugChannel.Warn; }
        //    set { m_DebugChannel.Warn = value; }
        //}

        //[TaskAttribute("trace")]
        //public bool Trace
        //{
        //    get { return m_DebugChannel.Trace; }
        //    set { m_DebugChannel.Trace = value; }
        //}

        //[TaskAttribute("fixme")]
        //public bool Fixme
        //{
        //    get { return m_DebugChannel.Fixme; }
        //    set { m_DebugChannel.Fixme = value; }
        //}

        //[TaskAttribute("error")]
        //public bool Error
        //{
        //    get { return m_DebugChannel.Error; }
        //    set { m_DebugChannel.Error = value; }
        //}

        //protected override void PreExecuteTask()
        //{
        //    m_DebugChannel = Project.DebugChannels.GetByName(ChannelName);
        //}

        protected override void ExecuteTask()
        {
            RBuildDebugChannel channel = Project.DebugChannels.GetByName(Value);

            if (channel == null)
                throw new BuildException("Unknown debug channel '{0}' referenced by <PlatformDebugChannel>", Value);

            if (Project.Platform.DebugChannels.Contains(channel))
                throw new BuildException("Only one debug channel '{0}' can be present per <PlatformDebugChannel>", Value);

            Project.Platform.DebugChannels.Add(channel);
        }
    }
}