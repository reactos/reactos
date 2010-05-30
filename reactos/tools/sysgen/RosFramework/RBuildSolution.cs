using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class RBuildSolution
    {
        private List<RBuildProject> m_Projects = null;

        /// <summary>
        /// The projects this solution contains.
        /// </summary>
        public List<RBuildProject> Projects
        {
            get { return m_Projects; }
            set { m_Projects = value; }
        }
    }
}
