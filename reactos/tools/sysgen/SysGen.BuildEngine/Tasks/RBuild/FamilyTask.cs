using System;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Attributes;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("family")]
    public class FamilyTask : Task
    {
        private RBuildFamily m_Family = new RBuildFamily();

        [TaskValue(Required = true)]
        public string FamilyName { get { return m_Family.Name; } set { m_Family.Name = value; } }

        protected override void ExecuteTask()
        {
            RBuildBuildFamily buildFamily = Project.BuildFamilies.GetByName(FamilyName);

            if (buildFamily == null)
                throw new BuildException("Module '{0}' references a no existant family '{1}'", 
                    Module.Name, 
                    FamilyName);

            Module.Families.Add(m_Family);
        }
    }
}