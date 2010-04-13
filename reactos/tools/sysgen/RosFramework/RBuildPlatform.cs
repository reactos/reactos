using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum OptimizeLevelType : int
    {
        Level_0 = 0,
        Level_1 = 1,
        Level_2 = 2,
        Level_3 = 3,
        Level_4 = 4,
        Level_5 = 5
    }

    public class RBuildPlatform
    {
        private string m_Name = "Unamed Platform";
        private string m_Description = "This Platform has not yet a description";
        private bool m_Debug = true;
        private bool m_KDebug = true;
        private bool m_GDB = false;
        private bool m_NSWPAT = false;
        private bool m_WINKD = false;
        private OptimizeLevelType m_OptimizeLevelType = OptimizeLevelType.Level_1;
        private RBuildModule m_ShellModule = null;
        private RBuildModule m_ScreenSaverModule = null;
        private RBuildLanguage m_Language = null;
        private RBuildWallpaperFile m_Wallpaper = null;
        private RBuildModuleCollection m_Modules = new RBuildModuleCollection();
        private RBuildModuleCollection m_Autorun = new RBuildModuleCollection();
        private RBuildLanguageCollection m_Languages = new RBuildLanguageCollection();
        private RBuildDebugChannelCollection m_DebugChannels = new RBuildDebugChannelCollection();

        public RBuildPlatform()
        {
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public string Description
        {
            get { return m_Description; }
            set { m_Description = value; }
        }

        public RBuildDebugChannelCollection DebugChannels
        {
            get { return m_DebugChannels; }
        }

        public RBuildModule Shell
        {
            get { return m_ShellModule; }
            set { m_ShellModule = value; }
        }

        public RBuildModule Screensaver
        {
            get { return m_ScreenSaverModule; }
            set { m_ScreenSaverModule = value; }
        }

        public RBuildLanguage Language
        {
            get { return m_Language; }
            set { m_Language = value; }
        }

        public RBuildWallpaperFile Wallpaper
        {
            get { return m_Wallpaper; }
            set { m_Wallpaper = value; }
        }

        public RBuildModuleCollection Modules
        {
            get { return m_Modules; }
        }

        public RBuildModuleCollection AutorunModules
        {
            get { return m_Autorun; }
        }

        public RBuildLanguageCollection Languages
        {
            get { return m_Languages; }
        }

        public bool Debug
        {
            get { return m_Debug; }
            set { m_Debug = value; }
        }

        public bool KDebug
        {
            get { return m_KDebug; }
            set { m_KDebug = value; }
        }

        public bool GDB
        {
            get { return m_GDB; }
            set { m_GDB = value; }
        }

        public bool NSWPAT
        {
            get { return m_NSWPAT; }
            set { m_NSWPAT = value; }
        }

        public bool WINKD
        {
            get { return m_WINKD; }
            set { m_WINKD = value; }
        }

        public OptimizeLevelType OptimizeLevel
        {
            get { return m_OptimizeLevelType; }
            set { m_OptimizeLevelType = value; }
        }
    }
}