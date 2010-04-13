using System;
using System;
using System.Collections;
using System.Collections.Generic;

using SysGen.BuildEngine.Log;
using SysGen.BuildEngine.Backends;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine 
{
    public sealed class BackendCollection : List<Backend>
    {
        public void Generate()
        {
            foreach (Backend backend in this)
            {
                backend.Run();
            }
        }
    }
}