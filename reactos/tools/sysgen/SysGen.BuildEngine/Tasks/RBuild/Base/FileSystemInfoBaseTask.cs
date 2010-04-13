using System;
using System.IO;

using SysGen.BuildEngine.Attributes;
using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine.Tasks
{
    public abstract class AutoResolvableFileSystemInfoBaseTask : FileSystemInfoBaseTask
    {
        protected override bool TryToResolveBasePath()
        {
            if (Base == "eventlog_server")
            {
                int i = 10;
            }

            if ((Base != null) && (Base != string.Empty))
            {
                if (Base != Project.Name)
                {
                    // Get the referenced module
                    RBuildModule module = Project.Modules.GetByName(Base);

                    if (module == null)
                        throw new BuildException(string.Format("Could not resolve module base '{0}' for module '{1}'", Base, Module.Name, Location));

                    // If no path has been specified by the user
                    // set the module default path
                    if (Root == PathRoot.Default)
                        Root = module.IncludeDefaultRoot;

                    // Set the base to the module root
                    m_FileSystemInfo.Base = module.Folder.FullPath;
                }
                else
                {
                    // Set the base to the project root
                    m_FileSystemInfo.Base = Project.Base;
                }
            }
            else
            {
                // Set the base to the folder containing the module
                m_FileSystemInfo.Base = InFolder.FullPath;
            }

            if (m_FileSystemInfo.Base == null || m_FileSystemInfo.Name == null)
            {
                int i = 10;
            }

            return true;
        }
    }

    public abstract class FileSystemInfoBaseTask : Task
    {
        protected RBuildFileSystemInfo m_FileSystemInfo = null;

        public FileSystemInfoBaseTask()
        {
            CreateFileSystemObject();
        }

        protected abstract void CreateFileSystemObject();

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("base")]
        public string Base { get { return m_FileSystemInfo.Base; } set { m_FileSystemInfo.Base = value; } }

        /// <summary>
        /// The name of the define to set.
        /// </summary>        
        [TaskAttribute("root")]
        public PathRoot Root { get { return m_FileSystemInfo.Root; } set { m_FileSystemInfo.Root = value; } }

        /// <summary>
        /// Get the underlying <see cref="RBuildFile"/>.
        /// </summary>
        public RBuildFileSystemInfo FileSystemInfo
        {
            get { return m_FileSystemInfo; }
        }

        protected virtual void SetRBuildElement()
        {
            //m_FileSystemInfo.Element = RBuildElement;
        }

        protected virtual void SetRootFromParent()
        {
        }

        protected virtual bool TryToResolveBasePath ()
        {
            return false;
        }

        protected override void PreExecuteTask()
        {
            SetRootFromParent();
            SetRBuildElement();
        }

        protected override void ExecuteTask()
        {
            // Give the oportunity to subclasses to resolve the path
            if (!TryToResolveBasePath())
            {
                // If no base path has been specified default to current
                if (string.IsNullOrEmpty(Base))
                    Base = SysGenPathResolver.GetPath(this, SysGen.RootTask);
            }
        }
    }
}