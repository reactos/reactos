using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("requires")]
    public class RequiresTask : ValueBaseTask
    {
        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue(Required = true)]
        public virtual string Value { get { return _value; } set { _value = value; } }

        protected override void ExecuteTask()
        {
            RBuildModule requeriment = Project.Modules.GetByName(Value);

            if (requeriment == null)
                throw new BuildException("Unknown requeriment '{0}' referenced by module '{1}'", Value, Module.Name);

            Module.Requeriments.Add(requeriment);
        }
    }
}
