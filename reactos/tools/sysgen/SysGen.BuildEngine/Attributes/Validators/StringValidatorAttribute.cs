using System;
using System.Reflection;

namespace SysGen.BuildEngine.Attributes
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, Inherited = true)]
    public class StringValidatorAttribute : ValidatorAttribute
    {
        private bool m_AllowEmpty = true;
        private bool m_AllowSpaces = true;

        public override bool Validate(object value)
        {
            if (!AllowSpaces && string.Equals(value, " "))
                throw new ValidationException("No spaces allowed");

            if (!AllowEmpty && value.ToString().Length == 0)
                throw new ValidationException("No empty string allowed");
        
            return true;
        }

        public bool AllowEmpty
        {
            get { return m_AllowEmpty; }
            set { m_AllowEmpty = value; }
        }

        public bool AllowSpaces
        {
            get { return m_AllowSpaces; }
            set { m_AllowSpaces = value; }
        }
    }
}