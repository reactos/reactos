using System;
using System.IO;
using System.Diagnostics;
using System.Configuration;
using System.Web.Mail;

namespace ReactOS.CustomRevisionAction
{
	public class MainClass
	{
		/// <summary>
		/// Path to store published binaries at.
		/// </summary>
		private static string publishPath;

		/// <summary>
		/// Whether or not to publish ISOs to a remote destination via FTP.
		/// </summary>
		private static bool PublishToRemoteFtpLocation
		{
			get
			{
				return publishPath.StartsWith("ftp://");
			}
		}

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
				return ConfigurationSettings.AppSettings[name];
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
				string toEmail = GetConfigurationOption("errorEmail", null);
				if (toEmail == null)
					return;
				string fromEmail = GetConfigurationOption("fromEmail", null);
				if (fromEmail == null)
					fromEmail = toEmail;
				MailMessage mm = new MailMessage();
				mm.Priority = MailPriority.Normal;
				mm.From = toEmail;
				mm.To = toEmail;
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
		/// <param name="revision">Repository revision.</param>
		/// <param name="text">Error message.</param>
		private static void Fail(int revision,
		                         string text)
		{
			Console.WriteLine(text);
			Console.Error.WriteLine(text);
			SendErrorMail(String.Format("[{0}] ReactOS Publish Error", revision), text);
		}

		/// <summary>
		/// Fail with an error message.
		/// </summary>
		/// <param name="text">Error message.</param>
		private static void Fail(string text)
		{
			Console.WriteLine(text);
			Console.Error.WriteLine(text);
			SendErrorMail("ReactOS Publish Error", text);
		}

		/// <summary>
		/// Generate filename of distribution.
		/// </summary>
		/// <param name="branch">Branch.</param>
		/// <param name="revision">Revision.</param>
		private static string GetDistributionFilename(string branch,
		                                              int revision)
		{
			return String.Format("ReactOS-{0}-r{1}.iso",
			                     branch,
			                     revision);
		}

		private static void SplitRemotePublishPath(string publishPath,
		                                           out string server,
		                                           out string directory)
		{
			string searchString = "://";
			int index = publishPath.IndexOf(searchString);
			if (index == -1)
				throw new InvalidOperationException();
			int endOfProtocolIndex = index + searchString.Length;
			string withoutProtocol = publishPath.Remove(0, endOfProtocolIndex);
			index = withoutProtocol.IndexOf("/");
			if (index == -1)
			{
				server = withoutProtocol;
				directory = "";
			}
			else
			{
				server = withoutProtocol.Substring(0, index);
				directory = withoutProtocol.Remove(0, index + 1);
			}
		}

		/// <summary>
		/// Copy ISO to the (remote) destination.
		/// </summary>
		/// <param name="sourceFilename">Name of source ISO file to copy.</param>
		/// <param name="branch">Branch.</param>
		/// <param name="revision">Revision.</param>
		/// <remarks>
		/// Structure is ftp://ftp.server.com/whereever/<branch>/ReactOS-<branch>-r<revision>.iso.
		/// </remarks>
		private static void CopyISOToRemoteFtpDestination(string sourceFilename,
		                                                  string branch,
		                                                  int revision)
		{
			string distributionFilename = GetDistributionFilename(branch,
			                                                      revision);
			string destinationFilename = Path.Combine(Path.GetDirectoryName(sourceFilename),
			                                          distributionFilename);
			File.Move(sourceFilename, destinationFilename);
			string server;
			string directory;
			SplitRemotePublishPath(publishPath, out server, out directory);
			FtpClient ftpClient = new FtpClient(server, "anonymous", "sin@svn.reactos.com");
			ftpClient.Login();
			if (directory != "")
				ftpClient.ChangeDir(directory);
			/* Create destination directory if it does not already exist */
			if (!ftpClient.DirectoryExists(branch))
				ftpClient.MakeDir(branch);
			ftpClient.ChangeDir(branch);
			ftpClient.Upload(destinationFilename);
			ftpClient.Close();
		}

		/// <summary>
		/// Copy ISO to the (local) destination.
		/// </summary>
		/// <param name="sourceFilename">Name of source ISO file to copy.</param>
		/// <param name="branch">Branch.</param>
		/// <param name="revision">Revision.</param>
		/// <remarks>
		/// Structure is <branch>\ReactOS-<branch>-r<revision>.iso.
		/// </remarks>
		private static void CopyISOToLocalDestination(string sourceFilename,
		                                              string branch,
		                                              int revision)
		{
			string distributionFilename = GetDistributionFilename(branch,
			                                                      revision);
			string destinationDirectory = Path.Combine(publishPath,
			                                           branch);
			string destinationFilename = Path.Combine(destinationDirectory,
			                                          distributionFilename);
			if (!Directory.Exists(destinationDirectory))
				Directory.CreateDirectory(destinationDirectory);
			File.Copy(sourceFilename,
			          destinationFilename);
		}

		/// <summary>
		/// Copy ISO to the destination.
		/// </summary>
		/// <param name="sourceFilename">Name of source ISO file to copy.</param>
		/// <param name="branch">Branch.</param>
		/// <param name="revision">Revision.</param>
		/// <remarks>
		/// Structure is <branch>\ReactOS-<branch>-r<revision>.iso.
		/// </remarks>
		private static void CopyISOToDestination(string sourceFilename,
		                                         string branch,
		                                         int revision)
		{
			if (PublishToRemoteFtpLocation)
				CopyISOToRemoteFtpDestination(sourceFilename, branch, revision);
			else
				CopyISOToLocalDestination(sourceFilename, branch, revision);
		}

		/// <summary>
		/// Publish a revision of ReactOS.
		/// </summary>
		/// <param name="text">Error message.</param>
		private static int Publish(string branch,
		                           int revision,
		                           string workingDirectory)
		{
			string make = "mingw32-make";
			string makeParameters = GetConfigurationOption("makeParameters", "");
			string reactosDirectory = Path.Combine(workingDirectory,
			                                       "reactos");
			Console.WriteLine(String.Format("ReactOS directory is {0}",
			                                reactosDirectory));
			string standardOutput;
			string standardError;
			int exitCode = RunScript(make,
			                         makeParameters + " bootcd",
			                         reactosDirectory,
			                         out standardOutput,
			                         out standardError);
			if (exitCode != 0)
			{
				Fail(revision,
				     String.Format("make bootcd failed: (error: {0}) {1}",
				                   standardError,
				                   standardOutput));
				return exitCode;
			}

			string sourceFilename = Path.Combine(reactosDirectory,
			                                     "ReactOS.iso");
			if (File.Exists(sourceFilename))
				CopyISOToDestination(sourceFilename,
				                     branch,
				                     revision);
			else
			{
				Fail(revision,
				     "make bootcd produced no ReactOS.iso");
				return exitCode;
			}

			return exitCode;
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
				System.Environment.ExitCode = 1;

				publishPath = ConfigurationSettings.AppSettings["publishPath"];
				if (publishPath == null)
				{
					Fail("PublishPath option not set.");
					return;
				}

				if (args.Length < 3)
				{
					Fail("Usage: ReactOS.CustomRevisionAction action branch revision");
					return;
				}

				string action = args[0]; /* bootcd */
				string branch = args[1];
				int revision = Int32.Parse(args[2]);
				
				System.Environment.ExitCode = Publish(branch,
				                                      revision,
				                                      System.Environment.CurrentDirectory);
			}
			catch (Exception ex)
			{
				Fail(String.Format("Exception: {0}", ex));
				System.Environment.ExitCode = 1;
			}
		}
	}
}
