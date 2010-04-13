using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwWin32DLLModuleHandler : MingwRBuildModuleHandler
    {
        public MingwWin32DLLModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsHeader || file.IsC || file.IsCPP || file.IsWindResource || file.IsAssembler || file.IsWineBuild || file.IsWidl || file.IsMessageTable);
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

            if (sourceFile.File.IsCPP)
            {
                WriteCPPCompiler(sourceFile);
            }

            if (sourceFile.File.IsWindResource)
            {
                WriteWindResCompiler(sourceFile);
            }

            if (sourceFile.File.IsAssembler)
            {
                WriteASMCompiler(sourceFile);
            }

            if (sourceFile.File.IsMessageTable)
            {
                WriteWMC(sourceFile);
            }

            if (sourceFile.File.IsWidl)
            {
                WriteWIDLHeader(sourceFile);
            }
        }
    }
}