using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwStaticLibraryModuleHandler : MingwRBuildModuleHandler
    {
        public MingwStaticLibraryModuleHandler(RBuildModule module)
            : base(module)
        
        {
        }

        /*
        protected override void WriteLibs(RBuildModule module)
        {
        }
         */

        protected override void WriteCFlags()
        {
            base.WriteCFlags();

            if (Module.IsStartupLib)
            {
                Makefile.WritePropertyAppend(Module.MakeFileCFlags, "-Wno-main");
            }
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsHeader ||file.IsC || file.IsAssembler);
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
        }

        protected override void WriteLinker()
        {
            WriteAr();
        }
    }
}
