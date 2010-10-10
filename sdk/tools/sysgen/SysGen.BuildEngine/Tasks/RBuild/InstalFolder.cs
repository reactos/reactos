using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks 
{
    [TaskName("installfolder")]
    public class InstalFolder : Task
    {
        RBuildInstallFolder m_InstallFolder = new RBuildInstallFolder();

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("id")]
        public string ID { get { return m_InstallFolder.ID; } set { m_InstallFolder.ID = value; } }

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("name")]
        [TaskValue(Required = true)]
        public string Name { get { return m_InstallFolder.Name; } set { m_InstallFolder.Name = value; } }

        protected override void ExecuteTask()
        {
            Project.InstallFolders.Add(m_InstallFolder);
        }
    }
}