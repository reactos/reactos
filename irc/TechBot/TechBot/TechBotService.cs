using System;
using System.Threading;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.ServiceProcess;
using System.Configuration.Install;

namespace TechBot
{
	public class TechBotService : System.ServiceProcess.ServiceBase
	{
		private Thread thread;
		private ServiceThread threadWorker;
		
		public TechBotService()
		{
			InitializeComponents();
		}

		private void InitializeComponents()
		{
			this.ServiceName = "TechBot";
		}
		
		/// <summary>
		/// This method starts the service.
		/// </summary>
		public static void Main()
		{
			System.ServiceProcess.ServiceBase.Run(new System.ServiceProcess.ServiceBase[] {
				new TechBotService() // To run more than one service you have to add them here
			});
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose(bool disposing)
		{
			base.Dispose(disposing);
		}

		/// <summary>
		/// Start this service.
		/// </summary>
		protected override void OnStart(string[] args)
		{
			try
			{
				threadWorker = new ServiceThread(EventLog);
				thread = new Thread(new ThreadStart(threadWorker.Start));
				thread.Start();
				EventLog.WriteEntry(String.Format("TechBot service is running."));
			}
			catch (Exception ex)
			{
				EventLog.WriteEntry(String.Format("Ex. {0}", ex));
			}
		}
 
		/// <summary>
		/// Stop this service.
		/// </summary>
		protected override void OnStop()
		{
			try
			{
				thread.Abort();
				thread.Join();
				thread = null;
				threadWorker = null;
				EventLog.WriteEntry(String.Format("TechBot service is stopped."));
			}
			catch (Exception ex)
			{
				EventLog.WriteEntry(String.Format("Ex. {0}", ex));
			}
		}
	}
}

[RunInstaller(true)]
public class ProjectInstaller : Installer
{
	public ProjectInstaller()
	{
		ServiceProcessInstaller spi = new ServiceProcessInstaller();
		spi.Account = ServiceAccount.LocalSystem;
		
		ServiceInstaller si = new ServiceInstaller();
		si.ServiceName = "TechBot";
		si.StartType = ServiceStartMode.Automatic;
		Installers.AddRange(new Installer[] {spi, si});
	}
}
