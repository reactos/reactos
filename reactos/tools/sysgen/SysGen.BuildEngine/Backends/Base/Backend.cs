using System;
using System.Reflection;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;
using SysGen.BuildEngine;
using SysGen.BuildEngine.Log;

namespace SysGen.BuildEngine.Backends
{
    public abstract class Backend
    {
        private SysGenEngine m_SysGenEngine = null;

        public Backend(SysGenEngine sysgen)
        {
            m_SysGenEngine = sysgen;
        }

        public SysGenEngine SysGen
        {
            get { return m_SysGenEngine; }
        }

        public RBuildProject Project
        {
            get { return m_SysGenEngine.Project; }
        }

        public string AppInfo
        {
            get { return string.Format("{0} {1}", AppName, AppVersion); }
        }

        public string AppName
        {
            get { return "SysGen"; }
        }

        public string AppVersion
        {
            get 
            {
                FileVersionInfo info = FileVersionInfo.GetVersionInfo(Assembly.GetExecutingAssembly().Location);

                return string.Format("{0}.{1}.{2}",
                    info.FileMajorPart,
                    info.FileMinorPart,
                    info.FileBuildPart);
            }
        }

        protected abstract string FriendlyName { get;}
        //protected abstract string Name { get;}

        public void Run()
        {
            BuildLog.WriteLine();
            BuildLog.Write("[Backend] {0} running ...", FriendlyName);

            try
            {
                //Run current Backend
                Generate();

                //Report OK
                BuildLog.Write("{0,30}", "[OK]");
            }
            catch (Exception e)
            {
                //Report OK
                BuildLog.Write("{0,30}", "[FAIL]");
                throw;
            }
        }

        protected abstract void Generate();
    }
}
