using System;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace SysGen.BuildEngine
{
    public interface IFileHandler
    {
        void Process(RBuildFile file);
    }

    public abstract class NamedFileHandler : IFileHandler
    {
        public abstract string FileName { get; }

        public void Process(RBuildFile file)
        {
            if (file.Name == FileName)
            {
            }
        }

        protected abstract void Process();
    }

    public abstract class RegenerateFileHandler : NamedFileHandler
    {
        public virtual void Generate()
        {

        }
    }
}