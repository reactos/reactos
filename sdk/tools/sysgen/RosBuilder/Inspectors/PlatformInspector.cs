using System;
using System.Windows.Forms;
using System.ComponentModel;
using System.Collections.Generic;
using System.Text;

using SysGen.RBuild.Framework;

namespace TriStateTreeViewDemo
{
    public class PlatformInspector
    {
        private RBuildPlatform m_Platform = null;

        public PlatformInspector(RBuildPlatform platform)
        {
            m_Platform = platform;
        }

        [Category("Info")]
        public string Name
        {
            get { return m_Platform.Name; }
            set { m_Platform.Name = value; }
        }

        [Category("Info")]
        public string Description
        {
            get { return m_Platform.Description; }
            set { m_Platform.Description = value; }
        }

        [Category("Applications")]
        [Description("The module to be used as a shell for the platform")]
        public string Shell
        {
            set
            {
                if (value != string.Empty)
                {
                    try
                    {
                        RBuildModule module = m_Platform.Modules.GetByName(value);

                        if (module == null)
                            throw new ArgumentException("Unknown '" + value + "' shell module");

                        if (module.Type != ModuleType.Win32CUI &&
                            module.Type != ModuleType.Win32GUI)
                            throw new ArgumentException("Only Win32 GUI and CUI applications can be set as shell");

                        /* set the shell to use */
                        m_Platform.Shell = module;
                    }
                    catch (ArgumentException e)
                    {
                        MessageBox.Show(e.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
            get 
            {
                if (m_Platform.Shell != null)
                    return m_Platform.Shell.Name;

                return string.Empty;
            }
        }

        [Category("Applications")]
        [Description("The module to be used as a screensaver for the platform")]
        public string Screensaver
        {
            set
            {
                if (value != string.Empty)
                {
                    try
                    {
                        RBuildModule module = m_Platform.Modules.GetByName(value);

                        if (module == null)
                            throw new ArgumentException("Unknown '" + value + "' screen saver module");

                        if (module.Type != ModuleType.Win32SCR)
                            throw new ArgumentException("Only Win32 SCR applications can be set as shell");

                        /* set the shell to use */
                        m_Platform.Screensaver = module;
                    }
                    catch (ArgumentException e)
                    {
                        MessageBox.Show(e.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
            get
            {
                if (m_Platform.Screensaver != null)
                    return m_Platform.Screensaver.Name;

                return string.Empty;
            }
        }

        //public IList<RBuildDebugChannel> DebugChannels
        //{
        //    get { return m_Platform.DebugChannels; }
        //}

        [Category("Appareance")]
        [Description("The module to be used as a screensaver for the platform")]
        public string Wallpaper
        {
            set
            {
                if (value != string.Empty)
                {
                    RBuildWallpaperFile iW = new RBuildWallpaperFile();

                    iW.Name = value;

                    m_Platform.Wallpaper = iW;

                    //foreach (RBuildModule module in m_Platform.Modules)
                    //{
                    //    foreach (RBuildFile file in module.Files)
                    //    {
                    //        RBuildInstallWallpaperFile wallpaper = file as RBuildInstallWallpaperFile;

                    //        if (wallpaper != null)
                    //        {
                    //            if (wallpaper.ID.ToLower() == value.ToLower())
                    //            {
                    //                m_Platform.Wallpaper = wallpaper;
                    //            }
                    //        }
                    //    }
                    //}

                    // specified wallpaper not found
                    //throw new ArgumentException();
                }
            }
            get
            {
                if (m_Platform.Wallpaper != null)
                    return m_Platform.Wallpaper.ID;

                return string.Empty;
            }
        }
    }
}
