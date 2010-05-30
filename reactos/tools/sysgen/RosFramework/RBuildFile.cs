using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum PathRoot
    {
        Default,
        SourceCode,
        Output,
        Intermediate,
        CDOutput,
        Temporary,
        Install,
        BootCD,
        LiveCD,
        Platform
    }

    public enum SourceType
    {
        Unknown,
        C,
        CPP,
        IDL,
        Assembler,
        NASM,
        WineBuild,
        WindResource,
        MessageTable,
        Header
    }

    public class RBuildFolder : RBuildFileSystemInfo
    {
        private List<RBuildFileSystemInfo> m_Contents = new List<RBuildFileSystemInfo>();

        public RBuildFolder()
        {
        }

        public RBuildFolder(RBuildFolder parentFolder , string name)
        {
            Root = parentFolder.Root;
            Base = parentFolder.Base;
            Name = name;
        }

        public RBuildFolder(PathRoot rootPath)
        {
            Root = rootPath;
            //Base = ".";
        }

        public RBuildFolder (PathRoot rootPath, string basePath)
        {
            Root = rootPath;
            Base = basePath;
        }

        public RBuildFolder Parent
        {
            get 
            {
                RBuildFolder folder = null;
                
                folder = new RBuildFolder();
                folder.Base = Path.GetDirectoryName(FullPath);
                folder.Name = "";
                folder.Root = Root;

                return folder;
                // return new RBuildFolder(Root, Path.GetDirectoryName(Base), Name); 
            }
        }

        public List<RBuildFileSystemInfo> Contents
        {
            get { return m_Contents; }
        }

        public override string ToString()
        {
            return string.Format("{0} - {1}", Root, FullPath);
        }
    }

    public class RBuildSourceFile : RBuildFile, IComparable
    {
        private bool m_First = false;
        private string m_Switches = string.Empty;

        public string Switches
        {
            get { return m_Switches; }
            set { m_Switches = value; }
        }

        public bool First
        {
            get { return m_First; }
            set { m_First = value; }
        }

        public SourceType Type
        {
            get
            {
                if (IsC)
                    return SourceType.C;
                if (IsCPP)
                    return SourceType.CPP;
                if (IsAssembler)
                    return SourceType.Assembler;
                if (IsNASM)
                    return SourceType.NASM;
                if (IsWidl)
                    return SourceType.IDL;
                if (IsWindResource)
                    return SourceType.WindResource;
                if (IsWineBuild)
                    return SourceType.WineBuild;
                if (IsMessageTable)
                    return SourceType.MessageTable;
                if (IsHeader)
                    return SourceType.Header;

                return SourceType.Unknown;
            }
        }

        public bool CompilableObject
        {
            get
            {
                if (IsWidl)
                    return false;
                if (IsMessageTable)
                    return false;

                return true;
            }
        }

        public bool IsCPP
        {
            get
            {
                switch (Extension)
                {
                    case ".cc":
                    case ".cpp":
                    case ".cxx":
                        return true;
                    default:
                        return false;
                }
            }
        }

        public bool IsWidl
        {
            get
            {
                if (Extension == ".idl")
                    return true;

                return false;
            }
        }

        public bool IsMessageTable
        {
            get
            {
                if (Extension == ".mc")
                    return true;

                return false;
            }
        }

        public bool IsHeader
        {
            get
            {
                if (Extension == ".h")
                    return true;

                return false;
            }
        }

        public bool IsWineBuild
        {
            get
            {
                if (Extension == ".spec")
                    return true;

                return false;
            }
        }

        public bool IsWindResource
        {
            get
            {
                if (Extension == ".rc")
                    return true;

                return false;
            }
        }

        public bool IsNASM
        {
            get
            {
                if (Extension == ".asm")
                    return true;

                return false;
            }
        }

        public bool IsAssembler
        {
            get
            {
                if (Extension == ".s")
                    return true;

                return false;
            }
        }

        public bool IsC
        {
            get
            {
                if (Extension == ".c")
                    return true;

                return false;
            }
        }

        public bool IsCompilable
        {
            get { return (IsC || IsCPP || IsAssembler || IsNASM || IsWidl || IsWindResource || IsWineBuild || IsHeader); }
        }

        public override string ToString()
        {
            return string.Format("{0} {1}" , Name , Type);
        }

        #region IComparable Members

        public int CompareTo(object obj)
        {
            RBuildSourceFile source = (RBuildSourceFile)obj;

            if (First != source.First)
            {
                if (source.First)
                    return 1;
                else
                    return -1;
            }

            return 0;
        }

        #endregion
    }

    public class RBuildFile : RBuildFileSystemInfo
    {
        public RBuildFile()
        {
        }

        public RBuildFile(RBuildElement element)
            : base (element)
        {
        }

        public string Extension
        {
            get { return Path.GetExtension(Name).Trim().ToLower(); }
        }

        public override object Clone()
        {
            RBuildFile file = new RBuildFile();

            file.Base = Base;
            file.Name = Name;
            file.Root = Root;
            file.Enabled = Enabled;

            return file;
        }

        public RBuildFolder Folder
        {
            get { return new RBuildFolder(Root, Base); }
        }
    }

    /// <summary>
    /// Represents the base class for source-code , installable file , folder , include folder , cdfile taks ... from a build.
    /// </summary>
    public abstract class RBuildFileSystemInfo : ICloneable
    {
        protected RBuildElement m_Element = null;
        protected PathRoot m_Root = PathRoot.Default;
        protected string m_Name = string.Empty; //".";
        protected string m_Base = string.Empty; //null;
        protected bool m_Enabled = true;

        public RBuildFileSystemInfo(RBuildElement element)
        {
            m_Element = element;
        }

        public RBuildFileSystemInfo()
        {
        }

        private string NormalizePath(string path)
        {
            return path.Replace(
                Path.AltDirectorySeparatorChar,
                Path.DirectorySeparatorChar);
        }

        public string FullPath
        {
            get {

                if (Base == null || Name == null)
                {
                    int i = 10;
                }

                return NormalizePath(Path.Combine(Base, Name));
            }
        }

        public string Name
        {
            get { return m_Name; }
            set
            {
                if (value == null)
                {
                    int i = 10;
                }
                m_Name = value; }
        }

        public string Base
        {
            get { return m_Base; }
            set
            {
                if (value == null)
                {
                    int i = 10;
                } 
                m_Base = value;
            }
        }

        public bool Enabled
        {
            get { return m_Enabled; }
            set { m_Enabled = value; }
        }

        //TODO : ELIMINAR
        public RBuildElement Element
        {
            get { return m_Element; }
            set { m_Element = value; }
        }

        public virtual PathRoot Root
        {
            get { return m_Root; }
            set { m_Root = value; }
        }

        public string[] BasePathParts
        {
            get { return Base.Split(new char[] { '\\' }); }
        }

        public override bool Equals(object obj)
        {
            if (obj is RBuildFileSystemInfo)
            {
                RBuildFileSystemInfo rfsInfo = obj as RBuildFileSystemInfo;

                if ((rfsInfo.Base == Base) &&
                    (rfsInfo.Root == Root) &&
                    (rfsInfo.Name == Name))
                {
                    return true;
                }

                if ((rfsInfo.FullPath == FullPath) && (rfsInfo.Root == Root))
                {
                    return true;
                }
            }

            // The instances are not equal
            return false;
        }

        public override int GetHashCode()
        {
            return Base.GetHashCode() ^ Name.GetHashCode() ^ Root.GetHashCode ();
        }

        #region ICloneable Members

        public virtual object Clone()
        {
            throw new Exception("The method or operation is not implemented.");
        }

        #endregion
    }
}