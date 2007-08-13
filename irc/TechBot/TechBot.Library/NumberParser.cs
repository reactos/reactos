using System;
using System.Globalization;

namespace TechBot.Library
{
	public class NumberParser
	{
		public bool Error = false;
		
		private const string SpecialHexCharacters = "ABCDEF";

		private static bool IsSpecialHexCharacter(char ch)
		{
			foreach (char specialChar in SpecialHexCharacters)
			{
				if (ch.ToString().ToUpper() == specialChar.ToString())
					return true;
			}
			return false;
		}

		private static bool HasSpecialHexCharacters(string s)
		{
			foreach (char ch in s)
			{
				if (IsSpecialHexCharacter(ch))
					return true;
			}
			return false;
		}
		
		public long Parse(string s)
		{
			try
			{
				Error = false;
				bool useHex = false;
				if (s.StartsWith("0x", StringComparison.InvariantCultureIgnoreCase))
				{
					s = s.Substring(2);
					useHex = true;
				}
				if (HasSpecialHexCharacters(s))
					useHex = true;
				if (useHex)
					return Int64.Parse(s,
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
