// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// Description: Generator for the common bits of UIElement and ContentElement
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

    public class Elements : Main.GeneratorBase
    {
        //------------------------------------------------------
        //
        //  Constructors
        //
        //------------------------------------------------------

        public Elements(CG data) : base (data) {}

        //------------------------------------------------------
        //
        //  Public Methods
        //
        //------------------------------------------------------

        public override void Go()
        {
            foreach(CGElement e in CG.Elements)
            {
                string path = Path.Combine(CG.OutputDirectory, e.ManagedDestinationDir);
                string filename = e.Name + ".cs";

                using (FileCodeSink cs = new FileCodeSink(path, filename, true /* Create dir if necessary */))
                {
                    cs.WriteBlock(Helpers.ManagedStyle.WriteFileHeader(filename));
                    cs.WriteBlock(
                        [[inline]]
                            using MS.Internal;
                            using MS.Internal.KnownBoxes;
                            using MS.Internal.PresentationCore;
                            using MS.Utility;
                            using System;
                            using System.Collections.Generic;
                            using System.ComponentModel;
                            using System.Diagnostics;
                            using System.Security;
                            using System.Security.Permissions;
                            using System.Windows.Input;
                            using System.Windows.Media.Animation;

                            #pragma warning disable 1634, 1691  // suppressing PreSharp warnings

                            namespace [[e.Namespace]]
                            {
                                partial class [[e.Name]] [[(e.ImplementsIAnimatable ? ": IAnimatable" : "")]]
                                {
                                    static private readonly Type _typeofThis = typeof([[e.Name]]);

                                    [[(e.ImplementsIAnimatable ? IAnimatableHelper.WriteImplementation() : "")]]

                                    [[WriteCommands(e)]]

                                    [[WriteEventInfrastructure(e)]]

                                    [[WriteRegisterEvents(e)]]

                                    [[WriteRegisterProperties(e)]]

                                    [[WriteThunks(e)]]

                                    [[WriteEvents(e)]]

                                    [[WriteProperties(e)]]

                                    [[WriteCoreFlags(e)]]
                                }
                            }
                        [[/inline]]);
                }
            }
        }

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

        private string WriteCommands(CGElement element)
        {
            StringCodeSink cs = new StringCodeSink();

            cs.WriteBlock("#region Commands");

            foreach(string bindType in new string[] {"Input", "Command"})
            {
                cs.WriteBlock(
                    [[inline]]
                        /// <summary>
                        /// Instance level [[bindType]]Binding collection, initialized on first use.
                        /// To have commands handled (QueryEnabled/Execute) on an element instance,
                        /// the user of this method can add [[bindType]]Binding with handlers thru this
                        /// method.
                        /// </summary>
                        [DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
                        public [[bindType]]BindingCollection [[bindType]]Bindings
                        {
                            get
                            {
                                VerifyAccess();
                                [[bindType]]BindingCollection bindings = [[bindType]]BindingCollectionField.GetValue(this);
                                if (bindings == null)
                                {
                                    bindings = new [[bindType]]BindingCollection([[conditional(bindType == "Input")]]this[[/conditional]]);
                                    [[bindType]]BindingCollectionField.SetValue(this, bindings);
                                }

                                return bindings;
                            }
                        }

                        // Used by CommandManager to avoid instantiating an empty collection
                        internal [[bindType]]BindingCollection [[bindType]]BindingsInternal
                        {
                            get
                            {
                                VerifyAccess();
                                return [[bindType]]BindingCollectionField.GetValue(this);
                            }
                        }

                        /// <summary>
                        /// This method is used by TypeDescriptor to determine if this property should
                        /// be serialized.
                        /// </summary>
                        // for serializer to serialize only when [[bindType]]Bindings is not empty
                        [EditorBrowsable(EditorBrowsableState.Never)]
                        public bool ShouldSerialize[[bindType]]Bindings()
                        {
                            [[bindType]]BindingCollection bindingCollection = [[bindType]]BindingCollectionField.GetValue(this);
                            if (bindingCollection != null && bindingCollection.Count > 0)
                            {
                                return true;
                            }

                            return false;
                        }
                    [[/inline]]);
            }

            cs.Write("#endregion Commands");

            return cs.ToString();
        }

        public string WriteEventInfrastructure(CGElement element)
        {
            return
                [[inline]]
                    #region Events

                    /// <summary>
                    ///     Allows [[element.Name]] to augment the
                    ///     <see cref="EventRoute"/>
                    /// </summary>
                    /// <remarks>
                    ///     Sub-classes of [[element.Name]] can override
                    ///     this method to custom augment the route
                    /// </remarks>
                    /// <param name="route">
                    ///     The <see cref="EventRoute"/> to be
                    ///     augmented
                    /// </param>
                    /// <param name="args">
                    ///     <see cref="RoutedEventArgs"/> for the
                    ///     RoutedEvent to be raised post building
                    ///     the route
                    /// </param>
                    /// <returns>
                    ///     Whether or not the route should continue past the visual tree.
                    ///     If this is true, and there are no more visual parents, the route
                    ///     building code will call the GetUIParentCore method to find the
                    ///     next non-visual parent.
                    /// </returns>
                    internal virtual bool BuildRouteCore(EventRoute route, RoutedEventArgs args)
                    {
                        return false;
                    }

                    /// <summary>
                    ///     Builds the <see cref="EventRoute"/>
                    /// </summary>
                    /// <param name="route">
                    ///     The <see cref="EventRoute"/> being
                    ///     built
                    /// </param>
                    /// <param name="args">
                    ///     <see cref="RoutedEventArgs"/> for the
                    ///     RoutedEvent to be raised post building
                    ///     the route
                    /// </param>
                    internal void BuildRoute(EventRoute route, RoutedEventArgs args)
                    {
                        UIElement.BuildRouteHelper(this, route, args);
                    }

                    /// <summary>
                    ///     Raise the events specified by
                    ///     <see cref="RoutedEventArgs.RoutedEvent"/>
                    /// </summary>
                    /// <remarks>
                    ///     This method is a shorthand for
                    ///     <see cref="[[element.Name]].BuildRoute"/> and
                    ///     <see cref="EventRoute.InvokeHandlers"/>
                    /// </remarks>
                    /// <param name="e">
                    ///     <see cref="RoutedEventArgs"/> for the event to
                    ///     be raised
                    /// </param>
                    ///<SecurityNote>
                    ///     By default clears the user initiated bit.
                    ///     To guard against "replay" attacks.
                    ///</SecurityNote>
                    public void RaiseEvent(RoutedEventArgs e)
                    {
                        // VerifyAccess();

                        if (e == null)
                        {
                            throw new ArgumentNullException("e");
                        }
                        e.ClearUserInitiated();

                        UIElement.RaiseEventImpl(this, e);
                    }

                    /// <summary>
                    ///     "Trusted" internal flavor of RaiseEvent.
                    ///     Used to set the User-initated RaiseEvent.
                    /// </summary>
                    ///<SecurityNote>
                    ///     Critical - sets the MarkAsUserInitiated bit.
                    ///</SecurityNote>
                    [SecurityCritical]
                    internal void RaiseEvent(RoutedEventArgs args, bool trusted)
                    {
                        if (args == null)
                        {
                            throw new ArgumentNullException("args");
                        }

                        if (trusted)
                        {
                            RaiseTrustedEvent(args);
                        }
                        else
                        {
                            args.ClearUserInitiated();

                            UIElement.RaiseEventImpl(this, args);
                        }
                    }

                    ///<SecurityNote>
                    ///     Critical - sets the MarkAsUserInitiated bit.
                    ///</SecurityNote>
                    [SecurityCritical]
                    [MS.Internal.Permissions.UserInitiatedRoutedEventPermissionAttribute(SecurityAction.Assert)]
                    internal void RaiseTrustedEvent(RoutedEventArgs args)
                    {
                        if (args == null)
                        {
                            throw new ArgumentNullException("args");
                        }

                        // Try/finally to ensure that UserInitiated bit is cleared.
                        args.MarkAsUserInitiated();

                        try
                        {
                            UIElement.RaiseEventImpl(this, args);
                        }
                        finally
                        {
                            // Clear the bit - just to guarantee it's not used again
                            args.ClearUserInitiated();
                        }
                    }
                        

                    /// <summary>
                    ///     Allows adjustment to the event source
                    /// </summary>
                    /// <remarks>
                    ///     Subclasses must override this method
                    ///     to be able to adjust the source during
                    ///     route invocation <para/>
                    ///
                    ///     NOTE: Expected to return null when no
                    ///     change is made to source
                    /// </remarks>
                    /// <param name="args">
                    ///     Routed Event Args
                    /// </param>
                    /// <returns>
                    ///     Returns new source
                    /// </returns>
                    internal virtual object AdjustEventSource(RoutedEventArgs args)
                    {
                        return null;
                    }

                    /// <summary>
                    ///     See overloaded method for details
                    /// </summary>
                    /// <remarks>
                    ///     handledEventsToo defaults to false <para/>
                    ///     See overloaded method for details
                    /// </remarks>
                    /// <param name="routedEvent"/>
                    /// <param name="handler"/>
                    public void AddHandler(RoutedEvent routedEvent, Delegate handler)
                    {
                        // HandledEventToo defaults to false
                        // Call forwarded
                        AddHandler(routedEvent, handler, false);
                    }

                    /// <summary>
                    ///     Adds a routed event handler for the particular
                    ///     <see cref="RoutedEvent"/>
                    /// </summary>
                    /// <remarks>
                    ///     The handler added thus is also known as
                    ///     an instance handler <para/>
                    ///     <para/>
                    ///
                    ///     NOTE: It is not an error to add a handler twice
                    ///     (handler will simply be called twice) <para/>
                    ///     <para/>
                    ///
                    ///     Input parameters <see cref="RoutedEvent"/>
                    ///     and handler cannot be null <para/>
                    ///     handledEventsToo input parameter when false means
                    ///     that listener does not care about already handled events.
                    ///     Hence the handler will not be invoked on the target if
                    ///     the RoutedEvent has already been
                    ///     <see cref="RoutedEventArgs.Handled"/> <para/>
                    ///     handledEventsToo input parameter when true means
                    ///     that the listener wants to hear about all events even if
                    ///     they have already been handled. Hence the handler will
                    ///     be invoked irrespective of the event being
                    ///     <see cref="RoutedEventArgs.Handled"/>
                    /// </remarks>
                    /// <param name="routedEvent">
                    ///     <see cref="RoutedEvent"/> for which the handler
                    ///     is attached
                    /// </param>
                    /// <param name="handler">
                    ///     The handler that will be invoked on this object
                    ///     when the RoutedEvent is raised
                    /// </param>
                    /// <param name="handledEventsToo">
                    ///     Flag indicating whether or not the listener wants to
                    ///     hear about events that have already been handled
                    /// </param>
                    public void AddHandler(
                        RoutedEvent routedEvent,
                        Delegate handler,
                        bool handledEventsToo)
                    {
                        // VerifyAccess();

                        if (routedEvent == null)
                        {
                            throw new ArgumentNullException("routedEvent");
                        }

                        if (handler == null)
                        {
                            throw new ArgumentNullException("handler");
                        }

                        if (!routedEvent.IsLegalHandler(handler))
                        {
                            throw new ArgumentException(SR.Get(SRID.HandlerTypeIllegal));
                        }

                        EnsureEventHandlersStore();
                        EventHandlersStore.AddRoutedEventHandler(routedEvent, handler, handledEventsToo);

                        OnAddHandler (routedEvent, handler);
                    }

                    /// <summary>
                    ///     Notifies subclass of a new routed event handler.  Note that this is
                    ///     called once for each handler added, but OnRemoveHandler is only called
                    ///     on the last removal.
                    /// </summary>
                    internal virtual void OnAddHandler(
                        RoutedEvent routedEvent,
                        Delegate handler)
                    {
                    }

                    /// <summary>
                    ///     Removes all instances of the specified routed
                    ///     event handler for this object instance
                    /// </summary>
                    /// <remarks>
                    ///     The handler removed thus is also known as
                    ///     an instance handler <para/>
                    ///     <para/>
                    ///
                    ///     NOTE: This method does nothing if there were
                    ///     no handlers registered with the matching
                    ///     criteria <para/>
                    ///     <para/>
                    ///
                    ///     Input parameters <see cref="RoutedEvent"/>
                    ///     and handler cannot be null <para/>
                    ///     This method ignores the handledEventsToo criterion
                    /// </remarks>
                    /// <param name="routedEvent">
                    ///     <see cref="RoutedEvent"/> for which the handler
                    ///     is attached
                    /// </param>
                    /// <param name="handler">
                    ///     The handler for this object instance to be removed
                    /// </param>
                    public void RemoveHandler(RoutedEvent routedEvent, Delegate handler)
                    {
                        // VerifyAccess();

                        if (routedEvent == null)
                        {
                            throw new ArgumentNullException("routedEvent");
                        }

                        if (handler == null)
                        {
                            throw new ArgumentNullException("handler");
                        }

                        if (!routedEvent.IsLegalHandler(handler))
                        {
                            throw new ArgumentException(SR.Get(SRID.HandlerTypeIllegal));
                        }

                        EventHandlersStore store = EventHandlersStore;
                        if (store != null)
                        {
                            store.RemoveRoutedEventHandler(routedEvent, handler);

                            OnRemoveHandler (routedEvent, handler);

                            if (store.Count == 0)
                            {
                                // last event handler was removed -- throw away underlying EventHandlersStore
                                EventHandlersStoreField.ClearValue(this);
                                WriteFlag(CoreFlags.ExistsEventHandlersStore, false);
                            }

                        }
                    }

                    /// <summary>
                    ///     Notifies subclass of an event for which a handler has been removed.
                    /// </summary>
                    internal virtual void OnRemoveHandler(
                        RoutedEvent routedEvent,
                        Delegate handler)
                    {
                    }

                    private void EventHandlersStoreAdd(EventPrivateKey key, Delegate handler)
                    {
                        EnsureEventHandlersStore();
                        EventHandlersStore.Add(key, handler);
                    }

                    private void EventHandlersStoreRemove(EventPrivateKey key, Delegate handler)
                    {
                        EventHandlersStore store = EventHandlersStore;
                        if (store != null)
                        {
                            store.Remove(key, handler);
                            if (store.Count == 0)
                            {
                                // last event handler was removed -- throw away underlying EventHandlersStore
                                EventHandlersStoreField.ClearValue(this);
                                WriteFlag(CoreFlags.ExistsEventHandlersStore, false);
                            }
                        }
                    }

                    /// <summary>
                    ///     Add the event handlers for this element to the route.
                    /// </summary>
                    public void AddToEventRoute(EventRoute route, RoutedEventArgs e)
                    {
                        if (route == null)
                        {
                            throw new ArgumentNullException("route");
                        }
                        if (e == null)
                        {
                            throw new ArgumentNullException("e");
                        }

                        // Get class listeners for this [[element.Name]]
                        RoutedEventHandlerInfoList classListeners =
                            GlobalEventManager.GetDTypedClassListeners(this.DependencyObjectType, e.RoutedEvent);

                        // Add all class listeners for this [[element.Name]]
                        while (classListeners != null)
                        {
                            for(int i = 0; i < classListeners.Handlers.Length; i++)
                            {
                                route.Add(this, classListeners.Handlers[i].Handler, classListeners.Handlers[i].InvokeHandledEventsToo);
                            }

                            classListeners = classListeners.Next;
                        }

                        // Get instance listeners for this [[element.Name]]
                        FrugalObjectList<RoutedEventHandlerInfo> instanceListeners = null;
                        EventHandlersStore store = EventHandlersStore;
                        if (store != null)
                        {
                            instanceListeners = store[e.RoutedEvent];

                            // Add all instance listeners for this [[element.Name]]
                            if (instanceListeners != null)
                            {
                                for (int i = 0; i < instanceListeners.Count; i++)
                                {
                                    route.Add(this, instanceListeners[i].Handler, instanceListeners[i].InvokeHandledEventsToo);
                                }
                            }
                        }

                        // Allow Framework to add event handlers in styles
                        AddToEventRouteCore(route, e);
                    }

                    /// <summary>
                    ///     This virtual method is to be overridden in Framework
                    ///     to be able to add handlers for styles
                    /// </summary>
                    internal virtual void AddToEventRouteCore(EventRoute route, RoutedEventArgs args)
                    {
                    }

                    /// <summary>
                    ///     Event Handlers Store
                    /// </summary>
                    /// <remarks>
                    ///     The idea of exposing this property is to allow
                    ///     elements in the Framework to generically use
                    ///     EventHandlersStore for Clr events as well.
                    /// </remarks>
                    internal EventHandlersStore EventHandlersStore
                    {
                        [FriendAccessAllowed] // Built into Core, also used by Framework.
                        get
                        {
                            if(!ReadFlag(CoreFlags.ExistsEventHandlersStore))
                            {
                                return null;
                            }
                            return EventHandlersStoreField.GetValue(this);
                        }
                    }

                    /// <summary>
                    ///     Ensures that EventHandlersStore will return
                    ///     non-null when it is called.
                    /// </summary>
                    [FriendAccessAllowed] // Built into Core, also used by Framework.
                    internal void EnsureEventHandlersStore()
                    {
                        if (EventHandlersStore == null)
                        {
                            EventHandlersStoreField.SetValue(this, new EventHandlersStore());
                            WriteFlag(CoreFlags.ExistsEventHandlersStore, true);
                        }
                    }

                    #endregion Events
                    
                    internal virtual bool InvalidateAutomationAncestorsCore(Stack<DependencyObject> branchNodeStack, out bool continuePastVisualTree)
                    {
                        continuePastVisualTree = false;
                        return true;
                    }
                [[/inline]];
        }

        private string WriteRegisterEvents(CGElement element)
        {
            if (!element.Name.Equals("UIElement", StringComparison.InvariantCulture))
            {
                return String.Empty;
            }

            StringCodeSink cs = new StringCodeSink();

            foreach(CGEvent evt in CG.Events)
            {
                string handledToo = evt.HandledToo ? "true" : "false";

                cs.Write(
                    [[inline]]
                        EventManager.RegisterClassHandler(type, [[evt.RoutedEventName]], new [[evt.HandlerType]]([[element.Name]].[[evt.ThunkName]]), [[handledToo]]);
                    [[/inline]]);
            }

            return
                [[inline]]
                    /// <summary>
                    /// Used by UIElement, ContentElement, and UIElement3D to register common Events.
                    /// </summary>
                    /// <SecurityNote>
                    ///  Critical: This code is used to register various thunks that are used to send input to the tree
                    ///  TreatAsSafe: This code attaches handlers that are inside the class and private. Not configurable or overridable
                    /// </SecurityNote>
                    [SecurityCritical,SecurityTreatAsSafe]
                    internal static void RegisterEvents(Type type)
                    {
                        [[cs]]
                    }
                [[/inline]];
        }

        private string WriteRegisterProperties(CGElement element)
        {
            if (!element.Name.Equals("ContentElement", StringComparison.InvariantCulture))
            {
                return String.Empty;
            }

            StringCodeSink cs = new StringCodeSink();

            // Create metadata for any aliased properties.
            foreach(CGProperty property in CG.Properties)
            {
                if (property.Owner != element.Name)
                {
                    cs.WriteBlock(
                        [[inline]]
                            [[property.Name]]PropertyKey.OverrideMetadata(
                                                _typeofThis,
                                                [[WritePropertyMetadata(property)]]);
                        [[/inline]]);
                }
            }

            return
                [[inline]]
                    private static void RegisterProperties()
                    {
                        [[cs]]
                    }
                [[/inline]];
        }

        private string WriteThunks(CGElement element)
        {
            if (!element.Name.Equals("UIElement", StringComparison.InvariantCulture))
            {
                return String.Empty;
            }

            StringCodeSink cs = new StringCodeSink();

            foreach(CGEvent evt in CG.Events)
            {
                StringCodeSink body = new StringCodeSink();

                // If the event is not registered to be called back when already
                // handled, add an Invariant.Assert to verify e.Handled is false.
                if (!evt.HandledToo)
                {
                    body.WriteBlock(
                        [[inline]]
                            Invariant.Assert(!e.Handled, "Unexpected: Event has already been handled.");
                        [[/inline]]);
                }

                if (evt.TranslateInput)
                {
                    string translateInput =
                        [[inline]]
                            CommandManager.TranslateInput((IInputElement)sender, e);
                        [[/inline]];

                    // If the event is registered to be called back when already handled
                    // add a clause to skip TranslateInput if e.Handled == true.
                    if (evt.HandledToo)
                    {
                        translateInput = SkipIfHandled(translateInput);
                    }

                    body.WriteBlock(translateInput);
                }

                string invokation;

                if (!evt.Commanding)
                {
                    invokation =
                        [[inline]]
                            UIElement uie = sender as UIElement;

                            if (uie != null)
                            {
                                uie.[[evt.VirtualName]](e);
                            }
                            else
                            {
                                ContentElement ce = sender as ContentElement;

                                if (ce != null)
                                {
                                    ce.[[evt.VirtualName]](e);
                                }
                                else
                                {
                                    ((UIElement3D)sender).[[evt.VirtualName]](e);
                                }
                            }
                        [[/inline]];
                }
                else
                {
                    invokation =
                        [[inline]]
                            // Command Manager will determine if preview or regular event.
                            CommandManager.[[evt.VirtualName]](sender, e);
                        [[/inline]];
                }

                // If the event is registered to be called back when already handled
                // or if a call to TranslateInput may have changed the state of e.Handled,
                // add a clause to skip invokation if e.Handled == true.
                if (evt.HandledToo || evt.TranslateInput)
                {
                    invokation = SkipIfHandled(invokation);
                }

                body.WriteBlock(invokation);

                if (evt.HandledToo)
                {
                    body.Write(
                        [[inline]]
                            // Always raise this "sub-event", but we pass along the handledness.
                            UIElement.CrackMouseButtonEventAndReRaiseEvent((DependencyObject)sender, e);
                        [[/inline]]);
                }

                cs.WriteBlock(
                    [[inline]]
                        /// <SecurityNote>
                        ///     Critical: This code can be used to spoof input
                        /// </SecurityNote>
                        [SecurityCritical]
                        private static void [[evt.ThunkName]](object sender, [[evt.ArgsType]] e)
                        {
                            [[body]]
                        }
                    [[/inline]]);
            }

            return cs.ToString();
        }

        private string SkipIfHandled(string body)
        {
            return
                [[inline]]
                    if(!e.Handled)
                    {
                        [[body]]
                    }
                [[/inline]];
        }

        private string WriteEvents(CGElement element)
        {
            StringCodeSink cs = new StringCodeSink();
            string accessModifier;

            if (element.Name.Equals("UIElement", StringComparison.InvariantCulture))
            {
                accessModifier = "protected";
            }
            else
            {
                accessModifier = "protected internal";
            }

            foreach(CGEvent evt in CG.Events)
            {
                // Skip commanding events.  These are forwarded to the CommandManager by the Thunk.
                if (evt.Commanding)
                {
                    continue;
                }

                // Declare or alias the routed event.
                if (evt.Owner == element.Name)
                {
                    // This element is the owner.  Declare the routed event.
                    cs.WriteBlock(
                        [[inline]]
                            /// <summary>
                            ///     Declaration of the routed event reporting [[evt.Comment]]
                            /// </summary>
                            public static readonly RoutedEvent [[evt.AliasedRoutedEventName]] = EventManager.RegisterRoutedEvent("[[evt.ClrEventName]]", RoutingStrategy.Direct, typeof([[evt.HandlerType]]), _typeofThis);
                        [[/inline]]);
                }
                else
                {
                    // Create an alias to the existing event.
                    cs.WriteBlock(
                        [[inline]]
                            /// <summary>
                            ///     Alias to the [[evt.RoutedEventName]].
                            /// </summary>
                            public static readonly RoutedEvent [[evt.AliasedRoutedEventName]] = [[evt.RoutedEventName]].AddOwner(_typeofThis);
                        [[/inline]]);
                }

                // Declare the CLR event wrapper and virtual method.
                cs.Write(
                    [[inline]]
                        /// <summary>
                        ///     Event reporting [[evt.Comment]]
                        /// </summary>
                    [[/inline]]);

                if (!String.IsNullOrEmpty(evt.CategoryID))
                {
                    cs.Write(
                        [[inline]]
                            [CustomCategory([[evt.CategoryID]])]
                        [[/inline]]);
                }

                cs.WriteBlock(
                    [[inline]]
                        public event [[evt.HandlerType]] [[evt.ClrEventName]]
                        {
                            add { AddHandler([[evt.RoutedEventName]], value, false); }
                            remove { RemoveHandler([[evt.RoutedEventName]], value); }
                        }

                        /// <summary>
                        ///     Virtual method reporting [[evt.Comment]]
                        /// </summary>
                        [[accessModifier]] virtual void [[evt.VirtualName]]([[evt.ArgsType]] e) {}
                    [[/inline]]);
            }

            return cs.ToString();
        }

        private string WriteCoreFlags(CGElement element)
        {
            return
                [[inline]]
                    internal bool ReadFlag(CoreFlags field)
                    {
                        return (_flags & field) != 0;
                    }

                    internal void WriteFlag(CoreFlags field,bool value)
                    {
                        if (value)
                        {
                             _flags |= field;
                        }
                        else
                        {
                             _flags &= (~field);
                        }
                    }

                    private CoreFlags       _flags;
                [[/inline]];
        }

        private string WriteProperties(CGElement element)
        {
            StringCodeSink cs = new StringCodeSink();

            foreach(CGProperty property in CG.Properties)
            {
                // Declare properties registered on this element.  Aliased properties are
                // taken care of in WriteRegisterProperties().
                if (property.Owner == element.Name)
                {
                    // This element is the owner.  Declare the property.
                    cs.WriteBlock(
                        [[inline]]
                            /// <summary>
                            ///     The key needed set a read-only property.
                            /// </summary>
                            internal static readonly DependencyPropertyKey [[property.PropertyName]]PropertyKey =
                                        DependencyProperty.RegisterReadOnly(
                                                    "[[property.PropertyName]]",
                                                    typeof([[property.Type]]),
                                                    _typeofThis,
                                                    [[WritePropertyMetadata(property)]]);
                        [[/inline]]);
                }

                cs.Write(
                    [[inline]]
                        /// <summary>
                        ///     The dependency property for the [[property.PropertyName]] property.
                        /// </summary>
                    [[/inline]]);

                if (property.Owner == element.Name)
                {
                    cs.WriteBlock(
                        [[inline]]
                            public static readonly DependencyProperty [[property.PropertyName]]Property =
                                [[property.PropertyName]]PropertyKey.DependencyProperty;
                        [[/inline]]);

                }
                else
                {
                    cs.WriteBlock(
                        [[inline]]
                            public static readonly DependencyProperty [[property.PropertyName]]Property = [[property.Name]]Property.AddOwner(_typeofThis);
                        [[/inline]]);
                }

                if (property.ChangedEvent)
                {
                    cs.WriteBlock(
                        [[inline]]
                            [[WritePropertyChangedEvent(element, property)]]
                        [[/inline]]);
                }
            }

            return cs.ToString();
        }

        private string WritePropertyMetadata(CGProperty property)
        {
            StringCodeSink cs = new StringCodeSink();

            cs.Write([[inline]][[property.DefaultValue]][[/inline]]);

            if (property.ChangedEvent && !property.ReverseInherit)
            {
                cs.WriteBlock(", // default value");
                cs.Write([[inline]]new PropertyChangedCallback([[property.PropertyName]]_Changed)[[/inline]]);
            }

            return
                [[inline]]new PropertyMetadata(
                                [[cs]])[[/inline]];
        }

        private string WritePropertyChangedEvent(CGElement element, CGProperty property)
        {
            Debug.Assert(property.ChangedEvent);

            StringCodeSink cs = new StringCodeSink();

            if (!property.ReverseInherit)
            {
                cs.WriteBlock(
                    [[inline]]
                        private static void [[property.PropertyName]]_Changed(DependencyObject d, DependencyPropertyChangedEventArgs e)
                        {
                            (([[element.Name]]) d).Raise[[property.PropertyName]]Changed(e);
                        }
                    [[/inline]]);
            }

            // Declare event keys registered on this element.
            if (property.Owner == element.Name)
            {
                // This element is the owner.  Declare the event key.
                cs.WriteBlock(
                    [[inline]]
                        /// <summary>
                        ///     [[property.PropertyName]]Changed private key
                        /// </summary>
                        internal static readonly EventPrivateKey [[property.PropertyName]]ChangedKey = new EventPrivateKey();

                    [[/inline]]);
            }

            cs.WriteBlock(
                [[inline]]
                    /// <summary>
                    ///     An event reporting that the [[property.PropertyName]] property changed.
                    /// </summary>
                    public event DependencyPropertyChangedEventHandler [[property.PropertyName]]Changed
                    {
                        add    { EventHandlersStoreAdd([[property.Owner]].[[property.PropertyName]]ChangedKey, value); }
                        remove { EventHandlersStoreRemove([[property.Owner]].[[property.PropertyName]]ChangedKey, value); }
                    }

                    /// <summary>
                    ///     An event reporting that the [[property.PropertyName]] property changed.
                    /// </summary>
                    protected virtual void On[[property.PropertyName]]Changed(DependencyPropertyChangedEventArgs e)
                    {
                    }

                    [[property.ReverseInherit ? "internal" : "private"]] void Raise[[property.PropertyName]]Changed(DependencyPropertyChangedEventArgs args)
                    {
                        // Call the virtual method first.
                        On[[property.PropertyName]]Changed(args);

                        // Raise the public event second.
                        RaiseDependencyPropertyChanged([[property.Owner]].[[property.PropertyName]]ChangedKey, args);
                    }
                [[/inline]]);

            return cs.ToString();
        }

        #endregion Private Methods
    }
}



