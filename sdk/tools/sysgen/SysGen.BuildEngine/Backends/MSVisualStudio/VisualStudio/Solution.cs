using System;
using System.IO;
using System.Xml;
using System.Collections;
using System.Collections.Generic;
using System.Text;

namespace SysGen.BuildEngine.Framework.VisualStudio
{
    public enum VisualStudioVersion
    {
        VS6,
        VS2002,
        VS2003,
        VS2005
    }

    #region ProjectFile
    public class ProjectFile
    {
        private string relPath = "";
        private string basePath = "";
        private string buildAction = "";
        private string subType = "";

        public string AbsolutePath
        {
            get
            {
                return Path.Combine(basePath, relPath);
            }
        }
        public string AbsoluteDirectory
        {
            get
            {
                return Path.GetDirectoryName(Path.Combine(basePath, relPath));
            }
        }

        public string RelativePath
        {
            get
            {
                return relPath;
            }
        }

        public string BasePath
        {
            get
            {
                return basePath;
            }
        }
        public string BuildAction
        {
            get
            {
                return buildAction;
            }
        }
        public string SubType
        {
            get
            {
                return subType;
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder("\nProject File:");
            buff.Append("\n\tRelativePath:");
            buff.Append(relPath);
            buff.Append("\n\tBuildAction:");
            buff.Append(buildAction);
            buff.Append("\n\tSubType:");
            buff.Append(subType);
            buff.Append("\n\tBasePath:");
            buff.Append(basePath);
            return buff.ToString();
        }

        public ProjectFile(
           string relPath,
           string buildAction,
           string subType,
           string basePath)
        {
            this.relPath = relPath;
            this.buildAction = buildAction;
            this.subType = subType;
            this.basePath = basePath;
        }
    }
    #endregion

    #region ProjectFileCollection
    public class ProjectFileCollection : ReadOnlyCollectionBase
    {
        public ProjectFile this[int index]
        {
            get
            {
                return (ProjectFile)this.InnerList[index];
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProjectFileCollection");
            foreach (ProjectFile pf in this.InnerList)
            {
                buff.Append(pf.ToString());
            }
            return buff.ToString();
        }

        public ProjectFileCollection(ProjectFile[] projectFileArray)
        {
            foreach (ProjectFile projectFile in projectFileArray)
            {
                this.InnerList.Add(projectFile);
            }
        }


    }
    #endregion

    #region ProjectReference
    public class ProjectReference
    {
        private string name = "";
        private string assemblyName = "";
        private string hintPath = "";
        private string basePath = "";

        public string Name
        {
            get
            {
                return this.name;

            }
        }
        public string AssemblyName
        {
            get
            {
                return this.assemblyName;
            }
        }
        public string HintPath
        {
            get
            {
                return this.hintPath;
            }
        }
        public string AbsolutePath
        {
            get
            {
                return Path.Combine(this.basePath, this.hintPath);
            }
        }

        public string BasePath
        {
            get
            {
                return this.basePath;
            }
        }
        public override string ToString()
        {
            StringBuilder buff = new StringBuilder("\nReference:");
            buff.Append("\n\tName");
            buff.Append(name);
            buff.Append("\n\tAssemblyName");
            buff.Append(assemblyName);
            buff.Append("\n\tHintPath:");
            buff.Append(hintPath);
            buff.Append("\n\tBasePath:");
            buff.Append(basePath);
            return buff.ToString();
        }

        public ProjectReference(string name,
           string assemblyName,
           string hintPath,
           string basePath)
        {
            this.name = name;
            this.assemblyName = assemblyName;
            this.hintPath = hintPath;
            this.basePath = basePath;
        }

    }
    #endregion

    #region ProjectReferenceCollection
    public class ProjectReferenceCollection : ReadOnlyCollectionBase
    {
        public ProjectReference this[int index]
        {
            get
            {
                return (ProjectReference)this.InnerList[index];
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProjectReferenceCollection");
            foreach (ProjectReference pr in this.InnerList)
            {
                buff.Append(pr.ToString());
            }
            return buff.ToString();
        }

        public ProjectReferenceCollection(ProjectReference[]
         projectReferenceArray)
        {
            foreach (ProjectReference projectReference in projectReferenceArray)
            {
                this.InnerList.Add(projectReference);
            }
        }

    }
    #endregion

    #region ProjectConfigItem
    public class ProjectConfigItem
    {
        private string key = "";
        private string keyValue = "";

