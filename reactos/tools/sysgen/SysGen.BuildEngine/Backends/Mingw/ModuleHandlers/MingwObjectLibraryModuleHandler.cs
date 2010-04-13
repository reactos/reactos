using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwObjectLibraryModuleHandler : MingwRBuildModuleHandler
    {
        public MingwObjectLibraryModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsHeader || file.IsC || file.IsNASM || file.IsAssembler || file.IsMessageTable);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsHeader)
            {
                WritePCH(sourceFile);
            }

            if (sourceFile.File.IsC)
            {
                WriteCCompiler(sourceFile);
            }

            if (sourceFile.File.IsAssembler)
            {
                WriteASMCompiler(sourceFile);
            }

            if (sourceFile.File.IsNASM)
            {
                WriteNASMCompiler(sourceFile);
            }

            if (sourceFile.File.IsMessageTable)
            {
                WriteWMC(sourceFile);
            }
        }

        protected override void WriteLinker()
        {
        }

        protected override string SubSystem
        {
            get { return "console"; }
        }
    }
}
