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
    public class CatalogTriStateTreeView : TriStateTreeView
    {
        private ISysGenDesigner m_SysGenDesigner = null;
        private TreeNode m_PlatformNode = null;

        public CatalogTriStateTreeView()
        {
        }

        private void CatalogTriStateTreeView_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            if (m_SysGenDesigner.SysGenEngine.Project != null)
            {
                if (e.Node is ModuleTreeNode)
                {
                    //Get the underlying module
                    ModuleTreeNode moduleNode = e.Node as ModuleTreeNode;

                    if (GetChecked(e.Node) == CheckState.Unchecked)
                    {
                        //Add the selected node and dependencies
                        m_SysGenDesigner.PlatformController.Add(moduleNode.Module);
                    }
                    else
                    {
                        //Remove the module from our platform
                        m_SysGenDesigner.PlatformController.Remove(moduleNode.Module);
                    }

                    //Update current module status
                    UpdateCatalogTree();
                }
                else if (e.Node is FolderTreeNode)
                {
                    if (m_AutoUpdatingParents == false)
                    {
                        //e.Cancel = (AddTreeNodeModules(e.Node) == false);
                    }
                }
            }
            else
            {
                MessageBox.Show("Cannot modify a catalog tree without platform associated");
            }
        }

        public void SetCatalog(ISysGenDesigner sysGenDesigner)
        {
            //Set the software catalog
            m_SysGenDesigner = sysGenDesigner;

            //Load the platform tree catalog
            LoadCatalogTree();
            UpdateCatalogTree();

            NodeMouseClick += new TreeNodeMouseClickEventHandler(CatalogTriStateTreeView_NodeMouseClick);
        }

        void CatalogTriStateTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            ModuleTreeNode moduleNode = e.Node as ModuleTreeNode;

            if (moduleNode != null)
                m_SysGenDesigner.InspectedObject = moduleNode.Module;
        }

        private void LoadCatalogTree()
        {
            m_PlatformNode = new TreeNode();

            LoadPlatformModules(m_PlatformNode, m_SysGenDesigner.SysGenEngine.ProjectTask);

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
                    m_SysGenDesigner.PlatformController.Project.Platform.Modules.Count,
                    m_SysGenDesigner.PlatformController.Project.Modules.Count);
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
                    if (m_SysGenDesigner.PlatformController.Project.Platform.Modules.Contains(moduleNode.Module))
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

        private void LoadPlatformModules(TreeNode node, Task task)
        {
            if (task is ModuleTask)
            {
                RBuildModule module = ((ModuleTask)task).Module;

                node.Nodes.Add(new ModuleTreeNode(module));
            }
            else if (task is ITaskContainer)
            {
                if (task is DirectoryTask)
                {
                    FolderTreeNode taskNode = new FolderTreeNode(((DirectoryTask)task).Name);

                    node.Nodes.Add(taskNode);

                    foreach (Task innerTask in ((ITaskContainer)task).ChildTasks)
                    {
                        LoadPlatformModules(taskNode, innerTask);
                    }
                }
                else
                {
                    foreach (Task innerTask in ((ITaskContainer)task).ChildTasks)
                    {
                        LoadPlatformModules(node, innerTask);
                    }
                }
            }
        }

        public abstract class CatalogTreeNode : TreeNode
        {
            public abstract string NodeName { get; }
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
        }
    }
}