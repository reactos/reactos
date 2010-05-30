using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("metadata")]
    public class MetadataTask : Task
    {
        RBuildMetadata m_Metadata = new RBuildMetadata();

        /// <summary>
        /// The module description.
        /// </summary>        
        [TaskAttribute("description")]
        public string Description { get { return m_Metadata.Description; } set { m_Metadata.Description = value; } }

        protected override void ExecuteTask()
        {
            if (Module.Metadata != null)
                throw new BuildException("Only one <metadata ../> is allowed per module.", Location);

            Module.Metadata = m_Metadata;
        }
    }
}
