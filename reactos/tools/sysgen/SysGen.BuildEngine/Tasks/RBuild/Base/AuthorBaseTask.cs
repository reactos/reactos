using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    public abstract class AuthorBaseTask : Task
    {
        protected string m_Alias = null;
        protected RBuildAuthor m_Author = new RBuildAuthor();

        [TaskValue(Required=true)]
        public virtual string Alias { get { return m_Alias; } set { m_Alias = value; } }

        protected override void ExecuteTask()
        {
            m_Author.Contributor = Project.Contributors.GetByName(Alias);

            if (m_Author.Contributor == null)
                throw new BuildException(string.Format("Could not resolve contributor '{0}' referenced by module '{1}'", Alias, Module.Name, Location));

            Module.Authors.Add(m_Author);
        }
    }
}