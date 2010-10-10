using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwRpcProxyModuleHandler : MingwRpcServerHeaderModuleHandler
    {
        public MingwRpcProxyModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsWidl);
        }

        protected override void WriteCleanTarget()
        {
            base.WriteCleanTarget();

            foreach (RBuildSourceFile file in Module.SourceFiles)
            {
                SourceFile cFile = new SourceFile(file, Module, SysGen);

                Makefile.WriteLine("\t-@$(rm) " + cFile.SourceCodeHeaderFile.IntermediateFullPath + " 2>$(NUL)");
                Makefile.WriteLine("\t-@$(rm) " + cFile.SourceCodeObjectFile.IntermediateFullPath + " 2>$(NUL)");
            }
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