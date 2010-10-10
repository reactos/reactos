using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwNativeDLLModuleHandler : MingwRBuildModuleHandler
    {
        public MingwNativeDLLModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsC || file.IsWindResource || file.IsAssembler || file.IsMessageTable);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsC)
            {
                WriteCCompiler(sourceFile);
            }

            if (sourceFile.File.IsMessageTable)
            {
                WriteWMC(sourceFile);
            }

            if (sourceFile.File.IsWindResource)
            {
                WriteWindResCompiler(sourceFile);
            }

            if (sourceFile.File.IsAssembler)
            {
                WriteASMCompiler(sourceFile);
            }
        }

        protected override string SubSystem
        {
            get { return "native"; }
        }
    }
}
