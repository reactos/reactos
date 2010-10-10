using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine.Framework;

namespace SysGen.BuildEngine
{
    public class SysGenDependencyTracker
    {
        RBuildProject m_Project = null;
        RBuildModuleCollection m_Modules = new RBuildModuleCollection();
        RBuildModuleCollection m_DependsOn = new RBuildModuleCollection();
        RBuildModuleCollection m_DependencyOf = new RBuildModuleCollection();

        public SysGenDependencyTracker(RBuildProject project)
        {
            m_Project = project;
        }

        public SysGenDependencyTracker(RBuildProject project, RBuildModule module)
            : this(project)
        {
            m_Modules.Add(module);
            Calculate();
        }

        public SysGenDependencyTracker(RBuildProject project, RBuildModuleCollection modules)
            : this(project)
        {
            m_Modules.Add(modules);
            Calculate();
        }

        public void Calculate()
        {
            m_DependsOn.Clear();
            m_DependencyOf.Clear();

            foreach (RBuildModule module in m_Modules)
            {
                GetModuleDependencies(module);
            }

            foreach (RBuildModule projectModule in m_Project.Modules)
            {
                foreach (RBuildModule module in m_Modules)
                {
                    if (projectModule.Needs.Contains(module))
                    {
                        m_DependencyOf.Add(projectModule);
                    }
                }
            }
        }

        private void GetModuleDependencies(RBuildModule module)
        {
            foreach (RBuildModule library in module.Needs)
            {
                if (m_DependsOn.Contains(library) == false)
                {
                    if (m_Modules.Contains(library) == false)
                    {
                        //Add it to the list of dependencies 
                        m_DependsOn.Add(library);

                        //Investigate the module to find its dependencies
                        GetModuleDependencies(library);
                    }
                }
            }
        }

        public RBuildModuleCollection DependsOn
        {
            get { return m_DependsOn; }
        }

        public RBuildModuleCollection DependencyOf
        {
            get { return m_DependencyOf; }
        }

        public RBuildModuleCollection Missing
        {
            get
            {
                RBuildModuleCollection missing = new RBuildModuleCollection();

                foreach (RBuildModule dependency in DependsOn)
                {
                    if (m_Project.Platform.Modules.Contains(dependency) == false)
                        missing.Add(dependency);
                }

                return missing;
            }
        }

        public RBuildModuleCollection Using
        {
            get
            {
                RBuildModuleCollection missing = new RBuildModuleCollection();

                foreach (RBuildModule dependency in DependencyOf)
                {
                    if (m_Project.Platform.Modules.Contains(dependency) == true)
                        missing.Add(dependency);
                }

                return missing;
            }
        }
    }
}
