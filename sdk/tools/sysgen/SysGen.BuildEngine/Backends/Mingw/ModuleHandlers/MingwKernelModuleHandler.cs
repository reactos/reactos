using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwKernelModuleHandler : MingwRBuildModuleHandler
    {
        public MingwKernelModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsHeader || file.IsC || file.IsWindResource || file.IsCPP || file.IsAssembler || file.IsMessageTable);
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

            if (sourceFile.File.IsMessageTable)
            {
                WriteWMC(sourceFile);
            }
        }

        protected override string AdditionalParameters2
        {
            get
            {
                return "";
                //return "-Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared";
            }
        }

        protected override string SubSystem
        {
            get { return "native"; }
        }
    }
}
