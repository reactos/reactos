namespace SysGen.BuildEngine.Attributes 
{
    using System;
    using System.Reflection;

    public abstract class ValidatorAttribute : Attribute 
    {
        /// <summary>
        /// Validates the object.
        /// </summary>
        /// <param name="value">The object to be validated</param>
        /// <remarks> Throws a ValidationException when validation fails.</remarks>
        /// <returns> Returns an indication of the result.</returns>
        public abstract bool Validate(object value);
    }
}