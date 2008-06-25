using System;
using System.Xml;
using System.Collections;

using TechBot.Library;

namespace TechBot.Commands.Common
{
    [Command("error", Help = "!error <value>")]
	public class ErrorCommand : Command
	{
		private NtStatusCommand ntStatus;
		private WinErrorCommand winerror;
		private HResultCommand hresult;

        public ErrorCommand()
        {
            this.ntStatus = new NtStatusCommand();
            this.winerror = new WinErrorCommand();
            this.hresult = new HResultCommand();
        }

		private static int GetSeverity(long error)
		{
			return (int)((error >> 30) & 0x3);
		}

		private static bool IsCustomer(long error)
		{
			return (error & 0x20000000) != 0;
		}
		
		private static bool IsReserved(long error)
		{
			return (error & 0x10000000) != 0;
		}

		private static int GetFacility(long error)
		{
			return (int)((error >> 16) & 0xFFF);
		}
		
		private static short GetCode(long error)
		{
			return (short)((error >> 0) & 0xFFFF);
		}

		private static string FormatSeverity(long error)
		{
			int severity = GetSeverity(error);
			switch (severity)
			{
			case 0: return "SUCCESS";
			case 1: return "INFORMATIONAL";
			case 2: return "WARNING";
			case 3: return "ERROR";
			}
			return null;
		}

		private static string FormatFacility(long error)
		{
			int facility = GetFacility(error);
			return facility.ToString();
		}

		private static string FormatCode(long error)
		{
			int code = GetCode(error);
			return code.ToString();
		}

		public override void  ExecuteCommand()
        {
            if (Parameters.Equals(String.Empty))
			{
				Say("Please provide an Error Code.");
				return;
			}
			
			string errorText = Parameters;

		retry:
			NumberParser np = new NumberParser();
			long error = np.Parse(errorText);
            if (np.Error)
            {
                Say("{0} is not a valid Error Code.", Parameters);
                return;
            }
			
			ArrayList descriptions = new ArrayList();

			// Error is out of bounds
			if ((ulong)error > uint.MaxValue)
			{
				// Do nothing
			}
			// Error is outside of the range [0, 65535]: it cannot be a plain Win32 error code
			else if ((ulong)error > ushort.MaxValue)
			{
				// Customer bit is set: custom error code
				if (IsCustomer(error))
				{
					string description = String.Format("[custom, severity {0}, facility {1}, code {2}]",
					                                   FormatSeverity(error),
					                                   FormatFacility(error),
					                                   FormatCode(error));
					descriptions.Add(description);
				}
				// Reserved bit is set: HRESULT_FROM_NT(ntstatus)
				else if (IsReserved(error))
				{
					int status = (int)(error & 0xCFFFFFFF);
					string description = ntStatus.GetNtstatusDescription(status);
					
					if (description == null)
						description = status.ToString("X");
					
					description = String.Format("HRESULT_FROM_NT({0})", description);
					descriptions.Add(description);
				}
				// Win32 facility: HRESULT_FROM_WIN32(winerror)
				else if (GetFacility(error) == 7)
				{
					// Must be an error code
					if (GetSeverity(error) == 2)
					{
						short err = GetCode(error);
						string description = winerror.GetWinerrorDescription(err);
						
						if (description == null)
							description = err.ToString("D");
						
						description = String.Format("HRESULT_FROM_WIN32({0})", description);
						descriptions.Add(description);
					}
				}
			}

			string winerrorDescription = winerror.GetWinerrorDescription(error);
			string ntstatusDescription = ntStatus.GetNtstatusDescription(error);
			string hresultDescription = hresult.GetHresultDescription(error);
			
			if (winerrorDescription != null)
				descriptions.Add(winerrorDescription);
			if (ntstatusDescription != null)
				descriptions.Add(ntstatusDescription);
			if (hresultDescription != null)
				descriptions.Add(hresultDescription);

			if (descriptions.Count == 0)
			{
				// Last chance heuristics: attempt to parse a 8-digit decimal as hexadecimal
				if (errorText.Length == 8)
				{
					errorText = "0x" + errorText;
					goto retry;
				}

                Say("I don't know about Error Code {0}.",
                                                      Parameters);
			}
            else if (descriptions.Count == 1)
            {
                string description = (string)descriptions[0];
                Say("{0} is {1}.",
                                                      Parameters,
                                                      description);
            }
            else
            {
                Say("{0} could be:", Parameters);

                foreach (string description in descriptions)
                    Say("\t{0}", description);
            }
		}
	}
}
