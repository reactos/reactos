using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("developer")]
    public class DeveloperTask : AuthorBaseTask
    {
        public DeveloperTask()
        {
            m_Author.Role = AuthorRole.Developer;
        }
    }
}