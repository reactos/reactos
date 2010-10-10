using System;
using System.Reflection;

namespace SysGen.BuildEngine.Attributes 
{
    /// <summary>
    /// Indicates that field should be able to be converted into a Int32 within the given range.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, Inherited = true)]
    public class Int32ValidatorAttribute : ValidatorAttribute
    {
        int _minValue = Int32.MinValue;
        int _maxValue = Int32.MaxValue;

        public Int32ValidatorAttribute()
        {
        }

        public Int32ValidatorAttribute(int minValue, int maxValue)
        {
            MinValue = minValue;
            MaxValue = maxValue;
        }

        public int MinValue
        {
            get { return _minValue; }
            set { _minValue = value; }
        }

        public int MaxValue
        {
            get { return _maxValue; }
            set { _maxValue = value; }
        }

        public override bool Validate(object value)
        {
            try
            {
                Int32 intValue = Convert.ToInt32(value);
                if (intValue < MinValue || intValue > MaxValue)
                {
                    throw new ValidationException(String.Format("Cannot resolve '{0}' to integer between '{1}' and '{2}'.", value.ToString(), MinValue, MaxValue));
                }
            }
            catch (Exception)
            {
                throw new ValidationException(String.Format("Cannot resolve '{0}' to integer value.", value.ToString()));
            }
            return true;
        }
    }
}