// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: This file contains the definition of template-based generation of
//              FrameworkElement and FrameworkContentElement
//
//---------------------------------------------------------------------------

using System;
using System.IO;
using System.Xml;
using System.Collections.Generic;

using MS.Internal.MilCodeGen;
using MS.Internal.MilCodeGen.Runtime;
using MS.Internal.MilCodeGen.ResourceModel;
using MS.Internal.MilCodeGen.Helpers;

namespace MS.Internal.MilCodeGen.ResourceModel
{
    /// <summary>
    /// FrameworkElementTemplate: This class represents one instantiation of the FrameworkElement template.
    /// </summary>
    public class FrameworkElementTemplate: Template
    {
        private struct FrameworkElementTemplateInstance
        {
            public FrameworkElementTemplateInstance(
                string mangedDestinationDir,
                string className,
                string classNamepace,
                string thisString)
            {
                ManagedDestinationDir = mangedDestinationDir;
                ClassName = className;
                ClassNamespace = classNamepace;
                ThisString = thisString;
            }

            public string ManagedDestinationDir;
            public string ClassName;
            public string ClassNamespace;
            public string ThisString;
        }

        /// <summary>
        /// AddTemplateInstance - This is called by the code which parses the generation control.
        /// It is called on each TemplateInstance XMLNode encountered.
        /// </summary>
        public override void AddTemplateInstance(ResourceModel resourceModel, XmlNode node)
        {
            Instances.Add(new FrameworkElementTemplateInstance(
                ResourceModel.ToString(node, "ManagedDestinationDir"),
                ResourceModel.ToString(node, "ClassName"),
                ResourceModel.ToString(node, "ClassNamespace"),
                ResourceModel.ToString(node, "ThisString")));
        }

