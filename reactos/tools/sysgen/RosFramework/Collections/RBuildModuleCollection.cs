using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public class ModulePreferenceComparer : IComparer<RBuildModule>
    {
        public int Compare(RBuildModule x, RBuildModule y)
        {
            if (x.Type == y.Type)
                return 0;

            if (x.Type == ModuleType.BuildTool)
                return -1;

            return 1;
        }
    }

    public class RBuildModuleCollection : List<RBuildModule>
    {
        public event EventHandler OnModuleAdded;

        public void Add(RBuildModuleCollection modules)
        {
            foreach (RBuildModule module in modules)
            {
                Add(module);
            }
        }

        public new void Add(RBuildModule module)
        {
            if (module == null)
                throw new Exception("Could not add a null instance");

            if (GetByName(module.Name) == null)
            {
                base.Add(module);
            }

            if (OnModuleAdded != null)
                OnModuleAdded(this, EventArgs.Empty);
        }

        public void Add(string moduleName)
        {
            RBuildModule module = GetByName(moduleName);

            if (module == null)
                throw new Exception(string.Format("Unknown '{0}' module", moduleName));

            Add(module);
        }

        public void Add(int index, RBuildModule moduleName)
        {
            base.Insert(index, moduleName);
        }

        public void DisableAll()
        {
            foreach (RBuildModule module in this)
            {
                // Disable module
                module.Enabled = false;
            }
        }

        public RBuildModule GetByName(string name)
        {
            foreach (RBuildModule module in this)
            {
                if (module.Name == name)
                    return module;
            }

            return null;
        }
    }
}
