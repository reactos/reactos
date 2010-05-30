using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwCabinetModuleHandler : MingwRBuildModuleHandler
    {
        public MingwCabinetModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return true;
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
        }

        protected override void WriteLinker()
        {
        }

        protected override void WriteSpecific()
        {
            base.WriteSpecific();

            Makefile.WriteLine(Module.MakeFileTargetMacro + ": $(cabman_TARGET) " + ModuleFolder.OutputFullPath);
            Makefile.WriteLine("\t$(ECHO_CABMAN)");
            Makefile.WriteLine("\t$(Q)$(cabman_TARGET) -M raw -S " + Module.MakeFileTargetMacro + " " + Module.MakeFileSourcesMacro);
            Makefile.WriteLine();
        }
    }
}