        public override void Go(ResourceModel resourceModel)
        {
            foreach (FrameworkElementTemplateInstance instance in Instances)
            {
                string fileName = instance.ClassName + ".cs";

                string fullPath = Path.Combine(resourceModel.OutputDirectory, instance.ManagedDestinationDir);

                using (FileCodeSink csFile = new FileCodeSink(fullPath, fileName, true /* Create dir if necessary */))
                {
                    csFile.WriteBlock(
                        [[inline]]
                            [[Helpers.ManagedStyle.WriteFileHeader(fileName,  @"wpf\src\Graphics\codegen\mcg\generators\FrameworkElementTemplate.cs")]]

                            using MS.Internal;
                            using MS.Utility;

                            using System;
                            using System.Collections;
                            using System.Diagnostics;
                            using System.Security;
                            using System.Security.Permissions;
                            using System.Windows.Controls;
                            using System.Windows.Diagnostics;
                            using System.Windows.Media;
                            using System.Windows.Markup;

                            using SR=System.Windows.SR;
                            using SRID=System.Windows.SRID;

                            namespace [[instance.ClassNamespace]]
                            {
                                [RuntimeNamePropertyAttribute("Name")]
                                public partial class [[instance.ClassName]]
                                {
                                    //------------------------------------------------------
                                    //
                                    //  Constructors
                                    //
                                    //------------------------------------------------------

                                    //------------------------------------------------------
                                    //
                                    //  Public Methods
                                    //
                                    //------------------------------------------------------

                                    #region Public Methods


                                    /// <summary>
                                    ///     Returns logical parent
                                    /// </summary>
                                    public [[conditional(instance.ClassName == "FrameworkContentElement")]]new [[/conditional]]DependencyObject Parent
                                    {
                                        get
                                        {
                                            // Verify Context Access
                                            // VerifyAccess();

                                            return ContextVerifiedGetParent();
                                        }
                                    }

                                    /// <summary>
                                    /// Registers the name - element combination from the
                                    /// NameScope that the current element belongs to.
                                    /// </summary>
                                    /// <param name="name">Name of the element</param>
                                    /// <param name="scopedElement">Element where name is defined</param>
                                    public void RegisterName(string name, object scopedElement)
                                    {
                                        INameScope nameScope = FrameworkElement.FindScope(this);
                                        if (nameScope != null)
                                        {
                                            nameScope.RegisterName(name, scopedElement);
                                        }
                                        else
                                        {
                                            throw new InvalidOperationException(SR.Get(SRID.NameScopeNotFound, name, "register"));
                                        }
                                    }

                                    /// <summary>
                                    /// Unregisters the name - element combination from the
                                    /// NameScope that the current element belongs to.
                                    /// </summary>
                                    /// <param name="name">Name of the element</param>
                                    public void UnregisterName(string name)
                                    {
                                        INameScope nameScope = FrameworkElement.FindScope(this);
                                        if (nameScope != null)
                                        {
                                            nameScope.UnregisterName(name);
                                        }
                                        else
                                        {
                                            throw new InvalidOperationException(SR.Get(SRID.NameScopeNotFound, name, "unregister"));
                                        }
                                    }

                                    /// <summary>
                                    /// Find the object with given name in the
                                    /// NameScope that the current element belongs to.
                                    /// </summary>
                                    /// <param name="name">string name to index</param>
                                    /// <returns>context if found, else null</returns>
                                    public object FindName(string name)
                                    {
                                        DependencyObject scopeOwner;
                                        return FindName(name, out scopeOwner);
                                    }

                                    // internal version of FindName that returns the scope owner
                                    internal object FindName(string name, out DependencyObject scopeOwner)
                                    {
                                        INameScope nameScope = FrameworkElement.FindScope(this, out scopeOwner);
                                        if (nameScope != null)
                                        {
                                            return nameScope.FindName(name);
                                        }

                                        return null;
                                    }

                                    /// <summary>
                                    /// Elements that arent connected to the tree do not receive theme change notifications.
                                    /// We leave it upto the app author to listen for such changes and invoke this method on
                                    /// elements that they know that arent connected to the tree. This method will update the
                                    /// DefaultStyle for the subtree starting at the current instance.
                                    /// </summary>
                                    public void UpdateDefaultStyle()
                                    {
                                        TreeWalkHelper.InvalidateOnResourcesChange([[instance.ThisString]], ResourcesChangeInfo.ThemeChangeInfo);
                                    }


                                    #endregion Public Methods

                                    //------------------------------------------------------
                                    //
                                    //  Protected Methods
                                    //
                                    //------------------------------------------------------

                                    #region Protected Methods

                                    /// <summary>
                                    ///     Returns enumerator to logical children
                                    /// </summary>
                                    protected internal virtual IEnumerator LogicalChildren
                                    {
                                        get { return null; }
                                    }

                                    #endregion Protected Methods

                                    //------------------------------------------------------
                                    //
                                    //  Internal Methods
                                    //
                                    //------------------------------------------------------

                                    #region Internal Methods

                                    /// <summary>
                                    ///     Tries to find a Resource for the given resourceKey in the current
                                    ///     element's ResourceDictionary.
                                    /// </summary>
                                    internal object FindResourceOnSelf(object resourceKey, bool allowDeferredResourceReference, bool mustReturnDeferredResourceReference)
                                    {
                                        ResourceDictionary resources = ResourcesField.GetValue(this);
                                        if ((resources != null) && resources.Contains(resourceKey))
                                        {
                                            bool canCache;
                                            return resources.FetchResource(resourceKey, allowDeferredResourceReference, mustReturnDeferredResourceReference, out canCache);
                                        }

                                        return DependencyProperty.UnsetValue;
                                    }

                                    internal DependencyObject ContextVerifiedGetParent()
                                    {
                                        return _parent;
                                    }

                                    // Consider marking whether Add should be called *before* or *after* the element adds it to its structure
                                    /// <summary>
                                    ///     Called by an element when that element adds the given object to
                                    ///     its logical tree.  FrameworkElement updates the affected
                                    ///     logical tree parent pointers to keep in sync with this insertion
                                    /// </summary>
                                    protected internal void AddLogicalChild(object child)
                                    {
                                        if (child != null)
                                        {
                                            // It is invalid to modify the children collection that we
                                            // might be iterating during a property invalidation tree walk.
                                            if (IsLogicalChildrenIterationInProgress)
                                            {
                                                throw new InvalidOperationException(SR.Get(SRID.CannotModifyLogicalChildrenDuringTreeWalk));
                                            }

                                            // Now that the child is going to be added, the FE/FCE construction is considered finished,
                                            // so we do not expect a change of InheritanceBehavior property,
                                            // so we can pick up properties from styles and resources.
                                            TryFireInitialized();

                                            bool exceptionThrown = true;
                                            try
                                            {
                                                HasLogicalChildren = true;

                                                // Child is present; reparent him to this element
                                                FrameworkObject fo = new FrameworkObject(child as DependencyObject);
                                                fo.ChangeLogicalParent(this);

                                                exceptionThrown = false;
                                            }
                                            finally
                                            {
                                                if (exceptionThrown)
                                                {
                                                    // 
                                                    // At the very least we should disconnect the child that we failed to add.

                                                    // Consider doing this...
                                                    //RemoveLogicalChild(child);
                                                }
                                            }
                                        }
                                    }


                                    // 
                                    /// <summary>
                                    ///     Called by an element when that element removes the given object from
                                    ///     its logical tree.  FrameworkElement updates the affected
                                    ///     logical tree parent pointers to keep in sync with this removal
                                    /// </summary>
                                    protected internal void RemoveLogicalChild(object child)
                                    {
                                        if (child != null)
                                        {
                                            // It is invalid to modify the children collection that we
                                            // might be iterating during a property invalidation tree walk.
                                            if (IsLogicalChildrenIterationInProgress)
                                            {
                                                throw new InvalidOperationException(SR.Get(SRID.CannotModifyLogicalChildrenDuringTreeWalk));
                                            }

                                            // Child is present
                                            FrameworkObject fo = new FrameworkObject(child as DependencyObject);
                                            if (fo.Parent == this)
                                            {
                                                fo.ChangeLogicalParent(null);
                                            }

                                            // This could have been the last child, so check if we have any more children
                                            IEnumerator children = LogicalChildren;

                                            // if null, there are no children.
                                            if (children == null)
                                            {
                                                HasLogicalChildren = false;
                                            }
                                            else
                                            {
                                                // If we can move next, there is at least one child
                                                HasLogicalChildren = children.MoveNext();
                                            }
                                        }
                                    }

                                    /// <summary>
                                    ///     Invoked when logical parent is changed.  This just
                                    ///     sets the parent pointer.
                                    /// </summary>
                                    /// <remarks>
                                    ///     A parent change is considered catastrohpic and results in a large
                                    ///     amount of invalidations and tree traversals. <cref see="DependencyFastBuild"/>
                                    ///     is recommended to reduce the work necessary to build a tree
                                    /// </remarks>
                                    /// <param name="newParent">
                                    ///     New parent that was set
                                    /// </param>
                                    internal void ChangeLogicalParent(DependencyObject newParent)
                                    {
                                        ///////////////////
                                        // OnNewParent:
                                        ///////////////////

                                        //
                                        // -- Approved By The Core Team --
                                        //
                                        // Do not allow foreign threads to change the tree.
                                        // (This is a noop if this object is not assigned to a Dispatcher.)
                                        //
                                        // We also need to ensure that the tree is homogenous with respect
                                        // to the dispatchers that the elements belong to.
                                        //
                                        this.VerifyAccess();
                                        if(newParent != null)
                                        {
                                            newParent.VerifyAccess();
                                        }

                                        // Logical Parent must first be dropped before you are attached to a newParent
                                        // This mitigates illegal tree state caused by logical child stealing
                                        if (_parent != null && newParent != null && _parent != newParent)
                                        {
                                            throw new System.InvalidOperationException(SR.Get(SRID.HasLogicalParent));
                                        }

                                        // Trivial check to avoid loops
                                        if (newParent == this)
                                        {
                                            throw new System.InvalidOperationException(SR.Get(SRID.CannotBeSelfParent));
                                        }

                                        // invalid during a VisualTreeChanged event
                                        VisualDiagnostics.VerifyVisualTreeChange(this);

                                        // Logical Parent implies no InheritanceContext
                                        if (newParent != null)
                                        {
                                            ClearInheritanceContext();
                                        }

                                        IsParentAnFE = newParent is FrameworkElement;

                                        DependencyObject oldParent = _parent;
                                        OnNewParent(newParent);

                                        // Update Has[Loaded/Unloaded]Handler Flags
                                        BroadcastEventHelper.AddOrRemoveHasLoadedChangeHandlerFlag(this, oldParent, newParent);

                                        [[conditional(instance.ClassName == "FrameworkContentElement")]]
                                        // Fire Loaded and Unloaded Events
                                        BroadcastEventHelper.BroadcastLoadedOrUnloadedEvent(this, oldParent, newParent);
                                        [[/conditional]]

                                        ///////////////////
                                        // OnParentChanged:
                                        ///////////////////

                                        // Invalidate relevant properties for this subtree
                                        DependencyObject parent = (newParent != null) ? newParent : oldParent;
                                        TreeWalkHelper.InvalidateOnTreeChange([[instance.ThisString]], parent, (newParent != null));

                                        // If no one has called BeginInit then mark the element initialized and fire Initialized event
                                        // (non-parser programmatic tree building scenario)
                                        TryFireInitialized();
                                    }

                                    /// <summary>
                                    ///     Called before the parent is chanded to the new value.
                                    /// </summary>
                                    internal virtual void OnNewParent(DependencyObject newParent)
                                    {
                                        //
                                        // This API is only here for compatability with the old
                                        // behavior.  Note that FrameworkElement does not have
                                        // this virtual, so why do we need it here?
                                        //

                                        DependencyObject oldParent = _parent;
                                        _parent = newParent;

                                        [[conditional(instance.ClassName == "FrameworkContentElement")]]
                                        // Synchronize ForceInherit properties
                                        if(_parent != null)
                                        {
                                            UIElement.SynchronizeForceInheritProperties(null, this, null, _parent);
                                        }
                                        else
                                        {
                                            UIElement.SynchronizeForceInheritProperties(null, this, null, oldParent);
                                        }
                                        [[/conditional]]
                                        [[conditional(instance.ClassName == "FrameworkElement")]]
                                        // Synchronize ForceInherit properties
                                        if(_parent != null && _parent is ContentElement)
                                        {
                                            UIElement.SynchronizeForceInheritProperties(this, null, null, _parent);
                                        }
                                        else if(oldParent is ContentElement)
                                        {
                                            UIElement.SynchronizeForceInheritProperties(this, null, null, oldParent);
                                        }
                                        [[/conditional]]

                                        // Synchronize ReverseInheritProperty Flags
                                        //
                                        // NOTE: do this AFTER synchronizing force-inherited flags, since
                                        // they often effect focusability and such.
                                        this.SynchronizeReverseInheritPropertyFlags(oldParent, false);
                                    }

                                    // OnAncestorChangedInternal variant when we know what type (FE/FCE) the
                                    //  tree node is.
                                    /// <SecurityNote>
                                    ///     Critical: This code calls into PresentationSource.OnAncestorChanged which is link demand protected
                                    ///     it does so only for content elements and not for FEs. But if called externally and configured
                                    ///     inappropriately it can be used to change the tree
                                    ///     TreatAsSafe: This does not let you get at the presentationsource which is what we do not want to expose
                                    /// </SecurityNote>
                                    [SecurityCritical,SecurityTreatAsSafe]
                                        internal void OnAncestorChangedInternal(TreeChangeInfo parentTreeState)
                                    {
                                        // Cache the IsSelfInheritanceParent flag
                                        bool wasSelfInheritanceParent = IsSelfInheritanceParent;

                                        if (parentTreeState.Root != this)
                                        {
                                            // Clear the HasStyleChanged flag
                                            HasStyleChanged = false;
                                            HasStyleInvalidated = false;
                                            [[conditional(instance.ClassName == "FrameworkElement")]]HasTemplateChanged = false;[[/conditional]]
                                        }

                                        // If this is a tree add operation update the ShouldLookupImplicitStyles
                                        // flag with respect to your parent.
                                        if (parentTreeState.IsAddOperation)
                                        {
                                            FrameworkObject fo =
                                                [[conditional(instance.ClassName == "FrameworkElement")]]new FrameworkObject(this, null);[[/conditional]]
                                                [[conditional(instance.ClassName == "FrameworkContentElement")]]new FrameworkObject(null, this);[[/conditional]]
                                            fo.SetShouldLookupImplicitStyles();
                                        }

                                        // Invalidate ResourceReference properties
                                        if (HasResourceReference)
                                        {
                                            // This operation may cause a style change and hence should be done before the call to
                                            // InvalidateTreeDependents as it relies on the HasStyleChanged flag
                                            TreeWalkHelper.OnResourcesChanged(this, ResourcesChangeInfo.TreeChangeInfo, false);
                                        }

                                        // If parent is a FrameworkElement
                                        // This is also an operation that could change the style
                                        FrugalObjectList<DependencyProperty> currentInheritableProperties =
                                        InvalidateTreeDependentProperties(parentTreeState, IsSelfInheritanceParent, wasSelfInheritanceParent);

                                        // we have inherited properties that changes as a result of the above;
                                        // invalidation; push that list of inherited properties on the stack
                                        // for the children to use
                                        parentTreeState.InheritablePropertiesStack.Push(currentInheritableProperties);

                                        [[conditional(instance.ClassName == "FrameworkContentElement")]]
                                        // Notify the PresentationSource that this element's ancestry may have changed.
                                        // We only need the ContentElement's because UIElements are taken care of
                                        // through the Visual class.
                                        PresentationSource.OnAncestorChanged(this);
                                        [[/conditional]]

                                        // Call OnAncestorChanged
                                        OnAncestorChanged();

                                        // Notify mentees if they exist
                                        if (PotentiallyHasMentees)
                                        {
                                            // Raise the ResourcesChanged Event so that ResourceReferenceExpressions
                                            // on non-[FE/FCE] listening for this can then update their values
                                            RaiseClrEvent(FrameworkElement.ResourcesChangedKey, EventArgs.Empty);
                                        }
                                    }

                                    // Invalidate all the properties that may have changed as a result of
                                    //  changing this element's parent in the logical (and sometimes visual tree.)
                                    internal FrugalObjectList<DependencyProperty> InvalidateTreeDependentProperties(TreeChangeInfo parentTreeState, bool isSelfInheritanceParent, bool wasSelfInheritanceParent)
                                    {
                                        AncestorChangeInProgress = true;
                                        [[conditional(instance.ClassName == "FrameworkElement")]]
                                        InVisibilityCollapsedTree = false;  // False == we don't know whether we're in a visibility collapsed tree.

                                        if (parentTreeState.TopmostCollapsedParentNode == null)
                                        {
                                            // There is no ancestor node with Visibility=Collapsed.
                                            //  See if "fe" is the root of a collapsed subtree.
                                            if (Visibility == Visibility.Collapsed)
                                            {
                                                // This is indeed the root of a collapsed subtree.
                                                //  remember this information as we proceed on the tree walk.
                                                parentTreeState.TopmostCollapsedParentNode = this;

                                                // Yes, this FE node is in a visibility collapsed subtree.
                                                InVisibilityCollapsedTree = true;
                                            }
                                        }
                                        else
                                        {
                                            // There is an ancestor node somewhere above us with
                                            //  Visibility=Collapsed.  We're in a visibility collapsed subtree.
                                            InVisibilityCollapsedTree = true;
                                        }
                                        [[/conditional]]

                                        try
                                        {
                                            // Style property is a special case of a non-inherited property that needs
                                            // invalidation for parent changes. Invalidate StyleProperty if it hasn't been
                                            // locally set because local value takes precedence over implicit references
                                            if ([[conditional(instance.ClassName == "FrameworkElement")]]IsInitialized && [[/conditional]]!HasLocalStyle && (this != parentTreeState.Root))
                                            {
                                                UpdateStyleProperty();
                                            }

                                            Style selfStyle = null;
                                            Style selfThemeStyle = null;
                                            DependencyObject templatedParent = null;

                                            int childIndex = -1;
                                            ChildRecord childRecord = new ChildRecord();
                                            bool isChildRecordValid = false;

                                            selfStyle = Style;
                                            selfThemeStyle = ThemeStyle;
                                            templatedParent = TemplatedParent;
                                            childIndex = TemplateChildIndex;

                                            // StyleProperty could have changed during invalidation of ResourceReferenceExpressions if it
                                            // were locally set or during the invalidation of unresolved implicitly referenced style
                                            bool hasStyleChanged = HasStyleChanged;

                                            // Fetch selfStyle, hasStyleChanged and childIndex for the current node
                                            FrameworkElement.GetTemplatedParentChildRecord(templatedParent, childIndex, out childRecord, out isChildRecordValid);

                                            FrameworkElement parentFE;
                                            FrameworkContentElement parentFCE;
                                            bool hasParent = FrameworkElement.GetFrameworkParent(this, out parentFE, out parentFCE);

                                            DependencyObject parent = null;
                                            InheritanceBehavior parentInheritanceBehavior = InheritanceBehavior.Default;
                                            if (hasParent)
                                            {
                                                if (parentFE != null)
                                                {
                                                    parent = parentFE;
                                                    parentInheritanceBehavior = parentFE.InheritanceBehavior;
                                                }
                                                else
                                                {
                                                    parent = parentFCE;
                                                    parentInheritanceBehavior = parentFCE.InheritanceBehavior;
                                                }
                                            }

                                            if (!TreeWalkHelper.SkipNext(InheritanceBehavior) && !TreeWalkHelper.SkipNow(parentInheritanceBehavior))
                                            {
                                                // Synchronize InheritanceParent
                                                this.SynchronizeInheritanceParent(parent);
                                            }
                                            else if (!IsSelfInheritanceParent)
                                            {
                                                // Set IsSelfInheritanceParet on the root node at a tree boundary
                                                // so that all inheritable properties are cached on it.
                                                SetIsSelfInheritanceParent();
                                            }

                                            // Loop through all cached inheritable properties for the parent to see if they should be invalidated.
                                            return TreeWalkHelper.InvalidateTreeDependentProperties(parentTreeState, [[instance.ThisString]], selfStyle, selfThemeStyle,
                                                ref childRecord, isChildRecordValid, hasStyleChanged, isSelfInheritanceParent, wasSelfInheritanceParent);
                                        }
                                        finally
                                        {
                                            AncestorChangeInProgress = false;
                                            [[conditional(instance.ClassName == "FrameworkElement")]]InVisibilityCollapsedTree = false;  // 'false' just means 'we don't know' - see comment at definition of the flag.[[/conditional]]
                                        }
                                    }

                                    /// <summary>
                                    ///    Check if the current element has a Loaded/Unloaded Change Handler.
                                    /// </summary>
                                    /// <remarks>
                                    ///    This is called this when a loaded handler element is removed.
                                    ///  We need to check if the parent should clear or maintain the
                                    ///  SubtreeHasLoadedChangeHandler bit.   For example the parent may also
                                    ///  have a handler.
                                    ///    This could be more efficent if it were a bit on the element,
                                    ///  set and cleared when the handler or templates state changes.  I expect
                                    ///  Event Handler state changes less often than element parentage.
                                    /// </remarks>
                                    internal bool ThisHasLoadedChangeEventHandler
                                    {
                                        get
                                        {
                                                if (null != EventHandlersStore)
                                                {
                                                    if (EventHandlersStore.Contains(LoadedEvent) || EventHandlersStore.Contains(UnloadedEvent))
                                                    {
                                                        return true;
                                                    }
                                                }
                                                if(null != Style && Style.HasLoadedChangeHandler)
                                                {
                                                    return true;
                                                }
                                                if(null != ThemeStyle && ThemeStyle.HasLoadedChangeHandler)
                                                {
                                                    return true;
                                                }
                                                [[conditional(instance.ClassName == "FrameworkElement")]]
                                                if(null != TemplateInternal && TemplateInternal.HasLoadedChangeHandler)
                                                {
                                                    return true;
                                                }
                                                [[/conditional]]
                                                if(HasFefLoadedChangeHandler)
                                                {
                                                    return true;
                                                }
                                                return false;
                                            }
                                    }

                                    internal bool HasFefLoadedChangeHandler
                                    {
                                        get
                                        {
                                            if(null == TemplatedParent)
                                            {
                                                return false;
                                            }
                                            FrameworkElementFactory fefRoot = BroadcastEventHelper.GetFEFTreeRoot(TemplatedParent);
                                            if(null == fefRoot)
                                            {
                                                return false;
                                            }
                                            FrameworkElementFactory fef = StyleHelper.FindFEF(fefRoot, TemplateChildIndex);
                                            if(null == fef)
                                            {
                                                return false;
                                            }
                                            return fef.HasLoadedChangeHandler;
                                        }
                                    }


                                    /// <summary>
                                    ///     This method causes the StyleProperty to be re-evaluated
                                    /// </summary>
                                    internal void UpdateStyleProperty()
                                    {
                                        if (!HasStyleInvalidated)
                                        {
                                            if (IsStyleUpdateInProgress == false)
                                            {
                                                IsStyleUpdateInProgress = true;
                                                try
                                                {
                                                    InvalidateProperty(StyleProperty);
                                                    HasStyleInvalidated = true;
                                                }
                                                finally
                                                {
                                                    IsStyleUpdateInProgress = false;
                                                }
                                            }
                                            else
                                            {
                                                throw new InvalidOperationException(SR.Get(SRID.CyclicStyleReferenceDetected, this));
                                            }
                                        }
                                    }

                                    /// <summary>
                                    ///     This method causes the ThemeStyleProperty to be re-evaluated
                                    /// </summary>
                                    internal void UpdateThemeStyleProperty()
                                    {
                                        if (IsThemeStyleUpdateInProgress == false)
                                        {
                                            IsThemeStyleUpdateInProgress = true;
                                            try
                                            {
                                                StyleHelper.GetThemeStyle([[instance.ThisString]]);

                                                // Update the ContextMenu and ToolTips separately because they aren't in the tree
                                                ContextMenu contextMenu =
                                                        GetValueEntry(
                                                                LookupEntry(ContextMenuProperty.GlobalIndex),
                                                                ContextMenuProperty,
                                                                null,
                                                                RequestFlags.DeferredReferences).Value as ContextMenu;
                                                if (contextMenu != null)
                                                {
                                                    TreeWalkHelper.InvalidateOnResourcesChange(contextMenu, null, ResourcesChangeInfo.ThemeChangeInfo);
                                                }

                                                DependencyObject toolTip =
                                                        GetValueEntry(
                                                                LookupEntry(ToolTipProperty.GlobalIndex),
                                                                ToolTipProperty,
                                                                null,
                                                                RequestFlags.DeferredReferences).Value as DependencyObject;

                                                if (toolTip != null)
                                                {
                                                    FrameworkObject toolTipFO = new FrameworkObject(toolTip);
                                                    if (toolTipFO.IsValid)
                                                    {
                                                        TreeWalkHelper.InvalidateOnResourcesChange(toolTipFO.FE, toolTipFO.FCE, ResourcesChangeInfo.ThemeChangeInfo);
                                                    }
                                                }

                                                OnThemeChanged();
                                            }
                                            finally
                                            {
                                                IsThemeStyleUpdateInProgress = false;
                                            }
                                        }
                                        else
                                        {
                                            throw new InvalidOperationException(SR.Get(SRID.CyclicThemeStyleReferenceDetected, this));
                                        }
                                    }

                                    // Called when the theme changes so resources not in the tree can be updated by subclasses
                                    internal virtual void OnThemeChanged()
                                    {
                                    }

                                    ///<summary>
                                    ///     Initiate the processing for Loaded event broadcast starting at this node
                                    /// </summary>
                                    /// <remarks>
                                    ///     This method is to allow firing Loaded event from a Helper class since the override is protected
                                    /// </remarks>
                                    internal void FireLoadedOnDescendentsInternal()
                                    {
                                        // This is to prevent duplicate Broadcasts for the Loaded event
                                        if (LoadedPending == null)
                                        {
                                            DependencyObject parent = Parent;
                                            [[conditional(instance.ClassName == "FrameworkElement")]]
                                            if (parent == null)
                                            {
                                                parent = VisualTreeHelper.GetParent(this);
                                            }
                                            [[/conditional]]

                                            // Check if this Loaded cancels against a previously queued Unloaded event
                                            // Note that if the Loaded and the Unloaded do not change the position of
                                            // the node within the loagical tree they are considered to cancel each other out.
                                            object[] unloadedPending = UnloadedPending;
                                            if (unloadedPending == null || unloadedPending[2] != parent)
                                            {
                                                // Add a callback to the MediaContext which will be called
                                                // before the first render after this point
                                                BroadcastEventHelper.AddLoadedCallback(this, parent);
                                            }
                                            else
                                            {
                                                // Dequeue Unloaded
                                                BroadcastEventHelper.RemoveUnloadedCallback(this, unloadedPending);
                                            }
                                        }
                                    }

                                    ///<summary>
                                    ///     Broadcast Unloaded event starting at this node
                                    /// </summary>
                                    internal void FireUnloadedOnDescendentsInternal()
                                    {
                                        // This is to prevent duplicate Broadcasts for the Unloaded event
                                        if (UnloadedPending == null)
                                        {
                                            DependencyObject parent = Parent;
                                            [[conditional(instance.ClassName == "FrameworkElement")]]
                                            if (parent == null)
                                            {
                                                parent = VisualTreeHelper.GetParent(this);
                                            }
                                            [[/conditional]]

                                            // Check if this Unloaded cancels against a previously queued Loaded event
                                            // Note that if the Loaded and the Unloaded do not change the position of
                                            // the node within the loagical tree they are considered to cancel each other out.
                                            object[] loadedPending = LoadedPending;
                                            if (loadedPending == null)
                                            {
                                                // Add a callback to the MediaContext which will be called
                                                // before the first render after this point
                                                BroadcastEventHelper.AddUnloadedCallback(this, parent);
                                            }
                                            else
                                            {
                                                // Dequeue Loaded
                                                BroadcastEventHelper.RemoveLoadedCallback(this, loadedPending);
                                            }
                                        }
                                    }

                                    /// <summary>
                                    ///     You are about to provided as the InheritanceContext for the target.
                                    ///     You can choose to allow this or not.
                                    /// </summary>
                                    internal override bool ShouldProvideInheritanceContext(DependencyObject target, DependencyProperty property)
                                    {
                                        // return true if the target is neither a FE or FCE
                                        FrameworkObject fo = new FrameworkObject(target);
                                        return !fo.IsValid;
                                    }

                                    // Define the DO's inheritance context
                                    internal override DependencyObject InheritanceContext
                                    {
                                        get { return InheritanceContextField.GetValue(this); }
                                    }

                                    /// <summary>
                                    ///     You have a new InheritanceContext
                                    /// </summary>
                                    /// <remarks>
                                    ///     This is to solve the case of programmatically creating a VisualBrush or BitmapCacheBrush
                                    ///     with an element in it and the element not getting Initialized.
                                    /// </remarks>
                                    internal override void AddInheritanceContext(DependencyObject context, DependencyProperty property)
                                    {
                                        base.AddInheritanceContext(context, property);

                                        // Initialize, if not already done
                                        TryFireInitialized();

                                        // accept the new inheritance context provided that
                                        // a) the requested link uses VisualBrush.Visual or BitmapCacheBrush.TargetProperty
                                        // b) this element has no visual or logical parent
                                        // c) the context does not introduce a cycle
                                        if ((property == VisualBrush.VisualProperty || property == BitmapCacheBrush.TargetProperty)
                                            && FrameworkElement.GetFrameworkParent(this) == null
                                             //!FrameworkObject.IsEffectiveAncestor(this, context, property))
                                            && !FrameworkObject.IsEffectiveAncestor(this, context))
                                        {
                                            //FrameworkObject.Log("+ {0}", FrameworkObject.LogIC(context, property, this));
                                            if (!HasMultipleInheritanceContexts && InheritanceContext == null)
                                            {
                                                // first request - accept the new inheritance context
                                                InheritanceContextField.SetValue(this, context);
                                                OnInheritanceContextChanged(EventArgs.Empty);
                                            }
                                            else if (InheritanceContext != null)
                                            {
                                                // second request - remove all context and enter "shared" mode
                                                InheritanceContextField.ClearValue(this);
                                                WriteInternalFlag2(InternalFlags2.HasMultipleInheritanceContexts, true);
                                                OnInheritanceContextChanged(EventArgs.Empty);
                                            }
                                            // else already in shared mode - ignore the request
                                        }
                                    }

                                    // Remove an inheritance context
                                    internal override void RemoveInheritanceContext(DependencyObject context, DependencyProperty property)
                                    {
                                        if (InheritanceContext == context)
                                        {
                                            //FrameworkObject.Log("- {0}", FrameworkObject.LogIC(context, property, this));
                                            InheritanceContextField.ClearValue(this);
                                            OnInheritanceContextChanged(EventArgs.Empty);
                                        }

                                        base.RemoveInheritanceContext(context, property);
                                    }

                                    // Clear the inheritance context (called when the element
                                    // gets a real parent
                                    private void ClearInheritanceContext()
                                    {
                                        if (InheritanceContext != null)
                                        {
                                            InheritanceContextField.ClearValue(this);
                                            OnInheritanceContextChanged(EventArgs.Empty);
                                        }
                                    }

                                    /// <summary>
                                    ///     This is a means for subclasses to get notification
                                    ///     of InheritanceContext changes and then they can do
                                    ///     their own thing.
                                    /// </summary>
                                    internal override void OnInheritanceContextChangedCore(EventArgs args)
                                    {
                                        DependencyObject oldMentor = MentorField.GetValue(this);
                                        DependencyObject newMentor = Helper.FindMentor(InheritanceContext);

                                        if (oldMentor != newMentor)
                                        {
                                            MentorField.SetValue(this, newMentor);

                                            if (oldMentor != null)
                                            {
                                                DisconnectMentor(oldMentor);
                                            }
                                            if (newMentor != null)
                                            {
                                                ConnectMentor(newMentor);
                                            }
                                        }
                                    }

                                    // connect to a new mentor
                                    void ConnectMentor(DependencyObject mentor)
                                    {
                                        FrameworkObject foMentor = new FrameworkObject(mentor);

                                        // register for InheritedPropertyChanged events
                                        foMentor.InheritedPropertyChanged += new InheritedPropertyChangedEventHandler(OnMentorInheritedPropertyChanged);

                                        // register for ResourcesChanged events
                                        foMentor.ResourcesChanged += new EventHandler(OnMentorResourcesChanged);

                                        // invalidate the mentee's tree
                                        TreeWalkHelper.InvalidateOnTreeChange(
                                                [[conditional(instance.ClassName == "FrameworkElement")]]this, null,[[/conditional]]
                                                [[conditional(instance.ClassName == "FrameworkContentElement")]]null, this,[[/conditional]]
                                                foMentor.DO,
                                                true /* isAddOperation */
                                                );

                                        // register for Loaded/Unloaded events.
                                        // Do this last so the tree is ready when Loaded is raised.
                                        if (this.SubtreeHasLoadedChangeHandler)
                                        {
                                            bool isLoaded = foMentor.IsLoaded;

                                            ConnectLoadedEvents(ref foMentor, isLoaded);

                                            if (isLoaded)
                                            {
                                                this.FireLoadedOnDescendentsInternal();
                                            }
                                        }
                                    }

                                    // disconnect from an old mentor
                                    void DisconnectMentor(DependencyObject mentor)
                                    {
                                        FrameworkObject foMentor = new FrameworkObject(mentor);

                                        // unregister for InheritedPropertyChanged events
                                        foMentor.InheritedPropertyChanged -= new InheritedPropertyChangedEventHandler(OnMentorInheritedPropertyChanged);

                                        // unregister for ResourcesChanged events
                                        foMentor.ResourcesChanged -= new EventHandler(OnMentorResourcesChanged);

                                        // invalidate the mentee's tree
                                        TreeWalkHelper.InvalidateOnTreeChange(
                                                [[conditional(instance.ClassName == "FrameworkElement")]]this, null,[[/conditional]]
                                                [[conditional(instance.ClassName == "FrameworkContentElement")]]null, this,[[/conditional]]
                                                foMentor.DO,
                                                false /* isAddOperation */
                                                );

                                        // unregister for Loaded/Unloaded events
                                        if (this.SubtreeHasLoadedChangeHandler)
                                        {
                                            bool isLoaded = foMentor.IsLoaded;

                                            DisconnectLoadedEvents(ref foMentor, isLoaded);

                                            if (foMentor.IsLoaded)
                                            {
                                                this.FireUnloadedOnDescendentsInternal();
                                            }
                                        }
                                    }

                                    // called by BroadcastEventHelper when the SubtreeHasLoadedChangedHandler
                                    // flag changes on a mentored FE/FCE
                                    internal void ChangeSubtreeHasLoadedChangedHandler(DependencyObject mentor)
                                    {
                                        FrameworkObject foMentor = new FrameworkObject(mentor);
                                        bool isLoaded = foMentor.IsLoaded;

                                        if (this.SubtreeHasLoadedChangeHandler)
                                        {
                                            ConnectLoadedEvents(ref foMentor, isLoaded);
                                        }
                                        else
                                        {
                                            DisconnectLoadedEvents(ref foMentor, isLoaded);
                                        }
                                    }

                                    // handle the Loaded event from the mentor
                                    void OnMentorLoaded(object sender, RoutedEventArgs e)
                                    {
                                        FrameworkObject foMentor = new FrameworkObject((DependencyObject)sender);

                                        // stop listening for Loaded, start listening for Unloaded
                                        foMentor.Loaded -= new RoutedEventHandler(OnMentorLoaded);
                                        foMentor.Unloaded += new RoutedEventHandler(OnMentorUnloaded);

                                        // broadcast the Loaded event to my framework subtree
                                        //FireLoadedOnDescendentsInternal();
                                        BroadcastEventHelper.BroadcastLoadedSynchronously(this, IsLoaded);
                                    }

                                    // handle the Unloaded event from the mentor
                                    void OnMentorUnloaded(object sender, RoutedEventArgs e)
                                    {
                                        FrameworkObject foMentor = new FrameworkObject((DependencyObject)sender);

                                        // stop listening for Unloaded, start listening for Loaded
                                        foMentor.Unloaded -= new RoutedEventHandler(OnMentorUnloaded);
                                        foMentor.Loaded += new RoutedEventHandler(OnMentorLoaded);

                                        // broadcast the Unloaded event to my framework subtree
                                        //FireUnloadedOnDescendentsInternal();
                                        BroadcastEventHelper.BroadcastUnloadedSynchronously(this, IsLoaded);
                                    }

                                    void ConnectLoadedEvents(ref FrameworkObject foMentor, bool isLoaded)
                                    {
                                        if (foMentor.IsValid)
                                        {
                                            if (isLoaded)
                                            {
                                                foMentor.Unloaded += new RoutedEventHandler(OnMentorUnloaded);
                                            }
                                            else
                                            {
                                                foMentor.Loaded += new RoutedEventHandler(OnMentorLoaded);
                                            }
                                        }
                                    }

                                    void DisconnectLoadedEvents(ref FrameworkObject foMentor, bool isLoaded)
                                    {
                                        if (foMentor.IsValid)
                                        {
                                            if (isLoaded)
                                            {
                                                foMentor.Unloaded -= new RoutedEventHandler(OnMentorUnloaded);
                                            }
                                            else
                                            {
                                                foMentor.Loaded -= new RoutedEventHandler(OnMentorLoaded);
                                            }
                                        }
                                    }

                                    // handle the InheritedPropertyChanged event from the mentor
                                    void OnMentorInheritedPropertyChanged(object sender, InheritedPropertyChangedEventArgs e)
                                    {
                                        TreeWalkHelper.InvalidateOnInheritablePropertyChange(
                                                [[conditional(instance.ClassName == "FrameworkElement")]]this, null,[[/conditional]]
                                                [[conditional(instance.ClassName == "FrameworkContentElement")]]null, this,[[/conditional]]
                                                e.Info, false /*skipStartNode*/);
                                    }

                                    // handle the ResourcesChanged event from the mentor
                                    void OnMentorResourcesChanged(object sender, EventArgs e)
                                    {
                                        TreeWalkHelper.InvalidateOnResourcesChange(
                                                [[conditional(instance.ClassName == "FrameworkElement")]]this, null,[[/conditional]]
                                                [[conditional(instance.ClassName == "FrameworkContentElement")]]null, this,[[/conditional]]
                                                ResourcesChangeInfo.CatastrophicDictionaryChangeInfo);
                                    }

                                    // Helper method to retrieve and fire the InheritedPropertyChanged event
                                    internal void RaiseInheritedPropertyChangedEvent(ref InheritablePropertyChangeInfo info)
                                    {
                                        EventHandlersStore store = EventHandlersStore;
                                        if (store != null)
                                        {
                                            Delegate handler = store.Get(FrameworkElement.InheritedPropertyChangedKey);
                                            if (handler != null)
                                            {
                                                InheritedPropertyChangedEventArgs args = new InheritedPropertyChangedEventArgs(ref info);
                                                ((InheritedPropertyChangedEventHandler)handler)(this, args);
                                            }
                                        }
                                    }

                                    #endregion Internal Methods

                                    //------------------------------------------------------
                                    //
                                    //  Internal Properties
                                    //
                                    //------------------------------------------------------

                                    #region Internal Properties

                                    // Indicates if the Style is being re-evaluated
                                    internal bool IsStyleUpdateInProgress
                                    {
                                        get { return ReadInternalFlag(InternalFlags.IsStyleUpdateInProgress); }
                                        set { WriteInternalFlag(InternalFlags.IsStyleUpdateInProgress, value); }
                                    }

                                    // Indicates if the ThemeStyle is being re-evaluated
                                    internal bool IsThemeStyleUpdateInProgress
                                    {
                                        get { return ReadInternalFlag(InternalFlags.IsThemeStyleUpdateInProgress); }
                                        set { WriteInternalFlag(InternalFlags.IsThemeStyleUpdateInProgress, value); }
                                    }

                                    // Indicates that we are storing "container template" provided values
                                    // on this element -- see StyleHelper.ParentTemplateValuesField
                                    internal bool StoresParentTemplateValues
                                    {
                                        get { return ReadInternalFlag(InternalFlags.StoresParentTemplateValues); }
                                        set { WriteInternalFlag(InternalFlags.StoresParentTemplateValues, value); }
                                    }

                                    // Indicates if this instance has had NumberSubstitutionChanged on it
                                    internal bool HasNumberSubstitutionChanged
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasNumberSubstitutionChanged); }
                                        set { WriteInternalFlag(InternalFlags.HasNumberSubstitutionChanged, value); }
                                    }

                                    // Indicates if this instance has a tree that
                                    // was generated via a Template
                                    internal bool HasTemplateGeneratedSubTree
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasTemplateGeneratedSubTree); }
                                        set { WriteInternalFlag(InternalFlags.HasTemplateGeneratedSubTree, value); }
                                    }

