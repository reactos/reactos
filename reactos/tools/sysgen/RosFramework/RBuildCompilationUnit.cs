using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    /// <summary>
    /// A compilationunit element specifies that one or more source code 
    /// files are to be compiled as a single compilation unit.
    /// </summary>
    public class RBuildCompilationUnitFile : RBuildSourceFile , IRBuildSourceFilesContainer
    {
        private RBuildSourceFileCollection m_SourceFiles = new RBuildSourceFileCollection();

        /// <summary>
        /// Gets the collection of <see cref="RBuildSourceFile"/>.
        /// </summary>
        public RBuildSourceFileCollection SourceFiles
        {
            get { return m_SourceFiles; }
        }
    }
}
