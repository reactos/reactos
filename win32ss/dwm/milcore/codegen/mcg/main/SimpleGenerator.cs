// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This is a simple CSP mades generator which loads its
//              resource model via the XmlSerializer.
//              

namespace MS.Internal.MilCodeGen.Main
{
    using System;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Xml;
    using System.Xml.Serialization;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;
    using Utilities;

    public abstract class GeneratorBase : GeneratorMethods
    {
        protected GeneratorBase(CG data)
        {
            CG = data;
        }

        public abstract void Go();

        protected readonly CG CG;
    }

    public class SimpleGenerator
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        private SimpleGenerator()
        {
        }
        #endregion Constructors


        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods
        //+-----------------------------------------------------------------------------
        //
        //  Member:    Main
        //
        //  Synopsis:  The Main method for the MilCodeGen project.
        //             Does the following:
        //
        //             1. Read the given XML file, to produce a resource model.
        //             2. For each subclass of "GeneratorBase", create an instance
        //                and call its "Go()" method.
        //
        //------------------------------------------------------------------------------

        public static void Main(string[] args)
        {
            CG data;

            ProcessArgs(args, out data);

            //
            // For each concrete subclass of "GeneratorBase":
            //

            foreach (Type type in Assembly.GetExecutingAssembly().GetTypes())
            {
                if (type.IsSubclassOf(typeof(GeneratorBase)) && !type.IsAbstract)
                {
                    //
                    // Create an instance of the class, and call its Go() method.
                    //

                    GeneratorBase generator = (GeneratorBase) Activator.CreateInstance(
                        type, 
                        new object[] {data}
                        );

                    Console.WriteLine("Calling {0}.Go():", type.Name);

                    generator.Go();
                }
            }
            Console.WriteLine("SimpleGenerator done.");
        }
        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Public Properties
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Public Events
        //
        //------------------------------------------------------


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private class Arguments 
        {
            [CommandLineArgumentAttribute(CommandLineArgumentType.Required)]
            public string xmlFile;

            [CommandLineArgumentAttribute(CommandLineArgumentType.Required)]
            public string dataType;

            [CommandLineArgumentAttribute(CommandLineArgumentType.Required)]
            public string outputDirectory;

            public bool disableSd;
            public bool help;
        }

        //+-----------------------------------------------------------------------------
        //
        //  Function:  ProcessArgs
        //
        //  Synopsis:  Process the command-line arguments.
        //
        //------------------------------------------------------------------------------

        private static void ProcessArgs(
            string [] args,
            out CG data
            )
        {
            Arguments parsedArgs = new Arguments();
            if (!Utilities.Utility.ParseCommandLineArguments(args, parsedArgs, ReportArgumentError))
            {
                // Should never get here - ReportArgumentError should throw an exception

                throw new ApplicationException("Internal error");
            }


            //
            // Handle "-h" option.
            //

            if (parsedArgs.help)
            {
                Usage();
                data = null;
                return;
            }


            //
            // Check that files exist
            //

            if (!File.Exists(parsedArgs.xmlFile))
            {
                throw new FileNotFoundException("XML file not found", parsedArgs.xmlFile);
            }

            //
            // Handle "-disableSd" option.
            // NOTE: Side-effect.
            //

            if (parsedArgs.disableSd)
            {
                FileCodeSink.DisableSd();
            }

            //
            // Load the XML file and create the resource model from it
            //

            // Create an instance of the XmlSerializer specifying type and namespace.
            XmlSerializer serializer = new XmlSerializer(Type.GetType(parsedArgs.dataType));

            // A FileStream is needed to read the XML document.
            using (FileStream fs = new FileStream(parsedArgs.xmlFile, FileMode.Open, FileAccess.Read))
            using (XmlReader reader = new XmlTextReader(fs))
            {
                data = (CG) serializer.Deserialize(reader);
            }

            data.OutputDirectory = parsedArgs.outputDirectory;
        }

        public static void ReportArgumentError(string message)
        {
            Usage();
            throw new ApplicationException(message);
        }

        //+-----------------------------------------------------------------------------
        //
        //  Function:  DisplayLogo
        //
        //------------------------------------------------------------------------------

        private static void DisplayLogo()
        {
            Console.WriteLine();
            Console.WriteLine("Avalon MilCodeGen source-simple code generator.");
            Console.WriteLine();
        }


        //+-----------------------------------------------------------------------------
        //
        //  Function:  Usage
        //
        //------------------------------------------------------------------------------

        private static void Usage()
        {
            DisplayLogo();

            // display usage
            Console.WriteLine(
                [[inline]]
                    Usage: -d:<file> -o:<dir> [-x:<file>] [-disableSD]

                        Mil Code Generation Utility

                    -d:<file>           An XML file describing the resources.
                    -o:<dir>            A target root directory for the generated files.  
                                        e.g. %SdxRoot%\wpf
                    -disableSD          Disables use of Source Depot to check out the generated files.

                    Only one file of each type can be specified.
                [[/inline]]
                );
        }
        #endregion Private Methods
    }
}