                                    // Indicates if this instance has an implicit style
                                    internal bool HasImplicitStyleFromResources
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasImplicitStyleFromResources); }
                                        set { WriteInternalFlag(InternalFlags.HasImplicitStyleFromResources, value); }
                                    }

                                    // Indicates if there are any implicit styles in the ancestry
                                    internal bool ShouldLookupImplicitStyles
                                    {
                                        get { return ReadInternalFlag(InternalFlags.ShouldLookupImplicitStyles); }
                                        set { WriteInternalFlag(InternalFlags.ShouldLookupImplicitStyles, value); }
                                    }

                                    // Indicates if this instance has a style set by a generator
                                    internal bool IsStyleSetFromGenerator
                                    {
                                        get { return ReadInternalFlag2(InternalFlags2.IsStyleSetFromGenerator); }
                                        set { WriteInternalFlag2(InternalFlags2.IsStyleSetFromGenerator, value); }
                                    }

                                    // Indicates if the StyleProperty has changed following a UpdateStyleProperty
                                    // call in OnAncestorChangedInternal
                                    internal bool HasStyleChanged
                                    {
                                        get { return ReadInternalFlag2(InternalFlags2.HasStyleChanged); }
                                        set { WriteInternalFlag2(InternalFlags2.HasStyleChanged, value); }
                                    }

                                    [[conditional(instance.ClassName == "FrameworkElement")]]
                                    // Indicates if the TemplateProperty has changed during a tree walk
                                    internal bool HasTemplateChanged
                                    {
                                        get { return ReadInternalFlag2(InternalFlags2.HasTemplateChanged); }
                                        set { WriteInternalFlag2(InternalFlags2.HasTemplateChanged, value); }
                                    }
                                    [[/conditional]]

                                    // Indicates if the StyleProperty has been invalidated during a tree walk
                                    internal bool HasStyleInvalidated
                                    {
                                        get { return ReadInternalFlag2(InternalFlags2.HasStyleInvalidated); }
                                        set { WriteInternalFlag2(InternalFlags2.HasStyleInvalidated, value); }
                                    }

                                    // Indicates that the StyleProperty full fetch has been
                                    // performed atleast once on this node
                                    internal bool HasStyleEverBeenFetched
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasStyleEverBeenFetched); }
                                        set { WriteInternalFlag(InternalFlags.HasStyleEverBeenFetched, value); }
                                    }

                                    // Indicates that the StyleProperty has been set locally on this element
                                    internal bool HasLocalStyle
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasLocalStyle); }
                                        set { WriteInternalFlag(InternalFlags.HasLocalStyle, value); }
                                    }

                                    // Indicates that the ThemeStyleProperty full fetch has been
                                    // performed atleast once on this node
                                    internal bool HasThemeStyleEverBeenFetched
                                    {
                                        get { return ReadInternalFlag(InternalFlags.HasThemeStyleEverBeenFetched); }
                                        set { WriteInternalFlag(InternalFlags.HasThemeStyleEverBeenFetched, value); }
                                    }

                                    // Indicates that an ancestor change tree walk is progressing
                                    // through the given node
                                    internal bool AncestorChangeInProgress
                                    {
                                        get { return ReadInternalFlag(InternalFlags.AncestorChangeInProgress); }
                                        set { WriteInternalFlag(InternalFlags.AncestorChangeInProgress, value); }
                                    }

                                    // Stores the inheritable properties that will need to invalidated on the children of this
                                    // node.This is a transient cache that is active only during an AncestorChange operation.
                                    internal FrugalObjectList<DependencyProperty> InheritableProperties
                                    {
                                        get { return _inheritableProperties; }
                                        set { _inheritableProperties = value; }
                                    }

                                    // Says if there is a loaded event pending
                                    internal object[] LoadedPending
                                    {
                                        get { return (object[]) GetValue(LoadedPendingProperty); }
                                    }

                                    // Says if there is an unloaded event pending
                                    internal object[] UnloadedPending
                                    {
                                        get { return (object[]) GetValue(UnloadedPendingProperty); }
                                    }

                                    // Indicates if this instance has multiple inheritance contexts
                                    internal override bool HasMultipleInheritanceContexts
                                    {
                                        get { return ReadInternalFlag2(InternalFlags2.HasMultipleInheritanceContexts); }
                                    }

                                    // Indicates if the current element has or had mentees at some point.
                                    internal bool PotentiallyHasMentees
                                    {
                                        get { return ReadInternalFlag(InternalFlags.PotentiallyHasMentees); }
                                        set
                                        {
                                            Debug.Assert(value == true,
                                                "This flag is set to true when a mentee attaches a listeners to either the " +
                                                "InheritedPropertyChanged event or the ResourcesChanged event. It never goes " +
                                                "back to being false because this would involve counting the remaining listeners " +
                                                "for either of the aforementioned events. This seems like an overkill for the perf " +
                                                "optimization we are trying to achieve here.");

                                            WriteInternalFlag(InternalFlags.PotentiallyHasMentees, value);
                                        }
                                    }

                                    [[conditional(instance.ClassName == "FrameworkElement")]]
                                    /// <summary>
                                    ///     ResourcesChanged private key
                                    /// </summary>
                                    internal static readonly EventPrivateKey ResourcesChangedKey = new EventPrivateKey();
                                    [[/conditional]]

                                    /// <summary>
                                    ///     ResourceReferenceExpressions on non-[FE/FCE] add listeners to this
                                    ///     event so they can get notified when there is a ResourcesChange
                                    /// </summary>
                                    /// <remarks>
                                    ///     make this pay-for-play by storing handlers
                                    ///     in EventHandlersStore
                                    /// </remarks>
                                    internal event EventHandler ResourcesChanged
                                    {
                                        add
                                        {
                                            PotentiallyHasMentees = true;
                                            EventHandlersStoreAdd(FrameworkElement.ResourcesChangedKey, value);
                                        }
                                        remove { EventHandlersStoreRemove(FrameworkElement.ResourcesChangedKey, value); }
                                    }

                                    [[conditional(instance.ClassName == "FrameworkElement")]]
                                    /// <summary>
                                    ///     InheritedPropertyChanged private key
                                    /// </summary>
                                    internal static readonly EventPrivateKey InheritedPropertyChangedKey = new EventPrivateKey();
                                    [[/conditional]]

                                    /// <summary>
                                    ///     Mentees add listeners to this
                                    ///     event so they can get notified when there is a InheritedPropertyChange
                                    /// </summary>
                                    /// <remarks>
                                    ///     make this pay-for-play by storing handlers
                                    ///     in EventHandlersStore
                                    /// </remarks>
                                    internal event InheritedPropertyChangedEventHandler InheritedPropertyChanged
                                    {
                                        add
                                        {
                                            PotentiallyHasMentees = true;
                                            EventHandlersStoreAdd(FrameworkElement.InheritedPropertyChangedKey, value);
                                        }
                                        remove { EventHandlersStoreRemove(FrameworkElement.InheritedPropertyChangedKey, value); }
                                    }


                                    #endregion Internal Properties

                                    //------------------------------------------------------
                                    //
                                    //  Internal Fields
                                    //
                                    //------------------------------------------------------

                                    #region Internal Fields

                                    // Optimization, to avoid calling FromSystemType too often
                                    internal new static DependencyObjectType DType = DependencyObjectType.FromSystemTypeInternal(typeof([[instance.ClassName]]));

                                    #endregion Internal Fields

                                    //------------------------------------------------------
                                    //
                                    //  Private Fields
                                    //
                                    //------------------------------------------------------

                                    #region Private Fields

                                    // The parent element in logical tree.
                                    private new DependencyObject _parent;
                                    private FrugalObjectList<DependencyProperty> _inheritableProperties;

                                    private static readonly UncommonField<DependencyObject> InheritanceContextField = new UncommonField<DependencyObject>();
                                    private static readonly UncommonField<DependencyObject> MentorField = new UncommonField<DependencyObject>();

                                    #endregion Private Fields
                                }
                            }
                        [[/inline]]
                        );
                }
            }
        }

        private List<FrameworkElementTemplateInstance> Instances = new List<FrameworkElementTemplateInstance>();
    }
}




