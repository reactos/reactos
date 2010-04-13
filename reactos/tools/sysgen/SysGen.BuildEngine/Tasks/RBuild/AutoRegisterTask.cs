using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("autoregister")]
    public class AutoRegisterTask : Task
    {
        RBuildAutoRegister m_AutoRegister = new RBuildAutoRegister();

        [TaskAttribute("type", Required = true)]
        public AutoRegisterType Type { get { return m_AutoRegister.Type; } set { m_AutoRegister.Type = value; } }

        [TaskAttribute("infsection", Required = true)]
        public string InfSection { get { return m_AutoRegister.InfSection; } set { m_AutoRegister.InfSection = value; } }

        protected override void ExecuteTask()
        {
            if ((Module.Type == ModuleType.Win32DLL) ||
                (Module.Type == ModuleType.Win32OCX))
            {
                if (Module.AutoRegister != null)
                    throw new BuildException("There can be only one <autoregister> element for a module", Location);

                Module.AutoRegister = m_AutoRegister;
            }
            else
                throw new BuildException("<autoregister> is not applicable for this module type", Location);
        }
    }
}