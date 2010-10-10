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
// File: TriStateTreeView.cs
// Responsibility: Eberhard Beilharz/Tim Steenwyk
// 
// <remarks>
// </remarks>
// ---------------------------------------------------------------------------------------------
using System;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Skybound.VisualStyles;

namespace SIL.FieldWorks.Common.Controls
{
    /// ----------------------------------------------------------------------------------------
    /// <summary>
    /// A tree view with tri-state check boxes
    /// </summary>
    /// <remarks>
    /// REVIEW: If we want to have icons in addition to the check boxes, we probably have to 
    /// set the icons for the check boxes in a different way. The windows tree view control
    /// can have a separate image list for states.
    /// </remarks>
    /// ----------------------------------------------------------------------------------------
    public class TriStateTreeView : TreeView
    {
        private System.Windows.Forms.ImageList m_TriStateImages;
        private System.ComponentModel.IContainer components;
        /// <summary>
        /// The check state
        /// </summary>
        /// <remarks>The states corresponds to image index</remarks>
        public enum CheckState
        {
            /// <summary>greyed out</summary>
            GreyChecked = 0,
            /// <summary>Unchecked</summary>
            Unchecked = 1,
            /// <summary>Checked</summary>
            Checked = 2,
        }

        #region Redefined Win-API structs and methods
        /// <summary></summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct TV_HITTESTINFO
        {
            /// <summary>Client coordinates of the point to test.</summary>
            public Point pt;
            /// <summary>Variable that receives information about the results of a hit test.</summary>
            public TVHit flags;
            /// <summary>Handle to the item that occupies the point.</summary>
            public IntPtr hItem;
        }

        /// <summary>Hit tests for tree view</summary>
        [Flags]
        public enum TVHit
        {
            /// <summary>In the client area, but below the last item.</summary>
            NoWhere = 0x0001,
            /// <summary>On the bitmap associated with an item.</summary>
            OnItemIcon = 0x0002,
            /// <summary>On the label (string) associated with an item.</summary>
            OnItemLabel = 0x0004,
            /// <summary>In the indentation associated with an item.</summary>
            OnItemIndent = 0x0008,
            /// <summary>On the button associated with an item.</summary>
            OnItemButton = 0x0010,
            /// <summary>In the area to the right of an item. </summary>
            OnItemRight = 0x0020,
            /// <summary>On the state icon for a tree-view item that is in a user-defined state.</summary>
            OnItemStateIcon = 0x0040,
            /// <summary>On the bitmap or label associated with an item. </summary>
            OnItem = (OnItemIcon | OnItemLabel | OnItemStateIcon),
            /// <summary>Above the client area. </summary>
            Above = 0x0100,
            /// <summary>Below the client area.</summary>
            Below = 0x0200,
            /// <summary>To the right of the client area.</summary>
            ToRight = 0x0400,
            /// <summary>To the left of the client area.</summary>
            ToLeft = 0x0800
        }

        /// <summary></summary>
        public enum TreeViewMessages
        {
            /// <summary></summary>
            TV_FIRST = 0x1100,      // TreeView messages
            /// <summary></summary>
            TVM_HITTEST = (TV_FIRST + 17),
        }

        /// <summary></summary>
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern int SendMessage(IntPtr hWnd, TreeViewMessages msg, int wParam, ref TV_HITTESTINFO lParam);
        #endregion

        #region Constructor and destructor
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Initializes a new instance of the <see cref="TriStateTreeView"/> class.
        /// </summary>
        /// ------------------------------------------------------------------------------------
        public TriStateTreeView()
        {
            // This call is required by the Windows.Forms Form Designer.
            InitializeComponent();

            if (ThemeInformation.VisualStylesEnabled)
            {
                Bitmap bmp = new Bitmap(m_TriStateImages.ImageSize.Width, m_TriStateImages.ImageSize.Height);
                Rectangle rc = new Rectangle(0, 0, bmp.Width, bmp.Height);
                Graphics graphics = Graphics.FromImage(bmp);

                ThemePaint.Draw(graphics, this, ThemeClasses.Button, ThemeParts.ButtonCheckBox,
                    ThemeStates.CheckBoxCheckedDisabled, rc, rc);
                m_TriStateImages.Images[0] = bmp;

                ThemePaint.Draw(graphics, this, ThemeClasses.Button, ThemeParts.ButtonCheckBox,
                    ThemeStates.CheckBoxUncheckedNormal, rc, rc);
                m_TriStateImages.Images[1] = bmp;

                ThemePaint.Draw(graphics, this, ThemeClasses.Button, ThemeParts.ButtonCheckBox,
                    ThemeStates.CheckBoxCheckedNormal, rc, rc);
                m_TriStateImages.Images[2] = bmp;
            }

            ImageList = m_TriStateImages;
            ImageIndex = (int)CheckState.Unchecked;
            SelectedImageIndex = (int)CheckState.Unchecked;
        }

