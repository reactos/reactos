// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Abstract:
//      Generate files for MILCE resources.
//
//---------------------------------------------------------------------------

namespace MS.Internal.MilCodeGen.Generators
{
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Text;
    using System.Xml;
    using System.Collections;
    using System.Collections.Specialized;

    using MS.Internal.MilCodeGen;
    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public class DuceResource : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors
        public DuceResource(ResourceModel rm) : base(rm)
        {
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
            FileCodeSink marshalFile = new FileCodeSink(_resourceModel.OutputDirectory, "src\\Graphics\\core\\resources\\marshal_generated.cpp");
            Helpers.Style.WriteFileHeader(marshalFile);
            Helpers.Style.WriteIncludePrecomp(marshalFile);

            // Write common routine to unmarshall an array of resources
            marshalFile.WriteBlock(
                [[inline]]
                    template <class TResourceType>
                    inline
                    HRESULT
                    UnmarshalResourceArray(
                        __inout_pcount_in_bcount(1, cbRawArray) BYTE const *&pbRawArray,
                        __inout_ecount(1) UINT32 &cbRawArray,
                        UINT32 cbReqArray,
                        MIL_RESOURCE_TYPE resType,
                        PERFMETERTAG mt,
                        __out_ecount(1) UINT32 &cResources,
                        __out_pcount_out_out_ecount_full(1,cResources) TResourceType ** &rgpResources,
                        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
                        bool fAllowNullHandles = true
                        )
                    {
                        //
                        // Ensure that the specified type inherits from CMilSlaveResource. The
                        // following line doesn't generate any code in debug or retail but it
                        // emits a compile-time error if TResourceType doesn't inherit from
                        // CMilSlaveResource.
                        //
                        static_cast<CMilSlaveResource *>(static_cast<TResourceType *>(NULL));

                        CMilSlaveResource **rgpResourcesTemp;

                        HRESULT hr = UnmarshalResourceArray(
                            pbRawArray,
                            cbRawArray,
                            cbReqArray,
                            resType,
                            mt,
                            cResources,
                            rgpResourcesTemp,
                            pHandleTable,
                            fAllowNullHandles
                            );

                        rgpResources = reinterpret_cast<TResourceType **>(rgpResourcesTemp);

                        return hr;
                    }

                    HRESULT
                    UnmarshalResourceArray(
                        __inout_pcount_in_bcount(1, cbRawArray) BYTE const *&pbRawArray,
                        __inout_ecount(1) UINT32 &cbRawArray,
                        UINT32 cbReqArray,
                        MIL_RESOURCE_TYPE resType,
                        PERFMETERTAG mt,
                        __out_ecount(1) UINT32 &cResources,
                        __out_pcount_out_out_ecount_full(1,cResources) CMilSlaveResource ** &rgpResources,
                        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
                        bool fAllowNullHandles = true
                        )
                    {
                        HRESULT hr = S_OK;

                        //
                        // Initialize out parameters
                        //

                        cResources = 0;
                        rgpResources = NULL;

                        //
                        // Make sure there is enough buffer and that the manifested resource
                        // table size is divisible by the handle size.
                        //

                        if (cbReqArray > cbRawArray
                            || cbReqArray % sizeof([[DuceHandle.UnmanagedTypeName]]) != 0)
                        {
                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                        }

                        //
                        // Compute resource count from expected array byte size
                        //

                        cResources = cbReqArray / sizeof([[DuceHandle.UnmanagedTypeName]]);

                        if (cResources > 0)
                        {
                            UINT const uCount = cResources;

                            //
                            // Allocate pointer buffer. We'll use this buffer to translate resource
                            // handles to pointers to resources.
                            //

                            IFC(HrMalloc(
                                mt,
                                sizeof(*rgpResources),
                                cResources,
                                (void **)&rgpResources
                                ));


                            //
                            // Copy data that contains only handles; collection resource
                            //

                            [[DuceHandle.UnmanagedTypeName]] const *phResourceHandles = reinterpret_cast<[[DuceHandle.UnmanagedTypeName]] const *>(pbRawArray);
                            CMilSlaveResource **ppResourcePointers = rgpResources;
                            for (UINT i = 0; i < uCount; i++)
                            {
                                HMIL_RESOURCE hResource = *phResourceHandles++;
                                CMilSlaveResource *pResource = NULL;

                                //
                                // If the handle is non-NULL we need to validate it
                                // against the handle table. Otherwise, we check whether
                                // this resource allows NULL pointers (based on fAllowNullHandles)
                                // and, if so, the pointer is implicitly NULL
                                //

                                if (hResource)
                                {
                                    pResource = pHandleTable->GetResource(hResource, resType);
                                    IFCNULL(pResource);
                                }
                                else if (!fAllowNullHandles)
                                {
                                    IFCNULL(hResource);
                                }

                                *ppResourcePointers++ = pResource;
                            }


                            //
                            // Update data pointer and size for any remaining data.
                            //
                            // Note that at the moment of writing this coment, the following operations
                            // are guaranteed to succeed. This follows from the fact that the batch size
                            // (and, hence, the maximum size of the resource array) is a 32-bit value.
                            //
                            // Adding these checks as a security-in-depth practice.
                            //

                            UINT_PTR cbUnmarshaled;
                            UINT_PTR cbRawArrayFixed;

                            IFC(UIntPtrSub(reinterpret_cast<UINT_PTR>(phResourceHandles), reinterpret_cast<UINT_PTR>(pbRawArray), &cbUnmarshaled));
                            IFC(UIntPtrSub(static_cast<UINT_PTR>(cbRawArray), cbUnmarshaled, &cbRawArrayFixed));
                            IFC(UIntPtrToUInt(cbRawArrayFixed, &cbRawArray));

                            pbRawArray = reinterpret_cast<BYTE const *>(phResourceHandles);
                        }

                    Cleanup:
                        if (FAILED(hr))
                        {
                            //
                            // We have failed to unmarshal the array -- perform the clean up here
                            // or otherwise we're risking problems during while unregistering.
                            //

                            if (rgpResources != NULL)
                            {
                                WPFFree(ProcessHeap, rgpResources);
                                rgpResources = NULL;
                            }

                            cResources = 0;
                        }

                        RRETURN(hr);
                    }
                [[/inline]]
                );

            FileCodeSink dataFile = new FileCodeSink(_resourceModel.OutputDirectory, "src\\Graphics\\core\\resources\\data_generated.h");
            Helpers.Style.WriteFileHeader(dataFile);

            // Write lines to ignore deprecation of D3DMATRIX
            dataFile.WriteBlock(
                [[inline]]
                    // error C4995: 'D3DMATRIX': name was marked as #pragma deprecated
                    //
                    // Ignore deprecation of D3DMATRIX for these types because
                    // they are generated from a defintion that is also used
                    // to generate types in wpf\src\Graphics\Include\Generated.
                    #pragma warning (push)
                    #pragma warning (disable : 4995)
                [[/inline]]
                );

            //
            // The data declarations file contains references to a number of
            // resource classes (consider, for example, the case of a Brush
            // resource which references a Transform resource). At the same
            // time, each resource needs to include its own data struct.
            // We thus have a circular dependency, so we need a set of forward
            // declarations for the resource classes in order to define the
            // structs. We generate a file with these forward declarations as
            // we see references to resources in WriteDataDecl. To avoid dupes
            // we use a hash table as a set.
            //

            FileCodeSink forwardsFile = new FileCodeSink(_resourceModel.OutputDirectory, "src\\Graphics\\core\\resources\\resources_generated.h");
            Helpers.Style.WriteFileHeader(forwardsFile);
            Hashtable forwards = new Hashtable();

            foreach (McgResource resource in _resourceModel.Resources)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.NativeDuce, resource))
                {
                    continue;
                }

                WriteProcessPacket(marshalFile, resource);
                WriteRegistrationMethods(marshalFile, resource);
                WriteSynchronizeAnimatedFieldsMethod(marshalFile, resource);
                WriteDataDecl(dataFile, resource, forwardsFile, forwards);
            }

            dataFile.WriteBlock(
                [[inline]]
                    #pragma warning (pop)
                [[/inline]]
                );

            forwardsFile.Dispose();
            dataFile.Dispose();
            marshalFile.Dispose();
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

        /// <summary>
        /// Append parameters to the parameter list, to represent the
        /// given list of DataItem nodes.
        ///
        ///    formal - if true, this is a formal parameter list;
        ///             otherwise, it is an actual parameter list.
        /// </summary>

        private void AppendParameters(
            ParameterList parameterList,
            McgField[] fields,
            bool formal
            )
        {
            if (formal)
            {
                Helpers.CodeGenHelpers.AppendParameters(parameterList, fields, Helpers.CodeGenHelpers.ParameterType.UnmanagedParamList);
            }
            else
            {
                Helpers.CodeGenHelpers.AppendParameters(parameterList, fields, Helpers.CodeGenHelpers.ParameterType.UnmanagedCallParamList);
            }
        }

        private void WriteSynchronizeAnimatedFieldsMethod(
            CodeSink codeSink,
            McgResource dataType
            )
        {
            if (dataType.IsAbstract)
            {
                return;
            }

            StringCodeSink csBody = new StringCodeSink();
            bool hasAnimatedFields = false;

            if (!dataType.IsOldStyle)
            {
                // Do not generate SynchronizeAnimatedFields() for 2D resources.
                return;
            }

            foreach (McgField field in dataType.AllUceFields)
            {
                if (field.IsAnimated)
                {
                    McgResource resource = (McgResource) field.Type;

                    string handleName = "m_data.m_p" + FirstCap(field.Name) + "Animation";
                    string dataName = "m_data.m_" + field.Name;

                    csBody.Write(
                        [[inline]]
                            if ([[handleName]])
                            {
                                IFC([[handleName]]->GetValue(&[[dataName]]));
                            }
                        [[/inline]]
                        );

                    hasAnimatedFields = true;
                }
            }

            if (hasAnimatedFields)
            {
                codeSink.WriteBlock(
                    [[inline]]

                        HRESULT [[dataType.DuceClass]]::SynchronizeAnimatedFields()
                        {
                            HRESULT hr = S_OK;

                            [[csBody]]

                        Cleanup:
                            RRETURN(hr);
                        }
                    [[/inline]]
                    );
            }
        }

        private string WriteRegisterCollectionNotifiers(McgResource parentResource, string fieldName, McgResource resource)
        {
            if (resource == null) return String.Empty;

            McgResource resourceType = resource.CollectionType as McgResource;

            if (resourceType != null && resourceType.UsesHandles)
            {
                return
                    [[inline]]
                        IFC(RegisterNNotifiers(m_data.m_rgp[[fieldName]], m_data.m_c[[fieldName]]));
                    [[/inline]];
            }
            else
            {
                return String.Empty;
            }
        }

        private string WriteUnRegisterCollectionNotifiers(McgResource parentResource, string fieldName, McgResource resource)
        {
            if (resource == null) return String.Empty;

            McgResource resourceType = resource.CollectionType as McgResource;

            if (resourceType != null && resourceType.UsesHandles)
            {
                return
                    [[inline]]
                        UnRegisterNNotifiers(m_data.m_rgp[[fieldName]], m_data.m_c[[fieldName]]);
                    [[/inline]];
            }
            else
            {
                return "";
            }
        }

        private void WriteRegistrationMethods(
            CodeSink codeSink,
            McgResource dataType
            )
        {
            // Don't write out this method, it keeps the resource class abstract too.
            if (dataType.IsAbstract)
            {
                return;
            }

            StringCodeSink registerBody = new StringCodeSink();
            StringCodeSink unregisterBody = new StringCodeSink();

            bool cleanup = false;
            bool registerCallsNeedsCleanup = false;

            foreach (McgField field in dataType.AllUceFields)
            {
                McgResource fieldResource = field.Type as McgResource;

                if (fieldResource != null &&
                    fieldResource.IsCollection &&
                    !fieldResource.HasUnmanagedResource)
                {
                    string registerCalls = WriteRegisterCollectionNotifiers(dataType as McgResource, field.Name, fieldResource);

                    registerBody.Write(
                        [[inline]]
                            [[registerCalls]]
                        [[/inline]]
                        );

                    if (registerCalls != "")
                    {
                        registerCallsNeedsCleanup = true;
                    }

                    string dataField;
                    string sizeField;

                    McgResource collectionType = fieldResource.CollectionType as McgResource;

                    if (!collectionType.UsesHandles)
                    {
                        dataField = [[inline]]m_data.m_p[[FirstCap(field.Name)]]Data[[/inline]];
                        sizeField = [[inline]]m_data.m_cb[[FirstCap(field.Name)]]Size[[/inline]];
                    }
                    else
                    {
                        dataField = [[inline]]m_data.m_rgp[[FirstCap(field.Name)]][[/inline]];
                        sizeField = [[inline]]m_data.m_c[[FirstCap(field.Name)]][[/inline]];
                    }

                    unregisterBody.Write(
                        [[inline]]

                            if ([[dataField]])
                            {
                                [[WriteUnRegisterCollectionNotifiers(dataType as McgResource, field.Name, fieldResource)]]
                                WPFFree(ProcessHeap, [[dataField]]);
                                [[dataField]] = NULL;
                            }
                            [[sizeField]] = 0;
                        [[/inline]]
                        );
                }
                else if ((field.Type is McgResource) && !field.Type.IsValueType)
                {
                    cleanup = true;

                    string fieldName = "m_data.m_p" + FirstCap(field.Name);

                    registerBody.Write(
                        [[inline]]
                            IFC(RegisterNotifier([[fieldName]]));
                        [[/inline]]
                        );

                    unregisterBody.Write(
                        [[inline]]
                            UnRegisterNotifier([[fieldName]]);
                        [[/inline]]
                        );
                }
            }

            foreach (McgField field in dataType.AllUceFields)
            {
                if (field.IsAnimated)
                {
                    cleanup = true;

                    string fieldName = "m_data.m_p" + FirstCap(field.Name) + "Animation";

                    registerBody.Write(
                        [[inline]]
                            IFC(RegisterNotifier([[fieldName]]));
                        [[/inline]]
                        );

                    unregisterBody.Write(
                        [[inline]]
                            UnRegisterNotifier([[fieldName]]);
                        [[/inline]]
                        );

                }
            }

            McgResource resource = dataType as McgResource;

            if (resource != null)
            {
                if (resource.IsCollection)
                {
                    cleanup = true;

                    registerBody.Write(WriteRegisterCollectionNotifiers(resource, "Resources", resource));
                    unregisterBody.Write(
                        [[inline]]

                            if (m_data.m_rgpResources)
                            {
                                [[WriteUnRegisterCollectionNotifiers(resource, "Resources", resource)]]
                                GpFree(m_data.m_rgpResources);
                                m_data.m_rgpResources = NULL;
                            }
                            m_data.m_cResources = 0;
                        [[/inline]]
                        );
                }
            }

            codeSink.WriteBlock(
                [[inline]]
                    HRESULT [[dataType.DuceClass]]::RegisterNotifiers(CMilSlaveHandleTable *pHandleTable)
                    {
                        HRESULT hr = S_OK;

                        [[registerBody]]
                [[/inline]]
                );

            if (cleanup || registerCallsNeedsCleanup)
            {
                codeSink.WriteBlock(
                    [[inline]]
                        Cleanup:
                    [[/inline]]
                    );
            }

            codeSink.WriteBlock(
                [[inline]]
                        RRETURN(hr);
                    }

                    /*override*/ void [[dataType.DuceClass]]::UnRegisterNotifiers()
                    {
                        [[unregisterBody]]
                    }
                [[/inline]]
                );

            if (dataType.CanIntroduceCycles
                && !(dataType.IsAbstract)
                && dataType.HasUnmanagedResource)
            {
                codeSink.WriteBlock(
                    [[inline]]
                        /*override*/ CMilSlaveResource* [[dataType.DuceClass]]::GetResource()
                        {
                            return this;
                        }
                    [[/inline]]
                );
            }
        }

        private void WriteProcessPacket(
            CodeSink codeSink,
            McgResource dataType
            )
        {
            String clearRealization = String.Empty;

            // Don't write out this method, it keeps the resource class abstract too.
            if (dataType.IsAbstract)
            {
                return;
            }

            if (dataType.Realization != null && dataType.Realization.IsCached)
            {
                clearRealization = "ClearRealization();";
            }

            //
            //  Because the struct and the data blob are frequently related
            //  (e.g., the struct has the count for the number of items in a
            //  collection in the data blob) it is convenient to codegen the
            //  code which process the struct and the data at the same time,
            //  hence we use two sinks.
            //

            // Statements which unmarshall the struct portion at the beginning of the packet
            StringCodeSink unmarshalStructBody = new StringCodeSink();

            // Statements which unmarshal the data portion at the end of the packet
            StringCodeSink unmarshalDataBody = new StringCodeSink();

            unmarshalStructBody.Write(
                [[inline]]

                    // Remove any pre-existing registered resources.
                    UnRegisterNotifiers();
                [[/inline]]
                );

            bool dataPointerBodyNeeded = false;
            // Use this code sink if we don't need the data pointer decl.
            StringCodeSink emptyDataPointerBody = new StringCodeSink();

            foreach (McgField field in dataType.AllUceFields)
            {
                if (field.Type.ParameterType == UnmanagedTypeType.Handle)
                {
                    McgResource fieldResource = field.Type as McgResource;
                    unmarshalStructBody.Write(
                        [[inline]]

                            if (pCmd->h[[field.Name]] != NULL)
                            {
                                m_data.m_p[[FirstCap(field.Name)]] =
                                    static_cast<[[fieldResource.DuceClass]]*>(pHandleTable->GetResource(
                                        pCmd->h[[field.Name]],
                                        [[fieldResource.MilTypeEnum]]
                                        ));

                                if (m_data.m_p[[FirstCap(field.Name)]] == NULL)
                                {
                                    RIP("Invalid handle.");
                                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                                }
                            }
                            else
                            {
                                m_data.m_p[[FirstCap(field.Name)]] = NULL;
                            }

                        [[/inline]]
                        );
                }
                else if (field.Type.IsValueType)
                {
                    //
                    // Type member variable is a value type and can be
                    // marshalled by copying it into the stream directly.
                    //
                    unmarshalStructBody.Write(
                        [[inline]]
                            m_data.m_[[field.Name]] = pCmd->[[field.Name]];
                        [[/inline]]
                        );
                }
                else
                {
                    McgResource fieldResource = field.Type as McgResource;

                    if (fieldResource != null
                        && fieldResource.IsCollection
                        && !fieldResource.HasUnmanagedResource)
                    {
                        // We have a field that is a collection and we'll need the data section pointer
                        dataPointerBodyNeeded = true;
                        McgResource collectionType = fieldResource.CollectionType as McgResource;

                        // Marshal the collection data. Do the copy

                        if (!collectionType.UsesHandles)
                        {
                            string dataField = [[inline]]m_data.m_p[[FirstCap(field.Name)]]Data[[/inline]];
                            string sizeField = [[inline]]m_data.m_cb[[FirstCap(field.Name)]]Size[[/inline]];

                            string bufferSizeCheck = String.Empty;

                            if (fieldResource.CollectionType.UnmanagedDataType == "MilPathGeometry")
                            {
                                bufferSizeCheck =
                                    [[inline]]
                                        //
                                        // Check if the manifested size of the payload matches what we have
                                        // received from the transport. Since the PathGeometry packets are
                                        // really complicated and difficult to perform automatically, more
                                        // detailed checks are performed at a later stage.
                                        //

                                        if ([[sizeField]] > cbPayload)
                                        {
                                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                                        }
                                    [[/inline]];
                            }
                            else
                            {
                                bufferSizeCheck =
                                    [[inline]]
                                        //
                                        // Check if the manifested size of the payload matches what we have
                                        // received from the transport. Also, the manifested size should be
                                        // a multiply of the size of the contained type.
                                        //

                                        if ([[sizeField]] > cbPayload
                                            || [[sizeField]] % sizeof([[fieldResource.CollectionType.UnmanagedDataType]]) != 0)
                                        {
                                            IFC(WGXERR_UCE_MALFORMEDPACKET);
                                        }
                                    [[/inline]];
                            }

                            unmarshalDataBody.Write(
                                [[inline]]
                                    // Read the [[field.Name]]
                                    [[sizeField]] = pCmd->[[field.Name]]Size;
                                    if ([[sizeField]] > 0)
                                    {
                                        [[bufferSizeCheck]]

                                        // Allocate memory for copy of data
                                        IFC(HrAlloc(
                                            Mt([[dataType.DuceClass]]),
                                            [[sizeField]],
                                            reinterpret_cast<void**>(&[[dataField]])
                                            ));

                                        // Copy data
                                        RtlCopyMemory([[dataField]], pbDataSection, [[sizeField]]);

                                        // Advance data pointer and reduce data size
                                        cbPayload -= [[sizeField]];
                                        pbDataSection += [[sizeField]];
                                    }
                                [[/inline]]
                                );

                            if (   dataType.MilTypeEnum == "TYPE_MESHGEOMETRY2D" 
                                && field.Name.Equals("TriangleIndices"))
                            {
                                unmarshalDataBody.Write(
                                    [[inline]]
                                    
                                        // Compute vertex buffer size
                                        // These buffers should all be the same size, since we need a position, opacity and texture coordinate set
                                        // for each vertex we're going to render. To be sure we won't try and render any incomplete vertices, take the
                                        // minimum of the buffer sizes as the vertex count.
                                        UINT vertexCount = m_data.m_cbPositionsSize / sizeof(m_data.m_pPositionsData[0]);
                                        UINT opacityCount = m_data.m_cbVertexOpacitiesSize / sizeof(m_data.m_pVertexOpacitiesData[0]);
                                        UINT textureCount = m_data.m_cbTextureCoordinatesSize / sizeof(m_data.m_pTextureCoordinatesData[0]);
                                        vertexCount = min(vertexCount, opacityCount);
                                        vertexCount = min(vertexCount, textureCount);

                                        //  Note that if an index is negative, its UINT representation will be greater than
                                        //  vertexCount and it will be rejected.
                                        const UINT *pTriangleIndicesData = reinterpret_cast<const UINT *>([[dataField]]);

                                        // Validate index buffer
                                        UINT triangleIndicesCount = [[sizeField]] / sizeof([[dataField]][0]);        
                                        for (UINT i = 0; i < triangleIndicesCount; i++)
                                        {
                                            if (pTriangleIndicesData[i] >= vertexCount)
                                            {
                                                IFC(WGXERR_UCE_MALFORMEDPACKET);
                                            }
                                        }
                                    
                                    [[/inline]]
                                );
                            }            
                        }
                        else
                        {
                            unmarshalDataBody.Write(
                                [[inline]]
                                    // Read the [[field.Name]]
                                    // This is for data with handles only; collection field
                                    // need to do the pointer to handle conversion here.
                                    IFC(UnmarshalResourceArray(
                                        IN OUT pbDataSection,
                                        IN OUT cbPayload,
                                        pCmd->[[field.Name]]Size,
                                        [[collectionType.MilTypeEnum]],
                                        Mt([[dataType.DuceClass]]),
                                        OUT m_data.m_c[[FirstCap(field.Name)]],
                                        OUT m_data.m_rgp[[FirstCap(field.Name)]],
                                        pHandleTable,
                                        [[fieldResource.IsCollectionWhichAllowsNullEntries ? "true" : "false"]]
                                        ));
                                [[/inline]]
                                );
                        }
                    }
                }

                if (field.IsAnimated)
                {
                    McgResource fieldResource = field.Type as McgResource;

                    unmarshalStructBody.Write(
                        [[inline]]
                            if (pCmd->h[[FirstCap(field.Name)]]Animations != NULL)
                            {
                                m_data.m_p[[FirstCap(field.Name)]]Animation =
                                    static_cast<[[fieldResource.DuceClass]]*>(pHandleTable->GetResource(
                                        pCmd->h[[FirstCap(field.Name)]]Animations,
                                        [[fieldResource.MilTypeEnum]]
                                        ));

                                if (m_data.m_p[[FirstCap(field.Name)]]Animation == NULL)
                                {
                                    RIP("Invalid handle.");
                                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                                }
                            }
                            else
                            {
                                m_data.m_p[[FirstCap(field.Name)]]Animation = NULL;
                            }

                        [[/inline]]
                        );
                }
            }

            McgResource resource = dataType as McgResource;

            if (resource != null)
            {
                if (resource.IsCollection)
                {
                    // We're going to use that pointer to the data
                    // section after the struct, so remember to
                    // include the declaration.
                    dataPointerBodyNeeded = true;

                    // Unmarshal the collection data.
                    McgResource collectionType =
                        resource.CollectionType as McgResource;

                    unmarshalStructBody.Write(
                        [[inline]]
                            IFC(UnmarshalResourceArray(
                                IN OUT pbDataSection,
                                IN OUT cbPayload,
                                pCmd->Size,
                                [[collectionType.MilTypeEnum]],
                                Mt([[dataType.DuceClass]]),
                                OUT m_data.m_cResources,
                                OUT m_data.m_rgpResources,
                                pHandleTable
                                ));
                        [[/inline]]
                        );
                }
            }

            unmarshalDataBody.Write(
                [[inline]]

                    // Register the new resources.
                    IFC(RegisterNotifiers(pHandleTable));
                [[/inline]]
                );


            //
            // For collections resources or collection fields we need a pointer
            // to the data segment after the structure part of the command.
            //
            // The pointer is created by the command batch handler.
            //

            string updateMethodParams = String.Empty;
            string dataSectionDeclaration = String.Empty;

            if (dataPointerBodyNeeded)
            {
                updateMethodParams =
                    [[inline]]
                        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
                        __in_ecount(1) const [[dataType.CommandName]]* pCmd,
                        __in_bcount(cbPayload) LPCVOID pPayload,
                        UINT cbPayload
                    [[/inline]];

                dataSectionDeclaration =
                    [[inline]]
                        const BYTE* pbDataSection =
                            reinterpret_cast<const BYTE*>(pPayload);
                    [[/inline]];
            }
            else
            {
                updateMethodParams =
                    [[inline]]
                        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
                        __in_ecount(1) const [[dataType.CommandName]]* pCmd
                    [[/inline]];
            }

            string processUpdateCore = String.Empty;

            if (resource.CallProcessUpdateCore)
            {
                processUpdateCore =
                    [[inline]]
                        IFC(ProcessUpdateCore());
                    [[/inline]];
            }
            
            string generatedProcessUpdateName = "ProcessUpdate";
            if (resource.UseProcessUpdateWrapper)
            {
                generatedProcessUpdateName = "GeneratedProcessUpdate";
            }

            codeSink.WriteBlock(
                [[inline]]
                    HRESULT [[resource.DuceClass]]::[[generatedProcessUpdateName]](
                        [[updateMethodParams]]
                        )
                    {
                        HRESULT hr = S_OK;

                        [[dataSectionDeclaration]]

                        [[clearRealization]]

                        [[unmarshalStructBody]]
                        [[unmarshalDataBody]]

                        [[processUpdateCore]]

                    Cleanup:
                        if (FAILED(hr))
                        {
                            //
                            // We have failed to process the update command. Performing unregistration
                            // now guarantees that we leave the resource in a predictable state.
                            //

                            UnRegisterNotifiers();
                        }

                        NotifyOnChanged(this);

                        RRETURN(hr);
                    }
                [[/inline]]
                );
        }

        private void WriteDataDecl(
            CodeSink codeSink,
            McgResource dataType,
            CodeSink forwardsSink,
            Hashtable forwards)
        {
            if (dataType.IsAbstract)
            {
                return;
            }

            string className = dataType.DuceClass;

            codeSink.Write(
                [[inline]]
                    struct [[className]]_Data
                    {
                [[/inline]]
                );

            foreach (McgField field in dataType.AllUceFields)
            {
                McgResource resource = field.Type as McgResource;
                if (resource != null &&
                    resource.IsCollection &&
                    !resource.HasUnmanagedResource)
                {
                    McgResource collectionType = resource.CollectionType as McgResource;

                    if (!collectionType.UsesHandles) {
                        codeSink.Write(
                            [[inline]]
                                    UINT32 m_cb[[FirstCap(field.Name)]]Size;
                                    [[collectionType.UnmanagedDataType]] *m_p[[FirstCap(field.Name)]]Data;
                            [[/inline]]
                            );
                    }
                    else
                    {
                        string fieldType = collectionType.DuceClass;

                        if (!collectionType.IsValueType)
                        {
                            // The field is a composition resource
                            if (!forwards.ContainsKey(fieldType))
                            {
                                forwardsSink.Write(
                                    [[inline]]
                                        class [[fieldType]];
                                    [[/inline]]
                                    );
                                forwards.Add(fieldType, null);
                            }
                        }

                        codeSink.Write(
                            [[inline]]
                                    UINT32 m_c[[FirstCap(field.Name)]];
                                    [[fieldType]] **m_rgp[[FirstCap(field.Name)]];
                            [[/inline]]
                            );
                    }
                }
                else
                {
                    string fieldType = (resource != null) ? resource.DuceClass : null;
                    if (resource != null && !resource.IsValueType)
                    {
                        // The field is a composition resource
                        if (!forwards.ContainsKey(fieldType))
                        {
                            forwardsSink.Write(
                                [[inline]]
                                    class [[fieldType]];
                                [[/inline]]
                                );
                            forwards.Add(fieldType, null);
                        }

                        codeSink.Write(
                            [[inline]]
                                    [[fieldType]] *m_p[[FirstCap(field.Name)]];
                            [[/inline]]
                            );
                    }
                    else
                    {
                        codeSink.Write(
                            [[inline]]
                                    [[field.Type.ToFormalParam("m_"+field.Name)]];
                            [[/inline]]
                            );
                    }

                    if (field.IsAnimated)
                    {
                        // Fields can only be animated by composition resources,
                        // so this field must be a McgResource.
                        codeSink.Write(
                            [[inline]]
                                    [[fieldType]] *m_p[[FirstCap(field.Name)]]Animation;
                            [[/inline]]
                            );
                    }
                }
            }

            if (dataType is McgResource)
            {
                McgResource dataResource = dataType as McgResource;
                if (dataResource.IsCollection)
                {
                    codeSink.WriteBlock(
                        [[inline]]
                                UINT m_cResources;
                                CMilSlaveResource **m_rgpResources;
                        [[/inline]]
                        );
                }
            }

            codeSink.Write(
                [[inline]]
                    };

                [[/inline]]
                );
        }

        private string WriteGetTypeMethod(McgResource resource)
        {
            if (resource.IsOldStyle)
            {
                // 3D resources return the actual type of the resource
                // (e.g., PointLight returns TYPE_POINTLIGHT not TYPE_LIGHT)
                //
                if (resource.IsAbstract)
                {
                    return String.Empty;
                }

                return
                    [[inline]]
                        MIL_RESOURCE_TYPE Type() { return TYPE_[[AllCaps(resource.Name)]]; }
                    [[/inline]];
            }
            else
            {
                // 2D resources return the base type of the resource
                // (e.g., LineGeometry return TYPE_GEOMETRY not TYPE_LINEGEOMETRY)
                //
                string resourceType = AllCaps(resource.Name);

                McgResource extendsBase = resource.ExtendsBase as McgResource;
                if (extendsBase != null)
                {
                    resourceType = AllCaps(extendsBase.Name);
                }

                return
                    [[inline]]
                        MIL_RESOURCE_TYPE Type() { return TYPE_[[resourceType]]; }
                    [[/inline]];
            }
        }

        private string GetBaseClass(McgResource resource)
        {
            string parentClassName = "CMilSlaveResource";

            if (resource.Extends != null)
            {
                McgResource parentResource = resource.Extends as McgResource;
                if (parentResource != null)
                {
                    parentClassName = parentResource.DuceClass;
                }
            }

            return parentClassName;
        }

        #endregion Private Methods

        private struct FileInfo
        {
            public FileInfo(FileCodeSink codeSink)
            {
                CodeSink = codeSink;
            }

            public string GetClassName(McgResource resource)
            {
                return resource.DuceClass;
            }

            public string Write(string str)
            {
                return str;
            }

            public FileCodeSink CodeSink;
        }
    }
}



