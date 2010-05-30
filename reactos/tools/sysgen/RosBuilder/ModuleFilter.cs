using System;
using System.Collections.Generic;
using System.Text;

using SysGen.BuildEngine;
using SysGen.BuildEngine.Framework;
using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
    public class ModuleFilterController
    {
        private List<ModuleFilter> m_ModuleFilters = new List<ModuleFilter>();
        private ISysGenDesigner m_SysGenDesigner = null;

        public ModuleFilterController(ISysGenDesigner engine)
        {
            //The engine...
            m_SysGenDesigner = engine;

            RegisterDinamicFilters();
            RegisterModuleGroups();
            InitializeFilters();
        }

        private void RegisterDinamicFilters()
        {
            m_ModuleFilters.Add(new AllModuleFilter());
            m_ModuleFilters.Add(new AllWin32CUIModuleFilter());
            m_ModuleFilters.Add(new AllWin32GUIModuleFilter());
            m_ModuleFilters.Add(new AllScreenSaversModuleFilter());
            m_ModuleFilters.Add(new AllKeyboardLayoutsModuleFilter());
            m_ModuleFilters.Add(new AllDriversModuleFilter());
            m_ModuleFilters.Add(new AllDllsModuleFilter());
        }

        private void RegisterModuleGroups()
        {
            foreach (RBuildModule module in m_SysGenDesigner.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.ModuleGroup)
                {
                    m_ModuleFilters.Add(new PackageModuleFilter(module));
                }
            }
        }

        private void InitializeFilters()
        {
            foreach (ModuleFilter filter in m_ModuleFilters)
            {
                filter.Designer = m_SysGenDesigner;
                filter.Initialize();
            }
        }

        public List<ModuleFilter> ModuleFilters
        {
            get { return m_ModuleFilters; }
        }

        public void Apply(ModuleFilter filter)
        {
            m_SysGenDesigner.ProjectController.Add(filter.Modules);
        }

        public void Remove(ModuleFilter filter)
        {
            //m_SysGenDesigner.PlatformController.Remove(filter.Modules);
        }
    }

    public abstract class ModuleFilter
    {
        protected ISysGenDesigner m_Project = null;
        protected RBuildModuleCollection m_Modules = new RBuildModuleCollection();

        public ModuleFilter()
        {
        }

        public virtual void Initialize()
        {
            ExecuteRule();
        }

        public abstract void ExecuteRule();
       
        public RBuildModuleCollection Modules
        {
            get { return m_Modules; }
        }

        public ISysGenDesigner Designer
        {
            get { return m_Project; }
            set { m_Project = value; }
        }

        public abstract string Name { get; }

        public override string ToString()
        {
            return string.Format("{0} - ({1} modules)",
                Name,
                Modules.Count);
        }
    }

    public class PackageModuleFilter : ModuleFilter
    {
        RBuildModule m_Module = null;

        public PackageModuleFilter(RBuildModule module)
        {
            //Save the underlaying module...
            m_Module = module;
        }

        public override string Name
        {
            get { return (m_Module.Description != null) ? m_Module.Description : m_Module.Name; }
        }

        public override void ExecuteRule()
        {
            Modules.Add(m_Module.Needs);
        }
    }

    public class AllModuleFilter : ModuleFilter
    {
        public AllModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (Modules.Contains(module) == false)
                    Modules.Add(module);
            }
        }
    }

    public class AllWin32CUIModuleFilter : ModuleFilter
    {
        public AllWin32CUIModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All Console Applications"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.Win32CUI)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }

    public class AllWin32GUIModuleFilter : ModuleFilter
    {
        public AllWin32GUIModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All Graphical Applications"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.Win32GUI)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }

    public class AllScreenSaversModuleFilter : ModuleFilter
    {
        public AllScreenSaversModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All ScreenSavers"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.Win32SCR)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }

    public class AllKeyboardLayoutsModuleFilter : ModuleFilter
    {
        public AllKeyboardLayoutsModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All Keyboard Layouts"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.KeyboardLayout)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }

    public class AllDriversModuleFilter : ModuleFilter
    {
        public AllDriversModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All Drivers"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.KernelModeDriver)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }

    public class AllDllsModuleFilter : ModuleFilter
    {
        public AllDllsModuleFilter()
        {
        }

        public override string Name
        {
            get { return "All Dlls"; }
        }

        public override void ExecuteRule()
        {
            foreach (RBuildModule module in Designer.ProjectController.AvailableModules)
            {
                if (module.Type == ModuleType.Win32DLL)
                {
                    if (Modules.Contains(module) == false)
                        Modules.Add(module);
                }
            }
        }
    }
}
