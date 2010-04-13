using System;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    /// <summary>
    /// Just a task container
    /// </summary>
    [TaskName("buildfamily")]
    public class BuildFamilyTask : Task
    {
        private RBuildBuildFamily m_BuildFamily = new RBuildBuildFamily();

        [TaskAttribute("name", Required = true)]
        public string FamilyName { get { return m_BuildFamily.Name; } set { m_BuildFamily.Name = value; } }

        [TaskAttribute("description")]
        public string FamilyDescription { get { return m_BuildFamily.Description; } set { m_BuildFamily.Description = value; } }

        protected override void PreExecuteTask()
        {
            Project.BuildFamilies.Add(m_BuildFamily);
        }
    }
}