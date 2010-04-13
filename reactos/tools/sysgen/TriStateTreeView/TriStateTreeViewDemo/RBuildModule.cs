using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;

namespace TriStateTreeViewDemo
{
    public enum TargetType
    {
        Win32CUI = 0,
    }

    [DefaultPropertyAttribute("Name")]
    public class RBuildModule
    {
        private string m_Path = null;
        private TargetType m_Type = TargetType.Win32CUI;

        public void GenerateFromPath(string path)
        {
            m_Path = path;
        }

        public string ModulePath
        {
            get { return m_Path; }
        }

        public string Name
        {
            get { return Path.GetFileName(m_Path); }
        }

        public string SourcePath
        {
            get { return Path.Combine(m_Path, "src"); }
        }

        public string ResourceFile
        {
            get { return Path.Combine(ModulePath, Name + ".rc"); }
        }

        public string CompiledFilename
        {
            get { return ""; }
        }

        public string Description
        {
            get { return ""; }
        }

        public TargetType Type
        {
            get { return m_Type; }
            set { m_Type = value; }
        }
    }
}
