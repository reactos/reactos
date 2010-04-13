using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;

namespace TriStateTreeViewDemo
{
    public class ModuleFiltersListViewItem : ListViewItem
    {
        private ModuleFilter m_ModuleFilter = null;

        public ModuleFiltersListViewItem(ModuleFilter filter)
        {
            m_ModuleFilter = filter;

            Text = filter.Name;
            SubItems.Add(filter.Modules.Count.ToString());
        }

        public ModuleFilter Filter
        {
            get { return m_ModuleFilter; }
        }
    }

    public class ModuleFiltersListView : ListView
    {
        private ISysGenDesigner m_SysGenDesigner = null;

        public ModuleFiltersListView()
        {
            View = View.Details;

            FullRowSelect = true;
            CheckBoxes = true;

            Columns.Add("Name", 200);
            Columns.Add("Modules", 100);
        }

        public void SetCatalog(ISysGenDesigner sysGenDesigner)
        {
            //Set the software catalog
            m_SysGenDesigner = sysGenDesigner;
            //m_SysGenDesigner.PlatformController.PlatformModulesUpdated += new EventHandler(PlatformController_PlatformModulesUpdated);

            foreach (ModuleFilter filter in sysGenDesigner.ModuleFilterController.ModuleFilters)
            {
                Items.Add(new ModuleFiltersListViewItem(filter));
            }
        }

        //protected override void OnItemCheck(ItemCheckEventArgs ice)
        //{
        //    base.OnItemCheck(ice);
        //}

        //protected override void OnItemChecked(ItemCheckedEventArgs e)
        //{
        //    base.OnItemChecked(e);
        //}

        private void PlatformController_PlatformModulesUpdated(object sender, EventArgs e)
        {
            BeginUpdate();

            foreach (ModuleFiltersListViewItem filterItem in Items)
            {
                foreach (RBuildModule module in m_SysGenDesigner.ProjectController.Project.Platform.Modules)
                {
                    if (!filterItem.Filter.Modules.Contains(module))
                    {
                        filterItem.Checked = false;
                        break;
                    }
                }

                filterItem.Checked = true;
            }

            EndUpdate();
        }
    }
}