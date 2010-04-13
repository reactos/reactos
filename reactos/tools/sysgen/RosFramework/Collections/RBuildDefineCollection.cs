using System;
using System.Collections;
using System.Collections.Generic;

namespace SysGen.RBuild.Framework
{
    public class RBuildDefineCollection : List<RBuildDefine>
    {
        public bool IsDefined(string name)
        {
            foreach (RBuildDefine define in this)
            {
                if (define.Name == name)
                    return true;
            }

            return false;
        }

        public void Add(string name)
        {
            Add(new RBuildDefine(name));
        }
    }
}