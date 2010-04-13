using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    /// <summary>
    /// A bootstrap element specifies that the generated file should 
    /// be put on the bootable CD as a bootstrap file.
    /// </summary>
    public class RBuildBootstrapFile : RBuildCDFileBase
    {
        public RBuildBootstrapFile()
        {
        }

        public RBuildBootstrapFile(string basePath , string name)
        {
            Base = basePath;
            Name = name;
        }

        //public override RBuildFile CDNewFile
        //{
        //    get
        //    {
        //        RBuildFile file = (RBuildFile)Clone();

        //        file.Name = NewName;
        //        file.Base = InstallBase;
        //        file.Root = PathRoot.CDOutput;

        //        return file;
        //    }
        //}

        //public virtual RBuildFile CDNewFile
        //{
        //    get
        //    {
        //        RBuildFile file = (RBuildFile)Clone();

        //        file.Name = NewName;
        //        file.Base = InstallBase;
        //        file.Root = PathRoot.CDOutput;

        //        return file;
        //    }
        //}
    }
}