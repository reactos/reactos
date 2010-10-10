using System;
using System.Runtime.Serialization;

namespace SysGen.BuildEngine 
{
    /// <summary>
    /// This exception indicates that an error has occured while performing a validate operation. 
    /// The ValidationEventHandler can cause this exception to be thrown during the validate operations
    /// </summary>
    [Serializable]
    public class ValidationException : BuildException 
    {
        /// <summary>
        /// Constructs a build exception with no descriptive information.
        /// </summary>
        public ValidationException() : base() {}

        /// <summary>
        /// Constructs an exception with a descriptive message.
        /// </summary>
        public ValidationException(String message) : base(message) {}

        /// <summary>
        /// Constructs an exception with a descriptive message and an
        /// instance of the Exception that is the cause of the current Exception.
        /// </summary>
        public ValidationException(String message, Exception e) : base(e, message) {}

        /// <summary>
        /// Constructs an exception with a descriptive message and location
        /// in the build file that caused the exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        public ValidationException(String message, Location location) : base(message, location) {}

        /// <summary>
        /// Constructs an exception with the given descriptive message, the
        /// location in the build file and an instance of the Exception that
        /// is the cause of the current Exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        /// <param name="e">An instance of Exception that is the cause of the current Exception.</param>
        public ValidationException(String message, Location location, Exception e) : base(message, location, e) {}

        /// <summary>Initializes a new instance of the ValidationException class with serialized data.</summary>
        public ValidationException(SerializationInfo info, StreamingContext context) : base(info, context) {}

        /// <summary>Sets the SerializationInfo object with information about the exception.</summary>
        /// <param name="info">The object that holds the serialized object data. </param>
        /// <param name="context">The contextual information about the source or destination. </param>
        /// <remarks>For more information, see SerializationInfo in the Microsoft documentation.</remarks>
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {}

        public override string Message {
            get {
                return base.Message;
            }
        }
    }
}
