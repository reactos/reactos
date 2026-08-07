// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

// 
// Description: This file contains ValidateValueCallback wrapped 
//

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Text;
    using System.Xml;
    using System.Collections;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class ManagedEnums : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public ManagedEnums(ResourceModel rm) : base(rm)
        {
        }
        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods


        /// <summary>
        ///     Controller method that generates each enumeration in the XML description.
        /// </summary>
        public override void Go()
        {
            foreach (McgEnum enumType in _resourceModel.AllEnums)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.ManagedClass, enumType))
                {
                    continue;
                }

                //
                // Generate enum values & validation code sinks
                //                    
                    
                StringCodeSink enumValuesCS = new StringCodeSink();
                StringCodeSink checkEnumValues = new StringCodeSink();

                bool first = true;
                foreach(McgEnumValue enumValue in enumType.AllValues)
                {
                    if (enumValue.UnmanagedOnly)
                    {
                        continue;
                    }

                    enumValuesCS.WriteBlock(
                        [[inline]]    
                            /// <summary>
                            [[Helpers.CodeGenHelpers.FormatComment(enumValue.Name + " - " + enumValue.Comment, 84, String.Empty, String.Empty, true)]]
                            /// </summary>
                            [[enumValue.Name]] = [[enumValue.Value]],
                        [[/inline]]
                    );

                    if (!first)
                    {
                        checkEnumValues.WriteBlock(" || ");
                    }
                    
                    checkEnumValues.Write(
                        [[inline]](value == [[enumType.ManagedName]].[[enumValue.Name]])[[/inline]]
                    );

                    first = false;
                }        


                //
                // Write the enum definition & validation files
                //                 
               
                WriteEnumDefinitionFile(enumType, enumValuesCS);
                
                WriteEnumValidationFile(enumType, checkEnumValues);
            }
        }        

        /// <summary>
        ///     Writes an enumerated type's header block and namespace block
        /// </summary>
        /// <param name="enumFile">
        ///     FileCodeSink to write the header to.
        /// </param>        
        /// <param name="enumType">
        ///     The class which contains all specifiers that were read in from the 
        ///     XML description.
        /// </param>
        /// <param name="fileName">
        ///     Name of file to include in header.
        /// </param>        
        void WriteEnumHeader(FileCodeSink enumFile, McgEnum enumType, string fileName)
        {                               
            // Write the header block
            enumFile.WriteBlock(
                [[inline]]
                    [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]
                [[/inline]]
            );

            // Write the using clauses
            foreach (string s in enumType.Namespaces)
            {
                enumFile.Write(
                    [[inline]]
                        using [[s]];
                    [[/inline]]
                ); 
            }            

            // Write the string resource declaration
            enumFile.Write(
                [[inline]]
                    #if PRESENTATION_CORE
                    using SR=MS.Internal.PresentationCore.SR;
                    using SRID=MS.Internal.PresentationCore.SRID;
                    #else
                    using SR=System.Windows.SR;
                    using SRID=System.Windows.SRID;
                    #endif
                [[/inline]]
            ); 

            // Open namespace this enum exists in            
            enumFile.Write(
                [[inline]]                        
                
                    namespace [[enumType.ManagedNamespace]]
                    {
                [[/inline]]
                );
        }

        /// <summary>
        ///     This method writes the file that contains the definition of this enum
        /// </summary>
        /// <param name="enumType">
        ///     The class which contains all specifiers that were read in from the 
        ///     XML description.
        /// </param>
        /// <param name="enumValuesCS">
        ///     StringCodeSink that contains a definition for each member of the enum
        /// </param>      
        void WriteEnumDefinitionFile(McgEnum enumType, StringCodeSink enumValuesCS)
        {
            string fullPath = Path.Combine(_resourceModel.OutputDirectory, enumType.ManagedDestinationDir);
            string fileName = enumType.Name + ".cs";

            using (FileCodeSink enumFile = new FileCodeSink(fullPath, fileName))
            {
                WriteEnumHeader(enumFile, enumType, fileName);                    

                // Write enum type definition
                enumFile.Write(
                    [[inline]]                        
                            /// <summary>
                            [[Helpers.CodeGenHelpers.FormatComment(enumType.Name + " - " + enumType.Comment, 84, String.Empty, String.Empty, true)]]
                            /// </summary>
                            public enum [[enumType.ManagedName]]
                            {
                                [[enumValuesCS]]
                            }   
                     [[/inline]]
                     );

                // Write closing brackets for namespace
                enumFile.WriteBlock(
                    [[inline]]  
                        }
                    [[/inline]] 
                    );
            }                
        }        

        /// <summary>
        ///     This method writes the enum validation file that contains CheckIfValid 
        ///     and Is<TypeName>Valid.
        /// </summary>
        /// <param name="enumType">
        ///     The class which contains all specifiers that were read in from the 
        ///     XML description.
        /// </param>
        /// <param name="checkEnumValues">
        ///     StringCodeSink contains a check against 'value' for each valid enumeration 
        ///     value.
        /// </param>        
        void WriteEnumValidationFile(McgEnum enumType, StringCodeSink checkEnumValues)
        {
            // The internal enum validation files exist in the shared managed directory so both
            // Core and Framework can access them
            string fullPath = Path.Combine(_resourceModel.OutputDirectory, enumType.ManagedSharedDestinationDir);
            // e.g., 'StretchValidation.cs'
            string fileName = enumType.ManagedName + "Validation.cs";

            using (FileCodeSink validationFile = new FileCodeSink(fullPath, fileName))
            {
                WriteEnumHeader(validationFile, enumType, fileName);
            
                validationFile.Write(                   
                    [[inline]] 
                            internal static partial class ValidateEnums
                            {
                                /// <summary>
                                ///     Returns whether or not an enumeration instance a valid value.
                                ///     This method is designed to be used with ValidateValueCallback, and thus
                                ///     matches it's prototype.
                                /// </summary>
                                /// <param name="valueObject">
                                ///     Enumeration value to validate.
                                /// </param>    
                                /// <returns> 'true' if the enumeration contains a valid value, 'false' otherwise. </returns>
                                public static bool Is[[enumType.ManagedName]]Valid(object valueObject)
                                {
                                    [[enumType.ManagedName]] value = ([[enumType.ManagedName]]) valueObject;
    
                                    return [[checkEnumValues]];
                                }                                
                            }
                        }
                    [[/inline]] 
                    );
            }
        }    
        
        #endregion Public Methods        
    }
}




