using System;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    [TaskName("localization")]
    public class LocalizationTask : FileBaseTask
    {
        public LocalizationTask()
        {
        }

        protected override void CreateFileSystemObject()
        {
            m_FileSystemInfo = new RBuildLocalizationFile();
        }

        [TaskAttribute("isoname")]
        public string IsoName { get { return LocalizationFile.IsoName; } set { LocalizationFile.IsoName = value; } }

        [TaskAttribute("dirty")]
        public bool Dirty { get { return LocalizationFile.Dirty; } set { LocalizationFile.Dirty = value; } }

        private RBuildLocalizationFile LocalizationFile
        {
            get { return m_FileSystemInfo as RBuildLocalizationFile; }
        }

        protected override void ExecuteTask()
        {
            RBuildLanguage language = Project.Languages.GetByName(IsoName);

            if (language == null)
                throw new BuildException("Unknown language '{0}' referenced by module '{1}'", IsoName, Module.Name);

            Module.LocalizationFiles.Add(LocalizationFile);
        }
    }
}