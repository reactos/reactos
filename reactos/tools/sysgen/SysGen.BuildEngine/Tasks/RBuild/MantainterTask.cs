using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("mantainer")]
    public class MantainterTask : AuthorBaseTask
    {
        public MantainterTask()
        {
            m_Author.Role = AuthorRole.Mantainer;
        }
    }
}