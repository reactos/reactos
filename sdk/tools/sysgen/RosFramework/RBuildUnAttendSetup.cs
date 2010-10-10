using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

namespace SysGen.RBuild.Framework
{
    public enum InstallType : int
    {
        SkipMBRInstall = 0,
        FloppyMBRInstall = 1,
        HDDMBRInstall = 2
    }

    public class RBuildUnAttendSetup
    {
        private InstallType m_InstallType = InstallType.HDDMBRInstall;
        private int m_DestinationDiskNumber = 0;
        private int m_DestinationPartitionNumber = 1;
        private bool m_FormatPartition;
        private bool m_AutoPartition;
        private bool m_DisableVmwDriverInstall;
        private bool m_Enabled = false;
        private string m_InstallDirectory;
        private string m_FullName;
        private string m_OrgName;
        private string m_ComputerName;
        private string m_AdminPassword;
        
        public InstallType InstallType
        {
            get { return m_InstallType; }
            set { m_InstallType = value; }
        }

        public int DestinationDiskNumber
        {
            get { return m_DestinationDiskNumber; }
            set { m_DestinationDiskNumber = value; }
        }

        public int DestinationPartitionNumber
        {
            get { return m_DestinationPartitionNumber; }
            set { m_DestinationPartitionNumber = value; }
        }

        public string InstallDirectory
        {
            get { return m_InstallDirectory; }
            set { m_InstallDirectory = value; }
        }

        public string FullName
        {
            get { return m_FullName; }
            set { m_FullName = value; }
        }

        public string OrgName
        {
            get { return m_OrgName; }
            set { m_OrgName = value; }
        }

        public string ComputerName
        {
            get { return m_ComputerName; }
            set { m_ComputerName = value; }
        }

        public string AdminPassword
        {
            get { return m_AdminPassword; }
            set { m_AdminPassword = value; }
        }

        public bool FormatPartition
        {
            get { return m_FormatPartition; }
            set { m_FormatPartition = value; }
        }

        public bool AutoPartition
        {
            get { return m_AutoPartition; }
            set { m_AutoPartition = value; }
        }

        public bool DisableVmwDriverInstall
        {
            get { return m_DisableVmwDriverInstall; }
            set { m_DisableVmwDriverInstall = value; }
        }

        public bool Enabled
        {
            get { return m_Enabled; }
            set { m_Enabled = value; }
        }
    }
}
