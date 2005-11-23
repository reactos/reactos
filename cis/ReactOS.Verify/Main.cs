using System;
using System.IO;
using System.Diagnostics;
using System.Configuration;
using System.Web.Mail;
using System.Collections;
using System.Collections.Specialized;

namespace ReactOS.Verify
{
	public class MainClass
	{
		/// <summary>
		/// Run the application.
		/// </summary>
		/// <param name="script">Script to run.</param>
		/// <param name="args">Arguments to pass to script.</param>
		/// <param name="workingDirectory">Working directory.</param>
		/// <param name="standardOutput">Receives standard output.</param>
		/// <param name="standardError">Receives standard error.</param>
		/// <returns>
		/// Exit code.
		/// </returns>
		private static int RunScript(string script,
		                             string args,
		                             string workingDirectory,
		                             StringDictionary environmentVarables,
		                             out string standardOutput,
		                             out string standardError)
		{
			ProcessStartInfo scriptProcessStartInfo = new ProcessStartInfo(script,
			                                                               args);
			scriptProcessStartInfo.CreateNoWindow = true;
			/*
			 * All standard streams must be redirected.
			 * Otherwise DuplicateHandle() will fail.
			 */
			scriptProcessStartInfo.RedirectStandardInput = true;
			scriptProcessStartInfo.RedirectStandardError = true;
			scriptProcessStartInfo.RedirectStandardOutput = true;
			scriptProcessStartInfo.UseShellExecute = false;
			scriptProcessStartInfo.WorkingDirectory = workingDirectory;
			if (environmentVarables != null)
			{
				foreach (DictionaryEntry de in environmentVarables)
					scriptProcessStartInfo.EnvironmentVariables.Add(de.Key as string, de.Value as string);
			}
			RedirectableProcess redirectableProcess = new RedirectableProcess(scriptProcessStartInfo);
			standardOutput = redirectableProcess.ProcessOutput;
			standardError = redirectableProcess.ProcessError;
			return redirectableProcess.ExitCode;
		}

		/// <summary>
		/// Retrieve value of configuration from configuration file.
		/// </summary>
		/// <param name="name">Name of configuration option.</param>
		/// <param name="defaultValue">
		/// Default value to be returned if the option does not exist.
		/// </param>
		/// <returns>
		/// Value of configuration option or null if the option does not
		/// exist and no default value is provided.
		/// </returns>
		private static string GetConfigurationOption(string name,
		                                             string defaultValue)
		{
			if (ConfigurationSettings.AppSettings[name] != null)
			{
				string s = ConfigurationSettings.AppSettings[name].Trim();
				return s.Equals(String.Empty) ? defaultValue : s;
			}
			else
				return defaultValue;
		}

		/// <summary>
		/// Send an email.
		/// </summary>
		/// <param name="subject">Subject of the email.</param>
		/// <param name="body">Content of the email.</param>
		private static void SendErrorMail(string subject, string body)
		{
			try
			{
				string smtpServer = GetConfigurationOption("smtpServer", "localhost");
				string email = GetConfigurationOption("errorEmail", null);
				if (email == null)
					return;
				MailMessage mm = new MailMessage();
				mm.Priority = MailPriority.Normal;
				mm.From = email;
				mm.To = email;
				mm.Subject = subject;
				mm.Body += body;
				mm.Body += "<br>";
				mm.BodyFormat = MailFormat.Html;
				SmtpMail.SmtpServer = smtpServer;
				SmtpMail.Send(mm);
			}
			catch (Exception ex)
			{
				Console.Error.WriteLine(ex.Message);
			}
		}

		/// <summary>
		/// Fail with an error message.
		/// </summary>
		/// <param name="text">Error message.</param>
		private static void Fail(string text)
		{
			Console.WriteLine(text);
		}
		
		/// <summary>
		/// Paths to fast disk for temporary files.
		/// </summary>
		private static string TemporaryFastDisk
		{
			get
			{
				string temporaryFastDisk = GetConfigurationOption("temporaryFastDisk", null);
				if (temporaryFastDisk == null || temporaryFastDisk.Trim().Equals(String.Empty))
					return null;
				return temporaryFastDisk.Trim();
			}
		}

