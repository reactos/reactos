using System;
using System;
using System.Collections;
using System.Collections.Generic;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine 
{
    public class DefineCollection : Dictionary<string, string>
    {
        public void Add(string name)
        {
            Add(name, string.Empty);
        }

        public void Add(string name , string value) 
        {
            if (!ContainsKey(name))
                base.Add(name, value);
        }
    }
}