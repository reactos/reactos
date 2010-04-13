using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwKernelModeDriverModuleHandler : MingwRBuildModuleHandler
    {
        public MingwKernelModeDriverModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsC || file.IsWindResource || file.IsCPP || file.IsAssembler || file.IsWineBuild || file.IsHeader);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsHeader)
            {
                WritePCH(sourceFile);
            }

            if (sourceFile.File.IsWineBuild)
            {
                WriteWineBuild(sourceFile);
            }

            if (sourceFile.File.IsC)
            {
                WriteCCompiler(sourceFile);
            }

            if (sourceFile.File.IsWindResource)
            {
                WriteWindResCompiler(sourceFile);
            }

            if (sourceFile.File.IsCPP)
            {
                WriteCPPCompiler(sourceFile);
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