		/// <summary>
		/// Paths to fast disk for intermediate files.
		/// </summary>
		private static string IntermediateFastDisk
		{
			get
			{
				string intermediateFastDisk = GetConfigurationOption("intermediateFastDisk", null);
				if (intermediateFastDisk == null || intermediateFastDisk.Trim().Equals(String.Empty))
					return null;
				return intermediateFastDisk.Trim();
			}
		}

		/// <summary>
		/// Paths to fast disk for output files.
		/// </summary>
		private static string OutputFastDisk
		{
			get
			{
				string outputFastDisk = GetConfigurationOption("outputFastDisk", null);
				if (outputFastDisk == null || outputFastDisk.Trim().Equals(String.Empty))
					return null;
				return outputFastDisk.Trim();
			}
		}

		/// <summary>
		/// Return collection of environment variables.
		/// </summary>
		/// <returns>Collection of environment variables or null if there is none.</returns>
		private static StringDictionary GetEnvironmentVarables()
		{
			StringDictionary environmentVarables = new StringDictionary();
			if (TemporaryFastDisk != null)
				environmentVarables.Add("ROS_TEMPORARY", TemporaryFastDisk);
			if (IntermediateFastDisk != null)
				environmentVarables.Add("ROS_INTERMEDIATE", IntermediateFastDisk);
			if (OutputFastDisk != null)
				environmentVarables.Add("ROS_OUTPUT", OutputFastDisk);
			return environmentVarables;
		}
		
		/// <summary>
		/// Run make.
		/// </summary>
		/// <param name="arguments">Arguments.</param>
		/// <param name="standardOutput">Receives standard output.</param>
		/// <param name="standardError">Receives standard error.</param>
		/// <returns>Make exit code.</returns>
		private static int RunMake(string arguments,
		                           out string standardOutput,
		                           out string standardError)
		{
			string make = "mingw32-make";
			string makeParameters = GetConfigurationOption("makeParameters", "");
			string reactosDirectory = Path.Combine(System.Environment.CurrentDirectory,
			                                       "reactos");
			return RunScript(make,
			                 makeParameters + " " + arguments,
			                 reactosDirectory,
			                 GetEnvironmentVarables(),
			                 out standardOutput,
			                 out standardError);
		}
		
		/// <summary>
		/// Verify a revision of ReactOS by building it all.
		/// </summary>
		private static int VerifyFull()
		{
			string standardOutput;
			string standardError;
			int exitCode = RunMake("bootcd",
			                       out standardOutput,
			                       out standardError);
			if (exitCode != 0)
			{
				Fail(String.Format("make bootcd failed: (error: {0}) {1}",
				                   standardError,
				                   standardOutput));
				return exitCode;
			}
			
			string reactosDirectory = Path.Combine(System.Environment.CurrentDirectory,
			                                       "reactos");
			string isoFilename = Path.Combine(reactosDirectory,
			                                  "ReactOS.iso");
			if (!File.Exists(isoFilename))
				Fail("make bootcd produced no ReactOS.iso");

			return exitCode;
		}

		/// <summary>
		/// Verify a revision of ReactOS by building parts.
		/// </summary>
		/// <param name="components">Comma separated list of components to build.</param>
		private static int VerifyPartial(string components)
		{
			string standardOutput;
			string standardError;
			string componentParameters = "\"" + components.Replace(",", "\" \"") + "\"";
			int exitCode = RunMake(componentParameters,
			                       out standardOutput,
			                       out standardError);
			if (exitCode != 0)
				Fail(String.Format("make failed for targets {0}: (error: {1}) {2}",
				                   componentParameters,
				                   standardError,
				                   standardOutput));
			return exitCode;
		}

		/// <summary>
		/// Verify a revision of ReactOS.
		/// </summary>
		/// <param name="args">Arguments from command line.</param>
		private static int Verify(string[] args)
		{
			if (args.Length > 0)
				return VerifyPartial(args[0]);
			else
				return VerifyFull();
		}

		/// <summary>
		/// Program entry point.
		/// </summary>
		/// <param name="args">Arguments from command line.</param>
		/// <remarks>
		/// If exit code is 0, then the commit was processed successfully.
		/// If exit code is 1, then the commit was not processed successfully.
		/// </remarks>
		public static void Main(string[] args)
		{
			try
			{
				System.Environment.ExitCode = Verify(args);
			}
			catch (Exception ex)
			{
				string text = String.Format("Exception: {0}", ex);
				SendErrorMail("ReactOS Verify Error", text);
				System.Environment.ExitCode = 1;
			}
		}
	}
}
