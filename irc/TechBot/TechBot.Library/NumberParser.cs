using System;
using System.Globalization;

namespace TechBot.Library
{
	public class NumberParser
	{
		public bool Error = false;
		
		public long Parse(string s)
		{
			try
			{
				Error = false;
				if (s.StartsWith("0x"))
					return Int64.Parse(s.Substring(2),
					                   NumberStyles.HexNumber);
				else
					return Int64.Parse(s);
			}
			catch (FormatException)
			{
				Error = true;
			}
			catch (OverflowException)
			{
				Error = true;
			}
			return -1;
		}
	}
}
