namespace LwipMibViewer
{
	partial class FormMain
	{
		/// <summary>
		/// Erforderliche Designervariable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Verwendete Ressourcen bereinigen.
		/// </summary>
		/// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Vom Windows Form-Designer generierter Code

		/// <summary>
		/// Erforderliche Methode für die Designerunterstützung.
		/// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
		/// </summary>
		private void InitializeComponent()
		{
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormMain));
            this.treeMib = new System.Windows.Forms.TreeView();
            this.imagelistTreeNodeImages = new System.Windows.Forms.ImageList(this.components);
            this.splitContainerMain = new System.Windows.Forms.SplitContainer();
            this.listviewNodeDetails = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.toolStripMain = new System.Windows.Forms.ToolStrip();
            this.toolbuttonOpenMib = new System.Windows.Forms.ToolStripButton();
            this.dialogOpenMib = new System.Windows.Forms.OpenFileDialog();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainerMain)).BeginInit();
            this.splitContainerMain.Panel1.SuspendLayout();
            this.splitContainerMain.Panel2.SuspendLayout();
            this.splitContainerMain.SuspendLayout();
            this.toolStripMain.SuspendLayout();
            this.SuspendLayout();
            // 
            // treeMib
            // 
            this.treeMib.Dock = System.Windows.Forms.DockStyle.Fill;
            this.treeMib.ImageIndex = 0;
            this.treeMib.ImageList = this.imagelistTreeNodeImages;
            this.treeMib.Location = new System.Drawing.Point(0, 0);
            this.treeMib.Name = "treeMib";
            this.treeMib.SelectedImageIndex = 0;
            this.treeMib.Size = new System.Drawing.Size(1028, 418);
            this.treeMib.TabIndex = 0;
            this.treeMib.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeMib_AfterSelect);
            // 
            // imagelistTreeNodeImages
            // 
            this.imagelistTreeNodeImages.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imagelistTreeNodeImages.ImageStream")));
            this.imagelistTreeNodeImages.TransparentColor = System.Drawing.Color.Transparent;
            this.imagelistTreeNodeImages.Images.SetKeyName(0, "ntimgContainer");
            this.imagelistTreeNodeImages.Images.SetKeyName(1, "ntimgTable");
            this.imagelistTreeNodeImages.Images.SetKeyName(2, "ntimgRow");
            this.imagelistTreeNodeImages.Images.SetKeyName(3, "ntimgColumn");
            this.imagelistTreeNodeImages.Images.SetKeyName(4, "ntimgScalar");
            this.imagelistTreeNodeImages.Images.SetKeyName(5, "ntimgUnknown");
            // 
            // splitContainerMain
            // 
            this.splitContainerMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainerMain.Location = new System.Drawing.Point(0, 25);
            this.splitContainerMain.Name = "splitContainerMain";
            this.splitContainerMain.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainerMain.Panel1
            // 
            this.splitContainerMain.Panel1.Controls.Add(this.treeMib);
            // 
            // splitContainerMain.Panel2
            // 
            this.splitContainerMain.Panel2.Controls.Add(this.listviewNodeDetails);
            this.splitContainerMain.Size = new System.Drawing.Size(1028, 625);
            this.splitContainerMain.SplitterDistance = 418;
            this.splitContainerMain.TabIndex = 1;
            // 
            // listviewNodeDetails
            // 
            this.listviewNodeDetails.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2});
            this.listviewNodeDetails.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listviewNodeDetails.FullRowSelect = true;
            this.listviewNodeDetails.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.None;
            this.listviewNodeDetails.Location = new System.Drawing.Point(0, 0);
            this.listviewNodeDetails.Name = "listviewNodeDetails";
            this.listviewNodeDetails.Size = new System.Drawing.Size(1028, 203);
            this.listviewNodeDetails.TabIndex = 0;
            this.listviewNodeDetails.UseCompatibleStateImageBehavior = false;
            this.listviewNodeDetails.View = System.Windows.Forms.View.Details;
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "";
            this.columnHeader1.Width = 150;
            // 
            // columnHeader2
            // 
            this.columnHeader2.Text = "";
            this.columnHeader2.Width = 777;
            // 
            // toolStripMain
            // 
            this.toolStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolbuttonOpenMib});
            this.toolStripMain.Location = new System.Drawing.Point(0, 0);
            this.toolStripMain.Name = "toolStripMain";
            this.toolStripMain.Size = new System.Drawing.Size(1028, 25);
            this.toolStripMain.TabIndex = 2;
            // 
            // toolbuttonOpenMib
            // 
            this.toolbuttonOpenMib.Image = ((System.Drawing.Image)(resources.GetObject("toolbuttonOpenMib.Image")));
            this.toolbuttonOpenMib.Name = "toolbuttonOpenMib";
            this.toolbuttonOpenMib.Size = new System.Drawing.Size(65, 22);
            this.toolbuttonOpenMib.Text = "Open...";
            this.toolbuttonOpenMib.Click += new System.EventHandler(this.toolbuttonOpenMib_Click);
            // 
            // FormMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1028, 650);
            this.Controls.Add(this.splitContainerMain);
            this.Controls.Add(this.toolStripMain);
            this.Name = "FormMain";
            this.Text = "MIB Viewer";
            this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
            this.splitContainerMain.Panel1.ResumeLayout(false);
            this.splitContainerMain.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainerMain)).EndInit();
            this.splitContainerMain.ResumeLayout(false);
            this.toolStripMain.ResumeLayout(false);
            this.toolStripMain.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.TreeView treeMib;
        private System.Windows.Forms.SplitContainer splitContainerMain;
        private System.Windows.Forms.ListView listviewNodeDetails;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ImageList imagelistTreeNodeImages;
        private System.Windows.Forms.ToolStrip toolStripMain;
        private System.Windows.Forms.ToolStripButton toolbuttonOpenMib;
        private System.Windows.Forms.OpenFileDialog dialogOpenMib;
	}
}

