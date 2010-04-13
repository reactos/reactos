// ---------------------------------------------------------------------------------------------
#region // Copyright (c) 2004-2005, SIL International. All Rights Reserved.   
// <copyright from='2004' to='2005' company='SIL International'>
//		Copyright (c) 2004-2005, SIL International. All Rights Reserved.   
//    
//		Distributable under the terms of either the Common Public License or the
//		GNU Lesser General Public License, as specified in the LICENSING.txt file.
// </copyright> 
#endregion
// 
// File: TriStateTreeViewTests.cs
// Responsibility: TE Team
// 
// <remarks>
// </remarks>
// ---------------------------------------------------------------------------------------------
using System;
using System.Windows.Forms;
using NUnit.Framework;

namespace SIL.FieldWorks.Common.Controls
{
	#region DummyTriStateTreeView
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// 
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	public class DummyTriStateTreeView: TriStateTreeView
	{
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Exposes ChangeNodeState for testing
		/// </summary>
		/// <param name="node"></param>
		/// ------------------------------------------------------------------------------------
		public void CallChangeNodeState(TreeNode node)
		{
			ChangeNodeState(node);
		}
	}
	#endregion

	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Tests for TriStateTreeView control.
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	[TestFixture]
	public class TriStateTreeViewTests
	{
		private DummyTriStateTreeView m_treeView;
		private TreeNode m_aNode;
		private TreeNode m_bNode;
		private TreeNode m_c1Node;
		private TreeNode m_c2Node;
		private TreeNode m_dNode;
		private bool m_fBeforeCheck;
		private bool m_fCancelInBeforeCheck;
		private bool m_fAfterCheck;

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Initialize a test
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[SetUp]
		public void Init()
		{
			m_fBeforeCheck = false;
			m_fAfterCheck = false;
			m_fCancelInBeforeCheck = false;
			m_treeView = new DummyTriStateTreeView();

			m_dNode = new TreeNode("d");
			m_c1Node = new TreeNode("c1", new TreeNode[] { m_dNode });
			m_c2Node = new TreeNode("c2");
			m_bNode = new TreeNode("b", new TreeNode[] { m_c1Node, m_c2Node});
			m_aNode = new TreeNode("a", new TreeNode[] { m_bNode });
			m_treeView.Nodes.Add(m_aNode);
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that all nodes in the tree view are initially unchecked
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void InitiallyUnchecked()
		{
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_aNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_bNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c1Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c2Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_dNode));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that changing a node changes all children
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void ChangeNodeChangesAllChildren_Check()
		{
			// Check a node -> should check all children
			m_treeView.SetChecked(m_bNode, TriStateTreeView.CheckState.Checked);

			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_bNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c1Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c2Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_dNode));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that changing a node changes all children
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void ChangeNodeChangesAllChildren_Uncheck()
		{
			// uncheck a node -> should uncheck all children
			m_treeView.SetChecked(m_bNode, TriStateTreeView.CheckState.Unchecked);

			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_bNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c1Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c2Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_dNode));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that parent get greyed out if children are not all in same state
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void ChangeParent_CheckOneChild()
		{
			// check child -> grey check all parents
			m_treeView.SetChecked(m_c2Node, TriStateTreeView.CheckState.Checked);

			Assert.AreEqual(TriStateTreeView.CheckState.GreyChecked, m_treeView.GetChecked(m_aNode));
			Assert.AreEqual(TriStateTreeView.CheckState.GreyChecked, m_treeView.GetChecked(m_bNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c1Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c2Node));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that parent get greyed out if children are not all in same state
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void ChangeParent_CheckAllChildren()
		{
			// check second child -> check all parents
			m_treeView.SetChecked(m_c2Node, TriStateTreeView.CheckState.Checked);
			m_treeView.SetChecked(m_c1Node, TriStateTreeView.CheckState.Checked);

			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_aNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_bNode));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c1Node));
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c2Node));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that the BeforeCheck event is raised
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void BeforeCheckCalled()
		{
			m_treeView.BeforeCheck += new TreeViewCancelEventHandler(OnBeforeCheck);

			m_treeView.SetChecked(m_c1Node, TriStateTreeView.CheckState.Checked);

			Assert.IsTrue(m_fBeforeCheck);
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c1Node));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that the BeforeCheck event is raised if first node is changed
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void BeforeCheckCalled_FirstNode()
		{
			m_treeView.BeforeCheck += new TreeViewCancelEventHandler(OnBeforeCheck);

			m_treeView.CallChangeNodeState(m_aNode);

			Assert.IsTrue(m_fBeforeCheck);
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_aNode));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Tests that the AfterCheck event is raised
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void AfterCheckCalled()
		{
			m_treeView.AfterCheck += new TreeViewEventHandler(OnAfterCheck);

			m_treeView.SetChecked(m_c1Node, TriStateTreeView.CheckState.Checked);

			Assert.IsTrue(m_fAfterCheck);
			Assert.AreEqual(TriStateTreeView.CheckState.Checked, m_treeView.GetChecked(m_c1Node));
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// When the cancel flag in BeforeCheck returns true we don't want to change the
		/// state of the node.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[Test]
		public void StateNotChangedIfBeforeCheckCancels()
		{
			m_treeView.BeforeCheck += new TreeViewCancelEventHandler(OnBeforeCheck);
			m_treeView.AfterCheck += new TreeViewEventHandler(OnAfterCheck);
			m_fCancelInBeforeCheck = true;

			m_treeView.SetChecked(m_c1Node, TriStateTreeView.CheckState.Checked);

			Assert.IsTrue(m_fBeforeCheck);
			Assert.IsFalse(m_fAfterCheck);
			Assert.AreEqual(TriStateTreeView.CheckState.Unchecked, m_treeView.GetChecked(m_c1Node));
		}

		#region Helper methods
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// 
		/// </summary>
		/// ------------------------------------------------------------------------------------
		private void OnBeforeCheck(object sender, TreeViewCancelEventArgs e)
		{
			e.Cancel = m_fCancelInBeforeCheck;
			m_fBeforeCheck = true;
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// 
		/// </summary>
		/// ------------------------------------------------------------------------------------
		private void OnAfterCheck(object sender, TreeViewEventArgs e)
		{
			m_fAfterCheck = true;
		}
		#endregion
	}
}
