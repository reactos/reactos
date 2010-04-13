using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine
{
    public class MingwIdlHeaderModuleHandler : MingwRBuildModuleHandler
    {
        public MingwIdlHeaderModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void WriteCommon()
        {
        }

        protected override void WriteLinker()
        {
        }

        protected override void WriteSpecific()
        {
            WriteWIDLFlags();
//            WriteTarget();
            WritePreconditions();
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsWidl);
        }

        protected override void WriteWIDLFlags()
        {
            Makefile.WriteLine(Module.MakeFileWIDLFlags + " := " + Project.MakeFileWIDLFlagsMacro + " -I" + ModuleFolder.BaseFullPath);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsWidl)
            {
                WriteWIDLHeader(sourceFile);
            }
        }
    }
}
