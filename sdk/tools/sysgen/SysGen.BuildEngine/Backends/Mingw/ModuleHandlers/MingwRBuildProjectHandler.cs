using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwRBuildProjectHandler : MingwRBuildElementHandler
    {
        public MingwRBuildProjectHandler(RBuildProject project)
            : base(project)
        {
        }

        protected override void WriteSpecific()
        {
            Makefile.WritePropertyListStart(Project.MakeFileGCCOptions);
            WriteCompilerFlags(Makefile, Project);
            Makefile.WritePropertyListEnd();

            Makefile.WritePropertyAppend(Project.MakeFileCFlags, "-Wall");
            
            if (Project.Properties["OARCH"].Value != string.Empty)
            {
                Makefile.WritePropertyAppend(Project.MakeFileCFlags, "-march=$(OARCH)");
            }
            
            Makefile.WritePropertyAppend(Project.MakeFileCFlags, Project.MakeFileGCCOptionsMacro);
        }

        public RBuildProject Project
        {
            get { return m_BuildElement as RBuildProject; }
        }
    }
}