        public string Key
        {
            get
            {
                return key;
            }
        }
        public string Value
        {
            get
            {
                return keyValue;
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProjectConfigItem:");
            buff.Append("\n\tKey:");
            buff.Append(key);
            buff.Append("\n\tValue:");
            buff.Append(keyValue);
            return buff.ToString();
        }

        public ProjectConfigItem(string key, string keyValue)
        {
            this.key = key;
            this.keyValue = keyValue;

        }
    }
    #endregion

    #region ProjectConfigItemCollection
    public class ProjectConfigItemCollection : ReadOnlyCollectionBase
    {
        private string basePath = "";

        public string Name
        {
            get
            {
                string name = "";
                foreach (ProjectConfigItem pc in this.InnerList)
                {
                    if (pc.Key.Equals("Name"))
                    {
                        name = pc.Value;
                        break;
                    }
                }
                return name;
            }
        }

        public string OutputRelPath
        {
            get
            {
                string relPath = "";
                foreach (ProjectConfigItem pc in this.InnerList)
                {
                    if (pc.Key.Equals("OutputPath"))
                    {
                        relPath = pc.Value;
                        break;
                    }
                }
                return relPath;
            }
        }

        public string OutputAbsolutePath
        {
            get
            {
                string absPath = OutputRelPath;
                absPath = Path.Combine(this.basePath, absPath);
                return absPath.Replace("\\.\\", "");
            }
        }

        public ProjectConfigItem this[int index]
        {
            get
            {
                return (ProjectConfigItem)this.InnerList[index];
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProjectConfigItemCollection:");
            foreach (ProjectConfigItem pc in this.InnerList)
            {
                buff.Append(pc.ToString());
            }
            return buff.ToString();
        }

        public ProjectConfigItemCollection(ProjectConfigItem[]
         projectConfigItemArray, string basePath)
        {
            this.basePath = basePath;
            foreach (ProjectConfigItem pc in projectConfigItemArray)
            {
                this.InnerList.Add(pc);
            }

        }
    }
    #endregion

    #region ProjectConfigCollection
    public class ProjectConfigCollection : ReadOnlyCollectionBase
    {
        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProjectConfigCollection:");
            foreach (ProjectConfigItemCollection pc in this.InnerList)
            {
                buff.Append(pc.ToString());
            }
            return buff.ToString();
        }

        public ProjectConfigItemCollection this[int index]
        {
            get
            {
                return (ProjectConfigItemCollection)this.InnerList[index];
            }
        }

        public ProjectConfigCollection(XmlNodeList configItems, string basePath)
        {
            if (configItems.Count > 0)
            {
                foreach (XmlNode configItem in configItems)
                {
                    // create an array of items:
                    ProjectConfigItem[] projectConfigItemArray = new
                     ProjectConfigItem[configItem.Attributes.Count];
                    int i = 0;
                    foreach (XmlAttribute attrib in configItem.Attributes)
                    {
                        projectConfigItemArray[i] = new
                         ProjectConfigItem(attrib.Name, attrib.Value);
                        i++;
                    }
                    // create a ProjectConfigItemCollection:
                    ProjectConfigItemCollection projectConfigItemCollection = new
                     ProjectConfigItemCollection(projectConfigItemArray, basePath);
                    this.InnerList.Add(projectConfigItemCollection);
                }
            }
        }
    }
    #endregion

    #region Project
    public class VSProject : VSItem
    {
        private string basePath = "";
        private string projectBasePath = "";
        private string projectFileName = "";
        private string projectGuid = "";
        private string projectConfigurationGuid = "";
        private string projectName = "";
        private string projectType = "";
        private ProjectConfigCollection configCollection = null;
        private ProjectReferenceCollection referenceCollection = null;
        private ProjectFileCollection fileCollection = null;

        public string AbsolutePath
        {
            get
            {
                return Path.Combine(basePath, projectFileName);
            }
        }

        public string AbsoluteDirectory
        {
            get
            {
                return basePath;
            }
        }

        public ProjectConfigCollection Configurations
        {
            get
            {
                return configCollection;
            }
        }
        public ProjectReferenceCollection ReferenceCollection
        {
            get
            {
                return referenceCollection;
            }
        }
        public ProjectFileCollection FileCollection
        {
            get
            {
                return fileCollection;
            }
        }

        public string RelPath
        {
            get
            {
                return projectFileName;
            }
        }

        public string Guid
        {
            get
            {
                return projectGuid;
            }
        }

