using System;
using System.Collections.Specialized;
using System.Collections.Generic;
using System.Xml;
using System.Collections;
using System.IO;
using System.Text;

using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
    public enum TargetArchitectureType
    {
        X86,
        X86_i486,
        X86_i586,
        X86_Pentium,
        X86_Pentium2,
        X86_Pentium3,
        X86_Pentium4,
        X86_AthlonXP,
        X86_AthlonMP,
        X86_Xbox,
        PPC,
        ARM
    }

    public enum OptimizeLevelType : int
    {
        Level_0 = 0,
        Level_1 = 1,
        Level_2 = 2,
        Level_3 = 3,
        Level_4 = 4,
        Level_5 = 5
    }

    public class Project : RBuildPlatform
	{
        RBuildProject m_Project = null;

		string m_FilePath; // full path to this project, including filename
        string m_FileName = "Unnamed";

        //MovieOptions movieOptions;
        //CompilerOptions compilerOptions;
        //PathCollection classpaths;
        //PathCollection compileTargets;
        //HiddenPathCollection hiddenPaths;
        //AssetCollection libraryAssets;
        bool traceEnabled; // selected configuration 

        public bool NoOutput; // Disable file building
        public string InputPath; // For code injection
		public string OutputPath;
		public string PreBuildEvent;
		public string PostBuildEvent;
        public string TestMovieCommand;
		public bool AlwaysRunPostBuild;
		public bool ShowHiddenPaths;

        public string Source;

		public Project(RBuildProject project,string path)
		{
			m_FilePath = path;
            m_FileName = Path.GetFileName(path);
            m_Project = project;
            m_Project.Platform = this;
		}

        public Project(RBuildProject project)
        {
            m_Project = project;
            m_Project.Platform = this;
        }

        public RBuildProject RBuildProject
        {
            get { return m_Project; }
        }

        public virtual bool UsesInjection { get { return false; } }
        public virtual bool HasLibraries { get { return false; } }
        public virtual void ValidateBuild(out string error) { error = null; }

		#region Simple Properties

        public string ProjectPath { get { return m_FilePath; } set { m_FilePath = value; } }
        //public string Name { get { return Path.GetFileNameWithoutExtension(path).Replace(' ', '-'); } }
		public string Directory { get { return Path.GetDirectoryName(m_FilePath); } }
        public Boolean TraceEnabled { set { traceEnabled = value; } get { return traceEnabled; } }

        public string FileName
        {
            get { return m_FileName; }
        }
		
        //// we only provide getters for these to preserve the original pointer
        //public MovieOptions MovieOptions { get { return movieOptions; } }
        //public PathCollection Classpaths { get { return classpaths; } }
        //public PathCollection CompileTargets { get { return compileTargets; } }
        //public HiddenPathCollection HiddenPaths { get { return hiddenPaths; } }
        //public AssetCollection LibraryAssets { get { return libraryAssets; } }

        //public CompilerOptions CompilerOptions
        //{
        //    get { return compilerOptions; }
        //    set { compilerOptions = value; }
        //}

        //public PathCollection AbsoluteClasspaths
        //{
        //    get
        //    {
        //        PathCollection absolute = new PathCollection();
        //        foreach (string cp in classpaths)
        //            absolute.Add(GetAbsolutePath(cp));
        //        return absolute;
        //    }
        //}

		//public string OutputPathAbsolute { get { return GetAbsolutePath(OutputPath); } }

        public bool CanBuild
        {
            get { return OutputPath != null && OutputPath.Length > 0; }
        }

		#endregion

		#region Methods

		// all the Set/Is methods expect absolute paths (as opposed to the way they're
		// actually stored)

        //public void SetPathHidden(string path, bool isHidden)
        //{
        //    path = GetRelativePath(path);

        //    if (isHidden)
        //    {
        //        hiddenPaths.Add(path);				
        //        compileTargets.RemoveAtOrBelow(path); // can't compile hidden files
        //        libraryAssets.RemoveAtOrBelow(path); // can't embed hidden resources
        //    }
        //    else hiddenPaths.Remove(path);
        //}

        //public bool IsPathHidden(string path)
        //{
        //    return hiddenPaths.IsHidden(GetRelativePath(path));
        //}
		
		public void SetCompileTarget(string path, bool isCompileTarget)
		{
            //if (isCompileTarget)
            //    compileTargets.Add(GetRelativePath(path));
            //else
            //    compileTargets.Remove(GetRelativePath(path));
		}

		//public bool IsCompileTarget(string path) { return compileTargets.Contains(GetRelativePath(path)); }

        public void SetLibraryAsset(string path, bool isLibraryAsset)
        {
            //if (isLibraryAsset)
            //    libraryAssets.Add(GetRelativePath(path));
            //else
            //    libraryAssets.Remove(GetRelativePath(path));
        }

        //public bool IsLibraryAsset(string path) { return libraryAssets.Contains(GetRelativePath(path)); }
//        public LibraryAsset GetAsset(string path) { return libraryAssets[GetRelativePath(path)]; }

        public void ChangeAssetPath(string fromPath, string toPath)
        {
            //if (IsLibraryAsset(fromPath))
            //{
            //    //LibraryAsset asset = libraryAssets[GetRelativePath(fromPath)];
            //    //libraryAssets.Remove(asset);
            //    //asset.Path = GetRelativePath(toPath);
            //    //libraryAssets.Add(asset);
            //}
        }

        //public bool IsInput(string path) { return GetRelativePath(path) == InputPath; }
        //public bool IsOutput(string path) { return GetRelativePath(path) == OutputPath; }

		/// <summary>
		/// Call this when you delete a path so we can remove all our references to it
		/// </summary>
		public void NotifyPathsDeleted(string path)
		{
			//path = GetRelativePath(path);
            //hiddenPaths.Remove(path);
            //compileTargets.RemoveAtOrBelow(path);
            //libraryAssets.RemoveAtOrBelow(path);
		}

		/// <summary>
		/// Returns the path to the "obj\" subdirectory, creating it if necessary.
		/// </summary>
		public string GetObjDirectory()
		{
			string objPath = Path.Combine(this.Directory, "obj");
			if (!System.IO.Directory.Exists(objPath))
				System.IO.Directory.CreateDirectory(objPath);
			return objPath;
		}

		#endregion

		#region Relative Path Helpers

        //public string GetRelativePath(string path)
        //{
        //    return ProjectPaths.GetRelativePath(this.Directory,path);
        //}

        //public string GetAbsolutePath(string path)
        //{
        //    return ProjectPaths.GetAbsolutePath(this.Directory,path);
        //}

		#endregion

        public Project Load()
        {
            ProjectReader reader = new ProjectReader(this, ProjectPath);

            try
            {
                return reader.ReadProject();
            }
            catch (XmlException exception)
            {
                string format = string.Format("Error in Project '{0}' line {1}, position {2}.",
                    ProjectPath,
                    exception.LineNumber,
                    exception.LinePosition);

                throw new Exception(format, exception);
            }
            finally
            {
                reader.Close();
            }
        }

        public void Save()
        {
            ProjectWriter writer = new ProjectWriter(this, ProjectPath);

            try
            {
                writer.WriteProject();
                writer.Flush();
            }
            finally
            {
                writer.Close();
            }
        }
    }
}
