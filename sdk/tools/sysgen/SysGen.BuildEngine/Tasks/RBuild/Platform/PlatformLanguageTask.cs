using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("platformlanguage")]
    public class PlatformLanguageTask : ValueBaseTask
    {
        public PlatformLanguageTask()
        {
        }

        protected override void ExecuteTask()
        {
            RBuildLanguage language = Project.Languages.GetByName(Value);

            if (language == null)
                throw new BuildException("Unknown language '{0}' referenced by <PlatformLanguage>", Value);

            Project.Platform.Languages.Add(language);
        }
    }
}