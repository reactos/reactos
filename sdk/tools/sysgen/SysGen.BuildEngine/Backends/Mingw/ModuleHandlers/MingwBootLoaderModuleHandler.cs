using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwBootLoaderModuleHandler : MingwRBuildModuleHandler
    {
        public MingwBootLoaderModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return false;
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
        }

        protected override void WriteLinker()
        {
            Makefile.WriteLine(Module.MakeFileTargetMacro + ": " + Module.MakeFileObjsMacro + " " + Module.MakeFileLinkDepsMacro + " | " + ModuleFolder.OutputFullPath);
            Makefile.WriteLine("\t$(ECHO_LD)");
            //Makefile.WriteLine("\t$(ld) {0} -N -Ttext=0x8000 -o {1} {2} {3}", Module.MakeFileLFlagsMacro, CompilableModule.JunkTempFileNameFullPath, Module.MakeFileObjsMacro, Module.MakeFileLinkDepsMacro);
            Makefile.WriteLine("\t$(gcc) -Wl,--subsystem,native -Wl,-N -Ttext=0x8000 -o {0} {1} {2} {3}", CompilableModule.JunkTempFileNameFullPath, Module.MakeFileObjsMacro, Module.MakeFileLinkDepsMacro, Module.MakeFileLFlagsMacro);
            Makefile.WriteLine("\t$(objcopy) -O binary {0} $@", CompilableModule.JunkTempFileNameFullPath);
            Makefile.WriteLine("\t-@$(rm) {0} 2>$(NUL)", CompilableModule.JunkTempFileNameFullPath);
        }
    }
}