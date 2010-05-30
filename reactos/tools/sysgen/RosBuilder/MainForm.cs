using System;
using System.Xml;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using TriStateTreeViewDemo;

using SysGen.Framework.Catalog;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Log;
using SysGen.BuildEngine.Framework;
using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
    public interface ISysGenDesigner
    {
        ModuleFilterController ModuleFilterController { get; }
        ProjectController ProjectController { get; }
        object InspectedObject { set; }
    }

    public partial class MainForm : Form, ISysGenDesigner
    {
        ProjectController m_ProjectController = null;
        ModuleFilterController m_FilterController = null;

        public MainForm()
        {
            InitializeComponent();

            m_ProjectController = new ProjectController(this);
            m_FilterController = new ModuleFilterController(this);

            tvPlatform.SetCatalog(this);
            tvProject.SetCatalog(this);
            tvProject.DoubleClick += new EventHandler(tvProject_DoubleClick);
            //lvModuleFilters.SetCatalog(this);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            //m_project = new RBuildProject();

            m_ProjectController = new ProjectController(this);
            m_FilterController = new ModuleFilterController(this);

            tvPlatform.SetCatalog(this);
            tvProject.SetCatalog(this);
            //lvModuleFilters.SetCatalog(this);

            //PlatformCatalogReader m_Reader = new PlatformCatalogReader(@"C:\Ros\trunk\reactos\rbuilddb.xml");

            /*
            BuildLog.Listeners.Clear();

            m_SysGenEngine.SetDefaults = false;
            m_SysGenEngine.RunBackends = false;
            m_SysGenEngine.CleanCustomConfigs();
            m_SysGenEngine.ReadBuildFiles();

            m_PlatformController = new PlatformController(this);
            m_FilterController = new ModuleFilterController(this);

            catalogTriStateTreeView1.SetCatalog(this);
            lvModuleFilters.SetCatalog(this);
             */

        }

        void tvProject_DoubleClick(object sender, EventArgs e)
        {
        }

        private void button2_Click(object sender, EventArgs e)
        {          
            using (XmlTextWriter writer = new XmlTextWriter(@"c:\pkg.rbuild", Encoding.ASCII))
            {
                writer.Indentation = 4;
                writer.Formatting = Formatting.Indented;

                // Starts a new document
                writer.WriteStartDocument();
                writer.WriteStartElement("module");
                writer.WriteAttributeString("name", "");
                writer.WriteAttributeString("type", "modulegroup");

                foreach (RBuildModule module in m_ProjectController.Project.Platform.Modules)
                {
                    writer.WriteElementString("requires", module.Name);
                }

                writer.WriteEndElement();
                writer.WriteEndDocument();
            }

        }

        private void button3_Click(object sender, EventArgs e)
        {
        }

        public ModuleFilterController ModuleFilterController
        {
            get { return m_FilterController; }
        }

        public ProjectController ProjectController
        {
            get { return m_ProjectController; }
        }

        private void lvModuleFilters_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        //private void lvModuleFilters_ItemCheck(object sender, ItemCheckEventArgs e)
        //{
        //    ModuleFiltersListViewItem filerListViewItem = lvModuleFilters.FocusedItem as ModuleFiltersListViewItem;

        //    if (filerListViewItem != null)
        //    {
        //        if (e.NewValue == CheckState.Checked)
        //        {
        //            m_FilterController.Apply(filerListViewItem.Filter);
        //        }
        //    }
        //}

        private void MainForm_Load(object sender, EventArgs e)
        {
            cmbArchitecture.Items.Add("x86 - (i486)");
            cmbArchitecture.Items.Add("x86 - (i586)");
            cmbArchitecture.Items.Add("x86 - (Pentium)");
            cmbArchitecture.Items.Add("x86 - (Pentium2)");
            cmbArchitecture.Items.Add("x86 - (Pentium3)");
            cmbArchitecture.Items.Add("x86 - (Pentium4)");
            cmbArchitecture.Items.Add("x86 - (athlon-xp)");
            cmbArchitecture.Items.Add("x86 - (athlon-mp)");
            cmbArchitecture.Items.Add("x86 - (k6-2)");
            cmbArchitecture.Items.Add("x86 - Xbox");
            cmbArchitecture.Items.Add("Power PC");
            cmbArchitecture.Items.Add("ARM");
            cmbArchitecture.SelectedIndex = 2;

            cmbDebug.Items.Add("Debug");
            cmbDebug.Items.Add("Release");
            cmbDebug.SelectedIndex = 0;

            cmbOptimization.Items.Add("Level 0");
            cmbOptimization.Items.Add("Level 1");
            cmbOptimization.Items.Add("Level 2");
            cmbOptimization.Items.Add("Level 3");
            cmbOptimization.Items.Add("Level 4");
            cmbOptimization.Items.Add("Level 5");
            cmbOptimization.SelectedIndex = 1;
        }

        public object InspectedObject
        {
            set
            {
                if (value is RBuildPlatform)
                {
                    pgProperties.SelectedObject = new PlatformInspector(m_ProjectController.Project.Platform);
                }
                else
                    pgProperties.SelectedObject = value;
            }
        }

        private void openToolStripButton_Click(object sender, EventArgs e)
        {
            m_ProjectController.Open();
        }

        private void saveToolStripButton_Click(object sender, EventArgs e)
        {
            m_ProjectController.Save();
        }

        private void saveConfigToolStripButton_Click(object sender, EventArgs e)
        {
            //if (MessageBox.Show("Your platform does not have a default shell selected ¿are you sure you want to continue?", 
            //    "Question", 
            //    MessageBoxButtons.YesNo, 
            //    MessageBoxIcon.Question) == DialogResult.OK)
            //{
                // Creates an XML file is not exist
                using (XmlTextWriter writer = new XmlTextWriter(m_ProjectController.SysGenProject.Source + @"\config.rbuild", Encoding.ASCII))
                {
                    writer.Indentation = 4;
                    writer.Formatting = Formatting.Indented;

                    // Starts a new document
                    writer.WriteStartDocument();
                    writer.WriteStartElement("group");

                    writer.WriteComment("Platform information");
                    writer.WriteElementString("platformname", m_ProjectController.Project.Platform.Name);
                    writer.WriteElementString("platformdescription", m_ProjectController.Project.Platform.Description);

                    writer.WriteComment("Default applications");

                    if (m_ProjectController.Project.Platform.Shell != null)
                    {
                        writer.WriteElementString("platformshell", m_ProjectController.Project.Platform.Shell.Name);
                    }

                    if (m_ProjectController.Project.Platform.Screensaver != null)
                    {
                        writer.WriteElementString("platformscreensaver", m_ProjectController.Project.Platform.Screensaver.Name);
                    }

                    if (m_ProjectController.Project.Platform.Wallpaper != null)
                    {
                        writer.WriteElementString("platformwallpaper", m_ProjectController.Project.Platform.Wallpaper.ID);
                    }

                    writer.WriteComment("Modules incuded in the platform");
                    foreach (RBuildModule module in m_ProjectController.Project.Platform.Modules)
                    {
                        writer.WriteElementString("platformmodule", module.Name);
                    }

                    writer.WriteComment("Languages incuded in the platform");
                    foreach (RBuildLanguage language in m_ProjectController.Project.Platform.Languages)
                    {
                        writer.WriteElementString("platformlanguage", language.Name);
                    }

                    writer.WriteComment("Debug Channels incuded in the platform");
                    foreach (RBuildDebugChannel debugChannel in m_ProjectController.Project.Platform.DebugChannels)
                    {
                        writer.WriteElementString("platformdebugchhanel", debugChannel.Name);
                    }

                    writer.WriteComment("Platform RBuild Properties");
                    writer.WriteComment("Properties");
                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "SARCH");
                    writer.WriteAttributeString("value", "");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "OARCH");
                    writer.WriteAttributeString("value", "pentium");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "OPTIMIZE");
                    writer.WriteAttributeString("value", ((int)m_ProjectController.SysGenProject.OptimizeLevel).ToString());
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "KDBG");
                    writer.WriteAttributeString("value", m_ProjectController.SysGenProject.KDebug ? "1" : "0");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "DBG");
                    writer.WriteAttributeString("value", m_ProjectController.SysGenProject.Debug ? "1" : "0");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "GDB");
                    writer.WriteAttributeString("value", m_ProjectController.SysGenProject.GDB ? "1" : "0");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "NSWPAT");
                    writer.WriteAttributeString("value", m_ProjectController.SysGenProject.NSWPAT ? "1" : "0");
                    writer.WriteEndElement();

                    writer.WriteStartElement("property");
                    writer.WriteAttributeString("name", "_WINKD_");
                    writer.WriteAttributeString("value", m_ProjectController.SysGenProject.WINKD ? "1" : "0");
                    writer.WriteEndElement();

                    writer.WriteEndElement();
                    writer.WriteEndDocument();
               // }
            }
        }

        private void newToolStripButton_Click(object sender, EventArgs e)
        {
            m_ProjectController.New();
        }

        private void addFiltersToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (NewItemForm newItem = new NewItemForm())
            {
                foreach (ModuleFilter filter in ModuleFilterController.ModuleFilters)
                {
                    newItem.ListView.Items.Add(new ModuleFiltersNewItemListViewItem(this, filter));
                }

                if (newItem.ShowDialog() == DialogResult.OK)
                {
                    foreach (ListViewItem lvItem in newItem.ListView.SelectedItems)
                    {
                        NewItemListViewItem item = lvItem as NewItemListViewItem;

                        if (item != null)
                            item.Apply();
                    }
                }
            }
        }

        private void addLanguagesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (NewItemForm newItem = new NewItemForm())
            {
                foreach (RBuildLanguage language in m_ProjectController.Project.Languages)
                {
                    newItem.ListView.Items.Add(new LanguageNewItemListViewItem(this, language));
                }

                if (newItem.ShowDialog() == DialogResult.OK)
                {
                    foreach (ListViewItem lvItem in newItem.ListView.SelectedItems)
                    {
                        NewItemListViewItem item = lvItem as NewItemListViewItem;

                        if (item != null)
                            item.Apply();
                    }
                }
            }
        }

        private void addDebToolStripMenuItem_Click(object sender, EventArgs e)
        {
            using (NewItemForm newItem = new NewItemForm())
            {
                foreach (RBuildDebugChannel channel in m_ProjectController.Project.DebugChannels)
                {
                    newItem.ListView.Items.Add(new DebugChannelNewItemListViewItem(this, channel));
                }

                if (newItem.ShowDialog() == DialogResult.OK)
                {
                    foreach (ListViewItem lvItem in newItem.ListView.SelectedItems)
                    {
                        NewItemListViewItem item = lvItem as NewItemListViewItem;

                        if (item != null)
                            item.Apply();
                    }
                }
            }
        }
    }
}