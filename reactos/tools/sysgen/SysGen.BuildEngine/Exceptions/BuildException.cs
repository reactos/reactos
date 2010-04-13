using System;
using System.Runtime.Serialization;

namespace SysGen.BuildEngine 
{
    /// <summary>
    /// Thrown whenever an error occurs during the build.
    /// </summary>
    [Serializable]
    public class BuildException : ApplicationException 
    {
        private Location _location = Location.UnknownLocation;

        /// <summary>
        /// Constructs a build exception with no descriptive information.
        /// </summary>
        public BuildException() : base() {
        }

        public BuildException(String message, params object[] args)
            : base(string.Format(message, args))
        {
        }

        /// <summary>
        /// Constructs an exception with a descriptive message.
        /// </summary>
        public BuildException(String message) : base(message) {
        }

        /// <summary>
        /// Constructs an exception with a descriptive message and an
        /// instance of the Exception that is the cause of the current Exception.
        /// </summary>
        public BuildException(Exception e, String message) : base(message, e) {
        }

        /// <summary>
        /// Constructs an exception with a descriptive message and an
        /// instance of the Exception that is the cause of the current Exception.
        /// </summary>
        public BuildException(Exception e, String message,params object[] args)
            : base(string.Format(message , args), e)
        {
        }

        /// <summary>
        /// Constructs an exception with a descriptive message and location
        /// in the build file that caused the exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        public BuildException(String message, Location location) : base(message) {
            _location = location;
        }

        /// <summary>
        /// Constructs an exception with the given descriptive message, the
        /// location in the build file and an instance of the Exception that
        /// is the cause of the current Exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="location">Location in the build file where the exception occured.</param>
        /// <param name="e">An instance of Exception that is the cause of the current Exception.</param>
        public BuildException(String message, Location location, Exception e) : base(message, e) {
            _location = location;
        }

        /// <summary>Initializes a new instance of the BuildException class with serialized data.</summary>
        public BuildException(SerializationInfo info, StreamingContext context) : base(info, context) {
            /*
            string fileName  = info.GetString("Location.FileName");
            int lineNumber   = info.GetInt32("Location.LineNumber");
            int columnNumber = info.GetInt32("Location.ColumnNumber");
            */
            _location = info.GetValue("Location", _location.GetType()) as Location;
        }

        /// <summary>Sets the SerializationInfo object with information about the exception.</summary>
        /// <param name="info">The object that holds the serialized object data. </param>
        /// <param name="context">The contextual information about the source or destination. </param>
        /// <remarks>For more information, see SerializationInfo in the Microsoft documentation.</remarks>
        public override void GetObjectData(SerializationInfo info, StreamingContext context) {
            base.GetObjectData(info, context);
            info.AddValue("Location", _location);      
        }

        public override string Message {
            get {
                string message = base.Message;

                // only include location string if not empty
                string locationString = _location.ToString();
                if (locationString != String.Empty) {
                    message = locationString + "\n " + message;
                }
                return message;
            }
        }
    }
}
