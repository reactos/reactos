using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum SetupType
    {
        Device,
        Component
    }

    public class RBuildSetupFile : RBuildPlatformFile
    {
        private SetupType m_SetupType = SetupType.Component;
        private bool m_InstallAlways = true;
        private string m_InstallSection = "DefaultInstall";

        public SetupType SetupType
        {
            get { return m_SetupType; }
            set { m_SetupType = value; }
        }

        public string InstallSection
        {
            get { return m_InstallSection; }
            set { m_InstallSection = value; }
        }

        public string DefaultInstallSection
        {
            get
            {
                switch (SetupType)
                {
                    case SetupType.Device:
                        return "DefaultInstall";
                    default:
                        return "DefaultInstall";
                }
            }
        }

        public bool InstallAlways
        {
            get { return m_InstallAlways; }
            set { m_InstallAlways = value; }
        }
    }
}
