namespace SysGen.BuildEngine.Attributes 
{
    using System;
    using System.Reflection;

    /// <summary>
    /// Indicates that the method should be exposed as a function in NAnt build 
    /// files.
    /// </summary>
    /// <remarks>
    /// Attach this attribute to a method of a class that derives from 
    /// <see cref="FunctionSetBase" /> to have NAnt be able to recognize it.
    /// </remarks>
    [AttributeUsage(AttributeTargets.Method, Inherited=false, AllowMultiple=false)]
    public sealed class FunctionAttribute : Attribute {
        #region Public Instance Constructors

        /// <summary>
        /// Initializes a new instance of the <see cref="FunctionAttribute" />
        /// class with the specified name.
        /// </summary>
        /// <param name="name">The name of the function.</param>
        /// <exception cref="ArgumentNullException"><paramref name="name" /> is <see langword="null" />.</exception>
        /// <exception cref="ArgumentOutOfRangeException"><paramref name="name" /> is a zero-length <see cref="string" />.</exception>
        public FunctionAttribute(string name) {
            if (name == null) {
                throw new ArgumentNullException("name");
            }

            if (name.Trim().Length == 0) {
                throw new ArgumentOutOfRangeException("name", name, "A zero-length string is not an allowed value.");
            }

            _name = name;
        }

        #endregion Public Instance Constructors
         
        #region Public Instance Properties

        /// <summary>
        /// Gets or sets the name of the function.
        /// </summary>
        /// <value>
        /// The name of the function.
        /// </value>
        public string Name {
            get { return _name; }
            set { _name = value; }
        }

        #endregion Public Instance Properties

        #region Private Instance Fields

        private string _name;
        
        #endregion Private Instance Fields
    }
}
