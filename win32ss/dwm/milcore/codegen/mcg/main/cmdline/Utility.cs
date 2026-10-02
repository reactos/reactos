// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


namespace Utilities
{
    using System;
    
    /// <summary>
    /// A delegate used in error reporting.
    /// </summary>
    public delegate void ErrorReporter(string message);

    /// <summary>
    /// Useful Stuff.
    /// </summary>
    public sealed class Utility
    {
        /// <summary>
        /// The System Defined new line string.
        /// </summary>
        public const string NewLine = "\r\n";
        
        /// <summary>
        /// Don't ever call this.
        /// </summary>
        private Utility() {}
        
        /// <summary>
        /// Parses Command Line Arguments. 
        /// Errors are output on Console.Error.
        /// Use CommandLineArgumentAttributes to control parsing behaviour.
        /// </summary>
        /// <param name="arguments"> The actual arguments. </param>
        /// <param name="destination"> The resulting parsed arguments. </param>
        /// <returns> true if no errors were detected. </returns>
        public static bool ParseCommandLineArguments(string [] arguments, object destination)
        {
            return ParseCommandLineArguments(arguments, destination, new ErrorReporter(Console.Error.WriteLine));
        }
        
        /// <summary>
        /// Parses Command Line Arguments. 
        /// Use CommandLineArgumentAttributes to control parsing behaviour.
        /// </summary>
        /// <param name="arguments"> The actual arguments. </param>
        /// <param name="destination"> The resulting parsed arguments. </param>
        /// <param name="reporter"> The destination for parse errors. </param>
        /// <returns> true if no errors were detected. </returns>
        public static bool ParseCommandLineArguments(string[] arguments, object destination, ErrorReporter reporter)
        {
            CommandLineArgumentParser parser = new CommandLineArgumentParser(destination.GetType(), reporter);
            return parser.Parse(arguments, destination);
        }
        /// <summary>
        /// Returns a Usage string for command line argument parsing.
        /// Use CommandLineArgumentAttributes to control parsing behaviour.
        /// </summary>
        /// <param name="argumentType"> The type of the arguments to display usage for. </param>
        /// <returns> Printable string containing a user friendly description of command line arguments. </returns>
        public static string CommandLineArgumentsUsage(Type argumentType)
        {
            return (new CommandLineArgumentParser(argumentType, null)).Usage;
        }
    }
}
