using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildAPIStatusCollection : List<RBuildAPIInfo>
    {
        public int Percentage
        {
            get 
            {
                if (TotalFunctions == 0)
                    return 100;

                return ((100  * ImplementedFunctionsCount) / TotalFunctions); 
            }
        }

        public int TotalFunctions
        {
            get { return (ImplementedFunctionsCount + UnImplementedFunctionsCount); }
        }

        public int ImplementedFunctionsCount
        {
            get
            {
                int count = 0;

                foreach (RBuildAPIInfo info in this)
                    if (info.Implemented == true)
                        count++;

                return count;
            }
        }

        public int UnImplementedFunctionsCount
        {
            get
            {
                int count = 0;

                foreach (RBuildAPIInfo info in this)
                    if (info.Implemented == false)
                        count++;

                return count;
            }
        }
    }
}
