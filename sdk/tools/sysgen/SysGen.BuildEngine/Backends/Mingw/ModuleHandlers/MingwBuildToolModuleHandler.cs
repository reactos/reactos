using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwBuildToolModuleHandler : MingwRBuildModuleHandler
    {
        public MingwBuildToolModuleHandler(RBuildModule module)
            : base(module)
        
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsC || file.IsCPP);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsC)
            {
                WriteCCompiler(sourceFile);
            }

            if (sourceFile.File.IsCPP)
            {
                WriteCPPCompiler(sourceFile);
            }
        }

        protected override void WriteLinker()
        {
            Makefile.WriteLine(Module.MakeFileTargetMacro + ": " + Module.MakeFileObjsMacro + " " + Module.MakeFileLinkDepsMacro + " | " + ModuleFolder.OutputFullPath);
            Makefile.WriteLine("\t$(ECHO_LD)");
            Makefile.WriteLine("\t" + Linker + " " + Module.MakeFileLFlagsMacro + " -o $@ " + Module.MakeFileObjsMacro + " " + Module.MakeFileLibsMacro );
            Makefile.WriteLine();
        }
    }
}
