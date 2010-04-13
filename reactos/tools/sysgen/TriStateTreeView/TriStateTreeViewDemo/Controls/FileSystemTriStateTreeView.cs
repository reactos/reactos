using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Windows.Forms;

using SysGen.RBuild.Framework;

using SIL.FieldWorks.Common.Controls;

namespace TriStateTreeViewDemo
{
    public class FileSystemTriStateTreeView : TriStateTreeView
    {
        RBuildModule m_Module = null;

        public void Load(RBuildModule module)
        {
            m_Module = module;

            if (Directory.Exists(module.ModulePath) == false)
                throw new DirectoryNotFoundException("Directory Not Found");

            Nodes.Clear();

            DirectoryNode node = new DirectoryNode(this, new DirectoryInfo(module.ModulePath));

            node.Expand();
        }

        public RBuildModule Module
        {
            get { return m_Module; }
        }

        protected override void OnAfterCheck(TreeViewEventArgs e)
        {
            base.OnAfterCheck(e);

            if (e.Node is FileNode)
            {
                FileNode fileNode = e.Node as FileNode;

                if (GetChecked(e.Node) == CheckState.Checked)
                {
                    if (fileNode.IsInclude)
                    {
                        //if (m_Module.Includes.Contains(fileNode.RelativeDirectory) == false)
                        //    m_Module.Includes.Add(fileNode.RelativeDirectory);
                    }
                    else
                    {
                        //if (m_Module.Files.Contains(fileNode.RelativePath) == false)
                        //    m_Module.Files.Add(fileNode.RelativePath);
                    }
                }
                else if (GetChecked(e.Node) == CheckState.Unchecked)
                {
                    if (fileNode.IsInclude == false)
                    {
                        //if (m_Module.Files.Contains(fileNode.RelativePath) == true)
                        //    m_Module.Files.Remove(fileNode.RelativePath);
                    }
                }
            }
        }

        public class DirectoryNode : TreeNode
        {
            private DirectoryInfo m_DirectoryInfo;

            public DirectoryNode(DirectoryNode parent, DirectoryInfo directoryInfo)
                : base(directoryInfo.Name)
            {
                m_DirectoryInfo = directoryInfo;

                parent.Nodes.Add(this);

                LoadDirectory();
                LoadFiles();
            }

            public DirectoryNode(FileSystemTriStateTreeView treeView, DirectoryInfo directoryInfo)
                : base(directoryInfo.Name)
            {
                m_DirectoryInfo = directoryInfo;

                treeView.Nodes.Add(this);

                LoadDirectory();
                LoadFiles();

            }

            public void LoadDirectory()
            {
                foreach (DirectoryInfo directoryInfo in m_DirectoryInfo.GetDirectories())
                {
                    new DirectoryNode(this, directoryInfo);
                }
            }

            public void LoadFiles()
            {
                foreach (FileInfo file in m_DirectoryInfo.GetFiles())
                {
                    if (file.Extension.ToLower() == ".c" ||
                        file.Extension.ToLower() == ".cpp" ||
                        file.Extension.ToLower() == ".cxx" ||
                        file.Extension.ToLower() == ".h")
                    {
                        new FileNode(this, file);
                    }
                }
            }

            public new FileSystemTriStateTreeView TreeView
            {
                get { return (FileSystemTriStateTreeView)base.TreeView; }
            }
        }

        public class FileNode : TreeNode
        {
            private FileInfo m_FileInfo;
            private DirectoryNode m_DirectoryNode;

            public FileNode(DirectoryNode directoryNode, FileInfo fileInfo)
                : base(fileInfo.Name)
            {
                m_DirectoryNode = directoryNode;
                m_FileInfo = fileInfo;

                m_DirectoryNode.Nodes.Add(this);
            }

            public string FilePath
            {
                get { return m_FileInfo.FullName; }
            }

            public string RelativePath
            {
                get { return FilePath.Replace(((FileSystemTriStateTreeView)TreeView).Module.ModulePath, string.Empty); }
            }

            public string RelativeDirectory
            {
                get { return RelativePath.Replace(@"\" + m_FileInfo.Name , string.Empty); }
            }

            public bool IsInclude
            {
                get { return (m_FileInfo.Extension.ToLower() == ".h"); }
            }
        }
    }
}
