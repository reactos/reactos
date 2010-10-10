using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("language")]
    public class LanguageTask : Task
    {
        RBuildLanguage m_Language = new RBuildLanguage();

        [TaskAttribute("isoname")]
        public string IsoName { get { return m_Language.Name; } set { m_Language.Name = value; } }

        [TaskAttribute("lcid")]
        public string LCID { get { return m_Language.LCID; } set { m_Language.LCID = value; } }

        protected override void ExecuteTask()
        {
            Project.Languages.Add(m_Language);
        }
    }
}