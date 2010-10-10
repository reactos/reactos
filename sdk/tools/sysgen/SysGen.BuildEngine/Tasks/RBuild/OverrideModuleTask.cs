using System;
using System.Xml;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("overridemodule")]
    public class OverrideModuleTask : ModuleTask
    {
        public OverrideModuleTask()
        {
            m_FailOnMissingRequired = false;
        }

        protected override void OnLoad()
        {
            //Evitamos actualizar la información del módulo de verdad
        }

        protected override void PostExecuteTask()
        {
            //Evitamos actualizar la información del módulo de verdad            
        }

        protected override void PreExecuteTask()
        {
            //Evitamos actualizar la información del módulo de verdad
        }

        protected override void ExecuteTask()
        {
            //Evitamos actualizar la información del módulo de verdad
        }

        protected override void InitializeTask(XmlNode taskNode)
        {
            base.InitializeTask(taskNode);

            if (taskNode.Attributes["name"] == null)
                throw new BuildException("Missing 'name' attribute");

            string moduleName = taskNode.Attributes["name"].Value;

            m_Module = Project.Modules.GetByName(moduleName);

            if (m_Module == null)
                throw new BuildException("Overrided module '{0}' not found" , moduleName);
        }
    }
}