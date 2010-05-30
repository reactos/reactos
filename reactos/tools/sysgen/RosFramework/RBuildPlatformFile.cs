using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildOutputFile : RBuildFile //, IRBuildInstallable
    {
        private string m_InstallBase = "."; //".";
        private string m_NewName = null;

        public string NewName
        {
            get
            {
                if (m_NewName == null)
                    return m_Name;
                return m_NewName;
            }
            set { m_NewName = value; }
        }

        public string InstallBase
        {
            get { return m_InstallBase; }
            set { m_InstallBase = value; }
        }

        public virtual RBuildFile CDNewFile
        {
            get
            {
                RBuildFile file = (RBuildFile)Clone();

                file.Name = NewName;
                file.Base = InstallBase;
                file.Root = PathRoot.CDOutput;

                return file;
            }
        }

        public virtual RBuildFile NewFile
        {
            get
            {
                RBuildFile file = (RBuildFile)Clone();

                file.Name = NewName;
                file.Base = InstallBase;
                file.Root = Root;

                return file;
            }
        }
    }

    public class RBuildPlatformFile : RBuildOutputFile
    {
        //protected RBuildInstallFolder m_InstallFolder = null;

        //public RBuildInstallFolder InstallFolder
        //{
        //    get { return m_InstallFolder; }
        //    set { m_InstallFolder = value; }
        //}

        public RBuildFile PlatformInstall
        {
            get
            {
                RBuildFile file = null;

                file = new RBuildFile();
                file.Base = "%SystemRoot%\\" + InstallBase;
                file.Name = Name;
                file.Root = PathRoot.Platform;

                return file;
            }
        }
    }
}
