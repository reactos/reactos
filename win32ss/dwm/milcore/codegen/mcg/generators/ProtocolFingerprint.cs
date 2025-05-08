// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// Abstract: 
//      Generator for the protocol fingerprint.
// 
// Description:
//      This generator computes a fingerprint of the protocol description
//      that can be used to determine whether binaries using the UCE
//      protocol are compatible.
//
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Xml;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;
    using MS.Internal.MilCodeGen.Helpers;

    public class ProtocolFingerprint : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ProtocolFingerprint(ResourceModel rm) : base(rm) 
        {
            /* do nothing */
        }

        #endregion Constructors

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        #region Public Methods

        public override void Go()
        {
            //
            // The MIL and DWM SDK versions are automagically calculated from the
            // UCE and DWM/redirection protocol fingerprints and manually incremented
            // SDK revisions.
            // 

            uint uiDwmSdkVersion = 0, uiMilSdkVersion = 0;

            string generatedPath = 
                Path.Combine(
                    _resourceModel.OutputDirectory,
                    "src\\Graphics\\Include\\Generated"
                    );

            FileCodeSink cppFile = new FileCodeSink(generatedPath, "wgx_sdk_version.h");
            FileCodeSink csFile = new FileCodeSink(generatedPath, "wgx_sdk_version.cs");

            m_cpp = new StringCodeSink();
            m_cs = new StringCodeSink();


            //
            // Collect the commands from the resource model and start generation.
            //

            PaddedCommandCollection commands =
                new PaddedCommandCollection(_resourceModel);            

            // Contains list of Commands which contain security critical resources
            List<String> commandList = new List<String>();


            //
            // Calculate the fingerprints of UCE and DWM protocols. Do it in two
            // passes and re-initialize the not-so-random number generator so that
            // DWM protocol changes do not affect MIL (and vice versae).
            //

            unchecked
            {
                InitializeSeeds();

                uint uiDwmFingerprint = Seed;
                
                foreach (PaddedCommand command in commands.PaddedCommands)
                {
                    if (command.Domain == "Redirection" || command.Domain == "DWM")
                    {
                        uiDwmFingerprint = Ror(uiDwmFingerprint ^ GetCommandHash(command));
                    }
                }

                uiDwmSdkVersion = uiDwmFingerprint;
            }


            unchecked
            {
                InitializeSeeds();

                uint uiMilFingerprint = Seed;
                
                foreach (PaddedCommand command in commands.PaddedCommands)
                {
                    if (command.Domain != "Redirection" && command.Domain != "DWM")
                    {
                        uiMilFingerprint = Ror(uiMilFingerprint ^ GetCommandHash(command));
                    }
                }

                uiMilSdkVersion = uiMilFingerprint;
            }


            // 
            // Take into account the manually incremented SDK revisions
            //

            unchecked
            {
                XmlDocument document = LoadDocument("xml\\Version.xml", "xml\\Version.xsd");

                string milSdkVersion = document.SelectSingleNode("/MilVersion/MilSdkVersion").InnerText;
                string dwmSdkVersion = document.SelectSingleNode("/MilVersion/DwmSdkVersion").InnerText;
                
                uiMilSdkVersion = Ror(uiMilSdkVersion ^ (Convert.ToUInt32(milSdkVersion) * 0x13579BDF));
                uiDwmSdkVersion = Ror(uiDwmSdkVersion ^ (Convert.ToUInt32(dwmSdkVersion) * 0xFDB97531));
            }


            //
            // Serialize the C++ header and the C# files:
            //

            Helpers.Style.WriteFileHeader(cppFile);

            cppFile.WriteBlock(
                [[inline]]
                    // MIL protocol fingerprint
                    #define MIL_SDK_VERSION 0x[[uiMilSdkVersion.ToString("X")]]

                    // DWM protocol fingerprint
                    #define DWM_SDK_VERSION 0x[[uiDwmSdkVersion.ToString("X")]]

                [[/inline]]
                );

            csFile.WriteBlock(
                Helpers.ManagedStyle.WriteFileHeader("wgx_sdk_version.cs")
                );

            csFile.WriteBlock(
                [[inline]]
                    // This code is generated from mcg\generators\ProtocolFingerprint.cs

                    namespace MS.Internal.Composition
                    {
                        /// <summary>
                        /// This is the automatically generated part of the versioning class that
                        /// contains definitions of methods returning MIL and DWM SDK versions.
                        /// </summary>
                        internal static class Version
                        {
                            /// <summary>
                            /// Returns the MIL SDK version this binary has been compiled against
                            /// </summary>
                            internal static uint MilSdkVersion
                            {
                                get
                                {
                                    unchecked
                                    {
                                        return (uint)0x[[uiMilSdkVersion.ToString("X")]];
                                    }
                                }
                            }

                            /// <summary>
                            /// Returns the DWM SDK version this binary has been compiled against
                            /// </summary>
                            internal static uint DwmSdkVersion
                            {
                                get
                                {
                                    unchecked
                                    {
                                        return (uint)0x[[uiDwmSdkVersion.ToString("X")]];
                                    }
                                }
                            }
                        }
                    }

                [[/inline]]
                );

            cppFile.Dispose();
            csFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        // Creates a pseudo-random number generator and initializes with a known
        // seed. This will guarantee that subsequent codegen runs will give same
        // results. 
        private void InitializeSeeds()
        {
            unchecked
            {
                m_random = new Random((int)0xDEADF00D);
            }
        }

        // Returns the next value to be used to calculate hashes... We will use
        // them when processing the protocol description. Note that we could use
        // some constant seeds in GetCommandHash, but that could result in command
        // reordering not affecting the final hash value.
        private uint Seed
        {
            get
            {
                unchecked
                {
                    // Note that Random.Next returns a 31-bit value, but we want 32 bits...
                    return ((uint)m_random.Next() * (uint)m_random.Next());
                }
            }
        }

        // Rotates the given 32-bit number right by one bit.
        private uint Ror(uint hash)
        {
            unchecked
            {
                return (hash >> 1) | ((hash & 1) << 31);
            }
        }

        /// <summary>
        /// Given a command, returns a 32-bit unsigned integer value that might
        /// but does not have to be related to the structure of the command.
        /// The hash algorithm is written ad hoc and no guarantees are made as 
        /// of the probability of hitting a collision. Experiments show that it
        /// is good enough to detect changes in the Resource.xml file and thus
        /// to prevent use of mismatched binaries in test environment.
        /// </summary>
        private uint GetCommandHash(PaddedCommand command)
        {
            unchecked
            {
                uint uiCommandHash = Seed;
                
                Helpers.CodeGenHelpers.PaddedStructData paddedStruct = 
                    command.PaddedStructData;
                
                foreach (CodeGenHelpers.AlignmentEntry entry in paddedStruct.AlignmentEntries)
                {
                    //
                    // On ((X + Seed) * Seed): I'm trying to ensure that X affects
                    // all 32 bits of the currently computed hash. 
                    // 
                    // Using the totally arbitrary constants for the boolean values 
                    // is fair because I rotate the hash after every field has been 
                    // xor'ed-in.
                    //

                    uiCommandHash ^= 
                        (((uint)entry.Name.GetHashCode() + Seed) * Seed)
                        ^ (((uint)entry.Size + Seed) * Seed)
                        ^ (entry.IsAnimation ? (uint)0x84329345 : (uint)0x54392347)
                        ^ (entry.IsPad ? (uint)0xAD93409FD : (uint)0xDF90439DB)
                        ^ (entry.IsHandle ? (uint)0x12344321 : (uint)0x43211235)
                        ^ (((uint)entry.Offset + Seed) * Seed);

                    if (entry.Field != null)
                    {
                        uiCommandHash ^= 
                            ((uint)entry.Field.Type.Name.GetHashCode() + Seed) * Seed;
                    }

                    uiCommandHash = Ror(uiCommandHash);
                }

                uiCommandHash ^=
                    ((uint)command.CommandName.GetHashCode() + Seed) * Seed;
                
                return uiCommandHash;
            }
        }

        private static XmlDocument LoadDocument(string xmlFile, string schemaFile)
        {
            XmlDocument document = XmlLoader.Load(xmlFile, schemaFile);

            if (document == null)
            {
                throw new ApplicationException(String.Format("Could not load version file."));
            }

            return document;
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private StringCodeSink m_cpp;
        private StringCodeSink m_cs;
        private Random m_random;

        #endregion Private Fields
     }
}


