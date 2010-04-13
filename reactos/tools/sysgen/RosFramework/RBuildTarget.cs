using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum TargetType
    {
        LiveCD,
        BootCD
    }

    public enum TargetDebugOutputType
    {
        COM1,
        COM2,
        Screen,
        Bochs
    }

    public enum TargetDebugType
    {
        None,
        Debug,
        KernelDebug
    }

    public enum TargetWindowsPlatformType
    {
        WindowsNT4 = 0x400,
        Windows2000 = 0x500,
        WindowsXP = 0x501,
        Windows2003 = 0x0502,
        WindowsVista = 0x600
    }

    public enum TargetWindowsSPType
    {
        NoServicePack,
        ServicePack1 = 0x100,
        ServicePack2 = 0x200,
        ServicePack3 = 0x300,
        ServicePack4 = 0x400,
        ServicePack5 = 0x500,
        ServicePack6 = 0x600,
        ServicePack7 = 0x700,
        ServicePack8 = 0x800,
        ServicePack9 = 0x900
    }

    public enum TargetPlatformType
    {
        NT4, /* Windows NT 4.0 */
        NT4_SP1,
        NT4_SP2,
        NT4_SP3,
        NT4_SP4,
        NT4_SP5,
        NT4_SP6,
        NT5, /* Windows 2000 */
        NT5_SP1,
        NT5_SP2,
        NT5_SP3,
        NT5_SP4,
        NT51, /* Windows XP */
        NT51_SP1,
        NT51_SP2,
        NT52, /* Windows 2003 */
        NT52_SP1,
        NT52_SP2,
        NT6 /* Windows Vista */
    }

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
        PPC
    }

    public enum TargetOptimizeLevelType
    {
        Level_0,
        Level_1,
        Level_2,
        Level_3,
        Level_4,
        Level_5
    }

    public class RBuildTarget
    {
        private string m_Name = null;

        private bool m_RegTest = false;
        private bool m_Multiprocessor = false;

        private TargetType m_Type = TargetType.BootCD;
        private TargetDebugType m_DebugType = TargetDebugType.None;
        private TargetPlatformType m_PlatformType = TargetPlatformType.NT5_SP4;
        private TargetDebugOutputType m_DebugOutputType = TargetDebugOutputType.COM1;
        private TargetArchitectureType m_ArchitectureType = TargetArchitectureType.X86_Pentium;
        private TargetOptimizeLevelType m_OptimizeLevelType = TargetOptimizeLevelType.Level_1;

        public RBuildTarget()
        {
        }

        public RBuildTarget(string name)
        {
            m_Name = name;
        }

        public RBuildTarget(string name, TargetType type)
        {
            m_Name = name;
            m_Type = type;
        }

        public RBuildTarget(string name, TargetType type, TargetDebugType debugType)
        {
            m_Name = name;
            m_Type = type;
            m_DebugType = debugType;
        }

        public string Name
        {
            get { return m_Name; }
            set { m_Name = value; }
        }

        public bool MultiProcessor
        {
            get { return m_Multiprocessor; }
            set { m_Multiprocessor = value; }
        }

        public bool RegressionTest
        {
            get { return m_RegTest; }
            set { m_RegTest = value; }
        }

        public bool Debug
        {
            get { return ((DebugType == TargetDebugType.Debug) || (DebugType == TargetDebugType.KernelDebug)); }
        }

        public bool KernelDebug
        {
            get { return (DebugType == TargetDebugType.KernelDebug); }
        }

        public TargetType Type
        {
            get { return m_Type; }
            set { m_Type = value; }
        }

        public TargetDebugType DebugType
        {
            get { return m_DebugType; }
            set { m_DebugType = value; }
        }

        public TargetDebugOutputType DebugOutputType
        {
            get { return m_DebugOutputType; }
            set { m_DebugOutputType = value; }
        }

        public TargetPlatformType PlatformType
        {
            get { return m_PlatformType; }
            set { m_PlatformType = value; }
        }

        public TargetArchitectureType ArchitectureType
        {
            get { return m_ArchitectureType; }
            set { m_ArchitectureType = value; }
        }

        public TargetOptimizeLevelType OptimizeType
        {
            get { return m_OptimizeLevelType; }
            set { m_OptimizeLevelType = value; }
        }

        public TargetWindowsSPType WindowsServicePack
        {
            get
            {
                switch (PlatformType)
                {
                    case TargetPlatformType.NT4:
                    case TargetPlatformType.NT5:
                    case TargetPlatformType.NT51:
                    case TargetPlatformType.NT52:
                    case TargetPlatformType.NT6:
                        return TargetWindowsSPType.NoServicePack;
                    case TargetPlatformType.NT4_SP1:
                    case TargetPlatformType.NT5_SP1:
                    case TargetPlatformType.NT51_SP1:
                    case TargetPlatformType.NT52_SP1:
                        return TargetWindowsSPType.ServicePack1;
                    case TargetPlatformType.NT4_SP2:
                    case TargetPlatformType.NT5_SP2:
                    case TargetPlatformType.NT51_SP2:
                    case TargetPlatformType.NT52_SP2:
                        return TargetWindowsSPType.ServicePack2;
                    case TargetPlatformType.NT4_SP3:
                    case TargetPlatformType.NT5_SP3:
                        return TargetWindowsSPType.ServicePack3;
                    case TargetPlatformType.NT4_SP4:
                    case TargetPlatformType.NT5_SP4:
                        return TargetWindowsSPType.ServicePack4;
                    case TargetPlatformType.NT4_SP5:
                        return TargetWindowsSPType.ServicePack5;
                    case TargetPlatformType.NT4_SP6:
                        return TargetWindowsSPType.ServicePack6;
                    default:
                        throw new Exception("");
                }
            }
        }

        public TargetWindowsPlatformType WindowsPlatfom
        {
            get
            {
                switch (PlatformType)
                {
                    case TargetPlatformType.NT4:
                    case TargetPlatformType.NT4_SP1:
                    case TargetPlatformType.NT4_SP2:
                    case TargetPlatformType.NT4_SP3:
                    case TargetPlatformType.NT4_SP4:
                    case TargetPlatformType.NT4_SP5:
                    case TargetPlatformType.NT4_SP6:
                        return TargetWindowsPlatformType.WindowsNT4;
                    case TargetPlatformType.NT5:
                    case TargetPlatformType.NT5_SP1:
                    case TargetPlatformType.NT5_SP2:
                    case TargetPlatformType.NT5_SP3:
                    case TargetPlatformType.NT5_SP4:
                        return TargetWindowsPlatformType.Windows2000;
                    case TargetPlatformType.NT51:
                    case TargetPlatformType.NT51_SP1:
                    case TargetPlatformType.NT51_SP2:
                        return TargetWindowsPlatformType.WindowsXP;
                    case TargetPlatformType.NT52:
                    case TargetPlatformType.NT52_SP1:
                    case TargetPlatformType.NT52_SP2:
                        return TargetWindowsPlatformType.Windows2003;
                    case TargetPlatformType.NT6:
                        return TargetWindowsPlatformType.WindowsVista;
                    default:
                        throw new Exception("");
                }
            }
        }
    }
}
