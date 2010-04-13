using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("modulestate")]
    public class ModuleStateTask : Task
    {
        private string m_ModuleName = null;
        private bool m_Enabled = true;

        [TaskAttribute("name")]
        public string ModuleName { get { return m_ModuleName; } set { m_ModuleName = value; } }

        [TaskAttribute("enabled")]
        [BooleanValidator]
        public bool Enabled { get { return m_Enabled; } set { m_Enabled = value; } }

        protected override void ExecuteTask() 
        {
            if (ModuleName != null)
            {
                RBuildModule module = Project.Modules.GetByName(ModuleName);

                if (module == null)
                    throw new BuildException(string.Format("Could not change state for module '{0}'", ModuleName, Location));

                module.Enabled = Enabled;
            }
        }
    }
}
