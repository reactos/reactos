// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Generate files for UCE master resources.
//

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

    public partial class ManagedResources : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        #region Constructors

        public ManagedResources(ResourceModel rm) : base(rm)
        {
            _reWhitespace = new Regex(@"^[\s\n]*$");
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
            foreach (McgResource resource in _resourceModel.Resources)
            {
                if (!_resourceModel.ShouldGenerate(CodeSections.ManagedClass, resource))
                {
                    continue;
                }

                string fileName = resource.Name + ".cs";

                string fullPath = Path.Combine(_resourceModel.OutputDirectory, resource.ManagedDestinationDir);

                WriteTypeConverter(resource);

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    _staticCtorText = new StringCodeSink();
                    
                    List<string> modifiers = new List<string>();
                    List<string> extends = new List<string>();
                    string typeconverter = String.Empty;
                    string attributes = String.Empty;

                    if (resource.IsAbstract)
                    {
                        modifiers.Add("abstract");
                    }
                    else if (resource.IsValueType)
                    {
                        attributes += "[Serializable]\n";
                    }

                    if (_resourceModel.ShouldGenerate(CodeSections.ManagedTypeConverter, resource))
                    {
                        attributes += "[TypeConverter(typeof(" + resource.Name + "Converter))]\n";
                        attributes += "[ValueSerializer(typeof(" + resource.Name + "ValueSerializer))] // Used by MarkupWriter\n";
                    }

                    if (resource.Extends != null)
                    {
                        extends.Add(resource.Extends.Name);
                    }
                    else
                    {
                        if (resource.IsAnimatable)
                        {
                            extends.Add("Animatable");
                        }
                        else if (resource.IsFreezable)
                        {
                            extends.Add("Freezable");
                        }

                        // If we are not to skip ToString, we need to support multiple cultures.
                        // As part of this, we implement IFormattable.
                        if (!resource.SkipToString)
                        {
                            extends.Add("IFormattable");
                        }
                    }

                    if (resource.IsCollection)
                    {
                        extends.Add("IList");
                        extends.Add("IList<" + resource.CollectionType.ManagedName + ">");

                        modifiers.Add("public");

                        if (resource.GenerateSerializerAttribute)
                        {
                            attributes = attributes +
                                [[inline]]
                                    [XamlDesignerSerializer("System.Windows.Markup.XamlIEnumerableSerializer, PresentationFramework, Version=" + MS.Utility.RefAssembly.Version + ", Culture=neutral, PublicKeyToken=" + Microsoft.Internal.BuildInfo.WCP_PUBLIC_KEY_TOKEN + ", Custom=null")]
                                [[/inline]];
                        }
                    }

                    if (resource.IsSealed)
                    {
                        modifiers.Add("sealed");
                    }

                    modifiers.Add("partial");

                    if (resource.HasUnmanagedResource && !resource.DerivesFromTypeWhichHasUnmanagedResource)
                    {
                        extends.Add("DUCE.IResource");
                    }

                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName)]]
                        [[/inline]]
                        );

                    foreach (string s in resource.Namespaces)
                    {
                        csFile.Write(
                            [[inline]]
                                using [[s]];
                            [[/inline]]
                                );
                    }

                    csFile.WriteBlock(
                        [[inline]]
                            // These types are aliased to match the unamanaged names used in interop
                            using BOOL = System.UInt32;
                            using WORD = System.UInt16;
                            using Float = System.Single;

                            namespace [[resource.ManagedNamespace]]
                            {
                                [[Helpers.CollectionHelper.WriteCollectionSummary(resource)]]
                                [[typeconverter]]
                                [[attributes]]
                                [[WriteClassDeclaration(resource.Name, !resource.IsValueType, modifiers, extends)]]
                                {
                                    [[Helpers.ManagedStyle.WriteSection("Public Methods")]]

                                    #region Public Methods

                                    [[WriteClone(resource)]]
                                    [[WriteCloneCurrentValue(resource)]]

                                    [[WriteObjectMethods(resource)]]

                                    #endregion Public Methods

                                    [[Helpers.ManagedStyle.WriteSection("Public Properties")]]

                                    [[WriteLocalPropertyDelegates(resource)]]
                                    [[Helpers.CollectionHelper.WriteCollectionMethods(resource)]]

                                    #region Public Properties

                                    [[WriteProperties(resource)]]

                                    #endregion Public Properties

                                    [[Helpers.ManagedStyle.WriteSection("Protected Methods")]]

                                    #region Protected Methods

                                    [[WriteCreateInstanceCore(resource)]]
                                    [[WriteCloneCoreMethods(resource)]]
                                    [[WriteFreezeCore(resource)]]

                                    #endregion ProtectedMethods

                                    [[Helpers.ManagedStyle.WriteSection("Internal Methods")]]

                                    #region Internal Methods

                                    [[WriteUpdateResource(resource)]]
                                    [[WriteAddRefOnChannel(resource)]]
                                    [[WriteReleaseOnChannel(resource)]]
                                    [[WriteGetDuceResource(resource)]]
                                    [[WriteGetChannelCount(resource)]]
                                    [[WriteGetChannel(resource)]]
                                    [[WriteListMarshalling(resource)]]

                                    #endregion Internal Methods

                                    [[Helpers.ManagedStyle.WriteSection("Internal Properties")]]

                                    #region Internal Properties

                                    [[WriteEffectiveValuesInitialSize(resource)]]
                                    [[WriteToString(resource)]]
                                    [[WriteParse(resource)]]

                                    #endregion Internal Properties

                                    [[Helpers.ManagedStyle.WriteSection("Dependency Properties")]]

                                    #region Dependency Properties

                                    [[WriteRegisterDPProperty(resource)]]

                                    #endregion Dependency Properties

                                    [[Helpers.ManagedStyle.WriteSection("Internal Fields")]]

                                    #region Internal Fields

                                    [[WriteLocallyCachedProperties(resource)]]
                                    [[WriteCacheDecls(resource)]]
                                    [[WriteResourceHandleField(resource)]]
                                    [[Helpers.CollectionHelper.WriteCollectionFields(resource)]]
                                    [[WriteDefaultValues(resource)]]

                                    #endregion Internal Fields

                                    [[Helpers.CollectionHelper.WriteCollectionEnumerator(resource)]]

                                    #region Constructors

                                    [[Helpers.ManagedStyle.WriteSection("Constructors")]]

                                    [[WriteStaticCtor(resource)]]
                                    [[Helpers.CollectionHelper.WriteCollectionConstructors(resource)]]

                                    #endregion Constructors

                                }
                            }
                        [[/inline]]
                    );
                }

                if (resource.ShouldGenerateEmptyClass)
                {
                    string emptyClassFileName = resource.EmptyClassName + ".cs";

                    using (FileCodeSink csFile = new FileCodeSink(fullPath, emptyClassFileName))
                    {
                        csFile.WriteBlock(Helpers.ManagedStyle.WriteFileHeader(emptyClassFileName));
                        csFile.WriteBlock(Helpers.EmptyClassHelper.WriteEmptyClass(resource));
                    }
                }
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

        private string WriteClassDeclaration(string name, bool isClass, List<string> modifiers, List<string> extends)
        {
            string modifiersStr = String.Empty;
            string extendsStr = String.Empty;

            if (modifiers != null && modifiers.Count > 0)
            {
                modifiersStr = String.Join(" ", modifiers.ToArray()) + " ";
            }

            if (extends != null && extends.Count > 0)
            {
                extendsStr = " : " + String.Join(", ", extends.ToArray());
            }

            if (isClass)
            {
                return
                    [[inline]]
                        [[modifiersStr]]class [[name]][[extendsStr]]
                    [[/inline]];
            }
            else // struct
            {
                return
                    [[inline]]
                        [[modifiersStr]]struct [[name]][[extendsStr]]
                    [[/inline]];
            }

        }

        private string WriteCloneCoreMethods(McgResource resource)
        {
            if (resource.IsValueType || (!resource.AddCloneHooks && !resource.IsCollection))
            {
                return String.Empty;
            }

            string cloneCoreMethods = String.Empty;

            if (resource.IsFreezable)
            {
                cloneCoreMethods += WriteCloneCoreMethodsHelper_CloneMethod(resource, "Clone", "Freezable");
                cloneCoreMethods += WriteCloneCoreMethodsHelper_CloneMethod(resource, "CloneCurrentValue", "Freezable");
                cloneCoreMethods += WriteCloneCoreMethodsHelper_CloneMethod(resource, "GetAsFrozen", "Freezable");
                cloneCoreMethods += WriteCloneCoreMethodsHelper_CloneMethod(resource, "GetCurrentValueAsFrozen", "Freezable");
            }

            return cloneCoreMethods;
        }

        private string WriteCloneCoreMethodsHelper_CloneMethod(McgResource resource, string method, string argType)
        {
            string sourceType = resource.Name;
            string source = "source" + sourceType;

            string body =
                [[inline]]
                    base.[[method]]Core(source);

                    [[Helpers.CollectionHelper.CloneCollection(resource, method, argType, source)]]
                [[/inline]];

            body = WriteCloneCoreMethodsHelper_AddCloneHooks(resource.AddCloneHooks, source, body);

            return
                [[inline]]
                    /// <summary>
                    /// Implementation of [[argType]].[[method]]Core()
                    /// </summary>
                    protected override void [[method]]Core([[argType]] source)
                    {
                        [[sourceType]] [[source]] = ([[sourceType]]) source;

                        [[body]]
                    }
                [[/inline]];
        }

        private string WriteCloneCoreMethodsHelper_AddCloneHooks(bool addHooks, string source, string body)
        {
            if (!addHooks)
                return body;

            return
                [[inline]]
                    // Set any state required before actual clone happens
                    ClonePrequel([[source]]);

                    [[body]]

                    // Set state once clone has finished
                    ClonePostscript([[source]]);
                [[/inline]];
        }

        private string WriteFreezeCore(McgResource resource)
        {
            // For now, only the collections need a FC override
            return Helpers.CollectionHelper.FreezeCore(resource);
        }

        private string WriteClone(McgResource resource)
        {
            if (!resource.IsFreezable) return String.Empty;

            return WriteStronglyTypedShadow("Clone", resource.Name);
        }

        private string WriteCloneCurrentValue(McgResource resource)
        {
            if (!resource.IsFreezable) return String.Empty;

            return WriteStronglyTypedShadow("CloneCurrentValue", resource.Name);
        }

        private string WriteStronglyTypedShadow(string method, string returnType)
        {
            return
                [[inline]]
                    /// <summary>
                    ///     Shadows inherited [[method]]() with a strongly typed
                    ///     version for convenience.
                    /// </summary>
                    public new [[returnType]] [[method]]()
                    {
                        return ([[returnType]])base.[[method]]();
                    }

                [[/inline]];
        }

        private void WritePerFieldMethod(McgResource resource,
                                         StringCodeSink cs,
                                         string methodName,
                                         string descriptionText,
                                         bool emitFloatWarning,
                                         string perParamText,
                                         string returnType,
                                         string returnText,
                                         string perFieldOperator,
                                         string fieldResultCombiner)
        {
            string lowerName = MS.Internal.MilCodeGen.Runtime.GeneratorMethods.FirstLower(resource.Name);

            cs.Write(
                [[inline]]
                    /// <summary>
                    /// [[descriptionText]]
                [[/inline]]
                );

            if (emitFloatWarning)
            {
                cs.Write(
                    [[inline]]
                        /// Note that double values can acquire error when operated upon, such that
                        /// an exact comparison between two values which are logically equal may fail.
                    [[/inline]]
                    );
            }

            cs.Write(
                [[inline]]
                    /// </summary>
                    /// <returns>
                    /// [[returnText]]
                    /// </returns>
                    /// <param name='[[lowerName]]1'>The first [[resource.Name]] to [[perParamText]]</param>
                    /// <param name='[[lowerName]]2'>The second [[resource.Name]] to [[perParamText]]</param>
                    public static [[returnType]] [[methodName]] ([[resource.Name]] [[lowerName]]1, [[resource.Name]] [[lowerName]]2)
                    {
                [[/inline]]
                );

            if (resource.LocalFields.Length < 1)
            {
                cs.Write(
                [[inline]]
                        return new [[returnType]]();
                    };
                [[/inline]]
                    );
            }
            else
            {
                cs.Write("    return ");

                cs.Write(Helpers.CodeGenHelpers.WriteFieldStatementsWithSeparator(resource.LocalFields,
                                                                          "(" + lowerName + "1.{fieldName} " + perFieldOperator + " " + lowerName + "2.{fieldName})",
                                                                          fieldResultCombiner));

                cs.Write(
                    [[inline]]
                        ;
                        }

                    [[/inline]]
                    );
            }
        }

        /// <summary>
        /// Produces a string that will set local cache for field field to value and change the
        /// cache status to status.  If the new status is Invalid, then note that you cannot specify
        /// a new value other than null.
        /// </summary>
        /// <param name="prefix">String to put before references to members of the resource, e.g. "target."</param>
        /// <param name="field">Field to set.</param>
        /// <param name="value">New value, must be null if status is LocalPropertyStatus.Invalid.</param>
        /// <param name="status">New status, should start with "LocalPropertyStatus." including the period.</param>
        private string WriteSetCachedValue(string prefix, McgField field, string value, string status)
        {
            if (status == "LocalPropertyStatus.Invalid")
            {
                // When setting the cached status to Invalid you cannot specify a new value.
                Debug.Assert( value == null );
                if (field.Type.IsValueType)
                {
                    return prefix + "SetLocalPropertyStatus("+field.PropertyFlag+", LocalPropertyStatus.Invalid);";
                }
                else
                {
                    return prefix + "SetLocal" + field.PropertyName + "Cache(null, LocalPropertyStatus.Invalid);";
                }
            }
        else
        {
                if (field.Type.IsValueType)
                {
                    return
                        prefix + ""+ field.InternalName + " = " + value + ";\n"+
                        prefix + "SetLocalPropertyStatus("+field.PropertyFlag+", " + status + ");";
                }
                else
                {
                    return prefix + "SetLocal" + field.PropertyName + "Cache(" + value + ", " + status + ");";
                }
            }
        }

        private bool NeedsPropertyChangedCallback(McgResource resource, McgField field)
        {
            // If we can skip all properties, we do not auto-generate the changed callback
            if (resource.SkipProperties)
            {
                return false;
            }

            // Value types don't have DependencyProperties
            if (resource.IsValueType)
            {
                return false;
            }

            // If we have a property changed hook, we must emit code to call it
            if (field.PropertyChangedHook)
            {
                return true;
            }

            // If the field is "independently" animated, we must call up to the base to update
            // the animation resource.
            if (field.IsAnimated)
            {
                return true;
            }

            // If this resource doesn't have an unmanaged resource, we don't need to work
            // (beyond the changed hook which has already been handled above)
            if (!resource.HasUnmanagedResourceOrDerivesFromTypeWhichHasUnmanagedResource)
            {
                return false;
            }

            return true;
        }

        private string WriteLocalPropertyDelegates(McgResource resource)
        {
            if (resource.SkipProperties) return String.Empty;
            if (resource.IsValueType) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                if (field.IsUnmanagedOnly) continue;

                bool needToUnindent = false;

                if (field.IsValidate)
                {
                    cs.Write(
                        [[inline]]
                            private static bool Validate[[field.PropertyName]]Value(object value)
                            {
                        [[/inline]]
                            );


                    // If "field.IsValidate" is true, we must call "OnFooChanging" and "OnFooChanged"
                    // before and after setting a new value.
                    cs.Write(
                        [[inline]]

                                // This resource needs to be notified on new values being set.
                                if (!On[[field.PropertyName]]Changing(([[field.Type.ManagedName]]) value))
                                {
                                    return false;
                                }
                        [[/inline]]
                            );

                    // If the field is an enum, we need to validate that the new value is
                    // a valid instance of the enum.  We do this by calling the CheckIfValid method
                    // in ValidateEnums for this enum type.
                    // We need to do this because any int can be cast to an instance of an enum, even
                    // if the numeric value doesn't have a corresponding enum value.
                    if (field.Type is McgEnum)
                    {
                        McgEnum enumerator = (McgEnum)field.Type;
                        cs.Write(
                            [[inline]]

                                    if (![[enumerator.ManagedNamespace]].ValidateEnums.Is[[field.Type.ManagedName]]Valid(value))
                                    {
                                        return false;
                                    }
                            [[/inline]]
                                );
                    }

                    cs.Write(
                        [[inline]]
                                return true;
                            }
                        [[/inline]]
                            );
                }

                if (NeedsPropertyChangedCallback(resource, field))
                {
                    if (field.Type.IsFreezable)
                    {
                        // This boolean is true if:
                        //   This field is a collection of a type which uses handles.
                        //
                        bool fieldIsResourceThatContainsHandles =
                            ((McgResource)field.Type).IsCollection &&
                            ((McgResource)field.Type).CollectionType is McgResource &&
                            ((McgResource)((McgResource)field.Type).CollectionType).UsesHandles;

                        // This boolean is true if:
                        //   This field doesn't contain inlined handles and this field is not itself a field which contains handles
                        //   OR
                        //   This field's type uses handles.
                        // If true, this means that subproperty changes are handled by the field's type and
                        // this resource doesn't need to do any work.  As a result, we can early out if the
                        // property change IsASubPropertyChange.
                        bool canEarlyOutIfIsASubPropertyChange =
                            fieldIsResourceThatContainsHandles ||
                            ((McgResource)field.Type).UsesHandles;

                        // This boolean is true if we don't need to cast "d" to a specific type before
                        // checking for an early out.  Today this is only true if there's a property changed
                        // hook to call (since we must call this even if we're early out'ing).
                        bool needTargetAsSpecificTypeBeforeSubPropertyChange = field.PropertyChangedHook;

                        cs.Write(
                            [[inline]]
                                private static void [[field.PropertyName]]PropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
                                {
                                    [[conditional(needTargetAsSpecificTypeBeforeSubPropertyChange)]]
                                    [[resource.Name]] target = (([[resource.Name]]) d);
                                    [[/conditional]]
                                    [[conditional(field.PropertyChangedHook)]]
                                    target.[[field.PropertyName]]PropertyChangedHook(e);
                                    [[/conditional]]
                                    [[conditional(canEarlyOutIfIsASubPropertyChange)]]

                                    // The first change to the default value of a mutable collection property (e.g. GeometryGroup.Children) 
                                    // will promote the property value from a default value to a local value. This is technically a sub-property 
                                    // change because the collection was changed and not a new collection set (GeometryGroup.Children.
                                    // Add versus GeometryGroup.Children = myNewChildrenCollection). However, we never marshalled 
                                    // the default value to the compositor. If the property changes from a default value, the new local value 
                                    // needs to be marshalled to the compositor. We detect this scenario with the second condition 
                                    // e.OldValueSource != e.NewValueSource. Specifically in this scenario the OldValueSource will be 
                                    // Default and the NewValueSource will be Local.
                                    if (e.IsASubPropertyChange && 
                                       (e.OldValueSource == e.NewValueSource))
                                    {
                                        return;
                                    }
                                    [[/conditional]]
                                    [[conditional(!needTargetAsSpecificTypeBeforeSubPropertyChange)]]
                                    [[resource.Name]] target = (([[resource.Name]]) d);
                                    [[/conditional]]
                                    [[conditional(field.CachedLocally)]]
                                    target._cached[[field.PropertyName]]Value = ([[field.Type.ManagedName]])e.NewValue;
                                    [[/conditional]]
                            [[/inline]]
                            );

                        //
                        //  Build up the addRemoveEvent string.  This contains the sinking and unsinking
                        //  of Changed and List events if applicable.
                        //

                        // This bool indicates the case where this type is a resource and the field is of a
                        // type which derives from Freezable but is *not* itself a resource.  In this case,
                        // we have (above) opted out of sub-property invalidation notification (so that even
                        // sub-property changes will cause a re-marshal), but we may also need to track
                        // add and remove to maintain DUCE reference counts.
                        bool fieldReferencesFreezableWithoutResource =
                            field.Type.IsFreezable &&
                            (field.Type is McgResource) &&
                            !((McgResource)field.Type).HasUnmanagedResource &&
                            resource.HasUnmanagedResource;

                        if (fieldReferencesFreezableWithoutResource)
                        {
                            Debug.Assert(field.Type.IsFreezable);
                            Debug.Assert(field.Type is McgResource);

                            string addListHandler = String.Empty;
                            string removeListHandler = String.Empty;

                            // If we are a collection hanging onto resources we need to sink
                            // insertion/remove events.
                            if (fieldIsResourceThatContainsHandles)
                            {
                                removeListHandler =
                                    [[inline]]
                                        oldCollection.ItemRemoved -= target.[[field.PropertyName]]ItemRemoved;
                                        oldCollection.ItemInserted -= target.[[field.PropertyName]]ItemInserted;
                                    [[/inline]];

                                addListHandler =
                                    [[inline]]
                                        newCollection.ItemInserted += target.[[field.PropertyName]]ItemInserted;
                                        newCollection.ItemRemoved += target.[[field.PropertyName]]ItemRemoved;
                                    [[/inline]];

                                cs.Write(
                                    [[inline]]
                                            // If this is both non-null and mutable, we need to unhook the Changed event.
                                            [[field.Type.ManagedName]] oldCollection = null;
                                            [[field.Type.ManagedName]] newCollection = null;

                                            if ((e.OldValueSource != BaseValueSourceInternal.Default) || e.IsOldValueModified)
                                            {
                                                oldCollection = ([[field.Type.ManagedName]]) e.OldValue;
                                                if ((oldCollection != null) && !oldCollection.IsFrozen)
                                                {
                                                    [[removeListHandler]]
                                                }
                                            }

                                            // If this is both non-null and mutable, we need to hook the Changed event.
                                            if ((e.NewValueSource != BaseValueSourceInternal.Default) || e.IsNewValueModified)
                                            {
                                                newCollection = ([[field.Type.ManagedName]]) e.NewValue;
                                                if ((newCollection != null) && !newCollection.IsFrozen)
                                                {
                                                    [[addListHandler]]
                                                }
                                            }
                                    [[/inline]]
                                    );
                            }

                            if (fieldIsResourceThatContainsHandles)
                            {
                                cs.Write(
                                    [[inline]]
                                            if (oldCollection != newCollection && target.Dispatcher != null)
                                            {
                                                using (CompositionEngineLock.Acquire())
                                                {
                                                    DUCE.IResource targetResource = (DUCE.IResource)target;
                                                    int channelCount = targetResource.GetChannelCount();

                                                    for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
                                                    {
                                                        DUCE.Channel channel = targetResource.GetChannel(channelIndex);
                                                        Debug.Assert(!channel.IsOutOfBandChannel);
                                                        Debug.Assert(!targetResource.GetHandle(channel).IsNull);
                                                        // resource shouldn't be null because
                                                        // 1) If the field is one of our collections, we don't allow null elements
                                                        // 2) Codegen already made sure the collection contains DUCE.IResources
                                                        // ... so we'll Assert it

                                                        if (newCollection != null)
                                                        {
                                                            int count = newCollection.Count;
                                                            for (int i = 0; i < count; i++)
                                                            {
                                                                DUCE.IResource resource = newCollection.Internal_GetItem(i) as DUCE.IResource;
                                                                Debug.Assert(resource != null);
                                                                resource.AddRefOnChannel(channel);
                                                            }
                                                        }

                                                        if (oldCollection != null)
                                                        {
                                                            int count = oldCollection.Count;
                                                            for (int i = 0; i < count; i++)
                                                            {
                                                                DUCE.IResource resource = oldCollection.Internal_GetItem(i) as DUCE.IResource;
                                                                Debug.Assert(resource != null);
                                                                resource.ReleaseOnChannel(channel);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                    [[/inline]]
                                    );
                            }
                        }
                    }
                    else
                    {
                        cs.Write(
                            [[inline]]
                                private static void [[field.PropertyName]]PropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
                                {
                                    [[resource.Name]] target = (([[resource.Name]]) d);
                                    [[conditional(field.CachedLocally)]]
                                    target._cached[[field.PropertyName]]Value = ([[field.Type.ManagedName]])e.NewValue;
                                    [[/conditional]]
                                    [[conditional(field.PropertyChangedHook)]]
                                    target.[[field.PropertyName]]PropertyChangedHook(e);
                                    [[/conditional]]
                            [[/inline]]
                            );
                    }

                    //
                    //  Build the duceUpdate string.  This contains the AddRef/Release required for
                    //  properties backed by resources.
                    //

                    if (NeedsDucePropertyUpdate(resource, field))
                    {
                        if(   (resource.Name == "VisualBrush" || resource.Name == "BitmapCacheBrush") 
                           && field.Type.ManagedName == "Visual")
                        {
                            cs.Write(
                                [[inline]]
                                        [[field.Type.ManagedName]] oldV = ([[field.Type.ManagedName]]) e.OldValue;

                                        //
                                        // If the Visual required layout but it is changed before we do Layout
                                        // on that Visual, then we dont want the async LayoutCallback method to run,
                                        // nor do we want the LayoutUpdated handler to run. So we abort/remove them.
                                        //
                                        if (target._pendingLayout)
                                        {
                                            //
                                            // Visual has to be a UIElement since _pendingLayout flag is
                                            // true only if we added the LayoutUpdated handler which can
                                            // only be done on UIElement.
                                            //
                                            UIElement element = (UIElement)oldV;
                                            Debug.Assert(element != null);
                                            element.LayoutUpdated -= target.OnLayoutUpdated;

                                            Debug.Assert(target._DispatcherLayoutResult != null);
                                            Debug.Assert(target._DispatcherLayoutResult.Status == System.Windows.Threading.DispatcherOperationStatus.Pending);
                                            bool abortStatus = target._DispatcherLayoutResult.Abort();
                                            Debug.Assert(abortStatus);

                                            target._pendingLayout = false;
                                        }

                                        [[field.Type.ManagedName]] newV = ([[field.Type.ManagedName]]) e.NewValue;
                                        System.Windows.Threading.Dispatcher dispatcher = target.Dispatcher;

                                        if (dispatcher != null)
                                        {
                                            DUCE.IResource targetResource = (DUCE.IResource)target;
                                            using (CompositionEngineLock.Acquire())
                                            {
                                                int channelCount = targetResource.GetChannelCount();

                                                for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
                                                {
                                                    DUCE.Channel channel = targetResource.GetChannel(channelIndex);
                                                    Debug.Assert(!channel.IsOutOfBandChannel);
                                                    Debug.Assert(!targetResource.GetHandle(channel).IsNull);
                                                    target.ReleaseResource(oldV,channel);
                                                    target.AddRefResource(newV,channel);
                                                }
                                            }
                                        }

                                [[/inline]]
                                );
                        }
                        else
                        {
                            cs.Write(
                                [[inline]]
                                        [[field.Type.ManagedName]] oldV = ([[field.Type.ManagedName]]) e.OldValue;
                                        [[field.Type.ManagedName]] newV = ([[field.Type.ManagedName]]) e.NewValue;
                                        System.Windows.Threading.Dispatcher dispatcher = target.Dispatcher;

                                        if (dispatcher != null)
                                        {
                                            DUCE.IResource targetResource = (DUCE.IResource)target;
                                            using (CompositionEngineLock.Acquire())
                                            {
                                                int channelCount = targetResource.GetChannelCount();

                                                for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
                                                {
                                                    DUCE.Channel channel = targetResource.GetChannel(channelIndex);
                                                    Debug.Assert(!channel.IsOutOfBandChannel);
                                                    Debug.Assert(!targetResource.GetHandle(channel).IsNull);
                                                    target.ReleaseResource(oldV,channel);
                                                    target.AddRefResource(newV,channel);
                                                }
                                            }
                                        }

                                [[/inline]]
                                );
                        }
                    }

                    if (needToUnindent)
                    {
                        cs.Unindent();

                        cs.WriteBlock(
                            [[inline]]
                                }
                            [[/inline]]);
                    }

                    if (resource.HasUnmanagedResourceOrDerivesFromTypeWhichHasUnmanagedResource || field.IsAnimated)
                    {
                        cs.Write(
                            [[inline]]
                                    target.PropertyChanged([[field.IsAliased ? field.PropertyAlias : field.DPPropertyName]]);
                            [[/inline]]
                                );
                    }

                    cs.Write(
                            [[inline]]
                                }
                            [[/inline]]
                                );
                }
           }

            return cs.ToString();
        }

        private string WriteRegisterDPProperty(McgResource resource)
        {
            if (resource.SkipProperties) return String.Empty;
            if (resource.IsValueType) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                if (field.IsAliased || field.IsUnmanagedOnly) continue;

                if (field.IsAnimated && resource.InlinedUnmanagedResource)
                {
                    string msg = resource.Name + "." + field.Name + " cannot be independently animated because " + resource.Name + " is an inlined resource.";
                    throw new InvalidOperationException(msg);
                }

                string visibility = field.Visibility;
                string isIndependentlyAnimated = [[inline]][[field.IsAnimated]][[/inline]];
                isIndependentlyAnimated = isIndependentlyAnimated.ToLower();

                string propertyChangedCallback = String.Empty;

                if (NeedsPropertyChangedCallback(resource, field))
                {
                    propertyChangedCallback = "new PropertyChangedCallback(" + field.PropertyName + "PropertyChanged)";
                }
                else
                {
                    propertyChangedCallback = "null";
                }

                cs.Write(
                    [[inline]]
                        /// <summary>
                        ///     The DependencyProperty for the [[resource.Name]].[[field.PropertyName]] property.
                        /// </summary>
                        [[visibility]] static readonly DependencyProperty [[field.DPPropertyName]];
                    [[/inline]]
                    );

                _staticCtorText.Write(
                    [[inline]]
                        [[field.DPPropertyName]] =
                              RegisterProperty("[[field.PropertyName]]",
                                               typeof([[field.Type.ManagedName]]),
                                               typeofThis,
                                               [[GetDefaultValue(field)]],
                                               [[propertyChangedCallback]],
                    [[/inline]]
                );

                // Conditionally emit validation logic.  If there is validation explicitly called for
                // by the field, register the static method on this type.
                if (field.IsValidate)
                {
                    _staticCtorText.Write(
                        [[inline]]
                                                   new ValidateValueCallback(Validate[[field.PropertyName]]Value),
                        [[/inline]]
                        );
                }
                // Otherwise, if it's an enum, we can just call directly into the enum validation code.
                else if (field.Type is McgEnum)
                {
                    _staticCtorText.Write(
                        [[inline]]
                                                   new ValidateValueCallback([[((McgEnum)field.Type).ManagedNamespace]].ValidateEnums.Is[[field.Type.ManagedName]]Valid),
                        [[/inline]]
                        );
                }
                else
                {
                    _staticCtorText.Write(
                        [[inline]]
                                                   null,
                        [[/inline]]
                        );
                }

                _staticCtorText.Write(
                    [[inline]]
                                               /* isIndependentlyAnimated  = */ [[isIndependentlyAnimated]],
                                               /* coerceValueCallback */ [[field.CoerceValueCallback == null ? "null" : "new CoerceValueCallback(" + field.CoerceValueCallback + ")"]]);
                    [[/inline]]
                    );
            }

            return cs.ToString();
        }

        private string GetDefaultValue(McgField field)
        {
            if (field.Default != "")
            {
                if (field.Type is McgResource && ((McgResource)field.Type).IsCollection)
                {
                    return "new FreezableDefaultValueFactory(" + field.Default + ")";
                }
                else
                {
                    return field.Default;
                }
            }
            else
            {
                return "null";
            }
        }

        private bool UseStaticDefaultFieldInitializer(McgField field)
        {
            return field.Default != String.Empty
                && (field.Type is McgResource || field.Type is McgEnum)
                && !field.IsUnmanagedOnly;
        }

        private string GetDefaultFieldName(McgField field)
        {
            if (!UseStaticDefaultFieldInitializer(field))
            {
                throw new Exception("Attempt to look up default value field for a field which does not have one ('" + field.Name + "')");
            }

            McgResource fieldResource = field.Type as McgResource;

            // If the field's type can be stored in a const var, we'll do so.
            if (fieldResource == null || fieldResource.CanBeConst)
            {
                return "c_" + field.PropertyName;
            }
            // Otherwise, we'll store it as a static.
            else
            {
                return "s_" + field.PropertyName;
            }
        }

        private string WriteLocallyCachedProperties(McgResource resource)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach (McgField field in resource.LocalFields)
            {
                if (field.CachedLocally)
                {
                    cs.WriteBlock(
                        [[inline]]private [[field.Type.ManagedName]] _cached[[field.PropertyName]]Value = [[field.Default]];[[/inline]]
                        );
                }
            }

            return cs.ToString();
        }

        /// <summary>
        /// Write declarations for cache fields, initializing to a
        /// default value if appropriate.
        /// </summary>
        private string WriteCacheDecls(McgResource resource)
        {
            if (resource.SkipFields) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalNonNewFields)
            {
                if (field.IsUnmanagedOnly) continue;

                if (UseStaticDefaultFieldInitializer(field))
                {
                    cs.WriteBlock(
                        [[inline]]internal [[field.Type.ManagedName]] [[field.InternalName]] = [[GetDefaultFieldName(field)]];[[/inline]]
                    );
                }
                else
                {
                    cs.WriteBlock(
                        [[inline]]internal [[field.Type.ManagedName]] [[field.InternalName]];[[/inline]]
                    );
                }
            }

            return cs.ToString();
        }

        private string WriteDefaultValues(McgResource resource)
        {
            if (resource.SkipProperties) return String.Empty;
            if (resource.IsValueType) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                if (field.IsUnmanagedOnly) continue;

                if (UseStaticDefaultFieldInitializer(field))
                {
                    string modifier;
                    McgResource fieldResource = field.Type as McgResource;

                    // If the field's type can be stored in a const var, we'll do so.
                    if (fieldResource == null || fieldResource.CanBeConst)
                    {
                        modifier = "const";
                    }
                    // Otherwise, we'll store it as a static.
                    else
                    {
                        modifier = "static";
                    }

                    cs.WriteBlock(
                        [[inline]]internal [[modifier]] [[field.Type.ManagedName]] [[GetDefaultFieldName(field)]] = [[field.Default]];[[/inline]]
                    );
                }
            }

            return cs.ToString();
        }

        private string WriteStaticCtor(McgResource resource)
        {
            if (_staticCtorText.IsEmpty)
            {
                if (resource.SkipProperties) return String.Empty;
                if (resource.IsValueType) return String.Empty;
            }

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                if (field.IsUnmanagedOnly) continue;

                if (field.Default != "")
                {
                    McgResource fieldResource = field.Type as McgResource;
                    if (fieldResource != null && fieldResource.IsFreezable)
                    {
                        cs.WriteBlock(
                            [[inline]]
                                Debug.Assert([[GetDefaultFieldName(field)]] == null || [[GetDefaultFieldName(field)]].IsFrozen,
                                    "Detected context bound default value [[resource.Name]].[[GetDefaultFieldName(field)]].");

                            [[/inline]]
                        );
                    }
                }

                // Override metadata on any aliased properties
                if (field.IsAliased)
                {
                    cs.WriteBlock(
                        [[inline]]
                            [[field.PropertyAlias]].OverrideMetadata(
                                typeof([[resource.ManagedName]]),
                                new UIPropertyMetadata([[GetDefaultValue(field)]],
                                                       new PropertyChangedCallback([[field.PropertyName]]PropertyChanged)));
                        [[/inline]]
                        );
                }
            }

            // Used to append to the generated static constructor.
            string staticCtorText = _staticCtorText.ToString();
            if (resource.UseStaticInitialize)
            {
                staticCtorText = "StaticInitialize(typeofThis);\n" + staticCtorText;
            }
            
            if (!cs.IsEmpty || !_staticCtorText.IsEmpty)
            {
                return
                    [[inline]]
                        static [[resource.Name]]()
                        {
                            // We check our static default fields which are of type Freezable
                            // to make sure that they are not mutable, otherwise we will throw
                            // if these get touched by more than one thread in the lifetime
                            // of your app.
                            //
                            [[cs]]

                            // Initializations
                            Type typeofThis = typeof([[resource.Name]]);
                            [[staticCtorText]]
                        }

                    [[/inline]];
            }
            else
            {
                return String.Empty;
            }
        }

        /// <summary>
        /// WriteEffectiveValuesInitialSize - this emits an internal property which simply
        /// returns a size as a hint to the initial size of the EffectiveValueCache.
        /// It is the count of the fields/properties on this resource which are marked as
        /// commonly set (field.IsCommonlySet)
        /// </summary>
        private string WriteEffectiveValuesInitialSize(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;

            // The default value is 2, so we don't need to emit this if the count == 2.
            if ((resource.CommonlySetFieldCount > 0) &&
                (resource.CommonlySetFieldCount != 2))
            {
                StringCodeSink cs = new StringCodeSink();

                cs.Write(
                    [[inline]]
                        //
                        //  This property finds the correct initial size for the _effectiveValues store on the
                        //  current DependencyObject as a performance optimization
                        //
                        //  This includes:
                    [[/inline]]
                    );

                foreach (McgField field in resource.CommonlySetFields)
                {
                    cs.Write(
                        [[inline]]
                            //    [[field.PropertyName]]
                        [[/inline]]
                        );
                }

                cs.WriteBlock(
                    [[inline]]
                        //
                        internal override int EffectiveValuesInitialSize
                        {
                            get
                            {
                                return [[resource.CommonlySetFieldCount]];
                            }
                        }
                    [[/inline]]
                        );

                return cs.ToString();
            }
            else
            {
                return String.Empty;
            }
        }

        private string WriteProperties(McgResource resource)
        {
            if (resource.SkipProperties) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                if (field.IsAliased || field.IsUnmanagedOnly) continue;

                if (resource.IsValueType)
                {
                    Debug.Assert(!field.IsAliased, "We can't have aliased properties on values types.");
                    cs.WriteBlock(WriteSimpleProperty(resource, field));
                }
                else
                {
                    cs.WriteBlock(WriteDPProperty(resource, field));
                }
            }

            return cs.ToString();
        }

        /// <summary>
        /// NeedsDucePropertyUpdate -
        /// This method returns whether or not a given field on a given resource needs
        /// to do AddRef/Release/Update to the field when its value is changed.
        /// This needs to occur if the parent and the child are both resources with
        /// unmanaged representations.
        /// </summary>
        private bool NeedsDucePropertyUpdate(McgResource owner, McgField field)
        {
            return owner.HasUnmanagedResource &&
                    (field.Type is McgResource) &&
                    ((McgResource)field.Type).HasUnmanagedResource
                    && !field.IsManagedOnly;
        }

        private string WriteDPProperty(McgResource resource, McgField field)
        {
            StringCodeSink cs = new StringCodeSink();

            string visibility = field.Visibility;
            if (field.IsNew)
            {
                Debug.Assert(!field.IsAliased, "Aliased properties cannot be overriden.");
                visibility+= " new";
            }

            string value;

            // Future: Move this into the XML/OM
            if (field.Type.ManagedName == "bool")
            {
                value = "BooleanBoxes.Box(value)";
            }
            else if (field.Type.ManagedName == "FillRule")
            {
                value = "FillRuleBoxes.Box(value)";
            }
            else
            {
                value = "value";
            }

            string comment = Helpers.CodeGenHelpers.FormatComment(field.Comment,
                                                                  84,
                                                                  "///     " + field.PropertyName + " - " + field.Type.ManagedName + ".  Default value is " +  GetDefaultValue(field) + ".\n",
                                                                  "///     " + field.PropertyName + " - " + field.Type.ManagedName + ".  Default value is " +  GetDefaultValue(field) + ".",
                                                                  true);

            string serializationVisibility = field.SerializationVisibility ? "" :
                    "[DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]\n\n";

            string typeConverter = field.TypeConverter == null ? "" :
                    "[TypeConverter(typeof(" + field.TypeConverter + "))]\n\n";

            string getValue = String.Empty;
            if (!field.CachedLocally)
            {
                getValue =
                    [[inline]]
                        return ([[field.Type.ManagedName]]) GetValue([[field.DPPropertyName]]);
                    [[/inline]];
            }
            else
            {
                getValue =
                    [[inline]]
                        [[conditional(resource.IsFreezable)]]ReadPreamble();[[/conditional]]
                        return _cached[[field.PropertyName]]Value;
                    [[/inline]];
            }

            cs.WriteBlock(
                [[inline]]
                    /// <summary>
                    [[comment]]
                    /// </summary>
                    [[typeConverter]][[serializationVisibility]][[visibility]] [[field.Type.ManagedName]] [[field.PropertyName]]
                    {
                        get
                        {
                            [[getValue]]
                        }
                        set
                        {
                            SetValueInternal([[field.DPPropertyName]], [[value]]);
                        }
                    }
                [[/inline]]
                );

            return cs.ToString();
        }

        // Isn't non-changeable = simple?
        private string WriteSimpleProperty(McgResource resource, McgField field)
        {
            StringCodeSink cs = new StringCodeSink();

            string fieldName = field.InternalName;

            string visibility = field.Visibility;
            if (field.IsNew) visibility+= " new";

            string comment = Helpers.CodeGenHelpers.FormatComment(field.Comment,
                                                                  84,
                                                                  "///     " + field.PropertyName + " - " + field.Type.ManagedName + "\n",
                                                                  "///     " + field.PropertyName + " - " + field.Type.ManagedName + ".  Default value is 0.",
                                                                  true);

            cs.WriteBlock(
                [[inline]]
                    /// <summary>
                    [[comment]]
                    /// </summary>
                    [[visibility]] [[field.Type.ManagedName]] [[field.PropertyName]]
                    {
                        get
                        {
                            return [[fieldName]];
                        }
                [[/inline]]
                );

            if (!field.IsReadOnly)
            {
                cs.WriteBlock(
                [[inline]]
                        set
                        {
                            [[fieldName]] = value;
                        }
                [[/inline]]
                    );
            }

            cs.WriteBlock(
                [[inline]]
                    }
                [[/inline]]
                );

            return cs.ToString();
        }

        private string WriteCreateInstanceCore(McgResource resource)
        {
            if (resource.IsAbstract || resource.IsValueType) return String.Empty;

            if (resource.CreateInstanceCoreViaActivator)
            {
                return
                    [[inline]]
                        /// <summary>
                        /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                        /// </summary>
                        /// <returns>The new Freezable.</returns>
                        protected override Freezable CreateInstanceCore()
                        {
                            // [[resource.Name]] is an abstract type, so
                            // explicitly create the invoking type via
                            // Activator. 
                            return (Freezable)Activator.CreateInstance(this.GetType());
                        }
                    [[/inline]];
            }
            else if (resource.IsFreezable || resource.IsAnimatable)
            {
                return
                    [[inline]]
                        /// <summary>
                        /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                        /// </summary>
                        /// <returns>The new Freezable.</returns>
                        protected override Freezable CreateInstanceCore()
                        {
                            return new [[resource.Name]]();
                        }
                    [[/inline]];
            }
            else
            {
                return String.Empty;
            }
        }

        private string WriteResourceHandleField(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            if (!resource.IsAbstract)
            {
                // PointLight is not abstract, but SpotLight inherits from it.
                string newKeyword = "";
                McgResource parent = resource.Extends as McgResource;

                if (parent != null && !parent.IsAbstract)
                {
                    newKeyword = "new ";
                }

                cs.WriteBlock(
                    [[inline]]
                        internal [[newKeyword]]System.Windows.Media.Composition.DUCE.MultiChannelResource _duceResource = new System.Windows.Media.Composition.DUCE.MultiChannelResource();
                    [[/inline]]
                );
            }

            return cs.ToString();
        }


        /// <summary>
        /// GetMarshalType - this returns the name of the type which represents marshalling
        /// the "type" to a packet.  For example, double becomes double, because it doesn't
        /// need conversion, but Point3D becomes MilPoint3F, etc.  Reference types
        /// become DUCE.IResource.
        /// </summary>
        /// <param name="type"> The McgType which will be marshaled. </param>
        private string GetMarshalType(McgType type)
        {
            // The size of the collection is the size of each marshalled element times the count,
            // or 0 if the collection is null.
            string elementTypeString = String.Empty;

            // If the resource's collection elements are value types, then the size of each element
            // is the size of the type.
            if (type.IsValueType)
            {
                // If the type is converted, we care about the size of unmanaged type
                if (type.NeedsConvert)
                {
                    elementTypeString = type.MarshalUnmanagedType;
                }
                // Otherwise, we care about the size of the managed type
                else
                {
                    elementTypeString = type.Name;
                }
            }
            // Otherwise, the size is the size of a Handle.  Note that this assumes that no managed
            // reference types are marshaled as unmanaged value types.
            else
            {
                // There's no particular reason why we can't have a managed reference type which
                // is marshalled as an unmanaged value type, but there are assumptions throughout
                // codegen which assume that this is the case.  Assert this here.
                Debug.Assert(!type.NeedsConvert);
                elementTypeString = "DUCE.ResourceHandle";
            }

            return elementTypeString;
        }

        /// <summary>
        /// WriteCollectionMarshal - this method produces a string in the form of:
        ///
        /// for(int i = 0; i < pointCollectionCount; i++)
        /// {
        ///     *((Point*)(pBuffer + cbPos)) = pointCollection.Internal_GetItem(i);
        ///     cbPos += sizeof(Point);
        /// }
        ///
        /// Where "pointCollectionCount" is specified in the "countVariable" parameter, "Point" is the
        /// marshal type of the elements of the collection, as specified in the "collectionType" variable,
        /// and "pointCollection" is the name of the collection, as specified in the "collectionName"
        /// parameter.
        /// </summary>
        /// <param name="collectionType"> The McgType which describes elements of the collection </param>
        /// <param name="collectionName"> The name of the collection, typically a local variable.</param>
        /// <param name="countVariable">
        ///     A string which will be used as the element count of the collection, typically a local
        ///     variable.  If, at runtime, thie evaluates to 0, the collection will not be dereference,
        ///     and thus may be null.
        /// </param>
        /// <param name="useInternalGetItem">
        ///     If true, members of the colleciton will be retrieved with "Internal_GetChildren(int)".
        ///     Otherwise, they will be retrieved via the indexer.
        /// </param>
        private string WriteCollectionMarshal(McgType collectionType,
                                              string collectionName,
                                              string countVariable,
                                              bool useInternalGetItem)
        {
            // The size of the collection is the size of each marshalled element times the count,
            // or 0 if the collection is null.
            string marshaledCollectionElement = String.Empty;
            string indexString = String.Empty;

            if (useInternalGetItem)
            {
                indexString = ".Internal_GetItem(i)";
            }
            else
            {
                indexString = "[i]";
            }

            // If the resource's collection elements are value types, then the size of each element
            // is the size of the type.
            if (collectionType.IsValueType)
            {
                // If the type is converted, we care about the size of unmanaged type
                if (collectionType.NeedsConvert)
                {
                    marshaledCollectionElement = CodeGenHelpers.ConvertToValueType(collectionType, collectionName + indexString);
                }
                // Otherwise, we care about the size of the managed type
                else
                {
                    marshaledCollectionElement = collectionName + indexString;
                }
            }
            // Otherwise, the size is the size of a Handle.  Note that this assumes that no managed
            // reference types are marshaled as unmanaged value types.
            else
            {
                marshaledCollectionElement = "((DUCE.IResource)" + collectionName + indexString + ").GetHandle(channel);";
            }

            return
                [[inline]]
                        // Copy this collection's elements (or their handles) to reserved data
                        for(int i = 0; i < [[countVariable]]; i++)
                        {
                            [[GetMarshalType(collectionType)]] resource = [[marshaledCollectionElement]];
                            channel.AppendCommandData(
                                (byte*)&resource,
                                sizeof([[GetMarshalType(collectionType)]])
                                );
                        }
                [[/inline]]
                ;
        }

        private string WriteUpdateResource(McgResource resource)
        {
            StringCodeSink cs = new StringCodeSink();
            StringCodeSink duceUpdate = new StringCodeSink();

            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            if (!resource.IsAbstract && (resource.IsCollection || resource.AllUceFields.Length > 0))
            {
                if (resource.SkipUpdate)
                {
                    cs.WriteBlock(
                        [[inline]]
                            /// <SecurityNote>
                            ///     Critical: This code calls into an unsafe code block
                            ///     TreatAsSafe: This code does not return any critical data.It is ok to expose
                            ///     Channels are safe to call into and do not go cross domain and cross process
                            /// </SecurityNote>
                            [SecurityCritical,SecurityTreatAsSafe]
                            internal override void UpdateResource(DUCE.Channel channel, bool skipOnChannelCheck)
                            {
                                ManualUpdateResource(channel, skipOnChannelCheck);
                                base.UpdateResource(channel, skipOnChannelCheck);
                            }
                        [[/inline]]
                    );

                    return cs.ToString();
                }

                //
                //
                //
                // Generate content of 1-parameter UpdateResource(DUCE.Channel) method
                //
                //
                //

                //
                //
                //
                // Generate code to read data needed for packing into local variables
                //
                //

                //
                // Generate code to read all reference-type properties into a local variable,
                // unless they are managed only.
                //
                // If they aren't managed-only, then we will need to do something to marshal them
                // during UpdateResource.
                McgField[] marshaledReferenceTypes = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsReferenceTypeAndNotManagedOnly);
                if (marshaledReferenceTypes.Length > 0)
                {
                    // E.g., Transform vTransform = Internal_transform;
                    string readPropertyString = "{managedType} v{propertyName} = {marshalValue};";
                    duceUpdate.WriteBlock(
                        [[inline]]
                            // Read values of properties into local variables
                            [[Helpers.CodeGenHelpers.WriteFieldStatements(marshaledReferenceTypes, readPropertyString)]]
                        [[/inline]]
                        );
                }

                //
                // Generate code to obtain handles for properties that have unmanaged resources
                //
                McgField[] resourceFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.HasUnmanagedResourceNoVisual);
                if (resourceFields.Length > 0)
                {
                    duceUpdate.Write(
                        [[inline]]
                            // Obtain handles for properties that implement DUCE.IResource
                        [[/inline]]
                            );

                    foreach (McgField resourceField in resourceFields)
                    {
                        if (resourceField.Type.HasMarshalledIdentity)
                        {
                            // Generate code that checks for an identity (e.g, Transform.Identity) before assigning
                            // the handle. If it is a MarshalledIdentity, we assign null instead of obtaining
                            // the handle.

                            if (resourceField.Type.IsValueType)
                            {
                                // Cannot replace identity with ResourceHandle.Null if the
                                // type doesn't have a handle.
                                throw new NotSupportedException("HasMarshalledIdentity is not supported on value types");
                            }

                            duceUpdate.Write(
                                [[inline]]
                                    DUCE.ResourceHandle h[[resourceField.PropertyName]];
                                    if (v[[resourceField.PropertyName]] == null ||
                                        Object.ReferenceEquals(v[[resourceField.PropertyName]], [[resourceField.Type.MarshalledIdentity]])
                                        )
                                    {
                                        h[[resourceField.PropertyName]] = DUCE.ResourceHandle.Null;
                                    }
                                    else
                                    {
                                        h[[resourceField.PropertyName]] = ((DUCE.IResource)v[[resourceField.PropertyName]]).GetHandle(channel);
                                    }
                                [[/inline]]
                                );
                        }
                        else
                        {
                            // Genenerate handle assignment without checking for a MarshalledIdentity
                            // E.g., DUCE.ResourceHandle hTransform = vTransform != null ? ((DUCE.IResource)vTransform).GetHandle(channel) : DUCE.ResourceHandle.Null;
                            duceUpdate.Write(
                                [[inline]]
                                    DUCE.ResourceHandle h[[resourceField.PropertyName]] = v[[resourceField.PropertyName]] != null ? ((DUCE.IResource)v[[resourceField.PropertyName]]).GetHandle(channel) : DUCE.ResourceHandle.Null;
                                [[/inline]]
                                );
                        }
                    }

                    // Append a newline to sepearte the handle assignment block from the next block
                    duceUpdate.Write("\n");
                }

                McgField[] visualFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsVisualAndNotManagedOnly);
                if (visualFields.Length > 0)
                {
                    string visualHandleString =
                        [[inline]]
                            DUCE.ResourceHandle  h{propertyName} = v{propertyName} != null ? ((DUCE.IResource)v{propertyName}).GetHandle(channel) : DUCE.ResourceHandle.Null;
                        [[/inline]];

                    duceUpdate.WriteBlock(
                        [[inline]]
                            // Obtain handles for properties that implement DUCE.IResource
                            [[Helpers.CodeGenHelpers.WriteFieldStatements(visualFields, visualHandleString)]]
                        [[/inline]]
                        );
                }

                //
                // Generate code to obtain handles for animated properties
                //
                McgField[] animatedFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsAnimated);
                if (animatedFields.Length > 0)
                {
                    // E.g., DUCE.ResourceHandle hOpacityAnimations = GetAnimationResourceHandle(OpacityProperty, channel);
                    string animationHandleString = "DUCE.ResourceHandle h{propertyName}Animations = GetAnimationResourceHandle({propertyName}Property, channel);";
                    duceUpdate.WriteBlock(
                        [[inline]]
                            // Obtain handles for animated properties
                            [[Helpers.CodeGenHelpers.WriteFieldStatements(animatedFields, animationHandleString)]]
                        [[/inline]]
                        );
                }

                //
                // Generate code to create local variables for the counts of this resource
                // (if it is a collection) and for each contained collection which must be
                // marshalled inline.
                //

                if (resource.IsCollection)
                {
                    duceUpdate.WriteBlock(
                        [[inline]]
                            // Store the count of this resource's collection in a local variable.
                            int collectionCount = _collection.Count;
                        [[/inline]]
                        );
                }

                // Obtain array of fields with collections that need to be marshaled in-line
                McgField[] containedCollectionFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsCollectionAndDoesntHaveUnmanagedResource);
                if (containedCollectionFields.Length > 0)
                {
                    // E.g., pointsCount = (vPoints == null) ? 0 : vPoints.Count;
                    string collectionCountstring = "int {localName}Count = (v{propertyName} == null) ? 0 : v{propertyName}.Count;";

                    duceUpdate.WriteBlock(
                        [[inline]]
                            // Store the count of this resource's contained collections in local variables.
                            [[Helpers.CodeGenHelpers.WriteFieldStatements(containedCollectionFields, collectionCountstring)]]
                        [[/inline]]
                        );
                }

                //
                //
                // Declare instance of command packet and generate header assignments
                //
                //

                //
                // Construct name of command packet structure
                //
                string milcmdStruct = "DUCE.MILCMD_" + resource.ManagedName.ToUpper();

                duceUpdate.Write(
                    [[inline]]
                        // Pack & send command packet
                        [[milcmdStruct]] data;
                        unsafe
                        {
                            data.Type = MILCMD.MilCmd[[resource.Name]];
                            data.Handle = _duceResource.GetHandle(channel);
                    [[/inline]]
                    );

                //
                //
                // Generate field assignments for command packet
                //
                //

                // Create a delimited list to append " + " to data size calculations.
                // As we iterate through the fields on this resource, we will append
                // calculations for any additional data the field requires to the list.
                //
                // Using a delimited list for this is helpful because it won't append a " + "
                // after the last calculation.
                DelimitedList additionalDataCalculation = new DelimitedList(" + ", DelimiterPosition.AfterItem, true);

                if (resource.IsCollection)
                {
                    // E.g., data.Size = sizeof(DUCE.ResourceHandle) * _children.Count
                    string collectionSizeString = "sizeof(" + GetMarshalType(resource.CollectionType) + ") * collectionCount";

                    // Generate assignment of the collection size
                    duceUpdate.Write(
                        [[inline]]
                                data.Size = (uint)([[collectionSizeString]]);
                        [[/inline]]
                        );

                    // Add size of this collection to the additional data size calculation
                    additionalDataCalculation.Append("data.Size");
                }

                //
                // Iterate over the UCE fields, and create field assignments for each of them.
                //
                foreach (McgField field in resource.AllUceFields)
                {
                    McgResource resourceType = field.Type as McgResource;

                    // If this field is a collection, marshal the contents in-line if the collection itself
                    // isn't a resource
                    if (ResourceModel.IsCollectionAndDoesntHaveUnmanagedResource(field))
                    {
                        string collectionContentsSize = "sizeof(" + GetMarshalType(resourceType.CollectionType) + ") * " + field.Name + "Count";

                        // E.g., data.ChildrenSize = (UInt32)(sizeof(DUCE.ResourceHandle) * vChildren.Count);
                        duceUpdate.Write(
                            [[inline]]
                                    data.[[field.Name]]Size = (uint)([[collectionContentsSize]]);
                            [[/inline]]
                            );

                        // Also append calculation of additional data size needed for this in-line collection
                        additionalDataCalculation.Append("data." + field.PropertyName + "Size");
                    }
                    // Is this field a reference type?
                    else if (!field.Type.IsValueType)
                    {
                        // E.g., data.hTransform = hTransform;
                        duceUpdate.Write(
                            [[inline]]
                                    data.h[[field.Name]] = h[[field.PropertyName]];
                            [[/inline]]
                            );
                    }
                    // Otherwise the field is a value-type.  If independently animated animated we send either the animation
                    // resource or the value, not both, just for efficiency's sake.
                    else if (field.IsAnimated)
                    {
                        // E.g., data.Opacity = Internal_opacity;
                        duceUpdate.Write(
                            [[inline]]
                                    if (h[[field.PropertyName]]Animations.IsNull)
                                    {
                                        data.[[field.Name]] = [[Helpers.CodeGenHelpers.ConvertToValueType(field.Type, field.PropertyName)]];
                                    }
                                    data.h[[field.PropertyName]]Animations = h[[field.PropertyName]]Animations;
                            [[/inline]]
                            );
                    }
                    else if (field.IsAliased)
                    {
                        // E.g., data.PropertyName = GetValue(PropertyAlias)
                        duceUpdate.Write(
                            [[inline]]
                                    data.[[field.Name]] = ([[field.Type.ManagedName]])GetValue([[field.PropertyAlias]]);
                            [[/inline]]
                            );
                    }
                    else
                    {
                        // E.g., data.NotAnimatedFloat = notAnimatedFloat;
                        duceUpdate.Write(
                            [[inline]]
                                    data.[[field.Name]] = [[Helpers.CodeGenHelpers.ConvertToValueType(field.Type, field.PropertyName)]];
                            [[/inline]]
                            );
                    }
                }

                //
                //
                // Generate SendCommand*
                //
                //

                // If this resource is a collection or contains collections, use
                // SendCommandReserveData to marshal the additional data
                if (resource.IsCollection ||
                    (containedCollectionFields.Length > 0))
                {
                    // Generate code that calls SendCommandReserveData
                    duceUpdate.WriteBlock(
                        [[inline]]

                                channel.BeginCommand(
                                    (byte*)&data,
                                    sizeof([[milcmdStruct]]),
                                    (int)([[additionalDataCalculation.ToString()]])
                                    );

                        [[/inline]]
                        );

                    // If this resource is a collection, marshal it's elements
                    if (resource.IsCollection)
                    {
                        duceUpdate.WriteBlock(WriteCollectionMarshal(resource.CollectionType,
                                                                     "_collection",
                                                                     "collectionCount",
                                                                     false /* do not useInternalGetItem */));
                    }

                    // Now do the same for any collection properties
                    foreach(McgField field in containedCollectionFields)
                    {
                        if (field.IsUnmanagedOnly) continue;

                        duceUpdate.WriteBlock(WriteCollectionMarshal(((McgResource)field.Type).CollectionType,
                                                                     "v" + field.PropertyName,
                                                                     field.Name + "Count",
                                                                     true /* useInternalGetItem */));
                    }

                    duceUpdate.WriteBlock(
                        [[inline]]
                                channel.EndCommand();
                        [[/inline]]
                        );

                }
                // Use SendCommand if no additional data needs to be marshalled.
                else
                {
                    duceUpdate.WriteBlock(
                        [[inline]]

                                // Send packed command structure
                                channel.SendCommand(
                                    (byte*)&data,
                                    sizeof([[milcmdStruct]]));
                        [[/inline]]
                    );
                }

                //
                //
                //
                // Write 1-parameter UpdateResource(DUCE.Channel) method
                //
                //
                //

                cs.WriteBlock(
                    [[inline]]
                        /// <SecurityNote>
                        ///     Critical: This code calls into an unsafe code block
                        ///     TreatAsSafe: This code does not return any critical data.It is ok to expose
                        ///     Channels are safe to call into and do not go cross domain and cross process
                        /// </SecurityNote>
                        [SecurityCritical,SecurityTreatAsSafe]
                        internal override void UpdateResource(DUCE.Channel channel, bool skipOnChannelCheck)
                        {
                            // If we're told we can skip the channel check, then we must be on channel
                            Debug.Assert(!skipOnChannelCheck || _duceResource.IsOnChannel(channel));

                            if (skipOnChannelCheck || _duceResource.IsOnChannel(channel))
                            {
                                base.UpdateResource(channel, skipOnChannelCheck);

                                [[duceUpdate]]
                                }
                            }
                        }
                    [[/inline]]
                );
             }

            return cs.ToString();
        }

        private string WriteAddRefOnChannel(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            if (resource.IsAbstract)
            {
                if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
                {
                    return
                        [[inline]]
                            internal abstract DUCE.ResourceHandle AddRefOnChannelCore(DUCE.Channel channel);

                            /// <summary>
                            /// AddRefOnChannel
                            /// </summary>
                            DUCE.ResourceHandle DUCE.IResource.AddRefOnChannel(DUCE.Channel channel)
                            {
                                // Reconsider the need for this lock when removing the MultiChannelResource.
                                using (CompositionEngineLock.Acquire())
                                {
                                    return AddRefOnChannelCore(channel);
                                }
                            }
                        [[/inline]];
                }
                else
                {
                    return "";
                }
            }

            string methodName = String.Empty;
            string duceAddRef = String.Empty;
            string duceAddRefAnimations = String.Empty;
            string addRefCollection = String.Empty;
            bool lockThis = false;

            if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
            {
                methodName = "DUCE.ResourceHandle DUCE.IResource.AddRefOnChannel(DUCE.Channel channel)";
                lockThis = true;
            }
            else
            {
                if (resource.UseOnChannelCoreWrapper)
                {
                    methodName = "private DUCE.ResourceHandle GeneratedAddRefOnChannelCore(DUCE.Channel channel)";
                }
                else
                {
                    methodName = "internal override DUCE.ResourceHandle AddRefOnChannelCore(DUCE.Channel channel)";
                }
                lockThis = false;
            }

            McgField[] resourceFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.HasUnmanagedResourceNoVisual);
            McgField[] collectionFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsCollectionOfDuceAndDoesntHaveUnmanagedResource);
            McgField[] visualFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsVisualAndNotManagedOnly);

            string addrefCollectionTemplate =
                [[inline]]

                    {managedType} v{propertyName} = {propertyName};

                    if (v{propertyName} != null)
                    {
                        int count = v{propertyName}.Count;
                        for (int i = 0; i < count; i++)
                        {
                            ((DUCE.IResource) v{propertyName}.Internal_GetItem(i)).AddRefOnChannel(channel);
                        }
                    }
                [[/inline]];

            duceAddRef =
                [[inline]]
                    [[Helpers.CodeGenHelpers.WriteFieldStatements(resourceFields,
                                                          "{managedType} v{propertyName} = {propertyName};\n" +
                                                          "if (v{propertyName} != null) ((DUCE.IResource)v{propertyName}).AddRefOnChannel(channel);")]]
                [[/inline]];

            string visualAddRef = Helpers.CodeGenHelpers.WriteFieldStatements(visualFields,
                                                          "{managedType} v{propertyName} = {propertyName};\n" +
                                                          "if (v{propertyName} != null) v{propertyName}.AddRefOnChannelForCyclicBrush(this, channel);");
            if (visualAddRef != String.Empty)
            {
                duceAddRef = duceAddRef + visualAddRef;
            }

            duceAddRef = duceAddRef +
                [[inline]]
                    [[Helpers.CodeGenHelpers.WriteFieldStatements(collectionFields, addrefCollectionTemplate)]]
                [[/inline]];

            if (resource.IsAnimatable)
            {
                duceAddRefAnimations = "AddRefOnChannelAnimations(channel);";
            }

            if (resource.IsCollection)
            {
                McgResource collectionType = resource.CollectionType as McgResource;

                if ((collectionType != null) &&
                    collectionType.HasUnmanagedResource)
                {
                    addRefCollection =
                        [[inline]]
                            for (int i=0; i<_collection.Count; i++)
                            {
                                ((DUCE.IResource) _collection[i]).AddRefOnChannel(channel);
                            }
                        [[/inline]];
                }
            }

            cs.WriteBlock(
                [[inline]]
                    [[methodName]]
                    {
                        [[(lockThis ? "using (CompositionEngineLock.Acquire()) \n{" : "")]]
                            if (_duceResource.CreateOrAddRefOnChannel(this, channel, System.Windows.Media.Composition.DUCE.ResourceType.TYPE_[[resource.ManagedName.ToUpper()]]))
                            {
                                [[duceAddRef]]
                                [[duceAddRefAnimations]]
                                [[addRefCollection]]

                                UpdateResource(channel, true /* skip "on channel" check - we already know that we're on channel */ );
                            }

                            return _duceResource.GetHandle(channel);
                        [[(lockThis ? "}" : "")]]
                    }
                [[/inline]]
            );

            return cs.ToString();
        }

        private string WriteReleaseOnChannel(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();
            string duceRelease = String.Empty;

            if (resource.IsAbstract)
            {
                if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
                {
                    return
                        [[inline]]
                            internal abstract void ReleaseOnChannelCore(DUCE.Channel channel);

                            /// <summary>
                            /// ReleaseOnChannel
                            /// </summary>
                            void DUCE.IResource.ReleaseOnChannel(DUCE.Channel channel)
                            {
                                // Reconsider the need for this lock when removing the MultiChannelResource.
                                using (CompositionEngineLock.Acquire())
                                {
                                    ReleaseOnChannelCore(channel);
                                }
                            }
                        [[/inline]];
                }
                else
                {
                    return "";
                }
            }

            string methodName = String.Empty;
            string duceReleaseAnimations = String.Empty;
            string releaseCollection = String.Empty;
            bool lockThis = false;

            if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
            {
                methodName = "void DUCE.IResource.ReleaseOnChannel(DUCE.Channel channel)";
                lockThis = true;
            }
            else
            {
                if (resource.UseOnChannelCoreWrapper)
                {
                    methodName = "private void GeneratedReleaseOnChannelCore(DUCE.Channel channel)";
                }
                else
                {
                    methodName = "internal override void ReleaseOnChannelCore(DUCE.Channel channel)";
                }
                
                lockThis = false;
            }

            McgField[] resourceFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.HasUnmanagedResourceNoVisual);
            McgField[] collectionFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsCollectionOfDuceAndDoesntHaveUnmanagedResource);
            McgField[] visualFields = ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsVisualAndNotManagedOnly);

            string releaseCollectionTemplate =
                [[inline]]

                    {managedType} v{propertyName} = {propertyName};

                    if (v{propertyName} != null)
                    {
                        int count = v{propertyName}.Count;
                        for (int i = 0; i < count; i++)
                        {
                            ((DUCE.IResource) v{propertyName}.Internal_GetItem(i)).ReleaseOnChannel(channel);
                        }
                    }
                [[/inline]];

            duceRelease =
                [[inline]]
                    [[Helpers.CodeGenHelpers.WriteFieldStatements(resourceFields,
                                                          "{managedType} v{propertyName} = {propertyName};\n" +
                                                          "if (v{propertyName} != null) ((DUCE.IResource)v{propertyName}).ReleaseOnChannel(channel);")]]
                [[/inline]];

            string visualRelease = Helpers.CodeGenHelpers.WriteFieldStatements(visualFields,
                                                          "{managedType} v{propertyName} = {propertyName};\n" +
                                                          "if (v{propertyName} != null) v{propertyName}.ReleaseOnChannelForCyclicBrush(this, channel);");

            if (visualRelease != String.Empty)
            {
                duceRelease = duceRelease + visualRelease;
            }

            duceRelease = duceRelease +
                [[inline]]
                    [[Helpers.CodeGenHelpers.WriteFieldStatements(collectionFields, releaseCollectionTemplate)]]
                [[/inline]];

            if (resource.IsAnimatable)
            {
                duceReleaseAnimations = "ReleaseOnChannelAnimations(channel);";
            }

            if (resource.IsCollection)
            {
                McgType collectionType = resource.CollectionType;

                if (collectionType.IsFreezable)
                {
                    releaseCollection =
                        [[inline]]
                            for (int i=0; i<_collection.Count; i++)
                            {
                                ((DUCE.IResource) _collection[i]).ReleaseOnChannel(channel);
                            }
                        [[/inline]];
                }
            }

            cs.WriteBlock(
                [[inline]]
                    [[methodName]]
                    {
                        [[(lockThis ? "using (CompositionEngineLock.Acquire()) \n{" : "")]]
                            Debug.Assert(_duceResource.IsOnChannel(channel));

                            if (_duceResource.ReleaseOnChannel(channel))
                            {
                                [[duceRelease]]
                                [[duceReleaseAnimations]]
                                [[releaseCollection]]
                            }
                        [[(lockThis ? "}" : "")]]
                    }
                [[/inline]]
                );

            return cs.ToString();
        }

        private string WriteGetDuceResource(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();
            StringCodeSink releaseString = new StringCodeSink();

            if (resource.IsAbstract)
            {
                if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
                {
                    return
                        [[inline]]
                            internal abstract DUCE.ResourceHandle GetHandleCore(DUCE.Channel channel);

                            /// <summary>
                            /// GetHandle
                            /// </summary>
                            DUCE.ResourceHandle DUCE.IResource.GetHandle(DUCE.Channel channel)
                            {
                                DUCE.ResourceHandle handle;

                                using (CompositionEngineLock.Acquire())
                                {
                                    handle = GetHandleCore(channel);
                                }

                                return handle;
                            }
                        [[/inline]];
                }
                else
                {
                    return String.Empty;
                }
            }

            string methodName = String.Empty;
            string releaseCollection = String.Empty;

            if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
            {
                methodName = "DUCE.ResourceHandle DUCE.IResource.GetHandle(DUCE.Channel channel)";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            DUCE.ResourceHandle h;
                            // Reconsider the need for this lock when removing the MultiChannelResource.
                            using (CompositionEngineLock.Acquire())
                            {
                                h = _duceResource.GetHandle(channel);
                            }
                            return h;
                        }
                    [[/inline]]
                    );
            }
            else
            {
                methodName = "internal override DUCE.ResourceHandle GetHandleCore(DUCE.Channel channel)";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            // Note that we are in a lock here already.
                            return _duceResource.GetHandle(channel);
                        }
                    [[/inline]]
                    );
            }
            return cs.ToString();
        }

        private string WriteGetChannelCount(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();
            StringCodeSink releaseString = new StringCodeSink();

            if (resource.IsAbstract)
            {
                if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
                {
                    return
                        [[inline]]
                            internal abstract int GetChannelCountCore();

                            /// <summary>
                            /// GetChannelCount
                            /// </summary>
                            int DUCE.IResource.GetChannelCount()
                            {
                                // must already be in composition lock here
                                return GetChannelCountCore();
                            }
                        [[/inline]];
                }
                else
                {
                    return String.Empty;
                }
            }

            string methodName = String.Empty;
            string releaseCollection = String.Empty;

            if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
            {
                methodName = "int DUCE.IResource.GetChannelCount()";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            // must already be in composition lock here
                            return _duceResource.GetChannelCount();
                        }
                    [[/inline]]
                    );
            }
            else
            {
                methodName = "internal override int GetChannelCountCore()";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            // must already be in composition lock here
                            return _duceResource.GetChannelCount();
                        }
                    [[/inline]]
                    );
            }

            return cs.ToString();
        }

        private string WriteGetChannel(McgResource resource)
        {
            if (resource.IsValueType) return String.Empty;
            if (!resource.HasUnmanagedResource) return String.Empty;

            StringCodeSink cs = new StringCodeSink();
            StringCodeSink releaseString = new StringCodeSink();

            if (resource.IsAbstract)
            {
                if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
                {
                    return
                        [[inline]]
                            internal abstract DUCE.Channel GetChannelCore(int index);

                            /// <summary>
                            /// GetChannel
                            /// </summary>
                            DUCE.Channel DUCE.IResource.GetChannel(int index)
                            {
                                // must already be in composition lock here
                                return GetChannelCore(index);
                            }
                        [[/inline]];
                }
                else
                {
                    return String.Empty;
                }
            }

            string methodName = String.Empty;
            string releaseCollection = String.Empty;

            if (!resource.DerivesFromTypeWhichHasUnmanagedResource)
            {
                methodName = "DUCE.Channel DUCE.IResource.GetChannel(int index)";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            // in a lock already
                            return _duceResource.GetChannel(index);
                        }
                    [[/inline]]
                    );
            }
            else
            {
                methodName = "internal override DUCE.Channel GetChannelCore(int index)";

                cs.WriteBlock(
                    [[inline]]
                        [[methodName]]
                        {
                            // Note that we are in a lock here already.
                            return _duceResource.GetChannel(index);
                        }
                    [[/inline]]
                    );
            }

            return cs.ToString();
        }

        private string WriteListMarshalling(McgResource resource)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach(McgField field in resource.LocalFields)
            {
                McgResource fieldResource = field.Type as McgResource;

                if (fieldResource != null
                    && fieldResource.IsFreezable
                    && fieldResource.IsCollectionOfHandles)
                {
                    cs.WriteBlock(
                        [[inline]]

                            private void [[field.PropertyName]]ItemInserted(object sender, object item)
                            {
                                if (this.Dispatcher != null)
                                {
                                    DUCE.IResource thisResource = (DUCE.IResource)this;
                                    using (CompositionEngineLock.Acquire())
                                    {
                                        int channelCount = thisResource.GetChannelCount();

                                        for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
                                        {
                                            DUCE.Channel channel = thisResource.GetChannel(channelIndex);
                                            Debug.Assert(!channel.IsOutOfBandChannel);
                                            Debug.Assert(!thisResource.GetHandle(channel).IsNull);

                                            // We're on a channel, which means our dependents are also on the channel.
                                            DUCE.IResource addResource = item as DUCE.IResource;
                                            if (addResource != null)
                                            {
                                                addResource.AddRefOnChannel(channel);
                                            }

                                            UpdateResource(channel, true /* skip on channel check */);
                                        }
                                    }
                                }
                            }

                            private void [[field.PropertyName]]ItemRemoved(object sender, object item)
                            {
                                if (this.Dispatcher != null)
                                {
                                    DUCE.IResource thisResource = (DUCE.IResource)this;
                                    using (CompositionEngineLock.Acquire())
                                    {
                                        int channelCount = thisResource.GetChannelCount();

                                        for (int channelIndex = 0; channelIndex < channelCount; channelIndex++)
                                        {
                                            DUCE.Channel channel = thisResource.GetChannel(channelIndex);
                                            Debug.Assert(!channel.IsOutOfBandChannel);
                                            Debug.Assert(!thisResource.GetHandle(channel).IsNull);

                                            UpdateResource(channel, true /* is on channel check */);

                                            // We're on a channel, which means our dependents are also on the channel.
                                            DUCE.IResource releaseResource = item as DUCE.IResource;
                                            if (releaseResource != null)
                                            {
                                                releaseResource.ReleaseOnChannel(channel);
                                            }
                                        }
                                    }
                                }
                            }
                        [[/inline]]
                        );
                }
            }

            return cs.ToString();
        }

        //------------------------------------------------------
        //
        //  Helper Methods
        //
        //------------------------------------------------------

        #region Helper Methods

        private string WriteOrFieldExpression(McgField[] fields, string statement)
        {
            OrList olist = new OrList();

            foreach(McgField field in fields)
            {
                olist.Append(Helpers.CodeGenHelpers.WriteFieldStatement(field, statement));
            }

            return olist.ToString();
        }

        private void AppendOrFieldExpression(OrList olist, McgField[] fields, string statement)
        {
            foreach(McgField field in fields)
            {
                olist.Append(Helpers.CodeGenHelpers.WriteFieldStatement(field, statement));
            }
        }

        private string WriteCallBase(McgResource resource, string methodInvokation)
        {
            // Protect against CS0205: Cannot call an abstract base member.
            if (resource.Extends != null)
            {
                return
                    [[inline]]
                        base.[[methodInvokation]];
                    [[/inline]];
            }

            return String.Empty;
        }

        private string WriteLocalAnimatedFieldStatements(McgResource resource, string statement)
        {
            return WriteAnimatedFieldStatements(
                ResourceModel.Filter(resource.LocalFields, ResourceModel.IsAnimated),
                statement,
                /* nullStatement = */ null);
        }

        private string WriteAllAnimatedFieldStatements(McgResource resource, string statement)
        {
            return WriteAllAnimatedFieldStatements(resource, statement, /* nullStatement = */ null);
        }

        private string WriteAllAnimatedFieldStatements(McgResource resource, string notNullStatement, string nullStatement)
        {
            return WriteAnimatedFieldStatements(
                ResourceModel.Filter(resource.AllManagedFields, ResourceModel.IsAnimated),
                notNullStatement,
                nullStatement);
        }


        private string WriteAnimatedFieldStatements(
            McgField[] fields,
            string notNullStatement,
            string nullStatement)
        {
            if (fields.Length == 0) return String.Empty;

            StringCodeSink cs = new StringCodeSink();

            cs.WriteBlock(
                [[inline]]
                    if (_animations != null)
                    {
                        [[Helpers.CodeGenHelpers.WriteFieldStatements(fields, notNullStatement)]]
                    }
                [[/inline]]
            );
            if (nullStatement != null)
            {
                cs.WriteBlock(
                    [[inline]]
                        else
                        {
                            [[Helpers.CodeGenHelpers.WriteFieldStatements(fields, nullStatement)]]
                        }
                    [[/inline]]
                );
            }

            return cs.ToString();
        }

        private string WriteVirtualModifier(McgResource resource)
        {
            if (resource.Extends == null)
            {
                if (resource.IsSealed)
                {
                    // Protected against CS0549: New virtual declared on a sealed type.
                    return String.Empty;
                }
                else
                {
                    return "virtual";
                }
            }
            else
            {
                return "override";
            }
        }

        public static string WriteToInstanceDescriptor(McgResource resource)
        {
            if (!resource.ToInstanceDescriptor) return String.Empty;

            if (resource.IsAbstract)
            {
                return
                    [[inline]]internal abstract InstanceDescriptor ToInstanceDescriptor();[[/inline]];
            }

            if (resource.IsCollection)
            {
                StringCodeSink notSupportedCheck = new StringCodeSink();
                string modifiers = "";

                if (resource.IsAnimatable)
                {
                    notSupportedCheck.Write(
                        [[inline]]
                            if (!CanFreeze)
                            {
                                throw new NotSupportedException(SR.Get(SRID.Converter_ConvertToNotSupported));
                            }
                        [[/inline]]
                    );
                }

                if (resource.Extends != null)
                {
                    modifiers = "override";
                }

                // Override if derived from parent class.  If abstract provide abstract implementation.

                return
                    [[inline]]
                        internal [[modifiers]] InstanceDescriptor ToInstanceDescriptor()
                        {
                            [[notSupportedCheck]]

                            ConstructorInfo ci = typeof([[resource.Name]]).GetConstructor(new Type[] { typeof(ICollection) } );

                            return new InstanceDescriptor(ci, new object[] { _collection } );
                        }
                    [[/inline]];
            }
            else
            {
                Console.WriteLine("WARNING: No default ToInstanceDescriptor implementation.\n");
                return String.Empty;
            }
        }

         private static string WriteParse(McgResource resource)
         {
             if (resource.SkipToString) return String.Empty;

             // Should we emit the parse code?
             if (resource.IsCollection ||
                 ((resource.ParseMethod != null) && (resource.ParseMethod.Length > 0)))
             {
                 string parseBody;

                 // A ParseMethod trumps the automatic handling of a collection
                 if ((resource.ParseMethod != null) && (resource.ParseMethod.Length > 0))
                 {
                     parseBody = "return " + resource.ParseMethod + "(source, formatProvider);\n";
                 }
                 else
                 {
                     parseBody =
                         [[inline]]
                             TokenizerHelper th = new TokenizerHelper(source, formatProvider);
                             [[resource.ManagedName]] resource = new [[resource.ManagedName]]();

                             [[resource.CollectionType.ManagedName]] value;

                             while (th.NextToken())
                             {
                                 [[WriteParseBody(resource.CollectionType, "th.GetCurrentToken()")]]

                                 resource.Add(value);
                             }

                             return resource;
                         [[/inline]];
                 }

                 return
                     [[inline]]
                         /// <summary>
                         /// Parse - returns an instance converted from the provided string
                         /// using the current culture
                         /// <param name="source"> string with [[resource.Name]] data </param>
                         /// </summary>
                         public static [[resource.Name]] Parse(string source)
                         {
                             IFormatProvider formatProvider = System.Windows.Markup.TypeConverterHelper.InvariantEnglishUS;

                             [[parseBody]]
                         }
                     [[/inline]];
             }
             else
             {
                 return String.Empty;
             }
        }
        #endregion Helper Methods


        //------------------------------------------------------
        //
        //  Private Fields
        //
        //------------------------------------------------------

        #region Private Fields
        private Regex _reWhitespace;
        private StringCodeSink _staticCtorText;
        #endregion Private Fields
    }
}





