using System;
using System.Drawing;
using System.Text;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

using SIL.FieldWorks.Common.Controls;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;

namespace TriStateTreeViewDemo
{
    public class PlatformTreeView : TriStateTreeView
    {
        private ISysGenDesigner m_SysGenDesigner = null;
        private PlatformTreeNode m_PlatformNode = null;

        public PlatformTreeView()
        {
        }

        private void CatalogTriStateTreeView_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            if (m_SysGenDesigner.ProjectController.Project != null)
            {
                if (e.Node is ModuleTreeNode)
                {
                    //Get the underlying module
                    ModuleTreeNode moduleNode = e.Node as ModuleTreeNode;

                    if (GetChecked(e.Node) == CheckState.Unchecked)
                    {
                        //Add the selected node and dependencies
                        m_SysGenDesigner.ProjectController.Add(moduleNode.Module);
                    }
                    else
                    {
                        //Remove the module from our platform
                        m_SysGenDesigner.ProjectController.Remove(moduleNode.Module);
                    }

                }
                else if (e.Node is FolderTreeNode)
                {
                }
            }
            else
            {
                MessageBox.Show("Cannot modify a catalog tree without platform associated");
            }

            /* cancel the paint event*/
            e.Cancel = true;
        }

        public void SetCatalog(ISysGenDesigner sysGenDesigner)
        {
            //Set the software catalog
            m_SysGenDesigner = sysGenDesigner;

            m_SysGenDesigner.ProjectController.PlatformModulesUpdated += new EventHandler(PlatformController_PlatformModulesUpdated);

            //Load the platform tree catalog
            LoadCatalogTree();
            UpdateCatalogTree();

            NodeMouseClick += new TreeNodeMouseClickEventHandler(CatalogTriStateTreeView_NodeMouseClick);
        }

        void PlatformController_PlatformModulesUpdated(object sender, EventArgs e)
        {
            //Update current module status
            UpdateCatalogTree();
        }

        void CatalogTriStateTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            CatalogTreeNode catalogNode = e.Node as CatalogTreeNode;

            if (catalogNode != null)
                m_SysGenDesigner.InspectedObject = catalogNode.NodeObject;
        }

        private void LoadCatalogTree()
        {
            m_PlatformNode = new PlatformTreeNode(m_SysGenDesigner.ProjectController.Project.Platform);

            LoadPlatformModules(m_PlatformNode);
            //LoadPlatformModules(m_PlatformNode, m_SysGenDesigner.SysGenEngine.ProjectTask);

            Nodes.Clear();
            Nodes.Add(m_PlatformNode);

            m_PlatformNode.Text = RootNodeText;
            m_PlatformNode.Expand();
        }

        private void UpdateCatalogTree()
        {
            BeginUpdate();
            BeforeCheck -= new TreeViewCancelEventHandler(CatalogTriStateTreeView_BeforeCheck);

            m_PlatformNode.Text = RootNodeText;
            m_PlatformNode.Expand();

            UpdatePlatformModules(m_PlatformNode);

            EndUpdate();
            BeforeCheck += new TreeViewCancelEventHandler(CatalogTriStateTreeView_BeforeCheck);
        }

        private string RootNodeText
        {
            get
            {
                return string.Format("Catalog ({0} Modules-{1} Available)",
                    m_SysGenDesigner.ProjectController.Project.Platform.Modules.Count,
                    m_SysGenDesigner.ProjectController.Project.Modules.Count);
            }
        }

        private void UpdatePlatformModules(TreeNode node)
        {
            if (node is ModuleTreeNode)
            {
                //Get the underlying module
                ModuleTreeNode moduleNode = node as ModuleTreeNode;

                if (moduleNode != null)
                {
                    if (m_SysGenDesigner.ProjectController.Project.Platform.Modules.Contains(moduleNode.Module))
                    {
                        SetChecked(moduleNode, CheckState.Checked);
                    }
                    else
                    {
                        SetChecked(moduleNode, CheckState.Unchecked);
                    }
                }
            }
            else
            {
                foreach (TreeNode subNode in node.Nodes)
                {
                    UpdatePlatformModules(subNode);
                }
            }
        }

        private void LoadPlatformModules(TreeNode node)
        {
            foreach (RBuildModule module in m_SysGenDesigner.ProjectController.AvailableModules)
            {
                TreeNode parent = node;

                foreach (string part in module.CatalogPath.Split(new char[] { '\\' }))
                {
                    if (part.Length > 0)
                    {
                        parent = GetFolderNode(parent, part);
                    }
                }

                parent.Nodes.Add(new ModuleTreeNode(module));
            }
        }

        private TreeNode GetFolderNode(TreeNode parent, string name)
        {
            foreach (TreeNode node in parent.Nodes)
            {
                CatalogTreeNode folderNode = node as CatalogTreeNode;

                if (folderNode != null)
                {
                    if (folderNode.NodeName == name)
                    {
                        return folderNode;
                    }
                }
            }

            FolderTreeNode newNode = new FolderTreeNode(name);

            parent.Nodes.Add(newNode);

            return newNode;
        }

        //private void LoadPlatformModules(TreeNode node, Task task)
        //{
        //    foreach (string path in 
        //    /*
        //    if (task is ModuleTask)
        //    {
        //        RBuildModule module = ((ModuleTask)task).Module;

        //        node.Nodes.Add(new ModuleTreeNode(module));
        //    }
        //    else if (task is ITaskContainer)
        //    {
        //        if (task is DirectoryTask)
        //        {
        //            FolderTreeNode taskNode = new FolderTreeNode(((DirectoryTask)task).Name);

        //            node.Nodes.Add(taskNode);

        //            foreach (Task innerTask in ((ITaskContainer)task).ChildTasks)
        //            {
        //                LoadPlatformModules(taskNode, innerTask);
        //            }
        //        }
        //        else
        //        {
        //            foreach (Task innerTask in ((ITaskContainer)task).ChildTasks)
        //            {
        //                LoadPlatformModules(node, innerTask);
        //            }
        //        }
        //    }
        //    */
        //}

        public abstract class CatalogTreeNode : TreeNode
        {
            public abstract string NodeName { get; }
            public abstract object NodeObject { get;}
        }

        public class PlatformTreeNode : CatalogTreeNode
        {
            RBuildPlatform m_Platform = null;

            public PlatformTreeNode(RBuildPlatform platform)
            {
                m_Platform = platform;
                Text = platform.Name;
            }

            public RBuildPlatform Platform
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