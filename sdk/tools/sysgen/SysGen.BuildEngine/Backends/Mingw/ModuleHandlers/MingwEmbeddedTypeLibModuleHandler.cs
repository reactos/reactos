using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Backends
{
    public class MingwEmbeddedTypeLibModuleHandler : MingwRBuildModuleHandler
    {
        public MingwEmbeddedTypeLibModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void CheckSourceFiles()
        {
            if (Module.SourceFiles.Count > 1)
            {
                throw new BuildException("Modules of type 'EmbeddedTypeLib' can only contain 1 source file , this module contains '{0}", Module.SourceFiles.Count);
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
                WriteWIDLTypeLibrary(sourceFile);
            }
        }

        protected override void WriteLinker()
        {
            //WriteAr();
        }
    }
}
