using System;
using System.ComponentModel;
using System.ServiceProcess;
using System.Configuration.Install;
using System.Collections.Generic;
using System.Text;

namespace TechBot
{
    [RunInstaller(true)]
    public class ProjectInstaller : Installer
    {
        public ProjectInstaller()
        {
            ServiceProcessInstaller spi = null;
            ServiceInstaller si = null;

            spi = new ServiceProcessInstaller();
            spi.Account = ServiceAccount.LocalSystem;
                
            si = new ServiceInstaller();
            si.ServiceName = "TechBot";
            si.StartType = ServiceStartMode.Automatic;

            Installers.AddRange(new Installer[] { spi, si });
        }
    }
}
