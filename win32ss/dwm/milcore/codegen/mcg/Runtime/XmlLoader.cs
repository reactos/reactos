// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Loads and validates an XML document.
//

namespace MS.Internal.MilCodeGen.Runtime
{
    using System;
    using System.CodeDom.Compiler;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;
    using System.Xml.Schema;
    using System.Xml.Serialization;

    public class XmlLoader
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public XmlLoader(string xmlFile, string schemaFile)
        {
            _xmlFile = xmlFile;
            _schemaFile = schemaFile;
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        static public XmlDocument Load(string xmlFile, string schemaFile)
        {
            XmlLoader loader = new XmlLoader(xmlFile, schemaFile);
            XmlReader reader = new XmlTextReader(File.OpenText(xmlFile));

            if(schemaFile != null && schemaFile.Length > 0)
            {
                XmlValidatingReader validatingReader = new XmlValidatingReader(reader);

                // Set a handler to handle validation errors.
                validatingReader.ValidationEventHandler += new ValidationEventHandler (loader.XmlValidationCallback);

                using (StreamReader sr = new StreamReader(schemaFile))
                {
                    XmlSchema schema = XmlSchema.Read(sr, new ValidationEventHandler (loader.SchemaValidationCallback));
                    validatingReader.Schemas.Add(schema);
                }

                // Upgrade our XmlTextReader to the new XmlValidatingReader
                reader = validatingReader;
            }

            XmlDocument doc = new XmlDocument();
            doc.Load(reader);

            return loader._success ? doc : null;
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

        private void SchemaValidationCallback(object sender, ValidationEventArgs args)
        {
            _success = false;

            Console.WriteLine(String.Format(
                "Error in {0}: {1}",
                _schemaFile,
                args.Message));
        }

        private void XmlValidationCallback(object sender, ValidationEventArgs args)
        {
            _success = false;

            Console.WriteLine(args.Message);
        }

        #endregion Private Methods

        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private bool _success = true;
        private string _xmlFile;
        private string _schemaFile;

        #endregion Private Fields
    }
}