        /// -----------------------------------------------------------------------------------
        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing"><c>true</c> to release both managed and unmanaged 
        /// resources; <c>false</c> to release only unmanaged resources. 
        /// </param>
        /// -----------------------------------------------------------------------------------
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
            }
            base.Dispose(disposing);
        }
        #endregion

        #region Component Designer generated code
        /// -----------------------------------------------------------------------------------
        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        /// -----------------------------------------------------------------------------------
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(TriStateTreeView));
            this.m_TriStateImages = new System.Windows.Forms.ImageList(this.components);
            // 
            // m_TriStateImages
            // 
            this.m_TriStateImages.ImageSize = new System.Drawing.Size(16, 16);
            this.m_TriStateImages.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("m_TriStateImages.ImageStream")));
            this.m_TriStateImages.TransparentColor = System.Drawing.Color.Magenta;

        }
        #endregion

        #region Hide no longer appropriate properties from Designer
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// 
        /// </summary>
        /// ------------------------------------------------------------------------------------
        [Browsable(false)]
        public new bool CheckBoxes
        {
            get { return base.CheckBoxes; }
            set { base.CheckBoxes = value; }
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// 
        /// </summary>
        /// ------------------------------------------------------------------------------------
        [Browsable(false)]
        public new int ImageIndex
        {
            get { return base.ImageIndex; }
            set { base.ImageIndex = value; }
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// 
        /// </summary>
        /// ------------------------------------------------------------------------------------
        [Browsable(false)]
        public new ImageList ImageList
        {
            get { return base.ImageList; }
            set { base.ImageList = value; }
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// 
        /// </summary>
        /// ------------------------------------------------------------------------------------
        [Browsable(false)]
        public new int SelectedImageIndex
        {
            get { return base.SelectedImageIndex; }
            set { base.SelectedImageIndex = value; }
        }
        #endregion

        #region Overrides
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Called when the user clicks on an item
        /// </summary>
        /// <param name="e"></param>
        /// ------------------------------------------------------------------------------------
        protected override void OnClick(EventArgs e)
        {
            base.OnClick(e);

            TV_HITTESTINFO hitTestInfo = new TV_HITTESTINFO();
            hitTestInfo.pt = PointToClient(Control.MousePosition);

            SendMessage(Handle, TreeViewMessages.TVM_HITTEST,
                0, ref hitTestInfo);
            if ((hitTestInfo.flags & TVHit.OnItemIcon) == TVHit.OnItemIcon)
            {
                TreeNode node = GetNodeAt(hitTestInfo.pt);
                if (node != null)
                    ChangeNodeState(node);
            }
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Toggle item if user presses space bar
        /// </summary>
        /// <param name="e"></param>
        /// ------------------------------------------------------------------------------------
        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);

            if (e.KeyCode == Keys.Space)
                ChangeNodeState(SelectedNode);
        }
        #endregion

        #region Private methods
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Checks or unchecks all children
        /// </summary>
        /// <param name="node"></param>
        /// <param name="state"></param>
        /// ------------------------------------------------------------------------------------
        private void CheckNode(TreeNode node, CheckState state)
        {
            InternalSetChecked(node, state);

            foreach (TreeNode child in node.Nodes)
                CheckNode(child, state);
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Called after a node changed its state. Has to go through all direct children and
        /// set state based on children's state.
        /// </summary>
        /// <param name="node">Parent node</param>
        /// ------------------------------------------------------------------------------------
        private void ChangeParent(TreeNode node)
        {
            if (node == null)
                return;

            CheckState state = GetChecked(node.FirstNode);
            foreach (TreeNode child in node.Nodes)
                state &= GetChecked(child);

            if (InternalSetChecked(node, state))
                ChangeParent(node.Parent);
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Handles changing the state of a node
        /// </summary>
        /// <param name="node"></param>
        /// ------------------------------------------------------------------------------------
        protected void ChangeNodeState(TreeNode node)
        {
            BeginUpdate();
            CheckState newState;
            if (node.ImageIndex == (int)CheckState.Unchecked || node.ImageIndex < 0)
                newState = CheckState.Checked;
            else
                newState = CheckState.Unchecked;
            CheckNode(node, newState);
            ChangeParent(node.Parent);
            EndUpdate();
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Sets the checked state of a node, but doesn't deal with children or parents
        /// </summary>
        /// <param name="node">Node</param>
        /// <param name="state">The new checked state</param>
        /// <returns><c>true</c> if checked state was set to the requested state, otherwise
        /// <c>false</c>.</returns>
        /// ------------------------------------------------------------------------------------
        private bool InternalSetChecked(TreeNode node, CheckState state)
        {
            TreeViewCancelEventArgs args =
                new TreeViewCancelEventArgs(node, false, TreeViewAction.Unknown);
            OnBeforeCheck(args);
            if (args.Cancel)
                return false;

            node.ImageIndex = (int)state;
            node.SelectedImageIndex = (int)state;

            OnAfterCheck(new TreeViewEventArgs(node, TreeViewAction.Unknown));
            return true;
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Build a list of all of the tag data for checked items in the tree.
        /// </summary>
        /// <param name="node"></param>
        /// <param name="list"></param>
        /// ------------------------------------------------------------------------------------
        private void BuildTagDataList(TreeNode node, ArrayList list)
        {
            if (GetChecked(node) == CheckState.Checked && node.Tag != null)
                list.Add(node.Tag);

            foreach (TreeNode child in node.Nodes)
                BuildTagDataList(child, list);
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Look through the tree nodes to find the node that has given tag data and check it.
        /// </summary>
        /// <param name="node"></param>
        /// <param name="tag"></param>
        /// <param name="state"></param>
        /// ------------------------------------------------------------------------------------
        private void FindAndCheckNode(TreeNode node, object tag, CheckState state)
        {
            if (node.Tag != null && node.Tag.Equals(tag))
            {
                SetChecked(node, state);
                return;
            }

            foreach (TreeNode child in node.Nodes)
                FindAndCheckNode(child, tag, state);
        }
        #endregion

        #region Public methods
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Gets the checked state of a node
        /// </summary>
        /// <param name="node">Node</param>
        /// <returns>The checked state</returns>
        /// ------------------------------------------------------------------------------------
        public CheckState GetChecked(TreeNode node)
        {
            if (node.ImageIndex < 0)
                return CheckState.Unchecked;
            else
                return (CheckState)node.ImageIndex;
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Sets the checked state of a node
        /// </summary>
        /// <param name="node">Node</param>
        /// <param name="state">The new checked state</param>
        /// ------------------------------------------------------------------------------------
        public void SetChecked(TreeNode node, CheckState state)
        {
            if (!InternalSetChecked(node, state))
                return;
            CheckNode(node, state);
            ChangeParent(node.Parent);
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Find a node in the tree that matches the given tag data and set its checked state
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="state"></param>
        /// ------------------------------------------------------------------------------------
        public void CheckNodeByTag(object tag, CheckState state)
        {
            if (tag == null)
                return;
            foreach (TreeNode node in Nodes)
                FindAndCheckNode(node, tag, state);
        }

        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Return a list of the tag data for all of the checked items in the tree
        /// </summary>
        /// <returns></returns>
        /// ------------------------------------------------------------------------------------
        public ArrayList GetCheckedTagData()
        {
            ArrayList list = new ArrayList();

            foreach (TreeNode node in Nodes)
                BuildTagDataList(node, list);
            return list;
        }
        #endregion
    }
}
