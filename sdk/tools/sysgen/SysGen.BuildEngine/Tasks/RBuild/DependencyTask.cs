using System;
using SysGen.BuildEngine.Attributes;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("dependency")]
    public class DependencyTask : ValueBaseTask
    {
        /// <summary>
        /// The define value.
        /// </summary>
        [TaskValue(Required = true)]
        public virtual string Value { get { return _value; } set { _value = value; } }

        protected override void ExecuteTask()
        {
            RBuildModule dependency = Project.Modules.GetByName(Value);

            if (dependency == null)
                throw new BuildException("Unknown dependency '{0}' referenced by module '{1}'", Value, Module.Name);

            Module.Dependencies.Add(dependency);
        }
    }
}
