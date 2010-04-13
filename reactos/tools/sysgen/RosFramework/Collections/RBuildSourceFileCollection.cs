using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class SourceCodePreferenceComparer : IComparer<RBuildSourceFile>
    {
        public int Compare(RBuildSourceFile x, RBuildSourceFile y)
        {
            if (x.First != y.First)
            {
                if (x.First)
                    return -1;
                else
                    return 1;
            }

            if (x.Extension == y.Extension)
                return 0;

            if (x.IsWidl)
                return -1;
            else if (y.IsWidl)
                return 1;

            if (x.IsAssembler)
                return 1;
            else if (y.IsAssembler)
                return -1;

            if (x.IsNASM)
                return 1;
            else if (y.IsNASM)
                return -1;

            return 0;
        }
    }

    public class RBuildSourceFileCollection : List<RBuildSourceFile>
    {
        public bool ContainsASM
        {
            get
            {
                foreach (RBuildSourceFile file in this)
                {
                    if ((file.IsAssembler) || (file.IsNASM))
                    {
                        return true;
                    }
                }

                // This module does not contain C++ code
                return false;
            }
        }
    }
}
