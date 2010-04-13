using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

namespace TriStateTreeView
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	public class Form1 : System.Windows.Forms.Form
	{
		private SIL.FieldWorks.Common.Controls.TriStateTreeView triStateTreeView1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Initializes a new instance of the <see cref="Form1"/> class.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//

			triStateTreeView1.ExpandAll();
		}

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing"><c>true</c> to release both managed and unmanaged 
		/// resources; <c>false</c> to release only unmanaged resources. 
		/// </param>
		/// ------------------------------------------------------------------------------------
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		private void InitializeComponent()
		{
			this.triStateTreeView1 = new SIL.FieldWorks.Common.Controls.TriStateTreeView();
			this.SuspendLayout();
			// 
			// triStateTreeView1
			// 
			this.triStateTreeView1.ImageIndex = 1;
			this.triStateTreeView1.Location = new System.Drawing.Point(16, 16);
			this.triStateTreeView1.Name = "triStateTreeView1";
			this.triStateTreeView1.Nodes.AddRange(new System.Windows.Forms.TreeNode[] {
																						  new System.Windows.Forms.TreeNode("Node0", new System.Windows.Forms.TreeNode[] {
																																											 new System.Windows.Forms.TreeNode("Node1", new System.Windows.Forms.TreeNode[] {
																																																																new System.Windows.Forms.TreeNode("Node2"),
																																																																new System.Windows.Forms.TreeNode("Node10", new System.Windows.Forms.TreeNode[] {
																																																																																					new System.Windows.Forms.TreeNode("Node11")})}),
																																											 new System.Windows.Forms.TreeNode("Node3", new System.Windows.Forms.TreeNode[] {
																																																																new System.Windows.Forms.TreeNode("Node4"),
																																																																new System.Windows.Forms.TreeNode("Node7"),
																																																																new System.Windows.Forms.TreeNode("Node8"),
																																																																new System.Windows.Forms.TreeNode("Node9")})})});
			this.triStateTreeView1.SelectedImageIndex = 1;
			this.triStateTreeView1.Size = new System.Drawing.Size(256, 232);
			this.triStateTreeView1.TabIndex = 0;
			// 
			// Form1
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(292, 266);
			this.Controls.Add(this.triStateTreeView1);
			this.Name = "Form1";
			this.Text = "Form1";
			this.ResumeLayout(false);

		}
		#endregion

		/// ------------------------------------------------------------------------------------
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		/// ------------------------------------------------------------------------------------
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}
	}
}
