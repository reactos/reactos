/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Martin Hentschel <info@cl-soft.de>
 *
 */

using System.Windows.Forms;
using Lextm.SharpSnmpLib.Mib;
using Lextm.SharpSnmpLib.Mib.Elements;
using Lextm.SharpSnmpLib.Mib.Elements.Types;
using Lextm.SharpSnmpLib.Mib.Elements.Entities;
using System.IO;

namespace LwipMibViewer
{
	public partial class FormMain : Form
	{
		readonly ListViewGroup listviewgroupAbstract;
		readonly ListViewGroup listviewgroupElement;
		readonly ListViewGroup listviewgroupBaseType;
		readonly ListViewGroup listviewgroupTypeChain;

		public FormMain()
		{
			this.Font = SystemInformation.MenuFont;
			InitializeComponent();

			this.listviewgroupAbstract = new ListViewGroup("Abstract", System.Windows.Forms.HorizontalAlignment.Left);
			this.listviewgroupElement = new ListViewGroup("Element Properties", System.Windows.Forms.HorizontalAlignment.Left);
			this.listviewgroupBaseType = new ListViewGroup("Element Base Type", System.Windows.Forms.HorizontalAlignment.Left);
			this.listviewgroupTypeChain = new ListViewGroup("Element Type Chain", System.Windows.Forms.HorizontalAlignment.Left);
			this.listviewNodeDetails.Groups.AddRange(new System.Windows.Forms.ListViewGroup[] {
					 listviewgroupAbstract,
					 listviewgroupElement,
					 listviewgroupBaseType,
					 listviewgroupTypeChain});

			try
			{
				DirectoryInfo dirInfo = new DirectoryInfo(Path.GetDirectoryName(Application.ExecutablePath));
				if (dirInfo != null)
				{
					dirInfo = dirInfo.Parent;
					if (dirInfo != null)
					{
						dirInfo = dirInfo.Parent;
						if (dirInfo != null)
						{
							dirInfo = new DirectoryInfo(Path.Combine(dirInfo.FullName, "Mibs"));
							if (dirInfo.Exists)
							{
								MibTypesResolver.RegisterResolver(new FileSystemMibResolver(dirInfo.FullName, true));
							}
						}
					}
				}
			}
			catch
			{ }
		}

		#region GUI Event Handler

		private void toolbuttonOpenMib_Click(object sender, System.EventArgs e)
		{
			if (this.dialogOpenMib.ShowDialog() == DialogResult.OK)
			{
				OpenMib(this.dialogOpenMib.FileName);
			}
		}

		private void treeMib_AfterSelect(object sender, TreeViewEventArgs e)
		{
			listviewNodeDetails.Items.Clear();

			if (e.Node != null)
			{
				MibTreeNode mtn = e.Node.Tag as MibTreeNode;
				if (mtn != null)
				{
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Abstract", mtn.NodeType.ToString() }, this.listviewgroupAbstract));

					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Module", (mtn.Entity.Module != null) ? mtn.Entity.Module.Name : "" }, this.listviewgroupElement));
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Type", mtn.Entity.GetType().Name }, this.listviewgroupElement));
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Name", mtn.Entity.Name }, this.listviewgroupElement));
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Description", mtn.Entity.Description }, this.listviewgroupElement));
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "OID", mtn.Entity.Value.ToString() }, this.listviewgroupElement));
					listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Full OID", MibTypesResolver.ResolveOid(mtn.Entity).GetOidString() }, this.listviewgroupElement));
					if (mtn.Entity is ObjectType)
					{
						listviewNodeDetails.Items.Add(new ListViewItem(new string[] { "Access", (mtn.Entity as ObjectType).Access.ToString() }, this.listviewgroupElement));
					}

					ITypeReferrer tr = mtn.Entity as ITypeReferrer;
					if (tr != null)
					{
						ShowTypeDetails(listviewNodeDetails, this.listviewgroupBaseType, tr.BaseType);
						ShowTypeChain(listviewNodeDetails, tr.ReferredType);
					}
				}
			}
		}

		#endregion

		#region Methods

		private void OpenMib(string file)
		{
			try
			{
				MibDocument md = new MibDocument(file);
				MibTypesResolver.ResolveTypes(md.Modules[0]);

				this.treeMib.Nodes.Clear();
				this.listviewNodeDetails.Items.Clear();

				MibTree mt = new MibTree(md.Modules[0] as MibModule);
				foreach (MibTreeNode mibTreeNode in mt.Root)
				{
					AddNode(mibTreeNode, this.treeMib.Nodes);

					foreach (TreeNode node in this.treeMib.Nodes)
					{
						node.Expand();
					}
				}
			}
			catch
			{
			}
		}

		private void AddNode(MibTreeNode mibNode, TreeNodeCollection parentNodes)
		{
			int imgIndex = 5; //unknown
			if ((mibNode.NodeType & MibTreeNodeType.Table) != 0)
			{
				imgIndex = 1;
			}
			else if ((mibNode.NodeType & MibTreeNodeType.TableRow) != 0)
			{
				imgIndex = 2;
			}
			else if ((mibNode.NodeType & MibTreeNodeType.TableCell) != 0)
			{
				imgIndex = 3;
			}
			else if ((mibNode.NodeType & MibTreeNodeType.Scalar) != 0)
			{
				imgIndex = 4;
			}
			else if ((mibNode.NodeType & MibTreeNodeType.Container) != 0)
			{
				imgIndex = 0;
			}

			TreeNode newNode = new TreeNode(mibNode.Entity.Name, imgIndex, imgIndex);
			newNode.Tag = mibNode;

			parentNodes.Add(newNode);

			foreach (MibTreeNode child in mibNode.ChildNodes)
			{
				AddNode(child, newNode.Nodes);
			}
		}

		private void ShowTypeChain(ListView lv, ITypeAssignment type)
		{
			ShowTypeDetails(lv, this.listviewgroupTypeChain, type);

			ITypeReferrer tr = type as ITypeReferrer;
			if ((tr != null) && (tr.ReferredType != null))
			{
				lv.Items.Add(new ListViewItem(new string[] { " >>>", "" }, this.listviewgroupTypeChain));
				ShowTypeChain(listviewNodeDetails, tr.ReferredType);
			}
		}

		private void ShowTypeDetails(ListView lv, ListViewGroup lvg, ITypeAssignment type)
		{
			lv.Items.Add(new ListViewItem(new string[] { "Module", (type.Module != null) ? type.Module.Name : "" }, lvg));
			lv.Items.Add(new ListViewItem(new string[] { "Type", type.GetType().Name }, lvg));
			lv.Items.Add(new ListViewItem(new string[] { "Name", type.Name }, lvg));
		}

		#endregion

	}
}
