using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
	/// ----------------------------------------------------------------------------------------
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	/// ----------------------------------------------------------------------------------------
	public class Form1 : System.Windows.Forms.Form
	{
        private FileSystemTriStateTreeView triStateTreeView1;
        private TextBox textBox1;
        private Button btnProcess;
        private Button btnGenerateRBuildFile;
        private PropertyGrid propertyGrid1;
        private IContainer components;

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
            this.components = new System.ComponentModel.Container();
            System.Windows.Forms.TreeNode treeNode1 = new System.Windows.Forms.TreeNode("Node2");
            System.Windows.Forms.TreeNode treeNode2 = new System.Windows.Forms.TreeNode("Node11");
            System.Windows.Forms.TreeNode treeNode3 = new System.Windows.Forms.TreeNode("Node10", new System.Windows.Forms.TreeNode[] {
            treeNode2});
            System.Windows.Forms.TreeNode treeNode4 = new System.Windows.Forms.TreeNode("Node1", new System.Windows.Forms.TreeNode[] {
            treeNode1,
            treeNode3});
            System.Windows.Forms.TreeNode treeNode5 = new System.Windows.Forms.TreeNode("Node4");
            System.Windows.Forms.TreeNode treeNode6 = new System.Windows.Forms.TreeNode("Node7");
            System.Windows.Forms.TreeNode treeNode7 = new System.Windows.Forms.TreeNode("Node8");
            System.Windows.Forms.TreeNode treeNode8 = new System.Windows.Forms.TreeNode("Node9");
            System.Windows.Forms.TreeNode treeNode9 = new System.Windows.Forms.TreeNode("Node3", new System.Windows.Forms.TreeNode[] {
            treeNode5,
            treeNode6,
            treeNode7,
            treeNode8});
            System.Windows.Forms.TreeNode treeNode10 = new System.Windows.Forms.TreeNode("Node0", new System.Windows.Forms.TreeNode[] {
            treeNode4,
            treeNode9});
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.btnProcess = new System.Windows.Forms.Button();
            this.btnGenerateRBuildFile = new System.Windows.Forms.Button();
            this.propertyGrid1 = new System.Windows.Forms.PropertyGrid();
            this.triStateTreeView1 = new TriStateTreeViewDemo.FileSystemTriStateTreeView();
            this.SuspendLayout();
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(340, 12);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(432, 20);
            this.textBox1.TabIndex = 1;
            this.textBox1.Text = "C:\\Ros\\Trunk\\reactos\\salamander\\lib\\sdl";
            // 
            // btnProcess
            // 
            this.btnProcess.Location = new System.Drawing.Point(687, 38);
            this.btnProcess.Name = "btnProcess";
            this.btnProcess.Size = new System.Drawing.Size(85, 28);
            this.btnProcess.TabIndex = 2;
            this.btnProcess.Text = "button1";
            this.btnProcess.UseVisualStyleBackColor = true;
            this.btnProcess.Click += new System.EventHandler(this.btnProcess_Click);
            // 
            // btnGenerateRBuildFile
            // 
            this.btnGenerateRBuildFile.Location = new System.Drawing.Point(701, 517);
            this.btnGenerateRBuildFile.Name = "btnGenerateRBuildFile";
            this.btnGenerateRBuildFile.Size = new System.Drawing.Size(75, 23);
            this.btnGenerateRBuildFile.TabIndex = 3;
            this.btnGenerateRBuildFile.Text = "button1";
            this.btnGenerateRBuildFile.UseVisualStyleBackColor = true;
            this.btnGenerateRBuildFile.Click += new System.EventHandler(this.btnGenerateRBuildFile_Click);
            // 
            // propertyGrid1
            // 
            this.propertyGrid1.Location = new System.Drawing.Point(340, 75);
            this.propertyGrid1.Name = "propertyGrid1";
            this.propertyGrid1.Size = new System.Drawing.Size(431, 426);
            this.propertyGrid1.TabIndex = 4;
            // 
            // triStateTreeView1
            // 
            this.triStateTreeView1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.triStateTreeView1.ImageIndex = 1;
            this.triStateTreeView1.Location = new System.Drawing.Point(12, 12);
            this.triStateTreeView1.Name = "triStateTreeView1";
            treeNode1.Name = "";
            treeNode1.Text = "Node2";
            treeNode2.Name = "";
            treeNode2.Text = "Node11";
            treeNode3.Name = "";
            treeNode3.Text = "Node10";
            treeNode4.Name = "";
            treeNode4.Text = "Node1";
            treeNode5.Name = "";
            treeNode5.Text = "Node4";
            treeNode6.Name = "";
            treeNode6.Text = "Node7";
            treeNode7.Name = "";
            treeNode7.Text = "Node8";
            treeNode8.Name = "";
            treeNode8.Text = "Node9";
            treeNode9.Name = "";
            treeNode9.Text = "Node3";
            treeNode10.Name = "";
            treeNode10.Text = "Node0";
            this.triStateTreeView1.Nodes.AddRange(new System.Windows.Forms.TreeNode[] {
            treeNode10});
            this.triStateTreeView1.SelectedImageIndex = 1;
            this.triStateTreeView1.Size = new System.Drawing.Size(322, 528);
            this.triStateTreeView1.TabIndex = 0;
            // 
            // Form1
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(788, 556);
            this.Controls.Add(this.propertyGrid1);
            this.Controls.Add(this.btnGenerateRBuildFile);
            this.Controls.Add(this.btnProcess);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.triStateTreeView1);
            this.Name = "Form1";
            this.Text = "RBuild Port Maker";
            this.ResumeLayout(false);
            this.PerformLayout();

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
 RBuildModule module = new RBuildModule();
        private void btnProcess_Click(object sender, EventArgs e)
        {
           

            module.GenerateFromPath(textBox1.Text);

            this.triStateTreeView1.Load(module);
            this.propertyGrid1.SelectedObject = module;

            
        }

        private void btnGenerateRBuildFile_Click(object sender, EventArgs e)
        {
module.SaveAs (@"c:\module.rbuild");
        }
	}
}