        public string ConfigurationGuid
        {
            get
            {
                return projectConfigurationGuid;
            }
        }

        public string Name
        {
            get
            {
                return projectName;
            }
        }

        public string ProjectType
        {
            get
            {
                return projectType;
            }
        }

        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProject:");
            buff.Append("\n\tName:");
            buff.Append(projectName);
            buff.Append("\n\tFileName:");
            buff.Append(projectFileName);
            buff.Append("\n\tBasePath:");
            buff.Append(basePath);
            buff.Append("\n\tGuid:");
            buff.Append(projectGuid);
            buff.Append("\nConfiguration:");
            buff.Append(configCollection.ToString());
            buff.Append("\nReferences:");
            buff.Append(referenceCollection.ToString());
            buff.Append("\nFiles:");
            buff.Append(fileCollection.ToString());
            return buff.ToString();
        }
    }
    #endregion

    #region ProjectCollection
    public class ProjectCollection : List<VSProject>
    {
        public override string ToString()
        {
            StringBuilder buff = new StringBuilder();
            buff.Append("\nProject Collection:");
            foreach (VSProject p in this)
            {
                buff.Append(p.ToString());
            }
            return buff.ToString();
        }
    }
    #endregion

    #region Solution
    public class VSSolution : VSItem
    {
        private string solutionFileName = "";
        private string solutionDirectory = "";
        private string solutionFileVersion = "";
        private ProjectCollection projectCollection = null;

        public string LongestSharedPath
        {
            get
            {
                // find the longest path which is shared by all of the 
                // objects in the project (if any)
                string longestSharedPath = solutionDirectory;
                foreach (VSProject p in projectCollection)
                {
                    string projectDir = p.AbsoluteDirectory;
                    longestSharedPath = getMinimumSharedPath(longestSharedPath,
                     projectDir);
                    foreach (ProjectFile pf in p.FileCollection)
                    {
                        string fileDir = pf.AbsoluteDirectory;
                        longestSharedPath = getMinimumSharedPath(longestSharedPath,
                         fileDir);
                    }
                }
                return longestSharedPath;
            }
        }

        private string getMinimumSharedPath(
           string sharedPath,
           string newPath
           )
        {
            string retSharedPath = "";
            if (sharedPath.Length > 0)
            {
                string[] sharedPathParts = sharedPath.Split(
                   (new char[] {Path.DirectorySeparatorChar,
                Path.AltDirectorySeparatorChar }));
                string[] newPathParts = newPath.Split(
                   (new char[] {Path.DirectorySeparatorChar,
                Path.AltDirectorySeparatorChar}));
                int max = Math.Min(sharedPathParts.Length, newPathParts.Length);
                int sharedCount = 0;
                for (int i = 0; i < max; i++)
                {
                    if
                     (!sharedPathParts[i].ToUpper().Equals(newPathParts[i].ToUpper())
                    )
                    {
                        break;
                    }
                    else
                    {
                        sharedCount = i + 1;
                    }
                }
                if (sharedCount == 0)
                {
                    return "";
                }
                else
                {
                    for (int i = 0; i < sharedCount; i++)
                    {
                        if (retSharedPath.Length > 0)
                        {
                            retSharedPath += Path.DirectorySeparatorChar;
                        }
                        retSharedPath += sharedPathParts[i];
                    }
                }
            }
            return retSharedPath;
        }

        public string FileVersion
        {
            get
            {
                return solutionFileVersion;
            }
        }

        public ProjectCollection Projects
        {
            get
            {
                return projectCollection;
            }
        }

        public string FileName
        {
            get
            {
                return solutionFileName;
            }
            set
            {
                solutionFileName = value;
            }
        }

        public string BasePath
        {
            get
            {
                return solutionDirectory;
            }
        }

        public override string ToString()
        {
            StringBuilder s = new StringBuilder();
            s.Append("\nSolution:");
            s.Append("\nFileName:");
            s.Append(solutionFileName);
            s.Append("\nVersion:");
            s.Append(solutionFileVersion);
            if (projectCollection != null)
            {
                s.Append(projectCollection.ToString());
            }
            return s.ToString();
        }

        public VSSolution()
        {
        }

        public VSSolution(string fileName)
        {
            solutionFileName = fileName;
        }
    }
    #endregion

    public class VSItem
    {
        private string m_Name = null;
        private string m_FileName = null;

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string FileName
        {
            get { return m_FileName; }
            set { m_FileName = value; }
        }

        public void SaveAs(VisualStudioVersion version)
        {
        }
    }
}