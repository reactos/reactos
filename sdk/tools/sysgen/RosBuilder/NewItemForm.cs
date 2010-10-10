using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace TriStateTreeViewDemo
{
    public partial class NewItemForm : Form
    {
        public NewItemForm()
        {
            InitializeComponent();
        }

        public ListView ListView
        {
            get { return lvAddItem; }
        }

        private void lvAddItem_DoubleClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }
    }
}