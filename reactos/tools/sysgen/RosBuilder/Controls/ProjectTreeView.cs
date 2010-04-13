using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;

namespace TriStateTreeViewDemo
{
    public class ProjectTreeView : TreeView
    {
        private ISysGenDesigner m_SysGenDesigner = null;
        private TreeNode m_Project = new TreeNode("Project");
        private TreeNode m_Platforms = new TreeNode("Platform");
        private TreeNode m_Languages = new TreeNode("Languages");
        private TreeNode m_DebugChannels = new TreeNode("Debug Channels");
        //private TreeNode m_Filters = new TreeNode("Filters");
        private TreeNode m_Files = new TreeNode("Files");
        private TreeNode m_Registry = new TreeNode("Registry");
        //private TreeNode m_Filters = new TreeNode("Filters");

        public ProjectTreeView()
        {
        }

        public void SetCatalog(ISysGenDesigner sysGenDesigner)
        {
            //Set the software catalog
            m_SysGenDesigner = sysGenDesigner;
            m_SysGenDesigner.ProjectController.ProjectLoaded += new EventHandler(PlatformController_ProjectLoaded);
            m_SysGenDesigner.ProjectController.ProjectUpdated += new EventHandler(PlatformController_ProjectUpdated);
                      

            LoadProject();
        }

        void PlatformController_ProjectLoaded(object sender, EventArgs e)
        {
            LoadProject();
            UpdateProject();
        }

        void PlatformController_ProjectUpdated(object sender, EventArgs e)
        {
            UpdateProject();
        }

        private void LoadProject()
        {
            Nodes.Clear();

            m_Project = new ProjectTreeNode(m_SysGenDesigner.ProjectController.SysGenProject);
            m_Project.Nodes.Add(m_Platforms);
            m_Project.Nodes.Add(m_Languages);
            m_Project.Nodes.Add(m_DebugChannels);
//            m_Project.Nodes.Add(m_Filters);
            m_Project.Nodes.Add(m_Files);
            m_Project.Nodes.Add(m_Registry);
            m_Project.Expand();

            Nodes.Add(m_Project);
        }

        private void UpdateProject()
        {
            m_Languages.Nodes.Clear();
            m_DebugChannels.Nodes.Clear();

            foreach (RBuildLanguage language in m_SysGenDesigner.ProjectController.Project.Platform.Languages)
            {
                m_Languages.Nodes.Add(language.Name);
            }

            foreach (RBuildDebugChannel channel in m_SysGenDesigner.ProjectController.Project.Platform.DebugChannels)
            {
                m_DebugChannels.Nodes.Add(channel.Name);
            }
        }

        public abstract class CatalogTreeNode : TreeNode
        {
            public abstract string NodeName { get; }
            public abstract object NodeObject { get;}
        }

        public class ProjectTreeNode : CatalogTreeNode
        {
            Project m_Platform = null;

            public ProjectTreeNode(Project platform)
            {
                m_Platform = platform;
                Text = platform.FileName;
            }

            public Project Platform
            {
                get { return m_Platform; }
            }

            public override string NodeName
            {
                get { return m_Platform.Name; }
            }

            public override object NodeObject
            {
                get { return Platform; }
            }
        }

        public class ModuleTreeNode : CatalogTreeNode
        {
            RBuildModule m_Module = null;

            public ModuleTreeNode(RBuildModule module)
            {
                m_Module = module;
                Text = module.Name;
            }

            public RBuildModule Module
            {
                get { return m_Module; }
            }

            public override string NodeName
            {
                get { return Module.Name; }
            }

            public override object NodeObject
            {
                get { return Module; }
            }
        }

        public class FolderTreeNode : CatalogTreeNode
        {
            private string m_FolderName = null;

            public FolderTreeNode(string name)
            {
                m_FolderName = name;
                Text = name;
            }

            public override string NodeName
            {
                get { return m_FolderName; }
            }

            public override object NodeObject
            {
                get { return NodeName; }
            }
        }
    }
}
