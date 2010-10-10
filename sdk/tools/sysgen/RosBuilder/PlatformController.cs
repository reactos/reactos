using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;

namespace TriStateTreeViewDemo
{
    public class PlatformController
    {
        private ISysGenDesigner m_SysGenDesigner = null;

        public PlatformController(ISysGenDesigner engine)
        {
            m_SysGenDesigner = engine;
        }

        public ProjectTask ProjectTask
        {
            get { return m_SysGenDesigner.SysGenEngine.ProjectTask; }
        }

        public RBuildProject Project
        {
            get { return m_SysGenDesigner.SysGenEngine.Project; }
        }

        public void Remove(RBuildModule module)
        {
        }

        public void Add(RBuildModule module)
        {
            SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(module , Project.Platform.Modules);

            if (AskAddModulesToPlatform(dependencyTracker.Missing))
            {
                Project.Platform.Modules.Add(dependencyTracker.Dependencies);
            }
        }

        public void Add(RBuildModuleCollection modules)
        {
            SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(modules, Project.Platform.Modules);

            if (AskAddModulesToPlatform(dependencyTracker.Missing))
            {
                Project.Platform.Modules.Add(dependencyTracker.Dependencies);
            }
        }

        public bool AskAddModulesToPlatform(RBuildModuleCollection missingDependencies)
        {
            if (missingDependencies.Count > 0)
            {
                StringBuilder str = new StringBuilder();

                str.AppendFormat("This action requieres adding {0} dependecies no present in your platform :", missingDependencies.Count);
                str.AppendLine();
                str.AppendLine();

                foreach (RBuildModule dependency in missingDependencies)
                {
                    str.AppendFormat("{0} on '{1}' \n",
                        dependency.Name,
                        dependency.Base);
                }

                str.AppendLine();
                str.AppendLine("¿Do you want to add this dependencies?");

                if (MessageBox.Show(str.ToString(), "RosBuilder", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    return true;
                }
            }
            else
                return true;

            return false;
        }
    }
}
