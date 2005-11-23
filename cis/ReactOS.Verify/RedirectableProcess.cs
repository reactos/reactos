/*
 * When using the ProcessStartInfo.RedirectStandardXxx properties there is a chance of
 * the parent and child process blocking due to a race condition. This class handles the
 * problem.
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/cpref/html/frlrfsystemdiagnosticsprocessstartinfoclassredirectstandardoutputtopic.asp
 */
using System;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace ReactOS.Verify
{
	/// <summary>
	/// Process that redirects standard output and standard error streams.
	/// </summary>
	public class RedirectableProcess
	{
		/// <summary>
		/// Process.
		/// </summary>
		private Process process;

		/// <summary>
		/// Redirected standard error stream.
		/// </summary>
		private string processError;

		/// <summary>
		/// Redirected standard output stream.
		/// </summary>
		private string processOutput;

		/// <summary>
		/// Exit code.
		/// </summary>
		private int exitCode;

		/// <summary>
		/// Redirected standard error stream.
		/// </summary>
		public string ProcessError
		{
			get
			{
				return processError;
			}
		}

		/// <summary>
		/// Redirected standard output stream.
		/// </summary>
		public string ProcessOutput
		{
			get
			{
				return processOutput;
			}
		}

		/// <summary>
		/// Exit code.
		/// </summary>
		public int ExitCode
		{
			get
			{
				return exitCode;
			}
		}

		/// <summary>
		/// Run an excutable and redirect standard error and/or standard output safely.
		/// </summary>
		public RedirectableProcess(ProcessStartInfo processStartInfo)
		{
			Run(processStartInfo, null);
		}

		/// <summary>
		/// Run an excutable and redirect standard error and/or standard output safely.
		/// </summary>
		public RedirectableProcess(ProcessStartInfo processStartInfo, string input)
		{
			Run(processStartInfo, input);
		}
		
		private void Run(ProcessStartInfo processStartInfo, string input)
		{
			process = new Process();
			process.StartInfo = processStartInfo;
			process.Start();
			if (processStartInfo.RedirectStandardInput && input != null)
			{
				process.StandardInput.AutoFlush = true;
				process.StandardInput.WriteLine(input);
			}
			Thread readStandardError = null;
			if (processStartInfo.RedirectStandardError)
			{
				readStandardError = new Thread(new ThreadStart(ReadStandardError));
				readStandardError.Start();
			}
			Thread readStandardOutput = null;
			if (processStartInfo.RedirectStandardOutput)
			{
				readStandardOutput = new Thread(new ThreadStart(ReadStandardOutput));
				readStandardOutput.Start();
			}
			if (processStartInfo.RedirectStandardError)
			{
				readStandardError.Join();
			}
			if (processStartInfo.RedirectStandardOutput)
			{
				readStandardOutput.Join();
			}
			process.WaitForExit();
			exitCode = process.ExitCode;
			process = null;
		}

		/// <summary>
		/// Read standard error thread entry-point.
		/// </summary>		
		private void ReadStandardError()
		{
			if (process != null)
			{
				processError = process.StandardError.ReadToEnd();
			}
		}

		/// <summary>
		/// Read standard output thread entry-point.
		/// </summary>		
		private void ReadStandardOutput()
		{
			if (process != null)
			{
				processOutput = process.StandardOutput.ReadToEnd();
			}
		}
	}
}
