using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Backends;

namespace SysGen.BuildEngine
{
    public class MingwMessageHeaderModuleHandler : MingwRBuildModuleHandler
    {
        public MingwMessageHeaderModuleHandler(RBuildModule module)
            : base(module)
        {
        }

        protected override void WriteCommon()
        {
        }

        protected override void WriteLinker()
        {
        }

        protected override void WriteSpecific()
        {
            WritePreconditions();
        }

        protected override bool CanCompile(RBuildSourceFile file)
        {
            return (file.IsMessageTable);
        }

        protected override void WriteFileBuildInstructions(SourceFile sourceFile)
        {
            if (sourceFile.File.IsMessageTable)
            {
                WriteWMC(sourceFile);
            }
        }
    }
}
