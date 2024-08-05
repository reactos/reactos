using System;
using System.Collections.Generic;

namespace Lextm.SharpSnmpLib.Mib
{
	public class ValueMap : Dictionary<Int64, string>
	{
		public ValueMap()
		{
		}

		/// <summary>
		/// Returns the values of the map as continuous range. At best as one range.
		/// </summary>
		/// <returns></returns>
		public ValueRanges GetContinousRanges()
		{
			ValueRanges result = new ValueRanges();

			if (this.Count > 0)
			{
				List<Int64> values = new List<long>(this.Keys);
				values.Sort();

				Int64 last   = values[0];
				Int64 offset = values[0];
				for (int i=1; i<values.Count; i++)
				{
					if (values[i] != last + 1)
					{
						if (last == offset)
						{
							result.Add(new ValueRange(offset, null));
						}
						else
						{
							result.Add(new ValueRange(offset, last));
						}

						offset = values[i];
					}

					last = values[i];
				}

				if (last == offset)
				{
					result.Add(new ValueRange(offset, null));
				}
				else
				{
					result.Add(new ValueRange(offset, last));
				}
			}

			return result;
		}

		/// <summary>
		/// Gets the highest value contained in this value map.
		/// </summary>
		/// <returns></returns>
		public Int64 GetHighestValue()
		{
			Int64 result = 0;

			foreach (Int64 value in this.Keys)
			{
				if (value > result)
				{
					result = value;
				}
			}

			return result;
		}

		/// <summary>
		/// Interprets the single values as bit positions and creates a mask of it.
		/// </summary>
		/// <returns></returns>
		public UInt32 GetBitMask()
		{
			UInt32 result = 0;

			foreach (Int64 key in this.Keys)
			{
				if (key < 0)
				{
					throw new NotSupportedException("Negative numbers are not allowed for Bits!");
				}
				if (key > 31)
				{
					throw new NotSupportedException("Bits with more than 32 bits are not supported!");
				}

				result |= (UInt32)(1 << (int)key);
			}

			return result;
		}
	}
}
