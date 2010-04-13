using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Tasks;
using SysGen.Framework.Catalog;

namespace TriStateTreeViewDemo
{
    public class ProjectController
    {
        private ISysGenDesigner m_SysGenDesigner = null;
        private RBuildProject m_Project = null;

        public event EventHandler PlatformModulesUpdated;
        public event EventHandler ProjectLoaded;
        public event EventHandler ProjectSaved;
        public event EventHandler ProjectUpdated;

        public ProjectController(ISysGenDesigner engine)
        {
            m_SysGenDesigner = engine;

            PlatformCatalogReader m_Catalog = new PlatformCatalogReader(@"C:\Ros\trunk\reactos\rbuilddb.xml");
            m_Catalog.Read();

            m_Project = m_Catalog.Project;

            New();
        }

        public void New()
        {
            m_Project.Platform = new Project(m_Project);

            if (ProjectLoaded != null)
                ProjectLoaded(this, EventArgs.Empty);

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);
        }

        public void Open(string file)
        {
            SysGenProject = new Project(m_Project, file);
            SysGenProject.Load();

            if (ProjectLoaded != null)
                ProjectLoaded(this, EventArgs.Empty);

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);
        }

        public void Open()
        {
            using (OpenFileDialog openFile = new OpenFileDialog())
            {
                openFile.Title = "Open SysGen Designer Project File";
                //openFile.InitialDirectory = m_SysGenEngine.BaseDirectory;
                openFile.Filter = "SysGen Project File|*.sgpd";

                if (openFile.ShowDialog() == DialogResult.OK)
                {
                    SysGenProject = new Project(m_Project, openFile.FileName);
                    SysGenProject.Load();

                    if (ProjectLoaded != null)
                        ProjectLoaded(this, EventArgs.Empty);

                    if (PlatformModulesUpdated != null)
                        PlatformModulesUpdated(this, EventArgs.Empty);
                }
            }
        }

        public void Save()
        {
            using (SaveFileDialog saveFile = new SaveFileDialog())
            {
                saveFile.Title = "Save SysGen Designer Project File";
                //saveFile.InitialDirectory = m_SysGenEngine.BaseDirectory;
                saveFile.Filter = "SysGen Project File|*.sgpd";

                if (saveFile.ShowDialog() == DialogResult.OK)
                {
                    SysGenProject.ProjectPath = saveFile.FileName;
                    SysGenProject.Save();

                    if (PlatformModulesUpdated != null)
                        PlatformModulesUpdated(this, EventArgs.Empty);

                    if (ProjectSaved != null)
                        ProjectSaved(this, EventArgs.Empty);
                }
            }
        }

        public RBuildModuleCollection AvailableModules
        {
            get { return m_Project.Modules; }
        }

        public RBuildProject Project
        {
            get { return m_Project; }
        }

        public Project SysGenProject
        {
            get { return Project.Platform as Project; }
            set { Project.Platform = value; }
        }

        public void Remove(RBuildModule module)
        {
            SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(Project, module);

            if (dependencyTracker.Using.Count > 0)
            {
                string s = string.Format("Cannot remove module '{0}' because '{1}' modules depends on it",
                    module.Name,
                    dependencyTracker.Using.Count);

                MessageBox.Show(s, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                Project.Platform.Modules.Remove(module);
            }

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);
        }

        public void Add(RBuildModule module)
        {
            SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(Project , module);

            if (AskAddModulesToPlatform(dependencyTracker.Missing))
            {
                Project.Platform.Modules.Add(dependencyTracker.DependsOn);
                Project.Platform.Modules.Add(module);
            }

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);
        }

        public void Add(RBuildModuleCollection modules)
        {
            SysGenDependencyTracker dependencyTracker = new SysGenDependencyTracker(Project, modules);

            if (AskAddModulesToPlatform(dependencyTracker.Missing))
            {
                Project.Platform.Modules.Add(dependencyTracker.DependsOn);
                Project.Platform.Modules.Add(modules);
            }

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);
        }

        public void AddLanguage(RBuildLanguage language)
        {
            if (Project.Platform.Languages.Contains(language) == false)
                Project.Platform.Languages.Add(language);

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);

            if (ProjectUpdated != null)
                ProjectUpdated(this, EventArgs.Empty);
        }

        public void AddDebugChannel(RBuildDebugChannel channel)
        {
            if (Project.Platform.DebugChannels.Contains(channel) == false)
                Project.Platform.DebugChannels.Add(channel);

            if (PlatformModulesUpdated != null)
                PlatformModulesUpdated(this, EventArgs.Empty);

            if (ProjectUpdated != null)
                ProjectUpdated(this, EventArgs.Empty);
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
