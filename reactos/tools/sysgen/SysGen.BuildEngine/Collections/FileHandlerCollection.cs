using System;
using System;
using System.Collections;
using System.Collections.Generic;

using SysGen.BuildEngine.Log;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine 
{
    public sealed class FileHandlerCollection : List<IFileHandler>
    {
        public void ProcessFiles(RBuildPlatformFileCollection files)
        {
            foreach (RBuildFile file in files)
            {
                foreach (IFileHandler handler in this)
                {
                    handler.Process(file);
                }
            }
        }
    }
}