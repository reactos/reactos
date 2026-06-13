// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      ResourceFactory.cs
//
// Abstract: 
//      Generator for resource factory class.
// 
// Description:
//      This generator builds the CResourceFactory class (as found 
//      in %SDXROOT%\wpf\src\Graphics\core\uce\generated\resource-factory.h
//      and %SDXROOT%\wpf\src\Graphics\core\uce\generated\resource-factory.cpp).
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

    public class ResourceFactory : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ResourceFactory(ResourceModel rm) : base(rm) 
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
            // Opened the generated files
            //
            
            string generatedPath = 
                Path.Combine(
                    _resourceModel.OutputDirectory,
                    "src\\Graphics\\core\\uce"
                    );

            FileCodeSink chFile = 
                new FileCodeSink(generatedPath, "generated_resource_factory.h");

            FileCodeSink ccFile = 
                new FileCodeSink(generatedPath, "generated_resource_factory.cpp");

            m_factory = new StringCodeSink();


            //
            // Collect the resource types...
            // 

            foreach (McgResource resource in _resourceModel.Resources) 
            {
                if (resource.IsValueType
                    || resource.IsAbstract
                    || !resource.HasUnmanagedResource) 
                {
                    //
                    // Do not allow creation of value types (Int32 etc.) or abstract resources.
                    //

                    continue;
                }

                EmitResourceCreator(resource);
            }


            //
            // Serialize the C++ file containing the resource factory implementation
            //

            Helpers.Style.WriteFileHeader(ccFile);

            ccFile.WriteBlock(
                [[inline]]
                    #include "precomp.hpp"
                    
                    /// <summary>
                    ///   Given a composition object and a resource type, creates
                    ///   a new empty resource object.
                    /// </summary>
                    HRESULT CResourceFactory::Create(
                        __in_ecount(1) CComposition* pComposition,
                        __in_ecount(1) CMilSlaveHandleTable *pHTable,                        
                        MIL_RESOURCE_TYPE type,
                        __deref_out_ecount(1) CMilSlaveResource** ppResource
                        )
                    {
                        HRESULT hr = S_OK;

                        CMilSlaveResource* pResource = NULL;

                        switch (type) 
                        {
                        [[m_factory.ToString()]]
                        default:
                            RIP("Invalid resource type.");
                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                        }

                        IFCOOM(pResource);

                        pResource->AddRef();

                        *ppResource = pResource;

                    Cleanup:
                        RRETURN(hr);
                    }

                [[/inline]]);


            //
            // Serialize the C++ header containing the resource factory implementation
            //

            Helpers.Style.WriteFileHeader(chFile);

            chFile.WriteBlock(
                [[inline]]
                    /// <summary>
                    ///   MILCE resource object factory
                    /// </summary>
                    class CResourceFactory
                    {
                    private:
                        /// <remarks>
                        ///   This is a static class.
                        /// </remarks>
                        virtual ~CResourceFactory() { }

                    public:
                        /// <summary>
                        ///   Given a composition object and a resource type, creates
                        ///   a new empty resource object.
                        /// </summary>
                        static HRESULT Create(
                            __in_ecount(1) CComposition* pComposition,
                            __in_ecount(1) CMilSlaveHandleTable *pHTable,
                            MIL_RESOURCE_TYPE type,
                            __deref_out_ecount(1) CMilSlaveResource** ppResource
                            );
                    };
                [[/inline]]);

            ccFile.Dispose();
            chFile.Dispose();
        }

        #endregion Public Methods


        //------------------------------------------------------
        //
        //  Private Methods
        //
        //------------------------------------------------------

        #region Private Methods

        private void EmitResourceCreator(McgResource resource)
        {
            //
            // Emit a part of the enumeration...
            //
            if (resource.CanIntroduceCycles)
            {
                m_factory.Write(
                    [[inline]]
                        case TYPE_[[resource.Name.ToUpper()]]:
                            pResource = new [[resource.DuceClass]](pComposition, pHTable);
                            break;

                    [[/inline]]
                    );
            }
            else
            {
                m_factory.Write(
                    [[inline]]
                        case TYPE_[[resource.Name.ToUpper()]]:
                            pResource = new [[resource.DuceClass]](pComposition);
                            break;

                    [[/inline]]
                    );
            }
        }

        #endregion Private Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields

        private StringCodeSink m_factory;

        #endregion Private Fields
    }
}



