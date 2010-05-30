namespace SysGen.BuildEngine.Attributes 
{
    using System;
    using System.Reflection;

    /// <summary>
    /// Indicates that field should be able to be converted into a Boolean.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property	, Inherited=true)]
    public class BooleanValidatorAttribute : ValidatorAttribute 
    {
        public BooleanValidatorAttribute() 
        {
        }

        public override bool Validate(object value)
        {
            return SysGenConversion.ToBolean(value);
        }
    }
}