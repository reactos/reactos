using System;
using System.Xml;
using System.Collections;

namespace TechBot.Library
{
	public class ErrorCommand : Command
	{
		private NtStatusCommand ntStatus;
		private WinerrorCommand winerror;
		private HResultCommand hresult;

        public ErrorCommand(TechBotService techBot)
            : base(techBot)
		{
			this.ntStatus = new NtStatusCommand(techBot);
			this.winerror = new WinerrorCommand(techBot);
			this.hresult = new HResultCommand(techBot);
		}
		
        /*
		public bool CanHandle(string commandName)
		{
			return CanHandle(commandName,
			                 new string[] { "error" });
		}
        */

        public override string[] AvailableCommands
        {
            get { return new string[] { "error" }; }
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

		public override  void Handle(MessageContext context,
		                   string commandName,
		                   string parameters)
		{
			string originalErrorText = parameters.Trim();
			if (originalErrorText.Equals(String.Empty))
			{
				TechBot.ServiceOutput.WriteLine(context,
				                        "Please provide an Error Code.");
				return;
			}
			
			string errorText = originalErrorText;

		retry:
			NumberParser np = new NumberParser();
			long error = np.Parse(errorText);
			if (np.Error)
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} is not a valid Error Code.",
													  originalErrorText));
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

                TechBot.ServiceOutput.WriteLine(context,
										String.Format("I don't know about Error Code {0}.",
													  originalErrorText));
			}
			else if (descriptions.Count == 1)
			{
				string description = (string)descriptions[0];
                TechBot.ServiceOutput.WriteLine(context,
										String.Format("{0} is {1}.",
													  originalErrorText,
													  description));
			}
			else
			{
                TechBot.ServiceOutput.WriteLine(context,
				                        String.Format("{0} could be:",
				                                      originalErrorText));
				
				foreach(string description in descriptions)
                    TechBot.ServiceOutput.WriteLine(context, String.Format("\t{0}", description));
			}
		}

        public override string Help()
		{
			return "!error <value>";
		}
	}
}
