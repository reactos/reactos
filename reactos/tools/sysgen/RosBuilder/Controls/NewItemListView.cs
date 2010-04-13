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
    public abstract class NewItemListViewItem : ListViewItem
    {
        protected ISysGenDesigner m_SysGenDesigner = null;

        public NewItemListViewItem(ISysGenDesigner designer)
        {
            m_SysGenDesigner = designer;
        }

        public abstract string Description { get; }
        public abstract string DefaultFileName { get; }

        public virtual void Apply()
        {
        }
    }

    public class ModuleFiltersNewItemListViewItem : NewItemListViewItem
    {
        private ModuleFilter m_ModuleFilter = null;

        public ModuleFiltersNewItemListViewItem(ISysGenDesigner designer ,ModuleFilter filter): base(designer)
        {
            m_ModuleFilter = filter;

            Text = filter.Name;
        }

        public override string Description
        {
            get { return m_ModuleFilter.Name; }
        }

        public override string DefaultFileName
        {
            get { return null; }
        }

        public override void Apply()
        {
            m_SysGenDesigner.ModuleFilterController.Apply(m_ModuleFilter);
        }
    }

    public class LanguageNewItemListViewItem : NewItemListViewItem
    {
        private RBuildLanguage m_Language = null;

        public LanguageNewItemListViewItem(ISysGenDesigner designer, RBuildLanguage language)
            : base(designer)
        {
            m_Language = language;

            Text = language.Name;
        }

        public override string Description
        {
            get { return m_Language.Name; }
        }

        public override string DefaultFileName
        {
            get { return null; }
        }

        public override void Apply()
        {
            m_SysGenDesigner.ProjectController.AddLanguage(m_Language);
        }
    }

    public class DebugChannelNewItemListViewItem : NewItemListViewItem
    {
        private RBuildDebugChannel m_DebugChannel = null;

        public DebugChannelNewItemListViewItem(ISysGenDesigner designer, RBuildDebugChannel channel)
            : base(designer)
        {
            m_DebugChannel = channel;

            Text = channel.Name;
        }

        public override string Description
        {
            get { return m_DebugChannel.Name; }
        }

        public override string DefaultFileName
        {
            get { return null; }
        }

        public override void Apply()
        {
            m_SysGenDesigner.ProjectController.AddDebugChannel(m_DebugChannel);
        }
    }
}