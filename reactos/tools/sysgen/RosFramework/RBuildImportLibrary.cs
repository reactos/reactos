using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    /// <summary>
    /// An importlibrary element specifies that an import library should be 
    /// generated which other modules can use to link with the current module.
    /// </summary>
    public sealed class RBuildImportLibrary : RBuildFile
    {
        private string m_DllName = null;

        /// <summary>
        /// Creates a new instance of the <see cref="RBuildImportLibrary"/>.
        /// </summary>
        public RBuildImportLibrary()
        { 
        }

        public string DllName 
        { 
            get { return m_DllName; } 
            set { m_DllName = value; } 
        }

        public bool IsSpecFile
        {
            get { return Name.EndsWith(".spec.def"); }
        }

        public string Definition
        {
            get { return Name; }
        }
    }
}
