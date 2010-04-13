using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwBootSectorModuleHandler : MingwRBuildModuleHandler
    {
        public MingwBootSectorModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsNASM);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsNASM)
            {
                WriteNASMCompiler(sourceFile);
            }
        }

        protected override void WriteLinker()
        {
        }
    }
}
