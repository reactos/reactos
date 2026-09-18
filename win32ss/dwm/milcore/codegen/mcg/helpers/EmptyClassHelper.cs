// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
// Description: Helpers to generate "Empty" classes for resources
//

namespace MS.Internal.MilCodeGen.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Xml;

    using MS.Internal.MilCodeGen.Runtime;
    using MS.Internal.MilCodeGen.ResourceModel;

    public partial class EmptyClassHelper : GeneratorMethods
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

        public static string WriteEmptyClass(McgResource resource)
        {
            string publicAnimatableProperties = String.Empty;
            string protectedAnimatableMethods = String.Empty;
            string createInstanceCore = String.Empty;

            createInstanceCore = 
                [[inline]]
                    /// <summary>
                    /// Implementation of <see cref="System.Windows.Freezable.CreateInstanceCore">Freezable.CreateInstanceCore</see>.
                    /// </summary>
                    /// <returns>The new Freezable.</returns>
                    protected override Freezable CreateInstanceCore()
                    {
                        return new [[resource.EmptyClassName]]();
                    }
                [[/inline]];

             // We don't need to implement CopyCore or CopyCurrentValueCore
             // because we aren't doing anything. The base class will
             // automatically be called.

            return
                [[inline]]
                    using System;
                    using System.Windows.Media.Animation;
                    using System.Windows.Media.Composition;

                    namespace [[resource.ManagedNamespace]]
                    {
                        // The equivalent of a null [[resource.Name]].
                        internal sealed partial class [[resource.EmptyClassName]] : [[resource.Name]], DUCE.IResource
                        {
                            //------------------------------------------------------
                            //
                            //  Constructors
                            //
                            //------------------------------------------------------

                            #region Construction

                            static [[resource.EmptyClassName]]()
                            {
                                // Create our singleton frozen instance
                                s_instance = new [[resource.EmptyClassName]]();
                                s_instance.Freeze();
                            }

                            // Private to prevent accidental creation of this singleton.
                            // Use the static Instance property.
                            private [[resource.EmptyClassName]](){}

                            #endregion Construction

                            //------------------------------------------------------
                            //
                            //  Public Properties
                            //
                            //------------------------------------------------------

                            //------------------------------------------------------
                            //
                            //  Protected Methods
                            //
                            //------------------------------------------------------

                            #region Protected Methods

                            [[createInstanceCore]]

                            [[protectedAnimatableMethods]]

                            #endregion Protected Methods

                            //------------------------------------------------------
                            //
                            //  Internal Methods
                            //
                            //------------------------------------------------------

                            #region Internal Methods

                            /// <summary>
                            /// AddRefOnChannelCore
                            /// </summary>
                            internal override DUCE.ResourceHandle AddRefOnChannelCore(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.

                                return DUCE.ResourceHandle.Null;
                            }

                            /// <summary>
                            /// AddRefOnChannel
                            /// </summary>
                            DUCE.ResourceHandle DUCE.IResource.AddRefOnChannel(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.

                                return DUCE.ResourceHandle.Null;
                            }

                            /// <summary>
                            /// ReleaseOnChannelCore
                            /// </summary>
                            internal override void ReleaseOnChannelCore(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.
                            }

                            /// <summary>
                            /// ReleaseOnChannel
                            /// </summary>
                            void DUCE.IResource.ReleaseOnChannel(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.
                            }

                            /// <summary>
                            /// GetHandleCore
                            /// </summary>
                            internal override DUCE.ResourceHandle GetHandleCore(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.

                                return DUCE.ResourceHandle.Null;
                            }

                            /// <summary>
                            /// GetHandle
                            /// </summary>
                            DUCE.ResourceHandle DUCE.IResource.GetHandle(DUCE.Channel channel)
                            {
                                // Empty classes have no impact on rendering so we avoid
                                // marshalling them.

                                return DUCE.ResourceHandle.Null;
                            }

                            #endregion Internal Methods

                            //------------------------------------------------------
                            //
                            //  Internal Properties
                            //
                            //------------------------------------------------------

                            #region Internal Properties

                            internal static [[resource.Name]] Instance
                            {
                                get { return s_instance; }
                            }

                            #endregion Internal Properties

                            //------------------------------------------------------
                            //
                            //  Private Fields
                            //
                            //------------------------------------------------------

                            #region Private Fields

                            private static readonly [[resource.EmptyClassName]] s_instance;

                            #endregion Private Fields
                        }
                    }

                [[/inline]];
        }

        #endregion Public Methods

    }
}




