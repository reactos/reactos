using System;
using System.Reflection;

namespace SysGen.BuildEngine.Attributes
{
    /// <summary>
    /// Indicates that field should be able to be converted into a Uri.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, Inherited = true)]
    public class UriValidatorAttribute : ValidatorAttribute
    {
        public override bool Validate(object value)
        {
            try
            {
                Uri uriValue = new Uri(value.ToString(), UriKind.Relative);
            }
            catch (Exception)
            {
                throw new ValidationException(String.Format("Cannot resolve '{0}' to Uri.", value.ToString()));
            }

            return true;
        }
    }
}