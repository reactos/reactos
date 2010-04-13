using System;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public abstract class RBuildElement : IRBuildNamed
    {
        protected string m_RBuild = null;
        protected string m_Name = null;
        protected string m_Base = null;
        protected string m_Path = null;
        protected string m_XmlFile = null;

        protected List<string> m_AssemblyFlags = new List<string>();
        protected List<string> m_CompilerFlags = new List<string>();
        protected List<string> m_LinkerFlags = new List<string>();
        protected RBuildIncludeFolderCollection m_Includes = new RBuildIncludeFolderCollection();
        protected RBuildPlatformFileCollection m_Files = new RBuildPlatformFileCollection();
        protected RBuildDefineCollection m_Defines = new RBuildDefineCollection();
        protected RBuildPropertyCollection m_Properties = new RBuildPropertyCollection();
        protected RBuildFolderCollection m_Folders = new RBuildFolderCollection();
        protected RBuildFolder m_Folder = new RBuildFolder();

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public virtual RBuildFolder Folder
        {
            get { return m_Folder; }
            set { m_Folder = value; }
        }

        public string XmlFile
        {
            get { return m_XmlFile; }
            set { m_XmlFile = value; }
        }

        public string Path
        {
            get { return m_Path; }
            set { m_Path = value; }
        }

        public string Base
        {
            get { return Folder.FullPath; }
        }

        public string RBuildPath
        {
            get { return System.IO.Path.Combine(Base, RBuildFile); }
        }

        public string RBuildFile
        {
            get { return m_RBuild; }
            set { m_RBuild = value; }
        }

        public Uri BaseURI
        {
            get { return new Uri(Base, UriKind.Relative); }
        }

        public string BaseParent
        {
            get { return Base.Replace(System.IO.Path.DirectorySeparatorChar + Name , string.Empty); }
        }

        public string FolderFullPath
        {
            get { return System.IO.Path.Combine(Path, Base); }
        }

        public RBuildFolderCollection Folders
        {
            get { return m_Folders; }
        }

        public RBuildPlatformFileCollection Files
        {
            get { return m_Files; }
        }

        public RBuildPropertyCollection Properties
        {
            get { return m_Properties; }
        }

        public List<string> CompilerFlags
        {
            get { return m_CompilerFlags; }
        }

        public List<string> LinkerFlags
        {
            get { return m_LinkerFlags; }
        }

        public List<string> AssemblyFlags
        {
            get { return m_AssemblyFlags; }
        }

        public RBuildDefineCollection Defines
        {
            get { return m_Defines; }
        }

        public RBuildIncludeFolderCollection IncludeFolders
        {
            get { return m_Includes; }
            set { m_Includes = value; }
        }

        public string MakeFilePreCondition
        {
            get { return string.Format("{0}_PRECONDITION", Name); }
        }

        public string MakeFileRCFlags
        {
            get { return string.Format("{0}_RCFLAGS", Name); }
        }

        public string MakeFileWIDLFlags
        {
            get { return string.Format("{0}_WIDLFLAGS", Name); }
        }

        public string MakeFileLFlags
        {
            get { return string.Format("{0}_LFLAGS", Name); }
        }

        public string MakeFileNASMFlags
        {
            get { return string.Format("{0}_NASMFLAGS", Name); }
        }

        public string MakeFileFoldersMacro
        {
            get { return string.Format("$({0}_FOLDERS)", Name); }
        }

        public string MakeFileFolders
        {
            get { return string.Format("{0}_FOLDERS", Name); }
        }

        public string MakeFileCFlags
        {
            get { return string.Format("{0}_CFLAGS", Name); }
        }

        public string MakeFileObjs
        {
            get { return string.Format("{0}_OBJS", Name); }
        }

        public string MakeFileSources
        {
            get { return string.Format("{0}_SOURCES", Name); }
        }

        public string MakeFileHeaders
        {
            get { return string.Format("{0}_HEADERS", Name); }
        }

        public string MakeFileMakeTarget
        {
            get { return string.Format("{0}", Name); }
        }

        public string MakeFileFlagDebugTarget
        {
            get { return string.Format("{0}_flagdebug", Name); }
        }

        public string MakeFileInfoTarget
        {
            get { return string.Format("{0}_info", Name); }
        }

        public string MakeFileCleanTarget
        {
            get { return string.Format("{0}_clean", Name); }
        }

        public string MakeFileDependsTarget
        {
            get { return string.Format("{0}_depends", Name); }
        }

        public string MakeFileInstallTarger
        {
            get { return string.Format("{0}_install", Name); }
        }

        public string MakeFileHeadersMacro
        {
            get { return string.Format("$({0}_HEADERS)", Name); }
        }

        public string MakeFileMCHeadersMacro
        {
            get { return string.Format("$({0}_MCHEADERS)", Name); }
        }

        public string MakeFileMCHeaders
        {
            get { return string.Format("{0}_MCHEADERS", Name); }
        }

        public string MakeFileRPCHeadersMacro
        {
            get { return string.Format("$({0}_RPCHEADERS)", Name); }
        }

        public string MakeFileRPCHeaders
        {
            get { return string.Format("{0}_RPCHEADERS", Name); }
        }

        public string MakeFileRPCSourcesMacro
        {
            get { return string.Format("$({0}_RPCSOURCES)", Name); }
        }

        public string MakeFileRPCSources
        {
            get { return string.Format("{0}_RPCSOURCES", Name); }
        }

        public string MakeFilePCHMacro
        {
            get { return string.Format("$({0}_PCH)", Name); }
        }

        public string MakeFilePCHHeaders
        {
            get { return string.Format("{0}_PCH", Name); }
        }

        public string MakeFileNASMMacro
        {
            get { return string.Format("$({0}_NASMFLAGS)", Name); }
        }

        public string MakeFileCFlagsMacro
        {
            get { return string.Format("$({0}_CFLAGS)", Name); }
        }

        public string MakeFileLFlagsMacro
        {
            get { return string.Format("$({0}_LFLAGS)", Name); }
        }

        public string MakeFileWIDLFlagsMacro
        {
            get { return string.Format("$({0}_WIDLFLAGS)", Name); }
        }

        public string MakeFileObjsMacro
        {
            get { return string.Format("$({0}_OBJS)", Name); }
        }

        public string MakeFileSourcesMacro
        {
            get { return string.Format("$({0}_SOURCES)", Name); }
        }

        public string MakeFilePreConditionMacro
        {
            get { return string.Format("$({0}_PRECONDITION)", Name); }
        }

        public string MakeFileRCFlagsMacro
        {
            get { return string.Format("$({0}_RCFLAGS)", Name); }
        }

        public abstract void SaveAs(string file);

        public override bool Equals(object obj)
        {
            if (obj is RBuildElement)
            {
                RBuildElement element = obj as RBuildElement;

                if (element.Name == Name)
                    return true;
            }

            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }
    }
}