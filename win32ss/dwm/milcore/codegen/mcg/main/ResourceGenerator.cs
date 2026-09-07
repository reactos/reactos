// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate code for UCE resources.
//

namespace MS.Internal.MilCodeGen.Main
{
    using System;
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Collections;
    using System.Reflection;
    using System.CodeDom.Compiler;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;
    using Utilities;

    public abstract class GeneratorBase : GeneratorMethods
    {
        protected GeneratorBase(ResourceModel rm)
        {
            _resourceModel = rm;
        }

        public abstract void Go();

        protected ResourceModel _resourceModel;
    }

    public class ResourceGenerator
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        private ResourceGenerator()
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
            ResourceModel resourceModel;

            ProcessArgs(args, out resourceModel);

            if (resourceModel != null)
            {
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
                            new object[] {resourceModel}
                            );
    
                        Console.WriteLine("Calling {0}.Go():", type.Name);
    
                        generator.Go();
                    }
                }
                Console.WriteLine("MilCodeGen done.");
            }
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
            public string xsdFile;

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
            out ResourceModel resourceModel
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
                resourceModel = null;
                return;
            }


            //
            // Check that files exist
            //

            if (!File.Exists(parsedArgs.xmlFile))
            {
                throw new FileNotFoundException("XML file not found", parsedArgs.xmlFile);
            }

            if (!File.Exists(parsedArgs.xsdFile))
            {
                throw new FileNotFoundException("Schema not found", parsedArgs.xsdFile);
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

            XmlDocument document = LoadDocument(parsedArgs.xmlFile, parsedArgs.xsdFile);
            resourceModel = new ResourceModel(document, parsedArgs.outputDirectory);
        }

        public static void ReportArgumentError(string message)
        {
            Usage();
            throw new ApplicationException(message);
        }


        //+-----------------------------------------------------------------------------
        //
        //  Function:  LoadDocument
        //
        //------------------------------------------------------------------------------

        private static XmlDocument LoadDocument(string xmlFile, string schemaFile)
        {
            XmlDocument document = XmlLoader.Load(xmlFile, schemaFile);

            if (document == null)
            {
                throw new ApplicationException(String.Format("Could not load data file."));
            }

            return document;
        }


        //+-----------------------------------------------------------------------------
        //
        //  Function:  DisplayLogo
        //
        //------------------------------------------------------------------------------

        private static void DisplayLogo()
        {
            Console.WriteLine();
            Console.WriteLine("Avalon MilCodeGen source-code generator.");
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
                    -x:<file>           A schema file with which to validate the XML file.
                    -disableSD          Disables use of Source Depot to check out the generated files.

                    Only one file of each type can be specified.
                [[/inline]]
                );
        }
        #endregion Private Methods
    }
}




