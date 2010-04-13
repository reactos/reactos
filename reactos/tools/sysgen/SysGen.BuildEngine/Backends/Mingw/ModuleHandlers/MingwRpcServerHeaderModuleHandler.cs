using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwRpcServerHeaderModuleHandler : MingwIdlHeaderModuleHandler
    {
        public MingwRpcServerHeaderModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void WriteSpecific()
        {
            WriteCFlags();
            base.WriteSpecific();
            WriteModuleCommon();
        }

        protected override void WriteCleanTarget()
        {
            base.WriteCleanTarget();

            foreach (RBuildSourceFile file in Module.SourceFiles)
            {
                SourceFile cFile = new SourceFile(file, Module, SysGen);

                Makefile.WriteLine("\t-@$(rm) " + cFile.SourceCodeHeaderFile.IntermediateFullPath + " 2>$(NUL)");
                Makefile.WriteLine("\t-@$(rm) " + cFile.SourceCodeActualFile.IntermediateFullPath + " 2>$(NUL)");
            }
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsWidl);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsWidl)
            {
                WriteWIDLRpcHeader(sourceFile);
            }
        }
    }
}